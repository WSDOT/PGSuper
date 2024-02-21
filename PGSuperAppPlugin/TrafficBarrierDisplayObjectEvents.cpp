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

// TrafficBarrierDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TrafficBarrierDisplayObjectEvents.h"
#include "BridgeModelViewChildFrame.h"
#include "PGSuperDocBase.h"
#include "BridgeSectionView.h"

#include <PgsExt\BridgeDescription2.h>

#include <DManip/DisplayObject.h>

CTrafficBarrierDisplayObjectEvents::CTrafficBarrierDisplayObjectEvents(IBroker* pBroker, CBridgeModelViewChildFrame* pFrame,pgsTypes::TrafficBarrierOrientation orientation)
{
   m_pBroker = pBroker;
   m_pFrame = pFrame;
   m_TrafficBarrierOrientation = orientation;
}

CTrafficBarrierDisplayObjectEvents::~CTrafficBarrierDisplayObjectEvents()
{
}

void CTrafficBarrierDisplayObjectEvents::EditBarrier()
{
   m_pFrame->PostMessage(WM_COMMAND,ID_PROJECT_BARRIER,0);
}

void CTrafficBarrierDisplayObjectEvents::SelectPrev()
{
   if (m_TrafficBarrierOrientation == pgsTypes::tboLeft)
   {
      // select last girder
      GET_IFACE(IBridgeDescription, pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      GroupIndexType grpIdx = m_pFrame->GetBridgeSectionView()->GetGroupIndex();
      GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
      m_pFrame->SelectGirder(CSegmentKey(grpIdx, nGirders - 1, INVALID_INDEX));
   }
   else
   {
      m_pFrame->SelectDeck();
   }
}

void CTrafficBarrierDisplayObjectEvents::SelectNext()
{
   if (m_TrafficBarrierOrientation == pgsTypes::tboLeft)
   {
      m_pFrame->SelectDeck();
   }
   else
   {
      // select first girder
      GroupIndexType grpIdx = m_pFrame->GetBridgeSectionView()->GetGroupIndex();
      m_pFrame->SelectGirder(CSegmentKey(grpIdx, 0, INVALID_INDEX));
   }
}

/////////////////////////////////////////////////////////////////////////////
// CTrafficBarrierDisplayObjectEvents message handlers
STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags,const POINT& point)
{
   if (pDO->IsSelected())
   {
      EditBarrier();
      return true;
   }
   else
   {
      return false;
   }
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_RETURN )
   {
      EditBarrier();
      return true;
   }
   else if (nChar == VK_LEFT)
   {
      SelectPrev();
      return true;
   }
   else if (nChar == VK_RIGHT)
   {
      SelectNext();
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   if ( pDO->IsSelected() )
   {
      auto pView = pDO->GetDisplayList()->GetDisplayMgr()->GetView();
      CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
      pMenu->AppendMenu(ID_PROJECT_BARRIER, _T("Edit Railing System"), nullptr);

      bool bResult = false;
      if ( 0 < pMenu->GetMenuItemCount() )
      {
         pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,m_pFrame);
         bResult = true;
      }

      delete pMenu;

      return bResult;
   }

   return false;
}

void CTrafficBarrierDisplayObjectEvents::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CTrafficBarrierDisplayObjectEvents::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
}

void CTrafficBarrierDisplayObjectEvents::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CTrafficBarrierDisplayObjectEvents::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CTrafficBarrierDisplayObjectEvents::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   m_pFrame->ClearSelection();
   m_pFrame->SelectTrafficBarrier(m_TrafficBarrierOrientation);
}

void CTrafficBarrierDisplayObjectEvents::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}
