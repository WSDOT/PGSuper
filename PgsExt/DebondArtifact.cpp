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
   m_FraDebondedStrands = 0;
   m_nDebondedStrands = 0;
   m_MaxDebondLength = 0;
   m_DebondLengthLimit =0;
   m_DebondLengthControl = pgsTypes::mdbDefault;
   m_MinDebondSectionSpacing = 0;
   m_DebondSectionSpacingLimit = 0;
}

pgsDebondArtifact::pgsDebondArtifact(const pgsDebondArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsDebondArtifact::~pgsDebondArtifact()
{
}

//======================== OPERATORS  =======================================
pgsDebondArtifact& pgsDebondArtifact::operator= (const pgsDebondArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
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

std::vector<bool> pgsDebondArtifact::GetIsExteriorStrandDebondedInRow() const
{
   return m_IsExteriorStrandDebonded;
}

void pgsDebondArtifact::AddIsExteriorStrandDebondedInRow(bool bExteriorStrandDebonded)
{
   m_IsExteriorStrandDebonded.push_back(bExteriorStrandDebonded);
}

void pgsDebondArtifact::AddMaxDebondStrandsAtSection(StrandIndexType nStrands,Float64 fra)
{
   m_nMaxDebondedAtSection = nStrands;
   m_fraMaxDebondedAtSection = fra;
}

void pgsDebondArtifact::GetMaxDebondStrandsAtSection(StrandIndexType* nStrands,Float64* fra) const
{
   *nStrands = m_nMaxDebondedAtSection;
   *fra      = m_fraMaxDebondedAtSection;
}

void pgsDebondArtifact::AddDebondSection(Float64 location,StrandIndexType nStrandsDebonded,Float64 fraStrandsDebonded)
{
   Section section;
   section.Location    = location;
   section.nDebonded   = nStrandsDebonded;
   section.fraDebonded = fraStrandsDebonded;

   m_Sections.push_back(section);
}

void pgsDebondArtifact::GetDebondSection(SectionIndexType idx,Float64* location,StrandIndexType* nStrandsDebonded,Float64* fraStrandsDebonded) const
{
   const Section& section = m_Sections[idx];
   *location           = section.Location;
   *nStrandsDebonded   = section.nDebonded;
   *fraStrandsDebonded = section.fraDebonded;
}

CollectionIndexType pgsDebondArtifact::GetNumDebondSections() const
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

//======================== ACCESS     =======================================

bool pgsDebondArtifact::Passed() const
{
   CollectionIndexType nSections = m_Sections.size();
   if (nSections==0)
   {
      return true;
   }

   bool bPassed = true;
   CollectionIndexType nRows = m_IsExteriorStrandDebonded.size();

   for ( CollectionIndexType row = 0; row < nRows; row++ )
   {
      bPassed &= RowPassed(row);
   }

   for ( CollectionIndexType section = 0; section < nSections; section++ )
   {
      bPassed &= SectionPassed(section);
   }

   const Float64 toler=1.0e-05;
   bPassed &= (m_MaxDebondLength <= m_DebondLengthLimit + toler);
   bPassed &= (m_MinDebondSectionSpacing >= m_DebondSectionSpacingLimit - toler);

   return bPassed;
}

bool pgsDebondArtifact::RowPassed(CollectionIndexType rowIndex) const
{
   bool bPassed = true;

   if ( m_MaxFraDebondedStrandsInRow[rowIndex] < m_FraDebondedStrandsInRow[rowIndex] )
   {
      bPassed = false;
   }

   if ( m_IsExteriorStrandDebonded[rowIndex] )
   {
      bPassed = false;
   }


   return bPassed;
}

bool pgsDebondArtifact::SectionPassed(CollectionIndexType sectionIndex) const
{
   StrandIndexType nMaxStrands2 = (StrandIndexType)floor(m_fraMaxDebondedAtSection * m_nDebondedStrands); // allow int to floor
   StrandIndexType nMaxStrands  = Max(m_nMaxDebondedAtSection,nMaxStrands2);

   return m_Sections[sectionIndex].nDebonded <= nMaxStrands;
}


//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsDebondArtifact::MakeCopy(const pgsDebondArtifact& rOther)
{
   m_MaxFraDebondedStrands      = rOther.m_MaxFraDebondedStrands;
   m_FraDebondedStrands         = rOther.m_FraDebondedStrands;
   m_nStrandsInRow              = rOther.m_nStrandsInRow;
   m_nDebondedStrandsInRow      = rOther.m_nDebondedStrandsInRow;
   m_FraDebondedStrandsInRow    = rOther.m_FraDebondedStrandsInRow;
   m_MaxFraDebondedStrandsInRow = rOther.m_MaxFraDebondedStrandsInRow;
   m_IsExteriorStrandDebonded   = rOther.m_IsExteriorStrandDebonded;
   m_Sections                   = rOther.m_Sections;
   m_nMaxDebondedAtSection      = rOther.m_nMaxDebondedAtSection;
   m_fraMaxDebondedAtSection    = rOther.m_fraMaxDebondedAtSection;
   m_nDebondedStrands           = rOther.m_nDebondedStrands;

   m_MaxDebondLength            = rOther.m_MaxDebondLength;
   m_DebondLengthLimit          = rOther.m_DebondLengthLimit;
   m_DebondLengthControl        = rOther.m_DebondLengthControl;
   m_MinDebondSectionSpacing    = rOther.m_MinDebondSectionSpacing;
   m_DebondSectionSpacingLimit  = rOther.m_DebondSectionSpacingLimit;
}

void pgsDebondArtifact::MakeAssignment(const pgsDebondArtifact& rOther)
{
   MakeCopy( rOther );
}
