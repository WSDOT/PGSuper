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

#ifndef INCLUDED_MOMENTLOADDRAWSTRATEGYIMPL_H_
#define INCLUDED_MOMENTLOADDRAWSTRATEGYIMPL_H_

#include "MomentLoadDrawStrategy.h" 
#include "GevEditLoad.h"

class CMomentLoadDrawStrategyImpl : public CCmdTarget
{
public:
   CMomentLoadDrawStrategyImpl();

   DECLARE_INTERFACE_MAP()

   BEGIN_INTERFACE_PART(Strategy,iMomentLoadDrawStrategy)
      STDMETHOD_(void,Init)(iPointDisplayObject* pDO, IBroker* pBroker, CMomentLoadData load, IndexType loadIndex, 
                            Float64 spanLength, Float64 maxMagnitude, COLORREF color);
   END_INTERFACE_PART(Strategy)


   BEGIN_INTERFACE_PART(DrawPointStrategy,iDrawPointStrategy)
      STDMETHOD_(void,Draw)(iPointDisplayObject* pDO,CDC* pDC) override;
      STDMETHOD_(void,DrawDragImage)(iPointDisplayObject* pDO,CDC* pDC, iCoordinateMap* map, const CPoint& dragStart, const CPoint& dragPoint) override;
      STDMETHOD_(void,DrawHighlite)(iPointDisplayObject* pDO,CDC* pDC,BOOL bHighlite) override;
      STDMETHOD_(void,GetBoundingBox)(iPointDisplayObject* pDO,IRect2d** rect) override;
   END_INTERFACE_PART(DrawPointStrategy)

   BEGIN_INTERFACE_PART(DisplayObjectEvents,iDisplayObjectEvents)
      STDMETHOD_(void,OnChanged)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnDragMoved)(iDisplayObject* pDO,ISize2d* offset) override;
      STDMETHOD_(void,OnMoved)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnCopied)(iDisplayObject* pDO) override;
      STDMETHOD_(bool,OnLButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseMove)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseWheel)(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point) override;
      STDMETHOD_(bool,OnKeyDown)(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags) override;
      STDMETHOD_(bool,OnContextMenu)(iDisplayObject* pDO,CWnd* pWnd,CPoint point) override;
      STDMETHOD_(void,OnSelect)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnUnselect)(iDisplayObject* pDO) override;
   END_INTERFACE_PART(DisplayObjectEvents)

   BEGIN_INTERFACE_PART(EditLoad,iGevEditLoad)
      STDMETHOD_(HRESULT,EditLoad)() override;
      STDMETHOD_(HRESULT,DeleteLoad)() override;
   END_INTERFACE_PART(EditLoad)


public: 
   static UINT ms_Format;

private:
   virtual void Draw(iPointDisplayObject* pDO,CDC* pDC,COLORREF color, IPoint2d* loc);
   // moment load diamter in difference coordinates
   void GetWSymbolSize(iCoordinateMap* pMap, Float64* pd);
   void GetLSymbolSize(iCoordinateMap* pMap, Uint32* pd);
   void GetTSymbolSize(iCoordinateMap* pMap, Uint32* pd);
   CMomentLoadData m_Load;
   IndexType m_LoadIndex;
   IBroker*       m_pBroker;
   COLORREF m_Color;
   Float64  m_MaxMagnitude;
   Float64  m_SpanLength;

   CComPtr<IPoint2d> m_CachePoint;

   void EditLoad();
   void DeleteLoad();
};

#endif // INCLUDED_MOMENTLOADDRAWSTRATEGYIMPL_H_