///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

class matPsStrand;

// Class to store direct-select strand fill input
// Only used for CStrandData::npsDirectSelection
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
class PGSEXTCLASS DirectStrandFillCollection 
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

   bool operator == (const DirectStrandFillCollection& rOther) const {
      return m_StrandFill == rOther.m_StrandFill; }

   bool operator != (const DirectStrandFillCollection& rOther) const {
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
   enum PermanentStrandType { npsTotal, npsStraightHarped, npsDirectSelection };
   PermanentStrandType NumPermStrandsType; // one of PermanentStrandType enum values
   // Note that the arrays with size 3 and4 below are indexed using pgsTypes::StrandType.
   // The pgsTypes::Permanent position is used when NumPermStrandsType==npsTotal.
   // When this is the case, values must be divided proportionally to straight and harped strands into 
   // the pgsTypes::Harped and pgsTypes::Straight strand locations because these are the values
   // used internally by the analysis and engineering agents
   StrandIndexType  Nstrands[4];
   Float64 Pjack[4];
   HarpedStrandOffsetType HsoEndMeasurement; // one of HarpedStrandOffsetType enums
   Float64 HpOffsetAtEnd;
   HarpedStrandOffsetType HsoHpMeasurement;  // one of HarpedStrandOffsetType enums
   Float64 HpOffsetAtHp;

   std::vector<CDebondData> Debond[3];
   bool bSymmetricDebond; // if true, left and right debond are the same (Only use Length1 of CDebondData struct)

   bool bPjackCalculated[4]; // true if Pjack was calculated
   Float64 LastUserPjack[4]; // Last Pjack entered by user

   pgsTypes::TTSUsage TempStrandUsage; // One of the tts constants above.

   const matPsStrand* StrandMaterial[3]; // pgsTypes::StrandType enum, except not using pgsTypes::Permanent

   // Grid index for extended strands. First array index is pgsTypes::StrandType (except don't use permanent)
   // second index is pgsTypes::MemberEndType
   std::vector<GridIndexType> NextendedStrands[3][2];
   bool bConvertExtendedStrands;
   friend CProjectAgentImp;

   // Strand fill when direct selection (npsDirectSelection) is used
   DirectStrandFillCollection m_StraightStrandFill;
   DirectStrandFillCollection m_HarpedStrandFill;
   DirectStrandFillCollection m_TemporaryStrandFill;

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
   // Fill using CStrandData::npsTotal
   void SetTotalPermanentNstrands(StrandIndexType nPermanent,StrandIndexType nStraight, StrandIndexType nHarped);
   // Fill using CStrandData::npsStraightHarped
   void SetHarpedStraightNstrands(StrandIndexType nStraight, StrandIndexType nHarped);
   void SetTemporaryNstrands(StrandIndexType nStrands);

   // Functions to fill selected strands directly (CStrandData::npsDirectSelection)
   // Filling indexes into library fill order
   void SetDirectStrandFillStraight(const DirectStrandFillCollection& rCollection);
   const DirectStrandFillCollection* GetDirectStrandFillStraight() const;
   void SetDirectStrandFillHarped(const DirectStrandFillCollection& rCollection);
   const DirectStrandFillCollection* GetDirectStrandFillHarped() const;
   void SetDirectStrandFillTemporary(const DirectStrandFillCollection& rCollection);
   const DirectStrandFillCollection* GetDirectStrandFillTemporary() const;

   // Get number of strands for any fill type
   StrandIndexType GetNstrands(pgsTypes::StrandType type) const;

   void AddExtendedStrand(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,GridIndexType gridIdx);
   const std::vector<GridIndexType>& GetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType) const;
   void SetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,const std::vector<GridIndexType>& extStrands);

   //------------------------------------------------------------------------
   // Resets all the prestressing input to default values.
   void ResetPrestressData();
   void ClearDirectFillData();
   void ClearExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType);

   void ClearDebondData();
   StrandIndexType GetDebondCount(pgsTypes::StrandType strandType) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress,Float64* pVersion);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   void MakeCopy(const CStrandData& rOther);
   virtual void MakeAssignment(const CStrandData& rOther);

   StrandIndexType ProcessDirectFillData(const DirectStrandFillCollection& rInCollection, DirectStrandFillCollection& rLocalCollection);
};
