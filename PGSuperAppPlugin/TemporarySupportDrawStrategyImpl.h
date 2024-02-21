///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

class CTemporarySupportDrawStrategyImpl : public WBFL::DManip::iDrawPointStrategy
{
public:
   CTemporarySupportDrawStrategyImpl(pgsTypes::TemporarySupportType supportType,Float64 leftBrgOffset,Float64 rightBrgOffset);

   virtual void Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC) const override;
   virtual void DrawDragImage(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC, std::shared_ptr<const WBFL::DManip::iCoordinateMap> map, const POINT& dragStart, const POINT& dragPoint) const override;
   virtual void DrawHighlight(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC, bool bHighlight) const override;
   virtual WBFL::Geometry::Rect2d GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO) const override;

private:
   pgsTypes::TemporarySupportType m_SupportType;
   Float64 m_LeftBrgOffset;
   Float64 m_RightBrgOffset;

   mutable WBFL::Geometry::Point2d m_CachePoint;

   void Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC,COLORREF color,const WBFL::Geometry::Point2d& loc) const;
   void GetLSymbolSize(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, long* psx, long* psy) const;
   void DrawGround(CDC* pDC, long cx, long cy, long wid, long hgt) const;
   void DrawTowerSupport(CDC* pDC, long cx, long cy, long wid, long hgt) const;
   void DrawStrongBack(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC,COLORREF color, const WBFL::Geometry::Point2d& loc) const;
};

