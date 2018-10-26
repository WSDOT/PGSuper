///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#ifndef INCLUDED_STRUCTUREDLOAD_H_
#define INCLUDED_STRUCTUREDLOAD_H_

#include "psgLibLib.h"

// SYSTEM INCLUDES
//
#include <WBFLCore.h>

// PROJECT INCLUDES
//
#if !defined INCLUDED_SYSTEM_ISTRUCTUREDLOAD_H_
#include <System\IStructuredLoad.h>
#endif

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CStructuredLoad

   Implements the sysIStructuredLoad interface with the IStructuredLoad
   interface.


DESCRIPTION
   Implements the sysIStructuredLoad interface with the IStructuredLoad
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

class PSGLIBCLASS CStructuredLoad : public sysIStructuredLoad
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CStructuredLoad(IStructuredLoad* pStrLoad);

   //------------------------------------------------------------------------
   // Destructor
   ~CStructuredLoad();

   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Check for the Beginning of a named structured data unit. If true is 
   // returned, the beginning of the unit was found and the file pointer is
   // advanced. If false is returned, the file pointer does not advance.
   // After a unit has been entered, GetVersion may be called to get its
   // version
   virtual bool BeginUnit(LPCTSTR name);

   //------------------------------------------------------------------------
   // Check for the end of a structured data chunk that was started by a call to 
   // BeginUnit.
   virtual bool EndUnit();

   //------------------------------------------------------------------------
   // Get the version number of the current unit
   virtual double GetVersion();

   //------------------------------------------------------------------------
   // Get the version number of the parent to the current unit
   virtual double GetParentVersion();

   virtual std::_tstring GetParentUnit();

   //------------------------------------------------------------------------
   // Get the version number of the top-most unit
   virtual double GetTopVersion();

   //------------------------------------------------------------------------
   // Property read routines. All of these calls try to read a property at the
   // current file pointer location. If the function returns true, the property
   // was read and the file pointer advances. If the function returns false,
   // the property was not at the current locaton and the file pointer does not
   // advance.
   // Read a string property
   virtual bool Property(LPCTSTR name, std::_tstring* pvalue);

   //------------------------------------------------------------------------
   // Read a real number property
   virtual bool Property(LPCTSTR name, Float64* pvalue);

   //------------------------------------------------------------------------
   // Read an integral property
   virtual bool Property(LPCTSTR name, Int16* pvalue);

   //------------------------------------------------------------------------
   // Read an unsigned integral property
   virtual bool Property(LPCTSTR name, Uint16* pvalue);

   //------------------------------------------------------------------------
   // Read an integral property
   virtual bool Property(LPCTSTR name, Int32* pvalue);

   //------------------------------------------------------------------------
   // Read an unsigned integral property
   virtual bool Property(LPCTSTR name, Uint32* pvalue);

   //------------------------------------------------------------------------
   // Read a bool property
   virtual bool Property(LPCTSTR name, bool* pvalue);

   //------------------------------------------------------------------------
   // Am I at the end of the "File"?
   virtual bool Eof()const;

   //------------------------------------------------------------------------
   // Dump state as a text string into os. This is primarily to be used for
   // error handling.
   virtual std::_tstring GetStateDump() const;

   virtual std::_tstring GetUnit() const;

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
   IStructuredLoad* m_pStrLoad;

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

#endif // INCLUDED_STRUCTUREDLOAD_H_
