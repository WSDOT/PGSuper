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

// ExampleExtensionAgent.cpp : Implementation of CExampleExtensionAgent

#include "stdafx.h"
#include "ExtensionAgent.h"
#include "ExampleExtensionAgent.h"
#include "resource.h"
#include "MyView.h"
#include "CLSID.h"

#include <IFace\Tools.h>
#include <IFace\EditByUI.h>
#include <EAF\EAFStatusCenter.h>
#include <EAF\Menu.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\Transaction.h>

// Includes for reporting
#include <EAF/EAFReportManager.h>
#include "MyReportSpecification.h"
#include "MyReportSpecificationBuilder.h"
#include "MyChapterBuilder.h"
#include <Reporting\PGSuperTitlePageBuilder.h>

// Includes for graphing
#include <EAF/EAFGraphManager.h>
#include "TestGraphBuilder.h"

#include <EAF\EAFOutputChildFrame.h>
#include <MFCTools\Prompts.h>

BEGIN_MESSAGE_MAP(CExampleExtensionAgent,CCmdTarget)
   ON_COMMAND(ID_COMMAND1,&CExampleExtensionAgent::OnCommand1)
	//ON_UPDATE_COMMAND_UI(ID_COMMAND1, OnUpdateCommand1)
   ON_COMMAND(ID_PLUGINAGENT_MYVIEW, &CExampleExtensionAgent::OnMyView)
END_MESSAGE_MAP()

void CExampleExtensionAgent::OnCommand1()
{
   SimulateUserInput();
}

void CExampleExtensionAgent::OnUpdateCommand1(CCmdUI* pCmdUI)
{
   // example of command ui processing
   //pCmdUI->SetCheck();
}

void CExampleExtensionAgent::OnMyView()
{
   CreateMyView();
}

/////////////////////////////////////////////////////////////////////////////
// IEAFCommandCallback
BOOL CExampleExtensionAgent::OnCommandMessage(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
   return OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

BOOL CExampleExtensionAgent::GetStatusBarMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // load appropriate string
   if (rMessage.LoadString(nID))
   {
      // first newline terminates actual string
      rMessage.Replace('\n', '\0');
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
   if (string.LoadString(nID))
   {
      // tip is after first newline 
      int pos = string.Find('\n');
      if (0 < pos)
         rMessage = string.Mid(pos + 1);
   }
   else
   {
      // not found
      TRACE1("Warning (CExampleExtensionAgent): no tool tip for ID 0x%04X.\n", nID);
   }

   return TRUE;
}

void CExampleExtensionAgent::RegisterViews()
{
   GET_IFACE(IEAFViewRegistrar,pViewRegistrar);
   int maxViewCount = 2; // only allow 2 instances of this view to be created at a time
   auto callback = std::dynamic_pointer_cast<WBFL::EAF::ICommandCallback>(shared_from_this());
   m_MyViewKey = pViewRegistrar->RegisterView(IDR_MENU,callback,RUNTIME_CLASS(CEAFOutputChildFrame),RUNTIME_CLASS(CMyView),nullptr,maxViewCount);
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
   GET_IFACE(IEAFGraphManager,pGraphMgr);

   std::unique_ptr<CTestGraphBuilder> pTestGraphBuilder = std::make_unique<CTestGraphBuilder>();
   pTestGraphBuilder->SetMenuBitmap(&m_bmpMenu);
   pGraphMgr->AddGraphBuilder(std::move(pTestGraphBuilder));

   std::unique_ptr<CTestGraphBuilder2> pTestGraphBuilder2 = std::make_unique<CTestGraphBuilder2>();
   pTestGraphBuilder2->SetMenuBitmap(&m_bmpMenu);
   pGraphMgr->AddGraphBuilder(std::move(pTestGraphBuilder2));

   std::unique_ptr<CTestGraphBuilder3> pTestGraphBuilder3 = std::make_unique<CTestGraphBuilder3>();
   pTestGraphBuilder3->SetMenuBitmap(&m_bmpMenu);
   pGraphMgr->AddGraphBuilder(std::move(pTestGraphBuilder3));
}

void CExampleExtensionAgent::CreateMenus()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IEAFMainMenu,pMainMenu);
   auto pMenu = pMainMenu->GetMainMenu();

   INT nMenus = pMenu->GetMenuItemCount();
   if ( nMenus == 0 )
      return;

   m_MyMenu = pMenu->CreatePopupMenu(nMenus-1,_T("MyExtension")); // put the menu before the last menu (Help)

   auto callback = std::dynamic_pointer_cast<WBFL::EAF::ICommandCallback>(shared_from_this());
   m_MyMenu->LoadMenu(IDR_MENU,callback);
}

void CExampleExtensionAgent::RemoveMenus()
{
   GET_IFACE(IEAFMainMenu,pMainMenu);
   auto pMenu = pMainMenu->GetMainMenu();
   pMenu->DestroyMenu(m_MyMenu);
}

void CExampleExtensionAgent::CreateToolBar()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   GET_IFACE(IEAFToolbars,pToolBars);
   m_ToolBarID = pToolBars->CreateToolBar(_T("Extension Agent Toolbar"));
   auto pToolBar = pToolBars->GetToolBar(m_ToolBarID);
   auto callback = std::dynamic_pointer_cast<WBFL::EAF::ICommandCallback>(shared_from_this());
   pToolBar->LoadToolBar(IDR_TOOLBAR,callback);

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
   GET_IFACE(IEAFReportManager,pRptMgr);

   //
   // Create report spec builders
   //

   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pMyRptSpecBuilder(std::make_shared<CMyReportSpecificationBuilder>(m_pBroker) );

   // My report
   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Extension Agent Report")));
   pRptBuilder->SetMenuBitmap(&m_bmpMenu);
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(std::make_shared<CPGSuperTitlePageBuilder>(m_pBroker,pRptBuilder->GetName(),false)) );
   pRptBuilder->SetReportSpecificationBuilder( pMyRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CMyChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CExampleExtensionAgent::RegisterUIExtensions()
{
   GET_IFACE(IExtendPGSuperUI,pExtendPGSuperUI);
   m_EditBridgeCallbackID = pExtendPGSuperUI->RegisterEditBridgeCallback(this);
   m_EditPierCallbackID = pExtendPGSuperUI->RegisterEditPierCallback(this,nullptr);
   m_EditSpanCallbackID = pExtendPGSuperUI->RegisterEditSpanCallback(this);
   m_EditGirderCallbackID = pExtendPGSuperUI->RegisterEditGirderCallback(this);

   GET_IFACE(IExtendPGSpliceUI,pExtendPGSpliceUI);
   m_EditTemporarySupportCallbackID = pExtendPGSpliceUI->RegisterEditTemporarySupportCallback(this, nullptr);
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

bool CExampleExtensionAgent::RegisterInterfaces()
{
   EAF_AGENT_REGISTER_INTERFACES;
   return true;
}

bool CExampleExtensionAgent::Init()
{
   EAF_AGENT_INIT;
   CREATE_LOGFILE(_T("ExtensionAgent"));

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   VERIFY(m_bmpMenu.LoadBitmap(IDB_LOGO));

   //
   // Attach to connection points
   //
   m_dwExtendUICookie = REGISTER_EVENT_SINK(IExtendUIEventSink);

   return true;
}

bool CExampleExtensionAgent::Reset()
{
   EAF_AGENT_RESET;
   m_bmpMenu.DeleteObject();
   return true;
}

bool CExampleExtensionAgent::ShutDown()
{
   EAF_AGENT_SHUTDOWN;

   //
   // Detach to connection points
   //
   UNREGISTER_EVENT_SINK(IExtendUIEventSink, m_dwExtendUICookie);

   return true;
}

CLSID CExampleExtensionAgent::GetCLSID() const
{
   return CLSID_ExampleExtensionAgent;
}


////////////////////////////////////////////////////////////////////
// IAgentPersist
WBFL::EAF::Broker::LoadResult CExampleExtensionAgent::Load(IStructuredLoad* pStrLoad)
{
   USES_CONVERSION;
   CComVariant var;
   var.vt = VT_BSTR;
   
   HRESULT hr = pStrLoad->BeginUnit(_T("ExampleExtensionAgent"));
   if ( FAILED(hr) )
      return WBFL::EAF::Broker::LoadResult::Error;

   var.vt = VT_BSTR;
   hr = pStrLoad->get_Property(_T("SampleData"),&var);
   if ( FAILED(hr) )
      return WBFL::EAF::Broker::LoadResult::Error;

   m_Answer = OLE2T(var.bstrVal);

   hr = pStrLoad->EndUnit();
   if ( FAILED(hr) )
      return WBFL::EAF::Broker::LoadResult::Error;

   return WBFL::EAF::Broker::LoadResult::Success;
}

bool CExampleExtensionAgent::Save(IStructuredSave* pStrSave)
{
   pStrSave->BeginUnit(_T("ExampleExtensionAgent"),1.0);
   pStrSave->put_Property(_T("SampleData"),CComVariant(m_Answer));
   pStrSave->EndUnit();
   return true;
}


////////////////////////////////////////////////////////////////////
// IAgentUIIntegration
bool CExampleExtensionAgent::IntegrateWithUI(bool bIntegrate)
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

   return true;
}

////////////////////////////////////////////////////////////////////
// IAgentReportingIntegration
bool CExampleExtensionAgent::IntegrateWithReporting(bool bIntegrate)
{
   if ( bIntegrate )
   {
      RegisterReports();
   }
   else
   {
   }

   return true;
}

////////////////////////////////////////////////////////////////////
// IAgentGraphingIntegration
bool CExampleExtensionAgent::IntegrateWithGraphing(bool bIntegrate)
{
   if ( bIntegrate )
   {
      RegisterGraphs();
   }
   else
   {
   }

   return true;
}

CPropertyPage* CExampleExtensionAgent::CreatePropertyPage(IEditBridgeData* pBridgeData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CPropertyPage(IDD_EDIT_PIER_PAGE);
}

std::unique_ptr<WBFL::EAF::Transaction> CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditBridgeData* pBridgeData)
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

std::unique_ptr<WBFL::EAF::Transaction> CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditPierData* pEditPierData)
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

std::unique_ptr<WBFL::EAF::Transaction> CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditTemporarySupportData* pEditTemporarySupportData)
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

std::unique_ptr<WBFL::EAF::Transaction> CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditSpanData* pSpanData)
{
   return nullptr;
}

CPropertyPage* CExampleExtensionAgent::CreatePropertyPage(IEditSegmentData* pSegmentData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CPropertyPage(IDD_EDIT_PIER_PAGE);
}

std::unique_ptr<WBFL::EAF::Transaction> CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditSegmentData* pSegmentData)
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

std::unique_ptr<WBFL::EAF::Transaction> CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditClosureJointData* pClosureJointData)
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

std::unique_ptr<WBFL::EAF::Transaction> CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditSplicedGirderData* pGirderData)
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

std::unique_ptr<WBFL::EAF::Transaction> CExampleExtensionAgent::OnOK(CPropertyPage* pPage,IEditGirderData* pGirderData)
{
   return nullptr;
}

HRESULT CExampleExtensionAgent::OnHintsReset()
{
   AfxMessageBox(_T("Example Extension Agent - Hints Reset"));
   return S_OK;
}
