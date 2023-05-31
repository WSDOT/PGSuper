///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include <EAF\EAFInterfaceCache.h>

#include <IFace\Project.h>
#include <IFace\UpdateTemplates.h>
#include <IFace\Selection.h>
#include <IFace\EditByUI.h>
#include <IFace\Design.h>
#include <IFace\RatingSpecification.h>
#include <IFace\VersionInfo.h>
#include <IFace\Views.h>
#include <IFace\ViewEvents.h>
#include <IFace\ExtendUI.h>
#include <IFace\DocumentType.h>
#include <EAF\EAFDisplayUnits.h>

#include "PGSuperStatusBar.h"
#include <EAF\EAFStatusBar.h>

#include "CPPGSuperDocProxyAgent.h"

class CPGSDocBase;
class CBridgeModelViewChildFrame;
struct IBroker;

// {71D5ABEE-23D6-4593-BB8D-20B092CB2E9A}
DEFINE_GUID(CLSID_PGSuperDocProxyAgent, 
0x71d5abee, 0x23d6, 0x4593, 0xbb, 0x8d, 0x20, 0xb0, 0x92, 0xcb, 0x2e, 0x9a);

/*****************************************************************************
CLASS 
   CPGSuperDocProxyAgent

   Proxy agent for CPGSuper document.


DESCRIPTION
   Proxy agent for CPGSuper document.

   Instances of this object allow the CDocument class to be plugged into the
   Agent-Broker architecture.

LOG
   rab : 11.01.1998 : Created file
*****************************************************************************/

class CPGSuperDocProxyAgent :
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CPGSuperDocProxyAgent,&CLSID_PGSuperDocProxyAgent>,
	public IConnectionPointContainerImpl<CPGSuperDocProxyAgent>,
   public CProxyIExtendUIEventSink<CPGSuperDocProxyAgent>,
   public IAgentEx,
   public IAgentUIIntegration,
   public IBridgeDescriptionEventSink,
   public IEnvironmentEventSink,
   public IProjectPropertiesEventSink,
   public IEAFDisplayUnitsEventSink,
   public ISpecificationEventSink,
   public IRatingSpecificationEventSink,
   public ILoadModifiersEventSink,
   public ILossParametersEventSink,
   public ILibraryConflictEventSink,
   public IUIEvents,
   public IUpdateTemplates,
   public ISelection,
   public IEditByUI,
   public IDesign,
   public IViews,
   public IVersionInfo,
   public IRegisterViewEvents,
   public IExtendPGSuperUI,
   public IExtendPGSpliceUI,
   public IDocumentType,
   public IDocumentUnitSystem
{
public:
   CPGSuperDocProxyAgent();
   ~CPGSuperDocProxyAgent();

BEGIN_COM_MAP(CPGSuperDocProxyAgent)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
   COM_INTERFACE_ENTRY(IAgent)
   COM_INTERFACE_ENTRY(IAgentEx)
   COM_INTERFACE_ENTRY(IAgentUIIntegration)
   COM_INTERFACE_ENTRY(IBridgeDescriptionEventSink)
   COM_INTERFACE_ENTRY(IEnvironmentEventSink)
   COM_INTERFACE_ENTRY(IProjectPropertiesEventSink)
   COM_INTERFACE_ENTRY(IEAFDisplayUnitsEventSink)
   COM_INTERFACE_ENTRY(ISpecificationEventSink)
   COM_INTERFACE_ENTRY(IRatingSpecificationEventSink)
   COM_INTERFACE_ENTRY(ILoadModifiersEventSink)
   COM_INTERFACE_ENTRY(ILossParametersEventSink)
   COM_INTERFACE_ENTRY(ILibraryConflictEventSink)
   COM_INTERFACE_ENTRY(IUIEvents)
   COM_INTERFACE_ENTRY(IUpdateTemplates)
   COM_INTERFACE_ENTRY(ISelection)
   COM_INTERFACE_ENTRY(IEditByUI)
   COM_INTERFACE_ENTRY(IDesign)
   COM_INTERFACE_ENTRY(IViews)
   COM_INTERFACE_ENTRY(IVersionInfo)
   COM_INTERFACE_ENTRY(IRegisterViewEvents)
   COM_INTERFACE_ENTRY(IExtendPGSuperUI)
   COM_INTERFACE_ENTRY(IExtendPGSpliceUI)
   COM_INTERFACE_ENTRY(IDocumentType)
   COM_INTERFACE_ENTRY(IDocumentUnitSystem)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CPGSuperDocProxyAgent)
   CONNECTION_POINT_ENTRY( IID_IExtendUIEventSink )
END_CONNECTION_POINT_MAP()

public:
   void SetDocument(CPGSDocBase* pDoc);
   void OnStatusChanged();

   void OnUIHintsReset();

// IAgentEx
public:
   STDMETHOD(SetBroker)(/*[in]*/ IBroker* pBroker);
	STDMETHOD(RegInterfaces)();
	STDMETHOD(Init)();
	STDMETHOD(Init2)();
	STDMETHOD(Reset)();
	STDMETHOD(ShutDown)();
   STDMETHOD(GetClassID)(CLSID* pCLSID);

// IAgentUIIntegration
public:
   STDMETHOD(IntegrateWithUI)(BOOL bIntegrate);


// IBridgeDescriptionEventSink
public:
   virtual HRESULT OnBridgeChanged(CBridgeChangedHint* pHint) override;
   virtual HRESULT OnGirderFamilyChanged() override;
   virtual HRESULT OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint) override;
   virtual HRESULT OnLiveLoadChanged() override;
   virtual HRESULT OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName) override;
   virtual HRESULT OnConstructionLoadChanged() override;

// IEnvironmentEventSink
public:
   virtual HRESULT OnExposureConditionChanged() override;
   virtual HRESULT OnRelHumidityChanged() override;

// IProjectPropertiesEventSink
public:
   virtual HRESULT OnProjectPropertiesChanged() override;

// IEAFDisplayUnitsEventSink
public:
   virtual HRESULT OnUnitsChanging() override;
   virtual HRESULT OnUnitsChanged(eafTypes::UnitMode newUnitsMode) override;

// ISpecificationEventSink
public:
   virtual HRESULT OnSpecificationChanged() override;
   virtual HRESULT OnAnalysisTypeChanged() override;

// IRatingSpecificationEventSink
public:
   virtual HRESULT OnRatingSpecificationChanged() override;

// ILoadModifersEventSink
public:
   virtual HRESULT OnLoadModifiersChanged() override;

// ILossParametersEventSink
public:
   virtual HRESULT OnLossParametersChanged() override;

// ILibraryConflictSink
public:
   virtual HRESULT OnLibraryConflictResolved() override;

// IUpdateTemplates
public:
   virtual bool UpdatingTemplates() override;

// IUIEvents
public:
   virtual void HoldEvents(bool bHold=true) override;
   virtual void FirePendingEvents() override;
   virtual void CancelPendingEvents() override;
   virtual void FireEvent(CView* pSender = nullptr,LPARAM lHint = 0,std::shared_ptr<CObject> pHint = nullptr) override;

// IVersionInfo
public:
   virtual CString GetVersionString(bool bIncludeBuildNumber=false) override;
   virtual CString GetVersion(bool bIncludeBuildNumber=false) override;


// ISelection
public:
   virtual CSelection GetSelection() override;
   virtual void ClearSelection() override;
   virtual PierIndexType GetSelectedPier() override;
   virtual SpanIndexType GetSelectedSpan() override;
   virtual CGirderKey GetSelectedGirder() override;
   virtual CSegmentKey GetSelectedSegment() override;
   virtual CClosureKey GetSelectedClosureJoint() override;
   virtual SupportIDType GetSelectedTemporarySupport() override;
   virtual bool IsDeckSelected() override;
   virtual bool IsAlignmentSelected() override;
   virtual bool IsRailingSystemSelected(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual void SelectPier(PierIndexType pierIdx) override;
   virtual void SelectSpan(SpanIndexType spanIdx) override;
   virtual void SelectGirder(const CGirderKey& girderKey) override;
   virtual void SelectSegment(const CSegmentKey& segmentKey) override;
   virtual void SelectClosureJoint(const CClosureKey& closureKey) override;
   virtual void SelectTemporarySupport(SupportIDType tsID) override;
   virtual void SelectDeck() override;
   virtual void SelectAlignment() override;
   virtual void SelectRailingSystem(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual Float64 GetSectionCutStation() override;

// IEditByUI
public:
   virtual void EditBridgeDescription(int nPage) override;
   virtual void EditAlignmentDescription(int nPage) override;
   virtual bool EditSegmentDescription(const CSegmentKey& segmentKey, int nPage) override;
   virtual bool EditSegmentDescription() override;
   virtual bool EditClosureJointDescription(const CClosureKey& closureKey, int nPage) override;
   virtual bool EditGirderDescription(const CGirderKey& girderKey, int nPage) override;
   virtual bool EditGirderDescription() override;
   virtual bool EditSpanDescription(SpanIndexType spanIdx, int nPage) override;
   virtual bool EditPierDescription(PierIndexType pierIdx, int nPage) override;
   virtual bool EditTemporarySupportDescription(PierIndexType pierIdx, int nPage) override; 
   virtual void EditLiveLoads() override;
   virtual void EditLiveLoadDistributionFactors(pgsTypes::DistributionFactorMethod method,LldfRangeOfApplicabilityAction roaAction) override;
   virtual bool EditPointLoad(CollectionIndexType loadIdx) override;
   virtual bool EditPointLoadByID(LoadIDType loadID) override;
   virtual bool EditDistributedLoad(CollectionIndexType loadIdx) override;
   virtual bool EditDistributedLoadByID(LoadIDType loadID) override;
   virtual bool EditMomentLoad(CollectionIndexType loadIdx) override;
   virtual bool EditMomentLoadByID(LoadIDType loadID) override;
   virtual bool EditTimeline() override;
   virtual bool EditCastDeckActivity() override;
   virtual UINT GetStdToolBarID() override;
   virtual UINT GetLibToolBarID() override;
   virtual UINT GetHelpToolBarID() override;
   virtual bool EditDirectSelectionPrestressing(const CSegmentKey& segmentKey) override;
   virtual bool EditDirectRowInputPrestressing(const CSegmentKey& segmentKey) override;
   virtual bool EditDirectStrandInputPrestressing(const CSegmentKey& segmentKey) override;
   virtual void AddPointLoad(const CPointLoadData& loadData) override;
   virtual void DeletePointLoad(CollectionIndexType loadIdx) override;
   virtual void AddDistributedLoad(const CDistributedLoadData& loadData) override;
   virtual void DeleteDistributedLoad(CollectionIndexType loadIdx) override;
   virtual void AddMomentLoad(const CMomentLoadData& loadData) override;
   virtual void DeleteMomentLoad(CollectionIndexType loadIdx) override;
   virtual bool EditEffectiveFlangeWidth() override;
   virtual bool SelectProjectCriteria() override;
   virtual bool EditBearings() override;
   virtual bool EditHaunch() override;

// IDesign
public:
   virtual void DesignGirder(bool bPrompt,bool bDesignSlabOffset,const CGirderKey& girderKey) override;

// IViews
public:
   virtual void CreateBridgeModelView(IBridgeModelViewController** ppViewController=nullptr) override;
   virtual void CreateGirderView(const CGirderKey& girderKey, IGirderModelViewController** ppViewController = nullptr) override;
   virtual void CreateLoadsView(ILoadsViewController** ppViewController = nullptr) override;
   virtual void CreateLibraryEditorView() override;
   virtual void CreateReportView(CollectionIndexType rptIdx,BOOL bPromptForSpec=TRUE) override;
   virtual void CreateGraphView(CollectionIndexType graphIdx, IEAFViewController** ppViewController = nullptr) override;
   virtual void CreateGraphView(LPCTSTR lpszGraph, IEAFViewController** ppViewController = nullptr) override;
   virtual void BuildReportMenu(CEAFMenu* pMenu, bool bQuickReport) override;
   virtual void BuildGraphMenu(CEAFMenu* pMenu) override;
   virtual long GetBridgeModelEditorViewKey() override;
   virtual long GetGirderModelEditorViewKey() override;
   virtual long GetLibraryEditorViewKey() override;
   virtual long GetReportViewKey() override;
   virtual long GetGraphingViewKey() override;
   virtual long GetLoadsViewKey() override;

// IRegisterViewEvents
public:
   virtual IDType RegisterBridgePlanViewCallback(IBridgePlanViewEventCallback* pCallback) override;
   virtual IDType RegisterBridgeSectionViewCallback(IBridgeSectionViewEventCallback* pCallback) override;
   virtual IDType RegisterAlignmentPlanViewCallback(IAlignmentPlanViewEventCallback* pCallback) override;
   virtual IDType RegisterAlignmentProfileViewCallback(IAlignmentProfileViewEventCallback* pCallback) override;
   virtual IDType RegisterGirderElevationViewCallback(IGirderElevationViewEventCallback* pCallback) override;
   virtual IDType RegisterGirderSectionViewCallback(IGirderSectionViewEventCallback* pCallback) override;
   virtual bool UnregisterBridgePlanViewCallback(IDType ID) override;
   virtual bool UnregisterBridgeSectionViewCallback(IDType ID) override;
   virtual bool UnregisterAlignmentPlanViewCallback(IDType ID) override;
   virtual bool UnregisterAlignmentProfileViewCallback(IDType ID) override;
   virtual bool UnregisterGirderElevationViewCallback(IDType ID) override;
   virtual bool UnregisterGirderSectionViewCallback(IDType ID) override;

// IExtendUI
public:
   virtual IDType RegisterEditPierCallback(IEditPierCallback* pCallback,ICopyPierPropertiesCallback* pCopyCallback) override;
   virtual IDType RegisterEditSpanCallback(IEditSpanCallback* pCallback) override;
   virtual IDType RegisterEditBridgeCallback(IEditBridgeCallback* pCallback) override;
   virtual IDType RegisterEditLoadRatingOptionsCallback(IEditLoadRatingOptionsCallback* pCallback) override;
   virtual bool UnregisterEditPierCallback(IDType ID) override;
   virtual bool UnregisterEditSpanCallback(IDType ID) override;
   virtual bool UnregisterEditBridgeCallback(IDType ID) override;
   virtual bool UnregisterEditLoadRatingOptionsCallback(IDType ID) override;

// IExtendPGSuperUI
public:
   virtual IDType RegisterEditGirderCallback(IEditGirderCallback* pCallback,ICopyGirderPropertiesCallback* pCopyCallback) override;
   virtual bool UnregisterEditGirderCallback(IDType ID) override;

// IExtendPGSpliceUI
public:
   virtual IDType RegisterEditTemporarySupportCallback(IEditTemporarySupportCallback* pCallback, ICopyTemporarySupportPropertiesCallback* pCopyCallBack) override;
   virtual IDType RegisterEditSplicedGirderCallback(IEditSplicedGirderCallback* pCallback,ICopyGirderPropertiesCallback* pCopyCallback) override;
   virtual IDType RegisterEditSegmentCallback(IEditSegmentCallback* pCallback) override;
   virtual IDType RegisterEditClosureJointCallback(IEditClosureJointCallback* pCallback) override;
   virtual bool UnregisterEditTemporarySupportCallback(IDType ID) override;
   virtual bool UnregisterEditSplicedGirderCallback(IDType ID) override;
   virtual bool UnregisterEditSegmentCallback(IDType ID) override;
   virtual bool UnregisterEditClosureJointCallback(IDType ID) override;

// IDocumentType
public:
   virtual bool IsPGSuperDocument() override;
   virtual bool IsPGSpliceDocument() override;

// IDocumentUnitSystem
public:
   virtual void GetUnitServer(IUnitServer** ppUnitServer) override;

private:
   DECLARE_EAF_AGENT_DATA;

   void AdviseEventSinks();
   void UnadviseEventSinks();

   DWORD m_dwBridgeDescCookie;
   DWORD m_dwEnvironmentCookie;
   DWORD m_dwProjectPropertiesCookie;
   DWORD m_dwDisplayUnitsCookie;
   DWORD m_dwSpecificationCookie;
   DWORD m_dwRatingSpecificationCookie;
   DWORD m_dwLoadModiferCookie;
   DWORD m_dwLibraryConflictGuiCookie;
   DWORD m_dwLossParametersCookie;

   int m_EventHoldCount;
   bool m_bFiringEvents;
   struct UIEvent
   {
      CView* pSender;
      LPARAM lHint;
      std::shared_ptr<CObject> pHint;
   };
   std::vector<UIEvent> m_UIEvents;


   // look up keys for extra views associated with our document
   void RegisterViews();
   void UnregisterViews();
   long m_BridgeModelEditorViewKey;
   long m_GirderModelEditorViewKey;
   long m_LibraryEditorViewKey;
   long m_ReportViewKey;
   long m_GraphingViewKey;
   long m_LoadsViewKey;
   
   CPGSDocBase* m_pMyDocument;

   UINT m_StdToolBarID;
   UINT m_LibToolBarID;
   UINT m_HelpToolBarID;

   void CreateToolBars();
   void RemoveToolBars();

   void CreateAcceleratorKeys();
   void RemoveAcceleratorKeys();

   void CreateStatusBar();
   void ResetStatusBar();

   CBridgeModelViewChildFrame* GetBridgeModelViewFrame();
};

