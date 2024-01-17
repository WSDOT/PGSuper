///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include "EffectivePrestressGraphController.h"
#include <Graphs/EffectivePrestressGraphBuilder.h>
#include <Graphs/ExportGraphXYTool.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>

#include <Hints.h>

#include <EAF\EAFGraphBuilderBase.h>
#include <EAF\EAFGraphView.h>
#include <EAF\EAFDocument.h>

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CEffectivePrestressGraphController,CMultiIntervalGirderGraphControllerBase)

CEffectivePrestressGraphController::CEffectivePrestressGraphController():
CMultiIntervalGirderGraphControllerBase(false/*don't use ALL_GROUPS*/),
m_DuctType(CEffectivePrestressGraphBuilder::Segment),m_DuctIdx(INVALID_INDEX)
{
}

BEGIN_MESSAGE_MAP(CEffectivePrestressGraphController, CMultiIntervalGirderGraphControllerBase)
   //{{AFX_MSG_MAP(CEffectivePrestressGraphController)
   ON_CBN_SELCHANGE(IDC_DUCT, OnDuctChanged)
   ON_CONTROL_RANGE(BN_CLICKED, IDC_PERMANENT, IDC_TEMPORARY, OnRadioButton)
   ON_CONTROL_RANGE(BN_CLICKED, IDC_STRESS, IDC_FORCE, OnRadioButton)
   ON_BN_CLICKED(IDC_EXPORT_GRAPH_BTN,OnGraphExportClicked)
   ON_UPDATE_COMMAND_UI(IDC_EXPORT_GRAPH_BTN,OnCommandUIGraphExport)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CEffectivePrestressGraphController::OnInitDialog()
{
   CMultiIntervalGirderGraphControllerBase::OnInitDialog();

   FillDuctCtrl();
   CheckRadioButton(IDC_PERMANENT,IDC_TEMPORARY,IDC_PERMANENT);
   CheckRadioButton(IDC_STRESS,IDC_FORCE,IDC_STRESS);

   return TRUE;
}

void CEffectivePrestressGraphController::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   CMultiIntervalGirderGraphControllerBase::OnUpdate(pSender,lHint,pHint);
   if ( lHint == HINT_BRIDGECHANGED )
   {
      FillDuctCtrl();
      FillIntervalCtrl();
   }
}

void CEffectivePrestressGraphController::SetViewMode(CEffectivePrestressGraphController::ViewMode mode)
{
   UINT nIDC = (mode == Stress ? IDC_STRESS : IDC_FORCE);
   CheckRadioButton(IDC_STRESS, IDC_FORCE, nIDC);
   UpdateGraph();
}

CEffectivePrestressGraphController::ViewMode CEffectivePrestressGraphController::GetViewMode() const
{
   int nIDC = GetCheckedRadioButton(IDC_STRESS, IDC_FORCE);
   ATLASSERT(nIDC != 0); // 0 means nothing is selected
   return (nIDC == IDC_STRESS ? Stress : Force);
}

void CEffectivePrestressGraphController::SetStrandType(CEffectivePrestressGraphController::StrandType strandType)
{
   UINT nIDC = (strandType == Permanent ? IDC_PERMANENT : IDC_TEMPORARY);
   CheckRadioButton(IDC_PERMANENT, IDC_TEMPORARY, nIDC);
   UpdateGraph();
}

CEffectivePrestressGraphController::StrandType CEffectivePrestressGraphController::GetStrandType() const
{
   int nIDC = GetCheckedRadioButton(IDC_PERMANENT, IDC_TEMPORARY);
   ATLASSERT(nIDC != 0); // 0 means nothing is selected
   return (nIDC == IDC_PERMANENT ? Permanent : Temporary);
}

void CEffectivePrestressGraphController::SetDuct(CEffectivePrestressGraphBuilder::DuctType ductType,DuctIndexType ductIdx)
{
   if (m_DuctType != ductType || m_DuctIdx != ductIdx)
   {
      m_DuctType = ductType;
      m_DuctIdx = ductIdx;
      FillIntervalCtrl();

      // only show Permanent/Temporary radio buttons if
      // pre-tensioning is selected (m_DuctIdx == INVALID_INDEX meams show prestress strands)
      int swShow = (m_DuctIdx == INVALID_INDEX ? SW_SHOW : SW_HIDE);
      GetDlgItem(IDC_PERMANENT)->ShowWindow(swShow);
      GetDlgItem(IDC_TEMPORARY)->ShowWindow(swShow);

      UpdateGraph();
   }
}

CEffectivePrestressGraphBuilder::DuctType CEffectivePrestressGraphController::GetDuctType() const
{
   return m_DuctType;
}

DuctIndexType CEffectivePrestressGraphController::GetDuct() const
{
   return m_DuctIdx;
}

bool CEffectivePrestressGraphController::IsStressGraph()
{
   if ( GetSafeHwnd() == nullptr )
   {
      return true;
   }
   else
   {
      int nIDC = GetCheckedRadioButton(IDC_STRESS, IDC_FORCE);
      ATLASSERT(nIDC != 0); // 0 means nothing is selected
      return (nIDC == IDC_STRESS ? true : false);
   }
}

bool CEffectivePrestressGraphController::IsPermanentStrands()
{
   if ( GetSafeHwnd() == nullptr )
   {
      return true;
   }
   else
   {
      int nIDC = GetCheckedRadioButton(IDC_PERMANENT, IDC_TEMPORARY);
      ATLASSERT(nIDC != 0); // 0 means nothing is selected
      return (nIDC == IDC_PERMANENT ? true : false);
   }
}

void CEffectivePrestressGraphController::OnRadioButton(UINT nIDC)
{
   UpdateGraph();
}

void CEffectivePrestressGraphController::OnGroupChanged()
{
   FillDuctCtrl();
}

void CEffectivePrestressGraphController::OnGirderChanged()
{
   FillDuctCtrl();
}

void CEffectivePrestressGraphController::OnDuctChanged()
{
   CComboBox* pcbDuct = (CComboBox*)GetDlgItem(IDC_DUCT);
   int curSel = pcbDuct->GetCurSel();
   DuctIndexType ductIdx = (DuctIndexType)pcbDuct->GetItemData(curSel);

   GroupIndexType grpIdx = GetGirderGroup();
   GirderIndexType gdrIdx = GetGirder();

   CGirderKey girderKey(grpIdx, gdrIdx);

   GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
   DuctIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);

   CEffectivePrestressGraphBuilder::DuctType ductType = (nMaxSegmentDucts < curSel) ? CEffectivePrestressGraphBuilder::Girder : CEffectivePrestressGraphBuilder::Segment;

   SetDuct(ductType,ductIdx);
}

void CEffectivePrestressGraphController::FillDuctCtrl()
{
   GroupIndexType grpIdx = GetGirderGroup();
   GirderIndexType gdrIdx = GetGirder();

   CGirderKey girderKey(grpIdx, gdrIdx);

   CComboBox* pcbDuct = (CComboBox*)GetDlgItem(IDC_DUCT);
   int curSel = pcbDuct->GetCurSel();
   pcbDuct->ResetContent();

   int idx = pcbDuct->AddString(_T("Pretensioning"));
   pcbDuct->SetItemData(idx,(DWORD_PTR)INVALID_INDEX);

   GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
   DuctIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);
   for (DuctIndexType ductIdx = 0; ductIdx < nMaxSegmentDucts; ductIdx++)
   {
      CString strDuct;
      strDuct.Format(_T("Segment Tendon %d"), LABEL_DUCT(ductIdx));
      idx = pcbDuct->AddString(strDuct);
      pcbDuct->SetItemData(idx, (DWORD_PTR)ductIdx);
   }

   GET_IFACE(IGirderTendonGeometry,pGirderTendonGeometry);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++ )
   {
      CString strDuct;
      strDuct.Format(_T("Girder Tendon %d"),LABEL_DUCT(ductIdx));
      idx = pcbDuct->AddString(strDuct);
      pcbDuct->SetItemData(idx,(DWORD_PTR)ductIdx);
   }

   curSel = pcbDuct->SetCurSel(curSel);
   if ( curSel == CB_ERR )
   {
      m_DuctIdx = INVALID_INDEX;
      curSel = pcbDuct->SetCurSel(0);
   }

   if ( curSel == CB_ERR || nMaxSegmentDucts+nGirderDucts == 0 )
   {
      m_DuctIdx = INVALID_INDEX;
      pcbDuct->EnableWindow(FALSE);
   }
   else
   {
      m_DuctType = (nMaxSegmentDucts < curSel ? CEffectivePrestressGraphBuilder::Girder : CEffectivePrestressGraphBuilder::Segment);
      m_DuctIdx = (DuctIndexType)pcbDuct->GetItemData(curSel);
      pcbDuct->EnableWindow(TRUE);
   }
}

void CEffectivePrestressGraphController::OnGraphExportClicked()
{
   // Build default file name
   CString strProjectFileNameNoPath = CExportGraphXYTool::GetTruncatedFileName();

   const CGirderKey& girderKey = GetGirderKey();
   CString girderName = GIRDER_LABEL(girderKey);

   CString actionName = _T("Effective Prestress");

   CString strDefaultFileName = strProjectFileNameNoPath + _T("_") + girderName + _T("_") + actionName;
   strDefaultFileName.Replace(' ','_'); // prefer not to have spaces or ,'s in file names
   strDefaultFileName.Replace(',','_');

   ((CEffectivePrestressGraphBuilder*)GetGraphBuilder())->ExportGraphData(strDefaultFileName);
}

// this has to be implemented otherwise button will not be enabled.
void CEffectivePrestressGraphController::OnCommandUIGraphExport(CCmdUI* pCmdUI)
{
   pCmdUI->Enable(TRUE);
}

IntervalIndexType CEffectivePrestressGraphController::GetFirstInterval()
{
   GET_IFACE(IIntervals,pIntervals);
   CGirderKey girderKey(GetGirderKey());
   if ( m_DuctIdx == INVALID_INDEX )
   {
      return pIntervals->GetFirstStressStrandInterval(girderKey);
   }
   else
   {
      if (m_DuctType == CEffectivePrestressGraphBuilder::Segment)
      {
         return pIntervals->GetFirstSegmentTendonStressingInterval(girderKey);
      }
      else
      {
         return pIntervals->GetFirstGirderTendonStressingInterval(girderKey);
      }
   }
}

#ifdef _DEBUG
void CEffectivePrestressGraphController::AssertValid() const
{
	CMultiIntervalGirderGraphControllerBase::AssertValid();
}

void CEffectivePrestressGraphController::Dump(CDumpContext& dc) const
{
	CMultiIntervalGirderGraphControllerBase::Dump(dc);
}
#endif //_DEBUG
