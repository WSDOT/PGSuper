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

#pragma once

#include <DManip/DisplayMgrEvents.h>
#include "PGSuperDocBase.h"
#include "GirderModelChildFrame.h"

class CPGSuperDoc;

class CGMDisplayMgrEventsImpl : public WBFL::DManip::iDisplayMgrEvents
{
private:
   CGMDisplayMgrEventsImpl(CPGSDocBase* pDoc, CGirderModelChildFrame* pFrame, CWnd* pParent, bool bGirderElevation);
public:
   static std::shared_ptr<CGMDisplayMgrEventsImpl> Create(CPGSDocBase* pDoc, CGirderModelChildFrame* pFrame, CWnd* pParent, bool bGirderElevation);
   ~CGMDisplayMgrEventsImpl() = default;

   virtual bool OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO, UINT nFlags, const POINT& point) override;
   virtual bool OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO, UINT nFlags, short zDelta, const POINT& point) override;
   virtual bool OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO, UINT nChar, UINT nRepCnt, UINT nFlags) override;
   virtual bool OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO, CWnd* pWnd, const POINT& point) override;

public:
   CPGSDocBase*        m_pDoc;
   CGirderModelChildFrame* m_pFrame;
   CWnd*                   m_pParent;
   bool                    m_bGirderElevation;
};
