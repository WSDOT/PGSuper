///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#pragma once

#include "PsgLibLib.h"
#include <System\IStructuredLoad.h>

/*****************************************************************************
CLASS 
   CStructuredLoad

   Implements the WBFL::System::IStructuredLoad interface with the IStructuredLoad
   interface.


DESCRIPTION
   Implements the WBFL::System::IStructuredLoad interface with the IStructuredLoad
   interface.  

   

   This class is a HACK implementation to get the library manager and the
   PGSuper project file working together.  At this time,  it is too late to
   re-tool the library manager class.

LOG
   rab : 08.20.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS CStructuredLoad : public WBFL::System::IStructuredLoad
{
public:
   CStructuredLoad(::IStructuredLoad* pStrLoad);
   ~CStructuredLoad();

   // Check for the Beginning of a named structured data unit. If true is 
   // returned, the beginning of the unit was found and the file pointer is
   // advanced. If false is returned, the file pointer does not advance.
   // After a unit has been entered, GetVersion may be called to get its
   // version
   bool BeginUnit(LPCTSTR name) override;

   // Check for the end of a structured data chunk that was started by a call to 
   // BeginUnit.
   bool EndUnit() override;

   // Get the version number of the current unit
   Float64 GetVersion() override;

   // Get the version number of the parent to the current unit
   Float64 GetParentVersion() override;

   std::_tstring GetParentUnit() override;

   // Get the version number of the top-most unit
   Float64 GetTopVersion() override;

   // Property read routines. All of these calls try to read a property at the
   // current file pointer location. If the function returns true, the property
   // was read and the file pointer advances. If the function returns false,
   // the property was not at the current location and the file pointer does not
   // advance.
   // Read a string property
   bool Property(LPCTSTR name, std::_tstring* pvalue) override;

   // Read a real number property
   bool Property(LPCTSTR name, Float64* pvalue) override;

   // Read an integral property
   bool Property(LPCTSTR name, Int16* pvalue) override;

   // Read an unsigned integral property
   bool Property(LPCTSTR name, Uint16* pvalue) override;

   // Read an integral property
   bool Property(LPCTSTR name, Int32* pvalue) override;

   // Read an unsigned integral property
   bool Property(LPCTSTR name, Uint32* pvalue) override;

   // Read an integral property
   bool Property(LPCTSTR name, Int64* pvalue) override;

   // Read an unsigned integral property
   bool Property(LPCTSTR name, Uint64* pvalue) override;

   // Read an integral property
   bool Property(LPCTSTR name, LONG* pvalue) override;

   // Read an unsigned integral property
   bool Property(LPCTSTR name, ULONG* pvalue) override;

   // Read a bool property
   bool Property(LPCTSTR name, bool* pvalue) override;

   // Am I at the end of the "File"?
   bool Eof() const override;

   // Dump state as a text string into os. This is primarily to be used for
   // error handling.
   std::_tstring GetStateDump() const override;

   std::_tstring GetUnit() const override;

private:
   ::IStructuredLoad* m_pStrLoad;
};
