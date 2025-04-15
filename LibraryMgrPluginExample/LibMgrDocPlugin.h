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

// LibMgrDocPlugin.h : Declaration of the CLibMgrDocPlugin

#pragma once

#include <EAF\EAFDocumentPlugin.h>
#include <EAF\EAFUIintegration.h>
#include <EAF\EAFPluginPersist.h>

#include <initguid.h>

// {894AE5FD-E157-4935-A1C6-2E032CF014A6}
DEFINE_GUID(CLSID_LibMgrDocPlugin,
   0x894ae5fd, 0xe157, 0x4395, 0xa1, 0xc6, 0x2e, 0x03, 0x2c, 0xf0, 0x14, 0xa6);

namespace LibraryMgr
{
   class ExampleDocPlugin;
};

class CMyCmdTarget : public CCmdTarget
{
public:
   CMyCmdTarget() {};
   
   void OnMyCommand();
   void OnUpdateMyCommand(CCmdUI* pCmdUI);
   void OnCreateView();

   LibraryMgr::ExampleDocPlugin* m_pMyDocPlugin = nullptr;

   DECLARE_MESSAGE_MAP()
};

namespace LibraryMgr
{
   class ExampleDocPlugin : public WBFL::EAF::ComponentObject,
	  public WBFL::EAF::IDocumentPlugin,
	  public WBFL::EAF::ICommandCallback,
	  public WBFL::EAF::IPluginPersist
   {
   public:
	  ExampleDocPlugin()
	  {
		 m_MyCommandTarget.m_pMyDocPlugin = this;
	  }

	  CEAFDocument* m_pDoc;
	  CMyCmdTarget m_MyCommandTarget;
	  CBitmap m_bmpMenuItem;

	  void CreateMenus();
	  void RemoveMenus();
	  CEAFMenu* m_pPluginMenu;

	  void RegisterViews();
	  void UnregisterViews();
	  void CreateView();
	  long m_MyViewKey;

	  // IDocumentPlugin
   public:
	  BOOL Init(CEAFDocument* pParent) override;
	  BOOL IntegrateWithUI(BOOL bIntegrate) override;
	  void Terminate() override;
	  CString GetName() override;
	  CString GetDocumentationSetName() override;
	  eafTypes::HelpResult GetDocumentLocation(LPCTSTR lpszDocSetName, UINT nHID, CString& strURL) override;

	  // ICommandCallback
   public:
	  BOOL OnCommandMessage(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) override;
	  BOOL GetStatusBarMessageString(UINT nID, CString& rMessage) const override;
	  BOOL GetToolTipMessageString(UINT nID, CString& rMessage) const override;

	  // IPluginPersist
   public:
	  HRESULT Save(IStructuredSave* pStrSave) override;
	  HRESULT Load(IStructuredLoad* pStrLoad) override;
   };
};
