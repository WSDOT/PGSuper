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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\AutoProgress.h>
#include <WBFLCore.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsAutoProgress
****************************************************************************/

pgsAutoProgress::pgsAutoProgress(IProgress* pProgress)
{
   m_pProgress = pProgress;

   m_bCreated = false;

   CreateProgressWindow(PW_NOCANCEL | PW_NOGAUGE, 100 );
}

pgsAutoProgress::~pgsAutoProgress()
{
   if ( m_bCreated )
      m_pProgress->DestroyProgressWindow();
}

HRESULT pgsAutoProgress::CreateProgressWindow(DWORD dwMask,UINT nDelay)
{
   if ( !m_pProgress )
      return E_FAIL;

   HRESULT hr = m_pProgress->CreateProgressWindow(dwMask,nDelay);
   m_bCreated = true;
   return hr;
}

HRESULT pgsAutoProgress::Continue()
{
   if ( !m_pProgress )
      return E_FAIL;

   HRESULT hr = m_pProgress->Continue();
   return hr;
}
