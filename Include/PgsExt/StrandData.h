///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////
#pragma once

#include <PgsExt\PgsExtExp.h>
#include <WBFLCore.h>
#include <StrData.h>

#include <PgsExt\DebondData.h>

#include <array>

class matPsStrand;
class GirderLibraryEntry;

// Class to store direct-select strand fill input
// Only used for pgsTypes::sdtDirectSelection
class PGSEXTCLASS CDirectStrandFillInfo
{
public:
   // Index into girder library's strand grid
   GridIndexType permStrandGridIdx;
   // Fill - this can be 0, 1, or 2 strands filled
   StrandIndexType numFilled; 

   CDirectStrandFillInfo():
      permStrandGridIdx(INVALID_INDEX), numFilled(INVALID_INDEX)
   {;}

   CDirectStrandFillInfo(GridIndexType index, StrandIndexType nfilled):
      permStrandGridIdx(index), numFilled(nfilled)
   {;}

	HRESULT Load(IStructuredLoad* pStrLoad);
	HRESULT Save(IStructuredSave* pStrSave) const;

   bool operator==(const CDirectStrandFillInfo& rOther) const; 
   bool operator!=(const CDirectStrandFillInfo& rOther) const;
};

// Container class to hold and process strand fill data.
// This container holds fill data in a compacted format (only filled strands are in it).
// It is also sorted by StrandIndex
class PGSEXTCLASS CDirectStrandFillCollection 
{
public:
   StrandIndexType GetFilledStrandCount() const; // total number of filled strands
   bool IsStrandFilled(GridIndexType indexGrid) const;
   StrandIndexType GetFillCountAtIndex(GridIndexType indexGrid) const; // number of strands filled at index (0,1,2)
   void RemoveFill(GridIndexType indexGrid); // Remove a filled strand
   void AddFill(const CDirectStrandFillInfo& rInfo);
   const CDirectStrandFillInfo& GetFill(CollectionIndexType fillNo) const;

   // stl behaviors
   void clear() {
      m_StrandFill.clear();}

   CollectionIndexType size() const {
      return m_StrandFill.size(); }

   void reserve(CollectionIndexType size) {
      m_StrandFill.reserve(size); }

   bool operator == (const CDirectStrandFillCollection& rOther) const {
      return m_StrandFill == rOther.m_StrandFill; }

   bool operator != (const CDirectStrandFillCollection& rOther) const {
      return m_StrandFill != rOther.m_StrandFill; }

   // allow const iteration
   typedef  std::vector<CDirectStrandFillInfo>::const_iterator  const_iterator;

   const_iterator begin() const {
      return m_StrandFill.begin();}

   const_iterator end() const {
      return m_StrandFill.end();}

private: 
   std::vector<CDirectStrandFillInfo> m_StrandFill;
};

class PGSEXTCLASS CStrandRow
{
public:
   CStrandRow();

   bool operator==(const CStrandRow& other) const;
   bool operator!=(const CStrandRow& other) const;
   bool operator<(const CStrandRow& other) const;

   pgsTypes::StrandType m_StrandType;

   Float64 m_Z; // location of individual strand in the cross section (sdtDirectStrandInput) or the spacing between strands that are on either side of the CL Beam (sdtDirectRowInput)
   Float64 m_Spacing; // spacing between all other strands (sdtDirectRowInput only)
   StrandIndexType m_nStrands; // total number of strands in the row. If an odd number, the center strand is on the CL of the beam and m_Z is ignored (sdtDirectRowInput only)

   // use one of the LOCATION_xxx constanst to access these arrays
   // Location of the strand measured from the top or bottom of the girder
   std::array<Float64,4> m_Y;
   std::array<pgsTypes::FaceType,4> m_Face;

   // Extended strand data
   // use the pgsTypes::MemberEndType constant to access the arrays in this class
   std::array<bool,2> m_bIsExtendedStrand;
   std::array<bool,2> m_bIsDebonded;
   std::array<Float64,2> m_DebondLength;

	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);
   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);

private:
   friend CStrandData;
   // In version 4 of the StrandRow data block, we removed the harp strand location information
   // in favor of using a single harp point definition in CStrandData. This eliminates the possiblity
   // of having many, many harping points. To deal with harp point locations in older files, we
   // save the definition in this static data member. The old definition is then converted to 
   // the new definition in CStrandData::Load. Do not use this data member for other purposes
   static std::array<std::vector<Float64>, 4> ms_oldHarpPoints;
};

typedef std::vector<CStrandRow> CStrandRowCollection;


/*****************************************************************************
CLASS 
   CStrandData

   Utility class for defining the input parameters for prestressing strands.

DESCRIPTION
   Utility class for defining the input parameters for prestressing strands.
   Input parameters include number of strands, harp strand geometry adjustments,
   jacking force, and temporary strand usage
*****************************************************************************/

class PGSEXTCLASS CStrandData
{
public:
   CStrandData();
   CStrandData(const CStrandData& rOther);
   ~CStrandData();

   CStrandData& operator = (const CStrandData& rOther);

   bool operator==(const CStrandData& rOther) const;
   bool operator!=(const CStrandData& rOther) const;

   // Functions to fill strands using permanent or harped/straight sequential fill order
   // NOTES:
   //    -No check is made here that np = ns + nh;
   //    -When changing from one fill type to another, YOU MUST SET THE NUMBER OR FILL OF STRANDS FIRST
   //     because a type change will reset all other strand data
   //
   // Fill using pgsTypes::sdtTotal
   void SetTotalPermanentNstrands(StrandIndexType nPermanent,StrandIndexType nStraight, StrandIndexType nHarped);
   // Fill using pgsTypes::sdtStraightHarped
   void SetHarpedStraightNstrands(StrandIndexType nStraight, StrandIndexType nHarped);
   void SetTemporaryNstrands(StrandIndexType nStrands);

   // Functions to fill selected strands directly (pgsTypes::sdtDirectSelection)
   // Filling indexes into library fill order
   void SetDirectStrandFillStraight(const CDirectStrandFillCollection& rCollection);
   const CDirectStrandFillCollection* GetDirectStrandFillStraight() const;
   void SetDirectStrandFillHarped(const CDirectStrandFillCollection& rCollection);
   const CDirectStrandFillCollection* GetDirectStrandFillHarped() const;
   void SetDirectStrandFillTemporary(const CDirectStrandFillCollection& rCollection);
   const CDirectStrandFillCollection* GetDirectStrandFillTemporary() const;

   // When strand rows or individual strands are used, we have to define the harp points here
   void SetHarpPoints(Float64 X0, Float64 X1, Float64 X2, Float64 X3);
   void GetHarpPoints(Float64* pX0, Float64* pX1, Float64* pX2, Float64* pX3) const;

   // Set/Get strand row data for row or individual strand input
   void AddStrandRow(const CStrandRow& strandRow);
   void AddStrandRows(const CStrandRowCollection& strandRows); // adds this collection to the current collection
   void SetStrandRows(const CStrandRowCollection& strandRows); // replaces the current collection this collection
   const CStrandRowCollection& GetStrandRows() const;
   const CStrandRow& GetStrandRow(RowIndexType rowIdx) const;
   // gets the strand row containing the specified strand
   bool GetStrandRow(pgsTypes::StrandType strandType,StrandIndexType strandIdx, const CStrandRow** ppRow) const;

   // Set number of strands for any fill except pgsTypes::sdtDirectRowInput or pgsTypes::sdtDirectStrandInput
   void SetStrandCount(pgsTypes::StrandType strandType,StrandIndexType nStrands);

   // Returns the number of strands
   StrandIndexType GetStrandCount(pgsTypes::StrandType strandType) const;

   // Adjustable strands can be straight or harped depending on girder library and project settings
   pgsTypes::AdjustableStrandType GetAdjustableStrandType() const;
   void SetAdjustableStrandType(pgsTypes::AdjustableStrandType type);

   // Strand extension paramaters (not used if using pgsTypes::sdtDirectRowInput)
   void AddExtendedStrand(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,GridIndexType gridIdx);
   const std::vector<GridIndexType>& GetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType) const;
   std::vector<GridIndexType>& GetExtendedStrands(pgsTypes::StrandType strandType, pgsTypes::MemberEndType endType);
   void SetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,const std::vector<GridIndexType>& extStrands);
   bool IsExtendedStrand(pgsTypes::StrandType strandType,GridIndexType gridIdx,pgsTypes::MemberEndType endType) const;
   StrandIndexType GetExtendedStrandCount(pgsTypes::StrandType strandType, pgsTypes::MemberEndType endType) const;

   //------------------------------------------------------------------------
   // Resets all the prestressing input to default values.
   void ResetPrestressData();
   void ClearDirectFillData();
   void ClearExtendedStrands();
   void ClearExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType);

   void ClearDebondData();
   StrandIndexType GetDebondCount(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,const GirderLibraryEntry* pGirderLibEntry) const;
   void SetDebonding(pgsTypes::StrandType strandType,const std::vector<CDebondData>& vDebond);
   const std::vector<CDebondData>& GetDebonding(pgsTypes::StrandType strandType) const;
   std::vector<CDebondData>& GetDebonding(pgsTypes::StrandType strandType);
   bool IsSymmetricDebond() const;
   void IsSymmetricDebond(bool bIsSymmetric);
   bool IsDebonded(pgsTypes::StrandType strandType,GridIndexType gridIdx,pgsTypes::MemberEndType endType,Float64* pLdebond) const;

   void SetStrandMaterial(pgsTypes::StrandType strandType,const matPsStrand* pStrandMaterial);
   const matPsStrand* GetStrandMaterial(pgsTypes::StrandType strandType) const;

   void IsPjackCalculated(pgsTypes::StrandType strandType,bool bIsCalculated);
   bool IsPjackCalculated(pgsTypes::StrandType strandType) const;
   void SetPjack(pgsTypes::StrandType strandType,Float64 Pjack);
   Float64 GetPjack(pgsTypes::StrandType strandType) const;
   void SetLastUserPjack(pgsTypes::StrandType strandType,Float64 Pjack);
   Float64 GetLastUserPjack(pgsTypes::StrandType strandType) const;

   void SetHarpStrandOffsetMeasurementAtEnd(HarpedStrandOffsetType offsetType);
   HarpedStrandOffsetType GetHarpStrandOffsetMeasurementAtEnd() const;

   void SetHarpStrandOffsetAtEnd(pgsTypes::MemberEndType endType,Float64 offset);
   Float64 GetHarpStrandOffsetAtEnd(pgsTypes::MemberEndType endType) const;

   void SetHarpStrandOffsetMeasurementAtHarpPoint(HarpedStrandOffsetType offsetType);
   HarpedStrandOffsetType GetHarpStrandOffsetMeasurementAtHarpPoint() const;

   void SetHarpStrandOffsetAtHarpPoint(pgsTypes::MemberEndType endType,Float64 offset);
   Float64 GetHarpStrandOffsetAtHarpPoint(pgsTypes::MemberEndType endType) const;

   void SetTemporaryStrandUsage(pgsTypes::TTSUsage ttsUsage);
   pgsTypes::TTSUsage GetTemporaryStrandUsage() const;

   void SetStrandDefinitionType(pgsTypes::StrandDefinitionType permStrandsType);
   pgsTypes::StrandDefinitionType GetStrandDefinitionType() const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress,Float64* pVersion);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

#if defined _DEBUG
   void AssertValid();
#endif

protected:
   void MakeCopy(const CStrandData& rOther);
   void MakeAssignment(const CStrandData& rOther);

   StrandIndexType ProcessDirectFillData(const CDirectStrandFillCollection& rInCollection, CDirectStrandFillCollection& rLocalCollection);
   void ProcessStrandRowData();

   pgsTypes::StrandDefinitionType m_NumPermStrandsType; // one of StrandDefinitionType enum values
   // Note that the arrays with size 3 and 4 below are indexed using pgsTypes::StrandType.
   // The pgsTypes::Permanent position is used when NumPermStrandsType==sdtTotal.
   // When this is the case, values must be divided proportionally to straight and harped strands into 
   // the pgsTypes::Harped and pgsTypes::Straight strand locations because these are the values
   // used internally by the analysis and engineering agents
   std::array<StrandIndexType,4> m_Nstrands;
   std::array<Float64,4> m_Pjack;
   HarpedStrandOffsetType m_HsoEndMeasurement; // one of HarpedStrandOffsetType enums
   std::array<Float64,2> m_HpOffsetAtEnd; // access array with pgsTypes::MemberEndType
   HarpedStrandOffsetType m_HsoHpMeasurement;  // one of HarpedStrandOffsetType enums
   std::array<Float64,2> m_HpOffsetAtHp; // access array with pgsTypes::MemberEndType

   std::array<std::vector<CDebondData>, 3> m_Debond;
   bool m_bSymmetricDebond; // if true, left and right debond are the same (Only use Length1 of CDebondData struct)

   std::array<bool,4> m_bPjackCalculated; // true if Pjack was calculated
   std::array<Float64,4> m_LastUserPjack; // Last Pjack entered by user

   pgsTypes::TTSUsage m_TempStrandUsage; // One of the tts constants above.

   std::array<const matPsStrand*,3> m_StrandMaterial; // pgsTypes::StrandType enum, except not using pgsTypes::Permanent

   // Grid index for extended strands. First array index is pgsTypes::StrandType (except don't use permanent)
   // second index is pgsTypes::MemberEndType
   std::array<std::array<std::vector<GridIndexType>,2>,3> m_NextendedStrands;
   bool m_bConvertExtendedStrands;
   friend CProjectAgentImp;

   pgsTypes::AdjustableStrandType m_AdjustableStrandType;

   // Strand fill when direct selection (sdtDirectSelection) is used
   CDirectStrandFillCollection m_StraightStrandFill;
   CDirectStrandFillCollection m_HarpedStrandFill;
   CDirectStrandFillCollection m_TemporaryStrandFill;

   // Strand information when user defined strands (sdtDirectRowInput or sdtDirectStrandInput) is used

   // Location of the harp strand deflection point measured from the left end of the girder
   // if this segment is cantilevered over the start/end abutment of the bridge, 
   // m_HarpPoint[ZoneBreakType::Start] and m_HarpPoint[ZoneBreakType::End] is the distance from the start/end
   // of the girder to the point where the strands transition to the harp points at m_HarpPoint[ZoneBreakType::LeftBreak]
   // and m_HarpPoint[ZoneBreakType::RightBreak]. This effectively creates four harp points
   //
   //  ____________                         _____________
   //              \                       /
   //               \                     /
   //                \___________________/
   //
   //            X0  X1                 X2  X3
   std::array<Float64, 4> m_HarpPoint; // use one of the LOCATION_xxx constants for the array index
   CStrandRowCollection m_StrandRows;
};
