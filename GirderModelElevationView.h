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

#if !defined(AFX_GIRDERMODELELEVATIONVIEW_H__E2B376CA_2D38_11D2_8EB4_006097DF3C68__INCLUDED_)
#define AFX_GIRDERMODELELEVATIONVIEW_H__E2B376CA_2D38_11D2_8EB4_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GirderModelElevationView.h : header file
//

class CGirderModelChildFrame;
class CPGSDocBase;

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

   virtual DROPEFFECT CanDrop(COleDataObject* pDataObject,DWORD dwKeyState,IPoint2d* point) override;

   pgsPointOfInterest GetCutLocation();

// Operations
public:
   void DoPrint(CDC* pDC, CPrintInfo* pInfo);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderModelElevationView)
	public:
	virtual void OnInitialUpdate() override;
	protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;
	virtual void OnDraw(CDC* pDC) override;
	//}}AFX_VIRTUAL

// Implementation
public:

protected:
	virtual ~CGirderModelElevationView();
#ifdef _DEBUG
	virtual void AssertValid() const override;
	virtual void Dump(CDumpContext& dc) const override;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CGirderModelElevationView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnUserCut();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnEditPrestressing();
	afx_msg void OnViewSettings();
	afx_msg void OnEditStirrups();
	afx_msg void OnGevCtxEditLoad();
	afx_msg void OnGevCtxDeleteLoad();
	afx_msg void OnDestroy();
   afx_msg BOOL OnMouseWheel(UINT nFlags,short zDelta,CPoint pt);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
   CGirderModelChildFrame* m_pFrame; // pointer to parent frame object
   bool                    m_bOnIntialUpdateComplete; // false until the call to OnInitialUpdate is complete
                                                      // some update/initialiazation can't be called until OnInitialUpdate runs
   long                    m_DisplayObjectID; // use to generate display object IDs

   CComPtr<iLegendDisplayObject> m_Legend;

   bool m_DoBlockUpdate;

   void BuildSupportDisplayObjects(         CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr);
   void BuildDropTargetDisplayObjects(      CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr);
   void BuildStrandDisplayObjects(          CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr);
   void BuildStrandCGDisplayObjects(        CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr);
   void BuildSegmentDisplayObjects(         CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr);
   void BuildClosureJointDisplayObjects(    CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr);
   void BuildTendonDisplayObjects(          CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr);
   void BuildRebarDisplayObjects(           CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr);
   void BuildPointLoadDisplayObjects(       CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr, bool* casesExist);
   void BuildDistributedLoadDisplayObjects( CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr, bool* casesExist);
   void BuildMomentLoadDisplayObjects(      CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr, bool* casesExist);
   void BuildLegendDisplayObjects(          CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr, bool* casesExist);
   void BuildDimensionDisplayObjects(       CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr);
   void BuildSectionCutDisplayObjects(      CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr);
   void BuildStirrupDisplayObjects(         CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr);
   
   void BuildDimensionLine(iDisplayList* pDL, IPoint2d* fromPoint,IPoint2d* toPoint,Float64 dimension,iDimensionLine** ppDimLine = nullptr);
   void BuildLine(iDisplayList* pDL, IPoint2dCollection* points, COLORREF color,UINT nWidth=1,iDisplayObject** ppDO = nullptr);
   void BuildLine(iDisplayList* pDL, IPoint2d* fromPoint,IPoint2d* toPoint, COLORREF color,UINT nWidth=1,iDisplayObject** ppDO = nullptr);
   void BuildDashLine(iDisplayList* pDL, IPoint2d* fromPoint,IPoint2d* toPoint, COLORREF color1, COLORREF color2,iDisplayObject** ppDO = nullptr);
   void BuildDebondTick(iDisplayList* pDL, IPoint2d* tickPoint,COLORREF color);
   void UpdateDisplayObjects();

   bool m_bUpdateError;
   std::_tstring m_ErrorMsg;

   // Store our current girder so we can ask frame if the selection has changed
   bool DidGirderSelectionChange();

   CGirderKey m_GirderKey;

   CGirderKey GetGirderKey();

   CString GetSegmentTooltip(IBroker* pBroker, const CSegmentKey& segmentKey);
   CString GetClosureTooltip(IBroker* pBroker, const CClosureKey& closureKey);


   void CreateSegmentEndSupportDisplayObject(Float64 groupOffset,const CPrecastSegmentData* pSegment,pgsTypes::MemberEndType endType,EventIndexType eventIdx,const CTimelineManager* pTimelineMgr,iDisplayList* pDL);
   void CreateIntermediatePierDisplayObject(Float64 firstStation,const CPrecastSegmentData* pSegment,EventIndexType eventIdx,const CTimelineManager* pTimelineMgr,iDisplayList* pDL);
   void CreateIntermediateTemporarySupportDisplayObject(Float64 firstStation,const CPrecastSegmentData* pSegment,EventIndexType eventIdx,const CTimelineManager* pTimelineMgr,iDisplayList* pDL);

   // Returns the X location at the start of a segment
   Float64 GetSegmentStartLocation(const CSegmentKey& segmentKey);

   // Returns the X location at the start of a span
   Float64 GetSpanStartLocation(const CSpanKey& spanKey);

   // Returns the range of spans that are being displayed
   void GetSpanRange(IBroker* pBroker,const CGirderKey& girderKey,SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERMODELELEVATIONVIEW_H__E2B376CA_2D38_11D2_8EB4_006097DF3C68__INCLUDED_)
