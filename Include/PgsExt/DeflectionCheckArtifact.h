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

#ifndef INCLUDED_PGSEXT_DEFLECTIONCHECKARTIFACT_H_
#define INCLUDED_PGSEXT_DEFLECTIONCHECKARTIFACT_H_

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
   pgsDeflectionCheckArtifact

   Artifact for checking optional deflection live load per lrfd


DESCRIPTION
   Artifact for checking optional deflection live load per lrfd


COPYRIGHT
   Copyright © 1997-2004
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 04.14.2004 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsDeflectionCheckArtifact
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsDeflectionCheckArtifact(SpanIndexType spanIdx = INVALID_INDEX);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsDeflectionCheckArtifact(const pgsDeflectionCheckArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsDeflectionCheckArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsDeflectionCheckArtifact& operator = (const pgsDeflectionCheckArtifact& rOther);

   // GROUP: OPERATIONS

   // GROUP: ACCESS

   void SetSpan(SpanIndexType spanIdx);
   SpanIndexType GetSpan() const;

   //------------------------------------------------------------------------
   void SetAllowableSpanRatio(Float64 AllowableSpanRatio);
   Float64 GetAllowableSpanRatio() const;

   void SetCapacity(Float64 capacity);
   Float64 GetCapacity() const;

   void SetDemand(Float64 min,Float64 max);
   void GetDemand(Float64* pMin,Float64* pMax) const;

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
   void MakeCopy(const pgsDeflectionCheckArtifact& rOther);

   //------------------------------------------------------------------------
   virtual void MakeAssignment(const pgsDeflectionCheckArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   bool m_bIsApplicable;
   Float64 m_MinDemand;// The min deflection along the girder (most negative)
   Float64 m_MaxDemand;// The max deflection along the girder (most positive)
   Float64 m_Capacity; // The allowable absolute deflection
   Float64 m_AllowableSpanRatio; // allowable as give by L/this

   SpanIndexType m_SpanIdx;

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

#endif // INCLUDED_PGSEXT_DEFLECTIONCHECKARTIFACT_H_
