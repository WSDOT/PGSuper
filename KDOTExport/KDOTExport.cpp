///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

// IEPluginExample.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f IEPluginExampleps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "KDOTExport.h"
#include <Plugins\PGSuperIEPlugin.h>
#include "PGSuperCatCom.h"
#include <BridgeLinkCATID.h>
#include <WBFLCogo.h>
#include <WBFLCogo_i.c>
#include <EAF/EAFProgress.h>

#include "KDOTDataExporter.h"
#include "KDOTComponentInfo.h"

#include "PGSuperInterfaces.h"
#include <IFace\DocumentType.h>

#include <EAF\ComponentModule.h>
WBFL::EAF::ComponentModule Module_;
EAF_BEGIN_OBJECT_MAP(ObjectMap)
   EAF_OBJECT_ENTRY(CLSID_KDOTDataExporter,CKDOTDataExporter)
   EAF_OBJECT_ENTRY(CLSID_KDOTPGSuperComponentInfo, CKDOTComponentInfo)
EAF_END_OBJECT_MAP()

class CKDOTExportAppPlugin : public CWinApp
{
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CKDOTExportAppPlugin)
	public:
    virtual BOOL InitInstance() override;
    virtual int ExitInstance() override;
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CKDOTExportAppPlugin)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
    afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CKDOTExportAppPlugin, CWinApp)
	//{{AFX_MSG_MAP(CKDOTExportAppPlugin)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
   ON_COMMAND(ID_HELP,OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CKDOTExportAppPlugin theApp;

void CKDOTExportAppPlugin::OnHelp()
{
   // we don't really need to handle this, but
   // an ID_HELP command handler must be present
   // or CDialog::OnInitDialog will hide the
   // [HELP] button on our dialogs
}

BOOL CKDOTExportAppPlugin::InitInstance()
{
   // To get grids working
   GXInit();

   Module_.Init(ObjectMap);

   return CWinApp::InitInstance();
}

int CKDOTExportAppPlugin::ExitInstance()
{
   GXForceTerminate();
   Module_.Term();
   return CWinApp::ExitInstance();
}
