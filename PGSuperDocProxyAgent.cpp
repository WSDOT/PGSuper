///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperDocProxyAgent.h"
#include "PGSuperDoc.h"
#include "PGSuperAppPlugin.h"

#include "PGSuperAppPlugin\Resource.h"

#include <IFace\Project.h>
#include <IFace\StatusCenter.h>
#include <IFace\Bridge.h>
#include <EAF\EAFUIIntegration.h>
#include <EAF\EAFStatusItem.h>

#include <PgsExt\BridgeDescription.h>
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperDoc.h"
#include "Hints.h"

#include <System\TxnManager.h>

#include <psgLib\psgLib.h>

#include "ChildFrm.h"
#include <PsgLib\LibChildFrm.h>

#include "BridgeModelViewChildFrame.h"
#include "BridgePlanView.h"

#include "FactorOfSafetyChildFrame.h"
#include "FactorOfSafetyView.h"

#include "EditLoadsChildFrm.h"
#include "EditLoadsView.h"

#include "AnalysisResultsChildFrame.h"
#include "AnalysisResultsView.h"

#include "GirderModelChildFrame.h"
#include "GirderModelElevationView.h"

#include "ReportViewChildFrame.h"
#include "ReportView.h"

#include <MfcTools\VersionInfo.h>

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
   m_pPGSuperDoc = NULL;
   m_bHoldingEvents = false;
   m_StdToolBarID = -1;
   m_LibToolBarID = -1;
   m_HelpToolBarID = -1;
}

CPGSuperDocProxyAgent::~CPGSuperDocProxyAgent()
{
}

void CPGSuperDocProxyAgent::SetDocument(CPGSuperDoc* pDoc)
{
   m_pPGSuperDoc = pDoc;
}

void CPGSuperDocProxyAgent::CreateStatusBar()
{
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   CPGSuperStatusBar* pStatusBar = new CPGSuperStatusBar();
   pStatusBar->Create(pFrame);
   pFrame->SetStatusBar(pStatusBar);

   m_pPGSuperDoc->UpdateAnalysisTypeStatusIndicator();
   m_pPGSuperDoc->SetModifiedFlag(m_pPGSuperDoc->IsModified());
   m_pPGSuperDoc->EnableAutoCalc(m_pPGSuperDoc->IsAutoCalcEnabled());
}

void CPGSuperDocProxyAgent::ResetStatusBar()
{
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   pFrame->SetStatusBar(NULL);
}

void CPGSuperDocProxyAgent::CreateAcceleratorKeys()
{
   GET_IFACE(IEAFAcceleratorTable,pAccelTable);
   pAccelTable->AddAccelKey(FVIRTKEY,           VK_F5, ID_PROJECT_UPDATENOW,NULL);
   pAccelTable->AddAccelKey(FCONTROL | FVIRTKEY,VK_U,  ID_PROJECT_UPDATENOW,NULL);
   //pAccelTable->AddAccelKey(FCONTROL | FALT | FVIRTKEY, VK_L, ID_DUMP_LBAM,NULL);
}

void CPGSuperDocProxyAgent::RemoveAcceleratorKeys()
{
   GET_IFACE(IEAFAcceleratorTable,pAccelTable);
   pAccelTable->RemoveAccelKey(FVIRTKEY,           VK_F5);
   pAccelTable->RemoveAccelKey(FCONTROL | FVIRTKEY,VK_U );
   //pAccelTable->RemoveAccelKey(FCONTROL | FALT | FVIRTKEY, VK_L);
}

void CPGSuperDocProxyAgent::CreateToolBars()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IEAFToolbars,pToolBars);

   m_StdToolBarID = pToolBars->CreateToolBar(_T("Standard"));
   CEAFToolBar* pToolBar = pToolBars->GetToolBar(m_StdToolBarID);
   pToolBar->LoadToolBar(IDR_STDTOOLBAR,NULL); // don't use a command callback because these commands are handled by 
                                               // the standard MFC message routing

   // Add a drop-down arrow to the Open and Report buttons
   pToolBar->CreateDropDownButton(ID_FILE_OPEN,   NULL,BTNS_DROPDOWN);
   pToolBar->CreateDropDownButton(ID_VIEW_REPORTS,NULL,BTNS_WHOLEDROPDOWN);

   m_LibToolBarID = pToolBars->CreateToolBar(_T("Library"));
   pToolBar = pToolBars->GetToolBar(m_LibToolBarID);
   pToolBar->LoadToolBar(IDR_LIBTOOLBAR,NULL);

   m_HelpToolBarID = pToolBars->CreateToolBar(_T("Help"));
   pToolBar = pToolBars->GetToolBar(m_HelpToolBarID);
   pToolBar->LoadToolBar(IDR_HELPTOOLBAR,NULL);

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

   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)(m_pPGSuperDoc->GetDocTemplate());
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);

   HMENU hMenu = pAppPlugin->GetSharedMenuHandle();

   // Register all secondary views that are associated with our document type
   // TODO: After the menu and command extensions can be made, the agents that are responsble
   // for the views below will register them. For example, tne analysis results view is the
   // responsiblity of the analysis results agent, so that view's implementation will move
   GET_IFACE(IEAFViewRegistrar,pViewReg);
   m_BridgeModelEditorViewKey = pViewReg->RegisterView(IDR_BRIDGEMODELEDITOR,NULL,RUNTIME_CLASS(CBridgeModelViewChildFrame), RUNTIME_CLASS(CBridgePlanView),           hMenu, 1);
   m_GirderModelEditorViewKey = pViewReg->RegisterView(IDR_GIRDERMODELEDITOR,NULL,RUNTIME_CLASS(CGirderModelChildFrame),     RUNTIME_CLASS(CGirderModelElevationView), hMenu, -1);
   m_LibraryEditorViewKey     = pViewReg->RegisterView(IDR_LIBRARYEDITOR,    NULL,RUNTIME_CLASS(CLibChildFrame),             RUNTIME_CLASS(CLibraryEditorView),        hMenu, 1);
   m_AnalysisResultsViewKey   = pViewReg->RegisterView(IDR_ANALYSISRESULTS,  NULL,RUNTIME_CLASS(CAnalysisResultsChildFrame), RUNTIME_CLASS(CAnalysisResultsView),      hMenu, -1);
   m_ReportViewKey            = pViewReg->RegisterView(IDR_REPORT,           NULL,RUNTIME_CLASS(CReportViewChildFrame),      RUNTIME_CLASS(CPGSuperReportView),        hMenu, -1); // unlimited number of reports
   m_FactorOfSafetyViewKey    = pViewReg->RegisterView(IDR_FACTOROFSAFETY,   NULL,RUNTIME_CLASS(CFactorOfSafetyChildFrame),  RUNTIME_CLASS(CFactorOfSafetyView),       hMenu, -1);
   m_LoadsViewKey             = pViewReg->RegisterView(IDR_EDITLOADS,        NULL,RUNTIME_CLASS(CEditLoadsChildFrame),       RUNTIME_CLASS(CEditLoadsView),            hMenu, 1);
}

void CPGSuperDocProxyAgent::UnregisterViews()
{
   GET_IFACE(IEAFViewRegistrar,pViewReg);
   pViewReg->RemoveView(m_BridgeModelEditorViewKey);
   pViewReg->RemoveView(m_GirderModelEditorViewKey);
   pViewReg->RemoveView(m_LibraryEditorViewKey);
   pViewReg->RemoveView(m_AnalysisResultsViewKey);
   pViewReg->RemoveView(m_ReportViewKey);
   pViewReg->RemoveView(m_FactorOfSafetyViewKey);
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
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwBridgeDescCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_IEnvironmentEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwEnvironmentCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_IProjectPropertiesEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwProjectPropertiesCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_IEAFDisplayUnitsEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwDisplayUnitsCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_ISpecificationEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwSpecificationCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_IRatingSpecificationEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwRatingSpecificationCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_ILibraryConflictEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwLibraryConflictGuiCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_ILoadModifiersEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwLoadModiferCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.
}

void CPGSuperDocProxyAgent::CreateBridgeModelView()
{
   GET_IFACE(IEAFViewRegistrar,pViewReg);
   pViewReg->CreateView(m_BridgeModelEditorViewKey);
}

void CPGSuperDocProxyAgent::CreateGirderView(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   if ( pStatusCenter->GetSeverity() == eafTypes::statusError )
   {
      AfxMessageBox(_T("One or more errors is preventing the creation of this view.\r\n\r\nSee the Status Center for details."),MB_OK);
   }
   else
   {
      GET_IFACE(IEAFViewRegistrar,pViewReg);
      SpanGirderHashType hash = HashSpanGirder(spanIdx,gdrIdx);
      CView* pView = pViewReg->CreateView(m_GirderModelEditorViewKey,(void*)&hash);
   }
}

void CPGSuperDocProxyAgent::CreateAnalysisResultsView(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   if ( pStatusCenter->GetSeverity() == eafTypes::statusError )
   {
      AfxMessageBox(_T("One or more errors is preventing the creation of this view.\r\n\r\nSee the Status Center for details."),MB_OK);
   }
   else
   {
      GET_IFACE(IEAFViewRegistrar,pViewReg);
      SpanGirderHashType hash = HashSpanGirder(spanIdx,gdrIdx);
      pViewReg->CreateView(m_AnalysisResultsViewKey,(void*)&hash);
   }
}

void CPGSuperDocProxyAgent::CreateStabilityView(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   if ( pStatusCenter->GetSeverity() == eafTypes::statusError )
   {
      AfxMessageBox(_T("One or more errors is preventing the creation of this view.\r\n\r\nSee the Status Center for details."),MB_OK);
   }
   else
   {
      GET_IFACE(IEAFViewRegistrar,pViewReg);
      SpanGirderHashType hash = HashSpanGirder(spanIdx,gdrIdx);
      pViewReg->CreateView(m_FactorOfSafetyViewKey,(void*)&hash);
   }
}

void CPGSuperDocProxyAgent::CreateLoadsView()
{
   GET_IFACE(IEAFViewRegistrar,pViewReg);
   pViewReg->CreateView(m_LoadsViewKey);
}

void CPGSuperDocProxyAgent::CreateLibraryEditorView()
{
   GET_IFACE(IEAFViewRegistrar,pViewReg);
   pViewReg->CreateView(m_LibraryEditorViewKey);
}

void CPGSuperDocProxyAgent::CreateReportView(CollectionIndexType rptIdx,bool bPromptForSpec)
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
   m_pPGSuperDoc->BuildReportMenu(pMenu,bQuickReport);
}

void CPGSuperDocProxyAgent::OnStatusChanged()
{
   if ( m_pBroker )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEAFToolbars,pToolBars);
      CEAFToolBar* pToolBar = pToolBars->GetToolBar(GetStdToolBarID());

      if ( pToolBar == NULL )
         return;
   
      switch(pStatusCenter->GetSeverity())
      {
      case eafTypes::statusOK:
         pToolBar->HideButton(ID_VIEW_STATUSCENTER, NULL, FALSE);
         pToolBar->HideButton(ID_VIEW_STATUSCENTER2,NULL, TRUE);
         pToolBar->HideButton(ID_VIEW_STATUSCENTER3,NULL, TRUE);
         break;

      case eafTypes::statusWarning:
         pToolBar->HideButton(ID_VIEW_STATUSCENTER, NULL, TRUE);
         pToolBar->HideButton(ID_VIEW_STATUSCENTER2,NULL, FALSE);
         pToolBar->HideButton(ID_VIEW_STATUSCENTER3,NULL, TRUE);
         break;

      case eafTypes::statusError:
         pToolBar->HideButton(ID_VIEW_STATUSCENTER, NULL, TRUE);
         pToolBar->HideButton(ID_VIEW_STATUSCENTER2,NULL, TRUE);
         pToolBar->HideButton(ID_VIEW_STATUSCENTER3,NULL, FALSE);
         break;
      }
   }
}

//////////////////////////////////////////////////////////
// IAgentEx
STDMETHODIMP CPGSuperDocProxyAgent::SetBroker(IBroker* pBroker)
{
   AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CPGSuperDocProxyAgent::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2> pBrokerInit(m_pBroker);
   pBrokerInit->RegInterface( IID_IEditByUI,           this );
   pBrokerInit->RegInterface( IID_IEditByUIEx,         this );
   pBrokerInit->RegInterface( IID_IDesign,             this );
   pBrokerInit->RegInterface( IID_IViews,              this );
   pBrokerInit->RegInterface( IID_ISelection,          this );
   pBrokerInit->RegInterface( IID_ISelectionEx,         this );
   pBrokerInit->RegInterface( IID_IUIEvents,           this );
   pBrokerInit->RegInterface( IID_IUpdateTemplates,    this );
   pBrokerInit->RegInterface( IID_IVersionInfo,        this );
   pBrokerInit->RegInterface( IID_IRegisterViewEvents, this );
   return S_OK;
}

STDMETHODIMP CPGSuperDocProxyAgent::Init()
{
   AGENT_INIT;

   AdviseEventSinks();

   return S_OK;
}

STDMETHODIMP CPGSuperDocProxyAgent::Init2()
{
   return S_OK;
}

STDMETHODIMP CPGSuperDocProxyAgent::Reset()
{
   txnTxnManager::GetInstance()->Clear();
   return S_OK;
}

STDMETHODIMP CPGSuperDocProxyAgent::ShutDown()
{
   UnadviseEventSinks();

   CLOSE_LOGFILE;

   AGENT_CLEAR_INTERFACE_CACHE;

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
HRESULT CPGSuperDocProxyAgent::OnBridgeChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   //
   // Check to see if the bridge has changed in such a way that the
   // selected girder is invalid
   //
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();

   CSelection selection = m_pPGSuperDoc->GetSelection();

   bool bClearSelection = false;

   if ( selection.Type == CSelection::Span || selection.Type == CSelection::Girder )
   {
      SpanIndexType spanIdx = selection.SpanIdx;
      const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);
      if ( pSpan == NULL )
      {
         bClearSelection = true;
      }
      else
      {
         GirderIndexType nGirders = pSpan->GetGirderCount();
         GirderIndexType gdrIdx   = selection.GirderIdx;

         if ( nGirders < gdrIdx )
            bClearSelection = true;
      }
   }
   else if ( selection.Type == CSelection::Pier )
   {
      PierIndexType pierIdx = selection.PierIdx;
      const CPierData* pPier = pBridgeDesc->GetPier(pierIdx);
      if ( pPier == NULL )
         bClearSelection = true;
   }

   if ( bClearSelection )
      m_pPGSuperDoc->ClearSelection();

   m_pPGSuperDoc->SetModifiedFlag();
   boost::shared_ptr<CObject> pnull;
   FireEvent( 0, HINT_BRIDGECHANGED, pnull );

   return S_OK;
}

HRESULT CPGSuperDocProxyAgent::OnGirderFamilyChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   GET_IFACE(IEAFDocument,pDoc);
   pDoc->SetModified();
   boost::shared_ptr<CObject> pnull;
   FireEvent( 0, HINT_GIRDERFAMILYCHANGED, pnull );
   return S_OK;
}
   
HRESULT CPGSuperDocProxyAgent::OnGirderChanged(SpanIndexType span,GirderIndexType gdr,Uint32 lHint)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SetModifiedFlag();

   boost::shared_ptr<CGirderHint> pHint(new CGirderHint());
   pHint->lHint = lHint;
   pHint->spanIdx = span;
   pHint->gdrIdx = gdr;

   FireEvent(NULL,HINT_GIRDERCHANGED,pHint);

   return S_OK;
}

HRESULT CPGSuperDocProxyAgent::OnLiveLoadChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SetModifiedFlag();
   boost::shared_ptr<CObject> pnull;
   FireEvent( 0, HINT_LIVELOADCHANGED, pnull );
   return S_OK;
}

HRESULT CPGSuperDocProxyAgent::OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SetModifiedFlag();
   boost::shared_ptr<CObject> pnull;
   FireEvent( 0, HINT_LIVELOADCHANGED, pnull );
   return S_OK;
}

HRESULT CPGSuperDocProxyAgent::OnConstructionLoadChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SetModifiedFlag();
   boost::shared_ptr<CObject> pnull;
   FireEvent( 0, HINT_BRIDGECHANGED, pnull );
   return S_OK;
}

// IEnvironmentEventSink
HRESULT CPGSuperDocProxyAgent::OnExposureConditionChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SetModifiedFlag();
   boost::shared_ptr<CObject> pnull;
   FireEvent( 0, HINT_ENVCHANGED, pnull );
   return S_OK;
}

HRESULT CPGSuperDocProxyAgent::OnRelHumidityChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SetModifiedFlag();
   boost::shared_ptr<CObject> pnull;
   FireEvent( 0, HINT_ENVCHANGED, pnull );
   return S_OK;
}

// IProjectPropertiesEventSink
HRESULT CPGSuperDocProxyAgent::OnProjectPropertiesChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SetModifiedFlag();

   boost::shared_ptr<CObject> pnull;
   FireEvent( 0, HINT_PROJECTPROPERTIESCHANGED, pnull );
   return S_OK;
}

// IEAFDisplayUnitsEventSink
HRESULT CPGSuperDocProxyAgent::OnUnitsChanged(eafTypes::UnitMode newUnitMode)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SetModifiedFlag();

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   CComPtr<IDocUnitSystem> pDocUnitSystem;
   m_pPGSuperDoc->GetDocUnitSystem(&pDocUnitSystem);
   pDocUnitSystem->put_UnitMode(UnitModeType(pDisplayUnits->GetUnitMode()));

   boost::shared_ptr<CObject> pnull;
   FireEvent( 0, HINT_UNITSCHANGED, pnull );
   return S_OK;
}

// ISpecificationEventSink
HRESULT CPGSuperDocProxyAgent::OnSpecificationChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SetModifiedFlag();
   boost::shared_ptr<CObject> pnull;
   FireEvent( 0, HINT_SPECCHANGED, pnull );
   return S_OK;
}

HRESULT CPGSuperDocProxyAgent::OnAnalysisTypeChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SetModifiedFlag();
   m_pPGSuperDoc->UpdateAnalysisTypeStatusIndicator();
   boost::shared_ptr<CObject> pnull;
   FireEvent( 0, HINT_ANALYSISTYPECHANGED, pnull );
   return S_OK;
}

// IRatingSpecificationEventSink
HRESULT CPGSuperDocProxyAgent::OnRatingSpecificationChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SetModifiedFlag();
   boost::shared_ptr<CObject> pnull;
   FireEvent( 0, HINT_RATINGSPECCHANGED, pnull );
   return S_OK;
}

// ILoadModiferEventSink
HRESULT CPGSuperDocProxyAgent::OnLoadModifiersChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SetModifiedFlag();
   boost::shared_ptr<CObject> pnull;
   FireEvent( 0, HINT_LOADMODIFIERSCHANGED, pnull );
   return S_OK;
}

HRESULT CPGSuperDocProxyAgent::OnLibraryConflictResolved()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SetModifiedFlag();
   return S_OK;
}

////////////////////////////////////////////////////////////////////
// ISelection
PierIndexType CPGSuperDocProxyAgent::GetPierIdx()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CSelection selection = m_pPGSuperDoc->GetSelection();
   return selection.PierIdx;
}

SpanIndexType CPGSuperDocProxyAgent::GetSpanIdx()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CSelection selection = m_pPGSuperDoc->GetSelection();
   return selection.SpanIdx;
}

GirderIndexType CPGSuperDocProxyAgent::GetGirderIdx()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CSelection selection = m_pPGSuperDoc->GetSelection();
   return selection.GirderIdx;
}

void CPGSuperDocProxyAgent::SelectPier(PierIndexType pierIdx)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SelectPier(pierIdx);
}

void CPGSuperDocProxyAgent::SelectSpan(SpanIndexType spanIdx)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SelectSpan(spanIdx);
}

void CPGSuperDocProxyAgent::SelectGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pPGSuperDoc->SelectGirder(spanIdx,gdrIdx);
}

Float64 CPGSuperDocProxyAgent::GetSectionCutStation()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());

   POSITION pos = m_pPGSuperDoc->GetFirstViewPosition();
   CView* pView;
   while ( pos != NULL )
   {
      pView = m_pPGSuperDoc->GetNextView(pos);
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
   return -999999;
}

/////////////////////////////////////////////////////////////////////////
// ISelectionEx
CSelection CPGSuperDocProxyAgent::GetSelection()
{
   return m_pPGSuperDoc->GetSelection();
}

void CPGSuperDocProxyAgent::SelectDeck()
{
   m_pPGSuperDoc->SelectDeck();
}

void CPGSuperDocProxyAgent::SelectAlignment()
{
   m_pPGSuperDoc->SelectAlignment();
}

void CPGSuperDocProxyAgent::ClearSelection()
{
   m_pPGSuperDoc->ClearSelection();
}

/////////////////////////////////////////////////////////////////////////
// IUpdateTemplates
bool CPGSuperDocProxyAgent::UpdatingTemplates()
{
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)(m_pPGSuperDoc->GetDocTemplate());
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);

   CPGSuperAppPlugin* pPlugin = dynamic_cast<CPGSuperAppPlugin*>(pAppPlugin.p);
   return pPlugin->UpdatingTemplates();
}

/////////////////////////////////////////////////////////////////////////
// IUIEvents
void CPGSuperDocProxyAgent::HoldEvents(bool bHold)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_bHoldingEvents = bHold;
   if ( !m_bHoldingEvents )
      m_UIEvents.clear();
}

void CPGSuperDocProxyAgent::FirePendingEvents()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   if ( m_bHoldingEvents )
   {
      m_bHoldingEvents = false;
      std::vector<UIEvent>::iterator iter;
      for ( iter = m_UIEvents.begin(); iter != m_UIEvents.end(); iter++ )
      {
         UIEvent event = *iter;
         m_pPGSuperDoc->UpdateAllViews(event.pSender,event.lHint,event.pHint.get());
      }
      m_UIEvents.clear();
   }
}

void CPGSuperDocProxyAgent::FireEvent(CView* pSender,LPARAM lHint,boost::shared_ptr<CObject> pHint)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   if ( m_bHoldingEvents )
   {
      UIEvent event;
      event.pSender = pSender;
      event.lHint = lHint;
      event.pHint = pHint;

      // skip all but one result hint - firing multiple result hints 
      // causes the UI to unnecessarilly update multiple times
      bool skip = false;
      std::vector<UIEvent>::iterator iter;
      for ( iter = m_UIEvents.begin(); iter != m_UIEvents.end(); iter++ )
      {
         UIEvent e = *iter;
         if ( MIN_RESULTS_HINT <= e.lHint && e.lHint <= MAX_RESULTS_HINT )
         {
            skip = true;
            break; // a result hint is already queued 
         }
      }

      if (!skip)
      {
         m_UIEvents.push_back(event);
      }
   }
   else
   {
      m_pPGSuperDoc->UpdateAllViews(pSender,lHint,pHint.get());
   }
}

///////////////////////////////////////////////////////////////////////////////////
// IEditByUI
void CPGSuperDocProxyAgent::EditBridgeDescription(int nPage)
{
   m_pPGSuperDoc->EditBridgeDescription(nPage);
}

void CPGSuperDocProxyAgent::EditAlignmentDescription(int nPage)
{
   m_pPGSuperDoc->EditAlignmentDescription(nPage);
}

bool CPGSuperDocProxyAgent::EditGirderDescription(SpanIndexType span,GirderIndexType girder, int nPage)
{
   return m_pPGSuperDoc->EditGirderDescription(span,girder,nPage);
}

bool CPGSuperDocProxyAgent::EditSpanDescription(SpanIndexType spanIdx, int nPage)
{
   return m_pPGSuperDoc->EditSpanDescription(spanIdx,nPage);
}

bool CPGSuperDocProxyAgent::EditPierDescription(PierIndexType pierIdx, int nPage)
{
   return m_pPGSuperDoc->EditPierDescription(pierIdx,nPage);
}

void CPGSuperDocProxyAgent::EditLiveLoads()
{
   return m_pPGSuperDoc->OnLiveLoads();
}

void CPGSuperDocProxyAgent::EditLiveLoadDistributionFactors(pgsTypes::DistributionFactorMethod method,LldfRangeOfApplicabilityAction roaAction)
{
   return m_pPGSuperDoc->OnLoadsLldf(method,roaAction);
}

bool CPGSuperDocProxyAgent::EditPointLoad(CollectionIndexType loadIdx)
{
   return m_pPGSuperDoc->EditPointLoad(loadIdx);
}

bool CPGSuperDocProxyAgent::EditDistributedLoad(CollectionIndexType loadIdx)
{
   return m_pPGSuperDoc->EditDistributedLoad(loadIdx);
}

bool CPGSuperDocProxyAgent::EditMomentLoad(CollectionIndexType loadIdx)
{
   return m_pPGSuperDoc->EditMomentLoad(loadIdx);
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

///////////////////////////////////////////////////////////////////////////////////
// IEditByUIEx
void CPGSuperDocProxyAgent::AddPointLoad(const CPointLoadData& loadData)
{
   return m_pPGSuperDoc->AddPointLoad(loadData);
}

void CPGSuperDocProxyAgent::DeletePointLoad(CollectionIndexType loadIdx)
{
   return m_pPGSuperDoc->DeletePointLoad(loadIdx);
}

void CPGSuperDocProxyAgent::AddDistributedLoad(const CDistributedLoadData& loadData)
{
   return m_pPGSuperDoc->AddDistributedLoad(loadData);
}

void CPGSuperDocProxyAgent::DeleteDistributedLoad(CollectionIndexType loadIdx)
{
   return m_pPGSuperDoc->DeleteDistributedLoad(loadIdx);
}

void CPGSuperDocProxyAgent::AddMomentLoad(const CMomentLoadData& loadData)
{
   return m_pPGSuperDoc->AddMomentLoad(loadData);
}

void CPGSuperDocProxyAgent::DeleteMomentLoad(CollectionIndexType loadIdx)
{
   return m_pPGSuperDoc->DeleteMomentLoad(loadIdx);
}

void CPGSuperDocProxyAgent::EditEffectiveFlangeWidth()
{
   m_pPGSuperDoc->OnEffectiveFlangeWidth();
}


///////////////////////////////////////////////////////////////////////////////////
// IDesign
void CPGSuperDocProxyAgent::DesignGirder(bool bPrompt,bool bDesignSlabOffset,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   m_pPGSuperDoc->DesignGirder(bPrompt,bDesignSlabOffset,spanIdx,gdrIdx);
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
   
   CString strVersion = verInfo.GetProductVersionAsString();

#if defined _DEBUG || defined _BETA_VERSION
   // always include the build number in debug and beta versions
   bIncludeBuildNumber = true;
#endif

   if (!bIncludeBuildNumber)
   {
      // remove the build number
      int pos = strVersion.ReverseFind(_T('.')); // find the last '.'
      strVersion = strVersion.Left(pos);
   }

   return strVersion;
}

// IRegisterViewEvents
Uint32 CPGSuperDocProxyAgent::RegisterBridgePlanViewCallback(IBridgePlanViewEventCallback* pCallback)
{
   return m_pPGSuperDoc->RegisterBridgePlanViewCallback(pCallback);
}

Uint32 CPGSuperDocProxyAgent::RegisterBridgeSectionViewCallback(IBridgeSectionViewEventCallback* pCallback)
{
   return m_pPGSuperDoc->RegisterBridgeSectionViewCallback(pCallback);
}

Uint32 CPGSuperDocProxyAgent::RegisterGirderElevationViewCallback(IGirderElevationViewEventCallback* pCallback)
{
   return m_pPGSuperDoc->RegisterGirderElevationViewCallback(pCallback);
}

Uint32 CPGSuperDocProxyAgent::RegisterGirderSectionViewCallback(IGirderSectionViewEventCallback* pCallback)
{
   return m_pPGSuperDoc->RegisterGirderSectionViewCallback(pCallback);
}

bool CPGSuperDocProxyAgent::UnregisterBridgePlanViewCallback(Uint32 ID)
{
   return m_pPGSuperDoc->UnregisterBridgePlanViewCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterBridgeSectionViewCallback(Uint32 ID)
{
   return m_pPGSuperDoc->UnregisterBridgeSectionViewCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterGirderElevationViewCallback(Uint32 ID)
{
   return m_pPGSuperDoc->UnregisterGirderElevationViewCallback(ID);
}

bool CPGSuperDocProxyAgent::UnregisterGirderSectionViewCallback(Uint32 ID)
{
   return m_pPGSuperDoc->UnregisterGirderSectionViewCallback(ID);
}
