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

// SpanDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"
#include "SpanDisplayObjectEvents.h"
#include "PGSuperDocBase.h"
#include <IFace\EditByUI.h>

#include <PgsExt\SpanData2.h>

#include <DManip/DisplayObject.h>
#include <DManip/DisplayList.h>
#include <DManip/DisplayMgr.h>
#include <DManip/DisplayView.h>


/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewSpanDisplayObjectEvents

CBridgePlanViewSpanDisplayObjectEvents::CBridgePlanViewSpanDisplayObjectEvents(SpanIndexType spanIdx,CBridgeModelViewChildFrame* pFrame)
{
   m_SpanIdx = spanIdx;
   m_pFrame  = pFrame;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CSpanData2* pSpan = pIBridgeDesc->GetSpan(spanIdx);
   m_TempSupports         = pSpan->GetTemporarySupports();
}

void CBridgePlanViewSpanDisplayObjectEvents::EditSpan(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   auto pView = pDO->GetDisplayList()->GetDisplayMgr()->GetView();
   CDocument* pDoc = pView->GetDocument();

   ((CPGSDocBase*)pDoc)->EditSpanDescription(m_SpanIdx,ESD_GENERAL);
}

void CBridgePlanViewSpanDisplayObjectEvents::SelectSpan(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   // do the selection at the frame level because it will do this view
   // and the other view
   m_pFrame->SelectSpan(m_SpanIdx);
}

void CBridgePlanViewSpanDisplayObjectEvents::SelectPrev(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   if ( m_TempSupports.size() == 0 )
   {
      PierIndexType prevPierIdx = (PierIndexType)(m_SpanIdx);
      m_pFrame->SelectPier(prevPierIdx);
   }
   else
   {
      // select temporary support in this span
      const CTemporarySupportData* pTS = m_TempSupports.back();
      m_pFrame->SelectTemporarySupport(pTS->GetID());
   }
}

void CBridgePlanViewSpanDisplayObjectEvents::SelectNext(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   if (m_TempSupports.size() == 0 )
   {
      // select pier at end of span
      PierIndexType nextPierIdx = (PierIndexType)(m_SpanIdx+1);
      m_pFrame->SelectPier(nextPierIdx);
   }
   else
   {
      // select temporary support in this span
      const CTemporarySupportData* pTS = m_TempSupports.front();
      m_pFrame->SelectTemporarySupport(pTS->GetID());
   }
}

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewSpanDisplayObjectEvents message handlers
bool CBridgePlanViewSpanDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   if (pDO->IsSelected())
   {
      EditSpan(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

bool CBridgePlanViewSpanDisplayObjectEvents::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CBridgePlanViewSpanDisplayObjectEvents::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgePlanViewSpanDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgePlanViewSpanDisplayObjectEvents::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CBridgePlanViewSpanDisplayObjectEvents::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgePlanViewSpanDisplayObjectEvents::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgePlanViewSpanDisplayObjectEvents::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CBridgePlanViewSpanDisplayObjectEvents::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_RETURN )
   {
      EditSpan(pDO);
      return true;
   }
   else if ( nChar == VK_LEFT )
   {
      SelectPrev(pDO);
      return true;
   }
   else if ( nChar == VK_RIGHT )
   {
      SelectNext(pDO);
      return true;
   }
   else if ( nChar == VK_UP || nChar == VK_DOWN )
   {
      m_pFrame->SelectGirder( CSegmentKey(m_SpanIdx,0,INVALID_INDEX) );
      return true;
   }
   else if ( nChar == VK_DELETE )
   {
      m_pFrame->PostMessage(WM_COMMAND,ID_DELETE,0);
      return true;
   }

   return false;
}

bool CBridgePlanViewSpanDisplayObjectEvents::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())

   if ( pDO->IsSelected() )
   {
      auto pView = pDO->GetDisplayList()->GetDisplayMgr()->GetView();
      CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
      pMenu->LoadMenu(IDR_SELECTED_SPAN_CONTEXT,nullptr);

      const std::map<IDType,IBridgePlanViewEventCallback*>& callbacks = pDoc->GetBridgePlanViewCallbacks();
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
      for ( ; callbackIter != callbackIterEnd; callbackIter++ )
      {
         IBridgePlanViewEventCallback* pCallback = callbackIter->second;
         pCallback->OnSpanContextMenu(m_SpanIdx,pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,m_pFrame);

      return true;
   }


   return false;
}

void CBridgePlanViewSpanDisplayObjectEvents::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgePlanViewSpanDisplayObjectEvents::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, const WBFL::Geometry::Size2d& offset)
{
}

void CBridgePlanViewSpanDisplayObjectEvents::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgePlanViewSpanDisplayObjectEvents::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgePlanViewSpanDisplayObjectEvents::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   SelectSpan(pDO);
}

void CBridgePlanViewSpanDisplayObjectEvents::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionViewSpanDisplayObjectEvents

CBridgeSectionViewSpanDisplayObjectEvents::CBridgeSectionViewSpanDisplayObjectEvents(Uint16 spanIdx,CBridgeModelViewChildFrame* pFrame)
{
   m_SpanIdx = spanIdx;
   m_pFrame  = pFrame;
}

void CBridgeSectionViewSpanDisplayObjectEvents::EditSpan(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   auto pView = pDO->GetDisplayList()->GetDisplayMgr()->GetView();
   CDocument* pDoc = pView->GetDocument();

   ((CPGSDocBase*)pDoc)->EditSpanDescription(m_SpanIdx,ESD_GENERAL);
}

void CBridgeSectionViewSpanDisplayObjectEvents::SelectSpan(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   // do the selection at the frame level because it will do this view
   // and the other view
   m_pFrame->SelectSpan(m_SpanIdx);
}

void CBridgeSectionViewSpanDisplayObjectEvents::SelectPrev(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   m_pFrame->SelectPier(m_SpanIdx);
}

void CBridgeSectionViewSpanDisplayObjectEvents::SelectNext(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   m_pFrame->SelectPier(m_SpanIdx+1);
}

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionViewSpanDisplayObjectEvents message handlers
bool CBridgeSectionViewSpanDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   if (pDO->IsSelected())
   {
      EditSpan(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

bool CBridgeSectionViewSpanDisplayObjectEvents::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CBridgeSectionViewSpanDisplayObjectEvents::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeSectionViewSpanDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeSectionViewSpanDisplayObjectEvents::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CBridgeSectionViewSpanDisplayObjectEvents::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeSectionViewSpanDisplayObjectEvents::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeSectionViewSpanDisplayObjectEvents::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CBridgeSectionViewSpanDisplayObjectEvents::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_RETURN )
   {
      EditSpan(pDO);
      return true;
   }
   else if ( nChar == VK_LEFT )
   {
      SelectPrev(pDO);
      return true;
   }
   else if ( nChar == VK_RIGHT )
   {
      SelectNext(pDO);
      return true;
   }
   else if ( nChar == VK_UP || nChar == VK_DOWN )
   {
      m_pFrame->SelectGirder( CSegmentKey(m_SpanIdx,0,INVALID_INDEX) );
      return true;
   }
   else if ( nChar == VK_DELETE )
   {
      m_pFrame->PostMessage(WM_COMMAND,ID_DELETE,0);
      return true;
   }

   return false;
}

bool CBridgeSectionViewSpanDisplayObjectEvents::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( pDO->IsSelected() )
   {
      CMenu menu;
      menu.LoadMenu(IDR_SELECTED_SPAN_CONTEXT);
      menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,pWnd->GetParent());

      return true;
   }

   return false;
}

void CBridgeSectionViewSpanDisplayObjectEvents::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgeSectionViewSpanDisplayObjectEvents::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
}

void CBridgeSectionViewSpanDisplayObjectEvents::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgeSectionViewSpanDisplayObjectEvents::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgeSectionViewSpanDisplayObjectEvents::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   SelectSpan(pDO);
}

void CBridgeSectionViewSpanDisplayObjectEvents::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}
