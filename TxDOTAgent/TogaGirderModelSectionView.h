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

#if !defined(AFX_TogaGirderModelSectionView_H__E2B376CA_2D38_11D2_8EB4_006097DF3C68__INCLUDED_)
#define AFX_TogaGirderModelSectionView_H__E2B376CA_2D38_11D2_8EB4_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TogaGirderModelSectionView.h : header file

#include <DManip\DisplayView.h>

//
class CTxDOTOptionalDesignDoc;
class CTxDOTOptionalDesignGirderViewPage;

/////////////////////////////////////////////////////////////////////////////
// CTogaGirderModelSectionView view

class CTogaGirderModelSectionView : public CDisplayView
{
protected:
	CTogaGirderModelSectionView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CTogaGirderModelSectionView)

// Attributes
public:

// Operations
public:
   void DoPrint(CDC* pDC, CPrintInfo* pInfo);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTogaGirderModelSectionView)
	public:
	virtual void OnInitialUpdate();
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	protected:
	virtual void OnDraw(CDC* pDC);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CTogaGirderModelSectionView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CTogaGirderModelSectionView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewSettings();
	afx_msg void OnLeftEnd();
	afx_msg void OnLeftHp();
	afx_msg void OnCenter();
	afx_msg void OnRightHp();
	afx_msg void OnRightEnd();
	afx_msg void OnUserCut();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   CTxDOTOptionalDesignGirderViewPage* m_pFrame;

   void CreateDisplayLists();
   void UpdateDisplayObjects();
   void BuildSectionDisplayObjects(CTxDOTOptionalDesignDoc* pDoc,IBroker* pBroker,const CSegmentKey& segmentKey,iDisplayMgr* pDispMgr);
   void BuildStrandDisplayObjects(CTxDOTOptionalDesignDoc* pDoc,IBroker* pBroker,const CSegmentKey& segmentKey,iDisplayMgr* pDispMgr);
   void BuildLongReinfDisplayObjects(CTxDOTOptionalDesignDoc* pDoc,IBroker* pBroker,const CSegmentKey& segmentKey,iDisplayMgr* pDispMgr);
   void BuildCGDisplayObjects(CTxDOTOptionalDesignDoc* pDoc,IBroker* pBroker,const CSegmentKey& segmentKey,iDisplayMgr* pDispMgr);
   void BuildDimensionDisplayObjects(CTxDOTOptionalDesignDoc* pDoc,IBroker* pBroker,const CSegmentKey& segmentKey,iDisplayMgr* pDispMgr);

   bool m_bUpdateError;
   std::_tstring m_ErrorMsg;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TogaGirderModelSectionView_H__E2B376CA_2D38_11D2_8EB4_006097DF3C68__INCLUDED_)
