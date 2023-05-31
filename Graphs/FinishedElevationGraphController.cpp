///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include "FinishedElevationGraphController.h"
#include <Graphs/FinishedElevationGraphBuilder.h>
#include <Graphs/ExportGraphXYTool.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>
#include <IFace\EditByUI.h>
#include <IFace\Transactions.h>

#include <Hints.h>
#include "FillHaunchDlg.h"

#include <EAF\EAFGraphBuilderBase.h>
#include <EAF\EAFGraphView.h>
#include <EAF\EAFDocument.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\EditBridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CFinishedElevationGraphController,CMultiIntervalGirderGraphControllerBase)

CFinishedElevationGraphController::CFinishedElevationGraphController():
CMultiIntervalGirderGraphControllerBase(true/*use ALL_GROUPS*/)
{
}

BEGIN_MESSAGE_MAP(CFinishedElevationGraphController, CMultiIntervalGirderGraphControllerBase)
   //{{AFX_MSG_MAP(CFinishedElevationGraphController)
   ON_BN_CLICKED(IDC_EXPORT_GRAPH_BTN,OnGraphExportClicked)
   ON_UPDATE_COMMAND_UI(IDC_EXPORT_GRAPH_BTN,OnCommandUIGraphExport)
   ON_BN_CLICKED(IDC_PGL,OnShowPGL)
   ON_BN_CLICKED(IDC_FINISHED_DECK,OnShowFinishedDeck)
   ON_BN_CLICKED(IDC_FINISHED_DECK_BOTTOM,OnShowFinishedDeckBottom)
   ON_BN_CLICKED(IDC_FINISHED_GIRDER_TOP,OnShowFinishedGirderTop)
   ON_BN_CLICKED(IDC_FINISHED_GIRDER_BOTTOM,OnShowFinishedGirderBottom)
   ON_BN_CLICKED(IDC_GIRDER_CHORD,OnShowGirderChord)
   ON_CBN_SELCHANGE(IDC_GRAPH_TYPE,OnGraphTypeChanged)
   ON_CBN_SELCHANGE(IDC_PLOT_AT,OnGraphSideChanged)
   ON_BN_CLICKED(IDC_HAUNCH_DEPTH,OnShowHaunchDepth)
   ON_BN_CLICKED(IDC_10TH_POINTS,OnShow10thPoints)
   ON_BN_CLICKED(IDC_ELEVATION_TOLERANCE,OnShowElevationTolerance)
   ON_BN_CLICKED(IDC_EDIT_HAUNCH,OnEditHaunch)
   ON_BN_CLICKED(IDC_FILL_HAUNCH,OnFillHaunchData)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CFinishedElevationGraphController::OnInitDialog()
{
   __super::OnInitDialog();

   CButton* pBut = (CButton*)GetDlgItem(IDC_PGL);
   BOOL show = ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowPGL();
   pBut->SetCheck(show == FALSE ? BST_UNCHECKED : BST_CHECKED);

   pBut = (CButton*)GetDlgItem(IDC_FINISHED_DECK);
   show = ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowFinishedDeck();
   pBut->SetCheck(show == FALSE ? BST_UNCHECKED : BST_CHECKED);

   pBut = (CButton*)GetDlgItem(IDC_GIRDER_CHORD);
   show = ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowGirderChord();
   pBut->SetCheck(show == FALSE ? BST_UNCHECKED : BST_CHECKED);

   int st;
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_GRAPH_TYPE);
   pBox->ResetContent();
   st = pBox->AddString(_T("Elevations"));
   st = pBox->AddString(_T("Elevation differential from PGL"));
   CFinishedElevationGraphBuilder::GraphType gt = ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->GetGraphType();
   st = pBox->SetCurSel((int)gt);

   pBox = (CComboBox*)GetDlgItem(IDC_PLOT_AT);
   pBox->ResetContent();
   st = pBox->AddString(_T("Left Edge of Top Flange"));
   st = pBox->AddString(_T("Centerline of Girder"));
   st = pBox->AddString(_T("Right Edge of Top Flange"));
   CFinishedElevationGraphBuilder::GraphSide gs = ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->GetGraphSide();
   st = pBox->SetCurSel((int)gs);

   pBut = (CButton*)GetDlgItem(IDC_HAUNCH_DEPTH);
   pBut->ShowWindow(SW_HIDE);

   pBut = (CButton*)GetDlgItem(IDC_ELEVATION_TOLERANCE);
   pBut->ShowWindow(SW_HIDE);

   UpdateControlStatus(true);

   return TRUE;
}

void CFinishedElevationGraphController::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   __super::OnUpdate(pSender,lHint,pHint);
   if ( lHint == HINT_BRIDGECHANGED )
   {
      FillIntervalCtrl();
      UpdateControlStatus(false);
   }
}

void CFinishedElevationGraphController::OnShowFinishedDeck()
{
   CButton* pBox = (CButton*)GetDlgItem(IDC_FINISHED_DECK);
   BOOL show = pBox->GetCheck() == 0 ? FALSE : TRUE;
   ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowFinishedDeck(show);
   UpdateGraph();
}

void CFinishedElevationGraphController::OnShowPGL()
{
   CButton* pBox = (CButton*)GetDlgItem(IDC_PGL);
   BOOL show = pBox->GetCheck() == 0 ? FALSE : TRUE;
   ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowPGL(show);
   UpdateGraph();
}

void CFinishedElevationGraphController::OnShowFinishedDeckBottom()
{
   CButton* pBox = (CButton*)GetDlgItem(IDC_FINISHED_DECK_BOTTOM);
   BOOL show = pBox->GetCheck() == 0 ? FALSE : TRUE;
   ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowFinishedDeckBottom(show);
   UpdateGraph();
}

void CFinishedElevationGraphController::OnShowFinishedGirderTop()
{
   CButton* pBox = (CButton*)GetDlgItem(IDC_FINISHED_GIRDER_TOP);
   BOOL show = pBox->GetCheck() == 0 ? FALSE : TRUE;
   ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowFinishedGirderTop(show);
   UpdateGraph();
}

void CFinishedElevationGraphController::OnShowFinishedGirderBottom()
{
   CButton* pBox = (CButton*)GetDlgItem(IDC_FINISHED_GIRDER_BOTTOM);
   BOOL show = pBox->GetCheck() == 0 ? FALSE : TRUE;
   ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowFinishedGirderBottom(show);
   UpdateGraph();
}

void CFinishedElevationGraphController::OnShowGirderChord()
{
   CButton* pBox = (CButton*)GetDlgItem(IDC_GIRDER_CHORD);
   BOOL show = pBox->GetCheck() == 0 ? FALSE : TRUE;
   ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowGirderChord(show);
   UpdateGraph();
}

void CFinishedElevationGraphController::OnShowHaunchDepth()
{
   CButton* pBox = (CButton*)GetDlgItem(IDC_HAUNCH_DEPTH);
   BOOL show = pBox->GetCheck() == 0 ? FALSE : TRUE;
   ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowHaunchDepth(show);
   UpdateGraph();
}

void CFinishedElevationGraphController::OnShow10thPoints()
{
   CButton* pBox = (CButton*)GetDlgItem(IDC_10TH_POINTS);
   BOOL show = pBox->GetCheck() == 0 ? FALSE : TRUE;
   ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->Show10thPoints(show);
   UpdateGraph();
}

void CFinishedElevationGraphController::OnShowElevationTolerance()
{
   CButton* pBox = (CButton*)GetDlgItem(IDC_ELEVATION_TOLERANCE);
   BOOL show = pBox->GetCheck() == 0 ? FALSE : TRUE;
   ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowElevationTolerance(show);
   UpdateGraph();
}

void CFinishedElevationGraphController::OnEditHaunch()
{
   GET_IFACE(IEditByUI,pEdit);
   pEdit->EditHaunch();

   UpdateControlStatus(false);
}

void CFinishedElevationGraphController::OnFillHaunchData()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pOldBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // See if we can even do this
   if (pOldBridgeDesc->GetHaunchInputDepthType() == pgsTypes::hidACamber)
   {
      // We normally won't get here, but there is a chance if the input type is changed outside and autocalc is Off
      ::AfxMessageBox(_T("The fill haunch feature is not available when haunches are defined using the Slab Offset (\"A\") input method"),MB_OK | MB_ICONINFORMATION);
   }
   else
   {
      CFillHaunchDlg dlg(this->GetGirderKey(),m_pBroker);
      if (IDOK == dlg.DoModal())
      {
         CBridgeDescription2 newBridgeDescr(*pOldBridgeDesc);

         if (dlg.ModifyBridgeDescription(newBridgeDescr))
         {
            GET_IFACE(IEnvironment,pEnvironment);
            enumExposureCondition oldExposureCondition = pEnvironment->GetExposureCondition();
            Float64 oldRelHumidity = pEnvironment->GetRelHumidity();

            std::unique_ptr<CEAFTransaction> pTxn(std::make_unique<txnEditBridge>(*pOldBridgeDesc,newBridgeDescr,
               oldExposureCondition,oldExposureCondition,oldRelHumidity,oldRelHumidity));

            GET_IFACE(IEAFTransactions,pTransactions);
            pTransactions->Execute(std::move(pTxn));

            // Give user some confirmation that values where changed. A report of some kind might be better, but not sure it's worth the effort.
            CString msg(_T("Haunch depths were modified. Click Yes to view/edit new haunch values"));
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            if (AfxMessageBox(msg,MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
               OnEditHaunch();
            }
         }
      }
   }
}

void CFinishedElevationGraphController::OnGraphTypeChanged()
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_GRAPH_TYPE);
   CFinishedElevationGraphBuilder::GraphType gt = (CFinishedElevationGraphBuilder::GraphType)pBox->GetCurSel();

   CButton* pButPgl = (CButton*)GetDlgItem(IDC_PGL);
   CButton* pButHaunch = (CButton*)GetDlgItem(IDC_HAUNCH_DEPTH);
   CComboBox* pBoxLoc = (CComboBox*)GetDlgItem(IDC_PLOT_AT);
   if (CFinishedElevationGraphBuilder::gtElevationDifferential == gt)
   {
      // graph of PGL is not an option for this case
      pButPgl->SetCheck(FALSE);
      pButPgl->ShowWindow(SW_HIDE);
      ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowPGL(FALSE);

      pButHaunch->ShowWindow(SW_SHOW);

      // Differentials are only at CL Girder
      pBoxLoc->SetCurSel((int)CFinishedElevationGraphBuilder::gsCenterLine);
      pBoxLoc->EnableWindow(FALSE);
      ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->SetGraphSide(CFinishedElevationGraphBuilder::gsCenterLine);
   }
   else
   {
      pButPgl->SetCheck(TRUE);
      pButPgl->ShowWindow(SW_SHOW);
      ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowPGL(TRUE);
      pButHaunch->ShowWindow(SW_HIDE);
      pBoxLoc->EnableWindow(TRUE);
   }

   GET_IFACE(IBridge,pBridge);
   pgsTypes::HaunchInputDepthType haunchInputType = pBridge->GetHaunchInputDepthType();

   // Elevation tolerance only available for differential view of direct haunch
   int showToler = CFinishedElevationGraphBuilder::gtElevationDifferential == gt && pgsTypes::hidACamber != haunchInputType ? SW_SHOW : SW_HIDE;
   CButton* pButTol = (CButton*)GetDlgItem(IDC_ELEVATION_TOLERANCE);
   pButTol->ShowWindow(showToler);
   if (showToler)
   {
      pButTol->SetCheck(((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowElevationTolerance());
   }

   ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->SetGraphType(gt);
   UpdateGraph();
}

void CFinishedElevationGraphController::OnGraphSideChanged()
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_PLOT_AT);
   CFinishedElevationGraphBuilder::GraphSide gs = (CFinishedElevationGraphBuilder::GraphSide)pBox->GetCurSel();
   ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->SetGraphSide(gs);
   if (CFinishedElevationGraphBuilder::gsCenterLine == gs)
   {
      UpdateGraph();
   }

   CButton* pBotBut = (CButton*)GetDlgItem(IDC_FINISHED_GIRDER_BOTTOM);
   CButton* pChordBut = (CButton*)GetDlgItem(IDC_GIRDER_CHORD);
   if (CFinishedElevationGraphBuilder::gsCenterLine != gs)
   {
      pBotBut->EnableWindow(FALSE);
      pBotBut->SetCheck(0);
      ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowFinishedGirderBottom(FALSE);
      pChordBut->EnableWindow(FALSE);
      pChordBut->SetCheck(0);
      ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowGirderChord(FALSE);
      UpdateGraph();
   }
   else
   {
      pBotBut->EnableWindow(TRUE);
      pChordBut->EnableWindow(TRUE);
   }
}

void CFinishedElevationGraphController::OnGroupChanged()
{
   FillIntervalCtrl();
}

void CFinishedElevationGraphController::OnGirderChanged()
{
   FillIntervalCtrl();
}

void CFinishedElevationGraphController::OnGraphExportClicked()
{
   // Build default file name
   CString strProjectFileNameNoPath = CExportGraphXYTool::GetTruncatedFileName();

   const CGirderKey& girderKey = GetGirderKey();
   CString girderName = GIRDER_LABEL(girderKey);

   CString actionName = _T("FinishedElevations");

   CString strDefaultFileName = strProjectFileNameNoPath + _T("_") + girderName + _T("_") + actionName;
   strDefaultFileName.Replace(' ','_'); // prefer not to have spaces or ,'s in file names
   strDefaultFileName.Replace(',','_');

   ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ExportGraphData(strDefaultFileName);
}

// this has to be implemented otherwise button will not be enabled.
void CFinishedElevationGraphController::OnCommandUIGraphExport(CCmdUI* pCmdUI)
{
   pCmdUI->Enable(TRUE);
}

IntervalIndexType CFinishedElevationGraphController::GetFirstInterval()
{
   GET_IFACE_NOCHECK(IBridge,pBridge);
   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(ILossParameters,pLossParams);
   if (pLossParams->GetLossMethod() != pgsTypes::TIME_STEP || pBridge->GetHaunchInputDepthType() == pgsTypes::hidACamber)
   {
      // We can only compute camber at the GCE.
      return pIntervals->GetGeometryControlInterval();
   }
   else
   {
      return pIntervals->GetLastSegmentErectionInterval(GetGirderKey());
   }
}

IntervalIndexType CFinishedElevationGraphController::GetLastInterval()
{
   GET_IFACE_NOCHECK(IBridge,pBridge);
   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(ILossParameters,pLossParams);
   if (pLossParams->GetLossMethod() != pgsTypes::TIME_STEP || pBridge->GetHaunchInputDepthType() == pgsTypes::hidACamber)
   {
      // We can only compute camber at the GCE.
      return pIntervals->GetGeometryControlInterval();
   }
   else
   {
      return pIntervals->GetIntervalCount() - 1;
   }
}

void CFinishedElevationGraphController::UpdateControlStatus(bool bInit)
{
   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   CButton* pBut = (CButton*)GetDlgItem(IDC_FINISHED_DECK_BOTTOM);
   if (pgsTypes::sdtNone == deckType)
   {
      pBut->SetCheck(BST_UNCHECKED);
      pBut->EnableWindow(FALSE);
   }
   else
   {
      pBut->EnableWindow(TRUE);
      BOOL show = ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowFinishedDeckBottom();
      pBut->SetCheck(show == FALSE ? BST_UNCHECKED : BST_CHECKED);
   }

   pBut = (CButton*)GetDlgItem(IDC_FINISHED_GIRDER_BOTTOM);
   if (pgsTypes::sdtNone == deckType)
   {
      pBut->SetCheck(BST_UNCHECKED);
      pBut->EnableWindow(FALSE);
   }
   else
   {
      pBut->EnableWindow(TRUE);
      BOOL show = ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowFinishedGirderBottom();
      pBut->SetCheck(show == FALSE ? BST_UNCHECKED : BST_CHECKED);
   }

   pBut = (CButton*)GetDlgItem(IDC_FINISHED_GIRDER_TOP);
   if (pgsTypes::sdtNone == deckType)
   {
      pBut->SetCheck(BST_UNCHECKED);
      pBut->EnableWindow(FALSE);
   }
   else
   {
      pBut->EnableWindow(TRUE);
      BOOL show = ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->ShowFinishedGirderTop();
      pBut->SetCheck(show == FALSE ? BST_UNCHECKED : BST_CHECKED);
   }

   GetDlgItem(IDC_EDIT_HAUNCH)->EnableWindow(pgsTypes::sdtNone != deckType);

   BOOL bEnable = pBridge->GetHaunchInputDepthType() != pgsTypes::hidACamber && pgsTypes::sdtNone != deckType;
   GetDlgItem(IDC_FILL_HAUNCH)->EnableWindow(bEnable);

   pgsTypes::HaunchInputDepthType haunchInputType = pBridge->GetHaunchInputDepthType();

   // Elevation tolerance only available for differential view of direct haunch
   CFinishedElevationGraphBuilder::GraphType gt = ((CFinishedElevationGraphBuilder*)GetGraphBuilder())->GetGraphType();
   int showToler = CFinishedElevationGraphBuilder::gtElevationDifferential == gt && pgsTypes::hidACamber != haunchInputType ? SW_SHOW : SW_HIDE;
   CButton* pButTol = (CButton*)GetDlgItem(IDC_ELEVATION_TOLERANCE);
   pButTol->ShowWindow(showToler);
}


#ifdef _DEBUG
void CFinishedElevationGraphController::AssertValid() const
{
   __super::AssertValid();
}

void CFinishedElevationGraphController::Dump(CDumpContext& dc) const
{
   __super::Dump(dc);
}
#endif //_DEBUG
