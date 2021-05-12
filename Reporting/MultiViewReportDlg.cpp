///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
//
// MultiViewReportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Reporting.h"
#include "MultiViewReportDlg.h"
#include "..\Documentation\PGSuper.hh"

#include <initguid.h>
#include <IFace\Tools.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>

#include <PgsExt\GirderLabel.h>

#include "RMultiGirderSelectDlg.h"
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CMultiViewReportDlg dialog

IMPLEMENT_DYNAMIC(CMultiViewReportDlg, CDialog)

CMultiViewReportDlg::CMultiViewReportDlg(IBroker* pBroker,const CReportDescription& rptDesc, std::shared_ptr<CReportSpecification>& pRptSpec,
                                         const CGirderKey& girderKey,
                                         UINT nIDTemplate,CWnd* pParent)
	: CDialog(nIDTemplate, pParent), m_RptDesc(rptDesc), m_pInitRptSpec(pRptSpec)
{
   m_GirderKey = girderKey;
   m_pBroker = pBroker;
}

CMultiViewReportDlg::~CMultiViewReportDlg()
{
}

void CMultiViewReportDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST, m_ChList);

   DDX_CBIndex(pDX, IDC_SPAN, (int&)m_GirderKey.groupIndex);
   DDX_CBIndex(pDX, IDC_GIRDER, (int&)m_GirderKey.girderIndex);

   if ( pDX->m_bSaveAndValidate )
   {
      // Girder list
      // Make list of one if single girder is checked
      BOOL enab_sgl = IsDlgButtonChecked(IDC_RADIO1) == BST_CHECKED ? TRUE : FALSE;
      if (enab_sgl)
      {
         m_GirderKeys.clear();
         m_GirderKeys.push_back(m_GirderKey);
      }
      else if (m_GirderKeys.empty())
      {
         AfxMessageBox(_T("You must select at least one girder"));
         pDX->Fail();
      }

      // Chapters
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

BEGIN_MESSAGE_MAP(CMultiViewReportDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_SPAN, &CMultiViewReportDlg::OnCbnSelchangeSpan)
   ON_BN_CLICKED(IDHELP, &CMultiViewReportDlg::OnHelp)
   ON_BN_CLICKED(IDC_RADIO1, &CMultiViewReportDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_RADIO2, &CMultiViewReportDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_SELECT_MULTIPLE_BUTTON, &CMultiViewReportDlg::OnBnClickedSelectMultipleButton)
END_MESSAGE_MAP()

BOOL CMultiViewReportDlg::OnInitDialog()
{
   CWnd* pwndTitle = GetDlgItem(IDC_REPORT_TITLE);
   pwndTitle->SetWindowText(m_RptDesc.GetReportName());

   GET_IFACE(IBridge, pBridge);
   bool bMultiSelect = false;
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   if (m_GirderKey.groupIndex == ALL_GROUPS || m_GirderKey.girderIndex == ALL_GIRDERS)
   {
      // if all groups or all girders, fill the girder keys
      bMultiSelect = true;
      GroupIndexType firstGroupIdx = (m_GirderKey.groupIndex == ALL_GROUPS ? 0 : m_GirderKey.groupIndex);
      GroupIndexType lastGroupIdx = (m_GirderKey.groupIndex == ALL_GROUPS ? nGroups - 1 : firstGroupIdx);
      for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++)
      {
         GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
         GirderIndexType firstGirderIdx = (m_GirderKey.girderIndex == ALL_GIRDERS ? 0 : m_GirderKey.girderIndex);
         GirderIndexType lastGirderIdx = (m_GirderKey.girderIndex == ALL_GIRDERS ? nGirders - 1 : firstGirderIdx);
         for (GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++)
         {
            m_GirderKeys.emplace_back(grpIdx,gdrIdx);
         }
      }
      m_GirderKey.groupIndex = firstGroupIdx;
      m_GirderKey.girderIndex = (m_GirderKey.girderIndex == ALL_GIRDERS ? 0 : m_GirderKey.girderIndex);
   }

   CheckDlgButton(bMultiSelect ? IDC_RADIO2 : IDC_RADIO1,BST_CHECKED);
   OnBnClickedRadio();

   // Fill up the span and girder combo boxes

   GET_IFACE(IDocumentType,pDocType);
   bool bIsPGSuper = pDocType->IsPGSuperDocument();

   // fill up the span box
   CComboBox* pSpanBox = (CComboBox*)GetDlgItem( IDC_SPAN );
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CString strLabel;
      if (bIsPGSuper)
      {
         strLabel.Format(_T("Span %s"), LABEL_SPAN(grpIdx));
      }
      else
      {
         strLabel.Format(_T("Group %d"), LABEL_GROUP(grpIdx));
      }

      pSpanBox->AddString(strLabel);
   }
   pSpanBox->SetCurSel((int)m_GirderKey.groupIndex);

   UpdateGirderComboBox(m_GirderKey.groupIndex);

   CDialog::OnInitDialog();

   UpdateChapterList();

   if ( m_pInitRptSpec )
      InitFromRptSpec();

   UpdateButtonText();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}


void CMultiViewReportDlg::UpdateGirderComboBox(SpanIndexType spanIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE( IBridge, pBridge );

   CComboBox* pGdrBox = (CComboBox*)GetDlgItem(IDC_GIRDER);
   Uint16 curSel = pGdrBox->GetCurSel();
   pGdrBox->ResetContent();

   GirderIndexType cGirders = pBridge->GetGirderCount( spanIdx );

   for ( GirderIndexType j = 0; j < cGirders; j++ )
   {
      CString strGdr;
      strGdr.Format( _T("Girder %s"), LABEL_GIRDER(j));
      pGdrBox->AddString( strGdr );
   }

   if ( pGdrBox->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR )
      pGdrBox->SetCurSel(0);
}

void CMultiViewReportDlg::UpdateChapterList()
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

void CMultiViewReportDlg::OnCbnSelchangeSpan()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SPAN);
   SpanIndexType spanIdx = SpanIndexType(pCB->GetCurSel());

   UpdateGirderComboBox(spanIdx);
}

void CMultiViewReportDlg::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DIALOG_REPORT );
}


void CMultiViewReportDlg::ClearChapterCheckMarks()
{
   int cChapters = m_ChList.GetCount();
   for ( int ch = 0; ch < cChapters; ch++ )
   {
      m_ChList.SetCheck(ch,0);
   }
}

void CMultiViewReportDlg::InitChapterListFromSpec()
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

void CMultiViewReportDlg::InitFromRptSpec()
{
   std::shared_ptr<CMultiViewSpanGirderReportSpecification> pRptSpec = std::dynamic_pointer_cast<CMultiViewSpanGirderReportSpecification>(m_pInitRptSpec);
   if (pRptSpec)
   {
      m_GirderKeys.clear();
      m_GirderKeys = pRptSpec->GetGirderKeys();
      m_GirderKey = m_GirderKeys.front();
   }
   else
   {
      ATLASSERT(false);
      return;
   }

   UpdateData(FALSE);

   InitChapterListFromSpec();
}

void CMultiViewReportDlg::OnBnClickedRadio()
{
   BOOL enab_sgl = IsDlgButtonChecked(IDC_RADIO1) == BST_CHECKED ? TRUE : FALSE;
   BOOL enab_mpl = enab_sgl ? FALSE : TRUE;

   GetDlgItem(IDC_SPAN)->EnableWindow(enab_sgl);
   GetDlgItem(IDC_GIRDER)->EnableWindow(enab_sgl);

   GetDlgItem(IDC_SELECT_MULTIPLE_BUTTON)->EnableWindow(enab_mpl);

   if ( enab_mpl && m_GirderKeys.size() == 0 )
   {
      OnBnClickedSelectMultipleButton();
   }
}

void CMultiViewReportDlg::OnBnClickedSelectMultipleButton()
{

   CRMultiGirderSelectDlg dlg;
   dlg.m_SelGdrs = m_GirderKeys;

   if (dlg.DoModal()==IDOK)
   {
      m_GirderKeys = dlg.m_SelGdrs;

      UpdateButtonText();
   }
}

std::vector<CGirderKey> CMultiViewReportDlg::GetGirderKeys() const
{
   return m_GirderKeys;
}

void CMultiViewReportDlg::UpdateButtonText()
{
   CString msg;
   msg.Format(_T("Select Girders\n(%d Selected)"), m_GirderKeys.size());
   GetDlgItem(IDC_SELECT_MULTIPLE_BUTTON)->SetWindowText(msg);
}
