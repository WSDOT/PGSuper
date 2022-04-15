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
///////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "SegmentGraphControllerBase.h"
#include <Graphing\SegmentGraphBuilderBase.h>

#include <Hints.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>

#include <EAF\EAFGraphBuilderBase.h>
#include <EAF\EAFGraphView.h>

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CSegmentGraphControllerBase,CEAFGraphControlWindow)

CSegmentGraphControllerBase::CSegmentGraphControllerBase(bool bAllGroups):
m_bAllGroups(bAllGroups),
m_SegmentKey(m_bAllGroups ? ALL_GROUPS : 0,ALL_GIRDERS, ALL_SEGMENTS)
{
}

BEGIN_MESSAGE_MAP(CSegmentGraphControllerBase, CEAFGraphControlWindow)
	//{{AFX_MSG_MAP(CSegmentGraphControllerBase)
   ON_CBN_SELCHANGE( IDC_GROUP,    CbnOnGroupChanged    )
   ON_CBN_SELCHANGE( IDC_GIRDER,   CbnOnGirderChanged   )
   ON_CBN_SELCHANGE( IDC_SEGMENT,   CbnOnSegmentChanged   )
   ON_BN_CLICKED(IDC_GRID, OnShowGrid)
   ON_BN_CLICKED(IDC_BEAM, OnShowBeam)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CSegmentGraphControllerBase::OnInitDialog()
{
   CEAFGraphControlWindow::OnInitDialog();

   EAFGetBroker(&m_pBroker);

   // Fill group and girder combo boxes
   FillGroupCtrl();
   FillGirderCtrl();
   FillSegmentCtrl();

   GET_IFACE(IDocumentType,pDocType);
   bool isPGSuper = pDocType->IsPGSuperDocument();

   // Set initial value based on the current selection
   CSegmentKey segmentKey = GetAppSelectedSegment();

   CComboBox* pcbGroup  = (CComboBox*)GetDlgItem(IDC_GROUP);
   if ( pcbGroup )
   {
      pcbGroup->SetCurSel(segmentKey.groupIndex == ALL_GROUPS ? 0 : (int)segmentKey.groupIndex + (m_bAllGroups ? 1:0));
      int curSel = pcbGroup->GetCurSel();
      DWORD_PTR itemData = pcbGroup->GetItemData(curSel);
      m_SegmentKey.groupIndex  = (GroupIndexType)(itemData);
   }

   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   if ( pcbGirder )
   {
      pcbGirder->SetCurSel(segmentKey.girderIndex == ALL_GIRDERS ? 0 : (int)segmentKey.girderIndex);
      m_SegmentKey.girderIndex = (GirderIndexType)pcbGirder->GetCurSel();
   }

   CComboBox* pcbSegment = (CComboBox*)GetDlgItem(IDC_SEGMENT);
   if (pcbSegment && !isPGSuper)
   {
      pcbSegment->SetCurSel(segmentKey.segmentIndex == ALL_SEGMENTS ? 0 : (int)segmentKey.segmentIndex);
      m_SegmentKey.segmentIndex = (SegmentIndexType)pcbSegment->GetCurSel();
   }

   CheckDlgButton(IDC_GRID, BST_CHECKED);
   CheckDlgButton(IDC_BEAM, BST_CHECKED);

   return TRUE;
}

CSegmentKey CSegmentGraphControllerBase::GetAppSelectedSegment()
{
   GET_IFACE(ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();

   // Default behavior is to select an individual segment
   GroupIndexType group = selection.GroupIdx == ALL_GROUPS ? 0 : selection.GroupIdx;
   GirderIndexType girder = selection.GirderIdx == ALL_GIRDERS ? 0 : selection.GirderIdx;
   SegmentIndexType segment = selection.SegmentIdx == ALL_SEGMENTS ? 0 : selection.SegmentIdx;

   return CSegmentKey(group,girder,segment);
}

GroupIndexType CSegmentGraphControllerBase::GetGirderGroup()             
{
   return m_SegmentKey.groupIndex;
}

GirderIndexType CSegmentGraphControllerBase::GetGirder() 
{
   return m_SegmentKey.girderIndex;
}

SegmentIndexType CSegmentGraphControllerBase::GetSegment()
{
   return m_SegmentKey.segmentIndex;
}

const CSegmentKey& CSegmentGraphControllerBase::GetSegmentKey() const
{
   return m_SegmentKey;
}

void CSegmentGraphControllerBase::SelectSegment(const CSegmentKey& segmentKey)
{
   if (m_SegmentKey != segmentKey)
   {
      m_SegmentKey = segmentKey;
      OnGroupChanged();
      OnGirderChanged();

      CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
      pcbGroup->SetCurSel(m_SegmentKey.groupIndex == ALL_GROUPS ? 0 : (int)m_SegmentKey.groupIndex + (m_bAllGroups ? 1 : 0));

      CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
      pcbGirder->SetCurSel(m_SegmentKey.girderIndex == ALL_GIRDERS ? 0 : (int)m_SegmentKey.girderIndex);
   }
}

void CSegmentGraphControllerBase::ShowGrid(bool bShow)
{
   if (bShow != ShowGrid())
   {
      CButton* pBtn = (CButton*)GetDlgItem(IDC_GRID);
      pBtn->SetCheck(bShow ? BST_CHECKED : BST_UNCHECKED);
      ((CSegmentGraphBuilderBase*)GetGraphBuilder())->ShowGrid(bShow);
   }
}

bool CSegmentGraphControllerBase::ShowGrid() const
{
   CButton* pBtn = (CButton*)GetDlgItem(IDC_GRID);
   return (pBtn->GetCheck() == BST_CHECKED ? true : false);
}

void CSegmentGraphControllerBase::ShowBeam(bool bShow)
{
   if (bShow != ShowBeam())
   {
      CButton* pBtn = (CButton*)GetDlgItem(IDC_BEAM);
      pBtn->SetCheck(bShow ? BST_CHECKED : BST_UNCHECKED);
      ((CSegmentGraphBuilderBase*)GetGraphBuilder())->ShowBeam(bShow);
   }
}

bool CSegmentGraphControllerBase::ShowBeam() const
{
   CButton* pBtn = (CButton*)GetDlgItem(IDC_BEAM);
   return (pBtn->GetCheck() == BST_CHECKED ? true : false);
}

void CSegmentGraphControllerBase::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   CEAFGraphControlWindow::OnUpdate(pSender,lHint,pHint);
   if ( lHint == HINT_UNITSCHANGED )
   {
      ((CSegmentGraphBuilderBase*)GetGraphBuilder())->UpdateXAxis();
      ((CSegmentGraphBuilderBase*)GetGraphBuilder())->UpdateYAxis();
   }

   if ( lHint == HINT_BRIDGECHANGED )
   {
      // The bridge changed, so reset the controls
      FillGroupCtrl();
      FillGirderCtrl();
   }
}

void CSegmentGraphControllerBase::CbnOnGroupChanged()
{
   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
   int curSel = pcbGroup->GetCurSel();
   GroupIndexType grpIdx = (GroupIndexType)pcbGroup->GetItemData(curSel);
   
   if ( m_SegmentKey.groupIndex == grpIdx )
   {
      return;
   }

   m_SegmentKey.groupIndex = grpIdx;

   // Rebuild the girder choices... the girder choices
   // have to be consistent with the currently selected group
   FillGirderCtrl();

   OnGroupChanged();

   // Update the graph
   UpdateGraph();
}

void CSegmentGraphControllerBase::CbnOnGirderChanged()
{
   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);

   int curSel = pcbGirder->GetCurSel();
   if (curSel == CB_ERR) 
   {
      m_SegmentKey.girderIndex = 0;
      pcbGirder->SetCurSel(0);
   }
   else if (GirderIndexType(curSel) == m_SegmentKey.girderIndex)
   {
      // The girder didn't actually change..
      return;
   }

   m_SegmentKey.girderIndex = GirderIndexType(curSel);

   OnGirderChanged();

   UpdateGraph();
}

void CSegmentGraphControllerBase::CbnOnSegmentChanged()
{
   CComboBox* pcbSegment = (CComboBox*)GetDlgItem(IDC_SEGMENT);

   int curSel = pcbSegment->GetCurSel();
   if (curSel == CB_ERR)
   {
      m_SegmentKey.segmentIndex = 0;
      pcbSegment->SetCurSel(0);
   }
   else if (SegmentIndexType(curSel) == m_SegmentKey.segmentIndex)
   {
      // The segment didn't actually change..
      return;
   }

   m_SegmentKey.segmentIndex = SegmentIndexType(curSel);

   OnSegmentChanged();

   UpdateGraph();
}

void CSegmentGraphControllerBase::OnShowGrid()
{
   ((CSegmentGraphBuilderBase*)GetGraphBuilder())->ShowGrid(ShowGrid());
}

void CSegmentGraphControllerBase::OnShowBeam()
{
   ((CSegmentGraphBuilderBase*)GetGraphBuilder())->ShowBeam(ShowBeam());
}


void CSegmentGraphControllerBase::OnGroupChanged()
{
}

void CSegmentGraphControllerBase::OnGirderChanged()
{
}

void CSegmentGraphControllerBase::OnSegmentChanged()
{
}

void CSegmentGraphControllerBase::FillGroupCtrl()
{
   CComboBox* pcbGroup  = (CComboBox*)GetDlgItem(IDC_GROUP);
   if ( pcbGroup == nullptr )
   {
      return; // not using a group list
   }

   pcbGroup->ResetContent();

   GET_IFACE(IDocumentType,pDocType);
   bool isPGSuper = pDocType->IsPGSuperDocument();
   CString strGroupLabel( isPGSuper ? _T("Span") : _T("Group"));

   int idx;

   if ( m_bAllGroups )
   {
      CString string(pDocType->IsPGSuperDocument() ? _T("All Spans") : _T("All Groups"));
      idx = pcbGroup->AddString(string);
      pcbGroup->SetItemData(idx,ALL_GROUPS);
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CString strItem;
      if (isPGSuper)
      {
         strItem.Format(_T("%s %s"), strGroupLabel, LABEL_SPAN(grpIdx));
      }
      else
      {
         strItem.Format(_T("%s %d"), strGroupLabel, LABEL_GROUP(grpIdx));
      }

      idx = pcbGroup->AddString(strItem);
      pcbGroup->SetItemData(idx,grpIdx);
   }

   pcbGroup->SetCurSel(0);
}

void CSegmentGraphControllerBase::FillGirderCtrl()
{
   GroupIndexType grpIdx = GetGirderGroup();

   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   if ( pcbGirder == nullptr )
   {
      return; // not using a girder list
   }

   int curSel = pcbGirder->GetCurSel();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GirderIndexType nGirders = 0;
   if ( grpIdx == ALL_GROUPS )
   {
      GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
      for ( GroupIndexType gIdx = 0; gIdx < nGroups; gIdx++ )
      {
         nGirders = Max(nGirders,pBridgeDesc->GetGirderGroup(gIdx)->GetGirderCount());
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

   if ( curSel == CB_ERR )
   {
      pcbGirder->SetCurSel(0);
      m_SegmentKey.girderIndex = 0;
   }
   else
   {
      curSel = Min(curSel,(int)(nGirders-1));
      pcbGirder->SetCurSel(curSel);
      m_SegmentKey.girderIndex = (GirderIndexType)curSel;
   }
}

void CSegmentGraphControllerBase::FillSegmentCtrl()
{
   CComboBox* pcbSegment = (CComboBox*)GetDlgItem(IDC_SEGMENT);
   if (pcbSegment == nullptr)
   {
      return; // not using a seg list
   }

   GET_IFACE(IDocumentType,pDocType);
   bool isPGSuper = pDocType->IsPGSuperDocument();
   if (isPGSuper)
   {
      pcbSegment->ShowWindow(SW_HIDE);
      m_SegmentKey.segmentIndex = 0;
      return;
   }

   GroupIndexType  grpIdx = GetGirderGroup();
   GirderIndexType gdrIdx = GetGirder();

   int curSel = pcbSegment->GetCurSel();
   pcbSegment->ResetContent();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SegmentIndexType nSegments = 0;
   if (grpIdx == ALL_GROUPS || gdrIdx == ALL_GIRDERS)
   {
      ATLASSERT(0);
      return;
   }
   else
   {
      nSegments = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirder(gdrIdx)->GetSegmentCount();
   }

   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CString strSegment;
      strSegment.Format(_T("Segment %d"),LABEL_SEGMENT(segIdx));
      pcbSegment->AddString(strSegment);
   }

   if (curSel == CB_ERR)
   {
      pcbSegment->SetCurSel(0);
      m_SegmentKey.segmentIndex = 0;
   }
   else
   {
      curSel = Min(curSel,(int)(nSegments - 1));
      pcbSegment->SetCurSel(curSel);
      m_SegmentKey.segmentIndex = (SegmentIndexType)curSel;
   }
}

void CSegmentGraphControllerBase::UpdateGraph()
{
   ((CSegmentGraphBuilderBase*)GetGraphBuilder())->InvalidateGraph();
   ((CSegmentGraphBuilderBase*)GetGraphBuilder())->Update();
}

#ifdef _DEBUG
void CSegmentGraphControllerBase::AssertValid() const
{
	CEAFGraphControlWindow::AssertValid();
}

void CSegmentGraphControllerBase::Dump(CDumpContext& dc) const
{
	CEAFGraphControlWindow::Dump(dc);
}
#endif //_DEBUG


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CIntervalSegmentGraphControllerBase,CSegmentGraphControllerBase)

CIntervalSegmentGraphControllerBase::CIntervalSegmentGraphControllerBase(bool bAllGroups):
CSegmentGraphControllerBase(bAllGroups),
m_IntervalIdx(INVALID_INDEX)
{
}

BEGIN_MESSAGE_MAP(CIntervalSegmentGraphControllerBase, CSegmentGraphControllerBase)
	//{{AFX_MSG_MAP(CIntervalSegmentGraphControllerBase)
   ON_CBN_SELCHANGE( IDC_INTERVAL, CbnOnIntervalChanged )
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CIntervalSegmentGraphControllerBase::OnInitDialog()
{
   CSegmentGraphControllerBase::OnInitDialog();

   // Fill the interval combo box
   FillIntervalCtrl();

   // Set initial value
   CComboBox* pcbInterval = (CComboBox*)GetDlgItem(IDC_INTERVAL);
   if ( pcbInterval )
   {
      pcbInterval->SetCurSel(0);
      m_IntervalIdx = (IntervalIndexType)pcbInterval->GetItemData(pcbInterval->GetCurSel());
   }
   else
   {
      m_IntervalIdx = INVALID_INDEX;
   }

   return TRUE;
}

void CIntervalSegmentGraphControllerBase::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   CSegmentGraphControllerBase::OnUpdate(pSender,lHint,pHint);
   if ( lHint == HINT_BRIDGECHANGED )
   {
      FillIntervalCtrl();
   }
}

void CIntervalSegmentGraphControllerBase::SetInterval(IntervalIndexType intervalIdx)
{
   if (m_IntervalIdx != intervalIdx)
   {
      m_IntervalIdx = intervalIdx;
      IntervalIndexType firstIntervalIdx = GetFirstInterval();
      CComboBox* pcbInterval = (CComboBox*)GetDlgItem(IDC_INTERVAL);
      pcbInterval->SetCurSel((int)(m_IntervalIdx - firstIntervalIdx));
      OnIntervalChanged();
      UpdateGraph();
   }
}

IntervalIndexType CIntervalSegmentGraphControllerBase::GetInterval() const
{
   return m_IntervalIdx;
}

void CIntervalSegmentGraphControllerBase::CbnOnIntervalChanged()
{
   IntervalIndexType interval;

   CComboBox* pcbInterval = (CComboBox*)GetDlgItem(IDC_INTERVAL);

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

void CIntervalSegmentGraphControllerBase::OnIntervalChanged()
{
}

IntervalIndexType CIntervalSegmentGraphControllerBase::GetFirstInterval()
{
   GET_IFACE(IIntervals, pIntervals);
   return pIntervals->GetFirstPrestressReleaseInterval(GetSegmentKey());
}

IntervalIndexType CIntervalSegmentGraphControllerBase::GetLastInterval()
{
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   return nIntervals - 1;
}

void CIntervalSegmentGraphControllerBase::FillIntervalCtrl()
{
   CComboBox* pcbIntervals = (CComboBox*)GetDlgItem(IDC_INTERVAL);
   if ( pcbIntervals == nullptr )
   {
      return; // not using an intervals list
   }

   int curSel = pcbIntervals->GetCurSel();
   IntervalIndexType curIntervalIdx = INVALID_INDEX;
   if (curSel != CB_ERR)
   {
      curIntervalIdx = pcbIntervals->GetItemData(curSel);
   }
   ATLASSERT(m_IntervalIdx == curIntervalIdx);

   pcbIntervals->ResetContent();

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType firstIntervalIdx = GetFirstInterval();
   IntervalIndexType lastIntervalIdx = GetLastInterval();
   for (IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++)
   {
      CString strInterval;
      strInterval.Format(_T("Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
      int idx = pcbIntervals->AddString(strInterval);
      pcbIntervals->SetItemData(idx,intervalIdx);

      if (intervalIdx == curIntervalIdx)
      {
         curSel = idx;
      }
   }

   curSel = pcbIntervals->SetCurSel(curSel);
   if ( curSel == CB_ERR )
   {
      // don't let selected interval be out of range
      IntervalIndexType cs = (firstIntervalIdx <= m_IntervalIdx && m_IntervalIdx <= lastIntervalIdx) ? m_IntervalIdx-firstIntervalIdx : 0;
      curSel = pcbIntervals->SetCurSel((int)cs);
      if(curSel==CB_ERR)
      {
         ATLASSERT(false); // logic to get cs f***ed?
         curSel = pcbIntervals->SetCurSel(0);
      }

      m_IntervalIdx = pcbIntervals->GetItemData(curSel);
   }
}

#ifdef _DEBUG
void CIntervalSegmentGraphControllerBase::AssertValid() const
{
	CSegmentGraphControllerBase::AssertValid();
}

void CIntervalSegmentGraphControllerBase::Dump(CDumpContext& dc) const
{
	CSegmentGraphControllerBase::Dump(dc);
}
#endif //_DEBUG


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CMultiIntervalSegmentGraphControllerBase,CSegmentGraphControllerBase)

CMultiIntervalSegmentGraphControllerBase::CMultiIntervalSegmentGraphControllerBase(bool bAllGroups):
CSegmentGraphControllerBase(bAllGroups)
{
}

BEGIN_MESSAGE_MAP(CMultiIntervalSegmentGraphControllerBase, CSegmentGraphControllerBase)
	//{{AFX_MSG_MAP(CMultiIntervalSegmentGraphControllerBase)
   ON_LBN_SELCHANGE( IDC_INTERVALS, OnIntervalsChanged )
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CMultiIntervalSegmentGraphControllerBase::OnInitDialog()
{
   CSegmentGraphControllerBase::OnInitDialog();
   FillIntervalCtrl();
   return TRUE;
}

void CMultiIntervalSegmentGraphControllerBase::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   CSegmentGraphControllerBase::OnUpdate(pSender,lHint,pHint);
   if ( lHint == HINT_BRIDGECHANGED )
   {
      FillIntervalCtrl();
   }
}

void CMultiIntervalSegmentGraphControllerBase::SelectInterval(IntervalIndexType intervalIdx)
{
   CListBox* plbIntervals = (CListBox*)GetDlgItem(IDC_INTERVALS);
   plbIntervals->SetCurSel(-1); // clears the current selection

   IntervalIndexType firstIntervalIdx = GetFirstInterval(); // this interval is at position 0
   int idx = (int)(intervalIdx - firstIntervalIdx);
   plbIntervals->SetSel(idx);
   UpdateGraph();
}

void CMultiIntervalSegmentGraphControllerBase::SelectIntervals(const std::vector<IntervalIndexType>& vIntervals)
{
   CListBox* plbIntervals = (CListBox*)GetDlgItem(IDC_INTERVALS);
   plbIntervals->SetCurSel(-1); // clears the current selection

   IntervalIndexType firstIntervalIdx = GetFirstInterval(); // this interval is at position 0
   for (const auto& intervalIdx : vIntervals)
   {
      int idx = (int)(intervalIdx - firstIntervalIdx);
      plbIntervals->SetSel(idx, TRUE);
   }
   UpdateGraph();
}

std::vector<IntervalIndexType> CMultiIntervalSegmentGraphControllerBase::GetSelectedIntervals() const
{
   std::vector<IntervalIndexType> vIntervals;

   CListBox* plbIntervals = (CListBox*)GetDlgItem(IDC_INTERVALS);

   // capture the current selection
   int selCount = plbIntervals->GetSelCount();
   CArray<int,int> selItemIndices;
   selItemIndices.SetSize(selCount);
   plbIntervals->GetSelItems(selCount,selItemIndices.GetData());

   int i;
   for ( i = 0; i < selCount; i++ )
   {
      IntervalIndexType intervalIdx = (IntervalIndexType)plbIntervals->GetItemData(selItemIndices[i]);
      vIntervals.push_back(intervalIdx);
   }

   return vIntervals;
}

IndexType CMultiIntervalSegmentGraphControllerBase::GetGraphCount() const
{
   CListBox* plbIntervals = (CListBox*)GetDlgItem(IDC_INTERVALS);
   return plbIntervals->GetSelCount();
}

void CMultiIntervalSegmentGraphControllerBase::OnIntervalsChanged()
{
   UpdateGraph();
}

void CMultiIntervalSegmentGraphControllerBase::FillIntervalCtrl()
{
   CListBox* plbIntervals = (CListBox*)GetDlgItem(IDC_INTERVALS);

   // capture the current selection
   int selCount = plbIntervals->GetSelCount();
   CArray<int,int> selItemIndices;
   selItemIndices.SetSize(selCount);
   plbIntervals->GetSelItems(selCount,selItemIndices.GetData());

   CStringArray selItems;
   int i;
   for ( i = 0; i < selCount; i++ )
   {
      CString strItem;
      plbIntervals->GetText(selItemIndices[i],strItem);
      selItems.Add( strItem );
   }

   // refill control
   plbIntervals->ResetContent();

   CSegmentKey segmentKey(GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType firstIntervalIdx = GetFirstInterval();
   IntervalIndexType lastIntervalIdx  = GetLastInterval();
   for ( IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++ )
   {
      CString str;
      str.Format(_T("%d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
      int idx = plbIntervals->AddString(str);
      plbIntervals->SetItemData(idx,intervalIdx);
   }

   // reselect anything that was previously selected
   for ( i = 0; i < selCount; i++ )
   {
      CString strItem = selItems[i];
      int idx = plbIntervals->FindStringExact(-1,strItem);
      if ( idx != LB_ERR )
      {
         plbIntervals->SetSel(idx);
      }
   }
}

IntervalIndexType CMultiIntervalSegmentGraphControllerBase::GetFirstInterval()
{
   GET_IFACE(IIntervals, pIntervals);
   return pIntervals->GetFirstPrestressReleaseInterval(GetSegmentKey());
}

IntervalIndexType CMultiIntervalSegmentGraphControllerBase::GetLastInterval()
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   return nIntervals-1;
}

#ifdef _DEBUG
void CMultiIntervalSegmentGraphControllerBase::AssertValid() const
{
	CSegmentGraphControllerBase::AssertValid();
}

void CMultiIntervalSegmentGraphControllerBase::Dump(CDumpContext& dc) const
{
	CSegmentGraphControllerBase::Dump(dc);
}
#endif //_DEBUG
