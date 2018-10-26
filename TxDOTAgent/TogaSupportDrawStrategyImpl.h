///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <DManip\DManip.h>

class CTxDOTOptionalDesignDoc;

class CTogaSupportDrawStrategyImpl : public CCmdTarget
{
public:
   CTogaSupportDrawStrategyImpl(CTxDOTOptionalDesignDoc* pDoc);


   DECLARE_INTERFACE_MAP()

   BEGIN_INTERFACE_PART(Strategy,iTogaSupportDrawStrategy)
//      STDMETHOD_(void,SetSupport)(ISupport* jnt, long supportID);
   END_INTERFACE_PART(Strategy)

   BEGIN_INTERFACE_PART(DrawPointStrategy,iDrawPointStrategy)
      STDMETHOD_(void,Draw)(iPointDisplayObject* pDO,CDC* pDC);
      STDMETHOD_(void,DrawDragImage)(iPointDisplayObject* pDO,CDC* pDC, iCoordinateMap* map, const CPoint& dragStart, const CPoint& dragPoint);
      STDMETHOD_(void,DrawHighlite)(iPointDisplayObject* pDO,CDC* pDC,BOOL bHighlite);
      STDMETHOD_(void,GetBoundingBox)(iPointDisplayObject* pDO, IRect2d** rect);
   END_INTERFACE_PART(DrawPointStrategy)

private:
   CTxDOTOptionalDesignDoc* m_pDoc;
//   CComPtr<ISupport>     m_Support;
   long                  m_SupportID;

   CComPtr<IPoint2d> m_CachePoint;

   virtual void Draw(iPointDisplayObject* pDO,CDC* pDC,COLORREF color,IPoint2d* loc);
   void GetWSymbolSize(iCoordinateMap* pMap, Float64* psx, Float64 *psy);
   void GetLSymbolSize(iCoordinateMap* pMap, long* psx, long* psy);
};

#endif // INCLUDED_TogaSupportDrawStrategyIMPL_H_