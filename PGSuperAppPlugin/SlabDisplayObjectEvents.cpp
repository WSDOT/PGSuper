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

// SlabDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"
#include "SlabDisplayObjectEvents.h"
#include "BridgeSectionCutDisplayImpl.h"
#include "BridgeModelViewChildFrame.h"
#include "mfcdual.h"
#include "PGSuperDoc.h"
#include <IFace\Bridge.h>
#include <IFace\EditByUI.h>

#include <DManip/DisplayObject.h>
#include <DManip/DisplayList.h>
#include <DManip/DisplayMgr.h>
#include <DManip/DisplayView.h>
#include <DManip/PointDisplayObject.h>
#include <DManip/ShapeDrawStrategy.h>
#include <DManip/CompositeDisplayObject.h>


/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewSlabDisplayObjectEvents

CBridgePlanViewSlabDisplayObjectEvents::CBridgePlanViewSlabDisplayObjectEvents(CPGSDocBase* pDoc,IBroker* pBroker, CBridgeModelViewChildFrame* pFrame,bool bFillIfNotSelected)
{
   m_pDoc = pDoc;
   m_pBroker = pBroker;
   m_pFrame = pFrame;
   m_bFillIfNotSelected = bFillIfNotSelected;

   GET_IFACE(IBridge,pBridge);
   m_nPiers = pBridge->GetPierCount();
}

void CBridgePlanViewSlabDisplayObjectEvents::EditSlab()
{
   m_pDoc->EditBridgeDescription(EBD_DECK);
}

void CBridgePlanViewSlabDisplayObjectEvents::SelectPrev()
{
   m_pFrame->SelectAlignment();
}

void CBridgePlanViewSlabDisplayObjectEvents::SelectNext()
{
   m_pFrame->SelectAlignment();
}

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewSlabDisplayObjectEvents message handlers
bool CBridgePlanViewSlabDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   if (pDO->IsSelected())
   {
      EditSlab();
      return true;
   }
   else
   {
      return false;
   }
}

bool CBridgePlanViewSlabDisplayObjectEvents::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true;
}

bool CBridgePlanViewSlabDisplayObjectEvents::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgePlanViewSlabDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgePlanViewSlabDisplayObjectEvents::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true;
}

bool CBridgePlanViewSlabDisplayObjectEvents::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgePlanViewSlabDisplayObjectEvents::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgePlanViewSlabDisplayObjectEvents::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CBridgePlanViewSlabDisplayObjectEvents::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_RETURN )
   {
      EditSlab();
      return true;
   }
   else if ( nChar == VK_LEFT )
   {
      SelectPrev();
      return true;
   }
   else if ( nChar == VK_RIGHT )
   {
      SelectNext();
      return true;
   }
   else if ( nChar == VK_UP || nChar == VK_DOWN )
   {
      m_pFrame->SelectGirder( CSegmentKey(0,0,INVALID_INDEX) );
      return true;
   }

   return false;
}

bool CBridgePlanViewSlabDisplayObjectEvents::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( pDO->IsSelected() )
   {
      auto pView = pDO->GetDisplayList()->GetDisplayMgr()->GetView();
      CPGSuperDoc* pDoc = (CPGSuperDoc*)pView->GetDocument();

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
      pMenu->LoadMenu(IDR_SELECTED_DECK_CONTEXT,nullptr);

      const std::map<IDType,IBridgePlanViewEventCallback*>& callbacks = pDoc->GetBridgePlanViewCallbacks();
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
      for ( ; callbackIter != callbackIterEnd; callbackIter++ )
      {
         IBridgePlanViewEventCallback* pCallback = callbackIter->second;
         pCallback->OnDeckContextMenu(pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,m_pFrame);

      return true;
   }

   return false;
}

void CBridgePlanViewSlabDisplayObjectEvents::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgePlanViewSlabDisplayObjectEvents::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
}

void CBridgePlanViewSlabDisplayObjectEvents::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgePlanViewSlabDisplayObjectEvents::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgePlanViewSlabDisplayObjectEvents::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   IDType id = pDO->GetID();
   if ( id == DECK_ID ) // plan view
   {
      // fill when selected
      auto doComp = std::dynamic_pointer_cast<WBFL::DManip::CompositeDisplayObject>(pDO);

      IndexType nDO = doComp->GetDisplayObjectCount();
      for (IndexType i = 0; i < nDO; i++)
      {
         auto dispObj = doComp->GetDisplayObject(i, WBFL::DManip::AccessType::ByIndex);
         auto pntDO = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(dispObj);
         auto strategy = pntDO->GetDrawingStrategy();
         auto shape_draw = std::dynamic_pointer_cast<WBFL::DManip::ShapeDrawStrategy>(strategy);
         shape_draw->Fill(true);
      }
   }

   m_pFrame->SelectDeck();
}

void CBridgePlanViewSlabDisplayObjectEvents::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   IDType id = pDO->GetID();
   if ( id == DECK_ID )
   {
      // don't fill when not selected
      auto doComp = std::dynamic_pointer_cast<WBFL::DManip::CompositeDisplayObject>(pDO);

      IndexType nDO = doComp->GetDisplayObjectCount();
      for (IndexType i = 0; i < nDO; i++)
      {
         auto dispObj = doComp->GetDisplayObject(i, WBFL::DManip::AccessType::ByIndex);
         auto pntDO = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(dispObj);
         auto strategy = pntDO->GetDrawingStrategy();
         auto shape_draw = std::dynamic_pointer_cast<WBFL::DManip::ShapeDrawStrategy>(strategy);
         shape_draw->Fill(m_bFillIfNotSelected);
      }
   }
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionViewSlabDisplayObjectEvents

CBridgeSectionViewSlabDisplayObjectEvents::CBridgeSectionViewSlabDisplayObjectEvents(CPGSDocBase* pDoc,IBroker* pBroker, CBridgeModelViewChildFrame* pFrame,bool bFillIfNotSelected)
{
   m_pDoc = pDoc;
   m_pBroker = pBroker;
   m_pFrame = pFrame;
   m_bFillIfNotSelected = bFillIfNotSelected;
}

void CBridgeSectionViewSlabDisplayObjectEvents::EditSlab()
{
   m_pDoc->EditBridgeDescription(EBD_DECK);
}

void CBridgeSectionViewSlabDisplayObjectEvents::SelectPrev()
{
   m_pFrame->SelectTrafficBarrier(pgsTypes::tboLeft);
}

void CBridgeSectionViewSlabDisplayObjectEvents::SelectNext()
{
   m_pFrame->SelectTrafficBarrier(pgsTypes::tboRight);
}

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionViewSlabDisplayObjectEvents message handlers
bool CBridgeSectionViewSlabDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   if (pDO->IsSelected())
   {
      EditSlab();
      return true;
   }
   else
   {
      return false;
   }
}

bool CBridgeSectionViewSlabDisplayObjectEvents::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true;
}

bool CBridgeSectionViewSlabDisplayObjectEvents::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeSectionViewSlabDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeSectionViewSlabDisplayObjectEvents::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true;
}

bool CBridgeSectionViewSlabDisplayObjectEvents::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeSectionViewSlabDisplayObjectEvents::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeSectionViewSlabDisplayObjectEvents::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CBridgeSectionViewSlabDisplayObjectEvents::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_RETURN )
   {
      EditSlab();
      return true;
   }
   else if ( nChar == VK_LEFT )
   {
      SelectPrev();
      return true;
   }
   else if ( nChar == VK_RIGHT )
   {
      SelectNext();
      return true;
   }

   return false;
}

bool CBridgeSectionViewSlabDisplayObjectEvents::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( pDO->IsSelected() )
   {
      auto pView = pDO->GetDisplayList()->GetDisplayMgr()->GetView();
      CPGSuperDoc* pDoc = (CPGSuperDoc*)pView->GetDocument();

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
      pMenu->LoadMenu(IDR_SELECTED_DECK_CONTEXT,nullptr);

      const std::map<IDType,IBridgeSectionViewEventCallback*>& callbacks = pDoc->GetBridgeSectionViewCallbacks();
      std::map<IDType,IBridgeSectionViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
      std::map<IDType,IBridgeSectionViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
      for ( ; callbackIter != callbackIterEnd; callbackIter++ )
      {
         IBridgeSectionViewEventCallback* pCallback = callbackIter->second;
         pCallback->OnDeckContextMenu(pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,m_pFrame);

      return true;
   }

   return false;
}

void CBridgeSectionViewSlabDisplayObjectEvents::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgeSectionViewSlabDisplayObjectEvents::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
}

void CBridgeSectionViewSlabDisplayObjectEvents::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgeSectionViewSlabDisplayObjectEvents::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgeSectionViewSlabDisplayObjectEvents::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   IDType id = pDO->GetID();
   if ( id == DECK_ID ) // plan view
   {
      // fill when selected
      auto pntDO = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(pDO);
      auto strategy = pntDO->GetDrawingStrategy();
      auto shape_draw = std::dynamic_pointer_cast<WBFL::DManip::ShapeDrawStrategy>(strategy);
      shape_draw->Fill(true);
   }

   m_pFrame->SelectDeck();
}

void CBridgeSectionViewSlabDisplayObjectEvents::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   IDType id = pDO->GetID();
   if ( id == DECK_ID )
   {
      // don't fill when not selected
      auto pntDO = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(pDO);
      auto strategy = pntDO->GetDrawingStrategy();
      auto shape_draw = std::dynamic_pointer_cast<WBFL::DManip::ShapeDrawStrategy>(strategy);
      shape_draw->Fill(m_bFillIfNotSelected);
   }
}
