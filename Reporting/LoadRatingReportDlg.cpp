///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

// LoadRatingReportDlg.cpp : implementation file
//

#include "stdafx.h"
#include <Reporting\LoadRatingReportDlg.h>
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

// CLoadRatingReportDlg dialog

IMPLEMENT_DYNAMIC(CLoadRatingReportDlg, CDialog)

CLoadRatingReportDlg::CLoadRatingReportDlg(IBroker* pBroker,const CReportDescription& rptDesc,std::shared_ptr<CReportSpecification>& pRptSpec,UINT nIDTemplate,CWnd* pParent)
	: CDialog(nIDTemplate, pParent), m_RptDesc(rptDesc), m_pInitRptSpec(pRptSpec)
{
   m_GirderLine = 0;
   m_Girder = 0;
   m_Span = 0;
   m_bReportAtAllPoi = false;

   m_pBroker = pBroker;

   m_RadioNum = 0;
}

CLoadRatingReportDlg::~CLoadRatingReportDlg()
{
}

void CLoadRatingReportDlg::SetGirderKey(const CGirderKey& girderKey)
{
   m_GirderLine = girderKey.girderIndex;
   m_Girder = girderKey.girderIndex;

   if (girderKey.groupIndex == ALL_GROUPS)
   {
      m_RadioNum = 0; // girderline
      m_Span = 0;
   }
   else
   {
      m_RadioNum = 1;
      m_Span = girderKey.groupIndex;
   }

}

CGirderKey CLoadRatingReportDlg::GetGirderKey() const
{
   if (IsSingleGirderLineSelected())
   {
      return CGirderKey(ALL_GROUPS, m_GirderLine);
   }
   else
   {
      return CGirderKey(m_Span, m_Girder);
   }
}

bool CLoadRatingReportDlg::IsSingleGirderLineSelected() const
{
   return m_RadioNum == 0;
}

void CLoadRatingReportDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_ChList);

   DDX_CBIndex(pDX, IDC_GIRDERLINE, (int&)m_GirderLine);
   DDX_CBIndex(pDX, IDC_GIRDER, (int&)m_Girder);
   DDX_CBIndex(pDX, IDC_SPAN, (int&)m_Span);
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
   }
}

BEGIN_MESSAGE_MAP(CLoadRatingReportDlg, CDialog)
	ON_COMMAND(IDHELP, OnHelp)
   ON_BN_CLICKED(IDC_RADIO_GIRDERLINE, &CLoadRatingReportDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_RADIO_INDIV_GIRDER, &CLoadRatingReportDlg::OnBnClickedRadio)
   ON_CBN_SELCHANGE(IDC_SPAN, &CLoadRatingReportDlg::OnCbnSelchangeSpan)
   ON_WM_DESTROY()
END_MESSAGE_MAP()

// CLoadRatingReportDlg message handlers
void CLoadRatingReportDlg::UpdateGirderLineComboBox()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComboBox* pGdrBox = (CComboBox*)GetDlgItem(IDC_GIRDERLINE);

   Uint16 curSel = pGdrBox->GetCurSel();
   pGdrBox->ResetContent();

   GET_IFACE( IBridge, pBridge );
   GirderIndexType cGirders = 0;
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType i = 0; i < nGroups; i++ )
   {
      cGirders = Max(cGirders,pBridge->GetGirderCount(i));
   }

   for ( GirderIndexType j = 0; j < cGirders; j++ )
   {
      CString strGdr;
      strGdr.Format( _T("Girderline %s"), LABEL_GIRDER(j));
      pGdrBox->AddString( strGdr );
   }

   if ( pGdrBox->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR )
   {
      pGdrBox->SetCurSel(0);
   }
}

void CLoadRatingReportDlg::UpdateSpanComboBox()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_SPAN);

   Uint16 curSel = pBox->GetCurSel();
   pBox->ResetContent();

   GET_IFACE( IBridge, pBridge );
   GirderIndexType cGirders = 0;
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType i = 0; i < nGroups; i++ )
   {
      CString strSpan = pgsGirderLabel::GetGroupLabel(i).c_str();
      pBox->AddString( strSpan );
   }

   if ( pBox->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR )
   {
      pBox->SetCurSel(0);
   }
}

void CLoadRatingReportDlg::UpdateGirderComboBox()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComboBox* pSpanBox = (CComboBox*)GetDlgItem(IDC_SPAN);
   CComboBox* pGdrBox = (CComboBox*)GetDlgItem(IDC_GIRDER);

   Uint16 curSpanSel = pSpanBox->GetCurSel();
   if (curSpanSel == CB_ERR)
   {
      ATLASSERT(0); // span should always be selected before calling us
      curSpanSel = 0;
   }

   GET_IFACE( IBridge, pBridge );
   GirderIndexType nGirders = pBridge->GetGirderCount( GroupIndexType(curSpanSel) );

   Uint16 curGdrSel = pGdrBox->GetCurSel();
   pGdrBox->ResetContent();

   for ( GirderIndexType ig = 0; ig < nGirders; ig++ )
   {
      CString strGdr;
      strGdr.Format( _T("Girder %s"), LABEL_GIRDER(ig));
      pGdrBox->AddString( strGdr );
   }

   if ( pGdrBox->SetCurSel(curGdrSel == CB_ERR ? 0 : curGdrSel) == CB_ERR )
   {
      pGdrBox->SetCurSel(0);
   }
}

BOOL CLoadRatingReportDlg::OnInitDialog()
{
   CWnd* pwndTitle = GetDlgItem(IDC_REPORT_TITLE);
   pwndTitle->SetWindowText(m_RptDesc.GetReportName());

   UpdateSpanComboBox();
   UpdateGirderComboBox();
   UpdateGirderLineComboBox();

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

   OnBnClickedRadio();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CLoadRatingReportDlg::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_LOADRATING_REPORT );
}

void CLoadRatingReportDlg::UpdateChapterList()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Clear out the list box
   m_ChList.ResetContent();

   // Get the chapters in the report
   std::vector<CChapterInfo> chInfos = m_RptDesc.GetChapterInfo();

   // Populate the list box with the names of the chapters
   std::vector<CChapterInfo>::iterator iter;
   for ( iter = chInfos.begin(); iter != chInfos.end(); iter++ )
   {
      CChapterInfo chInfo = *iter;

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

void CLoadRatingReportDlg::ClearChapterCheckMarks()
{
   int cChapters = m_ChList.GetCount();
   for ( int ch = 0; ch < cChapters; ch++ )
   {
      m_ChList.SetCheck(ch,0);
   }
}

void CLoadRatingReportDlg::InitChapterListFromSpec()
{
   ClearChapterCheckMarks();
   std::vector<CChapterInfo> chInfo = m_pInitRptSpec->GetChapterInfo();
   std::vector<CChapterInfo>::iterator iter;
   for ( iter = chInfo.begin(); iter != chInfo.end(); iter++ )
   {
      CChapterInfo& ch = *iter;
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

void CLoadRatingReportDlg::InitFromRptSpec()
{
   std::shared_ptr<CLoadRatingReportSpecificationBase> pRptSpec = std::dynamic_pointer_cast<CLoadRatingReportSpecificationBase>(m_pInitRptSpec);

   std::vector<CGirderKey> girderKeys = pRptSpec->GetGirderKeys();
   bool bIsSingleGirderLineSelected = girderKeys.size() == 1 && girderKeys.front().groupIndex == ALL_GROUPS;

   if (bIsSingleGirderLineSelected)
   {
      m_Girder = girderKeys.front().girderIndex;
      m_GirderLine = girderKeys.front().girderIndex;
      m_Span = 0;
      m_RadioNum = 0;
   }
   else
   {
      if (!girderKeys.empty())
      {
         m_Girder = girderKeys.front().girderIndex;
         m_Span = girderKeys.front().groupIndex;
         m_RadioNum = 1; // only option where a single girder is selected
      }
      else
      {
         m_Girder = 0;
         m_Span = 0;
         m_RadioNum = 0;
      }
   }

   m_bReportAtAllPoi = pRptSpec->ReportAtAllPointsOfInterest();
}

void CLoadRatingReportDlg::OnBnClickedRadio()
{
   BOOL enab_gl = IsDlgButtonChecked(IDC_RADIO_GIRDERLINE) == BST_CHECKED ? TRUE : FALSE;
   BOOL enab_mg = enab_gl ? FALSE : TRUE;

   GetDlgItem(IDC_GIRDERLINE)->EnableWindow(enab_gl);
   GetDlgItem(IDC_GIRDER)->EnableWindow(enab_mg);
   GetDlgItem(IDC_SPAN)->EnableWindow(enab_mg);
}

void CLoadRatingReportDlg::OnCbnSelchangeSpan()
{
   UpdateGirderComboBox();
}

void CLoadRatingReportDlg::OnDestroy()
{
   SaveSettings();
}

void CLoadRatingReportDlg::LoadSettings()
{
   // loads last settings from the registry
   CEAFDocument* pDoc = EAFGetDocument();
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)(pDoc->GetDocTemplate());
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSBase = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);

   CEAFApp* pApp = EAFGetApp();
   CAutoRegistry autoReg(pPGSBase->GetAppName(), pApp);

   CString strSelectionType = pApp->GetProfileString(_T("Settings"), _T("LoadRatingReportLoc"), _T("Girderline"));
   if (_T("Girderline") == strSelectionType)
   {
      m_RadioNum = 0;
   }
   else
   {
      m_RadioNum = 1;
   }
}

void CLoadRatingReportDlg::SaveSettings()
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
   pApp->WriteProfileString(_T("Settings"), _T("LoadRatingReportLoc"), strSelectionType);
}
