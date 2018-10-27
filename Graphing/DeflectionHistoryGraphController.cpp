///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

IMPLEMENT_DYNCREATE(CDeflectionHistoryGraphController,CLocationGraphController)

CDeflectionHistoryGraphController::CDeflectionHistoryGraphController()
{
   AlwaysSelect(FALSE); // we don't want to start off with the location selected. If AutoCalc mode is on, this will cause a full time-step analysis. Let the user pick the location before analysis
}

BEGIN_MESSAGE_MAP(CDeflectionHistoryGraphController, CLocationGraphController)
	//{{AFX_MSG_MAP(CStressHistoryGraphController)
   ON_BN_CLICKED(IDC_ELEV_ADJUSTMENT,OnElevAdjustment)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CDeflectionHistoryGraphController::OnInitDialog()
{
   CLocationGraphController::OnInitDialog();

   return TRUE;
}

void CDeflectionHistoryGraphController::OnElevAdjustment()
{
   UpdateGraph();
}

void CDeflectionHistoryGraphController::UpdateGraph()
{
   ((CDeflectionHistoryGraphBuilder*)GetGraphBuilder())->InvalidateGraph();
   ((CDeflectionHistoryGraphBuilder*)GetGraphBuilder())->Update();
}

bool CDeflectionHistoryGraphController::IncludeElevationAdjustment()
{
   return IsDlgButtonChecked(IDC_ELEV_ADJUSTMENT) == BST_CHECKED ? true : false;
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
