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
#include "ConcretePropertyGraphController.h"
#include <Graphing\ConcretePropertyGraphBuilder.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>
#include <IFace\PointOfInterest.h>
#include <EAF\EAFDisplayUnits.h>

#include <EAF\EAFGraphBuilderBase.h>
#include <EAF\EAFGraphView.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\TemporarySupportData.h>
#include <PgsExt\ClosureJointData.h>

#include <PGSuperUnits.h>

bool IsTSIndex(IndexType key) { return MAX_INDEX/2 <= key ? true : false; }
SupportIndexType EncodeTSIndex(SupportIndexType tsIdx) { return MAX_INDEX-tsIdx; }
SupportIndexType DecodeTSIndex(SupportIndexType tsIdx) { return MAX_INDEX-tsIdx; }

IMPLEMENT_DYNCREATE(CConcretePropertyGraphController,CEAFGraphControlWindow)

CConcretePropertyGraphController::CConcretePropertyGraphController()
{
}

BEGIN_MESSAGE_MAP(CConcretePropertyGraphController, CEAFGraphControlWindow)
	//{{AFX_MSG_MAP(CConcretePropertyGraphController)
   ON_BN_CLICKED(IDC_PRECAST_SEGMENT,OnGraphElement)
   ON_BN_CLICKED(IDC_CLOSURE_JOINT,OnGraphElement)
   ON_BN_CLICKED(IDC_DECK,OnGraphElement)
   ON_BN_CLICKED(IDC_FC,OnGraphType)
   ON_BN_CLICKED(IDC_EC,OnGraphType)
   ON_BN_CLICKED(IDC_SH,OnGraphType)
   ON_BN_CLICKED(IDC_CR,OnGraphType)
   ON_CBN_SELCHANGE(IDC_GROUP,OnGroupChanged)
   ON_CBN_SELCHANGE(IDC_GIRDER,OnGirderChanged)
   ON_CBN_SELCHANGE(IDC_SEGMENT,OnSegmentChanged)
   ON_CBN_SELCHANGE(IDC_CLOSURE,OnClosureChanged)
   ON_BN_CLICKED(IDC_TIME_LOG,OnXAxis)
   ON_BN_CLICKED(IDC_TIME_LINEAR,OnXAxis)
   ON_BN_CLICKED(IDC_AGE_LOG,OnXAxis)
   ON_BN_CLICKED(IDC_AGE_LINEAR,OnXAxis)
   ON_BN_CLICKED(IDC_INTERVALS,OnXAxis)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CConcretePropertyGraphController::OnInitDialog()
{
   CEAFGraphControlWindow::OnInitDialog();

   EAFGetBroker(&m_pBroker);

   CheckRadioButton(IDC_PRECAST_SEGMENT,IDC_DECK,IDC_PRECAST_SEGMENT);
   CheckRadioButton(IDC_FC,IDC_CR,IDC_FC);

   FillGroupControl();
   FillGirderControl();
   FillSegmentControl();

   FillClosureControl();

   UpdateElementControls();

   GET_IFACE(ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();

   if ( selection.Type == CSelection::Girder || selection.Type == CSelection::Segment )
   {
      CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
      pcbGroup->SetCurSel((int)selection.GroupIdx);

      CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
      pcbGirder->SetCurSel((int)selection.GirderIdx);
   }

   GET_IFACE(ILossParameters,pLossParams);
   if ( pLossParams->GetLossMethod() != pgsTypes::TIME_STEP )
   {
      GetDlgItem(IDC_SH)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_CR)->ShowWindow(SW_HIDE);
   }


   return TRUE;
}

int CConcretePropertyGraphController::GetGraphElement()
{
   int id = GetCheckedRadioButton(IDC_PRECAST_SEGMENT,IDC_DECK);
   switch(id)
   {
   case IDC_PRECAST_SEGMENT:
      return GRAPH_ELEMENT_SEGMENT;

   case IDC_CLOSURE_JOINT:
      return GRAPH_ELEMENT_CLOSURE;

   case IDC_DECK:
      return GRAPH_ELEMENT_DECK;

   default:
      ATLASSERT(false);
      return GRAPH_ELEMENT_DECK;
   }
}

int CConcretePropertyGraphController::GetGraphType()
{
   int id = GetCheckedRadioButton(IDC_FC,IDC_CR);
   switch(id)
   {
   case IDC_FC:
      return GRAPH_TYPE_FC;

   case IDC_EC:
      return GRAPH_TYPE_EC;

   case IDC_SH:
      return GRAPH_TYPE_SH;

   case IDC_CR:
      return GRAPH_TYPE_CR;

   default:
      ATLASSERT(false);
      return GRAPH_TYPE_FC;
   }
}

void CConcretePropertyGraphController::UpdateGraph()
{
   ((CConcretePropertyGraphBuilder*)GetGraphBuilder())->InvalidateGraph();
   ((CConcretePropertyGraphBuilder*)GetGraphBuilder())->Update();
}

void CConcretePropertyGraphController::OnGraphElement()
{
   UpdateElementControls();
   UpdateGraph();
}

void CConcretePropertyGraphController::OnGraphType()
{
   UpdateGraph();
}

void CConcretePropertyGraphController::UpdateElementControls()
{
   int id = GetCheckedRadioButton(IDC_PRECAST_SEGMENT,IDC_DECK);
   BOOL bEnableSegment(TRUE);
   BOOL bEnableClosure(TRUE);
   switch(id)
   {
   case IDC_PRECAST_SEGMENT:
      bEnableSegment = TRUE;
      bEnableClosure = FALSE;
      break;

   case IDC_CLOSURE_JOINT:
      bEnableSegment = FALSE;
      bEnableClosure = TRUE;
      break;

   case IDC_DECK:
      bEnableSegment = FALSE;
      bEnableClosure = FALSE;
      break;

   default:
      ATLASSERT(false);
   }

   GetDlgItem(IDC_GROUP)->EnableWindow(bEnableSegment);
   GetDlgItem(IDC_GIRDER)->EnableWindow(bEnableSegment);
   GetDlgItem(IDC_SEGMENT)->EnableWindow(bEnableSegment);

   GetDlgItem(IDC_CLOSURE)->EnableWindow(bEnableClosure);
}

void CConcretePropertyGraphController::FillGroupControl()
{
   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
   int curSel = pcbGroup->GetCurSel();
   pcbGroup->ResetContent();

   GET_IFACE(IDocumentType,pDocType);
   CString strGroupLabel(pDocType->IsPGSuperDocument() ? _T("Span") : _T("Group"));

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridge = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CString strGroup;
      strGroup.Format(_T("%s %d"),strGroupLabel,LABEL_GROUP(grpIdx));
      pcbGroup->AddString(strGroup);
   }

   if ( pcbGroup->SetCurSel(curSel) == CB_ERR )
   {
      pcbGroup->SetCurSel(0);
   }
}

void CConcretePropertyGraphController::FillGirderControl()
{
   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   int curSel = pcbGirder->GetCurSel();
   pcbGirder->ResetContent();

   GroupIndexType grpIdx = GetGroupIndex();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(grpIdx);
   GirderIndexType nGirders = pGroup->GetGirderCount();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      CString strGirder;
      strGirder.Format(_T("Girder %s"),LABEL_GIRDER(gdrIdx));
      pcbGirder->AddString(strGirder);
   }

   if ( curSel == CB_ERR )
   {
      pcbGirder->SetCurSel(0);
   }
   else
   {
      curSel = Min(curSel,(int)(nGirders-1));
      pcbGirder->SetCurSel(curSel);
   }
}

void CConcretePropertyGraphController::FillSegmentControl()
{
   CComboBox* pcbSegment = (CComboBox*)GetDlgItem(IDC_SEGMENT);
   int curSel = pcbSegment->GetCurSel();
   pcbSegment->ResetContent();

   GroupIndexType grpIdx = GetGroupIndex();
   GirderIndexType gdrIdx = GetGirderIndex();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(grpIdx);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CString strSegment;
      strSegment.Format(_T("Segment %d"),LABEL_SEGMENT(segIdx));
      pcbSegment->AddString(strSegment);
   }

   if ( curSel == CB_ERR )
   {
      pcbSegment->SetCurSel(0);
   }
   else
   {
      curSel = Min(curSel,(int)(nSegments-1));
      pcbSegment->SetCurSel(curSel);
   }

   GET_IFACE(IDocumentType,pDocType);
   if ( pDocType->IsPGSuperDocument() )
   {
      pcbSegment->ShowWindow(SW_HIDE);
   }
}

void CConcretePropertyGraphController::FillClosureControl()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_CLOSURE);
   
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSplicedGirderData* pGirder = pBridgeDesc->GetGirderGroup(GroupIndexType(0))->GetGirder(0);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
   const CClosureJointData* pClosure = pSegment->GetEndClosure();

   if ( pClosure == NULL )
   {
      // there aren't any closure joints... disable the combo box and the radio button
      pCB->EnableWindow(FALSE);
      GetDlgItem(IDC_CLOSURE_JOINT)->EnableWindow(FALSE);
   }
   else
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      while ( pClosure )
      {
         if ( pClosure->GetPier() )
         {
            PierIDType pierID = pClosure->GetPier()->GetID();
            PierIndexType pierIdx = pClosure->GetPier()->GetIndex();
            CString label(GetLabel(pClosure->GetPier(),pDisplayUnits));
            pCB->SetItemData(pCB->AddString(label),pierIdx);
         }
         else
         {
            ATLASSERT(pClosure->GetTemporarySupport());
            SupportIDType tsID = pClosure->GetTemporarySupport()->GetID();
            SupportIndexType tsIdx = pClosure->GetTemporarySupport()->GetIndex();
            CString label( GetLabel(pClosure->GetTemporarySupport(),pDisplayUnits) );
            pCB->SetItemData(pCB->AddString(label), EncodeTSIndex(tsIdx) );
         }

         if ( pClosure->GetRightSegment() )
         {
            pClosure = pClosure->GetRightSegment()->GetEndClosure();
         }
         else
         {
            pClosure = NULL;
         }
      }

      pCB->SetCurSel(0);
   }
}

void CConcretePropertyGraphController::OnGroupChanged()
{
   FillGirderControl();
   FillSegmentControl();
   UpdateGraph();
}

void CConcretePropertyGraphController::OnGirderChanged()
{
   FillSegmentControl();
   UpdateGraph();
}

void CConcretePropertyGraphController::OnSegmentChanged()
{
   UpdateGraph();
}

void CConcretePropertyGraphController::OnClosureChanged()
{
   UpdateGraph();
}

void CConcretePropertyGraphController::OnXAxis()
{
   UpdateGraph();
}

GroupIndexType CConcretePropertyGraphController::GetGroupIndex()
{
   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
   return (GroupIndexType)pcbGroup->GetCurSel();
}

GirderIndexType CConcretePropertyGraphController::GetGirderIndex()
{
   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   return (GirderIndexType)pcbGirder->GetCurSel();
}

SegmentIndexType CConcretePropertyGraphController::GetSegmentIndex()
{
   CComboBox* pcbSegment = (CComboBox*)GetDlgItem(IDC_SEGMENT);
   return (SegmentIndexType)pcbSegment->GetCurSel();
}

CSegmentKey CConcretePropertyGraphController::GetSegmentKey()
{
   return CSegmentKey(GetGroupIndex(),GetGirderIndex(),GetSegmentIndex());
}

CClosureKey CConcretePropertyGraphController::GetClosureKey()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_CLOSURE);
   int curSel = pCB->GetCurSel();
   if ( curSel == CB_ERR )
   {
      return CClosureKey(GetGroupIndex(),GetGirderIndex(),INVALID_INDEX);
   }

   IndexType idx = (IndexType)pCB->GetItemData(curSel);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( IsTSIndex(idx) )
   {
      SupportIndexType tsIdx = DecodeTSIndex(idx);
      const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);
      const CClosureJointData* pClosure = pTS->GetClosureJoint(0);
      return pClosure->GetClosureKey();
   }
   else
   {
      PierIndexType pierIdx = (PierIndexType)idx;
      const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
      const CClosureJointData* pClosure = pPier->GetClosureJoint(0);
      return pClosure->GetClosureKey();
   }
}

int CConcretePropertyGraphController::GetXAxisType()
{
   int id = GetCheckedRadioButton(IDC_TIME_LINEAR,IDC_INTERVALS);
   int axisType;
   switch(id)
   {
   case IDC_TIME_LINEAR:
      axisType = X_AXIS_TIME_LINEAR;
      break;

   case IDC_TIME_LOG:
      axisType = X_AXIS_TIME_LOG;
      break;

   case IDC_AGE_LINEAR:
      axisType = X_AXIS_AGE_LINEAR;
      break;

   case IDC_AGE_LOG:
      axisType = X_AXIS_AGE_LOG;
      break;

   case IDC_INTERVALS:
      axisType = X_AXIS_INTERVAL;
      break;

   default:
      ATLASSERT(false);
      axisType = X_AXIS_TIME_LOG;
   }
   return axisType;
}

#ifdef _DEBUG
void CConcretePropertyGraphController::AssertValid() const
{
	CEAFGraphControlWindow::AssertValid();
}

void CConcretePropertyGraphController::Dump(CDumpContext& dc) const
{
	CEAFGraphControlWindow::Dump(dc);
}
#endif //_DEBUG
