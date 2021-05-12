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

#ifndef INCLUDED_SHEARCAPENG_H_
#define INCLUDED_SHEARCAPENG_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#include <Details.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IBroker;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsShearCapacityEngineer

   Encapsulates the computations of shear capacities


DESCRIPTION
   Encapsulates the computations of shear capacities

LOG
   rab : 12.10.1998 : Created file
*****************************************************************************/

class pgsShearCapacityEngineer
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsShearCapacityEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsShearCapacityEngineer(const pgsShearCapacityEngineer& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsShearCapacityEngineer();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsShearCapacityEngineer& operator = (const pgsShearCapacityEngineer& rOther);

   // GROUP: OPERATIONS

   void SetBroker(IBroker* pBroker);
   void SetStatusGroupID(StatusGroupIDType statusGroupID);

   // Shear capacity
   //------------------------------------------------------------------------
   void ComputeShearCapacity(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi,const GDRCONFIG* pConfig, SHEARCAPACITYDETAILS* pmcd) const;

   // concrete stress at girder centroid used to calculate shear capacity
   //------------------------------------------------------------------------
   void ComputeFpc(const pgsPointOfInterest& pPoi, const GDRCONFIG* pConfig, FPCDETAILS* mcd) const;

   //------------------------------------------------------------------------
   void ComputeVsReqd(const pgsPointOfInterest& poi, SHEARCAPACITYDETAILS* pscd) const;

   //------------------------------------------------------------------------
   // When the poi being evaluated is outboard of the critical section for shear
   // The Vu and Vc values are forced to be the same as at the critical section
   // Update the shear capacity details to reflect this. Update all derived data
   // that will be effected by this change
   void TweakShearCapacityOutboardOfCriticalSection(const pgsPointOfInterest& poiCS,SHEARCAPACITYDETAILS* pscd,const SHEARCAPACITYDETAILS* pscd_at_cs) const;

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsShearCapacityEngineer& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsShearCapacityEngineer& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;
   StatusCallbackIDType m_scidGirderDescriptionError;


   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   

   bool GetGeneralInformation(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, SHEARCAPACITYDETAILS* pscd) const;
   bool GetInformation(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, SHEARCAPACITYDETAILS* pscd) const;
   void ComputeShearCapacityDetails(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi,SHEARCAPACITYDETAILS* pmcd) const;

   bool ComputeVc(const pgsPointOfInterest& poi, SHEARCAPACITYDETAILS* pscd) const;
   bool ComputeVs(const pgsPointOfInterest& poi, SHEARCAPACITYDETAILS* pscd) const;
   bool ComputeVf(const pgsPointOfInterest& poi, SHEARCAPACITYDETAILS* pscd) const;

   void EvaluateStirrupRequirements(SHEARCAPACITYDETAILS* pscd) const;

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

#endif // INCLUDED_SHEARCAPENG_H_
