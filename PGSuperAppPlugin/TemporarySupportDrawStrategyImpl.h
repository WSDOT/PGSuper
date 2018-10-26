///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#include <DManip\DManip.h>

class CTemporarySupportDrawStrategyImpl : public CCmdTarget
{
public:
   CTemporarySupportDrawStrategyImpl(pgsTypes::TemporarySupportType supportType,Float64 leftBrgOffset,Float64 rightBrgOffset);

   DECLARE_INTERFACE_MAP()

   BEGIN_INTERFACE_PART(DrawPointStrategy,iDrawPointStrategy)
      STDMETHOD_(void,Draw)(iPointDisplayObject* pDO,CDC* pDC);
      STDMETHOD_(void,DrawDragImage)(iPointDisplayObject* pDO,CDC* pDC, iCoordinateMap* map, const CPoint& dragStart, const CPoint& dragPoint);
      STDMETHOD_(void,DrawHighlite)(iPointDisplayObject* pDO,CDC* pDC,BOOL bHighlite);
      STDMETHOD_(void,GetBoundingBox)(iPointDisplayObject* pDO, IRect2d** rect);
   END_INTERFACE_PART(DrawPointStrategy)

private:
   pgsTypes::TemporarySupportType m_SupportType;
   Float64 m_LeftBrgOffset;
   Float64 m_RightBrgOffset;

   CComPtr<IPoint2d> m_CachePoint;

   virtual void Draw(iPointDisplayObject* pDO,CDC* pDC,COLORREF color,IPoint2d* loc);
   void GetLSymbolSize(iCoordinateMap* pMap, long* psx, long* psy);

   void DrawGround(CDC* pDC, long cx, long cy, long wid, long hgt);
   void DrawTowerSupport(CDC* pDC, long cx, long cy, long wid, long hgt);

   void DrawStrongBack(iPointDisplayObject* pDO,CDC* pDC,COLORREF color,IPoint2d* loc);
};

