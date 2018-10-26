///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include "StabilityGraphController.h"
#include <Graphing\StabilityGraphBuilder.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Selection.h>
#include <IFace\Bridge.h>
#include <IFace\GirderHandlingSpecCriteria.h>

IMPLEMENT_DYNCREATE(CStabilityGraphController,CEAFGraphControlWindow)

CStabilityGraphController::CStabilityGraphController()
{
   EAFGetBroker(&m_pBroker);
}

BEGIN_MESSAGE_MAP(CStabilityGraphController, CEAFGraphControlWindow)
	//{{AFX_MSG_MAP(CStabilityGraphController)
   ON_CBN_SELCHANGE( IDC_GROUP, OnGroupChanged )
   ON_CBN_SELCHANGE( IDC_GIRDER, OnGirderChanged )
   ON_CBN_SELCHANGE( IDC_SEGMENT, OnSegmentChanged )
   ON_CBN_SELCHANGE( IDC_EVENT, OnGraphTypeChanged )
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CStabilityGraphController::CreateControls(CWnd* pParent,UINT nID)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IDocumentType,pDocType);
   if ( pDocType->IsPGSuperDocument() )
   {
      return Create(pParent,IDD_PGSUPER_STABILITY_BAR, CBRS_TOP, nID);
   }
   else
   {
      return Create(pParent,IDD_PGSPLICE_STABILITY_BAR, CBRS_TOP, nID);
   }
}

BOOL CStabilityGraphController::OnInitDialog()
{
   CEAFGraphControlWindow::OnInitDialog();

   CComboBox* pcbStage = (CComboBox*)GetDlgItem(IDC_EVENT);
   pcbStage->AddString(_T("Lifting"));
   pcbStage->AddString(_T("Hauling"));
   pcbStage->SetCurSel(0);
   m_GraphType = GT_LIFTING;

   GET_IFACE(ISegmentHaulingSpecCriteria,pSpec);
   if ( pSpec->GetHaulingAnalysisMethod() == pgsTypes::hmKDOT )
   {
      // Transportation stability graph isn't applicable for KDOT mode
      // so disable and hide the mode selector
      pcbStage->EnableWindow(FALSE);
      pcbStage->ShowWindow(SW_HIDE);
   }


   FillGroupCtrl();

   // Set initial value based on the current selection
   GET_IFACE(ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();

   CComboBox* pcbGroup  = (CComboBox*)GetDlgItem(IDC_GROUP);
   pcbGroup->SetCurSel( selection.GroupIdx == ALL_GROUPS ? 0 : (int)selection.GroupIdx);
   m_SegmentKey.groupIndex  = (GroupIndexType)(pcbGroup->GetCurSel());

   FillGirderCtrl();

   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   pcbGirder->SetCurSel(selection.GirderIdx == ALL_GIRDERS ? 0 : (int)selection.GirderIdx);
   m_SegmentKey.girderIndex = (GirderIndexType)(pcbGirder->GetCurSel());

   GET_IFACE(IDocumentType,pDocType);
   if ( pDocType->IsPGSuperDocument() )
   {
      m_SegmentKey.segmentIndex = 0;
   }
   else
   {
      FillSegmentCtrl();
      CComboBox* pcbSegment = (CComboBox*)GetDlgItem(IDC_SEGMENT);
      pcbSegment->SetCurSel(selection.SegmentIdx == ALL_SEGMENTS ? 0 : (int)selection.SegmentIdx);
      m_SegmentKey.segmentIndex = (SegmentIndexType)(pcbSegment->GetCurSel());
   }

   return TRUE;
}

void CStabilityGraphController::FillGroupCtrl()
{
   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);

   GET_IFACE(IDocumentType,pDocType);
   CString strGroupLabel(pDocType->IsPGSuperDocument() ? _T("Span") : _T("Group"));
   GET_IFACE(IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CString strLabel;
      strLabel.Format(_T("%s %d"),strGroupLabel,LABEL_GROUP(grpIdx));
      pcbGroup->AddString(strLabel);
   }
}

void CStabilityGraphController::FillGirderCtrl()
{
   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   int curSel = pcbGirder->GetCurSel();

   GET_IFACE(IBridge,pBridge);
   GirderIndexType nGirders = pBridge->GetGirderCount(m_SegmentKey.groupIndex);
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      CString strLabel;
      strLabel.Format(_T("Girder %s"),LABEL_GIRDER(gdrIdx));
      pcbGirder->AddString(strLabel);
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

void CStabilityGraphController::FillSegmentCtrl()
{
   CComboBox* pcbSegment = (CComboBox*)GetDlgItem(IDC_SEGMENT);
   int curSel = pcbSegment->GetCurSel();

   GET_IFACE(IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(m_SegmentKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CString strLabel;
      strLabel.Format(_T("Segment %d"),LABEL_SEGMENT(segIdx));
      pcbSegment->AddString(strLabel);
   }

   if ( curSel != CB_ERR && curSel < pcbSegment->GetCount() )
   {
      pcbSegment->SetCurSel(curSel);
   }
   else
   {
      pcbSegment->SetCurSel(0);
   }
}

void CStabilityGraphController::OnGroupChanged()
{
   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
   GroupIndexType grpIdx = (GroupIndexType)(pcbGroup->GetCurSel());
   if ( grpIdx != m_SegmentKey.groupIndex )
   {
      m_SegmentKey.groupIndex = grpIdx;
      FillGirderCtrl();
      OnGirderChanged();
      UpdateGraph();
   }
}

void CStabilityGraphController::OnGirderChanged()
{
   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   GirderIndexType gdrIdx = (GirderIndexType)(pcbGirder->GetCurSel());
   if ( gdrIdx != m_SegmentKey.girderIndex )
   {
      m_SegmentKey.girderIndex = gdrIdx;

      GET_IFACE(IDocumentType,pDocType);
      if ( pDocType->IsPGSpliceDocument() )
      {
         FillSegmentCtrl();
         OnSegmentChanged();
      }
      UpdateGraph();
   }
}

void CStabilityGraphController::OnSegmentChanged()
{
   CComboBox* pcbSegment = (CComboBox*)GetDlgItem(IDC_SEGMENT);
   SegmentIndexType segIdx = (SegmentIndexType)(pcbSegment->GetCurSel());
   if ( segIdx != m_SegmentKey.segmentIndex )
   {
      m_SegmentKey.segmentIndex = segIdx;
      UpdateGraph();
   }
}

void CStabilityGraphController::OnGraphTypeChanged()
{
   CComboBox* pcbStage = (CComboBox*)GetDlgItem(IDC_EVENT);
   m_GraphType = pcbStage->GetCurSel();
   UpdateGraph();
}

int CStabilityGraphController::GetGraphType()
{
   return m_GraphType;
}

const CSegmentKey& CStabilityGraphController::GetSegmentKey()
{
   return m_SegmentKey;
}

void CStabilityGraphController::UpdateGraph()
{
   ((CStabilityGraphBuilder*)GetGraphBuilder())->InvalidateGraph();
   ((CStabilityGraphBuilder*)GetGraphBuilder())->Update();
}

#ifdef _DEBUG
void CStabilityGraphController::AssertValid() const
{
	CEAFGraphControlWindow::AssertValid();
}

void CStabilityGraphController::Dump(CDumpContext& dc) const
{
	CEAFGraphControlWindow::Dump(dc);
}
#endif //_DEBUG
