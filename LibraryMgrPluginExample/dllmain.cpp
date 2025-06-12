///////////////////////////////////////////////////////////////////////
// LibraryMgrPluginExample
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

// dllmain.cpp : Implementation of DllMain.

#include "stdafx.h"
#include "resource.h"
#include "CLSID.h"
#include "dllmain.h"

#include "LibMgrDocPlugin.h"
#include <EAF\ComponentModule.h>

WBFL::EAF::ComponentModule Module_;

EAF_BEGIN_OBJECT_MAP(ObjectMap)
   EAF_OBJECT_ENTRY(CLSID_LibMgrDocPlugin, LibraryMgr::ExampleDocPlugin)
EAF_END_OBJECT_MAP()

class CLibraryMgrPluginExampleApp : public CWinApp
{
public:

// Overrides
	virtual BOOL InitInstance() override;
	virtual int ExitInstance() override;

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CLibraryMgrPluginExampleApp, CWinApp)
END_MESSAGE_MAP()

CLibraryMgrPluginExampleApp theApp;

BOOL CLibraryMgrPluginExampleApp::InitInstance()
{
   Module_.Init(ObjectMap);
   return CWinApp::InitInstance();
}

int CLibraryMgrPluginExampleApp::ExitInstance()
{
   Module_.Term();
	return CWinApp::ExitInstance();
}
