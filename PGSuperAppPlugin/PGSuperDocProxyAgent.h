///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
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

#include <EAF\Agent.h>


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

#include "Reporting\ReporterEvents.h"

class CPGSDocBase;
class CBridgeModelViewChildFrame;

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

class CPGSuperDocProxyAgent : public WBFL::EAF::Agent,
   public CProxyIExtendUIEventSink<IExtendUIEventSink>,
   public WBFL::EAF::IAgentUIIntegration,
   public IBridgeDescriptionEventSink,
   public IEnvironmentEventSink,
   public IProjectPropertiesEventSink,
   public IEAFDisplayUnitsEventSink,
   public ISpecificationEventSink,
   public IRatingSpecificationEventSink,
   public ILoadModifiersEventSink,
   public ILossParametersEventSink,
   public ILibraryConflictEventSink,
   public IReporterEventSink,
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
   CPGSuperDocProxyAgent(CPGSDocBase* pDoc);
   ~CPGSuperDocProxyAgent();

   void OnStatusChanged();

   void OnUIHintsReset();

// IAgentEx
public:
   std::_tstring GetName() const override { return _T("DocProxyAgent"); }
   bool RegisterInterfaces() override;
   bool Init() override;
   bool Reset() override;
	bool ShutDown() override;
   CLSID GetCLSID() const override;

// IAgentUIIntegration
public:
   bool IntegrateWithUI(bool bIntegrate) override;


// IBridgeDescriptionEventSink
public:
   HRESULT OnBridgeChanged(CBridgeChangedHint* pHint) override;
   HRESULT OnGirderFamilyChanged() override;
   HRESULT OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint) override;
   HRESULT OnLiveLoadChanged() override;
   HRESULT OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName) override;
   HRESULT OnConstructionLoadChanged() override;

// IEnvironmentEventSink
public:
   HRESULT OnExposureConditionChanged() override;
   HRESULT OnClimateConditionChanged() override;
   HRESULT OnRelHumidityChanged() override;

// IProjectPropertiesEventSink
public:
   HRESULT OnProjectPropertiesChanged() override;

// IEAFDisplayUnitsEventSink
public:
   HRESULT OnUnitsChanging() override;
   HRESULT OnUnitsChanged(WBFL::EAF::UnitMode newUnitsMode) override;

// ISpecificationEventSink
public:
   HRESULT OnSpecificationChanged() override;
   HRESULT OnAnalysisTypeChanged() override;

   // IReporterEventSink
public:
	HRESULT OnReportsChanged() override;

// IRatingSpecificationEventSink
public:
   HRESULT OnRatingSpecificationChanged() override;

// ILoadModifiersEventSink
public:
   HRESULT OnLoadModifiersChanged() override;

// ILossParametersEventSink
public:
   HRESULT OnLossParametersChanged() override;

// ILibraryConflictSink
public:
   HRESULT OnLibraryConflictResolved() override;

// IUpdateTemplates
public:
   bool UpdatingTemplates() override;

// IUIEvents
public:
   void HoldEvents(bool bHold=true) override;
   void FirePendingEvents() override;
   void CancelPendingEvents() override;
   void FireEvent(CView* pSender = nullptr,LPARAM lHint = 0,std::shared_ptr<CObject> pHint = nullptr) override;

// IVersionInfo
public:
   CString GetVersionString(bool bIncludeBuildNumber=false) override;
   CString GetVersion(bool bIncludeBuildNumber=false) override;


// ISelection
public:
   CSelection GetSelection() override;
   void ClearSelection() override;
   PierIndexType GetSelectedPier() override;
   SpanIndexType GetSelectedSpan() override;
   CGirderKey GetSelectedGirder() override;
   CSegmentKey GetSelectedSegment() override;
   CClosureKey GetSelectedClosureJoint() override;
   SupportIDType GetSelectedTemporarySupport() override;
   bool IsDeckSelected() override;
   bool IsAlignmentSelected() override;
   bool IsRailingSystemSelected(pgsTypes::TrafficBarrierOrientation orientation) override;
   void SelectPier(PierIndexType pierIdx) override;
   void SelectSpan(SpanIndexType spanIdx) override;
   void SelectGirder(const CGirderKey& girderKey) override;
   void SelectSegment(const CSegmentKey& segmentKey) override;
   void SelectClosureJoint(const CClosureKey& closureKey) override;
   void SelectTemporarySupport(SupportIDType tsID) override;
   void SelectDeck() override;
   void SelectAlignment() override;
   void SelectRailingSystem(pgsTypes::TrafficBarrierOrientation orientation) override;
   Float64 GetSectionCutStation() override;

// IEditByUI
public:
   void EditBridgeDescription(int nPage) override;
   void EditAlignmentDescription(int nPage) override;
   bool EditSegmentDescription(const CSegmentKey& segmentKey, int nPage) override;
   bool EditSegmentDescription() override;
   bool EditClosureJointDescription(const CClosureKey& closureKey, int nPage) override;
   bool EditGirderDescription(const CGirderKey& girderKey, int nPage) override;
   bool EditGirderDescription() override;
   bool EditSpanDescription(SpanIndexType spanIdx, int nPage) override;
   bool EditPierDescription(PierIndexType pierIdx, int nPage) override;
   bool EditTemporarySupportDescription(PierIndexType pierIdx, int nPage) override; 
   void EditLiveLoads() override;
   void EditLiveLoadDistributionFactors(pgsTypes::DistributionFactorMethod method,WBFL::LRFD::RangeOfApplicabilityAction roaAction) override;
   bool EditPointLoad(IndexType loadIdx) override;
   bool EditPointLoadByID(LoadIDType loadID) override;
   bool EditDistributedLoad(IndexType loadIdx) override;
   bool EditDistributedLoadByID(LoadIDType loadID) override;
   bool EditMomentLoad(IndexType loadIdx) override;
   bool EditMomentLoadByID(LoadIDType loadID) override;
   bool EditTimeline() override;
   bool EditCastDeckActivity() override;
   UINT GetStdToolBarID() override;
   UINT GetLibToolBarID() override;
   UINT GetHelpToolBarID() override;
   bool EditDirectSelectionPrestressing(const CSegmentKey& segmentKey) override;
   bool EditDirectRowInputPrestressing(const CSegmentKey& segmentKey) override;
   bool EditDirectStrandInputPrestressing(const CSegmentKey& segmentKey) override;
   void AddPointLoad(const CPointLoadData& loadData) override;
   void DeletePointLoad(IndexType loadIdx) override;
   void AddDistributedLoad(const CDistributedLoadData& loadData) override;
   void DeleteDistributedLoad(IndexType loadIdx) override;
   void AddMomentLoad(const CMomentLoadData& loadData) override;
   void DeleteMomentLoad(IndexType loadIdx) override;
   bool EditEffectiveFlangeWidth() override;
   bool SelectProjectCriteria() override;
   bool EditBearings() override;
   bool EditHaunch() override;

// IDesign
public:
   void DesignGirder(bool bPrompt,bool bDesignSlabOffset,const CGirderKey& girderKey) override;
   bool DesignHaunch(const CGirderKey& girderKey) override;

// IViews
public:
   void CreateBridgeModelView(IBridgeModelViewController** ppViewController=nullptr) override;
   void CreateGirderView(const CGirderKey& girderKey, IGirderModelViewController** ppViewController = nullptr) override;
   void CreateLoadsView(ILoadsViewController** ppViewController = nullptr) override;
   void CreateLibraryEditorView() override;
   void CreateReportView(IndexType rptIdx,BOOL bPromptForSpec=TRUE) override;
   void CreateGraphView(IndexType graphIdx, IEAFViewController** ppViewController = nullptr) override;
   void CreateGraphView(LPCTSTR lpszGraph, IEAFViewController** ppViewController = nullptr) override;
   void BuildReportMenu(std::shared_ptr<WBFL::EAF::Menu> pMenu, bool bQuickReport) override;
   void BuildGraphMenu(std::shared_ptr<WBFL::EAF::Menu> pMenu) override;
   long GetBridgeModelEditorViewKey() override;
   long GetGirderModelEditorViewKey() override;
   long GetLibraryEditorViewKey() override;
   long GetReportViewKey() override;
   long GetGraphingViewKey() override;
   long GetLoadsViewKey() override;

// IRegisterViewEvents
public:
   IDType RegisterBridgePlanViewCallback(IBridgePlanViewEventCallback* pCallback) override;
   IDType RegisterBridgeSectionViewCallback(IBridgeSectionViewEventCallback* pCallback) override;
   IDType RegisterAlignmentPlanViewCallback(IAlignmentPlanViewEventCallback* pCallback) override;
   IDType RegisterAlignmentProfileViewCallback(IAlignmentProfileViewEventCallback* pCallback) override;
   IDType RegisterGirderElevationViewCallback(IGirderElevationViewEventCallback* pCallback) override;
   IDType RegisterGirderSectionViewCallback(IGirderSectionViewEventCallback* pCallback) override;
   bool UnregisterBridgePlanViewCallback(IDType ID) override;
   bool UnregisterBridgeSectionViewCallback(IDType ID) override;
   bool UnregisterAlignmentPlanViewCallback(IDType ID) override;
   bool UnregisterAlignmentProfileViewCallback(IDType ID) override;
   bool UnregisterGirderElevationViewCallback(IDType ID) override;
   bool UnregisterGirderSectionViewCallback(IDType ID) override;

// IExtendUI
public:
   IDType RegisterEditPierCallback(IEditPierCallback* pCallback,ICopyPierPropertiesCallback* pCopyCallback) override;
   IDType RegisterEditSpanCallback(IEditSpanCallback* pCallback) override;
   IDType RegisterEditBridgeCallback(IEditBridgeCallback* pCallback) override;
   IDType RegisterEditLoadRatingOptionsCallback(IEditLoadRatingOptionsCallback* pCallback) override;
   bool UnregisterEditPierCallback(IDType ID) override;
   bool UnregisterEditSpanCallback(IDType ID) override;
   bool UnregisterEditBridgeCallback(IDType ID) override;
   bool UnregisterEditLoadRatingOptionsCallback(IDType ID) override;

// IExtendPGSuperUI
public:
   IDType RegisterEditGirderCallback(IEditGirderCallback* pCallback,ICopyGirderPropertiesCallback* pCopyCallback) override;
   bool UnregisterEditGirderCallback(IDType ID) override;

// IExtendPGSpliceUI
public:
   IDType RegisterEditTemporarySupportCallback(IEditTemporarySupportCallback* pCallback, ICopyTemporarySupportPropertiesCallback* pCopyCallBack) override;
   IDType RegisterEditSplicedGirderCallback(IEditSplicedGirderCallback* pCallback,ICopyGirderPropertiesCallback* pCopyCallback) override;
   IDType RegisterEditSegmentCallback(IEditSegmentCallback* pCallback) override;
   IDType RegisterEditClosureJointCallback(IEditClosureJointCallback* pCallback) override;
   bool UnregisterEditTemporarySupportCallback(IDType ID) override;
   bool UnregisterEditSplicedGirderCallback(IDType ID) override;
   bool UnregisterEditSegmentCallback(IDType ID) override;
   bool UnregisterEditClosureJointCallback(IDType ID) override;

// IDocumentType
public:
   bool IsPGSuperDocument() const override;
   bool IsPGSpliceDocument() const override;

// IDocumentUnitSystem
public:
   void GetUnitServer(IUnitServer** ppUnitServer) override;

private:
   EAF_DECLARE_AGENT_DATA;

   void AdviseEventSinks();
   void UnadviseEventSinks();

   IDType m_BridgeDescCookie;
   IDType m_EnvironmentCookie;
   IDType m_ProjectPropertiesCookie;
   IDType m_DisplayUnitsCookie;
   IDType m_SpecificationCookie;
   IDType m_RatingSpecificationCookie;
   IDType m_LoadModiferCookie;
   IDType m_LibraryConflictGuiCookie;
   IDType m_LossParametersCookie;
   IDType m_ReportCookie;

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

