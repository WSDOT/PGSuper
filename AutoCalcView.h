///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#if !defined(AFX_AutoCalcView_H__450FFB52_2466_11D1_802F_0000F8776D5D__INCLUDED_)
#define AFX_AutoCalcView_H__450FFB52_2466_11D1_802F_0000F8776D5D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AutoCalcView.h : header file
//

class CAutoCalcDoc;

/////////////////////////////////////////////////////////////////////////////
// CAutoCalcView view

class CAutoCalcView : public CView
{
protected:
	CAutoCalcView();           // protected constructor used by dynamic creation

// Attributes
public:
   CAutoCalcDoc* GetDocument();
   DECLARE_DYNAMIC(CAutoCalcView);

// Operations
public:
   bool IsLpFrameEnabled() const;
   void OnUpdateNow();

   virtual bool DoResultsExist() const = 0;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAutoCalcView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
   virtual void UpdateNow() = 0;

   virtual ~CAutoCalcView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

   void EnableLpFrame(bool bEnable);

   // Generated message map functions
protected:

	//{{AFX_MSG(CAutoCalcView)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   bool m_bLpFrameEnabled;
};

inline CAutoCalcDoc* CAutoCalcView::GetDocument()
   { return (CAutoCalcDoc*)m_pDocument; }

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AutoCalcView_H__450FFB52_2466_11D1_802F_0000F8776D5D__INCLUDED_)
