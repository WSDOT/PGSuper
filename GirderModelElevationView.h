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

#if !defined(AFX_GIRDERMODELELEVATIONVIEW_H__E2B376CA_2D38_11D2_8EB4_006097DF3C68__INCLUDED_)
#define AFX_GIRDERMODELELEVATIONVIEW_H__E2B376CA_2D38_11D2_8EB4_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GirderModelElevationView.h : header file
//
class CGirderModelChildFrame;
class CPGSuperDoc;

#include <DManip\DManip.h>
#include <DManipTools\DManipTools.h>

interface IBroker;

/////////////////////////////////////////////////////////////////////////////
// CGirderModelElevationView view

class CGirderModelElevationView : public CDisplayView
{
   friend CGirderModelChildFrame;
protected:
	CGirderModelElevationView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CGirderModelElevationView)

// Attributes
public:

   virtual DROPEFFECT CanDrop(COleDataObject* pDataObject,DWORD dwKeyState,IPoint2d* point);
   virtual void OnDropped(COleDataObject* pDataObject,DROPEFFECT dropEffect,IPoint2d* point);


// Operations
public:
   void DoPrint(CDC* pDC, CPrintInfo* pInfo);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderModelElevationView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnDraw(CDC* pDC);
	//}}AFX_VIRTUAL

// Implementation
public:

protected:
	virtual ~CGirderModelElevationView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CGirderModelElevationView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLeftEnd();
	afx_msg void OnLeftHp();
	afx_msg void OnCenter();
	afx_msg void OnRightHp();
	afx_msg void OnRightEnd();
	afx_msg void OnUserCut();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnEditPrestressing();
	afx_msg void OnEditGirder();
	afx_msg void OnViewSettings();
	afx_msg void OnEditStirrups();
	afx_msg void OnGevCtxEditLoad();
	afx_msg void OnGevCtxDeleteLoad();
	afx_msg void OnDestroy();
   afx_msg BOOL OnMouseWheel(UINT nFlags,short zDelta,CPoint pt);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
   CGirderModelChildFrame* m_pFrame;
   bool                    m_First;
   long                    m_CurrID;

   CComPtr<iLegendDisplayObject> m_Legend;

   bool m_DoBlockUpdate;

   void BuildGirderDisplayObjects(CPGSuperDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr);
   void BuildSupportDisplayObjects(CPGSuperDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr);
   void BuildStrandDisplayObjects(CPGSuperDoc* pDoc, IBroker* pBroker,SpanIndexType span,GirderIndexType girder, iDisplayMgr* dispMgr);
   void BuildStrandCGDisplayObjects(CPGSuperDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr);
   void BuildRebarDisplayObjects(CPGSuperDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr);
   void BuildPointLoadDisplayObjects(CPGSuperDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr, bool* casesExist);
   void BuildDistributedLoadDisplayObjects(CPGSuperDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr, bool* casesExist);
   void BuildMomentLoadDisplayObjects(CPGSuperDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr, bool* casesExist);
   void BuildLegendDisplayObjects(CPGSuperDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr, bool* casesExist);
   void BuildDimensionDisplayObjects(CPGSuperDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr);
   void BuildSectionCutDisplayObjects(CPGSuperDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr);
   void BuildStirrupDisplayObjects(CPGSuperDoc* pDoc, IBroker* pBroker,SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr);
   
   iDimensionLine* BuildDimensionLine(iDisplayList* pDL, IPoint2d* fromPoint,IPoint2d* toPoint,double dimension);
   void BuildLine(iDisplayList* pDL, IPoint2d* fromPoint,IPoint2d* toPoint, COLORREF color);
   void BuildDebondTick(iDisplayList* pDL, IPoint2d* tickPoint,COLORREF color);
   void UpdateDisplayObjects();

   bool m_bUpdateError;
   std::string m_ErrorMsg;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERMODELELEVATIONVIEW_H__E2B376CA_2D38_11D2_8EB4_006097DF3C68__INCLUDED_)
