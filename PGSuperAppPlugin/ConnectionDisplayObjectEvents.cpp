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

// ConnectionDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "ConnectionDisplayObjectEvents.h"
#include "pgsuperdoc.h"
#include <IFace\EditByUI.h>

#include <DManip/DisplayObject.h>
#include <DManip/DisplayList.h>
#include <DManip/DisplayMgr.h>
#include <DManip/DisplayView.h>

CConnectionDisplayObjectEvents::CConnectionDisplayObjectEvents(PierIndexType pierIdx)
{
   m_PierIdx = pierIdx;
}

void CConnectionDisplayObjectEvents::EditPier(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   auto pView = pDO->GetDisplayList()->GetDisplayMgr()->GetView();
   CDocument* pDoc = pView->GetDocument();

   ((CPGSuperDoc*)pDoc)->EditPierDescription(m_PierIdx,EPD_CONNECTION);
}

/////////////////////////////////////////////////////////////////////////////
// CConnectionDisplayObjectEvents message handlers
bool CConnectionDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   if (pDO->IsSelected())
   {
      EditPier(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

bool CConnectionDisplayObjectEvents::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CConnectionDisplayObjectEvents::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CConnectionDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CConnectionDisplayObjectEvents::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CConnectionDisplayObjectEvents::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CConnectionDisplayObjectEvents::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CConnectionDisplayObjectEvents::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CConnectionDisplayObjectEvents::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_RETURN )
   {
      EditPier(pDO);
      return true;
   }

   return false;
}

bool CConnectionDisplayObjectEvents::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   return false;
}

void CConnectionDisplayObjectEvents::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CConnectionDisplayObjectEvents::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
}

void CConnectionDisplayObjectEvents::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CConnectionDisplayObjectEvents::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CConnectionDisplayObjectEvents::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CConnectionDisplayObjectEvents::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

