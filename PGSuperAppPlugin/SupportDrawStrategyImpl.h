///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#ifndef INCLUDED_SUPPORTDRAWSTRATEGYIMPL_H_
#define INCLUDED_SUPPORTDRAWSTRATEGYIMPL_H_

#include "SupportDrawStrategy.h"
#include <DManip\DManip.h>

class CPierData2;

class CSupportDrawStrategyImpl : public CCmdTarget
{
public:
   CSupportDrawStrategyImpl(const CPierData2* pPier);


   DECLARE_INTERFACE_MAP()

   BEGIN_INTERFACE_PART(Strategy,iSupportDrawStrategy)
//      STDMETHOD_(void,SetSupport)(ISupport* jnt, long supportID);
   END_INTERFACE_PART(Strategy)

   BEGIN_INTERFACE_PART(DrawPointStrategy,iDrawPointStrategy)
      STDMETHOD_(void,Draw)(iPointDisplayObject* pDO,CDC* pDC);
      STDMETHOD_(void,DrawDragImage)(iPointDisplayObject* pDO,CDC* pDC, iCoordinateMap* map, const CPoint& dragStart, const CPoint& dragPoint);
      STDMETHOD_(void,DrawHighlite)(iPointDisplayObject* pDO,CDC* pDC,BOOL bHighlite);
      STDMETHOD_(void,GetBoundingBox)(iPointDisplayObject* pDO, IRect2d** rect);
   END_INTERFACE_PART(DrawPointStrategy)

private:
   const CPierData2* m_pPier;

   CComPtr<IPoint2d> m_CachePoint;

   void Draw(iPointDisplayObject* pDO,CDC* pDC,COLORREF outline_color,COLORREF fill_color,IPoint2d* loc);
   void DrawGround(CDC* pDC, long cx, long cy, long wid, long hgt);
   void DrawFixedSupport(CDC* pDC, long cx, long cy, long wid, long hgt);
   void DrawPinnedSupport(CDC* pDC, long cx, long cy, long wid, long hgt);
   void DrawRollerSupport(CDC* pDC, long cx, long cy, long wid, long hgt);
   void GetWSymbolSize(iCoordinateMap* pMap, Float64* psx, Float64 *psy);
   void GetLSymbolSize(iCoordinateMap* pMap, long* psx, long* psy);
};

#endif // INCLUDED_SUPPORTDRAWSTRATEGYIMPL_H_