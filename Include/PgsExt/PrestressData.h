///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSLIB_PRESTRESSDATA_H_
#define INCLUDED_PGSLIB_PRESTRESSDATA_H_

// SYSTEM INCLUDES
//
#include <WBFLCore.h>

#if !defined INCLUDED_MATHEX_H_
#include <MathEx.h>
#endif

// PROJECT INCLUDES
//
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <StrData.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class CProjectAgentImp; // Only one privy to private conversion data

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CPrestressData

   Utility class for girder strand data.

DESCRIPTION

COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 04.08.2012 : Created file
*****************************************************************************/
// Method for describing number of permanent strands
#define NPS_TOTAL_NUMBER     0   // use total number and order as defined by library entry
#define NPS_STRAIGHT_HARPED  1   // use number of straight and number of harped.
#define NPS_DIRECT_SELECTION 2   // direct user selection of locations

// Class to store debonding input
class PGSEXTCLASS CDebondInfo
{
public:
   StrandIndexType strandTypeGridIdx; // Index of debonded strand (straight, harped, or temporary) in the GirderLibraryEntry strand grid
                             // the container this debond info is stored in defines the type of strand
   Float64 Length1; // debond length at left end of girder
   Float64 Length2; // debond length at right end of girder

   CDebondInfo():
      strandTypeGridIdx(INVALID_INDEX), Length1(0.0), Length2(0.0), needsConversion(false)
   {;}

	HRESULT Load(IStructuredLoad* pStrLoad);
	HRESULT Save(IStructuredSave* pStrSave);

   bool operator==(const CDebondInfo& rOther) const; 
   bool operator!=(const CDebondInfo& rOther) const;

friend CProjectAgentImp; // Only one privy to private conversion data
private:
   bool needsConversion; // Previous versions had two indices for strand data this can only be converted
                         // after we have access to the girder library
};


// Class to store direct-select strand fill input
// Only used for NPS_DIRECT_SELECTION
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

/////////////////////////////////////////////////////////////////////
class PGSEXTCLASS CPrestressData
{
public:
   // Public data members
   HarpedStrandOffsetType HsoEndMeasurement; // one of HarpedStrandOffsetType enums
   Float64 HpOffsetAtEnd;
   HarpedStrandOffsetType HsoHpMeasurement;  // one of HarpedStrandOffsetType enums
   Float64 HpOffsetAtHp;

   std::vector<CDebondInfo> Debond[3];
   bool bSymmetricDebond; // If true, left and right debond are the same (Only use Length1 of CDebondInfo struct)

   Float64 Pjack[4];
   bool bPjackCalculated[4]; // true if Pjack was calculated
   Float64 LastUserPjack[4]; // Last Pjack entered by user

   pgsTypes::TTSUsage TempStrandUsage; // One of the tts constants above.

// Strand number data must be accessed via member functions
private:
   int     NumPermStrandsType; // one of NPS_ above
   // # Filled strands when ordered fill sequences are used
   // Note that the arrays with size 3 and4 below are indexed using pgsTypes::StrandType.
   // The pgsTypes::Permanent position is used when NumPermStrandsType==NPS_TOTAL_NUMBER.
   // When this is the case, values must be divided proportionally to straight and harped strands into 
   // the pgsTypes::Harped and pgsTypes::Straight strand locations because these are the values
   // used internally by the analysis and engineering agents
   StrandIndexType  Nstrands[4];

   // Grid index for extended strands. First array index is pgsTypes::StrandType (except don't use permanent)
   // second index is pgsTypes::MemberEndType
   std::vector<GridIndexType> NextendedStrands[3][2];
   bool bConvertExtendedStrands;
   friend CProjectAgentImp;

   // Strand fill when direct selection (NPS_DIRECT_SELECTION) is used
   DirectStrandFillCollection m_StraightStrandFill;
   DirectStrandFillCollection m_HarpedStrandFill;
   DirectStrandFillCollection m_TemporaryStrandFill;

   pgsTypes::AdjustableStrandType m_AdjustableStrandType;

public:
   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // Constructor
   CPrestressData();
   //------------------------------------------------------------------------
   // Copy constructor
   CPrestressData(const CPrestressData& rOther);
   //------------------------------------------------------------------------
   // Destructor
   ~CPrestressData();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CPrestressData& operator = (const CPrestressData& rOther);
   bool operator == (const CPrestressData& rOther) const;
   bool operator != (const CPrestressData& rOther) const;

   // GROUP: OPERATIONS
   // Member functions for setting/getting number of strands

   // Returns one of the NPS_* variables defined above
   int GetNumPermStrandsType() const;

   // Functions to fill strands using permanent or harped/straight sequential fill order
   // NOTES:
   //    -No check is made here that np = ns + nh;
   //    -When changing from one fill type to another, YOU MUST SET THE NUMBER OR FILL OF STRANDS FIRST
   //     because a type change will reset all other strand data
   //
   // Fill using NPS_TOTAL_NUMBER
   void SetTotalPermanentNstrands(StrandIndexType nPermanent,StrandIndexType nStraight, StrandIndexType nHarped);
   // Fill using NPS_STRAIGHT_HARPED
   void SetHarpedStraightNstrands(StrandIndexType nStraight, StrandIndexType nHarped);
   void SetTemporaryNstrands(StrandIndexType nStrands);

   // Functions to fill selected strands directly (NPS_DIRECT_SELECTION)
   // Filling indexes into library fill order
   void SetDirectStrandFillStraight(const DirectStrandFillCollection& rCollection);
   const DirectStrandFillCollection* GetDirectStrandFillStraight() const;
   void SetDirectStrandFillHarped(const DirectStrandFillCollection& rCollection);
   const DirectStrandFillCollection* GetDirectStrandFillHarped() const;
   void SetDirectStrandFillTemporary(const DirectStrandFillCollection& rCollection);
   const DirectStrandFillCollection* GetDirectStrandFillTemporary() const;

   // Get number of strands for any fill type
   StrandIndexType GetNstrands(pgsTypes::StrandType type) const;

   // Adjustable strands can be straight or harped depending on girder library and project settings
   pgsTypes::AdjustableStrandType GetAdjustableStrandType() const;
   void SetAdjustableStrandType(pgsTypes::AdjustableStrandType type);

   void AddExtendedStrand(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,GridIndexType gridIdx);
   const std::vector<GridIndexType>& GetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType) const;
   void SetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,const std::vector<GridIndexType>& extStrands);

   //------------------------------------------------------------------------
   // Resets all the prestressing input to default values.
   void ResetPrestressData();
   void ClearDirectFillData();
   void ClearExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType);

   void ClearDebondData();

	HRESULT Load(IStructuredLoad* pStrLoad);
	HRESULT Save(IStructuredSave* pStrSave);

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   StrandIndexType ProcessDirectFillData(const DirectStrandFillCollection& rInCollection, DirectStrandFillCollection& rLocalCollection);

   //------------------------------------------------------------------------
   void MakeCopy(const CPrestressData& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CPrestressData& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
//   static HRESULT ShearProc(IStructuredSave*,IStructuredLoad*,IProgress*,CPrestressData*);
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PGSLIB_PRESTRESSDATA_H_
