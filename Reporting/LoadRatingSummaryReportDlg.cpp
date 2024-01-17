///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// LoadRatingSummaryReportDlg.cpp : implementation file
//

#include "stdafx.h"
#include <Reporting\LoadRatingSummaryReportDlg.h>
#include <Reporting\LoadRatingReportSpecificationBuilder.h>
#include "..\Documentation\PGSuper.hh"

#include <initguid.h>
#include <IFace\Tools.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>

#include <PgsExt\GirderLabel.h>
#include <EAF\EAFDocument.h>

#include <MFCTools\AutoRegistry.h>
#include "..\PGSuperAppPlugin\PGSuperBaseAppPlugin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CLoadRatingSummaryReportDlg dialog

IMPLEMENT_DYNAMIC(CLoadRatingSummaryReportDlg, CDialog)

CLoadRatingSummaryReportDlg::CLoadRatingSummaryReportDlg(IBroker* pBroker,const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec,UINT nIDTemplate,CWnd* pParent)
	: CDialog(nIDTemplate, pParent), m_RptDesc(rptDesc), m_pInitRptSpec(pRptSpec)
{
   m_Girder = 0;
   m_bReportAtAllPoi = false;

   m_pBroker = pBroker;

   m_pGrid = new CMultiGirderSelectGrid();
   m_RadioNum = 0;
}

CLoadRatingSummaryReportDlg::~CLoadRatingSummaryReportDlg()
{
   delete m_pGrid;
}

void CLoadRatingSummaryReportDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_ChList);

   DDX_CBIndex(pDX, IDC_GIRDER, (int&)m_Girder);
   DDX_Radio(pDX, IDC_RADIO_GIRDERLINE, m_RadioNum);

   int idx = (m_bReportAtAllPoi ? 1 : 0);
   DDX_CBIndex(pDX, IDC_POI, idx);
   m_bReportAtAllPoi = (idx == 0 ? false : true);

   if ( pDX->m_bSaveAndValidate )
   {
      m_ChapterList.clear();

      int cSelChapters = 0;
      int cChapters = m_ChList.GetCount();
      for ( int ch = 0; ch < cChapters; ch++ )
      {
         if ( m_ChList.GetCheck( ch ) == 1 )
         {
            cSelChapters++;

            CString strChapter;
            m_ChList.GetText(ch,strChapter);

            m_ChapterList.push_back(std::_tstring(strChapter));
         }
      }

      if ( cSelChapters == 0 )
      {
         pDX->PrepareCtrl(IDC_LIST);
         AfxMessageBox(IDS_E_NOCHAPTERS);
         pDX->Fail();
      }

      // Girders
      m_GirderKeys.clear();
      if (m_RadioNum == 0)
      {
         // make list of all girders along girder line
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker, IBridge, pBridge);
         GroupIndexType ngrps = pBridge->GetGirderGroupCount();
         for (GroupIndexType igrp = 0; igrp < ngrps; igrp++)
         {
            m_GirderKeys.push_back(CGirderKey(igrp, m_Girder));
         }

         m_bIsSingleGirderLineSelected = true;
      }
      else
      {
         m_GirderKeys = m_pGrid->GetData();
         if (m_GirderKeys.empty())
         {
            pDX->PrepareCtrl(IDC_SELECT_GRID);
            AfxMessageBox(IDS_E_NOGIRDERS);
            pDX->Fail();
         }

         m_bIsSingleGirderLineSelected = false;
      }
   }
}

BEGIN_MESSAGE_MAP(CLoadRatingSummaryReportDlg, CDialog)
	ON_COMMAND(IDHELP, OnHelp)
   ON_BN_CLICKED(IDC_SELECT_ALL, &CLoadRatingSummaryReportDlg::OnBnClickedSelectAll)
   ON_BN_CLICKED(IDC_CLEAR_ALL, &CLoadRatingSummaryReportDlg::OnBnClickedClearAll)
   ON_BN_CLICKED(IDC_RADIO_GIRDERLINE, &CLoadRatingSummaryReportDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_RADIO_INDIV_GIRDER, &CLoadRatingSummaryReportDlg::OnBnClickedRadio)
   ON_WM_DESTROY()
END_MESSAGE_MAP()

// CLoadRatingSummaryReportDlg message handlers
void CLoadRatingSummaryReportDlg::UpdateGirderComboBox()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   GET_IFACE( IBridge, pBridge );

   CComboBox* pGdrBox = (CComboBox*)GetDlgItem(IDC_GIRDER);
   Uint16 curSel = pGdrBox->GetCurSel();
   pGdrBox->ResetContent();

   GirderIndexType cGirders = 0;
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType i = 0; i < nGroups; i++ )
   {
      cGirders = Max(cGirders,pBridge->GetGirderCount(i));
   }

   for ( GirderIndexType j = 0; j < cGirders; j++ )
   {
      CString strGdr;
      strGdr.Format( _T("Girder %s"), LABEL_GIRDER(j));
      pGdrBox->AddString( strGdr );
   }

   if ( pGdrBox->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR )
   {
      pGdrBox->SetCurSel(0);
   }
}

void CLoadRatingSummaryReportDlg::UpdateChapterList()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Clear out the list box
   m_ChList.ResetContent();

   // Get the chapters in the report
   std::vector<WBFL::Reporting::ChapterInfo> chInfos = m_RptDesc.GetChapterInfo();

   // Populate the list box with the names of the chapters
   std::vector<WBFL::Reporting::ChapterInfo>::iterator iter;
   for ( iter = chInfos.begin(); iter != chInfos.end(); iter++ )
   {
      WBFL::Reporting::ChapterInfo chInfo = *iter;

      int idx = m_ChList.AddString( chInfo.Name.c_str() );
      if ( idx != LB_ERR ) // no error
      {
         m_ChList.SetCheck( idx, chInfo.Select ? 1 : 0 ); // turn the check on
         m_ChList.SetItemData( idx, chInfo.MaxLevel );
      }
   }

   // don't select any items in the chapter list
   m_ChList.SetCurSel(-1);
}

BOOL CLoadRatingSummaryReportDlg::OnInitDialog()
{
   if (!m_GirderKeys.empty())
   {
      m_Girder = m_GirderKeys.front().girderIndex;
   }

 	m_pGrid->SubclassDlgItem(IDC_SELECT_GRID, this);

   CWnd* pwndTitle = GetDlgItem(IDC_REPORT_TITLE);
   pwndTitle->SetWindowText(m_RptDesc.GetReportName().c_str());

   UpdateGirderComboBox();

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_POI);
   pCB->AddString(_T("Report at controlling and 10th points"));
   pCB->AddString(_T("Report at all points"));

   // load settings from registry. may be overridden by spec
   LoadSettings();

   if ( m_pInitRptSpec )
   {
      InitFromRptSpec();
   }

   CDialog::OnInitDialog();

   UpdateChapterList();

   if ( m_pInitRptSpec )
   {
      InitChapterListFromSpec();
   }

   // need list of groups/girders
   GET_IFACE( IBridge, pBridge );
   GroupGirderOnCollection coll;
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      std::vector<bool> gdrson;
      gdrson.assign(nGirders, false); // set all to false

      coll.push_back(gdrson);
   }

   // set selected girders
   std::vector<CGirderKey>::iterator iter(m_GirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_GirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      CGirderKey& girderKey(*iter);

      if (girderKey.groupIndex < nGroups && girderKey.groupIndex != ALL_GROUPS)
      {
         std::vector<bool>& rgdrson = coll[girderKey.groupIndex];
         if (girderKey.girderIndex < (GirderIndexType)rgdrson.size())
         {
            rgdrson[girderKey.girderIndex] = true;
         }
      }
   }

   m_pGrid->CustomInit(coll,pgsGirderLabel::GetGirderLabel);

   OnBnClickedRadio();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CLoadRatingSummaryReportDlg::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_LOADRATING_SUMMARY_REPORT );
}

void CLoadRatingSummaryReportDlg::ClearChapterCheckMarks()
{
   int cChapters = m_ChList.GetCount();
   for ( int ch = 0; ch < cChapters; ch++ )
   {
      m_ChList.SetCheck(ch,0);
   }
}

void CLoadRatingSummaryReportDlg::InitChapterListFromSpec()
{
   ClearChapterCheckMarks();
   std::vector<WBFL::Reporting::ChapterInfo> chInfo = m_pInitRptSpec->GetChapterInfo();
   std::vector<WBFL::Reporting::ChapterInfo>::iterator iter;
   for ( iter = chInfo.begin(); iter != chInfo.end(); iter++ )
   {
      WBFL::Reporting::ChapterInfo& ch = *iter;
      int cChapters = m_ChList.GetCount();
      for ( int idx = 0; idx < cChapters; idx++ )
      {
         CString strChapter;
         m_ChList.GetText(idx,strChapter);
         if ( strChapter == ch.Name.c_str() )
         {
            m_ChList.SetCheck(idx,1);
            break;
         }
      }
   }
}

void CLoadRatingSummaryReportDlg::InitFromRptSpec()
{
   std::shared_ptr<CLoadRatingReportSpecificationBase> pRptSpec = std::dynamic_pointer_cast<CLoadRatingReportSpecificationBase>(m_pInitRptSpec);

   m_GirderKeys = pRptSpec->GetGirderKeys();
   m_bIsSingleGirderLineSelected = m_GirderKeys.size() == 1 && m_GirderKeys.front().groupIndex == ALL_GROUPS;

   if (m_bIsSingleGirderLineSelected && !m_GirderKeys.empty())
   {
      m_Girder = m_GirderKeys.front().girderIndex;
   }

   m_RadioNum = m_bIsSingleGirderLineSelected ? 0 : 1;

   m_bReportAtAllPoi = pRptSpec->ReportAtAllPointsOfInterest();
}

void CLoadRatingSummaryReportDlg::OnBnClickedSelectAll()
{
   m_pGrid->SetAllValues(true);
}

void CLoadRatingSummaryReportDlg::OnBnClickedClearAll()
{
   m_pGrid->SetAllValues(false);
}

void CLoadRatingSummaryReportDlg::OnBnClickedRadio()
{
   BOOL enab_gl = IsDlgButtonChecked(IDC_RADIO_GIRDERLINE) == BST_CHECKED ? TRUE : FALSE;
   BOOL enab_mg = enab_gl ? FALSE : TRUE;

   GetDlgItem(IDC_GIRDER)->EnableWindow(enab_gl);

   m_pGrid->Enable(enab_mg==TRUE);
   GetDlgItem(IDC_SELECT_ALL)->EnableWindow(enab_mg);
   GetDlgItem(IDC_CLEAR_ALL)->EnableWindow(enab_mg);
}

void CLoadRatingSummaryReportDlg::OnDestroy()
{
   SaveSettings();
}

void CLoadRatingSummaryReportDlg::LoadSettings()
{
   // loads last settings from the registry
   CEAFDocument* pDoc = EAFGetDocument();
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)(pDoc->GetDocTemplate());
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSBase = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);

   CEAFApp* pApp = EAFGetApp();
   CAutoRegistry autoReg(pPGSBase->GetAppName(), pApp);

   CString strSelectionType = pApp->GetProfileString(_T("Settings"), _T("LoadRatingSummaryReportLoc"), _T("Girderline"));
   if (_T("Girderline") == strSelectionType)
   {
      m_RadioNum = 0;
   }
   else
   {
      m_RadioNum = 1;
   }
}

void CLoadRatingSummaryReportDlg::SaveSettings()
{
   // save settings to registry
   CEAFDocument* pDoc = EAFGetDocument();
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)(pDoc->GetDocTemplate());
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSBase = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);

   CEAFApp* pApp = EAFGetApp();
   CAutoRegistry autoReg(pPGSBase->GetAppName(), pApp);

   CString strSelectionType = m_RadioNum == 0 ? _T("Girderline") : _T("Selection");
   pApp->WriteProfileString(_T("Settings"), _T("LoadRatingSummaryReportLoc"), strSelectionType);
}
