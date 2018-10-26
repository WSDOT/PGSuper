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

#pragma once

#include <Graphing\ConcretePropertyGraphBuilder.h>
#include <EAF\EAFGraphControlWindow.h>

class CConcretePropertyGraphController : public CEAFGraphControlWindow
{
public:
   CConcretePropertyGraphController();
   DECLARE_DYNCREATE(CConcretePropertyGraphController);

   int GetGraphElement();
   int GetGraphType();
   CSegmentKey GetSegmentKey();
   CClosureKey GetClosureKey();
   int GetXAxisType();

protected:
   virtual BOOL OnInitDialog();

	//{{AFX_MSG(CConcretePropertyGraphController)
   //}}AFX_MSG

	DECLARE_MESSAGE_MAP()

   afx_msg void OnGraphElement();
   afx_msg void OnGraphType();
   afx_msg void OnGroupChanged();
   afx_msg void OnGirderChanged();
   afx_msg void OnSegmentChanged();
   afx_msg void OnClosureChanged();
   afx_msg void OnXAxis();

   void UpdateElementControls();

   void FillGroupControl();
   void FillGirderControl();
   void FillSegmentControl();
   void FillClosureControl();

   GroupIndexType GetGroupIndex();
   GirderIndexType GetGirderIndex();
   SegmentIndexType GetSegmentIndex();

   void UpdateGraph();

   CComPtr<IBroker> m_pBroker;

#ifdef _DEBUG
public:
   void AssertValid() const;
   void Dump(CDumpContext& dc) const;
#endif //_DEBUG
};