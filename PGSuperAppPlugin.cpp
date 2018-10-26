///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include "PGSuperAppPlugin.h"
#include "PGSuperCatCom.h"
#include "resource.h"

#include "PGSuperDoc.h"
#include "PGSuperDocTemplate.h"
#include "BridgeModelViewChildFrame.h"
#include "BridgePlanView.h"
#include <PsgLib\BeamFamilyManager.h>

#include "PluginManagerDlg.h"


BEGIN_MESSAGE_MAP(CMyCmdTarget,CCmdTarget)
   ON_COMMAND(ID_OPTIONS_PLUGINS,OnConfigurePlugins)
   ON_COMMAND(ID_UPDATE_TEMPLATE,OnUpdateTemplates)
END_MESSAGE_MAP()

void CMyCmdTarget::OnConfigurePlugins()
{
   m_pMyAppPlugin->ConfigurePlugins();
}

void CMyCmdTarget::OnUpdateTemplates()
{
   m_pMyAppPlugin->UpdateTemplates();
}

HRESULT CPGSuperAppPlugin::FinalConstruct()
{
   return OnFinalConstruct(); // CPGSuperBaseAppPlugin
}

void CPGSuperAppPlugin::FinalRelease()
{
   OnFinalRelease(); // CPGSuperBaseAppPlugin
}

void CPGSuperAppPlugin::ConfigurePlugins()
{
   CPluginManagerDlg dlg("Manage Plugins and Extensions");
   dlg.DoModal(); // this DoModal is correct... the dialog takes care of its own data
}

BOOL CPGSuperAppPlugin::Init(CEAFApp* pParent)
{
   // See MSKB Article ID: Q118435, "Sharing Menus Between MDI Child Windows"
   CWinApp* pApp = AfxGetApp();
   m_hMenuShared = ::LoadMenu( pApp->m_hInstance, MAKEINTRESOURCE(IDR_PGSUPERTYPE) );

   if ( m_hMenuShared == NULL )
      return FALSE;

   // Application start up can be improved if this call
   // is executed in its own thread... Need to add some
   // code that indicates if the call fails.. then throw
   // a shut down exception
   if ( FAILED(CBeamFamilyManager::Init()) )
      return FALSE;

   sysComCatMgr::CreateCategory(L"PGSuper Components",CATID_PGSuperComponents);

   return TRUE;
}

void CPGSuperAppPlugin::Terminate()
{
   // release the shared menu
   ::DestroyMenu( m_hMenuShared );
}

CEAFDocTemplate* CPGSuperAppPlugin::CreateDocTemplate()
{
   CPGSuperDocTemplate* pTemplate = new CPGSuperDocTemplate(
		IDR_BRIDGEMODELEDITOR,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(CBridgeModelViewChildFrame),
		RUNTIME_CLASS(CBridgePlanView),
      m_hMenuShared,1);

   pTemplate->SetPlugin(this);

   return pTemplate;
}

CCmdTarget* CPGSuperAppPlugin::GetCommandTarget()
{
   return &m_MyCmdTarget;
}

HMENU CPGSuperAppPlugin::GetSharedMenuHandle()
{
   return m_hMenuShared;
}

UINT CPGSuperAppPlugin::GetDocumentResourceID()
{
   return IDR_PGSUPERTYPE;
}

CString CPGSuperAppPlugin::GetName()
{
   return CString("PGSuper");
}

void CPGSuperAppPlugin::UpdateTemplates()
{
   int result = AfxMessageBox("All of the template library entries will be updated to match the Master Library.\n\nDo you want to proceed?",MB_YESNO);
   if ( result == IDYES )
   {
      m_bUpdatingTemplate = true;
      CWinApp* pApp = AfxGetApp();

      POSITION pos = pApp->GetFirstDocTemplatePosition();
      CDocTemplate* pTemplate = pApp->GetNextDocTemplate(pos);
      while ( pTemplate )
      {
         if ( pTemplate->IsKindOf(RUNTIME_CLASS(CPGSuperDocTemplate)) )
         {
            CPGSuperDoc* pPGSuperDoc = (CPGSuperDoc*)pTemplate->CreateNewDocument();
            pPGSuperDoc->m_bAutoDelete = false;
            pPGSuperDoc->UpdateTemplates();
            delete pPGSuperDoc;
            break;
         }

         pTemplate = pApp->GetNextDocTemplate(pos);
      }
      AfxMessageBox("Update complete",MB_OK);
      m_bUpdatingTemplate = false;
   }
}

bool CPGSuperAppPlugin::UpdatingTemplates()
{
   return m_bUpdatingTemplate;
}
