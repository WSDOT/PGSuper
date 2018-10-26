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

#if !defined(AFX_FACTOROFSAFETYVIEW_H__640D1EEF_1A1F_11D5_8BD4_006097C68A9C__INCLUDED_)
#define AFX_FACTOROFSAFETYVIEW_H__640D1EEF_1A1F_11D5_8BD4_006097C68A9C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FactorOfSafetyView.h : header file
//

#include <EAF\EAFAutoCalcView.h>

#if !defined INCLUDED_GRAPHICSLIB_GRAPHXY_H_
#include <GraphicsLib\GraphXY.h>
#endif

#include "FactorOfSafetyChildFrame.h"

interface IBroker;
class arvPhysicalConverter;

/////////////////////////////////////////////////////////////////////////////
// CFactorOfSafetyView view

class CFactorOfSafetyView : public CView,
                            public CEAFAutoCalcViewMixin
{
protected:
	CFactorOfSafetyView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CFactorOfSafetyView)

// Attributes
public:
   virtual void UpdateFromBar(); // user interacted with bar
   virtual void UpdateNow();
   void UpdateGrid();

   virtual bool DoResultsExist();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFactorOfSafetyView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
   virtual void OnInitialUpdate();
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CFactorOfSafetyView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CFactorOfSafetyView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnUpdateFilePrint(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void Update();
   void DoUpdateNow();

   void UpdateUnits();
   void AddGraphPoint(IndexType series, Float64 xval, Float64 yval);
   void DrawLegend(CDC* pDC);

   CFactorOfSafetyChildFrame* m_pFrame;

   bool m_bValidGraph;
   bool m_bUpdateError;
   std::_tstring m_ErrorMsg;

   IBroker* m_pBroker;

   arvPhysicalConverter* m_pXFormat;
   arvPhysicalConverter* m_pYFormat;

   grGraphXY m_Graph;
   CRect m_PrintRect;
   bool m_IsPrinting;
   std::_tstring m_PrintSubtitle;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FACTOROFSAFETYVIEW_H__640D1EEF_1A1F_11D5_8BD4_006097C68A9C__INCLUDED_)
