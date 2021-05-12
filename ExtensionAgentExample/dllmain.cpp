///////////////////////////////////////////////////////////////////////
// ExtensionAgentExample - Extension Agent Example Project for PGSuper
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

// dllmain.cpp : Implementation of DllMain.

#include "stdafx.h"
#include "resource.h"
#include "ExtensionAgentExample_i.h"
#include "dllmain.h"

#include <initguid.h>
#include "PGSuperCatCom.h"
#include "PGSpliceCatCom.h"
#include <IFace\Tools.h>
#include <IFace\EditByUI.h>
#include <EAF\EAFUIIntegration.h>
#include <EAF\EAFStatusCenter.h>
#include <EAF\EAFDisplayUnits.h>
#include <WBFLCore_i.c>
#include <IReportManager.h>
#include <IFace\ExtendUI.h>
#include <IGraphManager.h>

CExtensionAgentExampleModule _AtlModule;

class CExtensionAgentExampleApp : public CWinApp
{
public:

// Overrides
	virtual BOOL InitInstance() override;
	virtual int ExitInstance() override;

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CExtensionAgentExampleApp, CWinApp)
END_MESSAGE_MAP()

CExtensionAgentExampleApp theApp;

BOOL CExtensionAgentExampleApp::InitInstance()
{
	return CWinApp::InitInstance();
}

int CExtensionAgentExampleApp::ExitInstance()
{
	return CWinApp::ExitInstance();
}
