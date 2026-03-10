///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include <DManip/DManip.h>

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
   void BuildPropertiesDisplayObjects(CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsPointOfInterest& poi);
   void BuildSectionDisplayObjects(CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsPointOfInterest& poi);
   void BuildLongitudinalJointDisplayObject(CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsPointOfInterest& poi);
   void BuildStrandDisplayObjects(CPGSDocBase* pDoc,std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsPointOfInterest& poi);
   std::shared_ptr<WBFL::DManip::iDisplayObject> GetDuctDisplayObject(IntervalIndexType intervalIdx, IntervalIndexType ptIntervalIdx, const WBFL::Geometry::Point2d& pntDuct, Float64 ductDiameter, StrandIndexType nStrands, COLORREF fillColor,COLORREF borderColor);
   void BuildDuctDisplayObjects(CPGSDocBase* pDoc,std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsPointOfInterest& poi);
   void BuildLongReinfDisplayObjects(CPGSDocBase* pDoc,std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsPointOfInterest& poi);
   void BuildStrandCGDisplayObjects(CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsPointOfInterest& poi);
   void BuildCGDisplayObjects(CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsPointOfInterest& poi);
   void BuildDimensionDisplayObjects(CPGSDocBase* pDoc,std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsPointOfInterest& poi);
   void CreateLineDisplayObject(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const WBFL::Geometry::Point2d& start, const WBFL::Geometry::Point2d& end, COLORREF color, WBFL::DManip::LineStyleType lineStyle);

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

