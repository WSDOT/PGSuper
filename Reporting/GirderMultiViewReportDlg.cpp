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
//
// MultiViewReportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GirderMultiViewReportDlg.h"



// CGirderMultiViewReportDlg dialog

IMPLEMENT_DYNAMIC(CGirderMultiViewReportDlg, CMultiViewReportDlg)

CGirderMultiViewReportDlg::CGirderMultiViewReportDlg(const CGirderKey& girderKey,
    std::shared_ptr<WBFL::EAF::Broker> pBroker,const WBFL::Reporting::ReportDescription& rptDesc,
    std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec)
    :CMultiViewReportDlg(pBroker, rptDesc, pRptSpec)
{
   m_GirderKey = girderKey;
}

CGirderMultiViewReportDlg::~CGirderMultiViewReportDlg()
{
}


BOOL CGirderMultiViewReportDlg::OnInitDialog()
{
    CMultiViewReportDlg::OnInitDialog();

    if (m_pInitRptSpec)
        InitFromRptSpec();

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
                m_GirderKeys.emplace_back(grpIdx, gdrIdx);
            }
        }
        m_GirderKey.groupIndex = firstGroupIdx;
        m_GirderKey.girderIndex = (m_GirderKey.girderIndex == ALL_GIRDERS ? 0 : m_GirderKey.girderIndex);
    }

    CheckDlgButton(bMultiSelect ? IDC_RADIO2 : IDC_RADIO1, BST_CHECKED);

    OnBnClickedRadio();

    CComboBox* pGroupBox = (CComboBox*)GetDlgItem(IDC_GROUP_BEARING_SELECT);
    pGroupBox->ShowWindow(SW_HIDE);
    CComboBox* pBearingSelect = (CComboBox*)GetDlgItem(IDC_FACE);
    pBearingSelect->ShowWindow(SW_HIDE);

    CComboBox* pRadio1 = (CComboBox*)GetDlgItem(IDC_RADIO1);
    pRadio1->SetWindowText(_T("For a Single Girder"));

    CComboBox* pRadio2 = (CComboBox*)GetDlgItem(IDC_RADIO2);
    pRadio2->SetWindowText(_T("For Multiple Girders (Opens Multiple Windows)"));

    CComboBox* pButton = (CComboBox*)GetDlgItem(IDC_SELECT_MULTIPLE_BUTTON);
    pButton->SetWindowText(_T("Select Girders\n(0 Selected)"));

    CString msg;
    msg.Format(_T("Select Girders\n(%d Selected)"), m_GirderKeys.size());
    GetDlgItem(IDC_SELECT_MULTIPLE_BUTTON)->SetWindowText(msg);



    return TRUE;
}

void CGirderMultiViewReportDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CMultiViewReportDlg::DoDataExchange(pDX);

   if (pDX->m_bSaveAndValidate)
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

   }

}

void CGirderMultiViewReportDlg::InitFromRptSpec()
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

std::vector<CGirderKey> CGirderMultiViewReportDlg::GetGirderKeys() const
{
    return m_GirderKeys;
}

BEGIN_MESSAGE_MAP(CGirderMultiViewReportDlg, CMultiViewReportDlg)
   ON_BN_CLICKED(IDC_RADIO1, &CGirderMultiViewReportDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_RADIO2, &CGirderMultiViewReportDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_SELECT_MULTIPLE_BUTTON, &CGirderMultiViewReportDlg::OnBnClickedSelectMultipleButton)
END_MESSAGE_MAP()


void CGirderMultiViewReportDlg::OnBnClickedRadio()
{
    BOOL enab_sgl = IsDlgButtonChecked(IDC_RADIO1) == BST_CHECKED ? TRUE : FALSE;
    BOOL enab_mpl = enab_sgl ? FALSE : TRUE;

    GetDlgItem(IDC_SPAN)->EnableWindow(enab_sgl);
    GetDlgItem(IDC_GIRDER)->EnableWindow(enab_sgl);
    GetDlgItem(IDC_SELECT_MULTIPLE_BUTTON)->EnableWindow(enab_mpl);

    if (enab_mpl && m_GirderKeys.size() == 0)
    {
        OnBnClickedSelectMultipleButton();
    }
}


void CGirderMultiViewReportDlg::OnBnClickedSelectMultipleButton()
{
   CRMultiGirderSelectDlg dlg;
   dlg.m_SelGdrs = m_GirderKeys;

   if (dlg.DoModal()==IDOK)
   {
      m_GirderKeys = dlg.m_SelGdrs;

      CString msg;
      msg.Format(_T("Select Girders\n(%d Selected)"), m_GirderKeys.size());
      GetDlgItem(IDC_SELECT_MULTIPLE_BUTTON)->SetWindowText(msg);
   }
}



