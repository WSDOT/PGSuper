///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
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

#include "StdAfx.h"
#include "resource.h"

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\StatusItem.h>
#include "PGSuper.h"
#include "PGSuperDoc.h"
#include "DocProxyAgent.h"
#include "Hints.h"

#include "BridgeModelViewChildFrame.h"

#include <psgLib\psgLib.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsDocProxyAgent
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsDocProxyAgent::pgsDocProxyAgent()
{
   m_pBroker = 0;
   m_pDoc = 0;
   m_cRef = 0;
   m_bHoldingEvents = false;
}

pgsDocProxyAgent::~pgsDocProxyAgent()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
void pgsDocProxyAgent::SetDocument(CPGSuperDoc* pDoc)
{
   m_pDoc = pDoc;
}

//======================== INQUIRY    =======================================

STDMETHODIMP pgsDocProxyAgent::QueryInterface(const IID& iid, void** ppv)
{
   if ( iid == IID_IUnknown )
   {
      *ppv = static_cast<IAgentEx*>(this);
   }
   else if ( iid == IID_IAgent )
   {
      *ppv = static_cast<IAgentEx*>(this);
   }
   else if ( iid == IID_IAgentEx )
   {
      *ppv = static_cast<IAgentEx*>(this);
   }
   else if ( iid == IID_IBridgeDescriptionEventSink )
   {
      *ppv = static_cast<IBridgeDescriptionEventSink*>(this);
   }
   else if ( iid == IID_IEnvironmentEventSink )
   {
      *ppv = static_cast<IEnvironmentEventSink*>(this);
   }
   else if ( iid == IID_IProjectPropertiesEventSink )
   {
      *ppv = static_cast<IProjectPropertiesEventSink*>(this);
   }
   else if ( iid == IID_IProjectSettingsEventSink )
   {
      *ppv = static_cast<IProjectSettingsEventSink*>(this);
   }
    else if ( iid == IID_ISpecificationEventSink )
   {
      *ppv = static_cast<ISpecificationEventSink*>(this);
   }
   else if ( iid == IID_ILibraryConflictEventSink )
   {
      *ppv = static_cast<ILibraryConflictEventSink*>(this);
   }
   else if ( iid == IID_ILoadModifiersEventSink )
   {
      *ppv = static_cast<ILoadModifiersEventSink*>(this);
   }
   else if ( iid == IID_IFile )
   {
      *ppv = static_cast<IFile*>(this);
   }
   else if ( iid == IID_IStatusCenter )
   {
      *ppv = static_cast<IStatusCenter*>(this);
   }
   else if ( iid == IID_IUpdateTemplates )
   {
      *ppv = static_cast<IUpdateTemplates*>(this);
   }
   else if ( iid == IID_IUIEvents )
   {
      *ppv = static_cast<IUIEvents*>(this);
   }
   else if ( iid == IID_ISelection )
   {
      *ppv = static_cast<ISelection*>(this);
   }
   else if ( iid == IID_IEditByUI )
   {
      *ppv = static_cast<IEditByUI*>(this);
   }
   else
   {
      *ppv = NULL;
      return E_NOINTERFACE;
   }

   reinterpret_cast<IUnknown*>(*ppv)->AddRef();
   return S_OK;
}

STDMETHODIMP_(ULONG) pgsDocProxyAgent::AddRef()
{
   return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) pgsDocProxyAgent::Release()
{
   if ( InterlockedDecrement(&m_cRef) == 0 )
   {
      delete this;
      return 0;
   }

   return m_cRef;
}

   // IAgent
STDMETHODIMP pgsDocProxyAgent::SetBroker(IBroker* pBroker)
{
   AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP pgsDocProxyAgent::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   pBrokerInit->RegInterface( IID_IFile,            this );
   pBrokerInit->RegInterface( IID_IStatusCenter,    this );
   pBrokerInit->RegInterface( IID_IEditByUI,        this );
   pBrokerInit->RegInterface( IID_ISelection,       this );
   pBrokerInit->RegInterface( IID_IUIEvents,        this );
   pBrokerInit->RegInterface( IID_IUpdateTemplates, this );

   return S_OK;
}

STDMETHODIMP pgsDocProxyAgent::Init()
{
   AGENT_INIT;

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

   hr = pBrokerInit->FindConnectionPoint( IID_IProjectSettingsEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwProjectSettingsCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_ISpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwSpecificationCookie );
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

   return S_OK;
}

STDMETHODIMP pgsDocProxyAgent::Init2()
{
   return S_OK;
}

STDMETHODIMP pgsDocProxyAgent::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_NULL;
   return S_OK;
}

STDMETHODIMP pgsDocProxyAgent::Reset()
{
   return S_OK;
}

STDMETHODIMP pgsDocProxyAgent::ShutDown()
{
   AGENT_CLEAR_INTERFACE_CACHE;
   CLOSE_LOGFILE;

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

   hr = pBrokerInit->FindConnectionPoint( IID_IProjectSettingsEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwProjectSettingsCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = pBrokerInit->FindConnectionPoint( IID_ISpecificationEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwSpecificationCookie );
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

   return S_OK;
}

// IBridgeDescriptionEventSink
HRESULT pgsDocProxyAgent::OnBridgeChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   //
   // Check to see if the bridge has changed in such a way that the
   // selected girder is invalid
   //
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();
   SpanIndexType spanIdx = m_pDoc->GetSpanIdx();

   const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);
   bool bClearSelection = false;
   if ( pSpan )
   {
      GirderIndexType nGirders = pSpan->GetGirderCount();
      GirderIndexType gdrIdx   = m_pDoc->GetGirderIdx();

      if ( nGirders < gdrIdx )
         bClearSelection = true;
   }
//   else
//   {
//      bClearSelection = true;
//   }

   if ( bClearSelection )
      m_pDoc->SelectGirder(-1,-1);

   m_pDoc->SetModifiedFlag();
   FireEvent( 0, HINT_BRIDGECHANGED, 0 );

   return S_OK;
}
   
HRESULT pgsDocProxyAgent::OnGirderFamilyChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->SetModifiedFlag();
   FireEvent( 0, HINT_GIRDERFAMILYCHANGED, 0 );
   return S_OK;
}
   
HRESULT pgsDocProxyAgent::OnGirderChanged(SpanIndexType span,GirderIndexType gdr,Uint32 lHint)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->SetModifiedFlag();

   CGirderHint* pHint = new CGirderHint;
   pHint->lHint = lHint;

   FireEvent(NULL,HINT_GIRDERCHANGED,pHint);

   delete pHint;

   return S_OK;
}

HRESULT pgsDocProxyAgent::OnLiveLoadChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->SetModifiedFlag();
   FireEvent( 0, HINT_LIVELOADCHANGED, 0 );
   return S_OK;
}

HRESULT pgsDocProxyAgent::OnLiveLoadNameChanged(const char* strOldName,const char* strNewName)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->SetModifiedFlag();
   FireEvent( 0, HINT_LIVELOADCHANGED, 0 );
   return S_OK;
}

// IEnvironmentEventSink
HRESULT pgsDocProxyAgent::OnExposureConditionChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->SetModifiedFlag();
   FireEvent( 0, HINT_ENVCHANGED, 0 );
   return S_OK;
}

HRESULT pgsDocProxyAgent::OnRelHumidityChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->SetModifiedFlag();
   FireEvent( 0, HINT_ENVCHANGED, 0 );
   return S_OK;
}

// IProjectPropertiesEventSink
HRESULT pgsDocProxyAgent::OnProjectPropertiesChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->SetModifiedFlag();
   FireEvent( 0, HINT_PROJECTPROPERTIESCHANGED, 0 );
   return S_OK;
}

// IProjectSettingsEventSink
HRESULT pgsDocProxyAgent::OnUnitsChanged(Int32 units)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->SetModifiedFlag();
   FireEvent( 0, HINT_UNITSCHANGED, 0 );
   return S_OK;
}

// ISpecificationEventSink
HRESULT pgsDocProxyAgent::OnSpecificationChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->SetModifiedFlag();
   FireEvent( 0, HINT_SPECCHANGED, 0 );
   return S_OK;
}

HRESULT pgsDocProxyAgent::OnAnalysisTypeChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->SetModifiedFlag();
   m_pDoc->UpdateAnalysisTypeStatusIndicator();
   FireEvent( 0, HINT_ANALYSISTYPECHANGED, 0 );
   return S_OK;
}

// ILoadModiferEventSink
HRESULT pgsDocProxyAgent::OnLoadModifiersChanged()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->SetModifiedFlag();
   FireEvent( 0, HINT_LOADMODIFIERSCHANGED, 0 );
   return S_OK;
}

HRESULT pgsDocProxyAgent::OnLibraryConflictResolved()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->SetModifiedFlag();
   return S_OK;
}

std::string pgsDocProxyAgent::GetFilePath()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CFileFind finder;
   std::string strResult;
   if ( finder.FindFile(m_pDoc->GetPathName()) )
   {
      finder.FindNextFile();
      strResult = (LPCSTR)finder.GetFilePath();
      finder.Close();
   }

   return strResult;
}

std::string pgsDocProxyAgent::GetFileTitle()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CFileFind finder;
   std::string strResult;
   if ( finder.FindFile(m_pDoc->GetPathName()) )
   {
      finder.FindNextFile();
      strResult = (LPCSTR)finder.GetFileTitle();
      finder.Close();
   }

   return strResult;
}

std::string pgsDocProxyAgent::GetFileName()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CFileFind finder;
   std::string strResult;
   if ( finder.FindFile(m_pDoc->GetPathName()) )
   {
      finder.FindNextFile();
      strResult = (LPCSTR)finder.GetFileName();
      finder.Close();
   }

   return strResult;
}

std::string pgsDocProxyAgent::GetFileRoot()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CFileFind finder;
   std::string strResult;
   if ( finder.FindFile(m_pDoc->GetPathName()) )
   {
      finder.FindNextFile();
      strResult = (LPCSTR)finder.GetRoot();
      finder.Close();
   }

   return strResult;
}


//////////////////////////////////////////////////////////////
// IStatusCenter
StatusCallbackIDType pgsDocProxyAgent::RegisterCallback(iStatusCallback* pCallback)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->GetStatusCenter().RegisterCallbackItem(pCallback);
}

AgentIDType pgsDocProxyAgent::GetAgentID()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->GetStatusCenter().GetAgentID();
}

StatusItemIDType pgsDocProxyAgent::Add(pgsStatusItem* pItem)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->GetStatusCenter().Add(pItem);
}

bool pgsDocProxyAgent::RemoveByID(StatusItemIDType id)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->GetStatusCenter().RemoveByID(id);
}

bool pgsDocProxyAgent::RemoveByIndex(CollectionIndexType index)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->GetStatusCenter().RemoveByIndex(index);
}

bool pgsDocProxyAgent::RemoveByAgentID(AgentIDType id)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->GetStatusCenter().RemoveByAgentID(id);
}

pgsStatusItem* pgsDocProxyAgent::GetByID(StatusItemIDType id)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->GetStatusCenter().GetByID(id);
}

pgsStatusItem* pgsDocProxyAgent::GetByIndex(CollectionIndexType index)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->GetStatusCenter().GetByIndex(index);
}

pgsTypes::StatusSeverityType pgsDocProxyAgent::GetSeverity(const pgsStatusItem* pItem)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->GetStatusCenter().GetSeverity(pItem->GetCallbackID());
}

CollectionIndexType pgsDocProxyAgent::Count()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->GetStatusCenter().Count();
}

////////////////////////////////////////////////////////////////////
// ISelection
PierIndexType pgsDocProxyAgent::GetPierIdx()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->GetPierIdx();
}

SpanIndexType pgsDocProxyAgent::GetSpanIdx()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->GetSpanIdx();
}

GirderIndexType pgsDocProxyAgent::GetGirderIdx()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->GetGirderIdx();
}

void pgsDocProxyAgent::SelectPier(PierIndexType pierIdx)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->SelectPier(pierIdx);
}

void pgsDocProxyAgent::SelectSpan(SpanIndexType spanIdx)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->SelectSpan(spanIdx);
}

void pgsDocProxyAgent::SelectGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->SelectGirder(spanIdx,gdrIdx);
}

Float64 pgsDocProxyAgent::GetSectionCutStation()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());

   POSITION pos = m_pDoc->GetFirstViewPosition();
   CView* pView;
   while ( pos != NULL )
   {
      pView = m_pDoc->GetNextView(pos);
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
// IUpdatingTemplates
bool pgsDocProxyAgent::UpdatingTemplates()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CPGSuperApp* pApp = (CPGSuperApp*)AfxGetApp();

   return pApp->UpdatingTemplate();
}

/////////////////////////////////////////////////////////////////////////
// IUIEvents
void pgsDocProxyAgent::HoldEvents(bool bHold)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_bHoldingEvents = bHold;
   if ( !m_bHoldingEvents )
      m_UIEvents.clear();
}

void pgsDocProxyAgent::FirePendingEvents()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   if ( m_bHoldingEvents )
   {
      m_bHoldingEvents = false;
      std::vector<UIEvent>::iterator iter;
      for ( iter = m_UIEvents.begin(); iter != m_UIEvents.end(); iter++ )
      {
         UIEvent event = *iter;
         m_pDoc->UpdateAllViews(event.pSender,event.lHint,event.pHint);
      }
      m_UIEvents.clear();
   }
}

void pgsDocProxyAgent::FireEvent(CView* pSender,LPARAM lHint,CObject* pHint)
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
      std::vector<UIEvent>::iterator iter;
      for ( iter = m_UIEvents.begin(); iter != m_UIEvents.end(); iter++ )
      {
         UIEvent e = *iter;
         if ( MIN_RESULTS_HINT <= e.lHint && e.lHint <= MAX_RESULTS_HINT )
            break; // a result hint is already queued 
      }

      m_UIEvents.push_back(event);
   }
   else
   {
      m_pDoc->UpdateAllViews(pSender,lHint,pHint);
   }
}

///////////////////////////////////////////////////////////////////////////////////
// IEditByUI
void pgsDocProxyAgent::EditBridgeDescription(int nPage)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->EditBridgeDescription(nPage);
}

void pgsDocProxyAgent::EditAlignmentDescription(int nPage)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   m_pDoc->EditAlignmentDescription(nPage);
}

bool pgsDocProxyAgent::EditGirderDescription(SpanIndexType span,GirderIndexType girder, int nPage)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->EditGirderDescription(span,girder,nPage);
}

bool pgsDocProxyAgent::EditSpanDescription(SpanIndexType spanIdx, int nPage)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->EditSpanDescription(spanIdx,nPage);
}

bool pgsDocProxyAgent::EditPierDescription(PierIndexType pierIdx, int nPage)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->EditPierDescription(pierIdx,nPage);
}

void pgsDocProxyAgent::EditLiveLoads()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->OnLiveLoads();
}

void pgsDocProxyAgent::EditLiveLoadDistributionFactors(pgsTypes::DistributionFactorMethod method,LldfRangeOfApplicabilityAction roaAction)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->OnLoadsLldf(method,roaAction);
}

bool pgsDocProxyAgent::EditPointLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->EditPointLoad(loadIdx);
}

bool pgsDocProxyAgent::EditDistributedLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->EditDistributedLoad(loadIdx);
}

bool pgsDocProxyAgent::EditMomentLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   return m_pDoc->EditMomentLoad(loadIdx);
}


////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
IUnknown* pgsDocProxyAgent::GetUnknown()
{
   return reinterpret_cast<IUnknown*>(this);
}

//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsDocProxyAgent::AssertValid() const
{
   return true;
}

void pgsDocProxyAgent::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsDocProxyAgent" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsDocProxyAgent::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsDocProxyAgent");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsDocProxyAgent");

   TESTME_EPILOG("DocProxyAgent");
}
#endif // _UNITTEST
