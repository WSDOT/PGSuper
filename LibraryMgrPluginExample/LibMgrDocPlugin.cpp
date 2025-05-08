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

// LibMgrDocPlugin.cpp : Implementation of CLibMgrDocPlugin

#include "stdafx.h"
#include "LibMgrDocPlugin.h"
#include "resource.h"

#include <EAF\EAFDocument.h>
#include <EAF\EAFResources.h>

#include <EAF\EAFOutputChildFrame.h>
#include "MyView.h"

#include <MFCTools\Prompts.h>
#include <EAF\Help.h>

#define ID_MYCOMMAND EAF_FIRST_USER_COMMAND

using namespace LibraryMgr;

BEGIN_MESSAGE_MAP(ExampleDocPlugin,CCmdTarget)
   ON_COMMAND(ID_MYCOMMAND,OnMyCommand)
	//ON_UPDATE_COMMAND_UI(ID_MYCOMMAND, OnUpdateMyCommand)
   ON_COMMAND(IDM_PLUGIN_VIEW,OnCreateView)
END_MESSAGE_MAP()

void ExampleDocPlugin::CreateMenus()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   auto pMenu = m_pDoc->GetMainMenu();

   // add a command to the File menu
   UINT filePos = pMenu->FindMenuItem(_T("&File"));
   auto pFileMenu = pMenu->GetSubMenu(filePos);
   
   auto callback = std::dynamic_pointer_cast<WBFL::EAF::ICommandCallback>(shared_from_this());

   pFileMenu->InsertMenu(6, ID_MYCOMMAND, _T("Plug in Command"), callback);
   pFileMenu->SetMenuItemBitmaps(ID_MYCOMMAND, MF_BYCOMMAND, &m_bmpMenuItem, nullptr, callback);

   // add a new drop down menu from the main menu
   UINT nMenus = pMenu->GetMenuItemCount();
   m_PluginMenu = pMenu->CreatePopupMenu(nMenus - 1, _T("Plug in"));
   m_PluginMenu->LoadMenu(IDR_PLUGIN_MENU, callback);
}

void ExampleDocPlugin::RemoveMenus()
{
   auto pMenu = m_pDoc->GetMainMenu();

   // remove the command from the File menu
   UINT filePos = pMenu->FindMenuItem(_T("&File"));
   auto pFileMenu = pMenu->GetSubMenu(filePos);

   auto callback = std::dynamic_pointer_cast<WBFL::EAF::ICommandCallback>(shared_from_this());

   pFileMenu->RemoveMenu(ID_MYCOMMAND, MF_BYCOMMAND, callback);

   // remove the Plug in menu
   pMenu->DestroyMenu(m_PluginMenu);
   m_PluginMenu = nullptr;
}

void ExampleDocPlugin::RegisterViews()
{
   auto callback = std::dynamic_pointer_cast<WBFL::EAF::ICommandCallback>(shared_from_this());
   m_MyViewKey = m_pDoc->RegisterView(IDR_PLUGIN_MENU, callback, RUNTIME_CLASS(CEAFOutputChildFrame), RUNTIME_CLASS(CMyView), nullptr, 1);
}

void ExampleDocPlugin::UnregisterViews()
{
   m_pDoc->RemoveView(m_MyViewKey);
}

void ExampleDocPlugin::CreateView()
{
   m_pDoc->CreateView(m_MyViewKey);
}

//////////////////////////////////////////////////////////
// WBFL::EAF::IDocumentPlugin
BOOL ExampleDocPlugin::Init(CEAFDocument* pParent)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   VERIFY(m_bmpMenuItem.LoadBitmap(IDB_MENUITEM));

   m_pDoc = pParent;
   return TRUE;
}

BOOL ExampleDocPlugin::IntegrateWithUI(BOOL bIntegrate)
{
   if (bIntegrate)
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

void ExampleDocPlugin::Terminate()
{
}

CString ExampleDocPlugin::GetName()
{
   return _T("Example Library Document Plug in");
}

CString ExampleDocPlugin::GetDocumentationSetName()
{
   return GetName();
}

std::pair<WBFL::EAF::HelpResult,CString> ExampleDocPlugin::GetDocumentLocation(LPCTSTR lpszDocSetName, UINT nHID)
{
   if (GetDocumentationSetName() != CString(lpszDocSetName))
   {
	  return { WBFL::EAF::HelpResult::DocSetNotFound,CString() };
   }

   if (nHID == 5000)
   {
	  CString strURL = _T("http://www.wsdot.wa.gov/eesc/bridge/software");
	  return { WBFL::EAF::HelpResult::OK,strURL };
   }

   return { WBFL::EAF::HelpResult::TopicNotFound,CString() };
}

//////////////////////////////////////////////////////////
// ICommandCallback
BOOL ExampleDocPlugin::OnCommandMessage(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
   return OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

BOOL ExampleDocPlugin::GetStatusBarMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // load appropriate string
   if (rMessage.LoadString(nID))
   {
	  // first newline terminates actual string
	  rMessage.Replace('\n', '\0');
   }
   else
   {
	  // not found
	  TRACE1("Warning: no message line prompt for ID 0x%04X.\n", nID);
   }

   return TRUE;
}

BOOL ExampleDocPlugin::GetToolTipMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString string;
   // load appropriate string
   if (string.LoadString(nID))
   {
	  // tip is after first newline 
	  int pos = string.Find('\n');
	  if (0 < pos)
		 rMessage = string.Mid(pos + 1);
   }
   else
   {
	  // not found
	  TRACE1("Warning: no tool tip for ID 0x%04X.\n", nID);
   }

   return TRUE;
}

// IPluginPersist
bool ExampleDocPlugin::Save(IStructuredSave* pStrSave)
{
   pStrSave->BeginUnit(_T("ExamplePlugin"), 1.0);
   pStrSave->put_Property(_T("ExampleData"), CComVariant(3.1415));
   pStrSave->EndUnit();
   return true;
}

bool ExampleDocPlugin::Load(IStructuredLoad* pStrLoad)
{
   HRESULT hr = pStrLoad->BeginUnit(_T("ExamplePlugin"));
   if (FAILED(hr))
	  return false;

   CComVariant var;
   var.vt = VT_R8;
   hr = pStrLoad->get_Property(_T("ExampleData"), &var);
   if (FAILED(hr))
	  return false;

   hr = pStrLoad->EndUnit();
   if (FAILED(hr))
	  return false;

   return true;
}

void ExampleDocPlugin::OnMyCommand()
{
   WBFL::EAF::HelpHandler helpHandler(GetName(), 5000);
   int result = AfxChoose(_T("Question"), _T("What up?"), _T("Nothing\nSomething"), 0, FALSE, &helpHandler);
}

void ExampleDocPlugin::OnUpdateMyCommand(CCmdUI* pCmdUI)
{
   //   pCmdUI->SetCheck();
}

void ExampleDocPlugin::OnCreateView()
{
   CreateView();
}
