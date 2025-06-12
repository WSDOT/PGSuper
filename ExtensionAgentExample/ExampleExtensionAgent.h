///////////////////////////////////////////////////////////////////////
// ExtensionAgentExample - Extension Agent Example Project for PGSuper
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

// ExampleExtensionAgent.h : Declaration of the CExampleExtensionAgent

#pragma once
#include <EAF\Agent.h>

#include <EAF\EAFUIIntegration.h>

#include <IFace\ExtendUI.h>
#include "EditPierPage.h"


class CExampleExtensionAgent : public CCmdTarget, // it's very important CCmdTarget is the first parent for inheritance, see Warning C4407
   public WBFL::EAF::Agent,
   public WBFL::EAF::IAgentPersist,
   public WBFL::EAF::IAgentUIIntegration,
   public WBFL::EAF::IAgentReportingIntegration,
   public WBFL::EAF::IAgentGraphingIntegration,
   public IEditBridgeCallback,
   public IEditPierCallback, // not a COM interface
   public IEditTemporarySupportCallback,
   public IEditSpanCallback,  // not a COM interface
   public IEditSegmentCallback,
   public IEditClosureJointCallback,
   public IEditSplicedGirderCallback,
   public IEditGirderCallback,
   public IExtendUIEventSink,
   public WBFL::EAF::ICommandCallback
{
public:
   CExampleExtensionAgent() :
      m_Answer("User input")
	{
	}

   IDType m_dwExtendUICookie;

   CBitmap m_bmpMenu;

   std::shared_ptr<WBFL::EAF::Menu> m_MyMenu;
   void CreateMenus();
   void RemoveMenus();

   UINT m_ToolBarID;
   void CreateToolBar();
   void RemoveToolBar();

   long m_MyViewKey;
   void RegisterViews();
   void UnregisterViews();
   void CreateMyView();

   void RegisterReports();
   void RegisterGraphs();

   void SimulateUserInput();

   CString m_Answer; // sample data for persistence

// IAgentEx
public:
   std::_tstring GetName() const override { return _T("ExampleAgent"); }
   bool RegisterInterfaces() override;
   bool Init() override;
   bool Reset() override;
   bool ShutDown() override;
   CLSID GetCLSID() const override;

// IAgentPersist
public:
   WBFL::EAF::Broker::LoadResult Load(IStructuredLoad* pStrLoad) override;
   bool Save(IStructuredSave* pStrSave) override;

// IAgentUIIntegration
public:
   bool IntegrateWithUI(bool bIntegrate) override;

// IAgentReportingIntegration
public:
   bool IntegrateWithReporting(bool bIntegrate) override;

// IAgentGraphingIntegration
public:
   bool IntegrateWithGraphing(bool bIntegrate) override;


// IEditBridgeCallback
public:
   CPropertyPage* CreatePropertyPage(IEditBridgeData* pBridgeData) override;
   std::unique_ptr<WBFL::EAF::Transaction> OnOK(CPropertyPage* pPage,IEditBridgeData* pBridgeData) override;
   void EditPier_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pPierPropertyPage) override;
   void EditTemporarySupport_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pTempSupportPropertyPage) override;
   void EditSpan_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pSpanPropertyPage) override;

// IEditPierCallback
public:
   CPropertyPage* CreatePropertyPage(IEditPierData* pEditPierData) override;
   CPropertyPage* CreatePropertyPage(IEditPierData* pEditPierData,CPropertyPage* pBridgePropertyPage) override;
   std::unique_ptr<WBFL::EAF::Transaction> OnOK(CPropertyPage* pPage,IEditPierData* pEditPierData) override;
   IDType GetEditBridgeCallbackID() override;

// IEditTemporarySupportCallback
public:
   CPropertyPage* CreatePropertyPage(IEditTemporarySupportData* pEditTemporarySupportData) override;
   CPropertyPage* CreatePropertyPage(IEditTemporarySupportData* pEditTemporarySupportData,CPropertyPage* pBridgePropertyPage) override;
   std::unique_ptr<WBFL::EAF::Transaction> OnOK(CPropertyPage* pPage,IEditTemporarySupportData* pEditTemporarySupportData) override;
   //virtual IDType GetEditBridgeCallbackID() override;

// IEditSpanCallback
public:
   CPropertyPage* CreatePropertyPage(IEditSpanData* pEditSpanData) override;
   CPropertyPage* CreatePropertyPage(IEditSpanData* pEditSpanData,CPropertyPage* pBridgePropertyPage) override;
   std::unique_ptr<WBFL::EAF::Transaction> OnOK(CPropertyPage* pPage,IEditSpanData* pEditSpanData) override;
   //virtual IDType GetEditBridgeCallbackID() override;

// IEditSegmentCallback
public:
   CPropertyPage* CreatePropertyPage(IEditSegmentData* pSegmentData) override;
   std::unique_ptr<WBFL::EAF::Transaction> OnOK(CPropertyPage* pPage,IEditSegmentData* pSegmentData) override;
   IDType GetEditSplicedGirderCallbackID() override;
   CPropertyPage* CreatePropertyPage(IEditSegmentData* pEditSegmentData,CPropertyPage* pSplicedGirderPropertyPage) override;

// IEditClosureJointCallback
public:
   CPropertyPage* CreatePropertyPage(IEditClosureJointData* pClosureJointData) override;
   std::unique_ptr<WBFL::EAF::Transaction> OnOK(CPropertyPage* pPage,IEditClosureJointData* pClosureJointData) override;
   //virtual IDType GetEditSplicedGirderCallbackID() override;
   CPropertyPage* CreatePropertyPage(IEditClosureJointData* pEditClosureJointData,CPropertyPage* pSplicedGirderPropertyPage) override;

// IEditSplicedGirderCallback
public:
   CPropertyPage* CreatePropertyPage(IEditSplicedGirderData* pGirderData) override;
   std::unique_ptr<WBFL::EAF::Transaction> OnOK(CPropertyPage* pPage,IEditSplicedGirderData* pGirderData) override;
   void EditSegment_OnOK(CPropertyPage* pSplicedGirderPropertyPage,CPropertyPage* pSegmentPropertyPage) override;
   void EditClosureJoint_OnOK(CPropertyPage* pSplicedGirderPropertyPage,CPropertyPage* pClosureJointPropertyPage) override;

// IEditGirderCallback
public:
   CPropertyPage* CreatePropertyPage(IEditGirderData* pGirderData) override;
   std::unique_ptr<WBFL::EAF::Transaction> OnOK(CPropertyPage* pPage,IEditGirderData* pGirderData) override;


// IExtendUIEventSink
public:
   HRESULT OnHintsReset() override;


// ICommandCallback
public:
   BOOL OnCommandMessage(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) override;
   BOOL GetStatusBarMessageString(UINT nID, CString& rMessage) const override;
   BOOL GetToolTipMessageString(UINT nID, CString& rMessage) const override;

   afx_msg void OnCommand1();
   afx_msg void OnUpdateCommand1(CCmdUI* pCmdUI);
   afx_msg void OnMyView();

   DECLARE_MESSAGE_MAP()

private:
   EAF_DECLARE_AGENT_DATA;

   void RegisterUIExtensions();
   void UnregisterUIExtensions();
   IDType m_EditBridgeCallbackID;
   IDType m_EditPierCallbackID;
   IDType m_EditTemporarySupportCallbackID;
   IDType m_EditSpanCallbackID;
   IDType m_EditSegmentCallbackID;
   IDType m_EditClosureJointCallbackID;
   IDType m_EditSplicedGirderCallbackID;
   IDType m_EditGirderCallbackID;
   bool m_bCheck = true; // dummy data
};
