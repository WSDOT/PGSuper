///////////////////////////////////////////////////////////////////////
// ExtensionAgentExample - Extension Agent Example Project for PGSuper
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include "resource.h"       // main symbols
#include "ExtensionAgentExample_i.h"
#include <EAF\EAFInterfaceCache.h>
#include <EAF\EAFUIIntegration.h>

#include <IFace\ExtendUI.h>
#include "EditPierPage.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

class CExampleExtensionAgent;

class CMyCmdTarget : public CCmdTarget
{
public:
   CMyCmdTarget() {};

   void OnCommand1();
	void OnUpdateCommand1(CCmdUI* pCmdUI);
   void OnMyView();

   CExampleExtensionAgent* m_pMyAgent;

   DECLARE_MESSAGE_MAP()
};


// CExampleExtensionAgent

class ATL_NO_VTABLE CExampleExtensionAgent :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CExampleExtensionAgent, &CLSID_ExampleExtensionAgent>,
   public IEAFCommandCallback,
	public IAgentEx,
   public IAgentPersist,
   public IAgentUIIntegration,
   public IAgentReportingIntegration,
   public IAgentGraphingIntegration,
   public IEditBridgeCallback,
   public IEditPierCallback, // not a COM interface
   public IEditTemporarySupportCallback,
   public IEditSpanCallback,  // not a COM interface
   public IEditSegmentCallback,
   public IEditClosureJointCallback,
   public IEditSplicedGirderCallback,
   public IEditGirderCallback,
   public IExtendUIEventSink
{
public:
   CExampleExtensionAgent() :
      m_Answer("User input")
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_EXAMPLEEXTENSIONAGENT)

DECLARE_NOT_AGGREGATABLE(CExampleExtensionAgent)

BEGIN_COM_MAP(CExampleExtensionAgent)
	COM_INTERFACE_ENTRY(IAgent)
	COM_INTERFACE_ENTRY(IAgentEx)
	COM_INTERFACE_ENTRY(IAgentPersist)
   COM_INTERFACE_ENTRY(IAgentUIIntegration)
   COM_INTERFACE_ENTRY(IAgentReportingIntegration)
   COM_INTERFACE_ENTRY(IAgentGraphingIntegration)
   COM_INTERFACE_ENTRY(IExtendUIEventSink)
END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()
   DECLARE_EAF_AGENT_DATA;

	HRESULT FinalConstruct();
	void FinalRelease()
	{
	}

   CMyCmdTarget m_MyCommandTarget;

   DWORD m_dwExtendUICookie;

   CBitmap m_bmpMenu;

   CEAFMenu* m_pMyMenu;
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

   CString m_Answer; // sample data for persistance

// IAgentEx
public:
   STDMETHOD(SetBroker)(IBroker* pBroker) override;
   STDMETHOD(RegInterfaces)() override;
   STDMETHOD(Init)() override;
   STDMETHOD(Init2)() override;
   STDMETHOD(Reset)() override;
   STDMETHOD(ShutDown)() override;
   STDMETHOD(GetClassID)(CLSID* pCLSID) override;

// IAgentPersist
public:
   STDMETHOD(Load)(/*[in]*/ IStructuredLoad* pStrLoad) override;
   STDMETHOD(Save)(/*[in]*/ IStructuredSave* pStrSave) override;

// IAgentUIIntegration
public:
   STDMETHOD(IntegrateWithUI)(BOOL bIntegrate) override;

// IAgentReportingIntegration
public:
   STDMETHOD(IntegrateWithReporting)(BOOL bIntegrate) override;

// IAgentGraphingIntegration
public:
   STDMETHOD(IntegrateWithGraphing)(BOOL bIntegrate) override;


// IEAFCommandCallback
public:
   virtual BOOL OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo) override;
   virtual BOOL GetStatusBarMessageString(UINT nID, CString& rMessage) const override;
   virtual BOOL GetToolTipMessageString(UINT nID, CString& rMessage) const override;


// IEditBridgeCallback
public:
   virtual CPropertyPage* CreatePropertyPage(IEditBridgeData* pBridgeData) override;
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPage,IEditBridgeData* pBridgeData) override;
   virtual void EditPier_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pPierPropertyPage) override;
   virtual void EditTemporarySupport_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pTempSupportPropertyPage) override;
   virtual void EditSpan_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pSpanPropertyPage) override;

// IEditPierCallback
public:
   virtual CPropertyPage* CreatePropertyPage(IEditPierData* pEditPierData) override;
   virtual CPropertyPage* CreatePropertyPage(IEditPierData* pEditPierData,CPropertyPage* pBridgePropertyPage) override;
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPage,IEditPierData* pEditPierData) override;
   virtual IDType GetEditBridgeCallbackID() override;

// IEditTemporarySupportCallback
public:
   virtual CPropertyPage* CreatePropertyPage(IEditTemporarySupportData* pEditTemporarySupportData) override;
   virtual CPropertyPage* CreatePropertyPage(IEditTemporarySupportData* pEditTemporarySupportData,CPropertyPage* pBridgePropertyPage) override;
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPage,IEditTemporarySupportData* pEditTemporarySupportData) override;
   //virtual IDType GetEditBridgeCallbackID() override;

// IEditSpanCallback
public:
   virtual CPropertyPage* CreatePropertyPage(IEditSpanData* pEditSpanData) override;
   virtual CPropertyPage* CreatePropertyPage(IEditSpanData* pEditSpanData,CPropertyPage* pBridgePropertyPage) override;
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPage,IEditSpanData* pEditSpanData) override;
   //virtual IDType GetEditBridgeCallbackID() override;

// IEditSegmentCallback
public:
   virtual CPropertyPage* CreatePropertyPage(IEditSegmentData* pSegmentData) override;
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPage,IEditSegmentData* pSegmentData) override;
   virtual IDType GetEditSplicedGirderCallbackID() override;
   virtual CPropertyPage* CreatePropertyPage(IEditSegmentData* pEditSegmentData,CPropertyPage* pSplicedGirderPropertyPage) override;

// IEditClosureJointCallback
public:
   virtual CPropertyPage* CreatePropertyPage(IEditClosureJointData* pClosureJointData) override;
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPage,IEditClosureJointData* pClosureJointData) override;
   //virtual IDType GetEditSplicedGirderCallbackID() override;
   virtual CPropertyPage* CreatePropertyPage(IEditClosureJointData* pEditClosureJointData,CPropertyPage* pSplicedGirderPropertyPage) override;

// IEditSplicedGirderCallback
public:
   virtual CPropertyPage* CreatePropertyPage(IEditSplicedGirderData* pGirderData) override;
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPage,IEditSplicedGirderData* pGirderData) override;
   virtual void EditSegment_OnOK(CPropertyPage* pSplicedGirderPropertyPage,CPropertyPage* pSegmentPropertyPage) override;
   virtual void EditClosureJoint_OnOK(CPropertyPage* pSplicedGirderPropertyPage,CPropertyPage* pClosureJointPropertyPage) override;

// IEditGirderCallback
public:
   virtual CPropertyPage* CreatePropertyPage(IEditGirderData* pGirderData) override;
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPage,IEditGirderData* pGirderData) override;


// IExtendUIEventSink
public:
   virtual HRESULT OnHintsReset() override;

private:
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
   bool m_bCheck; // dummy data
};

OBJECT_ENTRY_AUTO(__uuidof(ExampleExtensionAgent), CExampleExtensionAgent)
