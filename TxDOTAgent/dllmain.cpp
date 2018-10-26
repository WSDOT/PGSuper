///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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


CTxDOTAgentModule _AtlModule;

class CTxDOTAgentApp : public CWinApp
{
public:

// Overrides
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CTxDOTAgentApp, CWinApp)
END_MESSAGE_MAP()

CTxDOTAgentApp theApp;

BOOL CTxDOTAgentApp::InitInstance()
{
   // Deal with help file name
   CString strHelpFile(m_pszHelpFilePath);
#if defined _DEBUG
#if defined _WIN64
   strHelpFile.Replace(_T("RegFreeCOM\\x64\\Debug\\"),_T("TxDOTAgent\\"));
#else
   strHelpFile.Replace(_T("RegFreeCOM\\Win32\\Debug\\"),_T("TxDOTAgent\\"));
#endif
#else
   // in a real release, the path doesn't contain RegFreeCOM\\Release, but that's
   // ok... the replace will fail and the string wont be altered.
#if defined _WIN64
   strHelpFile.Replace(_T("RegFreeCOM\\x64\\Release\\"),_T("TxDOTAgent\\"));
#else
   strHelpFile.Replace(_T("RegFreeCOM\\Win32\\Release\\"),_T("TxDOTAgent\\"));
#endif
#endif
   
   // rename the file itself
   int loc = strHelpFile.ReverseFind(_T('\\'));
   strHelpFile = strHelpFile.Left(loc+1);
   strHelpFile += _T("TOGA.chm");

   free((void*)m_pszHelpFilePath);
   m_pszHelpFilePath = _tcsdup(strHelpFile);

   // pretty sure this is required
   EnableHtmlHelp();

   // Set up the root of the registry keys
   SetRegistryKey( _T("Washington State Department of Transportation") );
   free((void*)m_pszProfileName);
   m_pszProfileName = _tcsdup(_T("PGSuper"));

	return CWinApp::InitInstance();
}

int CTxDOTAgentApp::ExitInstance()
{
	return CWinApp::ExitInstance();
}
