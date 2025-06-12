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

// GraphingAgent.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f GraphingAgentps.mk in the project directory.

#include "stdafx.h"
#include <initguid.h>
#include "resource.h"
#include "CLSID.h"


#include <WBFLCogo_i.c>

#include <EAF/EAFGraphManager.h>
#include <EAF/EAFStatusCenter.h>

#include "PGSuperGrapherImp.h"
#include "PGSpliceGrapherImp.h"

#include "PGSuperCatCom.h"
#include "PGSpliceCatCom.h"

#include <EAF/ComponentModule.h>
WBFL::EAF::ComponentModule _Module;

///////////////////////////////////////////////////////////////////////
// NOTE: This single DLL contains two agents. Both are graphing agents,
// one for PGSuper and one for PGSplice.
//
// This allows the graphing for each application to be different
// while sharing a common code base.

EAF_BEGIN_OBJECT_MAP(ObjectMap)
	EAF_OBJECT_ENTRY(CLSID_PGSuperGraphingAgent, CPGSuperGrapherImp)
	EAF_OBJECT_ENTRY(CLSID_PGSpliceGraphingAgent, CPGSpliceGrapherImp)
EAF_END_OBJECT_MAP()

class CGraphingAgentApp : public CWinApp
{
public:
	virtual BOOL InitInstance() override;
	virtual int ExitInstance() override;
};

CGraphingAgentApp theApp;

BOOL CGraphingAgentApp::InitInstance()
{
	_Module.Init(ObjectMap);
	return CWinApp::InitInstance();
}

int CGraphingAgentApp::ExitInstance()
{
	_Module.Term();
	return CWinApp::ExitInstance();
}
