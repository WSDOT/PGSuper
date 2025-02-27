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

// GirderDisplayObjectEvents.h : header file
//

#include "BridgeModelViewChildFrame.h"
#include "GirderModelChildFrame.h"
#include <DManip\DManip.h>


/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewGirderDisplayObjectEvents command target

class CBridgePlanViewGirderDisplayObjectEvents : public CCmdTarget
{
public:
	CBridgePlanViewGirderDisplayObjectEvents(const CGirderKey& girderKey,GroupIndexType nGroups,GirderIndexType nGirderThisGroup,CBridgeModelViewChildFrame* pFrame);

protected:
   CGirderKey m_GirderKey;
   GroupIndexType m_nGroups;
   GirderIndexType m_nGirdersThisGroup;

   CBridgeModelViewChildFrame* m_pFrame;

	DECLARE_INTERFACE_MAP()

   BEGIN_INTERFACE_PART(Events,iDisplayObjectEvents)
      STDMETHOD_(bool,OnLButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseMove)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseWheel)(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point) override;
      STDMETHOD_(bool,OnKeyDown)(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags) override;
      STDMETHOD_(bool,OnContextMenu)(iDisplayObject* pDO,CWnd* pWnd,CPoint point) override;
      STDMETHOD_(void,OnChanged)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnDragMoved)(iDisplayObject* pDO,ISize2d* offset) override;
      STDMETHOD_(void,OnMoved)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnCopied)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnSelect)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnUnselect)(iDisplayObject* pDO) override;
   END_INTERFACE_PART(Events)

   void EditGirder(iDisplayObject* pDO);
   void SelectGirder(iDisplayObject* pDO);
   void SelectPrevGirder();
   void SelectNextGirder();
   void SelectSpan();
};


/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewSegmentDisplayObjectEvents command target

class CBridgePlanViewSegmentDisplayObjectEvents : public CCmdTarget
{
public:
	CBridgePlanViewSegmentDisplayObjectEvents(const CSegmentKey& segmentKey,CBridgeModelViewChildFrame* pFrame);

protected:
   CSegmentKey m_SegmentKey;

   GirderIndexType m_nGirders; // number of girders in the associated group
   SegmentIndexType m_nSegments; // number of segments in the associated girder 

   CBridgeModelViewChildFrame* m_pFrame;

	DECLARE_INTERFACE_MAP()

   BEGIN_INTERFACE_PART(Events,iDisplayObjectEvents)
      STDMETHOD_(bool,OnLButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseMove)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseWheel)(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point) override;
      STDMETHOD_(bool,OnKeyDown)(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags) override;
      STDMETHOD_(bool,OnContextMenu)(iDisplayObject* pDO,CWnd* pWnd,CPoint point) override;
      STDMETHOD_(void,OnChanged)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnDragMoved)(iDisplayObject* pDO,ISize2d* offset) override;
      STDMETHOD_(void,OnMoved)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnCopied)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnSelect)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnUnselect)(iDisplayObject* pDO) override;
   END_INTERFACE_PART(Events)

   void EditSegment(iDisplayObject* pDO);
   void SelectSegment(iDisplayObject* pDO);

   // select prev/next segment in this girderline (longitudinal selection)
   void SelectPrevSegment();
   void SelectNextSegment();

   // select corresponding segment in prev/next adjacent girderline (transverse selection)
   void SelectAdjacentPrevSegment();
   void SelectAdjacentNextSegment();
};

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionViewGirderDisplayObjectEvents command target

class CBridgeSectionViewGirderDisplayObjectEvents : public CCmdTarget
{
public:
	CBridgeSectionViewGirderDisplayObjectEvents(const CGirderKey& girderKey,GroupIndexType nGroups,GirderIndexType nGirdersThisGroup,CBridgeModelViewChildFrame* pFrame);

protected:
   CGirderKey m_GirderKey;
   GroupIndexType m_nGroups;
   GirderIndexType m_nGirdersThisGroup;

   CBridgeModelViewChildFrame* m_pFrame;
   CBridgeSectionView* m_pView;

	DECLARE_INTERFACE_MAP()

   BEGIN_INTERFACE_PART(Events,iDisplayObjectEvents)
      STDMETHOD_(bool,OnLButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseMove)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseWheel)(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point) override;
      STDMETHOD_(bool,OnKeyDown)(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags) override;
      STDMETHOD_(bool,OnContextMenu)(iDisplayObject* pDO,CWnd* pWnd,CPoint point) override;
      STDMETHOD_(void,OnChanged)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnDragMoved)(iDisplayObject* pDO,ISize2d* offset) override;
      STDMETHOD_(void,OnMoved)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnCopied)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnSelect)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnUnselect)(iDisplayObject* pDO) override;
   END_INTERFACE_PART(Events)

   void EditGirder(iDisplayObject* pDO);
   void SelectGirder(iDisplayObject* pDO);
   void SelectPrevGirder();
   void SelectNextGirder();
   void SelectSpan();
};

/////////////////////////////////////////////////////////////////////////////
// CGirderElevationViewSegmentDisplayObjectEvents command target

class CGirderElevationViewSegmentDisplayObjectEvents : public CCmdTarget
{
public:
   CGirderElevationViewSegmentDisplayObjectEvents(const CSegmentKey& segmentKey, CGirderModelChildFrame* pFrame);

protected:
   CSegmentKey m_SegmentKey;

   CGirderModelChildFrame* m_pFrame;

   DECLARE_INTERFACE_MAP()

   BEGIN_INTERFACE_PART(Events, iDisplayObjectEvents)
   STDMETHOD_(bool, OnLButtonDblClk)(iDisplayObject* pDO, UINT nFlags, CPoint point) override;
   STDMETHOD_(bool, OnLButtonDown)(iDisplayObject* pDO, UINT nFlags, CPoint point) override;
   STDMETHOD_(bool, OnLButtonUp)(iDisplayObject* pDO, UINT nFlags, CPoint point) override;
   STDMETHOD_(bool, OnRButtonDblClk)(iDisplayObject* pDO, UINT nFlags, CPoint point) override;
   STDMETHOD_(bool, OnRButtonDown)(iDisplayObject* pDO, UINT nFlags, CPoint point) override;
   STDMETHOD_(bool, OnRButtonUp)(iDisplayObject* pDO, UINT nFlags, CPoint point) override;
   STDMETHOD_(bool, OnMouseMove)(iDisplayObject* pDO, UINT nFlags, CPoint point) override;
   STDMETHOD_(bool, OnMouseWheel)(iDisplayObject* pDO, UINT nFlags, short zDelta, CPoint point) override;
   STDMETHOD_(bool, OnKeyDown)(iDisplayObject* pDO, UINT nChar, UINT nRepCnt, UINT nFlags) override;
   STDMETHOD_(bool, OnContextMenu)(iDisplayObject* pDO, CWnd* pWnd, CPoint point) override;
   STDMETHOD_(void, OnChanged)(iDisplayObject* pDO) override;
   STDMETHOD_(void, OnDragMoved)(iDisplayObject* pDO, ISize2d* offset) override;
   STDMETHOD_(void, OnMoved)(iDisplayObject* pDO) override;
   STDMETHOD_(void, OnCopied)(iDisplayObject* pDO) override;
   STDMETHOD_(void, OnSelect)(iDisplayObject* pDO) override;
   STDMETHOD_(void, OnUnselect)(iDisplayObject* pDO) override;
   END_INTERFACE_PART(Events)

   void EditSegment(iDisplayObject* pDO);
};

/////////////////////////////////////////////////////////////////////////////
// CGirderSectionViewSegmentDisplayObjectEvents command target

class CGirderSectionViewSegmentDisplayObjectEvents : public CCmdTarget
{
public:
   CGirderSectionViewSegmentDisplayObjectEvents(const pgsPointOfInterest& poi, CGirderModelChildFrame* pFrame);

protected:
   pgsPointOfInterest m_POI;

   CGirderModelChildFrame* m_pFrame;

   DECLARE_INTERFACE_MAP()

   BEGIN_INTERFACE_PART(Events, iDisplayObjectEvents)
      STDMETHOD_(bool, OnLButtonDblClk)(iDisplayObject* pDO, UINT nFlags, CPoint point) override;
   STDMETHOD_(bool, OnLButtonDown)(iDisplayObject* pDO, UINT nFlags, CPoint point) override;
   STDMETHOD_(bool, OnLButtonUp)(iDisplayObject* pDO, UINT nFlags, CPoint point) override;
   STDMETHOD_(bool, OnRButtonDblClk)(iDisplayObject* pDO, UINT nFlags, CPoint point) override;
   STDMETHOD_(bool, OnRButtonDown)(iDisplayObject* pDO, UINT nFlags, CPoint point) override;
   STDMETHOD_(bool, OnRButtonUp)(iDisplayObject* pDO, UINT nFlags, CPoint point) override;
   STDMETHOD_(bool, OnMouseMove)(iDisplayObject* pDO, UINT nFlags, CPoint point) override;
   STDMETHOD_(bool, OnMouseWheel)(iDisplayObject* pDO, UINT nFlags, short zDelta, CPoint point) override;
   STDMETHOD_(bool, OnKeyDown)(iDisplayObject* pDO, UINT nChar, UINT nRepCnt, UINT nFlags) override;
   STDMETHOD_(bool, OnContextMenu)(iDisplayObject* pDO, CWnd* pWnd, CPoint point) override;
   STDMETHOD_(void, OnChanged)(iDisplayObject* pDO) override;
   STDMETHOD_(void, OnDragMoved)(iDisplayObject* pDO, ISize2d* offset) override;
   STDMETHOD_(void, OnMoved)(iDisplayObject* pDO) override;
   STDMETHOD_(void, OnCopied)(iDisplayObject* pDO) override;
   STDMETHOD_(void, OnSelect)(iDisplayObject* pDO) override;
   STDMETHOD_(void, OnUnselect)(iDisplayObject* pDO) override;
   END_INTERFACE_PART(Events)

   void EditSegment(iDisplayObject* pDO);
};
