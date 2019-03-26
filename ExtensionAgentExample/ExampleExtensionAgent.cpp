///////////////////////////////////////////////////////////////////////
// ExtensionAgentExample - Extension Agent Example Project for PGSuper
// Copyright © 1999-2019  Washington State Department of Transportation
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

// ExampleExtensionAgent.cpp : Implementation of CExampleExtensionAgent

#include "stdafx.h"
#include "ExampleExtensionAgent.h"

#include <EAF\EAFOutputChildFrame.h>
#include "MyView.h"

#include <IFace\Tools.h>
#include <IFace\EditByUI.h>
#include <EAF\EAFStatusCenter.h>
#include <EAF\EAFMenu.h>
#include <EAF\EAFDisplayUnits.h>

// Includes for reporting
#include <IReportManager.h>
#include "MyReportSpecification.h"
#include "MyReportSpecificationBuilder.h"
#include "MyChapterBuilder.h"
#include <Reporting\PGSuperTitlePageBuilder.h>

// Includes for graphing
#include <IGraphManager.h>
#include "TestGraphBuilder.h"

#include <MFCTools\Prompts.h>

BEGIN_MESSAGE_MAP(CMyCmdTarget,CCmdTarget)
   ON_COMMAND(ID_COMMAND1,OnCommand1)
	//ON_UPDATE_COMMAND_UI(ID_COMMAND1, OnUpdateCommand1)
   ON_COMMAND(ID_PLUGINAGENT_MYVIEW,OnMyView)
END_MESSAGE_MAP()

void CMyCmdTarget::OnCommand1()
{
   m_pMyAgent->SimulateUserInput();
}

void CMyCmdTarget::OnUpdateCommand1(CCmdUI* pCmdUI) 
{
   // example of command ui processing
   //pCmdUI->SetCheck();
}

void CMyCmdTarget::OnMyView()
{
   m_pMyAgent->CreateMyView();
}

// CExampleExtensionAgent

HRESULT CExampleExtensionAgent::FinalConstruct()
{
   m_MyCommandTarget.m_pMyAgent = this;

   m_bCheck = true;

	return S_OK;
}

void CExampleExtensionAgent::RegisterViews()
{
   GET_IFACE(IEAFViewRegistrar,pViewRegistrar);
   int maxViewCount = 2; // only allow 2 instances of this view to be created at a time
   m_MyViewKey = pViewRegistrar->RegisterView(IDR_MENU,this,RUNTIME_CLASS(CEAFOutputChildFrame),RUNTIME_CLASS(CMyView),nullptr,maxViewCount);
}

void CExampleExtensionAgent::UnregisterViews()
{
   GET_IFACE(IEAFViewRegistrar,pViewRegistrar);
   pViewRegistrar->RemoveView(m_MyViewKey);
}

void CExampleExtensionAgent::CreateMyView()
{
   GET_IFACE(IEAFViewRegistrar,pViewRegistrar);
   pViewRegistrar->CreateView(m_MyViewKey,this);
}

void CExampleExtensionAgent::RegisterGraphs()
{
   GET_IFACE(IGraphManager,pGraphMgr);

   CTestGraphBuilder* pTestGraphBuilder = new CTestGraphBuilder;
   pTestGraphBuilder->SetMenuBitmap(&m_bmpMenu);
   pGraphMgr->AddGraphBuilder(pTestGraphBuilder);

   CTestGraphBuilder2* pTestGraphBuilder2 = new CTestGraphBuilder2;
   pTestGraphBuilder2->SetMenuBitmap(&m_bmpMenu);
   pGraphMgr->AddGraphBuilder(pTestGraphBuilder2);

   CTestGraphBuilder3* pTestGraphBuilder3 = new CTestGraphBuilder3;
   pTestGraphBuilder3->SetMenuBitmap(&m_bmpMenu);
   pGraphMgr->AddGraphBuilder(pTestGraphBuilder3);
}

void CExampleExtensionAgent::CreateMenus()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IEAFMainMenu,pMainMenu);
   CEAFMenu* pMenu = pMainMenu->GetMainMenu();

   INT nMenus = pMenu->GetMenuItemCount();
   if ( nMenus == 0 )
      return;

   m_pMyMenu = pMenu->CreatePopupMenu(nMenus-1,_T("MyExtension")); // put the menu before the last menu (Help)
   m_pMyMenu->LoadMenu(IDR_MENU,this);
}

void CExampleExtensionAgent::RemoveMenus()
{
   GET_IFACE(IEAFMainMenu,pMainMenu);
   CEAFMenu* pMenu = pMainMenu->GetMainMenu();
   pMenu->DestroyMenu(m_pMyMenu);
}

void CExampleExtensionAgent::CreateToolBar()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   GET_IFACE(IEAFToolbars,pToolBars);
   m_ToolBarID = pToolBars->CreateToolBar(_T("Extension Agent Toolbar"));
   CEAFToolBar* pToolBar = pToolBars->GetToolBar(m_ToolBarID);
   pToolBar->LoadToolBar(IDR_TOOLBAR,this);

   //GET_IFACE(IEditByUI,pEditUI);
   //UINT stdID = pEditUI->GetStdToolBarID();
   //CEAFToolBar* pStdToolBar = pToolBars->GetToolBar(stdID);
   //UINT cmdID = ID_COMMAND1;
   //pStdToolBar->InsertButton(-1,cmdID,-1,_T("Extension Command"),this);
}

void CExampleExtensionAgent::RemoveToolBar()
{
   GET_IFACE(IEAFToolbars,pToolBars);
   pToolBars->DestroyToolBar(m_ToolBarID);

   //GET_IFACE(IEditByUI,pEditUI);
   //UINT stdID = pEditUI->GetStdToolBarID();
   //CEAFToolBar* pStdToolBar = pToolBars->GetToolBar(stdID);
   //pStdToolBar->RemoveButtons(this);
}

void CExampleExtensionAgent::RegisterReports()
{
   // Register our reports
   GET_IFACE(IReportManager,pRptMgr);

   //
   // Create report spec builders
   //

   std::shared_ptr<CReportSpecificationBuilder> pMyRptSpecBuilder(std::make_shared<CMyReportSpecificationBuilder>(m_pBroker) );

   // My report
   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Extension Agent Report")));
   pRptBuilder->SetMenuBitmap(&m_bmpMenu);
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName(),false)) );
   pRptBuilder->SetReportSpecificationBuilder( pMyRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CMyChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

void CExampleExtensionAgent::RegisterUIExtensions()
{
   GET_IFACE(IExtendPGSuperUI,pExtendPGSuperUI);
   m_EditBridgeCallbackID = pExtendPGSuperUI->RegisterEditBridgeCallback(this);
   m_EditPierCallbackID = pExtendPGSuperUI->RegisterEditPierCallback(this);
   m_EditSpanCallbackID = pExtendPGSuperUI->RegisterEditSpanCallback(this);
   m_EditGirderCallbackID = pExtendPGSuperUI->RegisterEditGirderCallback(this);

   GET_IFACE(IExtendPGSpliceUI,pExtendPGSpliceUI);
   m_EditTemporarySupportCallbackID = pExtendPGSpliceUI->RegisterEditTemporarySupportCallback(this);
   m_EditSplicedGirderCallbackID = pExtendPGSpliceUI->RegisterEditSplicedGirderCallback(this);
   m_EditSegmentCallbackID = pExtendPGSpliceUI->RegisterEditSegmentCallback(this);
   m_EditClosureJointCallbackID = pExtendPGSpliceUI->RegisterEditClosureJointCallback(this);
}

void CExampleExtensionAgent::UnregisterUIExtensions()
{
   GET_IFACE(IExtendPGSuperUI,pExtendPGSuperUI);
   pExtendPGSuperUI->UnregisterEditBridgeCallback(m_EditBridgeCallbackID);
   pExtendPGSuperUI->UnregisterEditPierCallback(m_EditPierCallbackID);
   pExtendPGSuperUI->UnregisterEditSpanCallback(m_EditSpanCallbackID);
   pExtendPGSuperUI->UnregisterEditGirderCallback(m_EditGirderCallbackID);

   GET_IFACE(IExtendPGSpliceUI,pExtendPGSpliceUI);
   pExtendPGSpliceUI->UnregisterEditTemporarySupportCallback(m_EditTemporarySupportCallbackID);
   pExtendPGSpliceUI->UnregisterEditSplicedGirderCallback(m_EditSplicedGirderCallbackID);
   pExtendPGSpliceUI->UnregisterEditSegmentCallback(m_EditSegmentCallbackID);
   pExtendPGSpliceUI->UnregisterEditClosureJointCallback(m_EditClosureJointCallbackID);
}

void CExampleExtensionAgent::SimulateUserInput()
{
   CString answer;
   if (AfxQuestion(_T("Input Dialog"),_T("This is an input dialog from a PGSuper Extension Agent.\nType some input text."),m_Answer,answer) && m_Answer != answer)
   {
      m_Answer = answer;

      GET_IFACE(IEAFDocument,pDoc);
      pDoc->SetModified(TRUE);
   }
}

/////////////////////////////////////////////////////////////////////////
// IAgentEx

STDMETHODIMP CExampleExtensionAgent::SetBroker(IBroker *pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CExampleExtensionAgent::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   // Register interfaces here
   // pBrokerInit->RegInterface( IID_ISomeInterfaceThisAgentImplements, this);

   return S_OK;
}

STDMETHODIMP CExampleExtensionAgent::Init()
{
   CREATE_LOGFILE(_T("ExtensionAgent"));
   EAF_AGENT_INIT;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   VERIFY(m_bmpMenu.LoadBitmap(IDB_LOGO));

   //
   // Attach to connection points
   //
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   // Connection point for the user interface extension events
   hr = pBrokerInit->FindConnectionPoint( IID_IExtendUIEventSink, &pCP );
   if ( SUCCEEDED(hr) )
   {
      hr = pCP->Advise( GetUnknown(), &m_dwExtendUICookie );
      ATLASSERT( SUCCEEDED(hr) );
      pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.
   }

   return S_OK;
}

STDMETHODIMP CExampleExtensionAgent::Init2()
{
   return S_OK;
}

STDMETHODIMP CExampleExtensionAgent::Reset()
{
   m_bmpMenu.DeleteObject();
   return S_OK;
}

STDMETHODIMP CExampleExtensionAgent::ShutDown()
{
   //
   // Detach to connection points
   //
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   hr = pBrokerInit->FindConnectionPoint(IID_IExtendUIEventSink, &pCP );
   if ( SUCCEEDED(hr) )
   {
      hr = pCP->Unadvise( m_dwExtendUICookie );
      ATLASSERT( SUCCEEDED(hr) );
      pCP.Release(); // Recycle the connection point
   }

   return S_OK;
}

STDMETHODIMP CExampleExtensionAgent::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_ExampleExtensionAgent;
   return S_OK;
}


////////////////////////////////////////////////////////////////////
// IAgentPersist
STDMETHODIMP CExampleExtensionAgent::Load(IStructuredLoad* pStrLoad)
{
   USES_CONVERSION;
   CComVariant var;
   var.vt = VT_BSTR;
   
   HRESULT hr = pStrLoad->BeginUnit(_T("ExampleExtensionAgent"));
   if ( FAILED(hr) )
      return hr;

   var.vt = VT_BSTR;
   hr = pStrLoad->get_Property(_T("SampleData"),&var);
   if ( FAILED(hr) )
      return hr;

   m_Answer = OLE2T(var.bstrVal);

   hr = pStrLoad->EndUnit();
   if ( FAILED(hr) )
      return hr;

   return S_OK;
}

STDMETHODIMP CExampleExtensionAgent::Save(IStructuredSave* pStrSave)
{
   pStrSave->BeginUnit(_T("ExampleExtensionAgent"),1.0);
   pStrSave->put_Property(_T("SampleData"),CComVariant(m_Answer));
   pStrSave->EndUnit();
   return S_OK;
}


////////////////////////////////////////////////////////////////////
// IAgentUIIntegration
STDMETHODIMP CExampleExtensionAgent::IntegrateWithUI(BOOL bIntegrate)
{
   if ( bIntegrate )
   {
      CreateMenus();
      CreateToolBar();
      RegisterViews();
      RegisterUIExtensions();
   }
   else
   {
      RemoveMenus();
      RemoveToolBar();
      UnregisterViews();
      UnregisterUIExtensions();
   }

   return S_OK;
}

////////////////////////////////////////////////////////////////////
// IAgentReportingIntegration
STDMETHODIMP CExampleExtensionAgent::IntegrateWithReporting(BOOL bIntegrate)
{
   if ( bIntegrate )
   {
      RegisterReports();
   }
   else
   {
   }

   return S_OK;
}

////////////////////////////////////////////////////////////////////
// IAgentGraphingIntegration
STDMETHODIMP CExampleExtensionAgent::IntegrateWithGraphing(BOOL bIntegrate)
{
   if ( bIntegrate )
   {
      RegisterGraphs();
   }
   else
   {
   }

   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IEAFCommandCallback
BOOL CExampleExtensionAgent::OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo)
{
   return m_MyCommandTarget.OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
}

BOOL CExampleExtensionAgent::GetStatusBarMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // load appropriate string
	if ( rMessage.LoadString(nID) )
	{
		// first newline terminates actual string
      rMessage.Replace('\n','\0');
	}
	else
	{
		// not found
		TRACE1("Warning (CExampleExtensionAgent): no message line prompt for ID 0x%04X.\n", nID);
	}

   return TRUE;
}

BOOL CExampleExtensionAgent::GetToolTipMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString string;
   // load appropriate string
	if ( string.LoadString(nID) )
	{
		// tip is after first newline 
      int pos = string.Find('\n');
      if ( 0 < pos )
         rMessage = string.Mid(pos+1);
	}
	else
	{
		// not found
		TRACE1("Warning (CExampleExtensionAgent): no tool tip for ID 0x%04X.\n", nID);
	}

   return TRUE;
}

CPropertyPage* CExampleExtensionAgent::CreatePropertyPage(IEditBridgeData* pBridgeData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CPropertyPage(IDD_EDIT_PIER_PAGE);
}

txnTransaction* CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditBridgeData* pBridgeData)
{
   return nullptr;
}

void CExampleExtensionAgent::EditPier_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pPierPropertyPage)
{
}

void CExampleExtensionAgent::EditTemporarySupport_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pTempSupportPropertyPage)
{
}

void CExampleExtensionAgent::EditSpan_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pSpanPropertyPage)
{
}

CPropertyPage* CExampleExtensionAgent::CreatePropertyPage(IEditPierData* pEditPierData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CEditPierPage* pPage = new CEditPierPage(pEditPierData);
   pPage->m_bCheck = m_bCheck;
   return pPage;
}

CPropertyPage* CExampleExtensionAgent::CreatePropertyPage(IEditPierData* pEditPierData,CPropertyPage* pBridgePropertyPage)
{
   ATLASSERT(false); // should never get here
   return nullptr;
}

txnTransaction* CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditPierData* pEditPierData)
{
   CEditPierPage* pMyPage = (CEditPierPage*)pPage;
   m_bCheck = pMyPage->m_bCheck;
   return nullptr;
}

IDType CExampleExtensionAgent::GetEditBridgeCallbackID()
{
   return INVALID_ID;
}

CPropertyPage* CExampleExtensionAgent::CreatePropertyPage(IEditTemporarySupportData* pEditTemporarySupportData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CPropertyPage(IDD_EDIT_PIER_PAGE);
}

CPropertyPage* CExampleExtensionAgent::CreatePropertyPage(IEditTemporarySupportData* pEditTemporarySupportData,CPropertyPage* pBridgePropertyPage)
{
   ATLASSERT(false);
   return nullptr;
}

txnTransaction* CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditTemporarySupportData* pEditTemporarySupportData)
{
   return nullptr;
}

CPropertyPage* CExampleExtensionAgent::CreatePropertyPage(IEditSpanData* pSpanData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CPropertyPage(IDD_EDIT_PIER_PAGE);
}

CPropertyPage* CExampleExtensionAgent::CreatePropertyPage(IEditSpanData* pEditSpanData,CPropertyPage* pBridgePropertyPage)
{
   ATLASSERT(false);
   return nullptr;
}

txnTransaction* CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditSpanData* pSpanData)
{
   return nullptr;
}

CPropertyPage* CExampleExtensionAgent::CreatePropertyPage(IEditSegmentData* pSegmentData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CPropertyPage(IDD_EDIT_PIER_PAGE);
}

txnTransaction* CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditSegmentData* pSegmentData)
{
   return nullptr;
}

IDType CExampleExtensionAgent::GetEditSplicedGirderCallbackID()
{
   return INVALID_ID;
}

CPropertyPage* CExampleExtensionAgent::CreatePropertyPage(IEditSegmentData* pEditSegmentData,CPropertyPage* pSplicedGirderPropertyPage)
{
   return nullptr;
}

CPropertyPage* CExampleExtensionAgent::CreatePropertyPage(IEditClosureJointData* pClosureJointData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CPropertyPage(IDD_EDIT_PIER_PAGE);
}

txnTransaction* CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditClosureJointData* pClosureJointData)
{
   return nullptr;
}

CPropertyPage* CExampleExtensionAgent::CreatePropertyPage(IEditClosureJointData* pEditClosureJointData,CPropertyPage* pSplicedGirderPropertyPage)
{
   return nullptr;
}

CPropertyPage* CExampleExtensionAgent::CreatePropertyPage(IEditSplicedGirderData* pGirderData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CPropertyPage(IDD_EDIT_PIER_PAGE);
}

txnTransaction* CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditSplicedGirderData* pGirderData)
{
   return nullptr;
}

void CExampleExtensionAgent::EditSegment_OnOK(CPropertyPage* pSplicedGirderPropertyPage,CPropertyPage* pSegmentPropertyPage)
{
}

void CExampleExtensionAgent::EditClosureJoint_OnOK(CPropertyPage* pSplicedGirderPropertyPage,CPropertyPage* pClosureJointPropertyPage)
{
}

CPropertyPage* CExampleExtensionAgent::CreatePropertyPage(IEditGirderData* pGirderData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CPropertyPage(IDD_EDIT_PIER_PAGE);
}

txnTransaction* CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditGirderData* pGirderData)
{
   return nullptr;
}

HRESULT CExampleExtensionAgent::OnHintsReset()
{
   AfxMessageBox(_T("Example Extension Agent - Hints Reset"));
   return S_OK;
}
