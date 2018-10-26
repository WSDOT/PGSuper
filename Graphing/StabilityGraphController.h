///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include <Graphing\StabilityGraphBuilder.h>
#include <EAF\EAFGraphControlWindow.h>

#define GT_LIFTING 0
#define GT_HAULING 1

class CStabilityGraphController : public CEAFGraphControlWindow
{
public:
   CStabilityGraphController();
   DECLARE_DYNCREATE(CStabilityGraphController);

   int CreateControls(CWnd* pParent,UINT nID);

   int GetGraphType();
   const CSegmentKey& GetSegmentKey();

protected:

   virtual BOOL OnInitDialog();

   void FillGroupCtrl();
   void FillGirderCtrl();
   void FillSegmentCtrl();

	//{{AFX_MSG(CStabilityGraphController)
   afx_msg void OnGroupChanged();
   afx_msg void OnGirderChanged();
   afx_msg void OnSegmentChanged();
   afx_msg void OnGraphTypeChanged();
   //}}AFX_MSG

	DECLARE_MESSAGE_MAP()

   CComPtr<IBroker> m_pBroker;

   void UpdateGraph();

   CSegmentKey m_SegmentKey;
   int m_GraphType;

#ifdef _DEBUG
public:
   void AssertValid() const;
   void Dump(CDumpContext& dc) const;
#endif //_DEBUG
};