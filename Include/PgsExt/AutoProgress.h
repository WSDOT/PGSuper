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

#ifndef INCLUDED_PGSEXT_AUTOPROGRESS_H_
#define INCLUDED_PGSEXT_AUTOPROGRESS_H_

#include <PgsExt\PgsExtExp.h>

interface IProgress;

/*****************************************************************************
CLASS 
   pgsAutoProgress

   Auto-pointer like class for the IProgress interface.


DESCRIPTION
   Auto-pointer like class for the IProgress interface.
   Code that creates progress windows must destroy them.  When exceptions
   are throw, the destroy code is often never called.  This class encapsulates
   the destroy code and is automatically called as the stack unwinds.

   You must create the progress window using the CreateProgressWindow
   on this class, instead of the IProgress pointer, to have the auto destory
   work properly.

EXAMPLE
   pgsAutoProgress ap(pProgress);
   ap.CreateWindow(PW_ALL,1000);
   // Don't do this  pProgress->CreateWindow(PW_ALL,1000);
   // Don't do this either pProgress->DestroyWindow


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 12.02.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsAutoProgress
{
public:
   //------------------------------------------------------------------------
   // Default constructor
   pgsAutoProgress(IProgress* pProgress);

   //------------------------------------------------------------------------
   // Destructor
   ~pgsAutoProgress();

   HRESULT CreateProgressWindow(DWORD dwMask,UINT nDelay);
   HRESULT Continue();

private:
   IProgress* m_pProgress; // weak reference
   bool m_bCreated;

   // Prevent accidental copying and assignment
   pgsAutoProgress(const pgsAutoProgress&);
   pgsAutoProgress& operator=(const pgsAutoProgress&);
};

#endif // INCLUDED_PGSEXT_AUTOPROGRESS_H_
