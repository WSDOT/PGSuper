///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

/*****************************************************************************
CLASS 
   pgsDebondArtifact

   Code check artifact for debond requirements.


DESCRIPTION
   Code check artifact for debond requirements.


COPYRIGHT
   Copyright © 2003
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 06.11.2003 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsDebondArtifact
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsDebondArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsDebondArtifact(const pgsDebondArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsDebondArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsDebondArtifact& operator = (const pgsDebondArtifact& rOther);

   // GROUP: OPERATIONS
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

   std::vector<bool> GetIsExteriorStrandDebondedInRow() const;
   void AddIsExteriorStrandDebondedInRow(bool bExteriorStrandDebonded);

   void AddMaxDebondStrandsAtSection(StrandIndexType nStrands,Float64 fra);
   void GetMaxDebondStrandsAtSection(StrandIndexType* nStrands,Float64* fra) const;

   void AddDebondSection(Float64 location,StrandIndexType nStrandsDebonded,Float64 fraStrandsDebonded);
   void GetDebondSection(SectionIndexType idx,Float64* location,StrandIndexType* nStrandsDebonded,Float64* fraStrandsDebonded) const;
   CollectionIndexType GetNumDebondSections() const;

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

   // GROUP: ACCESS
   
   bool Passed() const;
   bool RowPassed(CollectionIndexType rowIndex) const;
   bool SectionPassed(CollectionIndexType sectionIndex) const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsDebondArtifact& rOther);

   //------------------------------------------------------------------------
   virtual void MakeAssignment(const pgsDebondArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   StrandIndexType m_nDebondedStrands;
   Float64 m_FraDebondedStrands;
   Float64 m_MaxFraDebondedStrands;
   std::vector<StrandIndexType>  m_nStrandsInRow;
   std::vector<StrandIndexType>  m_nDebondedStrandsInRow;
   std::vector<Float64> m_FraDebondedStrandsInRow;
   std::vector<Float64> m_MaxFraDebondedStrandsInRow;
   std::vector<bool>    m_IsExteriorStrandDebonded;

   struct Section
   {
      Float64 Location;
      StrandIndexType nDebonded;
      Float64 fraDebonded;
   };

   std::vector<Section> m_Sections;

   Float64 m_MaxDebondLength;
   Float64 m_DebondLengthLimit;
   pgsTypes::DebondLengthControl m_DebondLengthControl;

   Float64 m_MinDebondSectionSpacing;
   Float64 m_DebondSectionSpacingLimit;

   // Allowable
   StrandIndexType m_nMaxDebondedAtSection;
   Float64 m_fraMaxDebondedAtSection;
};

#endif // INCLUDED_PGSEXT_DEBONDARTIFACT_H_
