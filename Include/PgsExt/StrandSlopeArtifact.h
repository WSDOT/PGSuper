///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSEXT_STRANDSLOPEARTIFACT_H_
#define INCLUDED_PGSEXT_STRANDSLOPEARTIFACT_H_

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
   pgsStrandSlopeArtifact

   Artifact for prestressing strand slope checks


DESCRIPTION
   Artifact for prestressing strand slope checks

LOG
   rab : 11.22.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsStrandSlopeArtifact
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsStrandSlopeArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsStrandSlopeArtifact(const pgsStrandSlopeArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsStrandSlopeArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsStrandSlopeArtifact& operator = (const pgsStrandSlopeArtifact& rOther);

   // GROUP: OPERATIONS

   // GROUP: ACCESS

   //------------------------------------------------------------------------
   void SetCapacity(Float64 capacity);
   Float64 GetCapacity() const;

   void SetDemand(Float64 demand);
   Float64 GetDemand() const;

   void IsApplicable(bool bIsApplicable);
   bool IsApplicable() const;

   bool Passed() const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsStrandSlopeArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsStrandSlopeArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   bool m_bIsApplicable;
   Float64 m_Demand;   // The actual slope
   Float64 m_Capacity; // The allowable slope

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

#endif // INCLUDED_PGSEXT_STRANDSLOPEARTIFACT_H_
