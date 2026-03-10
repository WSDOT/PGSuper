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

#ifndef INCLUDED_SECTIONCUTDISPLAYOBJECT_H_
#define INCLUDED_SECTIONCUTDISPLAYOBJECT_H_

#include <DManip/DisplayObject.h>
#include "TogaSectionCutDrawStrategy.h" 
#include <PsgLib\Keys.h>

#include <DManip/DrawPointStrategy.h>
#include <DManip/DisplayObjectEvents.h>
#include <DManip/DragData.h>

class CTogaSectionCutDisplayImpl : public iTogaSectionCutDrawStrategy, public WBFL::DManip::iDrawPointStrategy, public WBFL::DManip::iDisplayObjectEvents, public WBFL::DManip::iDragData
{
public:
   CTogaSectionCutDisplayImpl();
   ~CTogaSectionCutDisplayImpl();

   virtual void SetColor(COLORREF color) override;
   virtual void Init(std::shared_ptr<WBFL::DManip::iPointDisplayObject> pDO, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CSegmentKey& segmentKey, iCutLocation* pCutLoc) override;

   virtual void Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC) const override;
   virtual void DrawDragImage(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC, std::shared_ptr<const WBFL::DManip::iCoordinateMap> map, const POINT& dragStart, const POINT& dragPoint) const override;
   virtual void DrawHighlight(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC, bool bHighlight) const override;
   virtual WBFL::Geometry::Rect2d GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO) const override;

   virtual bool OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, short zDelta, const POINT& point) override;
   virtual bool OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nChar, UINT nRepCnt, UINT nFlags) override;
   virtual bool OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, CWnd* pWnd, const POINT& point) override;
   virtual void OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO) override;
   virtual void OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, const WBFL::Geometry::Size2d& offset) override;
   virtual void OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO) override;
   virtual void OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO) override;
   virtual void OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO) override;
   virtual void OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO) override;
 
   // iDragData Implementation
   virtual UINT Format() override;
   virtual bool PrepareForDrag(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, std::shared_ptr<WBFL::DManip::iDragDataSink> pSink) override;
   virtual void OnDrop(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, std::shared_ptr<WBFL::DManip::iDragDataSource> pSource) override;

public: 
   static UINT ms_Format;

private:
   void Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC, COLORREF color, const WBFL::Geometry::Point2d& loc) const;
   void GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, Float64 Xgl,
      Float64* top, Float64* left, Float64* right, Float64* bottom) const;

   Float64 GetGirderHeight(Float64 distFromStartOfGirder) const;

   COLORREF           m_Color;
   CSegmentKey        m_SegmentKey;
   std::shared_ptr<WBFL::EAF::Broker>           m_pBroker;
   Float64            m_gdrLength;
   iCutLocation*      m_pCutLocation;
   
   mutable WBFL::Geometry::Point2d m_CachePoint;

   void PutPosition(Float64 pos);
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.




#endif // INCLUDED_SECTIONCUTDISPLAYOBJECT_H_