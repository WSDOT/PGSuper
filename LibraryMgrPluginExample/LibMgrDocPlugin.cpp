///////////////////////////////////////////////////////////////////////
// LibraryMgrPluginExample
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

// LibMgrDocPlugin.cpp : Implementation of CLibMgrDocPlugin

#include "stdafx.h"
#include "LibMgrDocPlugin.h"
#include "resource.h"

#include <EAF\EAFDocument.h>
#include <EAF\EAFResources.h>

#include <EAF\EAFOutputChildFrame.h>
#include "MyView.h"

#include <MFCTools\Prompts.h>
#include <EAF\EAFHelp.h>

#define ID_MYCOMMAND EAF_FIRST_USER_COMMAND

BEGIN_MESSAGE_MAP(CMyCmdTarget,CCmdTarget)
   ON_COMMAND(ID_MYCOMMAND,OnMyCommand)
	//ON_UPDATE_COMMAND_UI(ID_MYCOMMAND, OnUpdateMyCommand)
   ON_COMMAND(IDM_PLUGIN_VIEW,OnCreateView)
END_MESSAGE_MAP()

void CMyCmdTarget::OnMyCommand()
{
   //AfxMessageBox(_T("This is a command implemented by a document plug in"));
   CEAFHelpHandler helpHandler(m_pMyDocPlugin->GetName(),5000);
   int result = AfxChoose(_T("Question"),_T("What up?"),_T("Nothing\nSomething"),0,FALSE,&helpHandler);
}

void CMyCmdTarget::OnUpdateMyCommand(CCmdUI* pCmdUI)
{
//   pCmdUI->SetCheck();
}

void CMyCmdTarget::OnCreateView()
{
   m_pMyDocPlugin->CreateView();
}

// CLibMgrDocPlugin
void CLibMgrDocPlugin::CreateMenus()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CEAFMenu* pMenu = m_pDoc->GetMainMenu();

   // add a command to the File menu
   UINT filePos = pMenu->FindMenuItem(_T("&File"));
   CEAFMenu* pFileMenu = pMenu->GetSubMenu(filePos);
   pFileMenu->InsertMenu(6,ID_MYCOMMAND,_T("Plug in Command"),this);
   pFileMenu->SetMenuItemBitmaps(ID_MYCOMMAND,MF_BYCOMMAND,&m_bmpMenuItem,nullptr,this);

   // add a new drop down menu from the main menu
   UINT nMenus = pMenu->GetMenuItemCount();
   m_pPluginMenu = pMenu->CreatePopupMenu(nMenus-1,_T("Plug in"));
   m_pPluginMenu->LoadMenu(IDR_PLUGIN_MENU,this);
}

void CLibMgrDocPlugin::RemoveMenus()
{
   CEAFMenu* pMenu = m_pDoc->GetMainMenu();

   // remove the command from the File menu
   UINT filePos = pMenu->FindMenuItem(_T("&File"));
   CEAFMenu* pFileMenu = pMenu->GetSubMenu(filePos);
   pFileMenu->RemoveMenu(ID_MYCOMMAND,MF_BYCOMMAND,this);

   // remove the Plug in menu
   pMenu->DestroyMenu(m_pPluginMenu);
}

void CLibMgrDocPlugin::RegisterViews()
{
   m_MyViewKey = m_pDoc->RegisterView(IDR_PLUGIN_MENU,this,RUNTIME_CLASS(CEAFOutputChildFrame),RUNTIME_CLASS(CMyView),nullptr,1);
}

void CLibMgrDocPlugin::UnregisterViews()
{
   m_pDoc->RemoveView(m_MyViewKey);
}

void CLibMgrDocPlugin::CreateView()
{
   m_pDoc->CreateView(m_MyViewKey);
}

//////////////////////////////////////////////////////////
// IEAFDocumentPlugin
BOOL CLibMgrDocPlugin::Init(CEAFDocument* pParent)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   VERIFY(m_bmpMenuItem.LoadBitmap(IDB_MENUITEM));

   m_pDoc = pParent;
   return TRUE;
}

BOOL CLibMgrDocPlugin::IntagrateWithUI(BOOL bIntegrate)
{
   if ( bIntegrate )
   {
      CreateMenus();
      RegisterViews();
   }
   else
   {
      RemoveMenus();
      UnregisterViews();
   }

   return TRUE;
}

void CLibMgrDocPlugin::Terminate()
{
}

CString CLibMgrDocPlugin::GetName()
{
   return _T("Example Library Document Plug in");
}

CString CLibMgrDocPlugin::GetDocumentationSetName()
{
   return GetName();
}

eafTypes::HelpResult CLibMgrDocPlugin::GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nHID,CString& strURL)
{
   if ( GetDocumentationSetName() != CString(lpszDocSetName) )
   {
      return eafTypes::hrDocSetNotFound;
   }

   if ( nHID == 5000 )
   {
      strURL = _T("http://www.wsdot.wa.gov/eesc/bridge/software");
      return eafTypes::hrOK;
   }

   return eafTypes::hrTopicNotFound;
}

//////////////////////////////////////////////////////////
// IEAFCommandCallback
BOOL CLibMgrDocPlugin::OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo)
{
   return m_MyCommandTarget.OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
}

BOOL CLibMgrDocPlugin::GetStatusBarMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // load appropriate string
	if ( rMessage.LoadString(nID) )
	{
		// first newline terminates actual string
      rMessage.Replace('\n','\0');
	}
	else
	{
		// not found
		TRACE1("Warning: no message line prompt for ID 0x%04X.\n", nID);
	}

   return TRUE;
}

BOOL CLibMgrDocPlugin::GetToolTipMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString string;
   // load appropriate string
	if ( string.LoadString(nID) )
	{
		// tip is after first newline 
      int pos = string.Find('\n');
      if ( 0 < pos )
         rMessage = string.Mid(pos+1);
	}
	else
	{
		// not found
		TRACE1("Warning: no tool tip for ID 0x%04X.\n", nID);
	}

   return TRUE;
}

// IEAFPluginPersist
HRESULT CLibMgrDocPlugin::Save(IStructuredSave* pStrSave)
{
   pStrSave->BeginUnit(_T("ExamplePlugin"),1.0);
   pStrSave->put_Property(_T("ExampleData"),CComVariant(3.1415));
   pStrSave->EndUnit();
   return S_OK;
}

HRESULT CLibMgrDocPlugin::Load(IStructuredLoad* pStrLoad)
{
   HRESULT hr = pStrLoad->BeginUnit(_T("ExamplePlugin"));
   if ( FAILED(hr) )
      return hr;

   CComVariant var;
   var.vt = VT_R8;
   hr = pStrLoad->get_Property(_T("ExampleData"),&var);
   if ( FAILED(hr) )
      return hr;

   hr = pStrLoad->EndUnit();
   if ( FAILED(hr) )
      return hr;

   return S_OK;
}