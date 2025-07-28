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
///////////////////////////////////////////////////////////////////////

// SpanGirderReportDlg.cpp : implementation file
//

#include "stdafx.h"
#include <Reporting\SpanGirderBearingReportDlg.h>

#include <IFace\Tools.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Project.h>
#include <IFace\DocumentType.h>

#include <PsgLib\GirderLabel.h>
#include <EAF\EAFDocument.h>
#include "..\Documentation\PGSuper.hh"
#include <Reporting/ReactionInterfaceAdapters.h>
#include <PsgLib\BridgeDescription2.h>
#include <Reporting/SpanGirderBearingReportSpecification.h>


 // CSpanBearingReportDlg dialog

 IMPLEMENT_DYNAMIC(CSpanGirderBearingReportDlg, CSpanItemReportDlg)

  CSpanGirderBearingReportDlg::CSpanGirderBearingReportDlg(std::shared_ptr<WBFL::EAF::Broker> pBroker, const WBFL::Reporting::ReportDescription& rptDesc, Mode mode, std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec, UINT nIDTemplate, CWnd* pParent)
    :CSpanItemReportDlg(pBroker, rptDesc, mode, pRptSpec, nIDTemplate, pParent)
 {

 }

 CSpanGirderBearingReportDlg::~CSpanGirderBearingReportDlg()
 {
 }


 void CSpanGirderBearingReportDlg::DoDataExchange(CDataExchange* pDX)
 {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    DDX_CBIndex(pDX, IDC_SPAN, (int&)m_SegmentKey.groupIndex);

    DDX_CBIndex(pDX, IDC_GIRDER, (int&)m_SegmentKey.girderIndex);

    DDX_CBIndex(pDX, IDC_SPAN_ITEM, (int&)m_Bearing.PierIdx);

    //GET_IFACE(IBearingDesign, pBearingDesign);
    //GET_IFACE(IIntervals, pIntervals);
    //GET_IFACE(IBridge, pBridge);
    //GET_IFACE(IBridgeDescription, pIBridgeDesc);

    //const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

    //IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();

    //std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, lastCompositeDeckIntervalIdx, CGirderKey(m_SegmentKey.groupIndex, m_SegmentKey.girderIndex)));

    //ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);
    //iter.First();
    //PierIndexType startPierIdx = (iter.IsDone() ? INVALID_INDEX : iter.CurrentItem().PierIdx);

    //CComboBox* pBrgBox = (CComboBox*)GetDlgItem(IDC_SPAN_ITEM);

    //m_RLmenuItems.clear();

    //// Use iterator to walk locations
    //for (iter.First(); !iter.IsDone(); iter.Next())
    //{

    //    const ReactionLocation& reactionLocation(iter.CurrentItem());

    //    m_RLmenuItems.emplace_back(reactionLocation);


    //    const CPierData2* pPier = pBridgeDesc->GetPier(reactionLocation.PierIdx);
    //    CString bcType;

    //    if (pPier->IsBoundaryPier())
    //    {
    //        bool bNoDeck = IsNonstructuralDeck(pBridgeDesc->GetDeckDescription()->GetDeckType());
    //        bcType = CPierData2::AsString(pPier->GetBoundaryConditionType(), bNoDeck);
    //    }
    //    else
    //    {
    //        bcType = CPierData2::AsString(pPier->GetSegmentConnectionType());
    //    }



    //    CString strBrg = reactionLocation.PierLabel.c_str();

    //    if (bcType == _T("Hinge"))
    //    {
    //        CString hingeBrgStr;
    //        hingeBrgStr.Format(_T("%s (Xc)"), strBrg);
    //        pBrgBox->AddString(hingeBrgStr);
    //    }
    //    else
    //    {
    //        pBrgBox->AddString(strBrg);
    //    }

    //}

    //if (pDX->m_bSaveAndValidate)
    //{
    //    IndexType idx;
    //    DDX_CBIndex(pDX, IDC_SPAN_ITEM, (int&)idx); // why is this invalid??

    //    m_Bearing = m_RLmenuItems[idx];

    //    // Girder list
    //    // Make list of one if single girder is checked
    //    BOOL enab_sgl = IsDlgButtonChecked(IDC_RADIO1) == BST_CHECKED ? TRUE : FALSE;
    //    if (enab_sgl)
    //    {
    //        m_Bearings.clear();
    //        m_Bearings.push_back(m_Bearing);
    //    }
    //    else if (m_Bearings.empty())
    //    {
    //        AfxMessageBox(_T("You must select at least one bearing"));
    //        pDX->Fail();
    //    }

    //}

    CSpanItemReportDlg::DoDataExchange(pDX);

 }

 BEGIN_MESSAGE_MAP(CSpanGirderBearingReportDlg, CSpanItemReportDlg)
     ON_CBN_SELCHANGE(IDC_SPAN, OnGroupChanged)
     ON_CBN_SELCHANGE(IDC_GIRDER, OnGirderChanged)
 END_MESSAGE_MAP()


 // CSpanGirderBearingReportDlg message handlers
 void CSpanGirderBearingReportDlg::UpdateGirderComboBox()
 {

     CSpanItemReportDlg::UpdateGirderComboBox();

     CComboBox* pGdrBox = (CComboBox*)GetDlgItem(IDC_GIRDER);
     Uint16 curSel = pGdrBox->GetCurSel();
     pGdrBox->ResetContent();

     GET_IFACE(IBridge, pBridge);

     GirderIndexType cGirders = 0;
     if (m_SegmentKey.groupIndex == ALL_GROUPS)
     {
         GroupIndexType nGroups = pBridge->GetGirderGroupCount();
         for (GroupIndexType i = 0; i < nGroups; i++)
         {
             cGirders = Max(cGirders, pBridge->GetGirderCount(i));
         }
     }
     else
     {
         cGirders = pBridge->GetGirderCount(m_SegmentKey.groupIndex);
     }

     for (GirderIndexType j = 0; j < cGirders; j++)
     {
         CString strGdr;
         strGdr.Format(_T("Girder %s"), LABEL_GIRDER(j));
         pGdrBox->AddString(strGdr);
     }

     if (pGdrBox->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR)
     {
         pGdrBox->SetCurSel(0);
     }

     UpdateBearingComboBox();
 }

 void CSpanGirderBearingReportDlg::UpdateBearingComboBox()
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

     std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, lastCompositeDeckIntervalIdx, CGirderKey(m_SegmentKey.groupIndex, m_SegmentKey.girderIndex)));

     ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);
     iter.First();
     PierIndexType startPierIdx = (iter.IsDone() ? INVALID_INDEX : iter.CurrentItem().PierIdx);

     CComboBox* pBrgBox = (CComboBox*)GetDlgItem(IDC_SPAN_ITEM);
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

 BOOL CSpanGirderBearingReportDlg::OnInitDialog()
 {
     CWnd* pwndTitle = GetDlgItem(IDC_REPORT_TITLE);
     pwndTitle->SetWindowText(m_RptDesc.GetReportName().c_str());

     GET_IFACE(IDocumentType, pDocType);
     bool bIsPGSuper = pDocType->IsPGSuperDocument();

     // It is possible that a span was removed from the bridge, and if so, the report description for this report 
     // will have a higher group that that exists. Nip in the bud here so we don't crash
     GET_IFACE(IBridge, pBridge);
     GroupIndexType numGroups = pBridge->GetGirderGroupCount();
     if (m_SegmentKey.groupIndex != ALL_GROUPS && m_SegmentKey.groupIndex > numGroups - 1)
     {
         m_SegmentKey.groupIndex = numGroups - 1;
     }

     // Fill up the span and girder combo boxes
     if (m_Mode == Mode::GroupGirderBearingAndChapters)
     {

        if (m_SegmentKey.groupIndex == ALL_GROUPS) m_SegmentKey.groupIndex = 0;
        if (m_SegmentKey.girderIndex == ALL_GIRDERS) m_SegmentKey.girderIndex = 0;
        if (m_Bearing.PierIdx == ALL_BEARINGS)
        {
            m_Bearing.PierIdx = m_Bearing.GirderKey.groupIndex = m_Bearing.GirderKey.girderIndex = 0;
            m_Bearing.Face = PierReactionFaceType::rftAhead;
        }
         
        //IndexType firstRL;
        //sel = m_ToBearing.GetCurSel();
        //firstRL = (IndexType)m_ToBearing.GetItemData(sel);

         // fill up the group list box
         CComboBox* pGroupBox = (CComboBox*)GetDlgItem(IDC_SPAN);
         for (GroupIndexType i = 0; i < numGroups; i++)
         {
             CString strGroup;
             if (bIsPGSuper)
             {
                 strGroup.Format(_T("Span %s"), LABEL_SPAN(i));
             }
             else
             {
                 strGroup.Format(_T("Group %d"), LABEL_GROUP(i));
             }

             pGroupBox->AddString(strGroup);
         }
         pGroupBox->SetCurSel((int)m_SegmentKey.groupIndex);

         UpdateGirderComboBox();
     }

     if (m_pInitRptSpec)
     {
         InitFromRptSpec();
     }

     CSpanItemReportDlg::OnInitDialog();

     return TRUE;  // return TRUE unless you set the focus to a control
     // EXCEPTION: OCX Property Pages should return FALSE
 }

 void CSpanGirderBearingReportDlg::OnGroupChanged()
 {
     CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SPAN);
     m_SegmentKey.groupIndex = GroupIndexType(pCB->GetCurSel());

     UpdateGirderComboBox();
 }

 void CSpanGirderBearingReportDlg::OnGirderChanged()
 {
     CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GIRDER);
     m_SegmentKey.girderIndex = GirderIndexType(pCB->GetCurSel());

     UpdateBearingComboBox();
 }

 void CSpanGirderBearingReportDlg::InitFromRptSpec()
 {

     std::shared_ptr<CBearingReportSpecification> pRptSpec = std::dynamic_pointer_cast<CBearingReportSpecification>(m_pInitRptSpec);

     m_Bearing = pRptSpec->GetReactionLocation();
     m_SegmentKey = CSegmentKey(m_Bearing.GirderKey.groupIndex, m_Bearing.GirderKey.girderIndex, 0);

     UpdateData(FALSE);

     InitChapterListFromSpec();
 }
