///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include <Graphs/ConcretePropertyGraphBuilder.h>
#include <Graphs/ExportGraphXYTool.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>
#include <IFace\PointOfInterest.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include <EAF\EAFGraphBuilderBase.h>
#include <EAF\EAFGraphView.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\TemporarySupportData.h>
#include <PgsExt\ClosureJointData.h>

#include <PGSuperUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool IsTSIndex(IndexType key) { return MAX_INDEX/2 <= key ? true : false; }
SupportIndexType EncodeTSIndex(SupportIndexType tsIdx) { return MAX_INDEX-tsIdx; }
SupportIndexType DecodeTSIndex(SupportIndexType tsIdx) { return MAX_INDEX-tsIdx; }

IMPLEMENT_DYNCREATE(CConcretePropertyGraphController,CEAFGraphControlWindow)

CConcretePropertyGraphController::CConcretePropertyGraphController() :
   m_SegmentKey(0,0,0),
   m_ClosureKey(0,0,0),
   m_DeckCastingRegionIdx(0)
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
   ON_CBN_SELCHANGE(IDC_DECK_CASTING_REGION,OnDeckCastingRegionChanged)
   ON_BN_CLICKED(IDC_TIME_LOG,OnXAxis)
   ON_BN_CLICKED(IDC_TIME_LINEAR,OnXAxis)
   ON_BN_CLICKED(IDC_AGE_LOG,OnXAxis)
   ON_BN_CLICKED(IDC_AGE_LINEAR,OnXAxis)
   ON_BN_CLICKED(IDC_INTERVALS,OnXAxis)
   ON_BN_CLICKED(IDC_GRID, OnShowGrid)
   ON_BN_CLICKED(IDC_EXPORT_GRAPH_BTN,OnGraphExportClicked)
   ON_UPDATE_COMMAND_UI(IDC_EXPORT_GRAPH_BTN,OnCommandUIGraphExport)
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

   FillDeckCastingRegionControl();

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
   if ( pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      GetDlgItem(IDC_SH)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_CR)->ShowWindow(SW_HIDE);
   }

   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( IsNonstructuralDeck(pBridgeDesc->GetDeckDescription()->GetDeckType()) )
   {
      GetDlgItem(IDC_DECK)->EnableWindow(FALSE);
      GetDlgItem(IDC_DECK_CASTING_REGION)->EnableWindow(FALSE);
   }

   return TRUE;
}

void CConcretePropertyGraphController::SetGraphElement(int element)
{
   UINT nIDC;
   switch (element)
   {
   case GRAPH_ELEMENT_SEGMENT: nIDC = IDC_PRECAST_SEGMENT; break;
   case GRAPH_ELEMENT_CLOSURE: nIDC = IDC_CLOSURE_JOINT; break;
   case GRAPH_ELEMENT_DECK: nIDC = IDC_DECK; break;
   }
   CheckRadioButton(IDC_PRECAST_SEGMENT, IDC_DECK, nIDC);
   UpdateGraph();
}

int CConcretePropertyGraphController::GetGraphElement() const
{
   int nIDC = GetCheckedRadioButton(IDC_PRECAST_SEGMENT,IDC_DECK);
   ATLASSERT(nIDC != 0); // 0 means nothing is selected
   switch(nIDC)
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

void CConcretePropertyGraphController::SetGraphType(int graphType)
{
   UINT nIDC;
   switch (graphType)
   {
   case GRAPH_TYPE_FC: nIDC = IDC_FC; break;
   case GRAPH_TYPE_EC: nIDC = IDC_EC; break;
   case GRAPH_TYPE_SH: nIDC = IDC_SH; break;
   case GRAPH_TYPE_CR: nIDC = IDC_CR; break;
   }
   CheckRadioButton(IDC_FC, IDC_CR, nIDC);
   UpdateGraph();
}

int CConcretePropertyGraphController::GetGraphType() const
{
   int nIDC = GetCheckedRadioButton(IDC_FC,IDC_CR);
   ATLASSERT(nIDC != 0); // 0 means nothing is selected
   switch(nIDC)
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

void CConcretePropertyGraphController::SetSegment(const CSegmentKey& segmentKey)
{
   m_SegmentKey = segmentKey;
   SetGraphElement(GRAPH_ELEMENT_SEGMENT);
   FillGroupControl();
   FillGirderControl();
   FillSegmentControl();

   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   CComboBox* pcbSegment = (CComboBox*)GetDlgItem(IDC_SEGMENT);

   pcbGroup->SetCurSel((int)m_SegmentKey.groupIndex);
   pcbGirder->SetCurSel((int)m_SegmentKey.girderIndex);
   pcbSegment->SetCurSel((int)m_SegmentKey.segmentIndex);

   UpdateGraph();
}

const CSegmentKey& CConcretePropertyGraphController::GetSegment() const
{
   return m_SegmentKey;
}

void CConcretePropertyGraphController::SetClosureJoint(const CClosureKey& closureKey)
{
   m_ClosureKey = closureKey;
   SetGraphElement(GRAPH_ELEMENT_CLOSURE);
   UpdateGraph();
}

const CClosureKey& CConcretePropertyGraphController::GetClosureJoint() const
{
   return m_ClosureKey;
}

void CConcretePropertyGraphController::SetDeckCastingRegion(IndexType castingRegionIdx)
{
   m_DeckCastingRegionIdx = castingRegionIdx;
   SetGraphElement(GRAPH_ELEMENT_DECK);
   UpdateGraph();
}

IndexType CConcretePropertyGraphController::GetDeckCastingRegion() const
{
   return m_DeckCastingRegionIdx;
}

void CConcretePropertyGraphController::OnShowGrid()
{
   // toggle the grid setting
   ((CConcretePropertyGraphBuilder*)GetGraphBuilder())->ShowGrid(ShowGrid());
}

bool CConcretePropertyGraphController::ShowGrid() const
{
   CButton* pBtn = (CButton*)GetDlgItem(IDC_GRID);
   return (pBtn->GetCheck() == BST_CHECKED ? true : false);
}

void CConcretePropertyGraphController::ShowGrid(bool bShowGrid)
{
   if (bShowGrid != ShowGrid())
   {
      CButton* pBtn = (CButton*)GetDlgItem(IDC_GRID);
      pBtn->SetCheck(bShowGrid ? BST_CHECKED : BST_UNCHECKED);
      ((CConcretePropertyGraphBuilder*)GetGraphBuilder())->ShowGrid(bShowGrid);
   }
}

void CConcretePropertyGraphController::OnGraphExportClicked()
{
   // Build default file name
   CString strProjectFileNameNoPath = CExportGraphXYTool::GetTruncatedFileName();

   CSegmentKey segmentKey(GetSegment());
   CString girderName = SEGMENT_LABEL(segmentKey);

   int graphType = GetGraphType();
   CString graphName;
   switch (graphType)
   {
   case GRAPH_TYPE_FC: graphName = _T("FC"); break;
   case GRAPH_TYPE_EC: graphName = _T("EC"); break;
   case GRAPH_TYPE_SH: graphName = _T("SH"); break;
   case GRAPH_TYPE_CR: graphName = _T("CR"); break;
   }


   CString strDefaultFileName = strProjectFileNameNoPath + _T("_") + girderName + _T("_") + graphName;
   strDefaultFileName.Replace(' ','_'); // prefer not to have spaces or ,'s in file names
   strDefaultFileName.Replace(',','_');

   ((CConcretePropertyGraphBuilder*)GetGraphBuilder())->ExportGraphData(strDefaultFileName);
}

// this has to be implemented otherwise button will not be enabled.
void CConcretePropertyGraphController::OnCommandUIGraphExport(CCmdUI* pCmdUI)
{
   pCmdUI->Enable(TRUE);
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
   int nIDC = GetCheckedRadioButton(IDC_PRECAST_SEGMENT,IDC_DECK);
   ATLASSERT(nIDC != 0); // 0 means nothing is selected
   BOOL bEnableSegment(TRUE);
   BOOL bEnableClosure(TRUE);
   BOOL bEnableDeckCastingRegion(TRUE);
   switch(nIDC)
   {
   case IDC_PRECAST_SEGMENT:
      bEnableSegment = TRUE;
      bEnableClosure = FALSE;
      bEnableDeckCastingRegion = FALSE;
      break;

   case IDC_CLOSURE_JOINT:
      bEnableSegment = FALSE;
      bEnableClosure = TRUE;
      bEnableDeckCastingRegion = FALSE;
      break;

   case IDC_DECK:
      bEnableSegment = FALSE;
      bEnableClosure = FALSE;
      bEnableDeckCastingRegion = TRUE;
      break;

   default:
      ATLASSERT(false);
   }

   GetDlgItem(IDC_GROUP)->EnableWindow(bEnableSegment);
   GetDlgItem(IDC_GIRDER)->EnableWindow(bEnableSegment);
   GetDlgItem(IDC_SEGMENT)->EnableWindow(bEnableSegment);

   GetDlgItem(IDC_CLOSURE)->EnableWindow(bEnableClosure);

   GetDlgItem(IDC_DECK_CASTING_REGION)->EnableWindow(bEnableDeckCastingRegion);
}

void CConcretePropertyGraphController::FillGroupControl()
{
   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
   int curSel = pcbGroup->GetCurSel();
   pcbGroup->ResetContent();

   GET_IFACE(IDocumentType,pDocType);
   bool isPGSuper = pDocType->IsPGSuperDocument();
   CString strGroupLabel( isPGSuper ? _T("Span") : _T("Group"));

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridge = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CString strGroup;
      if (isPGSuper)
      {
         strGroup.Format(_T("%s %s"), strGroupLabel, LABEL_SPAN(grpIdx));
      }
      else
      {
         strGroup.Format(_T("%s %d"), strGroupLabel, LABEL_GROUP(grpIdx));
      }

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

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(m_SegmentKey.groupIndex);
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
      m_SegmentKey.girderIndex = 0;
   }
   else
   {
      curSel = Min(curSel,(int)(nGirders-1));
      m_SegmentKey.girderIndex = curSel;
      pcbGirder->SetCurSel(curSel);
   }
}

void CConcretePropertyGraphController::FillSegmentControl()
{
   CComboBox* pcbSegment = (CComboBox*)GetDlgItem(IDC_SEGMENT);
   int curSel = pcbSegment->GetCurSel();
   pcbSegment->ResetContent();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(m_SegmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(m_SegmentKey.girderIndex);
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
      m_SegmentKey.segmentIndex = 0;
   }
   else
   {
      curSel = Min(curSel,(int)(nSegments-1));
      m_SegmentKey.segmentIndex = curSel;
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
   const CClosureJointData* pClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);

   if ( pClosure == nullptr )
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
            pClosure = pClosure->GetRightSegment()->GetClosureJoint(pgsTypes::metEnd);
         }
         else
         {
            pClosure = nullptr;
         }
      }

      pCB->SetCurSel(0);
   }
}

void CConcretePropertyGraphController::FillDeckCastingRegionControl()
{
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   if (pIBridgeDesc->GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone)
   {
      CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_DECK_CASTING_REGION);
      pCB->ResetContent();
      EventIndexType castDeckEventIdx = pIBridgeDesc->GetCastDeckEventIndex();
      const auto* pTimelineEvent = pIBridgeDesc->GetEventByIndex(castDeckEventIdx);
      const auto& castDeckActivity = pTimelineEvent->GetCastDeckActivity();
      ATLASSERT(castDeckActivity.IsEnabled());
      IndexType nCastingRegions = castDeckActivity.GetCastingRegionCount();
      for (IndexType regionIdx = 0; regionIdx < nCastingRegions; regionIdx++)
      {
         CString strRegion;
         strRegion.Format(_T("Region %d"), LABEL_INDEX(regionIdx));
         pCB->AddString(strRegion);
      }
      pCB->SetCurSel(0);
   }
}

void CConcretePropertyGraphController::OnGroupChanged()
{
   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
   m_SegmentKey.groupIndex = (GroupIndexType)pcbGroup->GetCurSel();

   FillGirderControl();
   FillSegmentControl();
   UpdateGraph();
}

void CConcretePropertyGraphController::OnGirderChanged()
{
   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   m_SegmentKey.girderIndex = (GirderIndexType)pcbGirder->GetCurSel();

   FillSegmentControl();
   UpdateGraph();
}

void CConcretePropertyGraphController::OnSegmentChanged()
{
   CComboBox* pcbSegment = (CComboBox*)GetDlgItem(IDC_SEGMENT);
   m_SegmentKey.segmentIndex = (GirderIndexType)pcbSegment->GetCurSel();

   UpdateGraph();
}

void CConcretePropertyGraphController::OnClosureChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_CLOSURE);
   int curSel = pCB->GetCurSel();
   
   IndexType idx = (IndexType)pCB->GetItemData(curSel);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( IsTSIndex(idx) )
   {
      SupportIndexType tsIdx = DecodeTSIndex(idx);
      const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);
      const CClosureJointData* pClosure = pTS->GetClosureJoint(0);
      m_ClosureKey = pClosure->GetClosureKey();
   }
   else
   {
      PierIndexType pierIdx = (PierIndexType)idx;
      const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
      const CClosureJointData* pClosure = pPier->GetClosureJoint(0);
      m_ClosureKey = pClosure->GetClosureKey();
   }

   UpdateGraph();
}

void CConcretePropertyGraphController::OnDeckCastingRegionChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_DECK_CASTING_REGION);
   int curSel = pCB->GetCurSel();
   ATLASSERT(curSel != CB_ERR);
   m_DeckCastingRegionIdx = (IndexType)curSel;
   UpdateGraph();
}

void CConcretePropertyGraphController::OnXAxis()
{
   UpdateGraph();
}

void CConcretePropertyGraphController::SetXAxisType(int type)
{
   UINT nIDC;
   switch (type)
   {
   case X_AXIS_TIME_LINEAR: nIDC = IDC_TIME_LINEAR; break;
   case X_AXIS_TIME_LOG:    nIDC = IDC_TIME_LOG;    break;
   case X_AXIS_AGE_LINEAR:  nIDC = IDC_AGE_LINEAR;  break;
   case X_AXIS_AGE_LOG:     nIDC = IDC_AGE_LOG;     break;
   case X_AXIS_INTERVAL:    nIDC = IDC_INTERVALS;   break;
   default:
      ATLASSERT(false);
      nIDC = IDC_TIME_LOG;
   }

   CheckRadioButton(IDC_TIME_LINEAR, IDC_INTERVALS, nIDC);
   UpdateGraph();
}

int CConcretePropertyGraphController::GetXAxisType() const
{
   int nIDC = GetCheckedRadioButton(IDC_TIME_LINEAR,IDC_INTERVALS);
   ATLASSERT(nIDC != 0); // 0 means nothing is selected
   int axisType;
   switch(nIDC)
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
