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

#if !defined(AFX_GIRDERMODELSECTIONVIEW_H__E2B376CA_2D38_11D2_8EB4_006097DF3C68__INCLUDED_)
#define AFX_GIRDERMODELSECTIONVIEW_H__E2B376CA_2D38_11D2_8EB4_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GirderModelSectionView.h : header file

#include <DManip\DisplayView.h>

// Forward declarations
class CGirderModelChildFrame;

/////////////////////////////////////////////////////////////////////////////
// CGirderModelSectionView view

class CGirderModelSectionView : public CDisplayView
{
   friend CGirderModelChildFrame;

protected:
	CGirderModelSectionView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CGirderModelSectionView)

// Attributes
public:
   CGirderModelChildFrame* GetFrame();

// Operations
public:
   void DoPrint(CDC* pDC, CPrintInfo* pInfo);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderModelSectionView)
	public:
	virtual void OnInitialUpdate() override;
	protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;
	virtual void OnDraw(CDC* pDC) override;
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CGirderModelSectionView();
#ifdef _DEBUG
	virtual void AssertValid() const override;
	virtual void Dump(CDumpContext& dc) const override;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CGirderModelSectionView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnEditPrestressing();
	afx_msg void OnViewSettings();
	afx_msg void OnEditStirrups();
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
   CGirderModelChildFrame* m_pFrame;
   bool m_bOnIntialUpdateComplete;

   void CreateDisplayLists();
   void UpdateDisplayObjects();
   void BuildPropertiesDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const pgsPointOfInterest& poi, iDisplayMgr* pDispMgr);
   void BuildSectionDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const pgsPointOfInterest& poi, iDisplayMgr* pDispMgr);
   void BuildLongitudinalJointDisplayObject(CPGSDocBase* pDoc, IBroker* pBroker, const pgsPointOfInterest& poi, iDisplayMgr* pDispMgr);
   void BuildStrandDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi,iDisplayMgr* pDispMgr);
   void BuildDuctDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi,iDisplayMgr* pDispMgr);
   void BuildLongReinfDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi,iDisplayMgr* pDispMgr);
   void BuildStrandCGDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const pgsPointOfInterest& poi, iDisplayMgr* pDispMgr);
   void BuildCGDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const pgsPointOfInterest& poi, iDisplayMgr* pDispMgr);
   void BuildDimensionDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi,iDisplayMgr* pDispMgr);

   virtual void UpdateDrawingScale();

   bool m_bUpdateError;
   std::_tstring m_ErrorMsg;

   // Store our current girder so we can ask frame if the selection has changed
   bool DidGirderSelectionChange();
   
   CGirderKey GetGirderKey();

   CGirderKey m_GirderKey;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERMODELSECTIONVIEW_H__E2B376CA_2D38_11D2_8EB4_006097DF3C68__INCLUDED_)
