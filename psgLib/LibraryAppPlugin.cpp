#include "stdafx.h"
#include "PGSuperLibrary_i.h"
#include "LibraryAppPlugin.h"

#include "resource.h"

#include <PsgLib\LibraryDocTemplate.h>
#include <PsgLib\LibraryEditorDoc.h>
#include <PsgLib\LibChildFrm.h>
#include <PsgLib\LibraryEditorView.h>

BOOL CLibraryAppPlugin::Init(CEAFApp* pParent)
{
   return TRUE;
}

void CLibraryAppPlugin::Terminate()
{
}

CEAFDocTemplate* CLibraryAppPlugin::CreateDocTemplate()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   // doc template for Library editor
	CLibraryDocTemplate* pLibMgrDocTemplate = new CLibraryDocTemplate(
		IDR_LIBRARTYPE,
		RUNTIME_CLASS(CLibraryEditorDoc),
		RUNTIME_CLASS(CLibChildFrame),
		RUNTIME_CLASS(CLibraryEditorView));

   pLibMgrDocTemplate->SetPlugin(this);

   return pLibMgrDocTemplate;
}

CCmdTarget* CLibraryAppPlugin::GetCommandTarget()
{
   return NULL;
}

HMENU CLibraryAppPlugin::GetSharedMenuHandle()
{
   return NULL;
}

UINT CLibraryAppPlugin::GetDocumentResourceID()
{
   return IDR_LIBRARTYPE;
}

CString CLibraryAppPlugin::GetName()
{
   return CString("PGSuper Library Editor");
}
