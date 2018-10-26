#include "stdafx.h"
#include "PGSuperProjectImporterAppPlugin.h"
#include "PGSuperImportPluginDocTemplate.h"
#include "resource.h"
#include "BridgeModelViewChildFrame.h"
#include "BridgePlanView.h"

#include "PGSuperProjectImporterMgr.h"

HRESULT CPGSuperProjectImporterAppPlugin::FinalConstruct()
{
   return OnFinalConstruct(); // CPGSuperBaseAppPlugin
}

void CPGSuperProjectImporterAppPlugin::FinalRelease()
{
   OnFinalRelease(); // CPGSuperBaseAppPlugin
}

BOOL CPGSuperProjectImporterAppPlugin::Init(CEAFApp* pParent)
{
   // See MSKB Article ID: Q118435, "Sharing Menus Between MDI Child Windows"
   CWinApp* pApp = AfxGetApp();
   m_hMenuShared = ::LoadMenu( pApp->m_hInstance, MAKEINTRESOURCE(IDR_PGSUPERTYPE) );

   return (m_hMenuShared != NULL);
}

void CPGSuperProjectImporterAppPlugin::Terminate()
{
   // release the shared menu
   ::DestroyMenu( m_hMenuShared );
}

void CPGSuperProjectImporterAppPlugin::IntegrateWithUI(BOOL bIntegrate)
{
   // no UI integration
}

CEAFDocTemplate* CPGSuperProjectImporterAppPlugin::CreateDocTemplate()
{
   CPGSuperImportPluginDocTemplate* pDocTemplate = new CPGSuperImportPluginDocTemplate(
		IDR_BRIDGEMODELEDITOR,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(CBridgeModelViewChildFrame),
		RUNTIME_CLASS(CBridgePlanView),
      m_hMenuShared,1);

   pDocTemplate->SetPlugin(this);

   // If there aren't any importers, we don't want the "PGSuper Project Importer" option to
   // show up in the File | New dialog
   // Returning a NULL doc template will do the trick
   const CPGSuperProjectImporterMgr& importerMgr = pDocTemplate->GetProjectImporterManager();
   if ( importerMgr.GetImporterCount() == 0 )
   {
      delete pDocTemplate;
      pDocTemplate = NULL;
   }

   return pDocTemplate;
}

HMENU CPGSuperProjectImporterAppPlugin::GetSharedMenuHandle()
{
   return m_hMenuShared;
}

UINT CPGSuperProjectImporterAppPlugin::GetDocumentResourceID()
{
   return IDR_PGSUPERTYPE;
}

CString CPGSuperProjectImporterAppPlugin::GetName()
{
   return CString("PGSuper Project Importer");
}
