///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include <Graphing\StressHistoryGraphBuilder.h>

#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
