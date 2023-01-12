///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// TemporarySupportDisplayObjectEvents.h : header file
//

#include <DManip\DManip.h>
#include "BridgeModelViewChildFrame.h"
#include <PgsExt\TemporarySupportData.h>

/////////////////////////////////////////////////////////////////////////////
// CTemporarySupportDisplayObjectEvents command target

class CTemporarySupportDisplayObjectEvents : public CCmdTarget
{
public:
	CTemporarySupportDisplayObjectEvents(const CTemporarySupportData* pTS,CBridgeModelViewChildFrame* pFrame);

protected:
   const CTemporarySupportData* m_pTS;
   CBridgeModelViewChildFrame* m_pFrame;

   const CTemporarySupportData* m_pPrevTS;
   const CTemporarySupportData* m_pNextTS;

   const CPierData2* m_pPrevPier;
   const CPierData2* m_pNextPier;

	DECLARE_INTERFACE_MAP()

   BEGIN_INTERFACE_PART(Events,iDisplayObjectEvents)
      STDMETHOD_(bool,OnLButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point);
      STDMETHOD_(bool,OnLButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point);
      STDMETHOD_(bool,OnLButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point);
      STDMETHOD_(bool,OnRButtonDblClk)(iDisplayObject* pDO,UINT nFlags,CPoint point);
      STDMETHOD_(bool,OnRButtonDown)(iDisplayObject* pDO,UINT nFlags,CPoint point);
      STDMETHOD_(bool,OnRButtonUp)(iDisplayObject* pDO,UINT nFlags,CPoint point);
      STDMETHOD_(bool,OnMouseMove)(iDisplayObject* pDO,UINT nFlags,CPoint point);
      STDMETHOD_(bool,OnMouseWheel)(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point);
      STDMETHOD_(bool,OnKeyDown)(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags);
      STDMETHOD_(bool,OnContextMenu)(iDisplayObject* pDO,CWnd* pWnd,CPoint point);
      STDMETHOD_(void,OnChanged)(iDisplayObject* pDO);
      STDMETHOD_(void,OnDragMoved)(iDisplayObject* pDO,ISize2d* offset);
      STDMETHOD_(void,OnMoved)(iDisplayObject* pDO);
      STDMETHOD_(void,OnCopied)(iDisplayObject* pDO);
      STDMETHOD_(void,OnSelect)(iDisplayObject* pDO);
      STDMETHOD_(void,OnUnselect)(iDisplayObject* pDO);
   END_INTERFACE_PART(Events)

   void EditTemporarySupport(iDisplayObject* pDO);
   void SelectTemporarySupport(iDisplayObject* pDO);
   void SelectPrev(iDisplayObject* pDO);
   void SelectNext(iDisplayObject* pDO);
};
