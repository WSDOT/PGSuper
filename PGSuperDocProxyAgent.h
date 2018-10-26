///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <EAF\EAFDisplayUnits.h>

#include "PGSuperStatusBar.h"
#include <EAF\EAFStatusBar.h>


class CPGSuperDoc;
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


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 11.01.1998 : Created file
*****************************************************************************/

class CPGSuperDocProxyAgent :
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CPGSuperDocProxyAgent,&CLSID_PGSuperDocProxyAgent>,
   public IAgentEx,
   public IAgentUIIntegration,
   public IBridgeDescriptionEventSink,
   public IEnvironmentEventSink,
   public IProjectPropertiesEventSink,
   public IEAFDisplayUnitsEventSink,
   public ISpecificationEventSink,
   public IRatingSpecificationEventSink,
   public ILoadModifiersEventSink,
   public ILibraryConflictEventSink,
   public IUIEvents,
   public IUpdateTemplates,
   //public ISelection,
   public ISelectionEx,
   public IEditByUIEx,
   public IDesign,
   public IViews,
   public IVersionInfo,
   public IRegisterViewEvents,
   public IExtendUI
{
public:
   CPGSuperDocProxyAgent();
   ~CPGSuperDocProxyAgent();

BEGIN_COM_MAP(CPGSuperDocProxyAgent)
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
   COM_INTERFACE_ENTRY(ILibraryConflictEventSink)
   COM_INTERFACE_ENTRY(IUIEvents)
   COM_INTERFACE_ENTRY(IUpdateTemplates)
   COM_INTERFACE_ENTRY(ISelection)
   COM_INTERFACE_ENTRY(ISelectionEx)
   COM_INTERFACE_ENTRY(IEditByUI)
   COM_INTERFACE_ENTRY(IEditByUIEx)
   COM_INTERFACE_ENTRY(IDesign)
   COM_INTERFACE_ENTRY(IViews)
   COM_INTERFACE_ENTRY(IVersionInfo)
   COM_INTERFACE_ENTRY(IRegisterViewEvents)
   COM_INTERFACE_ENTRY(IExtendUI)
END_COM_MAP()

public:
   void SetDocument(CPGSuperDoc* pDoc);
   void OnStatusChanged();
   long GetReportViewKey();

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
   virtual HRESULT OnBridgeChanged(CBridgeChangedHint* pHint);
   virtual HRESULT OnGirderFamilyChanged();
   virtual HRESULT OnGirderChanged(SpanIndexType span,GirderIndexType gdr,Uint32 lHint);
   virtual HRESULT OnLiveLoadChanged();
   virtual HRESULT OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName);
   virtual HRESULT OnConstructionLoadChanged();

// IEnvironmentEventSink
public:
   virtual HRESULT OnExposureConditionChanged();
   virtual HRESULT OnRelHumidityChanged();

// IProjectPropertiesEventSink
public:
   virtual HRESULT OnProjectPropertiesChanged();

// IEAFDisplayUnitsEventSink
public:
   virtual HRESULT OnUnitsChanged(eafTypes::UnitMode newUnitsMode);

// ISpecificationEventSink
public:
   virtual HRESULT OnSpecificationChanged();
   virtual HRESULT OnAnalysisTypeChanged();

// IRatingSpecificationEventSink
public:
   virtual HRESULT OnRatingSpecificationChanged();

// ILoadModifersEventSink
public:
   virtual HRESULT OnLoadModifiersChanged();

// ILibraryConflictSink
public:
   virtual HRESULT OnLibraryConflictResolved();

// IUpdateTemplates
public:
   virtual bool UpdatingTemplates();

// IUIEvents
public:
   virtual void HoldEvents(bool bHold=true);
   virtual void FirePendingEvents();

// IVersionInfo
public:
   virtual CString GetVersionString(bool bIncludeBuildNumber=false);
   virtual CString GetVersion(bool bIncludeBuildNumber=false);


// ISelection
public:
   virtual PierIndexType GetPierIdx();
   virtual SpanIndexType GetSpanIdx();
   virtual GirderIndexType GetGirderIdx();
   virtual void SelectPier(PierIndexType pierIdx);
   virtual void SelectSpan(SpanIndexType spanIdx);
   virtual void SelectGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetSectionCutStation();

// ISelectionEx
public:
   virtual CSelection GetSelection();
   virtual void SelectDeck();
   virtual void SelectAlignment();
   virtual void ClearSelection();

// IEditByUI
public:
   virtual void EditBridgeDescription(int nPage);
   virtual void EditAlignmentDescription(int nPage);
   virtual bool EditGirderDescription(SpanIndexType span,GirderIndexType girder, int nPage);
   virtual bool EditSpanDescription(SpanIndexType spanIdx, int nPage);
   virtual bool EditPierDescription(PierIndexType pierIdx, int nPage);
   virtual void EditLiveLoads();
   virtual void EditLiveLoadDistributionFactors(pgsTypes::DistributionFactorMethod method,LldfRangeOfApplicabilityAction roaAction);
   virtual bool EditPointLoad(CollectionIndexType loadIdx);
   virtual bool EditDistributedLoad(CollectionIndexType loadIdx);
   virtual bool EditMomentLoad(CollectionIndexType loadIdx);
   virtual UINT GetStdToolBarID();
   virtual UINT GetLibToolBarID();
   virtual UINT GetHelpToolBarID();
   virtual bool EditDirectInputPrestressing(SpanIndexType span,GirderIndexType girder);

// IEditByUIEx
public:
   virtual void AddPointLoad(const CPointLoadData& loadData);
   virtual void DeletePointLoad(CollectionIndexType loadIdx);
   virtual void AddDistributedLoad(const CDistributedLoadData& loadData);
   virtual void DeleteDistributedLoad(CollectionIndexType loadIdx);
   virtual void AddMomentLoad(const CMomentLoadData& loadData);
   virtual void DeleteMomentLoad(CollectionIndexType loadIdx);
   virtual void EditEffectiveFlangeWidth();

// IDesign
public:
   virtual void DesignGirder(bool bPrompt,bool bDesignSlabOffset,SpanIndexType spanIdx,GirderIndexType gdrIdx);

// IViews
public:
   virtual void CreateGirderView(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual void CreateBridgeModelView();
   virtual void CreateAnalysisResultsView(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual void CreateStabilityView(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual void CreateLoadsView();
   virtual void CreateLibraryEditorView();
   virtual void CreateReportView(CollectionIndexType rptIdx,bool bPromptForSpec=true);
   virtual void BuildReportMenu(CEAFMenu* pMenu,bool bQuickReport);

// IRegisterViewEvents
public:
   virtual IDType RegisterBridgePlanViewCallback(IBridgePlanViewEventCallback* pCallback);
   virtual IDType RegisterBridgeSectionViewCallback(IBridgeSectionViewEventCallback* pCallback);
   virtual IDType RegisterGirderElevationViewCallback(IGirderElevationViewEventCallback* pCallback);
   virtual IDType RegisterGirderSectionViewCallback(IGirderSectionViewEventCallback* pCallback);
   virtual bool UnregisterBridgePlanViewCallback(IDType ID);
   virtual bool UnregisterBridgeSectionViewCallback(IDType ID);
   virtual bool UnregisterGirderElevationViewCallback(IDType ID);
   virtual bool UnregisterGirderSectionViewCallback(IDType ID);

// IExtendUI
public:
   virtual IDType RegisterEditPierCallback(IEditPierCallback* pCallback);
   virtual IDType RegisterEditSpanCallback(IEditSpanCallback* pCallback);
   virtual IDType RegisterEditGirderCallback(IEditGirderCallback* pCallback);
   virtual IDType RegisterEditBridgeCallback(IEditBridgeCallback* pCallback);
   virtual bool UnregisterEditPierCallback(IDType ID);
   virtual bool UnregisterEditSpanCallback(IDType ID);
   virtual bool UnregisterEditGirderCallback(IDType ID);
   virtual bool UnregisterEditBridgeCallback(IDType ID);


private:
   DECLARE_AGENT_DATA;

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

   bool m_bHoldingEvents;
   struct UIEvent
   {
      CView* pSender;
      LPARAM lHint;
      boost::shared_ptr<CObject> pHint;
   };
   std::vector<UIEvent> m_UIEvents;
   void FireEvent(CView* pSender,LPARAM lHint,boost::shared_ptr<CObject> pHint);


   // look up keys for extra views associated with our document
   void RegisterViews();
   void UnregisterViews();
   long m_BridgeModelEditorViewKey;
   long m_GirderModelEditorViewKey;
   long m_LibraryEditorViewKey;
   long m_AnalysisResultsViewKey;
   long m_ReportViewKey;
   long m_FactorOfSafetyViewKey;
   long m_LoadsViewKey;
   
   CPGSuperDoc* m_pPGSuperDoc;

   UINT m_StdToolBarID;
   UINT m_LibToolBarID;
   UINT m_HelpToolBarID;

   void CreateToolBars();
   void RemoveToolBars();

   void CreateAcceleratorKeys();
   void RemoveAcceleratorKeys();

   void CreateStatusBar();
   void ResetStatusBar();

#if defined _EAF_USING_MFC_FEATURE_PACK
   CMFCTasksPane m_wndTasks;
   CDockablePaneAdapter m_wndLoadsViewAdapter;
#endif
};

