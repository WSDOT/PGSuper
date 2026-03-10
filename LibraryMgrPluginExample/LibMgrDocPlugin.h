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

#pragma once

#include <EAF\DocumentPlugin.h>
#include <EAF\EAFUIintegration.h>
#include <EAF\EAFPluginPersist.h>
#include <EAF/ComponentObject.h>


namespace LibraryMgr
{
   class ExampleDocPlugin : public CCmdTarget, public WBFL::EAF::ComponentObject,
	  public WBFL::EAF::IDocumentPlugin,
	  public WBFL::EAF::ICommandCallback,
	  public WBFL::EAF::IPluginPersist
   {
   public:
	  ExampleDocPlugin()
	  {
	  }

	  CEAFDocument* m_pDoc;
	  CBitmap m_bmpMenuItem;

	  void CreateMenus();
	  void RemoveMenus();
	  std::shared_ptr<WBFL::EAF::Menu> m_PluginMenu;

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
	  std::pair<WBFL::EAF::HelpResult,CString> GetDocumentLocation(LPCTSTR lpszDocSetName, UINT nHID) override;

	  // ICommandCallback
   public:
	  BOOL OnCommandMessage(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) override;
	  BOOL GetStatusBarMessageString(UINT nID, CString& rMessage) const override;
	  BOOL GetToolTipMessageString(UINT nID, CString& rMessage) const override;

	  // IPluginPersist
   public:
	  bool Save(IStructuredSave* pStrSave) override;
	  bool Load(IStructuredLoad* pStrLoad) override;

   private:
	  void OnMyCommand();
	  void OnUpdateMyCommand(CCmdUI* pCmdUI);
	  void OnCreateView();

	  DECLARE_MESSAGE_MAP()
   };
};
