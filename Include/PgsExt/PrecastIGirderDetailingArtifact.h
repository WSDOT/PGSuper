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

#ifndef INCLUDED_PGSEXT_PRECASTIGIRDERDETAILINGARTIFACT_H_
#define INCLUDED_PGSEXT_PRECASTIGIRDERDETAILINGARTIFACT_H_

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
   pgsPrecastIGirderDetailingArtifact

   Artifact precast I girder detailing checks


DESCRIPTION
   Artifact precast I girder detailing checks


COPYRIGHT
   Copyright © 1997-1999
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.10.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsPrecastIGirderDetailingArtifact
{
public:
   // GROUP: ENUM
   enum StatusType { Pass, Fail};

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsPrecastIGirderDetailingArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsPrecastIGirderDetailingArtifact(const pgsPrecastIGirderDetailingArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsPrecastIGirderDetailingArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsPrecastIGirderDetailingArtifact& operator = (const pgsPrecastIGirderDetailingArtifact& rOther);

   // GROUP: OPERATIONS

   // GROUP: ACCESS

   //------------------------------------------------------------------------
   // Min top flange thickness
   Float64 GetMinTopFlangeThickness() const;
   void SetMinTopFlangeThickness(Float64 thick);

   //------------------------------------------------------------------------
   // Min web thickness
   Float64 GetMinWebThickness() const;
   void SetMinWebThickness(Float64 thick);

   //------------------------------------------------------------------------
   // Min Bottom flange thickness
   Float64 GetMinBottomFlangeThickness() const;
   void SetMinBottomFlangeThickness(Float64 thick);

   //------------------------------------------------------------------------
   // Provided top flange thickness
   Float64 GetProvidedTopFlangeThickness() const;
   void SetProvidedTopFlangeThickness(Float64 thick);

   //------------------------------------------------------------------------
   // Provided web thickness
   Float64 GetProvidedWebThickness() const;
   void SetProvidedWebThickness(Float64 thick);

   //------------------------------------------------------------------------
   // Provided Bottom flange thickness
   Float64 GetProvidedBottomFlangeThickness() const;
   void SetProvidedBottomFlangeThickness(Float64 thick);

   bool Passed() const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsPrecastIGirderDetailingArtifact& rOther);

   //------------------------------------------------------------------------
   virtual void MakeAssignment(const pgsPrecastIGirderDetailingArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   Float64 m_MinTopFlangeThickness;
   Float64 m_MinWebThickness;
   Float64 m_MinBottomFlangeThickness;

   Float64 m_ProvidedTopFlangeThickness;
   Float64 m_ProvidedWebThickness;
   Float64 m_ProvidedBottomFlangeThickness;

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

#endif // INCLUDED_PGSEXT_PRECASTIGIRDERDETAILINGARTIFACT_H_
