///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

// dllmain.cpp : Implementation of DllMain.

#include "stdafx.h"
#include "resource.h"
#include "TxDOTAgent_i.h"
#include "dllmain.h"
#include "EAF\EAFUtilities.h"
#include "EAF\EAFApp.h"
#include <EAF\EAFDocument.h>


CTxDOTAgentModule _AtlModule;

class CTxDOTAgentApp : public CWinApp
{
public:

// Overrides
	virtual BOOL InitInstance() override;
	virtual int ExitInstance() override;

   afx_msg void OnHelp();

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CTxDOTAgentApp, CWinApp)
   ON_BN_CLICKED(ID_HELP,OnHelp)
END_MESSAGE_MAP()

CTxDOTAgentApp theApp;

void CTxDOTAgentApp::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_WELCOME);
}

BOOL CTxDOTAgentApp::InitInstance()
{
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
	return CWinApp::ExitInstance();
}
