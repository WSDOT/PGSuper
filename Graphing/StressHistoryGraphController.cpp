///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

IMPLEMENT_DYNCREATE(CStressHistoryGraphController,CLocationGraphController)

CStressHistoryGraphController::CStressHistoryGraphController()
{
}

BEGIN_MESSAGE_MAP(CStressHistoryGraphController, CLocationGraphController)
	//{{AFX_MSG_MAP(CStressHistoryGraphController)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CStressHistoryGraphController::OnInitDialog()
{
   CLocationGraphController::OnInitDialog();
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
