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

#pragma once

#include <Graphs\FinishedElevationGraphBuilder.h>
#include "GirderGraphControllerBase.h"

class CFinishedElevationGraphController : public CMultiIntervalGirderGraphControllerBase
{
public:
   CFinishedElevationGraphController();
   DECLARE_DYNCREATE(CFinishedElevationGraphController);

   // called by the framework when the view's OnUpdate method is called
   virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;

   virtual IntervalIndexType GetFirstInterval() override;
   virtual IntervalIndexType GetLastInterval() override;

   virtual bool ShowBeamBelowGraph() const override { return true; }

   void UpdateControlStatus(bool bInit);

protected:

   virtual BOOL OnInitDialog() override;

   virtual void OnGroupChanged() override;
   virtual void OnGirderChanged() override;

	//{{AFX_MSG(CFinishedElevationGraphController)
   afx_msg void OnGraphExportClicked();
   afx_msg void OnCommandUIGraphExport(CCmdUI* pCmdUI);
   afx_msg void OnShowFinishedDeck();
   afx_msg void OnShowPGL();
   afx_msg void OnShowFinishedDeckBottom();
   afx_msg void OnShowFinishedGirderTop();
   afx_msg void OnShowFinishedGirderBottom();
   afx_msg void OnShowGirderChord();
   afx_msg void OnShowHaunchDepth();
   afx_msg void OnShow10thPoints();
   afx_msg void OnShowElevationTolerance();
   afx_msg void OnEditHaunch();
   afx_msg void OnFillHaunchData();
   afx_msg void OnGraphTypeChanged();
   afx_msg void OnGraphSideChanged();

   //}}AFX_MSG

	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
public:
   void AssertValid() const;
   void Dump(CDumpContext& dc) const;
#endif //_DEBUG
};