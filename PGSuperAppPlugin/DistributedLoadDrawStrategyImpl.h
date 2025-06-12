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

#include <DManip/DrawPointStrategy.h>
#include <DManip/DisplayObjectEvents.h>
#include <DManip/GravityWellStrategy.h>
#include "DistributedLoadDrawStrategy.h" 
#include "GevEditLoad.h"

class CDistributedLoadDrawStrategyImpl : public iDistributedLoadDrawStrategy, public WBFL::DManip::iDrawPointStrategy, public WBFL::DManip::iDisplayObjectEvents, public iGevEditLoad, public WBFL::DManip::iGravityWellStrategy
{
public:
   CDistributedLoadDrawStrategyImpl();

   virtual void Init(std::shared_ptr<WBFL::DManip::iPointDisplayObject> pDO, std::shared_ptr<WBFL::EAF::Broker> pBroker, CDistributedLoadData load, IndexType loadIndex,
      Float64 loadLength, Float64 spanLength, Float64 maxMagnitude, COLORREF color) override;


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

   void EditLoad() override;
   void DeleteLoad() override;

   void GetGravityWell(std::shared_ptr<const WBFL::DManip::iDisplayObject> pDO, CRgn* pRgn) override;


public: 
   static UINT ms_Format;

private:
   virtual void Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC,COLORREF color, const WBFL::Geometry::Point2d& loc) const;
   // point load height and width in difference coordinates
   void GetWLoadHeight(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, Float64* psx, Float64 *psy) const;
   void GetLLoadHeight(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, long* psx, long* psy) const;
   void GetTLoadHeight(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, long* psx, long* psy) const;
   std::shared_ptr<WBFL::EAF::Broker> GetBroker() { return m_pBroker; }

   CDistributedLoadData m_Load;
   IndexType  m_LoadIndex;
   std::shared_ptr<WBFL::EAF::Broker>       m_pBroker;
   COLORREF m_Color;
   Float64  m_StartLoc;
   Float64  m_EndLoc;
   Float64  m_MaxMagnitude;
   Float64  m_SpanLength;
   Float64  m_LoadLength;
   Float64  m_ArrowSpacing;

   mutable WBFL::Geometry::Point2d m_ReusablePoint;
   mutable WBFL::Geometry::Point2d m_ReusableRect;
};
