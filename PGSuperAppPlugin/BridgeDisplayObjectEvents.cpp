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

// BridgeDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "BridgeDisplayObjectEvents.h"
#include "BridgeModelViewChildFrame.h"
#include "PGSuperDocBase.h"

#include <DManip/DisplayObject.h>

/////////////////////////////////////////////////////////////////////////////
// CBridgeDisplayObjectEvents

CBridgeDisplayObjectEvents::CBridgeDisplayObjectEvents(IBroker* pBroker, CBridgeModelViewChildFrame* pFrame,std::weak_ptr<WBFL::DManip::iDisplayObject> pDO,CBridgeDisplayObjectEvents::ViewType viewType)
{
   m_pBroker = pBroker;
   m_pFrame = pFrame;
   m_ViewType = viewType;
   m_DispObj = pDO;
}

CBridgeDisplayObjectEvents::~CBridgeDisplayObjectEvents()
{
}

void CBridgeDisplayObjectEvents::EditBridge()
{
   m_pFrame->PostMessage(WM_COMMAND,ID_PROJECT_BRIDGEDESC,0);
}

/////////////////////////////////////////////////////////////////////////////
// CAlignmentDisplayObjectEvents message handlers
bool CBridgeDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   if (pDO->IsSelected())
   {
      EditBridge();
      return true;
   }
   else
   {
      return false;
   }
}

bool CBridgeDisplayObjectEvents::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true;
}

bool CBridgeDisplayObjectEvents::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeDisplayObjectEvents::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeDisplayObjectEvents::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeDisplayObjectEvents::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeDisplayObjectEvents::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CBridgeDisplayObjectEvents::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_RETURN )
   {
      EditBridge();
      return true;
   }

   return false;
}

bool CBridgeDisplayObjectEvents::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   if ( pDO->IsSelected() )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
      CEAFMenu* pMenu;
      if ( m_ViewType == CBridgeDisplayObjectEvents::Plan )
      {
         const std::map<IDType,IAlignmentPlanViewEventCallback*>& callbacks = pDoc->GetAlignmentPlanViewCallbacks();

         // the alignment doesn't have its own context menu, so if there aren't callbacks to add anything
         // then just return
         if ( callbacks.size() == 0 )
         {
            return false;
         }

         pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
         std::map<IDType,IAlignmentPlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
         std::map<IDType,IAlignmentPlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
         for ( ; callbackIter != callbackIterEnd; callbackIter++ )
         {
            IAlignmentPlanViewEventCallback* pCallback = callbackIter->second;
            pCallback->OnBridgeContextMenu(pMenu);
         }
      }
      else
      {
         const std::map<IDType,IAlignmentProfileViewEventCallback*>& callbacks = pDoc->GetAlignmentProfileViewCallbacks();

         // the alignment doesn't have its own context menu, so if there aren't callbacks to add anything
         // then just return
         if ( callbacks.size() == 0 )
         {
            return false;
         }

         pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
         std::map<IDType,IAlignmentProfileViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
         std::map<IDType,IAlignmentProfileViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
         for ( ; callbackIter != callbackIterEnd; callbackIter++ )
         {
            IAlignmentProfileViewEventCallback* pCallback = callbackIter->second;
            pCallback->OnBridgeContextMenu(pMenu);
         }
      }

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

void CBridgeDisplayObjectEvents::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgeDisplayObjectEvents::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
}

void CBridgeDisplayObjectEvents::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgeDisplayObjectEvents::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgeDisplayObjectEvents::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   m_pFrame->ClearSelection();
   m_pFrame->SelectAlignment();
}

void CBridgeDisplayObjectEvents::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}
