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

// CopyGirderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"
#include "CopyBearingDlg.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>
#include <IFace\Transactions.h>
#include <IFace\EditByUI.h>

#include <PgsExt\MacroTxn.h>
#include <PsgLib\BridgeDescription2.h>
#include <EAF\EAFCustSiteVars.h>

#include <EAF/EAFReportManager.h>
#include <Reporting\CopyBearingPropertiesReportSpecification.h>
#include <Reporting\CopyBearingPropertiesChapterBuilder.h>
#include <Reporting\ReactionInterfaceAdapters.h>
#include <PgsExt\EditBridge.h>
#include <AgentTools.h>



/////////////////////////////////////////////////////////////////////////////
// CCopyBearingDlg dialog

CCopyBearingDlg::CCopyBearingDlg(std::shared_ptr<WBFL::EAF::Broker> pBroker, CWnd* pParent)
	: CDialog(CCopyBearingDlg::IDD, pParent),
   m_pBroker(pBroker)
{
	//{{AFX_DATA_INIT(CCopyGirderDlg)
	//}}AFX_DATA_INIT

   // keep selection around
   //GET_IFACE(ISelection,pSelection);
   //m_FromSelection = pSelection->GetSelection();
   //if (m_FromSelection.Type != CSelection::Girder && m_FromSelection.Type != CSelection::Segment)
   //{
   //   // A girder is not selected so force the selection to be the first girder
   //   m_FromSelection.Type = CSelection::Girder;
   //   m_FromSelection.GroupIdx = (m_FromSelection.GroupIdx == INVALID_INDEX ? 0 : m_FromSelection.GroupIdx);
   //   m_FromSelection.GirderIdx = (m_FromSelection.GirderIdx == INVALID_INDEX ? 0 : m_FromSelection.GirderIdx);
   //}

   CEAFDocument* pDoc = EAFGetDocument();
   m_bIsPGSplice = pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) != FALSE;

}

void CCopyBearingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyGirderDlg)
   DDX_Control(pDX, IDC_FROM_SPAN,   m_FromGroup);
   DDX_Control(pDX, IDC_FROM_GIRDER, m_FromGirder);
   DDX_Control(pDX, IDC_FROM_BEARING, m_FromBearing);

   DDX_Control(pDX, IDC_TO_SPAN,     m_ToGroup);
   DDX_Control(pDX, IDC_TO_GIRDER,   m_ToGirder);
   DDX_Control(pDX, IDC_TO_BEARING,   m_ToBearing);

	//}}AFX_DATA_MAP

   if ( pDX->m_bSaveAndValidate )
   {
      m_FromReactionLocation = GetFromReactionLocation();
      m_ToReactionLocations  = GetToReactionLocations();

      // Save selection for next time we open
      //m_FromSelection.Type = CSelection::Girder;
      //m_FromSelection.GirderIdx = m_FromGirderKey.girderIndex;
      //m_FromSelection.GroupIdx = m_FromGirderKey.groupIndex;
   }
   else
   {
      // start with combo box for To girder
      CButton* pBut = (CButton*)GetDlgItem(IDC_RADIO1);
      pBut->SetCheck(BST_CHECKED);
      GetDlgItem(IDC_SELECT_BEARINGS)->EnableWindow(false);

      if ( m_FromSelection.Type == CSelection::Pier || m_FromSelection.Type == CSelection::Segment )
      {
         m_FromGroup.SetCurSel((int)m_FromSelection.GroupIdx);
         m_FromGirder.SetCurSel((int)m_FromSelection.GirderIdx);

         m_ToGroup.SetCurSel((int)m_FromSelection.GroupIdx+ (m_bIsPGSplice? 0:1));
      }
   }
}

BEGIN_MESSAGE_MAP(CCopyBearingDlg, CDialog)
	//{{AFX_MSG_MAP(CCopyGirderDlg)
	ON_WM_SIZE()
   ON_CBN_SELCHANGE(IDC_FROM_SPAN,OnFromGroupChanged)
   ON_CBN_SELCHANGE(IDC_TO_SPAN,OnToGroupChanged)
   ON_CBN_SELCHANGE(IDC_TO_GIRDER,OnToGirderChanged)
   ON_CBN_SELCHANGE(IDC_FROM_GIRDER,OnFromGirderChanged)
   ON_CBN_SELCHANGE(IDC_TO_BEARING, OnToReactionLocationChanged)
   ON_CBN_SELCHANGE(IDC_FROM_BEARING, OnFromReactionLocationChanged)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_PRINT, OnPrint)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_RADIO1, &CCopyBearingDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_RADIO2, &CCopyBearingDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_SELECT_BEARINGS, &CCopyBearingDlg::OnBnClickedSelectReactionLocations)
   ON_COMMAND_RANGE(CCS_CMENU_BASE, CCS_CMENU_MAX, OnCmenuSelected)
   ON_WM_DESTROY()
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopyGirderDlg message handlers

BOOL CCopyBearingDlg::OnInitDialog() 
{
   // Want to keep our size GE original size
   CRect rect;
   GetWindowRect(&rect);
   m_cxMin = rect.Width();
   m_cyMin = rect.Height();

   // set up report window
   GET_IFACE(IEAFReportManager, pReportMgr);
   WBFL::Reporting::ReportDescription rptDesc = pReportMgr->GetReportDescription(_T("Copy Bearing Properties Report"));
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pRptSpecBuilder = pReportMgr->GetReportSpecificationBuilder(rptDesc);
   std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec = pRptSpecBuilder->CreateDefaultReportSpec(rptDesc);

   m_pRptSpec = std::dynamic_pointer_cast<CCopyBearingPropertiesReportSpecification, WBFL::Reporting::ReportSpecification>(pRptSpec);

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_COPY_PROPERTIES),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

   CEAFDocument* pDoc = EAFGetDocument();

   CComboBox* pcbFromGroup = (CComboBox*)GetDlgItem(IDC_FROM_SPAN);
   CComboBox* pcbFromGirder = (CComboBox*)GetDlgItem(IDC_FROM_GIRDER);
   CComboBox* pcbFromBearing = (CComboBox*)GetDlgItem(IDC_FROM_BEARING);
   CComboBox* pcbToGroup = (CComboBox*)GetDlgItem(IDC_TO_SPAN);
   CComboBox* pcbToGirder = (CComboBox*)GetDlgItem(IDC_TO_GIRDER);
   CComboBox* pcbToBearing = (CComboBox*)GetDlgItem(IDC_TO_BEARING);
   FillComboBoxes(*pcbFromGroup,*pcbFromGirder, *pcbFromBearing,false,false,false);
   FillComboBoxes(*pcbToGroup,  *pcbToGirder, *pcbToBearing, m_bIsPGSplice ? false : true, true, true);


   CDialog::OnInitDialog();

   OnFromGroupChangedNoUpdate();
   OnToGroupChangedNoUpdate();

   EnableToolTips(TRUE);

   if ( m_bIsPGSplice )
   {
      // in PGSplice, copying can only happen within a group
      // disable the to group combo box and keep it in sync with the from group combo box
      m_ToGroup.EnableWindow(FALSE);

      // don't allow multi-select. if desired, it will be difficult to modify the grid control
      GetDlgItem(IDC_RADIO2)->ShowWindow(FALSE); 
      GetDlgItem(IDC_SELECT_BEARINGS)->ShowWindow(FALSE); 
   }

   // set up reporting window
   UpdateReportData();

   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> nullSpecBuilder;
   CWnd* pWnd = GetDlgItem(IDC_BROWSER);
   m_pBrowser = pReportMgr->CreateReportBrowser(pWnd->GetSafeHwnd(), WS_BORDER, pRptSpec, nullSpecBuilder);

   // restore the size of the window
   {
      CEAFApp* pApp = EAFGetApp();
      WINDOWPLACEMENT wp;
      if (pApp->ReadWindowPlacement(CString("Window Positions"),CString("CopyGirderDialog"),&wp))
      {
         HMONITOR hMonitor = MonitorFromRect(&wp.rcNormalPosition, MONITOR_DEFAULTTONULL); // get the monitor that has maximum overlap with the dialog rectangle (returns null if none)
         if (hMonitor != NULL)
         {
            // if dialog is within a monitor, set its position... otherwise the default position will be sued
            SetWindowPos(NULL, wp.rcNormalPosition.left, wp.rcNormalPosition.top, wp.rcNormalPosition.right - wp.rcNormalPosition.left, wp.rcNormalPosition.bottom - wp.rcNormalPosition.top, 0);
         }
      }
   }

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCopyBearingDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

   m_pBrowser->FitToParent();
}

void CCopyBearingDlg::FillComboBoxes(CComboBox& cbGroup,CComboBox& cbGirder, CComboBox& cbReactionLocation,
    bool bIncludeAllGroups, bool bIncludeAllGirders, bool bIncludeAllReactionLocations)
{
   cbGroup.ResetContent();

   CString strGroupLabel;
   BOOL bIsPGSuper = !m_bIsPGSplice;
   if (bIsPGSuper)
   {
      strGroupLabel = _T("Span");
   }
   else
   {
      strGroupLabel = _T("Group");
   }

   if ( bIncludeAllGroups )
   {
      CString strItem;
      strItem.Format(_T("All %ss"),strGroupLabel);
      int idx = cbGroup.AddString(strItem);
      cbGroup.SetItemData(idx,ALL_GROUPS);
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CString str;
      if (bIsPGSuper)
      {
         str.Format(_T("%s %s"), strGroupLabel, LABEL_SPAN(grpIdx));
      }
      else
      {
         str.Format(_T("%s %d"), strGroupLabel, LABEL_GROUP(grpIdx));
      }

      int idx = cbGroup.AddString(str);
      cbGroup.SetItemData(idx,grpIdx);
   }

   cbGroup.SetCurSel(0);

   FillGirderComboBox(cbGirder, 0, bIncludeAllGirders);
   //FillReactionLocationComboBox(cbReactionLocation,0,0,bIncludeAllReactionLocations);
}

void CCopyBearingDlg::FillGirderComboBox(CComboBox& cbGirder, GroupIndexType grpIdx, bool bIncludeAllGirders)
{
    int curSel = cbGirder.GetCurSel();

    cbGirder.ResetContent();

    if (bIncludeAllGirders)
    {
        int idx = cbGirder.AddString(_T("All Girders"));
        cbGirder.SetItemData(idx, ALL_GIRDERS);
    }

    GET_IFACE(IBridgeDescription, pIBridgeDesc);
    const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
    GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx == ALL_GROUPS ? 0 : grpIdx)->GetGirderCount();
    for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
    {
        CString str;
        str.Format(_T("Girder %s"), LABEL_GIRDER(gdrIdx));

        int idx = cbGirder.AddString(str);
        cbGirder.SetItemData(idx, gdrIdx);
    }

    if (curSel != CB_ERR)
        curSel = cbGirder.SetCurSel(curSel);

    if (curSel == CB_ERR)
        cbGirder.SetCurSel(0);
}

void CCopyBearingDlg::FillReactionLocationComboBox(CComboBox& cbReactionLocation, GirderIndexType gdrIdx,
    GroupIndexType grpIdx,bool bIncludeAllReactionLocations)
{
   int curSel = cbReactionLocation.GetCurSel();

   cbReactionLocation.ResetContent();

   if ( bIncludeAllReactionLocations )
   {
      int idx = cbReactionLocation.AddString(_T("All Bearings"));
      cbReactionLocation.SetItemData(idx,ALL_GIRDERS);  // change this

   }

   GET_IFACE(IBearingDesign, pBearingDesign);
   GET_IFACE(IIntervals, pIntervals);
   GET_IFACE(IBridge, pBridge);

   IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();

   std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(
       pBearingDesign, lastCompositeDeckIntervalIdx, CGirderKey(grpIdx, gdrIdx)));

   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);
   iter.First();
   PierIndexType startPierIdx = (iter.IsDone() ? INVALID_INDEX : iter.CurrentItem().PierIdx);


   m_RLmenuItems.clear();

   // Use iterator to walk locations

   int i = 0;
   for (iter.First(); !iter.IsDone(); iter.Next())
   {

       const ReactionLocation& reactionLocation(iter.CurrentItem());

       m_RLmenuItems.emplace_back(reactionLocation);

       cbReactionLocation.AddString(reactionLocation.PierLabel.c_str());

   }



   if ( curSel != CB_ERR )
      curSel = cbReactionLocation.SetCurSel(curSel);

   if ( curSel == CB_ERR )
      cbReactionLocation.SetCurSel(0);
}

void CCopyBearingDlg::UpdateReportData()
{
   GET_IFACE(IEAFReportManager, pReportMgr);
   std::shared_ptr<WBFL::Reporting::ReportBuilder> pBuilder = pReportMgr->GetReportBuilder( m_pRptSpec->GetReportName() );

   ReactionLocation location = GetFromReactionLocation();

   // We know we put at least one of our own chapter builders into the report builder. Find it and set its data
   IndexType numchs = pBuilder->GetChapterBuilderCount();
   for (IndexType ich = 0; ich < numchs; ich++)
   {
      std::shared_ptr<WBFL::Reporting::ChapterBuilder> pChb = pBuilder->GetChapterBuilder(ich);
      std::shared_ptr<CCopyBearingPropertiesChapterBuilder> pRptCpBuilder = std::dynamic_pointer_cast<CCopyBearingPropertiesChapterBuilder,WBFL::Reporting::ChapterBuilder>(pChb);

      if (pRptCpBuilder)
      {
          pRptCpBuilder->SetCopyBearingProperties(location);
      }

   }
}

void CCopyBearingDlg::UpdateReport()
{
   if ( m_pBrowser )
   {
      UpdateReportData();

      GET_IFACE(IEAFReportManager,pReportMgr);
      std::shared_ptr<WBFL::Reporting::ReportBuilder> pBuilder = pReportMgr->GetReportBuilder( m_pRptSpec->GetReportName() );

      std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec = std::dynamic_pointer_cast<WBFL::Reporting::ReportSpecification,CCopyBearingPropertiesReportSpecification>(m_pRptSpec);

      std::shared_ptr<rptReport> pReport = pBuilder->CreateReport( pRptSpec );
      m_pBrowser->UpdateReport( pReport, true );
   }
}

void CCopyBearingDlg::OnFromGroupChanged()
{
   OnFromGroupChangedNoUpdate();

   EnableCopyNow();
   UpdateReport(); // Report needs to show newly selected girder
}

void CCopyBearingDlg::OnFromGroupChangedNoUpdate()
{
   int curSel = m_FromGroup.GetCurSel();
   if (curSel != CB_ERR)
   {
      GroupIndexType groupIdx = (GroupIndexType)m_FromGroup.GetItemData(curSel);

      FillReactionLocationComboBox(m_FromBearing, 0, groupIdx, false);
   }
   else
   {
      FillReactionLocationComboBox(m_FromBearing, 0,  0, false);
   }
}

void CCopyBearingDlg::OnToGroupChanged() 
{
   OnToGroupChangedNoUpdate();
   EnableCopyNow();
}

void CCopyBearingDlg::OnToGroupChangedNoUpdate()
{
   int curSel = m_ToGroup.GetCurSel();
   if (curSel != CB_ERR)
   {
      GroupIndexType groupIdx = (GroupIndexType)m_ToGroup.GetItemData(curSel);
      FillReactionLocationComboBox(m_ToBearing, 0, groupIdx, true);
   }
   else
   {
      FillReactionLocationComboBox(m_ToBearing, 0, 0, true);
   }
}

void CCopyBearingDlg::OnToGirderChanged()
{
   //EnableCopyNow();
}

void CCopyBearingDlg::OnFromGirderChanged()
{
   //EnableCopyNow();

   UpdateReport();
}

void CCopyBearingDlg::OnToReactionLocationChanged()
{

    EnableCopyNow();
}

void CCopyBearingDlg::OnFromReactionLocationChanged()
{
    EnableCopyNow();

    UpdateReport();
}

ReactionLocation CCopyBearingDlg::GetFromReactionLocation()
{
   GroupIndexType grpIdx = 0;
   int sel = m_FromGroup.GetCurSel();
   if ( sel != CB_ERR )
   {
      grpIdx = (GroupIndexType)m_FromGroup.GetItemData(sel);
   }

   GirderIndexType gdrIdx = 0;
   sel = m_FromGirder.GetCurSel();
   if( sel != CB_ERR )
   {
      gdrIdx = (GirderIndexType)m_FromGirder.GetItemData(sel);
   }

   IndexType rlIdx = 0;
   sel = m_FromBearing.GetCurSel();
   if (sel != CB_ERR)
   {
       rlIdx = sel;
   }
   

   GET_IFACE(IBearingDesign, pBearingDesign);
   GET_IFACE(IIntervals, pIntervals);
   GET_IFACE(IBridge, pBridge);

   IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();

   std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(
       pBearingDesign, lastCompositeDeckIntervalIdx, CGirderKey(grpIdx, gdrIdx)));

   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);
   iter.First();
   PierIndexType startPierIdx = (iter.IsDone() ? INVALID_INDEX : iter.CurrentItem().PierIdx);

   m_RLmenuItems.clear();

   for (iter.First(); !iter.IsDone(); iter.Next())
   {
       const ReactionLocation& reactionLocation(iter.CurrentItem());

       m_RLmenuItems.emplace_back(reactionLocation);
   }

   return m_RLmenuItems[rlIdx];
}

std::vector<ReactionLocation> CCopyBearingDlg::GetToReactionLocations()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   std::vector<ReactionLocation> vec;

   // See which control to get data from
   BOOL enab_combo = IsDlgButtonChecked(IDC_RADIO1) == BST_CHECKED ? TRUE : FALSE;

   if ( enab_combo )
   {
       GroupIndexType firstGroupIdx, lastGroupIdx;
       int sel = m_ToGroup.GetCurSel();
       firstGroupIdx = (GroupIndexType)m_ToGroup.GetItemData(sel);
       if ( firstGroupIdx == ALL_GROUPS )
       {
          firstGroupIdx = 0;
          lastGroupIdx  = pBridgeDesc->GetGirderGroupCount()-1;
       }
       else
       {
          lastGroupIdx = firstGroupIdx;
       }

       for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
       {
          const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

          GirderIndexType firstGdr, lastGdr;
          sel = m_ToGirder.GetCurSel();
          firstGdr = (GirderIndexType)m_ToGirder.GetItemData(sel);
          if ( firstGdr == ALL_GIRDERS )
          {
             firstGdr = 0;
             lastGdr = pGroup->GetGirderCount()-1;
          }
          else
          {
             lastGdr = firstGdr;
          }

          GirderIndexType nGirders = pGroup->GetGirderCount();
          for (GirderIndexType gdrIdx = firstGdr; gdrIdx <= lastGdr; gdrIdx++ )
          {
             GirderIndexType realGdrIdx = gdrIdx;
             if ( nGirders <= gdrIdx )
                realGdrIdx = nGirders-1;


             GET_IFACE(IBearingDesign, pBearingDesign);
             GET_IFACE(IIntervals, pIntervals);
             GET_IFACE(IBridge, pBridge);

             IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();

             std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(
                 pBearingDesign, lastCompositeDeckIntervalIdx, CGirderKey(grpIdx, gdrIdx)));

             ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);
             iter.First();
             PierIndexType startPierIdx = (iter.IsDone() ? INVALID_INDEX : iter.CurrentItem().PierIdx);

             // Use iterator to walk locations

             IndexType firstRL;
             sel = m_ToBearing.GetCurSel();
             firstRL = (IndexType)m_ToBearing.GetItemData(sel);

             for (iter.First(); !iter.IsDone(); iter.Next())
             {

                 const ReactionLocation& reactionLocation(iter.CurrentItem());

                 vec.emplace_back(reactionLocation);

             }

             if (firstRL != ALL_BEARINGS)
             {
                 vec = std::vector<ReactionLocation>{ vec[sel-1] };
             }

          }
       }
   }
   else
   {
      // data is in grid
      vec = m_MultiDialogSelections;
   }

   return vec;
}


void CCopyBearingDlg::OnBnClickedRadio()
{
   BOOL enab_sgl = IsDlgButtonChecked(IDC_RADIO1) == BST_CHECKED ? TRUE : FALSE;
   BOOL enab_mpl = enab_sgl ? FALSE : TRUE;

   GetDlgItem(IDC_TO_SPAN)->EnableWindow(enab_sgl);
   GetDlgItem(IDC_TO_GIRDER)->EnableWindow(enab_sgl);

   GetDlgItem(IDC_SELECT_BEARINGS)->EnableWindow(enab_mpl);

   if ( enab_mpl && m_MultiDialogSelections.size() == 0 )
   {
      //OnBnClickedSelectGirders();
   }

   EnableCopyNow();
}

void CCopyBearingDlg::OnBnClickedSelectReactionLocations()
{
   CMultiBearingSelectDlg dlg;
   std::vector<ReactionLocation>::iterator iter(m_MultiDialogSelections.begin());
   std::vector<ReactionLocation>::iterator end(m_MultiDialogSelections.end());
   for ( ; iter != end; iter++ )
   {
      ReactionLocation& reactionLocation(*iter);
      dlg.m_Bearings.push_back( reactionLocation );
   }

   if (dlg.DoModal()==IDOK)
   {
      // update button text
      CString msg;
      msg.Format(_T("Select Bearings\n(%d Selected)"), dlg.m_Bearings.size());
      GetDlgItem(IDC_SELECT_BEARINGS)->SetWindowText(msg);

      m_MultiDialogSelections.clear();
      std::vector<ReactionLocation>::const_iterator iter(dlg.m_Bearings.begin());
      std::vector<ReactionLocation>::const_iterator end(dlg.m_Bearings.end());
      for ( ; iter != end; iter++ )
      {
         const ReactionLocation& reactionLocation(*iter);
         m_MultiDialogSelections.push_back(reactionLocation);
      }

      EnableCopyNow();
   }
   else
   {
      if (m_MultiDialogSelections.empty())
      {
         // was cancelled and nothing selected. Go back to single selection
         CButton* pBut = (CButton*)GetDlgItem(IDC_RADIO1);
         pBut->SetCheck(BST_CHECKED);
         pBut = (CButton*)GetDlgItem(IDC_RADIO2);
         pBut->SetCheck(BST_UNCHECKED);

         OnBnClickedRadio();
      }
   }
}

void CCopyBearingDlg::OnOK()
{
    UpdateData(TRUE);

    GET_IFACE2(m_pBroker, IBridgeDescription, pIBridgeDesc);

    const CBridgeDescription2* pOldBridgeDesc = pIBridgeDesc->GetBridgeDescription();

    CBridgeDescription2 newBridgeDesc = *pOldBridgeDesc;

    pgsTypes::PierFaceType fromFace;
    if (m_FromReactionLocation.Face == PierReactionFaceType::rftAhead)
    {
        fromFace = pgsTypes::PierFaceType::Ahead;
    }
    else
    {
        fromFace = pgsTypes::PierFaceType::Back;
    }

    const auto pbd = pIBridgeDesc->GetBearingData(
        m_FromReactionLocation.PierIdx, fromFace,
        m_FromReactionLocation.GirderKey.girderIndex);

    for (const auto& rl : m_ToReactionLocations)
    {
        CPierData2* pPier = newBridgeDesc.GetPier(rl.PierIdx);

        pgsTypes::PierFaceType toFace;
        if (rl.Face == PierReactionFaceType::rftAhead)
        {
            toFace = pgsTypes::PierFaceType::Ahead;
        }
        else
        {
            toFace = pgsTypes::PierFaceType::Back;
        }

        pPier->SetBearingData(rl.GirderKey.girderIndex, toFace, *pbd);
    }

    GET_IFACE(IEnvironment, pEnvironment);
    auto oldExposureCondition = pEnvironment->GetExposureCondition();
    auto oldClimateCondition = pEnvironment->GetClimateCondition();
    Float64 oldRelHumidity = pEnvironment->GetRelHumidity();


    std::unique_ptr<txnEditBridge> pTxn(std::make_unique<txnEditBridge>(*pOldBridgeDesc, newBridgeDesc,
        oldExposureCondition, oldExposureCondition,
        oldClimateCondition, oldClimateCondition,
        oldRelHumidity, oldRelHumidity));


    GET_IFACE(IEAFTransactions, pTransactions);
    pTransactions->Execute(std::move(pTxn));


   UpdateReport();
   EnableCopyNow();
}

void CCopyBearingDlg::OnEdit()
{
   ReactionLocation fromKey = GetFromReactionLocation();


   UpdateReport(); // we update whether any changes are made or not
   EnableCopyNow();
}

LRESULT CCopyBearingDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    // prevent the dialog from getting smaller than the original size
    if (message == WM_SIZING)
    {
        LPRECT rect = (LPRECT)lParam;
        int cx = rect->right - rect->left;
        int cy = rect->bottom - rect->top;

        if (cx < m_cxMin || cy < m_cyMin)
        {
            // prevent the dialog from moving right or down
            if (wParam == WMSZ_BOTTOMLEFT ||
                wParam == WMSZ_LEFT ||
                wParam == WMSZ_TOP ||
                wParam == WMSZ_TOPLEFT ||
                wParam == WMSZ_TOPRIGHT)
            {
                CRect r;
                GetWindowRect(&r);
                rect->left = r.left;
                rect->top = r.top;
            }

            if (cx < m_cxMin)
            {
                rect->right = rect->left + m_cxMin;
            }

            if (cy < m_cyMin)
            {
                rect->bottom = rect->top + m_cyMin;
            }

            return TRUE;
        }
    }

    return CDialog::WindowProc(message, wParam, lParam);
}

void CCopyBearingDlg::OnPrint() 
{
   m_pBrowser->Print(true);
}

void CCopyBearingDlg::EnableCopyNow()
{
   // Must be able to copy all to girders before enabling control
   //ReactionLocation copyFrom = GetFromReactionLocation();
   //std::vector<ReactionLocation> copyTo = GetToReactionLocations();

   GetDlgItem(IDOK)->EnableWindow(true);
}

void CCopyBearingDlg::CleanUp()
{
   if ( m_pBrowser )
   {
      m_pBrowser = std::shared_ptr<WBFL::Reporting::ReportBrowser>();
   }

   // save the size of the window
   WINDOWPLACEMENT wp;
   wp.length = sizeof wp;
   {
      CEAFApp* pApp = EAFGetApp();
      if (GetWindowPlacement(&wp))
      {
         wp.flags = 0;
         wp.showCmd = SW_SHOWNORMAL;
         pApp->WriteWindowPlacement(CString("Window Positions"),CString("CopyBearingDialog"),&wp);
      }
   }
}



void CCopyBearingDlg::OnDestroy()
{
   CDialog::OnDestroy();

   CleanUp();
}

BOOL CCopyBearingDlg::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      switch(nID)
      {
      case IDC_FROM_SPAN:
      case IDC_FROM_GIRDER:
         m_strTip = _T("The selected \"From\" girder is highlighted in Yellow in Comparison report");
         break;
      case IDC_EDIT:
         m_strTip = _T("Edit the selected \"From\" girder");
         break;

      default:
         return FALSE;
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      pTTT->lpszText = m_strTip.GetBuffer();
      pTTT->hinst = nullptr;
      return TRUE;
   }
   return FALSE;
}

void CCopyBearingDlg::OnCmenuSelected(UINT id)
{
  UINT cmd = id-CCS_CMENU_BASE ;

  switch(cmd)
  {
  case CCS_RB_EDIT:
     OnEdit();
     break;

  case CCS_RB_FIND:
     m_pBrowser->Find();
     break;

  case CCS_RB_SELECT_ALL:
     m_pBrowser->SelectAll();
     break;

  case CCS_RB_COPY:
     m_pBrowser->Copy();
  break;

  case CCS_RB_PRINT:
     m_pBrowser->Print(true);
     break;

  case CCS_RB_REFRESH:
     m_pBrowser->Refresh();
     break;

  case CCS_RB_VIEW_SOURCE:
     m_pBrowser->ViewSource();
     break;

  case CCS_RB_VIEW_BACK:
     m_pBrowser->Back();
     break;

  case CCS_RB_VIEW_FORWARD:
     m_pBrowser->Forward();
     break;

  default:
     // must be a toc anchor
     ATLASSERT(cmd>=CCS_RB_TOC);
     m_pBrowser->NavigateAnchor(cmd-CCS_RB_TOC);
  }
}




