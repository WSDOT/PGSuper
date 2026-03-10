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

class CGirderModelChildFrame;
class CPGSDocBase;

#include <DManip/DManip.h>

class CGirderModelElevationView : public CDisplayView
{
   friend CGirderModelChildFrame;

protected:
	CGirderModelElevationView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CGirderModelElevationView)

// Attributes
public:

   virtual DROPEFFECT CanDrop(COleDataObject* pDataObject,DWORD dwKeyState, const WBFL::Geometry::Point2d& point) override;

   pgsPointOfInterest GetCutLocation();

   CGirderModelChildFrame* GetFrame();

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
                                                      // some update/initialization can't be called until OnInitialUpdate runs
   long                    m_DisplayObjectID; // use to generate display object IDs

   std::shared_ptr<WBFL::DManip::iLegendDisplayObject> m_Legend;

   bool m_DoBlockUpdate;

   void BuildSupportDisplayObjects(         CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx);
   void BuildDropTargetDisplayObjects(      CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx);
   void BuildStrandDisplayObjects(          CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx);
   void BuildStrandCGDisplayObjects(        CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx);
   void BuildSegmentCGDisplayObjects(       CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx);
   void BuildSegmentDisplayObjects(         CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx);
   void BuildClosureJointDisplayObjects(    CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx);
   void BuildTendonDisplayObjects(          CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx);
   void BuildRebarDisplayObjects(           CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx);
   void BuildPointLoadDisplayObjects(       CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, bool* casesExist);
   void BuildDistributedLoadDisplayObjects( CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, bool* casesExist);
   void BuildMomentLoadDisplayObjects(      CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, bool* casesExist);
   void BuildLegendDisplayObjects(          CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, bool* casesExist);
   void BuildDimensionDisplayObjects(       CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx);
   void BuildSectionCutDisplayObjects(      CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx);
   void BuildStirrupDisplayObjects(         CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx);
   void BuildPropertiesDisplayObjects(      CPGSDocBase* pDoc, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, EventIndexType eventIdx);
   
   std::shared_ptr<WBFL::DManip::DimensionLine> BuildDimensionLine(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const WBFL::Geometry::Point2d& fromPoint, const WBFL::Geometry::Point2d& toPoint, Float64 dimension);
   std::shared_ptr<WBFL::DManip::iDisplayObject> BuildLine(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const std::vector<WBFL::Geometry::Point2d>& points, COLORREF color, UINT nWidth = 1);
   std::shared_ptr<WBFL::DManip::iDisplayObject> BuildLine(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const WBFL::Geometry::Point2d& fromPoint, const WBFL::Geometry::Point2d& toPoint, COLORREF color, UINT nWidth = 1);
   std::shared_ptr<WBFL::DManip::iDisplayObject> BuildDashLine(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const WBFL::Geometry::Point2d& fromPoint, const WBFL::Geometry::Point2d& toPoint, COLORREF color1, COLORREF color2);
   void BuildDebondTick(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const WBFL::Geometry::Point2d& tickPoint,COLORREF color);
   void UpdateDisplayObjects();

   bool m_bUpdateError;
   std::_tstring m_ErrorMsg;

   // Store our current girder so we can ask frame if the selection has changed
   bool DidGirderSelectionChange();

   CGirderKey m_GirderKey;

   CGirderKey GetGirderKey();

   CString GetSegmentTooltip(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CSegmentKey& segmentKey);
   CString GetClosureTooltip(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CClosureKey& closureKey);


   void CreateSegmentEndSupportDisplayObject(Float64 groupOffset,const CPrecastSegmentData* pSegment,pgsTypes::MemberEndType endType,EventIndexType eventIdx,const CTimelineManager* pTimelineMgr,std::shared_ptr<WBFL::DManip::iDisplayList> pDL);
   void CreateIntermediatePierDisplayObject(Float64 firstStation,const CPrecastSegmentData* pSegment,EventIndexType eventIdx,const CTimelineManager* pTimelineMgr,std::shared_ptr<WBFL::DManip::iDisplayList> pDL);
   void CreateIntermediateTemporarySupportDisplayObject(Float64 firstStation, const CPrecastSegmentData* pSegment, EventIndexType eventIdx, const CTimelineManager* pTimelineMgr, std::shared_ptr<WBFL::DManip::iDisplayList> pDL);

   // Returns the X location at the start of a segment
   Float64 GetSegmentStartLocation(const CSegmentKey& segmentKey);

   // Returns the X location at the start of a span
   Float64 GetSpanStartLocation(const CSpanKey& spanKey);

   // Returns the range of spans that are being displayed
   void GetSpanRange(std::shared_ptr<WBFL::EAF::Broker> pBroker,const CGirderKey& girderKey,SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx);
};
