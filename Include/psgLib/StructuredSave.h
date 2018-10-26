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

#ifndef INCLUDED_STRUCTUREDSAVE_H_
#define INCLUDED_STRUCTUREDSAVE_H_

// SYSTEM INCLUDES
//
#include <WBFLCore.h>

// PROJECT INCLUDES
//
#if !defined INCLUDED_SYSTEM_ISTRUCTUREDSAVE_H_
#include <System\IStructuredSave.h>
#endif

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CStructuredSave

   Implements the sysIStructuredSave interface with the IStructuredSave
   interface.


DESCRIPTION
   Implements the sysIStructuredSave interface with the IStructuredSave
   interface.  

   

   This class is a HACK implementation to get the library manager and the
   PGSuper project file working together.  At this time,  it is too late to
   re-tool the library manager class.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 08.20.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS CStructuredSave : public sysIStructuredSave
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CStructuredSave(IStructuredSave* pStrSave);

   //------------------------------------------------------------------------
   // Destructor
   ~CStructuredSave();

   // GROUP: OPERATORS

   //------------------------------------------------------------------------
   // Mark the Beginning of a structured data chunk. This call must be always
   // balanced by a corresponding call to EndUnit. An optional version number
   // may be used to tag major units.
   virtual void BeginUnit(LPCTSTR name, Float64 version=0);

   //------------------------------------------------------------------------
   // Mark the end of a structured data chunk that was started by a call to 
   // BeginUnit.
   virtual void EndUnit();

   //------------------------------------------------------------------------
   // Get the version number of the current unit
   virtual Float64 GetVersion();

   //------------------------------------------------------------------------
   // Get the version number of the top-most unit
   virtual Float64 GetTopVersion();

   //------------------------------------------------------------------------
   // Get the version number of the parent to the current unit
   virtual Float64 GetParentVersion();

   virtual std::_tstring GetParentUnit();

   //------------------------------------------------------------------------
   // Write a string property
   virtual void Property(LPCTSTR name, LPCTSTR value);

   //------------------------------------------------------------------------
   // Write a real number property
   virtual void Property(LPCTSTR name, Float64 value);

   //------------------------------------------------------------------------
   // Write an integral property
   virtual void Property(LPCTSTR name, Int16 value);

   //------------------------------------------------------------------------
   // Write an unsigned integral property
   virtual void Property(LPCTSTR name, Uint16 value);

   //------------------------------------------------------------------------
   // Write an integral property
   virtual void Property(LPCTSTR name, Int32 value);

   //------------------------------------------------------------------------
   // Write an unsigned integral property
   virtual void Property(LPCTSTR name, Uint32 value);

   //------------------------------------------------------------------------
   // Write an integral property
   virtual void Property(LPCTSTR name, Int64 value);

   //------------------------------------------------------------------------
   // Write an unsigned integral property
   virtual void Property(LPCTSTR name, Uint64 value);

   //------------------------------------------------------------------------
   // Write an integral property
   virtual void Property(LPCTSTR name, LONG value);

   //------------------------------------------------------------------------
   // Write an unsigned integral property
   virtual void Property(LPCTSTR name, ULONG value);

   //------------------------------------------------------------------------
   // Write a bool property
   virtual void Property(LPCTSTR name, bool value);

   virtual void PutUnit(LPCTSTR xml);

   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   IStructuredSave* m_pStrSave;

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

#endif // INCLUDED_STRUCTUREDSAVE_H_
