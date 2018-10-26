///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#ifndef INCLUDED_PGSEXT_STRANDSTRESSARTIFACT_H_
#define INCLUDED_PGSEXT_STRANDSTRESSARTIFACT_H_

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
   pgsStrandStressArtifact

   Artifact for strand stress checks.


DESCRIPTION
   Artifact for strand stress checks.  Records the demand, capacity, and
   pass/fail status of prestressing strand stress checks.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 10.28.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsStrandStressArtifact
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsStrandStressArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsStrandStressArtifact(const pgsStrandStressArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsStrandStressArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsStrandStressArtifact& operator = (const pgsStrandStressArtifact& rOther);

   // GROUP: OPERATIONS

   // GROUP: ACCESS

   void SetPointOfInterest(const pgsPointOfInterest& poi);
   pgsPointOfInterest GetPointOfInterest() const;

   //------------------------------------------------------------------------
   // Sets the data for a strand stress check at jacking.  Causes 
   // IsCheckAtJackingApplicable() to return true.
   void SetCheckAtJacking(pgsTypes::StrandType strandType,Float64 demand,Float64 capacity);

   //------------------------------------------------------------------------
   // Sets the data for a strand stress check immediately prior to prestress
   // transfer.  Causes IsCheckBeforeXferApplicable() to return true.
   void SetCheckBeforeXfer(pgsTypes::StrandType strandType,Float64 demand,Float64 capacity);

   //------------------------------------------------------------------------
   // Sets the data for a strand stress check immediately after to prestress
   // transfer.  Causes IsCheckAfterXferApplicable() to return true.
   void SetCheckAfterXfer(pgsTypes::StrandType strandType,Float64 demand,Float64 capacity);

   //------------------------------------------------------------------------
   // Sets the data for a strand stress check after all prestress losses.
   // Causes IsCheckAfterLossesApplicable() to return true.
   void SetCheckAfterLosses(pgsTypes::StrandType strandType,Float64 demand,Float64 capacity);

   //------------------------------------------------------------------------
   // Retreive the strand stress check data at jacking.
   void GetCheckAtJacking(pgsTypes::StrandType strandType,Float64* pDemand,Float64* pCapacity,bool* pbPassed) const;

   //------------------------------------------------------------------------
   // Retreive the strand stress check data before prestress transfer.
   void GetCheckBeforeXfer(pgsTypes::StrandType strandType,Float64* pDemand,Float64* pCapacity,bool* pbPassed) const;

   //------------------------------------------------------------------------
   // Retreive the strand stress check data after prestress transfer.
   void GetCheckAfterXfer(pgsTypes::StrandType strandType,Float64* pDemand,Float64* pCapacity,bool* pbPassed) const;

   //------------------------------------------------------------------------
   // Retreive the strand stress check data after all prestress losses.
   void GetCheckAfterLosses(pgsTypes::StrandType strandType,Float64* pDemand,Float64* pCapacity,bool* pbPassed) const;

   // GROUP: INQUIRY

   //------------------------------------------------------------------------
   // Returns true if the stress in the prestressing strands were checked
   // at jacking.
   bool IsCheckAtJackingApplicable(pgsTypes::StrandType strandType) const;

   //------------------------------------------------------------------------
   // Returns true if the stress in the prestressing strands were checked
   // before prestress transfer.
   bool IsCheckBeforeXferApplicable(pgsTypes::StrandType strandType) const;

   //------------------------------------------------------------------------
   // Returns true if the stress in the prestressing strands were checked
   // after prestress transfer.
   bool IsCheckAfterXferApplicable(pgsTypes::StrandType strandType) const;

   //------------------------------------------------------------------------
   // Returns true if the stress in the prestressing strands were checked
   // after all prestress losses.
   bool IsCheckAfterLossesApplicable(pgsTypes::StrandType strandType) const;

   bool Passed(pgsTypes::StrandType strandType) const;
   bool Passed() const;

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsStrandStressArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsStrandStressArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   pgsPointOfInterest m_POI;

   Float64 m_Stress[4][4][2];
   // Index 0 = perm/temp strand (0 = straight, 1 = harped, 2 = temporary, 3 = permanent)
   // Index 1 = Stress type (0 = jacking, 1 = befer xfer, 2 = after xfer, 3 = after losses)
   // Index 2 = Stress magnitude (0 = capacity, 1 = demand)

   bool m_bIsApplicable[4][4];
   // Index 0 = perm/temp strand (0 = straight, 1 = harped, 2 = temporary, 3 = permanent)
   // Index 1 = Stress type (0 = jacking, 1 = befer xfer, 2 = after xfer, 3 = after losses)

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

#endif // INCLUDED_PGSEXT_STRANDSTRESSARTIFACT_H_
