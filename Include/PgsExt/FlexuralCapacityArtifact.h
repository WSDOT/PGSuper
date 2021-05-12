///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSEXT_FLEXURALCAPACITYARTIFACT_H_
#define INCLUDED_PGSEXT_FLEXURALCAPACITYARTIFACT_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <PgsExt\PointOfInterest.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsFlexuralCapacityArtifact

   Artifact for flexural capacity checks


DESCRIPTION
   Artifact for flexural capacity checks

LOG
   rab : 11.22.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsFlexuralCapacityArtifact
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsFlexuralCapacityArtifact(bool bPositiveMoment);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsFlexuralCapacityArtifact(const pgsFlexuralCapacityArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsFlexuralCapacityArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsFlexuralCapacityArtifact& operator = (const pgsFlexuralCapacityArtifact& rOther);

   // GROUP: OPERATIONS

   // GROUP: ACCESS
   void SetPointOfInterest(const pgsPointOfInterest& poi);
   const pgsPointOfInterest& GetPointOfInterest() const;

   //------------------------------------------------------------------------
   void SetMaxReinforcementRatio(Float64 cde);
   Float64 GetMaxReinforcementRatio() const;
   void SetMaxReinforcementRatioLimit(Float64 cdeMax);
   Float64 GetMaxReinforcementRatioLimit() const;
   void SetMinCapacity(Float64 MrMin);
   Float64 GetMinCapacity() const;
   void SetDemand(Float64 Mu);
   Float64 GetDemand() const;
   void SetCapacity(Float64 Mr);
   Float64 GetCapacity() const;

   bool IsOverReinforced() const;
   bool IsUnderReinforced() const;
   bool CapacityPassed() const;
   bool Passed() const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsFlexuralCapacityArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsFlexuralCapacityArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   pgsPointOfInterest m_Poi;
   bool m_bPositiveMoment;
   Float64 m_cde;
   Float64 m_cdeMax;
   Float64 m_MrMin;
   Float64 m_Mu;
   Float64 m_Mr;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

public:
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns true if the object is in a valid state, otherwise returns false.
   virtual bool AssertValid() const;

   //------------------------------------------------------------------------
   // Dumps the contents of the object to the given dump context.
   virtual void Dump(dbgDumpContext& os) const;
   #endif // _DEBUG

   #if defined _UNITTEST
   //------------------------------------------------------------------------
   // Runs a self-diagnostic test.  Returns true if the test passed,
   // otherwise false.
   static bool TestMe(dbgLog& rlog);
   #endif // _UNITTEST
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PGSEXT_FLEXURALCAPACITYARTIFACT_H_
