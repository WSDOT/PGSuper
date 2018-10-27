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

#pragma once

#include <Graphing\StressHistoryGraphBuilder.h>
#include "LocationGraphController.h"

class CStressHistoryGraphController : public CLocationGraphController
{
public:
   CStressHistoryGraphController();
   DECLARE_DYNCREATE(CStressHistoryGraphController);

   void Stresses(pgsTypes::StressLocation stressLocation, bool bEnable);
   bool Stresses(pgsTypes::StressLocation stressLocation) const;

   bool ShowGrid() const;
   void ShowGrid(bool bShowGrid);

protected:
   virtual BOOL OnInitDialog() override;
   void UpdateGraph();

   UINT GetControlID(pgsTypes::StressLocation stressLocation) const;

	//{{AFX_MSG(CStressHistoryGraphController)
   afx_msg void OnTopDeck();
   afx_msg void OnBottomDeck();
   afx_msg void OnTopGirder();
   afx_msg void OnBottomGirder();
   afx_msg void OnShowGrid();
   //}}AFX_MSG

	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
public:
   void AssertValid() const;
   void Dump(CDumpContext& dc) const;
#endif //_DEBUG
};