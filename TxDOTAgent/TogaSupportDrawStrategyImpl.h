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

#ifndef INCLUDED_TogaSupportDrawStrategyIMPL_H_
#define INCLUDED_TogaSupportDrawStrategyIMPL_H_

#include "TogaSupportDrawStrategy.h"

class CTxDOTOptionalDesignDoc;

class CTogaSupportDrawStrategyImpl : public iSupportDrawStrategy
{
public:
   CTogaSupportDrawStrategyImpl(CTxDOTOptionalDesignDoc* pDoc);

   virtual void Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC) const override;
   virtual void DrawDragImage(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC, std::shared_ptr<const WBFL::DManip::iCoordinateMap> map, const POINT& dragStart, const POINT& dragPoint) const override;
   virtual void DrawHighlight(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC, bool bHighlight) const override;
   virtual WBFL::Geometry::Rect2d GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO) const override;

private:
   CTxDOTOptionalDesignDoc* m_pDoc;
   long                  m_SupportID;

   mutable WBFL::Geometry::Point2d m_CachePoint;

   void Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC, COLORREF outline, const WBFL::Geometry::Point2d& loc) const;
   void GetWSymbolSize(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, Float64* psx, Float64* psy) const;
   void GetLSymbolSize(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, long* psx, long* psy) const;
};

#endif // INCLUDED_TogaSupportDrawStrategyIMPL_H_