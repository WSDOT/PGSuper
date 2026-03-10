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

#include "stdafx.h"
#include "resource.h"
#include "StressHistoryGraphController.h"
#include <Graphs/StressHistoryGraphBuilder.h>
#include <Graphs/ExportGraphXYTool.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDocument.h>


IMPLEMENT_DYNCREATE(CStressHistoryGraphController,CLocationGraphController)

CStressHistoryGraphController::CStressHistoryGraphController()
{
}

BEGIN_MESSAGE_MAP(CStressHistoryGraphController, CLocationGraphController)
	//{{AFX_MSG_MAP(CStressHistoryGraphController)
   ON_BN_CLICKED(IDC_TOPDECK, OnTopDeck)
   ON_BN_CLICKED(IDC_BOTTOMDECK, OnBottomDeck)
   ON_BN_CLICKED(IDC_TOPGIRDER, OnTopGirder)
   ON_BN_CLICKED(IDC_BOTTOMGIRDER, OnBottomGirder)
   ON_BN_CLICKED(IDC_GRID, OnShowGrid)
   ON_BN_CLICKED(IDC_EXPORT_GRAPH_BTN,OnGraphExportClicked)
   ON_UPDATE_COMMAND_UI(IDC_EXPORT_GRAPH_BTN,OnCommandUIGraphExport)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

UINT CStressHistoryGraphController::GetControlID(pgsTypes::StressLocation stressLocation) const
{
   UINT nIDC = IDC_BOTTOMGIRDER;;
   switch (stressLocation)
   {
   case pgsTypes::TopGirder:    nIDC = IDC_TOPGIRDER;    break;
   case pgsTypes::BottomGirder: nIDC = IDC_BOTTOMGIRDER; break;
   case pgsTypes::TopDeck:      nIDC = IDC_TOPDECK;      break;
   case pgsTypes::BottomDeck:   nIDC = IDC_BOTTOMDECK;   break;
   default:
      ATLASSERT(false);
      nIDC = IDC_BOTTOMGIRDER;
   }
   return nIDC;
}

BOOL CStressHistoryGraphController::OnInitDialog()
{
   CLocationGraphController::OnInitDialog();

   GET_IFACE(IBridge, pBridge);
   if (IsNonstructuralDeck(pBridge->GetDeckType()))
   {
      GetDlgItem(IDC_TOPDECK)->EnableWindow(FALSE);
      GetDlgItem(IDC_BOTTOMDECK)->EnableWindow(FALSE);
   }

   return TRUE;
}

void CStressHistoryGraphController::UpdateGraph()
{
   ((CStressHistoryGraphBuilder*)GetGraphBuilder())->InvalidateGraph();
   ((CStressHistoryGraphBuilder*)GetGraphBuilder())->Update();
}

#ifdef _DEBUG
void CStressHistoryGraphController::AssertValid() const
{
	CLocationGraphController::AssertValid();
}

void CStressHistoryGraphController::Dump(CDumpContext& dc) const
{
	CLocationGraphController::Dump(dc);
}
#endif //_DEBUG

void CStressHistoryGraphController::Stresses(pgsTypes::StressLocation stressLocation, bool bEnable)
{
   if (Stresses(stressLocation) != bEnable)
   {
      CheckDlgButton(GetControlID(stressLocation), bEnable ? BST_CHECKED : BST_UNCHECKED);
      ((CStressHistoryGraphBuilder*)GetGraphBuilder())->Stresses(stressLocation,bEnable);
   }
}

bool CStressHistoryGraphController::Stresses(pgsTypes::StressLocation stressLocation) const
{
   return IsDlgButtonChecked(GetControlID(stressLocation)) == BST_CHECKED ? true : false;
}

bool CStressHistoryGraphController::ShowGrid() const
{
   CButton* pBtn = (CButton*)GetDlgItem(IDC_GRID);
   return (pBtn->GetCheck() == BST_CHECKED ? true : false);
}

void CStressHistoryGraphController::ShowGrid(bool bShowGrid)
{
   if (bShowGrid != ShowGrid())
   {
      CButton* pBtn = (CButton*)GetDlgItem(IDC_GRID);
      pBtn->SetCheck(bShowGrid ? BST_CHECKED : BST_UNCHECKED);
      ((CStressHistoryGraphBuilder*)GetGraphBuilder())->ShowGrid(bShowGrid);
   }
}

void CStressHistoryGraphController::OnShowGrid()
{
   // toggle the grid setting
   ((CStressHistoryGraphBuilder*)GetGraphBuilder())->ShowGrid(ShowGrid());
}

void CStressHistoryGraphController::OnTopDeck()
{
   ((CStressHistoryGraphBuilder*)GetGraphBuilder())->Stresses(pgsTypes::TopDeck, Stresses(pgsTypes::TopDeck));
}

void CStressHistoryGraphController::OnBottomDeck()
{
   ((CStressHistoryGraphBuilder*)GetGraphBuilder())->Stresses(pgsTypes::BottomDeck, Stresses(pgsTypes::BottomDeck));
}

void CStressHistoryGraphController::OnTopGirder()
{
   ((CStressHistoryGraphBuilder*)GetGraphBuilder())->Stresses(pgsTypes::TopGirder, Stresses(pgsTypes::TopGirder));
}

void CStressHistoryGraphController::OnBottomGirder()
{
   ((CStressHistoryGraphBuilder*)GetGraphBuilder())->Stresses(pgsTypes::BottomGirder, Stresses(pgsTypes::BottomGirder));
}

void CStressHistoryGraphController::OnGraphExportClicked()
{
   CComboBox* pcbLocation = (CComboBox*)GetDlgItem(IDC_POI);
   int curSel = pcbLocation->GetCurSel();
   if (curSel == CB_ERR)
   {
      ::AfxMessageBox(_T("No Location is Selected. Please selection a location"),MB_ICONERROR);
      return;
   }

   // Build default file name
   CString strProjectFileNameNoPath = CExportGraphXYTool::GetTruncatedFileName();

   CString girderName;
   pcbLocation->GetLBText(curSel,girderName);

   CString actionName = _T("Stress History");

   CString strDefaultFileName = strProjectFileNameNoPath + _T("_") + girderName + _T("_") + actionName;
   strDefaultFileName.Replace(' ','_'); // prefer not to have spaces or ,'s in file names
   strDefaultFileName.Replace(',','_');

   ((CStressHistoryGraphBuilder*)GetGraphBuilder())->ExportGraphData(strDefaultFileName);
}

// this has to be implemented otherwise button will not be enabled.
void CStressHistoryGraphController::OnCommandUIGraphExport(CCmdUI* pCmdUI)
{
   pCmdUI->Enable(TRUE);
}
