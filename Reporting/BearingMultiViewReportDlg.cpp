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
#include "Reporting.h"
#include "BearingMultiViewReportDlg.h"
#include "..\Documentation\PGSuper.hh"

#include <initguid.h>
#include <IFace\Tools.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>


#include <PsgLib\BridgeDescription2.h>


#include "RMultiBearingSelectDlg.h"
#include <EAF\EAFDocument.h>
#include <Reporting/ReactionInterfaceAdapters.h>
#include <Reporting\SpanGirderBearingReportSpecification.h>
#include <MfcTools\CustomDDX.h>


// CMultiViewReportDlg dialog

IMPLEMENT_DYNAMIC(CBearingMultiViewReportDlg, CMultiViewReportDlg)

CBearingMultiViewReportDlg::CBearingMultiViewReportDlg(std::shared_ptr<WBFL::EAF::Broker> pBroker,const WBFL::Reporting::ReportDescription& rptDesc,
    std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec,
                                         const CGirderKey& girderKey, const ReactionLocation& reactionLocation)
	: 
    CMultiViewReportDlg(pBroker, rptDesc, pRptSpec)
{
    m_GirderKey = girderKey;
    m_ReactionLocation = reactionLocation;
}

CBearingMultiViewReportDlg::~CBearingMultiViewReportDlg()
{
}

void CBearingMultiViewReportDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CMultiViewReportDlg::DoDataExchange(pDX);

    GET_IFACE(IBearingDesign, pBearingDesign);
    GET_IFACE(IIntervals, pIntervals);
    GET_IFACE(IBridge, pBridge);
    GET_IFACE(IBridgeDescription, pIBridgeDesc);

    const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

    IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();

    std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, lastCompositeDeckIntervalIdx, m_GirderKey));

    ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);
    iter.First();
    PierIndexType startPierIdx = (iter.IsDone() ? INVALID_INDEX : iter.CurrentItem().PierIdx);

    CComboBox* pBrgBox = (CComboBox*)GetDlgItem(IDC_FACE);

    m_RLmenuItems.clear();

    // Use iterator to walk locations
    for (iter.First(); !iter.IsDone(); iter.Next())
    {

        const ReactionLocation& reactionLocation(iter.CurrentItem());

        m_RLmenuItems.emplace_back(reactionLocation);


        const CPierData2* pPier = pBridgeDesc->GetPier(reactionLocation.PierIdx);
        CString bcType;

        if (pPier->IsBoundaryPier())
        {
            bool bNoDeck = IsNonstructuralDeck(pBridgeDesc->GetDeckDescription()->GetDeckType());
            bcType = CPierData2::AsString(pPier->GetBoundaryConditionType(), bNoDeck);
        }
        else
        {
            bcType = CPierData2::AsString(pPier->GetSegmentConnectionType());
        }



        CString strBrg = reactionLocation.PierLabel.c_str();

        if (bcType == _T("Hinge"))
        {
            CString hingeBrgStr;
            hingeBrgStr.Format(_T("%s (Xc)"), strBrg);
            pBrgBox->AddString(hingeBrgStr);
        }
        else
        {
            pBrgBox->AddString(strBrg);
        }

    }

    if (pDX->m_bSaveAndValidate)
    {
        IndexType idx;
        DDX_CBIndex(pDX, IDC_FACE, idx);

        m_ReactionLocation = m_RLmenuItems[idx];

        // Girder list
        // Make list of one if single girder is checked
        BOOL enab_sgl = IsDlgButtonChecked(IDC_RADIO1) == BST_CHECKED ? TRUE : FALSE;
        if (enab_sgl)
        {
            m_ReactionLocations.clear();
            m_ReactionLocations.push_back(m_ReactionLocation);
        }
        else if (m_ReactionLocations.empty())
        {
            AfxMessageBox(_T("You must select at least one bearing"));
            pDX->Fail();
        }

    }
}

void CBearingMultiViewReportDlg::InitFromRptSpec()
{
    std::shared_ptr<CMultiViewSpanGirderBearingReportSpecification> pRptSpec = std::dynamic_pointer_cast<CMultiViewSpanGirderBearingReportSpecification>(m_pInitRptSpec);
    if (pRptSpec)
    {
        m_ReactionLocations.clear();
        m_ReactionLocations = pRptSpec->GetReactionLocations();
        m_ReactionLocation = m_ReactionLocations.front();
    }
    else
    {
        ATLASSERT(false);
        return;
    }

    UpdateData(FALSE);

    InitChapterListFromSpec();
}

BOOL CBearingMultiViewReportDlg::OnInitDialog()
{

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   
   CMultiViewReportDlg::OnInitDialog();

   if (m_pInitRptSpec)
       InitFromRptSpec();

   auto bMultiSelect = false;
   CheckDlgButton(bMultiSelect ? IDC_RADIO2 : IDC_RADIO1, BST_CHECKED);

   CComboBox* pGroupBox = (CComboBox*)GetDlgItem(IDC_GROUP_GIRDER_SELECT);
   pGroupBox->ShowWindow(SW_HIDE);

   CComboBox* pRadio1 = (CComboBox*)GetDlgItem(IDC_RADIO1);
   pRadio1->SetWindowText(_T("For a Single Bearing"));

   CComboBox* pRadio2 = (CComboBox*)GetDlgItem(IDC_RADIO2);
   pRadio2->SetWindowText(_T("For Multiple Bearings (Opens Multiple Windows)"));

   CComboBox* pButton = (CComboBox*)GetDlgItem(IDC_SELECT_MULTIPLE_BUTTON);
   pButton->SetWindowText(_T("Select Bearings\n(0 Selected)"));
   pButton->EnableWindow(bMultiSelect);

   CComboBox* pBrgBox = (CComboBox*)GetDlgItem(IDC_FACE);

   pBrgBox->SetCurSel(0);

   CRMultiBearingSelectDlg dlg;

   CString msg;
   msg.Format(_T("Select Bearings\n(%d Selected)"), dlg.m_SelRLs.size());
   GetDlgItem(IDC_SELECT_MULTIPLE_BUTTON)->SetWindowText(msg);


   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}



// update bearing combo box?

std::vector<ReactionLocation> CBearingMultiViewReportDlg::GetReactionLocations() const
{
    return m_ReactionLocations;
}


BEGIN_MESSAGE_MAP(CBearingMultiViewReportDlg, CMultiViewReportDlg)
    ON_CBN_SELCHANGE(IDC_SPAN, &CBearingMultiViewReportDlg::UpdateSpanComboBox)
    ON_CBN_SELCHANGE(IDC_GIRDER, &CBearingMultiViewReportDlg::UpdateGirderComboBox)
    ON_BN_CLICKED(IDC_RADIO1, &CBearingMultiViewReportDlg::OnBnClickedRadio)
    ON_BN_CLICKED(IDC_RADIO2, &CBearingMultiViewReportDlg::OnBnClickedRadio)
    ON_BN_CLICKED(IDC_SELECT_MULTIPLE_BUTTON, &CBearingMultiViewReportDlg::OnBnClickedSelectMultipleButton)
END_MESSAGE_MAP()
//

void CBearingMultiViewReportDlg::OnBnClickedRadio()
{
   BOOL enab_sgl = IsDlgButtonChecked(IDC_RADIO1) == BST_CHECKED ? TRUE : FALSE;
   BOOL enab_mpl = enab_sgl ? FALSE : TRUE;

   GetDlgItem(IDC_SPAN)->EnableWindow(enab_sgl);
   GetDlgItem(IDC_GIRDER)->EnableWindow(enab_sgl);
   GetDlgItem(IDC_FACE)->EnableWindow(enab_sgl);



   GetDlgItem(IDC_SELECT_MULTIPLE_BUTTON)->EnableWindow(enab_mpl);

   if ( enab_mpl && m_ReactionLocations.size() == 0 )
   {
      OnBnClickedSelectMultipleButton();
   }
}

void CBearingMultiViewReportDlg::UpdateSpanComboBox()
{
    CMultiViewReportDlg::OnCbnSelchangeSpan();

    UpdateBearingComboBox();
}

void CBearingMultiViewReportDlg::UpdateGirderComboBox()
{
    CMultiViewReportDlg::UpdateGirderComboBox(m_ReactionLocation.GirderKey.groupIndex);

    UpdateBearingComboBox();
}



void CBearingMultiViewReportDlg::UpdateBearingComboBox()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_SPAN);
    GroupIndexType grpIdx = (GroupIndexType)pcbGroup->GetCurSel();

    CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
    GirderIndexType gdrIdx = (GirderIndexType)pcbGirder->GetCurSel();

    GET_IFACE(IBearingDesign, pBearingDesign);
    GET_IFACE(IIntervals, pIntervals);
    GET_IFACE(IBridge, pBridge);
    GET_IFACE(IBridgeDescription, pIBridgeDesc);

    const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

    IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();

    std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, 
        lastCompositeDeckIntervalIdx, CGirderKey(grpIdx, gdrIdx)));

    ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);
    iter.First();
    PierIndexType startPierIdx = (iter.IsDone() ? INVALID_INDEX : iter.CurrentItem().PierIdx);

    CComboBox* pBrgBox = (CComboBox*)GetDlgItem(IDC_FACE);
    Uint16 curSel = pBrgBox->GetCurSel();
    pBrgBox->ResetContent();

    m_RLmenuItems.clear();

    // Use iterator to walk locations
    for (iter.First(); !iter.IsDone(); iter.Next())
    {

        const ReactionLocation& reactionLocation(iter.CurrentItem());

        m_RLmenuItems.emplace_back(reactionLocation);


        const CPierData2* pPier = pBridgeDesc->GetPier(reactionLocation.PierIdx);
        CString bcType;

        if (pPier->IsBoundaryPier())
        {
            bool bNoDeck = IsNonstructuralDeck(pBridgeDesc->GetDeckDescription()->GetDeckType());
            bcType = CPierData2::AsString(pPier->GetBoundaryConditionType(), bNoDeck);
        }
        else
        {
            bcType = CPierData2::AsString(pPier->GetSegmentConnectionType());
        }


        CString strBrg = reactionLocation.PierLabel.c_str();

        if (bcType == _T("Hinge"))
        {
            CString hingeBrgStr;
            hingeBrgStr.Format(_T("%s (Xc)"), strBrg);
            pBrgBox->AddString(hingeBrgStr);
        }
        else
        {
            pBrgBox->AddString(strBrg);
        }

    }

    if (pBrgBox->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR)
    {
        pBrgBox->SetCurSel(0);
    }

}



void CBearingMultiViewReportDlg::OnBnClickedSelectMultipleButton()
{

   CRMultiBearingSelectDlg dlg;
   dlg.m_SelRLs = m_ReactionLocations;

   if (dlg.DoModal()==IDOK)
   {
       m_ReactionLocations = dlg.m_SelRLs;

       CString msg;
       msg.Format(_T("Select Bearings\n(%d Selected)"), m_ReactionLocations.size());
       GetDlgItem(IDC_SELECT_MULTIPLE_BUTTON)->SetWindowText(msg);
   }
}
