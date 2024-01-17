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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\DebondArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsDebondArtifact
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsDebondArtifact::pgsDebondArtifact()
{
   m_bCheckMaxFraDebondedStrands = false; // this is not a requirement in LRFD 9th Edition
   m_FraDebondedStrands = 0;
   m_nDebondedStrands = 0;
   m_MaxDebondLength = 0;
   m_DebondLengthLimit = 0;
   m_DebondLengthControl = pgsTypes::mdbDefault;
   m_MinDebondSectionSpacing = 0;
   m_DebondSectionSpacingLimit = 0;

   m_nMaxDebondedAtSection = 6;
   m_bCheckMaxFraDebondedStrandsPerSection = false; // this is not a requirement in LRFD 9th Edition
   m_fraMaxDebondedAtSection = 0.40;

   m_bCheckDebondingSymmetry = true;
   m_IsDebondingSymmetrical = DEBOND_SYMMETRY_FALSE;

   m_bCheckAdjacentDebonding = true;
   m_bCheckDebondingInWebWidthProjection = true;

   m_Section = I;
}

pgsDebondArtifact::~pgsDebondArtifact()
{
}

//======================== OPERATIONS =======================================
void pgsDebondArtifact::SetSection(pgsDebondArtifact::Section section)
{
   m_Section = section;
}

pgsDebondArtifact::Section pgsDebondArtifact::GetSection() const
{
   return m_Section;
}

void pgsDebondArtifact::CheckMaxFraDebondedStrands(bool bCheck)
{
   m_bCheckMaxFraDebondedStrands = bCheck;
}

bool pgsDebondArtifact::CheckMaxFraDebondedStrands() const
{
   return m_bCheckMaxFraDebondedStrands;
}

Float64 pgsDebondArtifact::GetMaxFraDebondedStrands() const
{
   return m_MaxFraDebondedStrands;
}

void pgsDebondArtifact::SetMaxFraDebondedStrands(Float64 fra)
{
   m_MaxFraDebondedStrands = fra;
}

Float64 pgsDebondArtifact::GetFraDebondedStrands() const
{
   return m_FraDebondedStrands;
}

void pgsDebondArtifact::SetFraDebondedStrands(Float64 fra)
{
   m_FraDebondedStrands = fra;
}

std::vector<Float64> pgsDebondArtifact::GetFraDebondedStrandsInRow() const
{
   return m_FraDebondedStrandsInRow;
}

void pgsDebondArtifact::AddFraDebondedStrandsInRow(Float64 fra)
{
   m_FraDebondedStrandsInRow.push_back(fra);
}

std::vector<StrandIndexType> pgsDebondArtifact::GetStrandCountInRow() const
{
   return m_nStrandsInRow;
}

void pgsDebondArtifact::AddNumStrandsInRow(StrandIndexType nStrandsInRow)
{
   m_nStrandsInRow.push_back(nStrandsInRow);
}

std::vector<StrandIndexType> pgsDebondArtifact::GetNumDebondedStrandsInRow() const
{
   return m_nDebondedStrandsInRow;
}

void pgsDebondArtifact::AddNumDebondedStrandsInRow(StrandIndexType nDebondStrandsInRow)
{
   m_nDebondedStrandsInRow.push_back(nDebondStrandsInRow);
}

std::vector<Float64> pgsDebondArtifact::GetMaxFraDebondedStrandsInRow() const
{
   return m_MaxFraDebondedStrandsInRow;
}

void pgsDebondArtifact::AddMaxFraDebondedStrandsInRow(Float64 maxFra)
{
   m_MaxFraDebondedStrandsInRow.push_back(maxFra);
}

const std::set<std::tuple<RowIndexType, pgsDebondArtifact::State, WebIndexType>>& pgsDebondArtifact::GetExteriorStrandBondState() const
{
   return m_ExteriorStrandBondState;
}

void pgsDebondArtifact::SetExtriorStrandBondState(RowIndexType rowIdx, pgsDebondArtifact::State state,WebIndexType webIdx)
{
   auto value = std::make_tuple(rowIdx, state, webIdx);
   ATLASSERT(m_ExteriorStrandBondState.find(value) == m_ExteriorStrandBondState.end()); // if this fires, we already have a bond state for this row
   m_ExteriorStrandBondState.emplace(value);
}

void pgsDebondArtifact::AddMaxDebondStrandsAtSection(StrandIndexType nStrands,bool bCheck,Float64 fra)
{
   m_nMaxDebondedAtSection = nStrands;
   m_bCheckMaxFraDebondedStrandsPerSection = bCheck;
   m_fraMaxDebondedAtSection = fra;
}

void pgsDebondArtifact::GetMaxDebondStrandsAtSection(StrandIndexType* nStrands,bool* pbCheck,Float64* fra) const
{
   *nStrands = m_nMaxDebondedAtSection;
   *pbCheck = m_bCheckMaxFraDebondedStrandsPerSection;
   *fra      = m_fraMaxDebondedAtSection;
}

void pgsDebondArtifact::AddDebondSection(Float64 location,StrandIndexType nStrandsDebonded,Float64 fraStrandsDebonded)
{
   m_Sections.emplace_back(location, nStrandsDebonded, fraStrandsDebonded);
}

void pgsDebondArtifact::GetDebondSection(SectionIndexType idx,Float64* location,StrandIndexType* nStrandsDebonded,Float64* fraStrandsDebonded) const
{
   const auto& section = m_Sections[idx];
   *location           = section.Location;
   *nStrandsDebonded   = section.nDebonded;
   *fraStrandsDebonded = section.fraDebonded;
}

IndexType pgsDebondArtifact::GetNumDebondSections() const
{
   return m_Sections.size();
}

void pgsDebondArtifact::SetNumDebondedStrands(StrandIndexType nStrands)
{
   m_nDebondedStrands = nStrands;
}

StrandIndexType pgsDebondArtifact::GetNumDebondedStrands() const
{
   return m_nDebondedStrands;
}

Float64 pgsDebondArtifact::GetMaxDebondLength() const
{
   return m_MaxDebondLength;
}

void pgsDebondArtifact::SetMaxDebondLength(Float64 length)
{
   m_MaxDebondLength = length;
}

void pgsDebondArtifact::GetDebondLengthLimit(Float64* length, pgsTypes::DebondLengthControl* control) const
{
   *length = m_DebondLengthLimit;
   *control = m_DebondLengthControl;
}

void pgsDebondArtifact::SetDebondLengthLimit(Float64 length, pgsTypes::DebondLengthControl control)
{
   m_DebondLengthLimit = length;
   m_DebondLengthControl = control;
}

Float64 pgsDebondArtifact::GetMinDebondSectionSpacing() const
{
   return m_MinDebondSectionSpacing;
}

void pgsDebondArtifact::SetMinDebondSectionSpacing(Float64 spacing)
{
   m_MinDebondSectionSpacing = spacing;
}

Float64 pgsDebondArtifact::GetDebondSectionSpacingLimit() const
{
   return m_DebondSectionSpacingLimit;
}

void pgsDebondArtifact::SetDebondSectionSpacingLimit(Float64 spacing)
{
   m_DebondSectionSpacingLimit = spacing;
}

void pgsDebondArtifact::CheckDebondingSymmetry(bool bCheck)
{
   m_bCheckDebondingSymmetry = bCheck;
}

bool pgsDebondArtifact::CheckDebondingSymmetry() const
{
   return m_bCheckDebondingSymmetry;
}

void pgsDebondArtifact::IsDebondingSymmetrical(Uint16 sym)
{
   m_IsDebondingSymmetrical = sym;
}

Uint16 pgsDebondArtifact::IsDebondingSymmetrical() const
{
   return m_IsDebondingSymmetrical;
}

void pgsDebondArtifact::CheckAdjacentDebonding(bool bCheck) 
{
   m_bCheckAdjacentDebonding = bCheck;
}

bool pgsDebondArtifact::CheckAdjacentDebonding() const
{
   return m_bCheckAdjacentDebonding;
}

void pgsDebondArtifact::AddAdjacentDebondedStrands(pgsTypes::MemberEndType endType, StrandIndexType strandIdx1, StrandIndexType strandIdx2)
{
   ATLASSERT(strandIdx1 != INVALID_INDEX);
   ATLASSERT(strandIdx2 != INVALID_INDEX);
   m_AdjacentStrands[endType].emplace_back(strandIdx1, strandIdx2);
}

IndexType pgsDebondArtifact::GetAdjacentDebondedStrandsCount(pgsTypes::MemberEndType endType) const
{
   return m_AdjacentStrands[endType].size();
}

void pgsDebondArtifact::GetAdjacentDebondedStrands(pgsTypes::MemberEndType endType, IndexType idx, StrandIndexType* pStrandIdx1, StrandIndexType* pStrandIdx2) const
{
   *pStrandIdx1 = m_AdjacentStrands[endType].at(idx).first;
   *pStrandIdx2 = m_AdjacentStrands[endType].at(idx).second;
}

bool pgsDebondArtifact::PassedAdjacentDebondedStrands(pgsTypes::MemberEndType endType) const
{
   if (m_bCheckAdjacentDebonding)
   {
      return 0 < m_AdjacentStrands[endType].size() ? false : true; // if there are adjacent debonded strand, this check fails
   }
   else
   {
      return true;
   }
}

bool pgsDebondArtifact::PassedAdjacentDebondedStrands() const
{
   return PassedAdjacentDebondedStrands(pgsTypes::metStart) && PassedAdjacentDebondedStrands(pgsTypes::metEnd);
}

void pgsDebondArtifact::SetBottomFlangeToWebWidthRatio(pgsTypes::MemberEndType endType,Float64 ratio)
{
   m_bf_bw_ratio[endType] = ratio;
}

Float64 pgsDebondArtifact::GetBottomFlangeToWebWidthRatio(pgsTypes::MemberEndType endType) const
{
   return m_bf_bw_ratio[endType];
}

void pgsDebondArtifact::CheckDebondingInWebWidthProjection(bool bCheck)
{
   m_bCheckDebondingInWebWidthProjection = bCheck;
}

bool pgsDebondArtifact::CheckDebondingInWebWidthProjection() const
{
   return m_bCheckDebondingInWebWidthProjection;
}

void pgsDebondArtifact::AddDebondedStrandInWebWidthProjection(pgsTypes::MemberEndType endType, StrandIndexType strandIdx)
{
   m_DebondedStrandsInWebWidthProjection[endType].push_back(strandIdx);
   std::sort(std::begin(m_DebondedStrandsInWebWidthProjection[endType]), std::end(m_DebondedStrandsInWebWidthProjection[endType]));
}

const std::vector<StrandIndexType>& pgsDebondArtifact::GetDebondedStrandsInWebWidthProjection(pgsTypes::MemberEndType endType) const
{
   return m_DebondedStrandsInWebWidthProjection[endType];
}

bool pgsDebondArtifact::PassedBondedStrandsInWebWidthProjection(pgsTypes::MemberEndType endType) const
{
   if (m_bCheckDebondingInWebWidthProjection)
   {
      return m_DebondedStrandsInWebWidthProjection[endType].size() == 0 ? true : false;
   }
   else
   {
      return true;
   }
}

bool pgsDebondArtifact::PassedBondedStrandsInWebWidthProjection() const
{
   return PassedBondedStrandsInWebWidthProjection(pgsTypes::metStart) && PassedBondedStrandsInWebWidthProjection(pgsTypes::metEnd);
}

//======================== ACCESS     =======================================
bool pgsDebondArtifact::PassedDebondLength() const
{
   return 0 < m_nDebondedStrands ? IsLE(m_MaxDebondLength,m_DebondLengthLimit) : true;
}

bool pgsDebondArtifact::PassedDebondTerminationSectionLocation() const
{
   return 0 < m_nDebondedStrands ? IsLE(m_DebondSectionSpacingLimit,m_MinDebondSectionSpacing) : true;
}

bool pgsDebondArtifact::PassedDebondingSymmetry() const
{
   return CheckDebondingSymmetry() ? m_IsDebondingSymmetrical != DEBOND_SYMMETRY_FALSE : true;
}

bool pgsDebondArtifact::Passed() const
{
   if (m_nDebondedStrands == 0)
   {
      return true;
   }

   IndexType nSections = m_Sections.size();
   if (nSections == 0)
   {
      return true;
   }

   // if any one element of the check fails, return false and be done with checking

   IndexType nRows = m_nStrandsInRow.size();
   for ( IndexType row = 0; row < nRows; row++ )
   {
      if (!RowPassed(row))
      {
         return false;
      }
   }

   for ( IndexType section = 0; section < nSections; section++ )
   {
      if (!SectionPassed(section))
      {
         return false;
      }
   }

   if (!PassedDebondingSymmetry() || !PassedDebondLength() || !PassedDebondTerminationSectionLocation() || !PassedAdjacentDebondedStrands() || !PassedBondedStrandsInWebWidthProjection())
   {
      return false;
   }

   return true;
}

bool pgsDebondArtifact::RowPassed(IndexType rowIndex) const
{
   bool bPassed = true;

   // if the fraction of debonded srands in this row exceeds the maximum permitted fraction
   // this check does not pass
   if ( m_MaxFraDebondedStrandsInRow[rowIndex] < m_FraDebondedStrandsInRow[rowIndex] )
   {
      bPassed = false;
   }

   // if the exterior strands in this row are in a debonded state this check does not pass
   // (it's ok if they are in a bonded or None state... LRFD 9th Edition 5.9.4.3.3, Item I, not all rows need
   // to have extrior strands bonded)
   for (const auto& item : m_ExteriorStrandBondState)
   {
      if (std::get<0>(item) == rowIndex && std::get<1>(item) == State::Debonded)
      {
         bPassed = false;
         break;
      }
   }

   return bPassed;
}

bool pgsDebondArtifact::SectionPassed(IndexType sectionIndex) const
{
   StrandIndexType nMaxStrands2 = (StrandIndexType)floor(m_fraMaxDebondedAtSection * m_nDebondedStrands); // allow int to floor
   StrandIndexType nMaxStrands  = m_bCheckMaxFraDebondedStrandsPerSection ? Max(m_nMaxDebondedAtSection,nMaxStrands2) : m_nMaxDebondedAtSection;

   return m_Sections[sectionIndex].nDebonded <= nMaxStrands;
}
