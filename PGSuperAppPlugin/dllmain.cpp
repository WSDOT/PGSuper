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

// dllmain.cpp : Implementation of DllMain.

#include "stdafx.h"
#include "resource.h"
#include "dllmain.h"
#include "PGSuperApp.h"

#include <initguid.h>
#include <EAF\ComponentInfo.h>
#include <EAF\EAFUIIntegration.h>
#include <System\ComCatMgr.h>
#include <BridgeLinkCATID.h>
#include <PGSuperCatCom.h>
#include <PGSpliceCatCom.h>
#include <Plugins\PGSuperIEPlugin.h>
#include <IFace\BeamFactory.h>

#include <BridgeModelViewController.h>
#include <GirderModelViewController.h>
#include <LoadsViewController.h>

#include <EAF\EAFApp.h>
#include <EAF\EAFUtilities.h>

#include "CLSID.h"
#include "PGSuperComponentInfo.h"
#include "PGSpliceComponentInfo.h"
#include <EAF\ComponentModule.h>
#include "PGSuperPluginApp.h"
#include "PGSplicePluginApp.h"
#include "PGSuperProjectImporterPluginApp.h"
#include "PGSpliceProjectImporterPluginApp.h"

WBFL::EAF::ComponentModule Module_;
EAF_BEGIN_OBJECT_MAP(ObjectMap)
EAF_OBJECT_ENTRY(CLSID_PGSuperComponentInfo,CPGSuperComponentInfo)
EAF_OBJECT_ENTRY(CLSID_PGSpliceComponentInfo, CPGSpliceComponentInfo)
EAF_OBJECT_ENTRY(CLSID_PGSuperPluginApp, CPGSuperPluginApp)
EAF_OBJECT_ENTRY(CLSID_PGSplicePluginApp, CPGSplicePluginApp)
EAF_OBJECT_ENTRY(CLSID_PGSuperProjectImporterPluginApp, CPGSuperProjectImporterPluginApp)
EAF_OBJECT_ENTRY(CLSID_PGSpliceProjectImporterPluginApp, CPGSpliceProjectImporterPluginApp)
EAF_END_OBJECT_MAP()

CPGSuperPluginAppApp theApp;

BEGIN_MESSAGE_MAP(CPGSuperPluginAppApp, CWinApp)
   ON_COMMAND(ID_HELP,OnHelp)
END_MESSAGE_MAP()


BOOL CPGSuperPluginAppApp::InitInstance()
{
   Module_.Init(ObjectMap);

   GXInit();
	GXSetNewGridLineMode(TRUE);	// use smarter grid lines (and dotted) 

   CGXTabWnd::RegisterClass(m_hInstance);

   SetRegistryKey( _T("Washington State Department of Transportation") );

#pragma Reminder("WORKING HERE - Removing COM - clean out dead code")
   //// Register the component categories PGSuper needs
   //// This should be done in the installer, but do it again here just in case
   //WBFL::System::ComCatMgr::CreateCategory(L"PGSuper Agent",                   CATID_PGSuperAgent);
   //WBFL::System::ComCatMgr::CreateCategory(L"PGSuper Extension Agent",         CATID_PGSuperExtensionAgent);
   //WBFL::System::ComCatMgr::CreateCategory(L"PGSuper Beam Family",             CATID_PGSuperBeamFamily);
   ////WBFL::System::ComCatMgr::CreateCategory(L"PGSuper Project Importer Plugin", CATID_PGSuperProjectImporter);
   //WBFL::System::ComCatMgr::CreateCategory(L"PGSuper Data Importer Plugin",    CATID_PGSuperDataImporter);
   //WBFL::System::ComCatMgr::CreateCategory(L"PGSuper Data Exporter Plugin",    CATID_PGSuperDataExporter);
   //WBFL::System::ComCatMgr::CreateCategory(L"PGSuper Component Information",   CATID_PGSuperComponentInfo);

   //WBFL::System::ComCatMgr::CreateCategory(L"PGSplice Agent",                   CATID_PGSpliceAgent);
   //WBFL::System::ComCatMgr::CreateCategory(L"PGSplice Extension Agent",         CATID_PGSpliceExtensionAgent);
   //WBFL::System::ComCatMgr::CreateCategory(L"PGSplice Beam Family",             CATID_PGSpliceBeamFamily);
   ////WBFL::System::ComCatMgr::CreateCategory(L"PGSplice Project Importer Plugin", CATID_PGSpliceProjectImporter);
   //WBFL::System::ComCatMgr::CreateCategory(L"PGSplice Data Importer Plugin",    CATID_PGSpliceDataImporter);
   //WBFL::System::ComCatMgr::CreateCategory(L"PGSplice Data Exporter Plugin",    CATID_PGSpliceDataExporter);
   //WBFL::System::ComCatMgr::CreateCategory(L"PGSplice Component Information",   CATID_PGSpliceComponentInfo);

	return CWinApp::InitInstance();
}

int CPGSuperPluginAppApp::ExitInstance()
{
   Module_.Term();
   GXForceTerminate();
	return CWinApp::ExitInstance();
}

void CPGSuperPluginAppApp::OnHelp()
{
   // just need a default handler so the CDialog doesn't hide our help buttons
}

CString CPGSuperPluginAppApp::GetVersion(bool bIncludeBuildNumber) const
{
   CString strExe( m_pszExeName );
   strExe += ".dll";

   CVersionInfo verInfo;
   if ( !verInfo.Load(strExe) )
   {
      ATLASSERT(false);
      return CString(_T("Unknown"));
   }
   
#if defined _DEBUG || defined _BETA_VERSION
   // always include the build number in debug and beta versions
   bIncludeBuildNumber = true;
#endif
   CString strVersion = verInfo.GetProductVersionAsString(bIncludeBuildNumber);

   return strVersion;
}

