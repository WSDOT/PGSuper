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

#ifndef INCLUDED_POINTLOADDRAWSTRATEGYIMPL_H_
#define INCLUDED_POINTLOADDRAWSTRATEGYIMPL_H_

#include "PointLoadDrawStrategy.h" 
#include "GevEditLoad.h"

class CPointLoadDrawStrategyImpl : public CCmdTarget
{
public:
   CPointLoadDrawStrategyImpl();

   DECLARE_INTERFACE_MAP()

   BEGIN_INTERFACE_PART(Strategy,iPointLoadDrawStrategy)
      STDMETHOD_(void,Init)(iPointDisplayObject* pDO, IBroker* pBroker, const CPointLoadData& load, IndexType loadIndex, 
                            Float64 spanLength, Float64 maxMagnitude, COLORREF color);
   END_INTERFACE_PART(Strategy)


   BEGIN_INTERFACE_PART(DrawPointStrategy,iDrawPointStrategy)
      STDMETHOD_(void,Draw)(iPointDisplayObject* pDO,CDC* pDC);
      STDMETHOD_(void,DrawDragImage)(iPointDisplayObject* pDO,CDC* pDC, iCoordinateMap* map, const CPoint& dragStart, const CPoint& dragPoint);
      STDMETHOD_(void,DrawHighlite)(iPointDisplayObject* pDO,CDC* pDC,BOOL bHighlite);
      STDMETHOD_(void,GetBoundingBox)(iPointDisplayObject* pDO,IRect2d** rect);
   END_INTERFACE_PART(DrawPointStrategy)

   BEGIN_INTERFACE_PART(DisplayObjectEvents,iDisplayObjectEvents)
      STDMETHOD_(void,OnChanged)(iDisplayObject* pDO);
      STDMETHOD_(void,OnDragMoved)(iDisplayObject* pDO,ISize2d* offset);
      STDMETHOD_(void,OnMoved)(iDisplayObject* pDO);
      STDMETHOD_(void,OnCopied)(iDisplayObject* pDO);
      STDMETHOD_(bool,OnLButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point);
      STDMETHOD_(bool,OnLButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point);
      STDMETHOD_(bool,OnRButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point);
      STDMETHOD_(bool,OnRButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point);
      STDMETHOD_(bool,OnLButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point);
      STDMETHOD_(bool,OnRButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point);
      STDMETHOD_(bool,OnMouseMove)(iDisplayObject* pDO,UINT nFlags,CPoint point);
      STDMETHOD_(bool,OnMouseWheel)(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point);
      STDMETHOD_(bool,OnKeyDown)(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags);
      STDMETHOD_(bool,OnContextMenu)(iDisplayObject* pDO,CWnd* pWnd,CPoint point);
      STDMETHOD_(void,OnSelect)(iDisplayObject* pDO);
      STDMETHOD_(void,OnUnselect)(iDisplayObject* pDO);
   END_INTERFACE_PART(DisplayObjectEvents)

   BEGIN_INTERFACE_PART(EditLoad,iGevEditLoad)
      STDMETHOD_(HRESULT,EditLoad)();
      STDMETHOD_(HRESULT,DeleteLoad)();
   END_INTERFACE_PART(EditLoad)


public: 
   static UINT ms_Format;

private:
   void Draw(iPointDisplayObject* pDO,CDC* pDC,COLORREF color, IPoint2d* loc);
   // point load height and width in difference coordinates
   void GetWSymbolSize(iCoordinateMap* pMap, Float64* psx, Float64 *psy);
   void GetLSymbolSize(iCoordinateMap* pMap, long* psx, long* psy);
   void GetTSymbolSize(iCoordinateMap* pMap, long* psx, long* psy);
   CPointLoadData m_Load;
   IndexType m_LoadIndex;
   IBroker*       m_pBroker;
   COLORREF m_Color;
   Float64  m_MaxMagnitude;
   Float64  m_SpanLength;

   CComPtr<IPoint2d> m_CachePoint;

   void EditLoad();
   void DeleteLoad();
};

#endif // INCLUDED_POINTLOADDRAWSTRATEGYIMPL_H_