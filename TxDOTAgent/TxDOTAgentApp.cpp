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

// TxDOTAgentApp.cpp : Implementation of CTxDOTAgentApp.

#include "stdafx.h"
#include "TxDOTAgentApp.h"

#include "EAF\EAFApp.h"
#include <EAF\EAFDocument.h>

#include <MFCTools\VersionInfo.h>

#include "CLSID.h"
#include "TxDOTCADExporter.h"
#include "TxDOTAppPluginComponentInfo.h"
#include "TxDOTComponentInfo.h"
#include "TOGAPluginApp.h"
#include "TxDOTAgentImp.h"
#include <EAF\ComponentModule.h>

WBFL::EAF::ComponentModule Module_;

EAF_BEGIN_OBJECT_MAP(ObjectMap)
   EAF_OBJECT_ENTRY(CLSID_TxDOTCadExporter, CTxDOTCadExporter)
   EAF_OBJECT_ENTRY(CLSID_TxDOTPGSuperComponentInfo, CTxDOTComponentInfo)
   EAF_OBJECT_ENTRY(CLSID_TOGAPluginAppComponentInfo, CTxDOTAppPluginComponentInfo)
   EAF_OBJECT_ENTRY(CLSID_TOGAPluginApp, CTOGAPluginApp)
   EAF_OBJECT_ENTRY(CLSID_TxDOTAgent, CTxDOTAgentImp)
EAF_END_OBJECT_MAP()



BEGIN_MESSAGE_MAP(CTxDOTAgentApp, CWinApp)
   ON_BN_CLICKED(ID_HELP,OnHelp)
END_MESSAGE_MAP()

void CTxDOTAgentApp::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_WELCOME);
}

BOOL CTxDOTAgentApp::InitInstance()
{
   Module_.Init(ObjectMap);

   GXInit();

   // Set up the root of the registry keys
   SetRegistryKey( _T("Washington State Department of Transportation") );
   free((void*)m_pszProfileName);
   m_pszProfileName = _tcsdup(_T("PGSuper"));

	return CWinApp::InitInstance();
}

int CTxDOTAgentApp::ExitInstance()
{
   GXForceTerminate();
   Module_.Term();
	return CWinApp::ExitInstance();
}

CString CTxDOTAgentApp::GetVersion()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString strExe = m_pszExeName;
   strExe += _T(".dll");

   CVersionInfo verInfo;
   verInfo.Load(strExe);
   CString strVersion = verInfo.GetFileVersionAsString();
   CString strCopyright = verInfo.GetLegalCopyright();

   return strVersion;
}

CString CTxDOTAgentApp::GetCopyRight()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString strExe = m_pszExeName;
   strExe += _T(".dll");

   CVersionInfo verInfo;
   verInfo.Load(strExe);
   CString strCopyright = verInfo.GetLegalCopyright();

   return strCopyright;
}

