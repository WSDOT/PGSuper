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

#if !defined(AFX_SpanDISPLAYOBJECTEVENTS_H__4C0EE0CB_964D_407D_9204_311964B859D5__INCLUDED_)
#define AFX_SpanDISPLAYOBJECTEVENTS_H__4C0EE0CB_964D_407D_9204_311964B859D5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpanDisplayObjectEvents.h : header file
//

#include <DManip\DManip.h>
#include "BridgeModelViewChildFrame.h"
#include <PgsExt\TemporarySupportData.h>

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewSpanDisplayObjectEvents command target

class CBridgePlanViewSpanDisplayObjectEvents : public CCmdTarget
{
public:
	CBridgePlanViewSpanDisplayObjectEvents(SpanIndexType spanIdx,CBridgeModelViewChildFrame* pFrame);

protected:
   SpanIndexType m_SpanIdx;
   CBridgeModelViewChildFrame* m_pFrame;
   std::vector<const CTemporarySupportData*> m_TempSupports;

	DECLARE_INTERFACE_MAP()

   BEGIN_INTERFACE_PART(Events,iDisplayObjectEvents)
      STDMETHOD_(bool,OnLButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseMove)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseWheel)(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point) override;
      STDMETHOD_(bool,OnKeyDown)(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags) override;
      STDMETHOD_(bool,OnContextMenu)(iDisplayObject* pDO,CWnd* pWnd,CPoint point) override;
      STDMETHOD_(void,OnChanged)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnDragMoved)(iDisplayObject* pDO,ISize2d* offset) override;
      STDMETHOD_(void,OnMoved)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnCopied)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnSelect)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnUnselect)(iDisplayObject* pDO) override;
   END_INTERFACE_PART(Events)

   void EditSpan(iDisplayObject* pDO);
   void SelectSpan(iDisplayObject* pDO);
   void SelectPrev(iDisplayObject* pDO);
   void SelectNext(iDisplayObject* pDO);
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionViewSpanDisplayObjectEvents command target

class CBridgeSectionViewSpanDisplayObjectEvents : public CCmdTarget
{
public:
	CBridgeSectionViewSpanDisplayObjectEvents(Uint16 spanIdx,CBridgeModelViewChildFrame* pFrame);

protected:
   Uint16 m_SpanIdx;
   CBridgeModelViewChildFrame* m_pFrame;

	DECLARE_INTERFACE_MAP()

   BEGIN_INTERFACE_PART(Events,iDisplayObjectEvents)
      STDMETHOD_(bool,OnLButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseMove)(iDisplayObject* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseWheel)(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point) override;
      STDMETHOD_(bool,OnKeyDown)(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags) override;
      STDMETHOD_(bool,OnContextMenu)(iDisplayObject* pDO,CWnd* pWnd,CPoint point) override;
      STDMETHOD_(void,OnChanged)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnDragMoved)(iDisplayObject* pDO,ISize2d* offset) override;
      STDMETHOD_(void,OnMoved)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnCopied)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnSelect)(iDisplayObject* pDO) override;
      STDMETHOD_(void,OnUnselect)(iDisplayObject* pDO) override;
   END_INTERFACE_PART(Events)

   void EditSpan(iDisplayObject* pDO);
   void SelectSpan(iDisplayObject* pDO);
   void SelectPrev(iDisplayObject* pDO);
   void SelectNext(iDisplayObject* pDO);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SpanDISPLAYOBJECTEVENTS_H__4C0EE0CB_964D_407D_9204_311964B859D5__INCLUDED_)
