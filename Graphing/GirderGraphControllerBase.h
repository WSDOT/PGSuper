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

#include <EAF\EAFGraphControlWindow.h>

class CGirderGraphControllerBase : public CEAFGraphControlWindow
{
public:
   CGirderGraphControllerBase(bool bAllGroups=true);
   DECLARE_DYNCREATE(CGirderGraphControllerBase);

   virtual IndexType GetGraphCount() = 0;

   GroupIndexType GetGirderGroup();
   GirderIndexType GetGirder();
   IntervalIndexType GetInterval();

protected:

   virtual BOOL OnInitDialog();

	//{{AFX_MSG(CGirderGraphControllerBase)
   afx_msg void CbnOnGroupChanged();
   afx_msg void CbnOnGirderChanged();
   afx_msg void CbnOnIntervalChanged();
   afx_msg void OnShowGrid();
   afx_msg void OnShowBeam();
   //}}AFX_MSG

   // Called from CbnOnXXXChanged just before UpdateGraph is called
   virtual void OnGroupChanged();
   virtual void OnGirderChanged();
   virtual void OnIntervalChanged();


	DECLARE_MESSAGE_MAP()

   CComPtr<IBroker> m_pBroker;

   void FillGroupCtrl();
   void FillGirderCtrl();
   void FillIntervalCtrl();

   void UpdateGraph();

   bool m_bAllGroups;

   // control variables
   GroupIndexType         m_GroupIdx;
   GirderIndexType        m_GirderIdx;
   IntervalIndexType      m_IntervalIdx;
 
#ifdef _DEBUG
public:
   void AssertValid() const;
   void Dump(CDumpContext& dc) const;
#endif //_DEBUG
};