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

// BridgeDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "ProfileDisplayObjectEvents.h"
#include "BridgeModelViewChildFrame.h"
#include "PGSuperDocBase.h"

#include <DManip/DisplayObject.h>

/////////////////////////////////////////////////////////////////////////////
// CProfileDisplayObjectEvents

CProfileDisplayObjectEvents::CProfileDisplayObjectEvents(std::shared_ptr<WBFL::EAF::Broker> pBroker, CBridgeModelViewChildFrame* pFrame,std::weak_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   m_pBroker = pBroker;
   m_pFrame = pFrame;
   m_DispObj = pDO;
}

CProfileDisplayObjectEvents::~CProfileDisplayObjectEvents()
{
}

void CProfileDisplayObjectEvents::EditProfile()
{
   m_pFrame->PostMessage(WM_COMMAND,ID_PROJECT_PROFILE,0);
}

/////////////////////////////////////////////////////////////////////////////
// CProfileDisplayObjectEvents message handlers
bool CProfileDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   if (pDO->IsSelected())
   {
      EditProfile();
      return true;
   }
   else
   {
      return false;
   }
}

bool CProfileDisplayObjectEvents::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true;
}

bool CProfileDisplayObjectEvents::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CProfileDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CProfileDisplayObjectEvents::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CProfileDisplayObjectEvents::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CProfileDisplayObjectEvents::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CProfileDisplayObjectEvents::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CProfileDisplayObjectEvents::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_RETURN )
   {
      EditProfile();
      return true;
   }

   return false;
}

bool CProfileDisplayObjectEvents::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   if ( pDO->IsSelected() )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
      const std::map<IDType,IAlignmentProfileViewEventCallback*>& callbacks = pDoc->GetAlignmentProfileViewCallbacks();

      // the alignment doesn't have its own context menu, so if there aren't callbacks to add anything
      // then just return
      if ( callbacks.size() == 0 )
      {
         return false;
      }

      auto pMenu = WBFL::EAF::Menu::CreateContextMenu(pDoc->GetPluginCommandManager());
      std::map<IDType,IAlignmentProfileViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
      std::map<IDType,IAlignmentProfileViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
      for ( ; callbackIter != callbackIterEnd; callbackIter++ )
      {
         IAlignmentProfileViewEventCallback* pCallback = callbackIter->second;
         pCallback->OnBridgeContextMenu(pMenu);
      }

      bool bResult = false;
      if ( 0 < pMenu->GetMenuItemCount() )
      {
         pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,m_pFrame);
         bResult = true;
      }

      return bResult;
   }

   return false;
}

STDMETHODIMP_(void) CProfileDisplayObjectEvents::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

STDMETHODIMP_(void) CProfileDisplayObjectEvents::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
}

STDMETHODIMP_(void) CProfileDisplayObjectEvents::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

STDMETHODIMP_(void) CProfileDisplayObjectEvents::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

STDMETHODIMP_(void) CProfileDisplayObjectEvents::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   m_pFrame->ClearSelection();
   m_pFrame->SelectAlignment();
}

STDMETHODIMP_(void) CProfileDisplayObjectEvents::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}
