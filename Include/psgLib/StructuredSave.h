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
#include <System\IStructuredSave.h>

/*****************************************************************************
CLASS 
   CStructuredSave

   Implements the WBFL::System::IStructuredSave interface with the IStructuredSave
   interface.


DESCRIPTION
   Implements the WBFL::System::IStructuredSave interface with the IStructuredSave
   interface.  

   

   This class is a HACK implementation to get the library manager and the
   PGSuper project file working together.  At this time,  it is too late to
   re-tool the library manager class.

LOG
   rab : 08.20.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS CStructuredSave : public WBFL::System::IStructuredSave
{
public:
   CStructuredSave(::IStructuredSave* pStrSave);
   ~CStructuredSave();


   // Mark the Beginning of a structured data chunk. This call must be always
   // balanced by a corresponding call to EndUnit. An optional version number
   // may be used to tag major units.
   void BeginUnit(LPCTSTR name, Float64 version=0) override;

   // Mark the end of a structured data chunk that was started by a call to 
   // BeginUnit.
   void EndUnit() override;

   // Get the version number of the current unit
   Float64 GetVersion() override;

   // Get the version number of the top-most unit
   Float64 GetTopVersion() override;

   // Get the version number of the parent to the current unit
   virtual Float64 GetParentVersion();

   virtual std::_tstring GetParentUnit();

   // Write a string property
   void Property(LPCTSTR name, LPCTSTR value) override;

   // Write a real number property
   void Property(LPCTSTR name, Float64 value) override;

   // Write an integral property
   void Property(LPCTSTR name, Int16 value) override;

   // Write an unsigned integral property
   void Property(LPCTSTR name, Uint16 value) override;

   // Write an integral property
   void Property(LPCTSTR name, Int32 value) override;

   // Write an unsigned integral property
   void Property(LPCTSTR name, Uint32 value) override;

   // Write an integral property
   void Property(LPCTSTR name, Int64 value) override;

   // Write an unsigned integral property
   void Property(LPCTSTR name, Uint64 value) override;

   // Write an integral property
   void Property(LPCTSTR name, LONG value) override;

   // Write an unsigned integral property
   void Property(LPCTSTR name, ULONG value) override;

   // Write a bool property
   void Property(LPCTSTR name, bool value) override;

   virtual void PutUnit(LPCTSTR xml);

private:
   ::IStructuredSave* m_pStrSave;
};
