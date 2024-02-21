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

#if !defined(AFX_PIERDISPLAYOBJECTEVENTS_H__4C0EE0CB_964D_407D_9204_311964B859D5__INCLUDED_)
#define AFX_PIERDISPLAYOBJECTEVENTS_H__4C0EE0CB_964D_407D_9204_311964B859D5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PierDisplayObjectEvents.h : header file
//

#include <DManip/DisplayObjectEvents.h>
#include "BridgeModelViewChildFrame.h"
#include <PgsExt\BridgeDescription2.h>

/////////////////////////////////////////////////////////////////////////////
// CPierDisplayObjectEvents command target

class CPierDisplayObjectEvents : public WBFL::DManip::iDisplayObjectEvents
{
public:
	CPierDisplayObjectEvents(PierIndexType pierIdx,const CBridgeDescription2* pBridgeDesc,CBridgeModelViewChildFrame* pFrame);

   virtual bool OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, short zDelta, const POINT& point) override;
   virtual bool OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nChar, UINT nRepCnt, UINT nFlags) override;
   virtual bool OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, CWnd* pWnd, const POINT& point) override;
   virtual void OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO) override;
   virtual void OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, const WBFL::Geometry::Size2d& offset) override;
   virtual void OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO) override;
   virtual void OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO) override;
   virtual void OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO) override;
   virtual void OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO) override;
protected:
   PierIndexType m_PierIdx;
   const CBridgeDescription2* m_pBridgeDesc;
   CBridgeModelViewChildFrame* m_pFrame;

   void EditPier(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO);
   void SelectPier(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO);
   void SelectPrev(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO);
   void SelectNext(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PIERDISPLAYOBJECTEVENTS_H__4C0EE0CB_964D_407D_9204_311964B859D5__INCLUDED_)
