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

// TestAgent.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f TestAgentps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>

#include "CLSID.h"


#include "TestAgent.h"
#include "TestAgentImp.h"

#include "PGSuperCatCom.h"
#include "PGSpliceCatCom.h"

#include <IFace\Alignment.h>
#include <EAF/EAFStatusCenter.h>
#include <IFace\RatingSpecification.h>
#include <EAF\EAFUIIntegration.h>
#include <IFace\Intervals.h>
#include <IFace\TestFileExport.h>
#include <IFace\GirderHandling.h>
#include <IFace\DocumentType.h>
#include <IFace\PrincipalWebStress.h>
#include <IFace\BearingDesignParameters.h>

#include <EAF\EAFStatusCenter.h>
#include <EAF\EAFProgress.h>

#include <EAF\ComponentModule.h>
WBFL::EAF::ComponentModule _Module;

EAF_BEGIN_OBJECT_MAP(ObjectMap)
   EAF_OBJECT_ENTRY(CLSID_TestAgent, CTestAgentImp)
EAF_END_OBJECT_MAP()

class CTestAgentApp : public CWinApp
{
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTestAgentApp)
	public:
    virtual BOOL InitInstance() override;
    virtual int ExitInstance() override;
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CTestAgentApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CTestAgentApp, CWinApp)
	//{{AFX_MSG_MAP(CTestAgentApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CTestAgentApp theApp;

BOOL CTestAgentApp::InitInstance()
{
    _Module.Init(ObjectMap); //, &LIBID_TESTAGENTLib);
    return CWinApp::InitInstance();
}

int CTestAgentApp::ExitInstance()
{
    _Module.Term();
    return CWinApp::ExitInstance();
}
