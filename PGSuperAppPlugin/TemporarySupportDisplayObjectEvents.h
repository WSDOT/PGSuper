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

#pragma once

#include <DManip/DisplayObjectEvents.h>
#include "BridgeModelViewChildFrame.h"
#include <PgsExt\TemporarySupportData.h>

class CTemporarySupportDisplayObjectEvents : public WBFL::DManip::iDisplayObjectEvents
{
public:
	CTemporarySupportDisplayObjectEvents(const CTemporarySupportData* pTS,CBridgeModelViewChildFrame* pFrame);

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
   const CTemporarySupportData* m_pTS;
   CBridgeModelViewChildFrame* m_pFrame;

   const CTemporarySupportData* m_pPrevTS;
   const CTemporarySupportData* m_pNextTS;

   const CPierData2* m_pPrevPier;
   const CPierData2* m_pNextPier;

   void EditTemporarySupport(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO);
   void SelectTemporarySupport(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO);
   void SelectPrev(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO);
   void SelectNext(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO);
};
