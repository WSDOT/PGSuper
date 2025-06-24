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

// MultiGirderReportDlg.cpp : implementation file
//

#include "stdafx.h"
#include <Reporting\MultiBearingReportDlg.h>
#include "..\Documentation\PGSuper.hh"

#include <initguid.h>
#include <IFace\Tools.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>

#include <PsgLib\GirderLabel.h>
#include <EAF\EAFDocument.h>
#include <Reporting\ReactionInterfaceAdapters.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CMultiGirderReportDlg dialog

IMPLEMENT_DYNAMIC(CMultiBearingReportDlg, CDialog)

CMultiBearingReportDlg::CMultiBearingReportDlg(std::shared_ptr<WBFL::EAF::Broker> pBroker,const WBFL::Reporting::ReportDescription& rptDesc,
    std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec,UINT nIDTemplate,CWnd* pParent)
	: CDialog(nIDTemplate, pParent), m_RptDesc(rptDesc), m_pInitRptSpec(pRptSpec)
{
   m_pBroker = pBroker;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   m_pGrid = new CMultiBearingSelectGrid();
}

CMultiBearingReportDlg::~CMultiBearingReportDlg()
{
   delete m_pGrid;
}

void CMultiBearingReportDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_ChList);

   if ( pDX->m_bSaveAndValidate )
   {
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

      // Girders
      m_Bearings = m_pGrid->GetData();
      if ( m_Bearings.empty() )
      {
         pDX->PrepareCtrl(IDC_SELECT_GRID);
         AfxMessageBox(IDS_E_NOGIRDERS);
         pDX->Fail();
      }

   }


}


BEGIN_MESSAGE_MAP(CMultiBearingReportDlg, CDialog)
   ON_BN_CLICKED(IDC_SELECT_ALL, &CMultiBearingReportDlg::OnBnClickedSelectAll)
   ON_BN_CLICKED(IDC_CLEAR_ALL, &CMultiBearingReportDlg::OnBnClickedClearAll)
END_MESSAGE_MAP()

void CMultiBearingReportDlg::UpdateChapterList()
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

BOOL CMultiBearingReportDlg::OnInitDialog()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CWnd* pwndTitle = GetDlgItem(IDC_REPORT_TITLE);
    pwndTitle->SetWindowText(m_RptDesc.GetReportName().c_str());

    CDialog::OnInitDialog();

    m_pGrid->SubclassDlgItem(IDC_SELECT_GRID, this);

    std::shared_ptr<WBFL::EAF::Broker> pBroker = EAFGetBroker();

    GET_IFACE2(pBroker, IBridge, pBridge);
    GET_IFACE2(pBroker, IBearingDesign, pBearingDesign);
    GET_IFACE2(pBroker, IIntervals, pIntervals);

    GroupGirderCollection grpGdrColl;

    GroupIndexType nGroups = pBridge->GetGirderGroupCount();
    for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
    {

        GirderReactionLocationOnCollection gdrRLColl;
        grpGdrColl.emplace_back(gdrRLColl);

        GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
        for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
        {

            ReactionLocationOnVector RLOn;
            grpGdrColl[grpIdx].emplace_back(RLOn);

            CGirderKey girderKey(grpIdx, gdrIdx);

            IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();

            std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(
                pBearingDesign, lastCompositeDeckIntervalIdx, girderKey));

            ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

            IndexType nBearings = 0;
            for (iter.First(); !iter.IsDone(); iter.Next())
            {
                nBearings += 1;
            }
            std::vector<bool> brgOn;
            brgOn.assign(nBearings, false);
            grpGdrColl[grpIdx][gdrIdx] = brgOn; // set all to false

        }

    }

    // set selected reaction locations
    for (std::vector<ReactionLocation>::iterator it = m_Bearings.begin(); it != m_Bearings.end(); it++)
    {
        const ReactionLocation& reactionLocation(*it);

        IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();

        std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(
            pBearingDesign, lastCompositeDeckIntervalIdx, reactionLocation.GirderKey));

        ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

        IndexType rlIdx = 0;
        IndexType rlOnIdx;
        for (iter.First(); !iter.IsDone(); iter.Next())
        {
            if (iter.CurrentItem() == reactionLocation)
            {
                rlOnIdx = rlIdx;
            }
            rlIdx += 1;
        }

        grpGdrColl[reactionLocation.GirderKey.groupIndex][reactionLocation.GirderKey.girderIndex][rlOnIdx] = true;
    }

    m_pGrid->CustomInit(grpGdrColl);

    UpdateChapterList();

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}



void CMultiBearingReportDlg::OnBnClickedSelectAll()
{
    m_pGrid->SetAllValues(true);
}

void CMultiBearingReportDlg::OnBnClickedClearAll()
{
    m_pGrid->SetAllValues(false);
}
