///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSEXT_CONSTRUCTABILITYARTIFACT_H_
#define INCLUDED_PGSEXT_CONSTRUCTABILITYARTIFACT_H_

// SYSTEM INCLUDES
//

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
   pgsConstructabilityArtifact

   Artifact for constructability checks


DESCRIPTION
   Artifact for constructability checks.
   Constructability consists of
   1) "A" dimension check + warning if stirrups may be short
   2) Girder Global Rollover Stability for non-plumb girders


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 12.01.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsConstructabilityArtifact
{
public:
   // GROUP: ENUM
   enum SlabOffsetStatusType { Passed, Failed, Excessive, NA };

   struct GlobalGirderStabilityParameters
   {
      Float64 Wbottom; // bottom width of the girder
      Float64 Yb;      // distance from bottom to CG of noncomposite girder
      Float64 Orientation; // orientation of the girder
   };

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsConstructabilityArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsConstructabilityArtifact(const pgsConstructabilityArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsConstructabilityArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsConstructabilityArtifact& operator = (const pgsConstructabilityArtifact& rOther);

   // GROUP: OPERATIONS

   // GROUP: ACCESS

   //------------------------------------------------------------------------
   void SetRequiredSlabOffset(Float64 reqd);
   Float64 GetRequiredSlabOffset() const;

   void SetProvidedSlabOffset(Float64 provided);
   Float64 GetProvidedSlabOffset() const;

   void SetSlabOffsetApplicability(bool bSet);
   bool IsSlabOffsetApplicable() const;

   SlabOffsetStatusType SlabOffsetStatus() const;
   bool SlabOffsetPassed() const;

   // do stirrup lengths need to be checked because of the roadway geometry
   // (excessive haunch along the girder could lead to short stirrups in the field.
   //  special detailing may be required)
   void CheckStirrupLength(bool bCheck);
   bool CheckStirrupLength() const;

   void SetGlobalGirderStabilityApplicability(bool bSet);
   bool IsGlobalGirderStabilityApplicable() const;
   void SetGlobalGirderStabilityParameters(Float64 Wbottom,Float64 Ybottom,Float64 Orientation);
   void GetGlobalGirderStabilityParameters(Float64 *Wbottom,Float64 *Ybottom,Float64 *Orientation) const;
   Float64 GetMaxGirderIncline() const;
   bool GlobalGirderStabilityPassed() const;

   // Vector containing rows that have rebars outside of the girder section
   void SetRebarRowsOutsideOfSection(const std::vector<RowIndexType>& rows);
   std::vector<RowIndexType> GetRebarRowsOutsideOfSection() const;
   bool RebarGeometryCheckPassed() const; // fails if there are rebars in space

   bool Pass() const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsConstructabilityArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsConstructabilityArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   Float64 m_Provided; // The actual slab offset
   Float64 m_Required; // The required required slab offset
   bool m_bCheckStirrupLength;
   bool m_bIsSlabOffsetApplicable;

   bool m_bIsGlobalGirderStabilityApplicable;
   Float64 m_Wbottom;
   Float64 m_Ybottom;
   Float64 m_Orientation;

   // Rebar rows with bars outside of girder section
   std::vector<RowIndexType> m_RebarRowsOutsideSection;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PGSEXT_CONSTRUCTABILITYARTIFACT_H_
