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

#include "SectionCutDrawStrategy.h" 
#include <PsgLib\PointOfInterest.h>

#include <DManip/DrawPointStrategy.h>
#include <DManip/DisplayObjectEvents.h>
#include <DManip/DragData.h>

class CSectionCutDisplayImpl : public iSectionCutDrawStrategy, public WBFL::DManip::iDrawPointStrategy, public WBFL::DManip::iDisplayObjectEvents, public WBFL::DManip::iDragData
{
public:
   CSectionCutDisplayImpl();
   ~CSectionCutDisplayImpl();

   void SetColor(COLORREF color) override;
   void Init(std::shared_ptr<WBFL::DManip::iPointDisplayObject> pDO, std::weak_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey, iCutLocation* pCutLoc) override;
   pgsPointOfInterest GetCutPOI(Float64 Xgl) const override;

   void Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC) const override;
   void DrawDragImage(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC, std::shared_ptr<const WBFL::DManip::iCoordinateMap> map, const POINT& dragStart, const POINT& dragPoint) const override;
   void DrawHighlight(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC, bool bHighlight) const override;
   WBFL::Geometry::Rect2d GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO) const override;

   bool OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   bool OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   bool OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   bool OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   bool OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   bool OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   bool OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   bool OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, short zDelta, const POINT& point) override;
   bool OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nChar, UINT nRepCnt, UINT nFlags) override;
   bool OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, CWnd* pWnd, const POINT& point) override;
   void OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO) override;
   void OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, const WBFL::Geometry::Size2d& offset) override;
   void OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO) override;
   void OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO) override;
   void OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO) override;
   void OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO) override;

   // iDragData Implementation
   UINT Format() override;
   bool PrepareForDrag(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, std::shared_ptr<WBFL::DManip::iDragDataSink> pSink) override;
   void OnDrop(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, std::shared_ptr<WBFL::DManip::iDragDataSource> pSource) override;

public: 
   static UINT ms_Format;

private:
   void Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC,COLORREF color, const WBFL::Geometry::Point2d& loc) const;
   void GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, Float64 Xgl, 
                       Float64* top, Float64* left, Float64* right, Float64* bottom) const;

   COLORREF           m_Color;
   CGirderKey         m_GirderKey;
   std::weak_ptr<WBFL::EAF::Broker>           m_pBroker;
   inline std::shared_ptr<WBFL::EAF::Broker> GetBroker() const { return m_pBroker.lock(); }
   Float64            m_MinCutLocation;  // in girder coordinates when a single group is displayed, otherwise in girderline coordinate (for ALL_GROUPS)
   Float64            m_MaxCutLocation; // in girder coordinates when a single group is displayed, otherwise in girderline coordinate (for ALL_GROUPS)
   iCutLocation*      m_pCutLocation;
   
   mutable WBFL::Geometry::Point2d m_CachePoint;

   void PutPosition(Float64 Xgl);
};
