///////////////////////////////////////////////////////////////////////
// TxDOTAgent
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

#include "stdafx.h"
#include "TxDOTAgentApp.h"
#include "dllmain.h"

#include "TxDOTAppPluginComponentInfo.h"
#include "resource.h"
#include <EAF\EAFApp.h>
#include <EAF\EAFUtilities.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


HRESULT CTxDOTAppPluginComponentInfo::FinalConstruct()
{
   return S_OK;
}

void CTxDOTAppPluginComponentInfo::FinalRelease()
{
}

BOOL CTxDOTAppPluginComponentInfo::Init(CEAFApp* pApp)
{
   return TRUE;
}

void CTxDOTAppPluginComponentInfo::Terminate()
{
}

CString CTxDOTAppPluginComponentInfo::GetName()
{
   return _T("TOGA - TxDOT Optional Girder Analysis");
}

CString CTxDOTAppPluginComponentInfo::GetDescription()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CTxDOTAgentApp* pApp = (CTxDOTAgentApp*)AfxGetApp();
   CString strVersion = pApp->GetVersion();
   CString strCopyright = pApp->GetCopyRight();

   CString strDesc;
   strDesc.Format(_T("TOGA - TxDOT Optional Girder Analysis\nVersion %s\n%s"),strVersion,strCopyright);
   return strDesc;
}

HICON CTxDOTAppPluginComponentInfo::GetIcon()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return AfxGetApp()->LoadIcon(IDR_TXDOTOPTIONALDESIGN);
}

bool CTxDOTAppPluginComponentInfo::HasMoreInfo()
{
   return false;
}

void CTxDOTAppPluginComponentInfo::OnMoreInfo()
{
}
