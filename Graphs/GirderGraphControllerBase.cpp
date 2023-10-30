///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <Graphs\GirderGraphBuilderBase.h>

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

IMPLEMENT_DYNAMIC(CGirderGraphControllerBase,CEAFGraphControlWindow)

CGirderGraphControllerBase::CGirderGraphControllerBase(bool bAllGroups):
m_bAllGroups(bAllGroups),
m_GirderKey(m_bAllGroups ? ALL_GROUPS : 0,ALL_GIRDERS)
{
}

BEGIN_MESSAGE_MAP(CGirderGraphControllerBase, CEAFGraphControlWindow)
	//{{AFX_MSG_MAP(CGirderGraphControllerBase)
   ON_CBN_SELCHANGE( IDC_GROUP,    CbnOnGroupChanged    )
   ON_CBN_SELCHANGE( IDC_GIRDER,   CbnOnGirderChanged   )
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
   GET_IFACE(ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();

   CComboBox* pcbGroup  = (CComboBox*)GetDlgItem(IDC_GROUP);
   if ( pcbGroup )
   {
      pcbGroup->SetCurSel( selection.GroupIdx == ALL_GROUPS ? 0 : (int)selection.GroupIdx + (m_bAllGroups ? 1:0));
      int curSel = pcbGroup->GetCurSel();
      DWORD_PTR itemData = pcbGroup->GetItemData(curSel);
      m_GirderKey.groupIndex  = (GroupIndexType)(itemData);
   }

   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   if ( pcbGirder )
   {
      pcbGirder->SetCurSel(selection.GirderIdx == ALL_GIRDERS ? 0 : (int)selection.GirderIdx);
      m_GirderKey.girderIndex = (GirderIndexType)pcbGirder->GetCurSel();
   }

   CheckDlgButton(IDC_GRID, BST_CHECKED);
   CheckDlgButton(IDC_BEAM, BST_CHECKED);

   return TRUE;
}

GroupIndexType CGirderGraphControllerBase::GetGirderGroup()             
{
   return m_GirderKey.groupIndex;
}

GirderIndexType CGirderGraphControllerBase::GetGirder() 
{
   return m_GirderKey.girderIndex;
}

const CGirderKey& CGirderGraphControllerBase::GetGirderKey() const
{
   return m_GirderKey;
}

void CGirderGraphControllerBase::SelectGirder(const CGirderKey& girderKey)
{
   if (m_GirderKey != girderKey)
   {
      m_GirderKey = girderKey;
      OnGroupChanged();
      OnGirderChanged();

      // this also capture the span setting if that's the case
      CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
      pcbGroup->SetCurSel(m_GirderKey.groupIndex == ALL_GROUPS ? 0 : (int)m_GirderKey.groupIndex + (m_bAllGroups ? 1 : 0));

      CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
      pcbGirder->SetCurSel(m_GirderKey.girderIndex == ALL_GIRDERS ? 0 : (int)m_GirderKey.girderIndex);
   }
}

void CGirderGraphControllerBase::ShowGrid(bool bShow)
{
   if (bShow != ShowGrid())
   {
      CButton* pBtn = (CButton*)GetDlgItem(IDC_GRID);
      pBtn->SetCheck(bShow ? BST_CHECKED : BST_UNCHECKED);
      ((CGirderGraphBuilderBase*)GetGraphBuilder())->ShowGrid(bShow);
   }
}

bool CGirderGraphControllerBase::ShowGrid() const
{
   CButton* pBtn = (CButton*)GetDlgItem(IDC_GRID);
   return (pBtn->GetCheck() == BST_CHECKED ? true : false);
}

void CGirderGraphControllerBase::ShowBeam(bool bShow)
{
   if (bShow != ShowBeam())
   {
      CButton* pBtn = (CButton*)GetDlgItem(IDC_BEAM);
      pBtn->SetCheck(bShow ? BST_CHECKED : BST_UNCHECKED);
      ((CGirderGraphBuilderBase*)GetGraphBuilder())->ShowBeam(bShow);
   }
}

bool CGirderGraphControllerBase::ShowBeam() const
{
   CButton* pBtn = (CButton*)GetDlgItem(IDC_BEAM);
   return (pBtn->GetCheck() == BST_CHECKED ? true : false);
}

void CGirderGraphControllerBase::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   CEAFGraphControlWindow::OnUpdate(pSender,lHint,pHint);
   if ( lHint == HINT_UNITSCHANGED )
   {
      ((CGirderGraphBuilderBase*)GetGraphBuilder())->UpdateXAxis();
      ((CGirderGraphBuilderBase*)GetGraphBuilder())->UpdateYAxis();
   }

   if ( lHint == HINT_BRIDGECHANGED )
   {
      // The bridge changed, so reset the controls
      FillGroupCtrl();
      FillGirderCtrl();
   }
}

void CGirderGraphControllerBase::CbnOnGroupChanged()
{
   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
   int curSel = pcbGroup->GetCurSel();
   GroupIndexType grpIdx = (GroupIndexType)pcbGroup->GetItemData(curSel);
   
   if ( m_GirderKey.groupIndex == grpIdx )
   {
      return;
   }

   m_GirderKey.groupIndex = grpIdx;

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
      m_GirderKey.girderIndex = 0;
      pcbGirder->SetCurSel(0);
   }
   else if (GirderIndexType(curSel) == m_GirderKey.girderIndex)
   {
      // The girder didn't actually change..
      return;
   }

   m_GirderKey.girderIndex = GirderIndexType(curSel);

   OnGirderChanged();

   UpdateGraph();
}

void CGirderGraphControllerBase::OnShowGrid()
{
   ((CGirderGraphBuilderBase*)GetGraphBuilder())->ShowGrid(ShowGrid());
}

void CGirderGraphControllerBase::OnShowBeam()
{
   ((CGirderGraphBuilderBase*)GetGraphBuilder())->ShowBeam(ShowBeam());
}

void CGirderGraphControllerBase::OnGroupChanged()
{
}

void CGirderGraphControllerBase::OnGirderChanged()
{
}

void CGirderGraphControllerBase::FillGroupCtrl()
{
   CComboBox* pcbGroup  = (CComboBox*)GetDlgItem(IDC_GROUP);
   if ( pcbGroup == nullptr )
   {
      return; // not using a group list
   }

   int cursel = pcbGroup->GetCurSel();

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

   int nCount = pcbGroup->GetCount();

   if ((cursel == CB_ERR) || cursel > nCount - 1)
   {
      cursel = 0;
   }

   pcbGroup->SetCurSel(cursel);
}

void CGirderGraphControllerBase::FillGirderCtrl()
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
      m_GirderKey.girderIndex = 0;
   }
   else
   {
      curSel = Min(curSel,(int)(nGirders-1));
      pcbGirder->SetCurSel(curSel);
      m_GirderKey.girderIndex = (GirderIndexType)curSel;
   }
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


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CIntervalGirderGraphControllerBase,CGirderGraphControllerBase)

CIntervalGirderGraphControllerBase::CIntervalGirderGraphControllerBase(bool bAllGroups):
CGirderGraphControllerBase(bAllGroups),
m_IntervalIdx(INVALID_INDEX)
{
}

BEGIN_MESSAGE_MAP(CIntervalGirderGraphControllerBase, CGirderGraphControllerBase)
	//{{AFX_MSG_MAP(CIntervalGirderGraphControllerBase)
   ON_CBN_SELCHANGE( IDC_INTERVAL, CbnOnIntervalChanged )
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CIntervalGirderGraphControllerBase::OnInitDialog()
{
   CGirderGraphControllerBase::OnInitDialog();

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

void CIntervalGirderGraphControllerBase::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   CGirderGraphControllerBase::OnUpdate(pSender,lHint,pHint);
   if ( lHint == HINT_BRIDGECHANGED )
   {
      FillIntervalCtrl();
   }
}

void CIntervalGirderGraphControllerBase::SetInterval(IntervalIndexType intervalIdx)
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

IntervalIndexType CIntervalGirderGraphControllerBase::GetInterval() const
{
   return m_IntervalIdx;
}

void CIntervalGirderGraphControllerBase::CbnOnIntervalChanged()
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

void CIntervalGirderGraphControllerBase::OnIntervalChanged()
{
}

IntervalIndexType CIntervalGirderGraphControllerBase::GetFirstInterval()
{
   GET_IFACE(IIntervals, pIntervals);
   return pIntervals->GetFirstPrestressReleaseInterval(GetGirderKey());
}

IntervalIndexType CIntervalGirderGraphControllerBase::GetLastInterval()
{
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   return nIntervals - 1;
}

void CIntervalGirderGraphControllerBase::FillIntervalCtrl()
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
      strInterval.Format(_T("Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx).c_str());
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
void CIntervalGirderGraphControllerBase::AssertValid() const
{
	CGirderGraphControllerBase::AssertValid();
}

void CIntervalGirderGraphControllerBase::Dump(CDumpContext& dc) const
{
	CGirderGraphControllerBase::Dump(dc);
}
#endif //_DEBUG


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CMultiIntervalGirderGraphControllerBase,CGirderGraphControllerBase)

CMultiIntervalGirderGraphControllerBase::CMultiIntervalGirderGraphControllerBase(bool bAllGroups):
CGirderGraphControllerBase(bAllGroups)
{
}

BEGIN_MESSAGE_MAP(CMultiIntervalGirderGraphControllerBase, CGirderGraphControllerBase)
	//{{AFX_MSG_MAP(CMultiIntervalGirderGraphControllerBase)
   ON_LBN_SELCHANGE( IDC_INTERVALS, OnIntervalsChanged )
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CMultiIntervalGirderGraphControllerBase::OnInitDialog()
{
   CGirderGraphControllerBase::OnInitDialog();
   FillIntervalCtrl();
   return TRUE;
}

void CMultiIntervalGirderGraphControllerBase::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   CGirderGraphControllerBase::OnUpdate(pSender,lHint,pHint);
   if ( lHint == HINT_BRIDGECHANGED || lHint == HINT_SPECCHANGED)
   {
      FillIntervalCtrl();
   }
}

void CMultiIntervalGirderGraphControllerBase::SelectInterval(IntervalIndexType intervalIdx)
{
   CListBox* plbIntervals = (CListBox*)GetDlgItem(IDC_INTERVALS);
   plbIntervals->SetCurSel(-1); // clears the current selection

   IntervalIndexType firstIntervalIdx = GetFirstInterval(); // this interval is at position 0
   int idx = (int)(intervalIdx - firstIntervalIdx);
   plbIntervals->SetSel(idx);
   UpdateGraph();
}

void CMultiIntervalGirderGraphControllerBase::SelectIntervals(const std::vector<IntervalIndexType>& vIntervals)
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

std::vector<IntervalIndexType> CMultiIntervalGirderGraphControllerBase::GetSelectedIntervals() const
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

IndexType CMultiIntervalGirderGraphControllerBase::GetGraphCount() const
{
   CListBox* plbIntervals = (CListBox*)GetDlgItem(IDC_INTERVALS);
   return plbIntervals->GetSelCount();
}

void CMultiIntervalGirderGraphControllerBase::OnIntervalsChanged()
{
   UpdateGraph();
}

void CMultiIntervalGirderGraphControllerBase::FillIntervalCtrl()
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

   CGirderKey girderKey(GetGirderKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType firstIntervalIdx = GetFirstInterval();
   IntervalIndexType lastIntervalIdx  = GetLastInterval();
   for ( IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++ )
   {
      CString str;
      str.Format(_T("%d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx).c_str());
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

IntervalIndexType CMultiIntervalGirderGraphControllerBase::GetFirstInterval()
{
   GET_IFACE(IIntervals, pIntervals);
   return pIntervals->GetFirstPrestressReleaseInterval(GetGirderKey());
}

IntervalIndexType CMultiIntervalGirderGraphControllerBase::GetLastInterval()
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   return nIntervals-1;
}

#ifdef _DEBUG
void CMultiIntervalGirderGraphControllerBase::AssertValid() const
{
	CGirderGraphControllerBase::AssertValid();
}

void CMultiIntervalGirderGraphControllerBase::Dump(CDumpContext& dc) const
{
	CGirderGraphControllerBase::Dump(dc);
}
#endif //_DEBUG
