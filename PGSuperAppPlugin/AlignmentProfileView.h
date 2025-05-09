///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

// AlignmentProfileView.h : header file
//
//
#include "BridgeViewPane.h"

/////////////////////////////////////////////////////////////////////////////
// CAlignmentProfileView view

class CAlignmentProfileView : public CBridgeViewPane
{
protected:
	CAlignmentProfileView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CAlignmentProfileView)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAlignmentProfileView)
	public:
	virtual void OnInitialUpdate();
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CAlignmentProfileView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void HandleLButtonDblClk(UINT nFlags, CPoint logPoint);
   virtual void HandleContextMenu(CWnd* pWnd,CPoint logPoint);

	//{{AFX_MSG(CAlignmentPlanView)
	afx_msg void OnViewSettings();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   virtual void BuildDisplayLists();
   virtual void UpdateDrawingScale();
   virtual void UpdateDisplayObjects();
   void BuildTitleDisplayObjects();
   void BuildProfileDisplayObjects();
   void BuildBridgeDisplayObjects();
   void BuildLabelDisplayObjects();

   void CreateStationLabel(iDisplayList* pDisplayList,Float64 station,LPCTSTR strBaseLabel=nullptr,UINT textAlign=TA_BASELINE | TA_RIGHT);
   void CreateStationLabel(iDisplayList* pDisplayList,Float64 station,Float64 elevation,LPCTSTR strBaseLabel=nullptr,UINT textAlign=TA_BASELINE | TA_RIGHT);
};
