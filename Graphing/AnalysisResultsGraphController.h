///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include <Graphing\AnalysisResultsGraphBuilder.h>
#include "GirderGraphControllerBase.h"

class CAnalysisResultsGraphController : public CGirderGraphControllerBase
{
public:
   CAnalysisResultsGraphController();
   DECLARE_DYNCREATE(CAnalysisResultsGraphController);

   ActionType GetActionType();
   pgsTypes::AnalysisType GetAnalysisType();

   virtual IndexType GetGraphCount();

   IDType SelectedGraphIndexToGraphID(IndexType graphIdx);

protected:

   virtual BOOL OnInitDialog();

	//{{AFX_MSG(CAnalysisResultsGraphController)
   afx_msg void OnActionChanged();
   afx_msg void OnLoadCaseChanged();
   afx_msg void OnAnalysisTypeClicked();
   //}}AFX_MSG

   virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
   virtual void OnGirderChanged();
   virtual void OnIntervalChanged();

	DECLARE_MESSAGE_MAP()

   void FillActionTypeCtrl();
   void FillLoadCaseList();
   void UpdateAnalysisType();

   // conotrl variables
   ActionType             m_ActionType;
   pgsTypes::AnalysisType m_AnalysisType;

#ifdef _DEBUG
public:
   void AssertValid() const;
   void Dump(CDumpContext& dc) const;
#endif //_DEBUG

   friend CAnalysisResultsGraphBuilder;
};