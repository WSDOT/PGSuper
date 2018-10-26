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

#ifndef INCLUDED_PGSEXT_POIARTIFACTKEY_H_
#define INCLUDED_PGSEXT_POIARTIFACTKEY_H_

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
   pgsFlexuralStressArtifactKey

   A lookup key for POI based artifacts


DESCRIPTION
   A lookup key for POI based artifacts


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 11.17.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsFlexuralStressArtifactKey
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsFlexuralStressArtifactKey();

   //------------------------------------------------------------------------
   pgsFlexuralStressArtifactKey(pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType stress,Float64 distFromStart);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsFlexuralStressArtifactKey(const pgsFlexuralStressArtifactKey& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsFlexuralStressArtifactKey();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsFlexuralStressArtifactKey& operator = (const pgsFlexuralStressArtifactKey& rOther);

   bool operator<(const pgsFlexuralStressArtifactKey& rOther) const;

   pgsTypes::Stage GetStage() const { return m_Stage; }
   pgsTypes::LimitState GetLimitState() const { return m_LimitState; }
   pgsTypes::StressType GetStressType() const { return m_StressType; }
   Float64 GetDistFromStart() const { return m_DistFromStart; }

   // GROUP: OPERATIONS

   // GROUP: ACCESS

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsFlexuralStressArtifactKey& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsFlexuralStressArtifactKey& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   pgsTypes::Stage m_Stage;
   pgsTypes::LimitState m_LimitState;
   pgsTypes::StressType m_StressType;
   Float64 m_DistFromStart;

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


/*****************************************************************************
CLASS 
   pgsPoiArtifactKey

   A lookup key for POI based artifacts


DESCRIPTION
   A lookup key for POI based artifacts


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 11.17.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsPoiArtifactKey
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsPoiArtifactKey();

   //------------------------------------------------------------------------
   pgsPoiArtifactKey(pgsTypes::Stage stage,pgsTypes::LimitState ls,Float64 distFromStart);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsPoiArtifactKey(const pgsPoiArtifactKey& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsPoiArtifactKey();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsPoiArtifactKey& operator = (const pgsPoiArtifactKey& rOther);

   bool operator<(const pgsPoiArtifactKey& rOther) const;

   pgsTypes::Stage GetStage() const { return m_Stage; }
   pgsTypes::LimitState GetLimitState() const { return m_LimitState; }
   Float64 GetDistFromStart() const { return m_DistFromStart; }

   // GROUP: OPERATIONS

   // GROUP: ACCESS

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsPoiArtifactKey& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsPoiArtifactKey& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   pgsTypes::Stage m_Stage;
   pgsTypes::LimitState m_LimitState;
   Float64 m_DistFromStart;

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



/*****************************************************************************
CLASS 
   pgsFlexuralCapacityArtifactKey

   A lookup key for POI based artifacts


DESCRIPTION
   A lookup key for POI based artifacts


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 11.17.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsFlexuralCapacityArtifactKey : public pgsPoiArtifactKey
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsFlexuralCapacityArtifactKey();

   //------------------------------------------------------------------------
   pgsFlexuralCapacityArtifactKey(pgsTypes::Stage stage,pgsTypes::LimitState ls,Float64 distFromStart);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsFlexuralCapacityArtifactKey(const pgsFlexuralCapacityArtifactKey& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsFlexuralCapacityArtifactKey();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsFlexuralCapacityArtifactKey& operator = (const pgsFlexuralCapacityArtifactKey& rOther);


   // GROUP: OPERATIONS

   // GROUP: ACCESS

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsFlexuralCapacityArtifactKey& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsFlexuralCapacityArtifactKey& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
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



#endif // INCLUDED_PGSEXT_POIARTIFACTKEY_H_
