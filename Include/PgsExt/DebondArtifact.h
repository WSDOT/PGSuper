///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSEXT_DEBONDARTIFACT_H_
#define INCLUDED_PGSEXT_DEBONDARTIFACT_H_

// SYSTEM INCLUDES
//
#include <vector>

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif


// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

#define DEBOND_SYMMETRY_FALSE 0
#define DEBOND_SYMMETRY_TRUE 1
#define DEBOND_SYMMETRY_NA 2

/*****************************************************************************
CLASS 
   pgsDebondArtifact

   Code check artifact for debond requirements.


DESCRIPTION
   Code check artifact for debond requirements.

LOG
   rab : 06.11.2003 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsDebondArtifact
{
public:
   // GROUP: LIFECYCLE
   enum State
   {
      Bonded, // strand is bonded
      Debonded, // strand is debonded
      None // state is not applicable to this strand
   };

   // enum that indicates the type of section and which restriction (I, J, or K) is applicable
   enum Section
   {
      I,
      J,
      K
   };

   pgsDebondArtifact();
   pgsDebondArtifact(const pgsDebondArtifact& rOther) = default;
   virtual ~pgsDebondArtifact();

   pgsDebondArtifact& operator= (const pgsDebondArtifact& rOther) = default;

   void SetSection(Section section);
   Section GetSection() const;

   void CheckMaxFraDebondedStrands(bool bCheck);
   bool CheckMaxFraDebondedStrands() const;
      
   Float64 GetMaxFraDebondedStrands() const;
   void SetMaxFraDebondedStrands(Float64 fra);

   Float64 GetFraDebondedStrands() const;
   void SetFraDebondedStrands(Float64 fra);

   std::vector<Float64> GetFraDebondedStrandsInRow() const;
   void AddFraDebondedStrandsInRow(Float64 fra);

   std::vector<StrandIndexType> GetStrandCountInRow() const;
   void AddNumStrandsInRow(StrandIndexType nStrandsInRow);

   std::vector<StrandIndexType> GetNumDebondedStrandsInRow() const;
   void AddNumDebondedStrandsInRow(StrandIndexType nDebondStrandsInRow);

   std::vector<Float64> GetMaxFraDebondedStrandsInRow() const;
   void AddMaxFraDebondedStrandsInRow(Float64 maxFra);

   const std::set<std::tuple<RowIndexType,State,WebIndexType>>& GetExteriorStrandBondState() const;
   void SetExtriorStrandBondState(RowIndexType rowIdx,State state,WebIndexType webIdx = INVALID_INDEX);

   void AddMaxDebondStrandsAtSection(StrandIndexType nStrands,bool bCheck,Float64 fra);
   void GetMaxDebondStrandsAtSection(StrandIndexType* nStrands,bool* pbCheck, Float64* fra) const;

   void AddDebondSection(Float64 location,StrandIndexType nStrandsDebonded,Float64 fraStrandsDebonded);
   void GetDebondSection(SectionIndexType idx,Float64* location,StrandIndexType* nStrandsDebonded,Float64* fraStrandsDebonded) const;
   IndexType GetNumDebondSections() const;

   StrandIndexType GetNumDebondedStrands() const;
   void SetNumDebondedStrands(StrandIndexType nStrands);

   Float64 GetMaxDebondLength() const;
   void SetMaxDebondLength(Float64 length);

   void GetDebondLengthLimit(Float64* length, pgsTypes::DebondLengthControl* control) const;
   void SetDebondLengthLimit(Float64 length, pgsTypes::DebondLengthControl control);

   Float64 GetMinDebondSectionSpacing() const;
   void SetMinDebondSectionSpacing(Float64 spacing);

   Float64 GetDebondSectionSpacingLimit() const;
   void SetDebondSectionSpacingLimit(Float64 spacing);

   // LRFD 9th Edition, 5.9.4.3.3 Restriction D
   void CheckDebondingSymmetry(bool bCheck);
   bool CheckDebondingSymmetry() const;
   void IsDebondingSymmetrical(Uint16 symm);
   Uint16 IsDebondingSymmetrical() const;

   // LRFD 9th Edition, 5.9.4.3.3, Restriction E
   void CheckAdjacentDebonding(bool bCheck);
   bool CheckAdjacentDebonding() const;
   void AddAdjacentDebondedStrands(pgsTypes::MemberEndType endType,StrandIndexType strandIdx1, StrandIndexType strandIdx2);
   IndexType GetAdjacentDebondedStrandsCount(pgsTypes::MemberEndType endType) const;
   void GetAdjacentDebondedStrands(pgsTypes::MemberEndType endType, IndexType idx, StrandIndexType* pStrandIdx1, StrandIndexType* pStrandIdx2) const;
   bool PassedAdjacentDebondedStrands(pgsTypes::MemberEndType endType) const;
   bool PassedAdjacentDebondedStrands() const;

   // LRFD 9th Edition, 5.9.4.3.3, Restriction I and J - strands must be bonded within web width projections
   void SetBottomFlangeToWebWidthRatio(pgsTypes::MemberEndType endType,Float64 ratio);
   Float64 GetBottomFlangeToWebWidthRatio(pgsTypes::MemberEndType endType) const; // only valid for Item I
   void CheckDebondingInWebWidthProjection(bool bCheck);
   bool CheckDebondingInWebWidthProjection() const;
   void AddDebondedStrandInWebWidthProjection(pgsTypes::MemberEndType endType, StrandIndexType strandIdx);
   const std::vector<StrandIndexType>& GetDebondedStrandsInWebWidthProjection(pgsTypes::MemberEndType endType) const;
   bool PassedBondedStrandsInWebWidthProjection(pgsTypes::MemberEndType endType) const;
   bool PassedBondedStrandsInWebWidthProjection() const;

   bool Passed() const;
   bool PassedDebondingSymmetry() const;
   bool PassedDebondLength() const;
   bool PassedDebondTerminationSectionLocation() const;
   bool RowPassed(IndexType rowIndex) const;
   bool SectionPassed(IndexType sectionIndex) const;

protected:

private:
   Section m_Section;

   bool m_bCheckMaxFraDebondedStrands;
   Float64 m_FraDebondedStrands;
   Float64 m_MaxFraDebondedStrands;

   StrandIndexType m_nDebondedStrands;
   std::vector<StrandIndexType>  m_nStrandsInRow;
   std::vector<StrandIndexType>  m_nDebondedStrandsInRow;
   std::vector<Float64> m_FraDebondedStrandsInRow;
   std::vector<Float64> m_MaxFraDebondedStrandsInRow;
   std::set<std::tuple<RowIndexType,State,WebIndexType>> m_ExteriorStrandBondState; // one entry per row

   struct DebondTerminationSection
   {
      DebondTerminationSection(Float64 l, StrandIndexType n, Float64 f) :Location(l), nDebonded(n), fraDebonded(f) {}
      Float64 Location;
      StrandIndexType nDebonded;
      Float64 fraDebonded;
   };

   std::vector<DebondTerminationSection> m_Sections;

   Float64 m_MaxDebondLength;
   Float64 m_DebondLengthLimit;
   pgsTypes::DebondLengthControl m_DebondLengthControl;

   Float64 m_MinDebondSectionSpacing;
   Float64 m_DebondSectionSpacingLimit;

   bool m_bCheckDebondingSymmetry;
   Uint16 m_IsDebondingSymmetrical;

   bool m_bCheckAdjacentDebonding;
   std::array<std::vector<std::pair<StrandIndexType, StrandIndexType>>, 2> m_AdjacentStrands;

   std::array<Float64, 2> m_bf_bw_ratio{-1,-1};
   bool m_bCheckDebondingInWebWidthProjection;
   std::array<std::vector<StrandIndexType>, 2> m_DebondedStrandsInWebWidthProjection;

   // Allowable
   StrandIndexType m_nMaxDebondedAtSection;
   bool m_bCheckMaxFraDebondedStrandsPerSection;
   Float64 m_fraMaxDebondedAtSection;
};

#endif // INCLUDED_PGSEXT_DEBONDARTIFACT_H_
