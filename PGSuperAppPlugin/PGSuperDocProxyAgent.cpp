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

#include "stdafx.h"
#include "PGSuperDocProxyAgent.h"
#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"

#include "PGSuperAppPlugin.h"
#include "PGSpliceAppPlugin.h"

#include "resource.h"

#include <IFace\Project.h>
#include <IFace\StatusCenter.h>
#include <IFace\Bridge.h>
#include <EAF\EAFUIIntegration.h>
#include <EAF\EAFStatusItem.h>

#include <PgsExt\BridgeDescription2.h>
#include "PGSuperApp.h"
#include "PGSuperDoc.h"
#include "Hints.h"

#include <EAF\EAFTxnManager.h>

#include <psgLib\psgLib.h>

#include "ChildFrm.h"
#include <PsgLib\LibChildFrm.h>

#include "BridgeModelViewChildFrame.h"
#include "BridgePlanView.h"

#include "EditLoadsChildFrm.h"
#include "EditLoadsView.h"

#include "GirderModelChildFrame.h"
#include "GirderModelElevationView.h"

#include "ReportViewChildFrame.h"
#include "ReportView.h"

#include "GraphViewChildFrame.h"
#include "GraphView.h"

#include <BridgeModelViewController.h>
#include <GirderModelViewController.h>
#include <LoadsViewController.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CPGSuperDocProxyAgent
****************************************************************************/

CPGSuperDocProxyAgent::CPGSuperDocProxyAgent()
{
   m_pMyDocument = nullptr;
   m_EventHoldCount = 0;
   m_bFiringEvents = false;
   m_StdToolBarID = -1;
   m_LibToolBarID = -1;
   m_HelpToolBarID = -1;

   m_BridgeModelEditorViewKey = -1;
   m_GirderModelEditorViewKey = -1;
   m_LibraryEditorViewKey = -1;
   m_ReportViewKey = -1;
   m_GraphingViewKey = -1;
   m_LoadsViewKey = -1;
}

CPGSuperDocProxyAgent::~CPGSuperDocProxyAgent()
{
}

void CPGSuperDocProxyAgent::SetDocument(CPGSDocBase* pDoc)
{
   m_pMyDocument = pDoc;
}

void CPGSuperDocProxyAgent::CreateStatusBar()
{
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   CPGSuperStatusBar* pStatusBar = new CPGSuperStatusBar();
   pStatusBar->Create(pFrame);
   pFrame->SetStatusBar(pStatusBar);

   m_pMyDocument->UpdateAnalysisTypeStatusIndicator();
   m_pMyDocument->UpdateProjectCriteriaIndicator();
   m_pMyDocument->SetModifiedFlag(m_pMyDocument->IsModified());
   m_pMyDocument->EnableAutoCalc(m_pMyDocument->IsAutoCalcEnabled());
}

void CPGSuperDocProxyAgent::ResetStatusBar()
{
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   pFrame->SetStatusBar(nullptr);
}

void CPGSuperDocProxyAgent::CreateAcceleratorKeys()
{
   m_pMyDocument->CreateAcceleratorKeys();
}

void CPGSuperDocProxyAgent::RemoveAcceleratorKeys()
{
   m_pMyDocument->RemoveAcceleratorKeys();
}

void CPGSuperDocProxyAgent::CreateToolBars()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IEAFToolbars,pToolBars);

   m_StdToolBarID = pToolBars->CreateToolBar(_T("Standard"));
   CEAFToolBar* pToolBar = pToolBars->GetToolBar(m_StdToolBarID);
   pToolBar->LoadToolBar(m_pMyDocument->GetStandardToolbarResourceID(),nullptr); // don't use a command callback because these commands are handled by 
                                               // the standard MFC message routing

   // Add a drop-down arrow to the Open and Report buttons
   pToolBar->CreateDropDownButton(ID_FILE_OPEN,   nullptr,BTNS_DROPDOWN);
   pToolBar->CreateDropDownButton(ID_VIEW_GRAPHS, nullptr,BTNS_WHOLEDROPDOWN);
   pToolBar->CreateDropDownButton(ID_VIEW_REPORTS,nullptr,BTNS_WHOLEDROPDOWN);

   pToolBar->CreateDropDownButton(ID_COPY_GIRDER_PROPS,nullptr,BTNS_WHOLEDROPDOWN);
   pToolBar->CreateDropDownButton(ID_COPY_PIER_PROPS,nullptr,BTNS_WHOLEDROPDOWN);
   pToolBar->CreateDropDownButton(ID_COPY_TEMPSUPPORT_PROPS,nullptr,BTNS_WHOLEDROPDOWN);

   m_LibToolBarID = pToolBars->CreateToolBar(_T("Library"));
   pToolBar = pToolBars->GetToolBar(m_LibToolBarID);
   pToolBar->LoadToolBar(IDR_LIBTOOLBAR,nullptr);

   m_HelpToolBarID = pToolBars->CreateToolBar(_T("Help"));
   pToolBar = pToolBars->GetToolBar(m_HelpToolBarID);
   pToolBar->LoadToolBar(IDR_HELPTOOLBAR,nullptr);

   OnStatusChanged(); // set the status items
}

void CPGSuperDocProxyAgent::RemoveToolBars()
{
   GET_IFACE(IEAFToolbars,pToolBars);
   pToolBars->DestroyToolBar(m_StdToolBarID);
   pToolBars->DestroyToolBar(m_LibToolBarID);
   pToolBars->DestroyToolBar(m_HelpToolBarID);
}

void CPGSuperDocProxyAgent::RegisterViews()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)(m_pMyDocument->GetDocTemplate());
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);

   HMENU hMenu = pAppPlugin->GetSharedMenuHandle();

   // Register all secondary views that are associated with our document type
   // TODO: After the menu and command extensions can be made, the agents that are responsble
   // for the views below will register them. For example, the analysis results view is the
   // responsiblity of the analysis results agent, so that view's implementation will move
   GET_IFACE(IEAFViewRegistrar,pViewReg);
   m_BridgeModelEditorViewKey = pViewReg->RegisterView(IDR_BRIDGEMODELEDITOR, nullptr, RUNTIME_CLASS(CBridgeModelViewChildFrame), RUNTIME_CLASS(CBridgePlanView),           hMenu, 1);
   m_GirderModelEditorViewKey = pViewReg->RegisterView(IDR_GIRDERMODELEDITOR, nullptr, RUNTIME_CLASS(CGirderModelChildFrame),     RUNTIME_CLASS(CGirderModelElevationView), hMenu, -1); // unlimited number of reports
   m_LibraryEditorViewKey     = pViewReg->RegisterView(IDR_LIBRARYEDITOR,     nullptr, RUNTIME_CLASS(CLibChildFrame),             RUNTIME_CLASS(CLibraryEditorView),        hMenu, 1);
   m_ReportViewKey            = pViewReg->RegisterView(IDR_REPORT,            nullptr, RUNTIME_CLASS(CReportViewChildFrame),      RUNTIME_CLASS(CPGSuperReportView),        hMenu, -1); // unlimited number of reports
   m_GraphingViewKey          = pViewReg->RegisterView(IDR_GRAPHING,          nullptr, RUNTIME_CLASS(CGraphViewChildFrame),        RUNTIME_CLASS(CGraphView),            hMenu, -1); // unlimited number of reports
   m_LoadsViewKey             = pViewReg->RegisterView(IDR_EDITLOADS,         nullptr, RUNTIME_CLASS(CEditLoadsChildFrame),       RUNTIME_CLASS(CEditLoadsView),            hMenu, 1);
}

void CPGSuperDocProxyAgent::UnregisterViews()
{
   GET_IFACE(IEAFViewRegistrar,pViewReg);
   pViewReg->RemoveView(m_BridgeModelEditorViewKey);
   pViewReg->RemoveView(m_GirderModelEditorViewKey);
   pViewReg->RemoveView(m_LibraryEditorViewKey);
   pViewReg->RemoveView(m_ReportViewKey);
   pViewReg->RemoveView(m_GraphingViewKey);
   pViewReg->RemoveView(m_LoadsViewKey);
}

void CPGSuperDocProxyAgent::AdviseEventSinks()
{
   //
   // Attach to connection points
   //
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   hr = pBrokerInit->FindConnectionPoint( IID_IBridgeDescriptionEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwBridgeDescCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_IEnvironmentEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwEnvironmentCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_IProjectPropertiesEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwProjectPropertiesCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_IEAFDisplayUnitsEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwDisplayUnitsCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_ISpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwSpecificationCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_IRatingSpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwRatingSpecificationCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_ILibraryConflictEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwLibraryConflictGuiCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_ILoadModifiersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwLoadModiferCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_ILossParametersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwLossParametersCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.
}

void CPGSuperDocProxyAgent::UnadviseEventSinks()
{
   //
   // Detach to connection points
   //
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   hr = pBrokerInit->FindConnectionPoint( IID_IBridgeDescriptionEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwBridgeDescCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_IEnvironmentEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwEnvironmentCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_IProjectPropertiesEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwProjectPropertiesCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_IEAFDisplayUnitsEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwDisplayUnitsCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_ISpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwSpecificationCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_IRatingSpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwRatingSpecificationCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_ILibraryConflictEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwLibraryConflictGuiCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_ILoadModifiersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwLoadModiferCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_ILossParametersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwLossParametersCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.
}

void CPGSuperDocProxyAgent::CreateBridgeModelView(IBridgeModelViewController** ppViewController)
{
   GET_IFACE(IEAFViewRegistrar,pViewReg);
   CView* pView = pViewReg->CreateView(m_BridgeModelEditorViewKey);

   if (ppViewController && pView->IsKindOf(RUNTIME_CLASS(CBridgeViewPane)) )
   {
      CBridgeViewPane* pPane = (CBridgeViewPane*)pView;
      CBridgeModelViewChildFrame* pFrame = pPane->GetFrame();
      CEAFViewControllerFactory* pFactory = dynamic_cast<CEAFViewControllerFactory*>(pFrame);
      if (pFactory)
      {
         CComPtr<IEAFViewController> controller;
         pFactory->GetViewController(&controller);
         controller.QueryInterface(ppViewController);
      }
      else
      {
         *ppViewController = nullptr;
      }
   }
}

void CPGSuperDocProxyAgent::CreateGirderView(const CGirderKey& girderKey, IGirderModelViewController** ppViewController)
{
   GET_IFACE(IEAFViewRegistrar,pViewReg);
   CView* pView = pViewReg->CreateView(m_GirderModelEditorViewKey,(void*)&girderKey);

   if (ppViewController && pView->IsKindOf(RUNTIME_CLASS(CGirderModelElevationView)))
   {
      CGirderModelElevationView* pGirderElevationView = (CGirderModelElevationView*)pView;
      CGirderModelChildFrame* pFrame = pGirderElevationView->GetFrame();
      CEAFViewControllerFactory* pFactory = dynamic_cast<CEAFViewControllerFactory*>(pFrame);
      if (pFactory)
      {
         CComPtr<IEAFViewController> controller;
         pFactory->GetViewController(&controller);
         controller.QueryInterface(ppViewController);
      }
      else
      {
         *ppViewController = nullptr;
      }
   }
}

void CPGSuperDocProxyAgent::CreateLoadsView(ILoadsViewController** ppViewController)
{
   GET_IFACE(IEAFViewRegistrar, pViewReg);
   CView* pView = pViewReg->CreateView(m_LoadsViewKey);

   if (ppViewController && pView->IsKindOf(RUNTIME_CLASS(CEditLoadsView)))
   {
      CEditLoadsView* pLoadsView = (CEditLoadsView*)pView;
      CEditLoadsChildFrame* pFrame = (CEditLoadsChildFrame*)pLoadsView->GetParentFrame();
      CEAFViewControllerFactory* pFactory = dynamic_cast<CEAFViewControllerFactory*>(pFrame);
      if (pFactory)
      {
         CComPtr<IEAFViewController> controller;
         pFactory->GetViewController(&controller);
         controller.QueryInterface(ppViewController);
      }
      else
      {
         *ppViewController = nullptr;
      }
   }
}

void CPGSuperDocProxyAgent::CreateLibraryEditorView()
{
   GET_IFACE(IEAFViewRegistrar,pViewReg);
   pViewReg->CreateView(m_LibraryEditorViewKey);
}

void CPGSuperDocProxyAgent::CreateReportView(CollectionIndexType rptIdx,BOOL bPromptForSpec)
{
   CEAFReportViewCreationData data;
   data.m_RptIdx = rptIdx;
   data.m_bPromptForSpec = bPromptForSpec;

   GET_IFACE(IReportManager,pRptMgr);
   data.m_pRptMgr = pRptMgr;

   GET_IFACE(IEAFViewRegistrar,pViewReg);
   pViewReg->CreateView(m_ReportViewKey,(LPVOID)&data);
}

void CPGSuperDocProxyAgent::BuildReportMenu(CEAFMenu* pMenu,bool bQuickReport)
{
   m_pMyDocument->BuildReportMenu(pMenu,bQuickReport);
}

void CPGSuperDocProxyAgent::CreateGraphView(CollectionIndexType graphIdx, IEAFViewController** ppViewController)
{
   CEAFGraphViewCreationData data;
   GET_IFACE(IGraphManager,pGraphMgr);
   data.m_pIGraphMgr = pGraphMgr;
   data.m_GraphIndex = graphIdx;

   GET_IFACE(IEAFViewRegistrar,pViewReg);
   CView* pView = pViewReg->CreateView(m_GraphingViewKey,(LPVOID)&data);

   if (ppViewController && pView->IsKindOf(RUNTIME_CLASS(CGraphView)))
   {
      CGraphView* pGraphView = (CGraphView*)pView;
      CEAFGraphChildFrame* pFrame = (CEAFGraphChildFrame*)pGraphView->GetFrame();
      
      CEAFViewControllerFactory* pFactory = dynamic_cast<CEAFViewControllerFactory*>(pFrame);
      ATLASSERT(pFactory != nullptr);
      if (pFactory)
      {
         pFactory->GetViewController(ppViewController);
      }
      else
      {
         *ppViewController = nullptr;
      }
   }
}

void CPGSuperDocProxyAgent::CreateGraphView(LPCTSTR lpszGraph, IEAFViewController** ppViewController)
{
   GET_IFACE(IGraphManager, pGraphMgr);
   IndexType nGraphs = pGraphMgr->GetGraphBuilderCount();
   for (IndexType graphIdx = 0; graphIdx < nGraphs; graphIdx++)
   {
      auto& pGraphBuilder = pGraphMgr->GetGraphBuilder(graphIdx);
      if (CString(lpszGraph).Compare(pGraphBuilder->GetName().c_str()) == 0)
      {
         CreateGraphView(graphIdx, ppViewController);
         return;
      }
   }

   ATLASSERT(false); // graph name not found
}

void CPGSuperDocProxyAgent::BuildGraphMenu(CEAFMenu* pMenu)
{
   m_pMyDocument->BuildGraphMenu(pMenu);
}

long CPGSuperDocProxyAgent::GetBridgeModelEditorViewKey()
{
   return m_BridgeModelEditorViewKey;
}

long CPGSuperDocProxyAgent::GetGirderModelEditorViewKey()
{
   return m_GirderModelEditorViewKey;
}

long CPGSuperDocProxyAgent::GetLibraryEditorViewKey()
{
   return m_LibraryEditorViewKey;
}

long CPGSuperDocProxyAgent::GetReportViewKey()
{
   return m_ReportViewKey;
}

long CPGSuperDocProxyAgent::GetGraphingViewKey()
{
   return m_GraphingViewKey;
}

long CPGSuperDocProxyAgent::GetLoadsViewKey()
{
   return m_LoadsViewKey;
}


///////////////////////////////////////////////////
void CPGSuperDocProxyAgent::OnStatusChanged()
{
   if ( m_pBroker )
   {
      GET_IFACE(IEAFToolbars,pToolBars);
      CEAFToolBar* pToolBar = pToolBars->GetToolBar(GetStdToolBarID());

      if ( pToolBar == nullptr )
         return;
   
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      switch(pStatusCenter->GetSeverity())
      {
      case eafTypes::statusInformation:
         pToolBar->HideButton(EAFID_VIEW_STATUSCENTER, nullptr, FALSE);
         pToolBar->HideButton(EAFID_VIEW_STATUSCENTER2,nullptr, TRUE);
         pToolBar->HideButton(EAFID_VIEW_STATUSCENTER3,nullptr, TRUE);
         break;

      case eafTypes::statusWarning:
         pToolBar->HideButton(EAFID_VIEW_STATUSCENTER, nullptr, TRUE);
         pToolBar->HideButton(EAFID_VIEW_STATUSCENTER2,nullptr, FALSE);
         pToolBar->HideButton(EAFID_VIEW_STATUSCENTER3,nullptr, TRUE);
         break;

      case eafTypes::statusError:
         pToolBar->HideButton(EAFID_VIEW_STATUSCENTER, nullptr, TRUE);
         pToolBar->HideButton(EAFID_VIEW_STATUSCENTER2,nullptr, TRUE);
         pToolBar->HideButton(EAFID_VIEW_STATUSCENTER3,nullptr, FALSE);
         break;
      }
   }
}

void CPGSuperDocProxyAgent::OnUIHintsReset()
{
   Fire_OnUIHintsReset();
}

//////////////////////////////////////////////////////////
// IAgentEx
STDMETHODIMP CPGSuperDocProxyAgent::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CPGSuperDocProxyAgent::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2> pBrokerInit(m_pBroker);
   pBrokerInit->RegInterface( IID_IEditByUI,           this );
   pBrokerInit->RegInterface( IID_IDesign,             this );
   pBrokerInit->RegInterface( IID_IViews,              this );
   pBrokerInit->RegInterface( IID_ISelection,          this );
   pBrokerInit->RegInterface( IID_IUIEvents,           this );
   pBrokerInit->RegInterface( IID_IUpdateTemplates,    this );
   pBrokerInit->RegInterface( IID_IVersionInfo,        this );
   pBrokerInit->RegInterface( IID_IRegisterViewEvents, this );
   pBrokerInit->RegInterface( IID_IExtendPGSuperUI,    this );
   pBrokerInit->RegInterface( IID_IExtendPGSpliceUI,   this );
   pBrokerInit->RegInterface( IID_IDocumentType,       this );
   pBrokerInit->RegInterface( IID_IDocumentUnitSystem, this );
   return S_OK;
}

STDMETHODIMP CPGSuperDocProxyAgent::Init()
{
   EAF_AGENT_INIT;

   return AGENT_S_SECONDPASSINIT;
}

STDMETHODIMP CPGSuperDocProxyAgent::Init2()
{
   AdviseEventSinks();
   return S_OK;
}

STDMETHODIMP CPGSuperDocProxyAgent::Reset()
{
   CEAFTxnManager::GetInstance()->Clear();
   return S_OK;
}

STDMETHODIMP CPGSuperDocProxyAgent::ShutDown()
{
   UnadviseEventSinks();

   EAF_AGENT_CLEAR_INTERFACE_CACHE;
   CLOSE_LOGFILE;

   return S_OK;
}

STDMETHODIMP CPGSuperDocProxyAgent::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_PGSuperDocProxyAgent;
   return S_OK;
}

////////////////////////////////////////////////////////////////////
// IAgentUIIntegration

STDMETHODIMP CPGSuperDocProxyAgent::IntegrateWithUI(BOOL bIntegrate)
{
   if ( bIntegrate )
   {
      RegisterViews();
      CreateToolBars();
      CreateAcceleratorKeys();
      CreateStatusBar();
   }
   else
   {
      ResetStatusBar();
      RemoveAcceleratorKeys();
      RemoveToolBars();
      UnregisterViews();
   }

   return S_OK;
}

////////////////////////////////////////////////////////////////////
// IBridgeDescriptionEventSink
HRESULT CPGSuperDocProxyAgent::OnBridgeChanged(CBridgeChangedHint* pHint)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   //
   // Check to see if the bridge has changed in such a way that the
   // selected element is no longer valid
   //
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();

   CSelection selection = m_pMyDocument->GetSelection();

   bool bClearSelection = false;

   if ( selection.Type == CSelection::Span )
   {
      const CSpanData2* pSpan = pBridgeDesc->GetSpan(selection.SpanIdx);
      if ( pSpan == nullptr )
      {
         // the selected span no longer exists
         bClearSelection = true;
      }
   }
   else if ( selection.Type == CSelection::Girder || selection.Type == CSelection::Segment )
   {
      if ( selection.GroupIdx == INVALID_INDEX )
      {
         bClearSelection = true;
      }
      else
      {
         const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(selection.GroupIdx);
         if ( pGroup == nullptr || pGroup->GetGirderCount() < selection.GirderIdx )
         {
            // the selected girder no longer exists
            bClearSelection = true;
         }
      }
   }
   else if ( selection.Type == CSelection::Pier )
   {
      PierIndexType pierIdx = selection.PierIdx;
      const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
      if ( pPier == nullptr )
         bClearSelection = true;
   }
   else if (selection.Type == CSelection::TemporarySupport)
   {
      const CTemporarySupportData* pTS = pBridgeDesc->FindTemporarySupport(selection.tsID);
      if (pTS == nullptr)
         bClearSelection = true;
   }

   if ( bClearSelection )
      m_pMyDocument->ClearSelection();

   m_pMyDocument->SetModifiedFlag();

   std::shared_ptr<CBridgeHint> pBridgeHint;
   if ( pHint )
   {
      pBridgeHint = std::make_shared<CBridgeHint>();
      pBridgeHint->PierIdx = pHint->PierIdx;
      pBridgeHint->PierFace = pHint->PierFace;
      pBridgeHint->bAdded = pHint->bAdded;
   }

   std::shared_ptr<CObject> pObjHint = std::dynamic_pointer_cast<CObject,CBridgeHint>(pBridgeHint);
   FireEvent( 0, HINT_BRIDGECHANGED, pObjHint );

   return S_OK;
}

HRESULT CPGSuperDocProxyAgent::OnGirderFamilyChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   GET_IFACE(IEAFDocument,pDoc);
   pDoc->SetModified();
   FireEvent( 0, HINT_GIRDERFAMILYCHANGED, nullptr );
   return S_OK;
}
   
HRESULT CPGSuperDocProxyAgent::OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SetModifiedFlag();

   std::shared_ptr<CGirderHint> pHint(std::make_shared<CGirderHint>());
   pHint->lHint   = lHint;
   pHint->girderKey = girderKey;

   FireEvent(nullptr,HINT_GIRDERCHANGED,pHint);

   return S_OK;
}

HRESULT CPGSuperDocProxyAgent::OnLiveLoadChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SetModifiedFlag();

   FireEvent( 0, HINT_LIVELOADCHANGED, nullptr );
   return S_OK;
}

HRESULT CPGSuperDocProxyAgent::OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SetModifiedFlag();
   
   FireEvent( 0, HINT_LIVELOADCHANGED, nullptr );
   return S_OK;
}

HRESULT CPGSuperDocProxyAgent::OnConstructionLoadChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SetModifiedFlag();

   FireEvent( 0, HINT_BRIDGECHANGED, nullptr );
   return S_OK;
}

// IEnvironmentEventSink
HRESULT CPGSuperDocProxyAgent::OnExposureConditionChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SetModifiedFlag();

   FireEvent( 0, HINT_ENVCHANGED, nullptr );
   return S_OK;
}

HRESULT CPGSuperDocProxyAgent::OnRelHumidityChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SetModifiedFlag();

   FireEvent( 0, HINT_ENVCHANGED, nullptr );
   return S_OK;
}

// IProjectPropertiesEventSink
HRESULT CPGSuperDocProxyAgent::OnProjectPropertiesChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SetModifiedFlag();

   FireEvent( 0, HINT_PROJECTPROPERTIESCHANGED, nullptr );
   return S_OK;
}

// IEAFDisplayUnitsEventSink
HRESULT CPGSuperDocProxyAgent::OnUnitsChanging()
{
   return S_OK;
}

HRESULT CPGSuperDocProxyAgent::OnUnitsChanged(eafTypes::UnitMode newUnitMode)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SetModifiedFlag();

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   CComPtr<IDocUnitSystem> pDocUnitSystem;
   m_pMyDocument->GetDocUnitSystem(&pDocUnitSystem);
   pDocUnitSystem->put_UnitMode(UnitModeType(pDisplayUnits->GetUnitMode()));

   FireEvent( 0, HINT_UNITSCHANGED, nullptr );
   return S_OK;
}

// ISpecificationEventSink
HRESULT CPGSuperDocProxyAgent::OnSpecificationChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SetModifiedFlag();
   m_pMyDocument->UpdateProjectCriteriaIndicator();
   m_pMyDocument->UpdateAnalysisTypeStatusIndicator();

   FireEvent( 0, HINT_SPECCHANGED, nullptr );
   return S_OK;
}

HRESULT CPGSuperDocProxyAgent::OnAnalysisTypeChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SetModifiedFlag();
   m_pMyDocument->UpdateAnalysisTypeStatusIndicator();

   FireEvent( 0, HINT_ANALYSISTYPECHANGED, nullptr );
   return S_OK;
}

// IRatingSpecificationEventSink
HRESULT CPGSuperDocProxyAgent::OnRatingSpecificationChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SetModifiedFlag();

   FireEvent( 0, HINT_RATINGSPECCHANGED, nullptr );
   return S_OK;
}

// ILoadModiferEventSink
HRESULT CPGSuperDocProxyAgent::OnLoadModifiersChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SetModifiedFlag();

   FireEvent( 0, HINT_LOADMODIFIERSCHANGED, nullptr );
   return S_OK;
}

// ILossParametersEventSink
HRESULT CPGSuperDocProxyAgent::OnLossParametersChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SetModifiedFlag();
   
   FireEvent( 0, HINT_LOSSPARAMETERSCHANGED, nullptr );
   return S_OK;
}

HRESULT CPGSuperDocProxyAgent::OnLibraryConflictResolved()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SetModifiedFlag();
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////
// ISelection
CSelection CPGSuperDocProxyAgent::GetSelection()
{
   return m_pMyDocument->GetSelection();
}

void CPGSuperDocProxyAgent::ClearSelection()
{
   m_pMyDocument->ClearSelection();
}

PierIndexType CPGSuperDocProxyAgent::GetSelectedPier()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CSelection selection = m_pMyDocument->GetSelection();
   return selection.PierIdx;
}

SpanIndexType CPGSuperDocProxyAgent::GetSelectedSpan()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CSelection selection = m_pMyDocument->GetSelection();
   return selection.SpanIdx;
}

CGirderKey CPGSuperDocProxyAgent::GetSelectedGirder()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CSelection selection = m_pMyDocument->GetSelection();
   return CGirderKey(selection.GroupIdx,selection.GirderIdx);
}

CSegmentKey CPGSuperDocProxyAgent::GetSelectedSegment()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CSelection selection = m_pMyDocument->GetSelection();
   return CSegmentKey(selection.GroupIdx,selection.GirderIdx,selection.SegmentIdx);
}

CClosureKey CPGSuperDocProxyAgent::GetSelectedClosureJoint()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CSelection selection = m_pMyDocument->GetSelection();
   return CClosureKey(selection.GroupIdx,selection.GirderIdx,selection.SegmentIdx);
}

SupportIDType CPGSuperDocProxyAgent::GetSelectedTemporarySupport()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CSelection selection = m_pMyDocument->GetSelection();
   return selection.tsID;
}

bool CPGSuperDocProxyAgent::IsDeckSelected()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CSelection selection = m_pMyDocument->GetSelection();
   return selection.Type == CSelection::Deck;
}

bool CPGSuperDocProxyAgent::IsAlignmentSelected()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CSelection selection = m_pMyDocument->GetSelection();
   return selection.Type == CSelection::Alignment;
}

bool CPGSuperDocProxyAgent::IsRailingSystemSelected(pgsTypes::TrafficBarrierOrientation orientation)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CSelection selection = m_pMyDocument->GetSelection();
   if (orientation == pgsTypes::tboLeft)
   {
      return selection.Type == CSelection::LeftRailingSystem;
   }
   else
   {
      return selection.Type == CSelection::RightRailingSystem;
   }
}

void CPGSuperDocProxyAgent::SelectPier(PierIndexType pierIdx)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SelectPier(pierIdx);
}

void CPGSuperDocProxyAgent::SelectSpan(SpanIndexType spanIdx)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SelectSpan(spanIdx);
}

void CPGSuperDocProxyAgent::SelectGirder(const CGirderKey& girderKey)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SelectGirder(girderKey);
}

void CPGSuperDocProxyAgent::SelectSegment(const CSegmentKey& segmentKey)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SelectSegment(segmentKey);
}

void CPGSuperDocProxyAgent::SelectClosureJoint(const CClosureKey& closureKey)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SelectClosureJoint(closureKey);
}

void CPGSuperDocProxyAgent::SelectTemporarySupport(SupportIDType tsID)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pMyDocument->SelectTemporarySupport(tsID);
}

void CPGSuperDocProxyAgent::SelectDeck()
{
   m_pMyDocument->SelectDeck();
}

void CPGSuperDocProxyAgent::SelectAlignment()
{
   m_pMyDocument->SelectAlignment();
}

void CPGSuperDocProxyAgent::SelectRailingSystem(pgsTypes::TrafficBarrierOrientation orientation)
{
   m_pMyDocument->SelectTrafficBarrier(orientation);
}

Float64 CPGSuperDocProxyAgent::GetSectionCutStation()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());

   POSITION pos = m_pMyDocument->GetFirstViewPosition();
   CView* pView;
   while ( pos != nullptr )
   {
      pView = m_pMyDocument->GetNextView(pos);
      if ( pView->IsKindOf( RUNTIME_CLASS(CBridgePlanView) ) )
      {
         CBridgePlanView* pPlanView = (CBridgePlanView*)pView;
         CFrameWnd* pParentFrame = pPlanView->GetParentFrame();
         ASSERT_KINDOF(CBridgeModelViewChildFrame,pParentFrame);
         CBridgeModelViewChildFrame* pFrame = (CBridgeModelViewChildFrame*)pParentFrame;
         return pFrame->GetCurrentCutLocation();
      }
   };

   ASSERT(false); // should never get here
   return -99999;
}


/////////////////////////////////////////////////////////////////////////
// IUpdateTemplates
bool CPGSuperDocProxyAgent::UpdatingTemplates()
{
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)(m_pMyDocument->GetDocTemplate());
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);

   CPGSuperAppPlugin* pPGSuperPlugin = dynamic_cast<CPGSuperAppPlugin*>(pAppPlugin.p);
   if ( pPGSuperPlugin )
      return pPGSuperPlugin->UpdatingTemplates();

   CPGSpliceAppPlugin* pPGSplicePlugin = dynamic_cast<CPGSpliceAppPlugin*>(pAppPlugin.p);
   if ( pPGSplicePlugin )
      return pPGSplicePlugin->UpdatingTemplates();

   return false;
}

/////////////////////////////////////////////////////////////////////////
// IUIEvents
void CPGSuperDocProxyAgent::HoldEvents(bool bHold)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   if ( bHold )
   {
      m_EventHoldCount++;
   }
   else
   {
      m_EventHoldCount--;
   }

   if ( m_EventHoldCount <= 0 && !m_bFiringEvents )
   {
      m_EventHoldCount = 0;
      m_UIEvents.clear();
   }
}

void CPGSuperDocProxyAgent::FirePendingEvents()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   if ( m_EventHoldCount == 0 )
   {
      return;
   }

   if ( 1 == m_EventHoldCount )
   {
      m_EventHoldCount--;

      m_bFiringEvents = true;

      try
      {
         for( const auto& event : m_UIEvents)
         {
            m_pMyDocument->UpdateAllViews(event.pSender, event.lHint, event.pHint.get());
         }

         m_bFiringEvents = false;
         m_UIEvents.clear();
      }
      catch (...)
      {
         m_bFiringEvents = false;
         m_UIEvents.clear();
         throw;
      }
   }
   else
   {
      m_EventHoldCount--;
   }
}

void CPGSuperDocProxyAgent::CancelPendingEvents()
{
   m_EventHoldCount--;
   if ( m_EventHoldCount <= 0 && !m_bFiringEvents )
   {
      m_EventHoldCount = 0;
      m_UIEvents.clear();
   }
}

void CPGSuperDocProxyAgent::FireEvent(CView* pSender,LPARAM lHint,std::shared_ptr<CObject> pHint)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   if ( 0 < m_EventHoldCount )
   {
      UIEvent event;
      event.pSender = pSender;
      event.lHint = lHint;
      event.pHint = pHint;

      // skip all but one result hint - firing multiple result hints 
      // causes the UI to unnecessarilly update multiple times
      bool skip = false;
      if ( MIN_RESULTS_HINT <= lHint && lHint <= MAX_RESULTS_HINT )
      {
         // the current hint is in the range that indicates analysis results are going to be updated
         std::vector<UIEvent>::iterator iter(m_UIEvents.begin());
         std::vector<UIEvent>::iterator iterEnd(m_UIEvents.end());
         for ( ; iter != iterEnd; iter++ )
         {
            UIEvent e = *iter;
            if ( MIN_RESULTS_HINT <= e.lHint && e.lHint <= MAX_RESULTS_HINT )
            {
               // we already have a hint in that range
               skip = true;
               break;
            }
         }
      }

      if (!skip)
      {
         ATLASSERT( !m_bFiringEvents ); // don't add to the container while we are iterating through it
         m_UIEvents.push_back(event);
      }
   }
   else
   {
      m_pMyDocument->UpdateAllViews(pSender,lHint,pHint.get());
   }
}

///////////////////////////////////////////////////////////////////////////////////
// IEditByUI
void CPGSuperDocProxyAgent::EditBridgeDescription(int nPage)
{
   m_pMyDocument->EditBridgeDescription(nPage);
}

void CPGSuperDocProxyAgent::EditAlignmentDescription(int nPage)
{
   m_pMyDocument->EditAlignmentDescription(nPage);
}

bool CPGSuperDocProxyAgent::EditSegmentDescription(const CSegmentKey& segmentKey, int nPage)
{
   return m_pMyDocument->EditGirderSegmentDescription(segmentKey,nPage);
}

bool CPGSuperDocProxyAgent::EditSegmentDescription()
{
   return m_pMyDocument->EditGirderSegmentDescription();
}

bool CPGSuperDocProxyAgent::EditClosureJointDescription(const CClosureKey& closureKey, int nPage)
{
   return m_pMyDocument->EditClosureJointDescription(closureKey,nPage);
}

bool CPGSuperDocProxyAgent::EditGirderDescription(const CGirderKey& girderKey, int nPage)
{
   return m_pMyDocument->EditGirderDescription(girderKey,nPage);
}

bool CPGSuperDocProxyAgent::EditGirderDescription()
{
   return m_pMyDocument->EditGirderDescription();
}

bool CPGSuperDocProxyAgent::EditSpanDescription(SpanIndexType spanIdx, int nPage)
{
   return m_pMyDocument->EditSpanDescription(spanIdx,nPage);
}

bool CPGSuperDocProxyAgent::EditPierDescription(PierIndexType pierIdx, int nPage)
{
   return m_pMyDocument->EditPierDescription(pierIdx,nPage);
}

bool CPGSuperDocProxyAgent::EditTemporarySupportDescription(PierIndexType pierIdx, int nPage)
{
   if (IsPGSpliceDocument())
   {
      CPGSpliceDoc* pPGSplice = dynamic_cast<CPGSpliceDoc*>(m_pMyDocument);
      ATLASSERT(pPGSplice != nullptr);

      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      const CTemporarySupportData* pTsData = pBridgeDesc->GetTemporarySupport(pierIdx);

      PierIndexType tsid = pTsData->GetID();

      return pPGSplice->EditTemporarySupportDescription(tsid, nPage);
   }
   else
   {
      return false;
   }
}

void CPGSuperDocProxyAgent::EditLiveLoads()
{
   return m_pMyDocument->OnLiveLoads();
}

void CPGSuperDocProxyAgent::EditLiveLoadDistributionFactors(pgsTypes::DistributionFactorMethod method,LldfRangeOfApplicabilityAction roaAction)
{
   return m_pMyDocument->OnLoadsLldf(method,roaAction);
}

bool CPGSuperDocProxyAgent::EditPointLoad(CollectionIndexType loadIdx)
{
   return m_pMyDocument->EditPointLoad(loadIdx);
}

bool CPGSuperDocProxyAgent::EditPointLoadByID(LoadIDType loadID)
{
   return m_pMyDocument->EditPointLoadByID(loadID);
}

bool CPGSuperDocProxyAgent::EditDistributedLoad(CollectionIndexType loadIdx)
{
   return m_pMyDocument->EditDistributedLoad(loadIdx);
}

bool CPGSuperDocProxyAgent::EditDistributedLoadByID(LoadIDType loadID)
{
   return m_pMyDocument->EditDistributedLoadByID(loadID);
}

bool CPGSuperDocProxyAgent::EditMomentLoad(CollectionIndexType loadIdx)
{
   return m_pMyDocument->EditMomentLoad(loadIdx);
}

bool CPGSuperDocProxyAgent::EditMomentLoadByID(LoadIDType loadID)
{
   return m_pMyDocument->EditMomentLoadByID(loadID);
}

bool CPGSuperDocProxyAgent::EditTimeline()
{
   return m_pMyDocument->EditTimeline();
}

bool CPGSuperDocProxyAgent::EditCastDeckActivity()
{
   return m_pMyDocument->EditCastDeckActivity();
}

UINT CPGSuperDocProxyAgent::GetStdToolBarID()
{
   return m_StdToolBarID;
}

UINT CPGSuperDocProxyAgent::GetLibToolBarID()
{
   return m_LibToolBarID;
}

UINT CPGSuperDocProxyAgent::GetHelpToolBarID()
{
   return m_HelpToolBarID;
}

bool CPGSuperDocProxyAgent::EditDirectSelectionPrestressing(const CSegmentKey& segmentKey)
{
   return m_pMyDocument->EditDirectSelectionPrestressing(segmentKey);
}

bool CPGSuperDocProxyAgent::EditDirectRowInputPrestressing(const CSegmentKey& segmentKey)
{
   return m_pMyDocument->EditDirectRowInputPrestressing(segmentKey);
}

bool CPGSuperDocProxyAgent::EditDirectStrandInputPrestressing(const CSegmentKey& segmentKey)
{
   return m_pMyDocument->EditDirectStrandInputPrestressing(segmentKey);
}

void CPGSuperDocProxyAgent::AddPointLoad(const CPointLoadData& loadData)
{
   m_pMyDocument->AddPointLoad(loadData);
}

void CPGSuperDocProxyAgent::DeletePointLoad(CollectionIndexType loadIdx)
{
   m_pMyDocument->DeletePointLoad(loadIdx);
}

void CPGSuperDocProxyAgent::AddDistributedLoad(const CDistributedLoadData& loadData)
{
   m_pMyDocument->AddDistributedLoad(loadData);
}

void CPGSuperDocProxyAgent::DeleteDistributedLoad(CollectionIndexType loadIdx)
{
   m_pMyDocument->DeleteDistributedLoad(loadIdx);
}

void CPGSuperDocProxyAgent::AddMomentLoad(const CMomentLoadData& loadData)
{
   m_pMyDocument->AddMomentLoad(loadData);
}

void CPGSuperDocProxyAgent::DeleteMomentLoad(CollectionIndexType loadIdx)
{
   m_pMyDocument->DeleteMomentLoad(loadIdx);
}

bool CPGSuperDocProxyAgent::EditEffectiveFlangeWidth()
{
   return m_pMyDocument->EditEffectiveFlangeWidth();
}

bool CPGSuperDocProxyAgent::SelectProjectCriteria()
{
   return m_pMyDocument->SelectProjectCriteria();
}

bool CPGSuperDocProxyAgent::EditBearings()
{
   return m_pMyDocument->DoEditBearing();
}

bool CPGSuperDocProxyAgent::EditHaunch()
{
   return m_pMyDocument->DoEditHaunch();
}

///////////////////////////////////////////////////////////////////////////////////
// IDesign
void CPGSuperDocProxyAgent::DesignGirder(bool bPrompt,bool bDesignSlabOffset,const CGirderKey& girderKey)
{
   arSlabOffsetDesignType designSlabOffset = bDesignSlabOffset ? sodDesignHaunch : sodPreserveHaunch;

   ((CPGSuperDoc*)m_pMyDocument)->DesignGirder(bPrompt,designSlabOffset,girderKey);
}

///////////////////////////////////////////////////////////////////////////////////
// IVersionInfo
CString CPGSuperDocProxyAgent::GetVersionString(bool bIncludeBuildNumber)
{
   CString str(_T("Version "));
   str += GetVersion(bIncludeBuildNumber);
#if defined _BETA_VERSION
   str += CString(_T(" BETA"));
#endif

   str += CString(_T(" - Built on "));
   str += CString(__DATE__);
   return str;
}

CString CPGSuperDocProxyAgent::GetVersion(bool bIncludeBuildNumber)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CWinApp* pApp = AfxGetApp();
   CString strExe( pApp->m_pszExeName );
   strExe += _T(".dll");

   CVersionInfo verInfo;
   verInfo.Load(strExe);
   

#if defined _DEBUG || defined _BETA_VERSION
   // always include the build number in debug and beta versions
   bIncludeBuildNumber = true;
#endif
   CString strVersion = verInfo.GetProductVersionAsString(bIncludeBuildNumber);

   return strVersion;
}

// IRegisterViewEvents
IDType CPGSuperDocProxyAgent::RegisterBridgePlanViewCallback(IBridgePlanViewEventCallback* pCallback)
{
   return m_pMyDocument->RegisterBridgePlanViewCallback(pCallback);
}

IDType CPGSuperDocProxyAgent::RegisterBridgeSectionViewCallback(IBridgeSectionViewEventCallback* pCallback)
{
   return m_pMyDocument->RegisterBridgeSectionViewCallback(pCallback);
}

IDType CPGSuperDocProxyAgent::RegisterAlignmentPlanViewCallback(IAlignmentPlanViewEventCallback* pCallback)
{
   return m_pMyDocument->RegisterAlignmentPlanViewCallback(pCallback);
}

IDType CPGSuperDocProxyAgent::RegisterAlignmentProfileViewCallback(IAlignmentProfileViewEventCallback* pCallback)
{
   return m_pMyDocument->RegisterAlignmentProfileViewCallback(pCallback);
}

IDType CPGSuperDocProxyAgent::RegisterGirderElevationViewCallback(IGirderElevationViewEventCallback* pCallback)
{
   return m_pMyDocument->RegisterGirderElevationViewCallback(pCallback);
}

IDType CPGSuperDocProxyAgent::RegisterGirderSectionViewCallback(IGirderSectionViewEventCallback* pCallback)
{
   return m_pMyDocument->RegisterGirderSectionViewCallback(pCallback);
}

bool CPGSuperDocProxyAgent::UnregisterBridgePlanViewCallback(IDType ID)
{
   return m_pMyDocument->UnregisterBridgePlanViewCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterBridgeSectionViewCallback(IDType ID)
{
   return m_pMyDocument->UnregisterBridgeSectionViewCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterAlignmentPlanViewCallback(IDType ID)
{
   return m_pMyDocument->UnregisterAlignmentPlanViewCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterAlignmentProfileViewCallback(IDType ID)
{
   return m_pMyDocument->UnregisterAlignmentProfileViewCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterGirderElevationViewCallback(IDType ID)
{
   return m_pMyDocument->UnregisterGirderElevationViewCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterGirderSectionViewCallback(IDType ID)
{
   return m_pMyDocument->UnregisterGirderSectionViewCallback(ID);
}

IDType CPGSuperDocProxyAgent::RegisterEditPierCallback(IEditPierCallback* pCallback,ICopyPierPropertiesCallback* pCopyCallback)
{
   return m_pMyDocument->RegisterEditPierCallback(pCallback, pCopyCallback);
}

IDType CPGSuperDocProxyAgent::RegisterEditTemporarySupportCallback(IEditTemporarySupportCallback* pCallback, ICopyTemporarySupportPropertiesCallback* pCopyCallBack)
{
   return m_pMyDocument->RegisterEditTemporarySupportCallback(pCallback, pCopyCallBack);
}

IDType CPGSuperDocProxyAgent::RegisterEditSpanCallback(IEditSpanCallback* pCallback)
{
   return m_pMyDocument->RegisterEditSpanCallback(pCallback);
}

IDType CPGSuperDocProxyAgent::RegisterEditGirderCallback(IEditGirderCallback* pCallback,ICopyGirderPropertiesCallback* pCopyCallback)
{
   return m_pMyDocument->RegisterEditGirderCallback(pCallback,pCopyCallback);
}

IDType CPGSuperDocProxyAgent::RegisterEditSplicedGirderCallback(IEditSplicedGirderCallback* pCallback,ICopyGirderPropertiesCallback* pCopyCallback)
{
   return m_pMyDocument->RegisterEditSplicedGirderCallback(pCallback,pCopyCallback);
}

IDType CPGSuperDocProxyAgent::RegisterEditSegmentCallback(IEditSegmentCallback* pCallback)
{
   return m_pMyDocument->RegisterEditSegmentCallback(pCallback);
}

IDType CPGSuperDocProxyAgent::RegisterEditClosureJointCallback(IEditClosureJointCallback* pCallback)
{
   return m_pMyDocument->RegisterEditClosureJointCallback(pCallback);
}

IDType CPGSuperDocProxyAgent::RegisterEditBridgeCallback(IEditBridgeCallback* pCallback)
{
   return m_pMyDocument->RegisterEditBridgeCallback(pCallback);
}

IDType CPGSuperDocProxyAgent::RegisterEditLoadRatingOptionsCallback(IEditLoadRatingOptionsCallback* pCallback)
{
   return m_pMyDocument->RegisterEditLoadRatingOptionsCallback(pCallback);
}

bool CPGSuperDocProxyAgent::UnregisterEditPierCallback(IDType ID)
{
   return m_pMyDocument->UnregisterEditPierCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterEditTemporarySupportCallback(IDType ID)
{
   return m_pMyDocument->UnregisterEditTemporarySupportCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterEditSpanCallback(IDType ID)
{
   return m_pMyDocument->UnregisterEditSpanCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterEditGirderCallback(IDType ID)
{
   return m_pMyDocument->UnregisterEditGirderCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterEditSplicedGirderCallback(IDType ID)
{
   return m_pMyDocument->UnregisterEditSplicedGirderCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterEditSegmentCallback(IDType ID)
{
   return m_pMyDocument->UnregisterEditSegmentCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterEditClosureJointCallback(IDType ID)
{
   return m_pMyDocument->UnregisterEditClosureJointCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterEditBridgeCallback(IDType ID)
{
   return m_pMyDocument->UnregisterEditBridgeCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterEditLoadRatingOptionsCallback(IDType ID)
{
   return m_pMyDocument->UnregisterEditLoadRatingOptionsCallback(ID);
}

bool CPGSuperDocProxyAgent::IsPGSuperDocument()
{
   return m_pMyDocument->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) ? true : false;
}

bool CPGSuperDocProxyAgent::IsPGSpliceDocument()
{
   return m_pMyDocument->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) ? true : false;
}

// IDocumentUnitSystem
void CPGSuperDocProxyAgent::GetUnitServer(IUnitServer** ppUnitServer)
{
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)(m_pMyDocument->GetDocTemplate());
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSuper = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);

   CComPtr<IAppUnitSystem> appUnitSystem;
   pPGSuper->GetAppUnitSystem(&appUnitSystem);

   appUnitSystem->get_UnitServer(ppUnitServer);
}


CBridgeModelViewChildFrame* CPGSuperDocProxyAgent::GetBridgeModelViewFrame()
{
   GET_IFACE(IEAFViewRegistrar, pViewReg);
   std::vector<CView*> vViews = pViewReg->GetRegisteredView(m_BridgeModelEditorViewKey);
   if (vViews.size() == 0)
   {
      vViews.push_back(pViewReg->CreateView(m_BridgeModelEditorViewKey));
   }

   CView* pView = vViews.front();
   CFrameWnd* pParentFrame = pView->GetParentFrame();
   ASSERT_KINDOF(CBridgeModelViewChildFrame, pParentFrame);
   CBridgeModelViewChildFrame* pFrame = (CBridgeModelViewChildFrame*)pParentFrame;
   return pFrame;
}
