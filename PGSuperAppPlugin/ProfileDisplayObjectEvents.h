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

#pragma once

// ProfileDisplayObjectEvents.h : header file
//

#include <DManip\DManip.h>
interface IBroker;
class CBridgeModelViewChildFrame;


/////////////////////////////////////////////////////////////////////////////
// CProfileDisplayObjectEvents command target

class CProfileDisplayObjectEvents : public CCmdTarget
{
public:
	CProfileDisplayObjectEvents(IBroker* pDoc,CBridgeModelViewChildFrame* pFrame,iDisplayObject* pDO);
   ~CProfileDisplayObjectEvents();

   virtual void OnFinalRelease() override;

protected:

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

private:
   IBroker* m_pBroker;
   CBridgeModelViewChildFrame* m_pFrame;
   CComPtr<iDisplayObject> m_DispObj;

   void EditProfile();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
