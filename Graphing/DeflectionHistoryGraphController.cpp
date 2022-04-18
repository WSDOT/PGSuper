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
#include "DeflectionHistoryGraphController.h"
#include <Graphing\DeflectionHistoryGraphBuilder.h>
#include <IFace\DocumentType.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CDeflectionHistoryGraphController,CLocationGraphController)

CDeflectionHistoryGraphController::CDeflectionHistoryGraphController()
{
   AlwaysSelect(FALSE); // we don't want to start off with the location selected. If AutoCalc mode is on, this will cause a full time-step analysis. Let the user pick the location before analysis
}

BEGIN_MESSAGE_MAP(CDeflectionHistoryGraphController, CLocationGraphController)
	//{{AFX_MSG_MAP(CStressHistoryGraphController)
   ON_BN_CLICKED(IDC_ELEV_ADJUSTMENT,OnElevAdjustment)
   ON_BN_CLICKED(IDC_PRECAMBER,OnPrecamber)
   ON_BN_CLICKED(IDC_GRID, OnShowGrid)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CDeflectionHistoryGraphController::OnInitDialog()
{
   CLocationGraphController::OnInitDialog();

   GET_IFACE(IDocumentType, pDocType);
   if (pDocType->IsPGSuperDocument())
   {
      // elevation adjustments don't apply to PGSuper
      GetDlgItem(IDC_ELEV_ADJUSTMENT)->ShowWindow(SW_HIDE);
   }

   CheckDlgButton(IDC_PRECAMBER,BST_CHECKED);

   return TRUE;
}

void CDeflectionHistoryGraphController::OnElevAdjustment()
{
   UpdateGraph();
}

void CDeflectionHistoryGraphController::OnPrecamber()
{
   UpdateGraph();
}

void CDeflectionHistoryGraphController::OnShowGrid()
{
   // toggle the grid setting
   ((CDeflectionHistoryGraphBuilder*)GetGraphBuilder())->ShowGrid(ShowGrid());
}

void CDeflectionHistoryGraphController::UpdateGraph()
{
   ((CDeflectionHistoryGraphBuilder*)GetGraphBuilder())->InvalidateGraph();
   ((CDeflectionHistoryGraphBuilder*)GetGraphBuilder())->Update();
}

void CDeflectionHistoryGraphController::IncludeElevationAdjustment(bool bAdjust)
{
   if (IncludeElevationAdjustment() != bAdjust)
   {
      CheckDlgButton(IDC_ELEV_ADJUSTMENT, bAdjust ? BST_CHECKED : BST_UNCHECKED);
      UpdateGraph();
   }
}

bool CDeflectionHistoryGraphController::IncludeElevationAdjustment() const
{
   return IsDlgButtonChecked(IDC_ELEV_ADJUSTMENT) == BST_CHECKED ? true : false;
}

void CDeflectionHistoryGraphController::IncludePrecamber(bool bInclude)
{
   if (IncludePrecamber() != bInclude)
   {
      CheckDlgButton(IDC_PRECAMBER, bInclude ? BST_CHECKED : BST_UNCHECKED);
      UpdateGraph();
   }
}

bool CDeflectionHistoryGraphController::IncludePrecamber() const
{
   return IsDlgButtonChecked(IDC_PRECAMBER) == BST_CHECKED ? true : false;
}

bool CDeflectionHistoryGraphController::ShowGrid() const
{
   CButton* pBtn = (CButton*)GetDlgItem(IDC_GRID);
   return (pBtn->GetCheck() == BST_CHECKED ? true : false);
}

void CDeflectionHistoryGraphController::ShowGrid(bool bShowGrid)
{
   if (bShowGrid != ShowGrid())
   {
      CButton* pBtn = (CButton*)GetDlgItem(IDC_GRID);
      pBtn->SetCheck(bShowGrid ? BST_CHECKED : BST_UNCHECKED);
      ((CDeflectionHistoryGraphBuilder*)GetGraphBuilder())->ShowGrid(bShowGrid);
   }
}

#ifdef _DEBUG
void CDeflectionHistoryGraphController::AssertValid() const
{
	CLocationGraphController::AssertValid();
}

void CDeflectionHistoryGraphController::Dump(CDumpContext& dc) const
{
	CLocationGraphController::Dump(dc);
}
#endif //_DEBUG
