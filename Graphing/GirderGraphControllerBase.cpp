///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#include "stdafx.h"
#include "resource.h"
#include "GirderGraphControllerBase.h"
#include <Graphing\GirderGraphBuilderBase.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>

#include <EAF\EAFGraphBuilderBase.h>
#include <EAF\EAFGraphView.h>

#include <PgsExt\BridgeDescription2.h>

IMPLEMENT_DYNAMIC(CGirderGraphControllerBase,CEAFGraphControlWindow)

CGirderGraphControllerBase::CGirderGraphControllerBase(bool bAllGroups):
m_bAllGroups(bAllGroups),
m_GroupIdx(0),
m_GirderIdx(0),
m_IntervalIdx(0)
{
}

BEGIN_MESSAGE_MAP(CGirderGraphControllerBase, CEAFGraphControlWindow)
	//{{AFX_MSG_MAP(CGirderGraphControllerBase)
   ON_CBN_SELCHANGE( IDC_GROUP,    CbnOnGroupChanged    )
   ON_CBN_SELCHANGE( IDC_GIRDER,   CbnOnGirderChanged   )
   ON_CBN_SELCHANGE( IDC_INTERVAL, CbnOnIntervalChanged )
   ON_BN_CLICKED(IDC_GRID, OnShowGrid)
   ON_BN_CLICKED(IDC_BEAM, OnShowBeam)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CGirderGraphControllerBase::OnInitDialog()
{
   CEAFGraphControlWindow::OnInitDialog();

   EAFGetBroker(&m_pBroker);

   // Fill group and girder combo boxes
   FillGroupCtrl();
   FillGirderCtrl();

   // Set initial value based on the current selection
   GET_IFACE(ISelectionEx,pSelection);
   CSelection selection = pSelection->GetSelection();
   CComboBox* pcbGroup  = (CComboBox*)GetDlgItem(IDC_GROUP);
   pcbGroup->SetCurSel( selection.GroupIdx == ALL_GROUPS ? 0 : (int)selection.GroupIdx+1);
   m_GroupIdx  = (GroupIndexType)(pcbGroup->GetItemData(pcbGroup->GetCurSel()));

   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   pcbGirder->SetCurSel(selection.GirderIdx == ALL_GIRDERS ? 0 : (int)selection.GirderIdx);
   m_GirderIdx = (GirderIndexType)(pcbGirder->GetItemData(pcbGirder->GetCurSel()));

   // Fill the interval combo box
   FillIntervalCtrl();

   // Set initial value
   CComboBox* pcbInterval = (CComboBox*)GetDlgItem(IDC_INTERVAL);
   pcbInterval->SetCurSel(0);
   m_IntervalIdx = (IntervalIndexType)pcbInterval->GetItemData(pcbInterval->GetCurSel());

   return TRUE;
}

GroupIndexType CGirderGraphControllerBase::GetGirderGroup()             
{
   return m_GroupIdx;
}

GirderIndexType CGirderGraphControllerBase::GetGirder() 
{
   return m_GirderIdx;
}

IntervalIndexType CGirderGraphControllerBase::GetInterval()     
{
   return m_IntervalIdx;
}

void CGirderGraphControllerBase::CbnOnGroupChanged()
{
   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
   int curSel = pcbGroup->GetCurSel();
   GroupIndexType grpIdx = (GroupIndexType)pcbGroup->GetItemData(curSel);
   
   if ( m_GroupIdx == grpIdx )
      return;

   m_GroupIdx = grpIdx;

   // Rebuild the girder choices... the girder choices
   // have to be consistent with the currently selected group
   FillGirderCtrl();

   OnGroupChanged();

   // Update the graph
   UpdateGraph();
}

void CGirderGraphControllerBase::CbnOnGirderChanged()
{
   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);

   int curSel = pcbGirder->GetCurSel();
   if (curSel == CB_ERR) 
   {
      m_GirderIdx = 0;
      pcbGirder->SetCurSel(0);
   }
   else if (GirderIndexType(curSel) == m_GirderIdx)
   {
      // The girder didn't actually change..
      return;
   }

   m_GirderIdx = GirderIndexType(curSel);

   OnGirderChanged();

   UpdateGraph();
}

void CGirderGraphControllerBase::CbnOnIntervalChanged()
{
   IntervalIndexType interval;

   CComboBox* pcbInterval = (CComboBox*)GetDlgItem(IDC_INTERVAL);
   CListBox* plbLoadCase  = (CListBox*)GetDlgItem(IDC_LOAD_CASE);

   int curSel = pcbInterval->GetCurSel();
   if (curSel == CB_ERR) 
   {
      pcbInterval->SetCurSel(0);
      interval = (IntervalIndexType)(pcbInterval->GetItemData(0));
   }
   else
   {
      interval = (IntervalIndexType)(pcbInterval->GetItemData(curSel));
   }

   if ( interval == m_IntervalIdx )
   {
      // The interval didn't actually change
      return;
   }

   m_IntervalIdx = interval;

   OnIntervalChanged();

   UpdateGraph();
}

void CGirderGraphControllerBase::OnShowGrid()
{
   BOOL bIsChecked = IsDlgButtonChecked(IDC_GRID);
   ((CGirderGraphBuilderBase*)GetGraphBuilder())->ShowGrid(bIsChecked == TRUE ? true : false);
}

void CGirderGraphControllerBase::OnShowBeam()
{
   BOOL bIsChecked = IsDlgButtonChecked(IDC_BEAM);
   ((CGirderGraphBuilderBase*)GetGraphBuilder())->ShowBeam(bIsChecked == TRUE ? true : false);
}

void CGirderGraphControllerBase::OnGroupChanged()
{
}

void CGirderGraphControllerBase::OnGirderChanged()
{
}

void CGirderGraphControllerBase::OnIntervalChanged()
{
}

void CGirderGraphControllerBase::FillGroupCtrl()
{
   CComboBox* pcbGroup  = (CComboBox*)GetDlgItem(IDC_GROUP);
   pcbGroup->ResetContent();

   GET_IFACE(IDocumentType,pDocType);
   CString strGroupLabel(pDocType->IsPGSuperDocument() ? _T("Span") : _T("Group"));

   int idx;

   if ( m_bAllGroups )
   {
      idx = pcbGroup->AddString(pDocType->IsPGSuperDocument() ? _T("All Spans") : _T("All Groups"));
      pcbGroup->SetItemData(idx,ALL_GROUPS);
   }


   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CString strItem;
      strItem.Format(_T("%s %d"),strGroupLabel,LABEL_GROUP(grpIdx));
      idx = pcbGroup->AddString(strItem);
      pcbGroup->SetItemData(idx,grpIdx);
   }

   pcbGroup->SetCurSel(0);
}

void CGirderGraphControllerBase::FillGirderCtrl()
{
   GroupIndexType grpIdx = GetGirderGroup();

   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   int curSel = pcbGirder->GetCurSel();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GirderIndexType nGirders = ALL_GIRDERS;
   if ( grpIdx == ALL_GROUPS )
   {
      GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
      for ( GroupIndexType gIdx = 0; gIdx < nGroups; gIdx++ )
      {
         nGirders = min(nGirders,pBridgeDesc->GetGirderGroup(gIdx)->GetGirderCount());
      }
   }
   else
   {
      nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
   }

   pcbGirder->ResetContent();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      CString strGirder;
      strGirder.Format(_T("Girder %s"),LABEL_GIRDER(gdrIdx));
      pcbGirder->AddString(strGirder);
   }

   if ( curSel != CB_ERR && curSel < nGirders)
   {
      pcbGirder->SetCurSel(curSel);
   }
   else
   {
      pcbGirder->SetCurSel(0);
   }
}

void CGirderGraphControllerBase::FillIntervalCtrl()
{
   CComboBox* pcbIntervals = (CComboBox*)GetDlgItem(IDC_INTERVAL);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType startIntervalIdx = pIntervals->GetPrestressReleaseInterval(CSegmentKey(0,0,0));
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( IntervalIndexType intervalIdx = startIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      CString strInterval;
      strInterval.Format(_T("Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
      int idx = pcbIntervals->AddString(strInterval);
      pcbIntervals->SetItemData(idx,intervalIdx);
   }

   pcbIntervals->SetCurSel(0);
}

void CGirderGraphControllerBase::UpdateGraph()
{
   ((CGirderGraphBuilderBase*)GetGraphBuilder())->InvalidateGraph();
   ((CGirderGraphBuilderBase*)GetGraphBuilder())->Update();
}

#ifdef _DEBUG
void CGirderGraphControllerBase::AssertValid() const
{
	CEAFGraphControlWindow::AssertValid();
}

void CGirderGraphControllerBase::Dump(CDumpContext& dc) const
{
	CEAFGraphControlWindow::Dump(dc);
}
#endif //_DEBUG
