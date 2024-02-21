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

// AlignmentDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "AlignmentDisplayObjectEvents.h"
#include "BridgeSectionCutDisplayImpl.h"
#include "BridgeModelViewChildFrame.h"
#include "PGSuperDocBase.h"
#include <IFace\Bridge.h>
#include <IFace\EditByUI.h>

#include <PgsExt\BridgeDescription2.h>

#include <DManip/DisplayObject.h>
#include <DManip/DisplayList.h>
#include <DManip/DisplayView.h>
#include <DManip/DragDataImpl.h>

using namespace WBFL::DManip;

CAlignmentDisplayObjectEvents::CAlignmentDisplayObjectEvents(IBroker* pBroker, CBridgeModelViewChildFrame* pFrame,ViewType viewType,std::shared_ptr<iDisplayObject> pDO)
{
   m_ViewType = viewType;
   m_pBroker = pBroker;
   m_pFrame = pFrame;

   m_DispObj = pDO;
}

CAlignmentDisplayObjectEvents::~CAlignmentDisplayObjectEvents()
{
}

void CAlignmentDisplayObjectEvents::EditAlignment()
{
   GET_IFACE(IEditByUI, pEditByUI);
   int page = (m_ViewType == BridgePlan || m_ViewType == Alignment ? EAD_ROADWAY : EAD_SECTION);
   pEditByUI->EditAlignmentDescription(page);
}

/////////////////////////////////////////////////////////////////////////////
// CAlignmentDisplayObjectEvents message handlers
bool CAlignmentDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   if (pDO->IsSelected())
   {
      EditAlignment();
      return true;
   }
   else
   {
      return false;
   }
}

bool CAlignmentDisplayObjectEvents::OnLButtonDown(std::shared_ptr<iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true;
}

bool CAlignmentDisplayObjectEvents::OnLButtonUp(std::shared_ptr<iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CAlignmentDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CAlignmentDisplayObjectEvents::OnRButtonDown(std::shared_ptr<iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CAlignmentDisplayObjectEvents::OnRButtonUp(std::shared_ptr<iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CAlignmentDisplayObjectEvents::OnMouseMove(std::shared_ptr<iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CAlignmentDisplayObjectEvents::OnMouseWheel(std::shared_ptr<iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CAlignmentDisplayObjectEvents::OnKeyDown(std::shared_ptr<iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_RETURN )
   {
      EditAlignment();
      return true;
   }

   if (m_ViewType == BridgePlan || m_ViewType == BridgeSection)
   {
      if ( nChar == VK_DOWN )
      {
         m_pFrame->SelectGirder(CSegmentKey(0,0,0));
         return true;
      }
      else if ( nChar == VK_UP )
      {
         GET_IFACE2(m_pBroker,IBridgeDescription,pIBridgeDesc);
         const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
         GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
         GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(nGroups-1)->GetGirderCount();
         SegmentIndexType nSegments = pBridgeDesc->GetGirderGroup(nGroups-1)->GetGirder(nGirders-1)->GetSegmentCount();
         m_pFrame->SelectGirder(CSegmentKey(nGroups-1,nGirders-1,nSegments-1));

         return true;
      }
      else if ( nChar == VK_LEFT )
      {
         GET_IFACE2(m_pBroker,IBridgeDescription,pIBridgeDesc);
         const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
         m_pFrame->SelectPier(pBridgeDesc->GetPierCount()-1);
         return true;
      }
      else if ( nChar == VK_RIGHT )
      {
         GET_IFACE2(m_pBroker,IBridgeDescription,pIBridgeDesc);
         const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
         m_pFrame->SelectPier(0);
         return true;
      }
   }


   return false;
}

bool CAlignmentDisplayObjectEvents::OnContextMenu(std::shared_ptr<iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   if ( pDO->IsSelected() )
   {
      auto pList = pDO->GetDisplayList();
      auto pDispMgr = pList->GetDisplayMgr();
      auto pView = pDispMgr->GetView();

      CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();

      CEAFMenu* pMenu;

      if ( m_ViewType == BridgePlan || m_ViewType == BridgeSection )
      {
         const std::map<IDType,IBridgePlanViewEventCallback*>& callbacks = pDoc->GetBridgePlanViewCallbacks();

         // the alignment doesn't have its own context menu, so if there aren't callbacks to add anything
         // then just return
         if ( callbacks.size() == 0 )
         {
            return false;
         }

         pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
         std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
         std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
         for ( ; callbackIter != callbackIterEnd; callbackIter++ )
         {
            IBridgePlanViewEventCallback* pCallback = callbackIter->second;
            pCallback->OnAlignmentContextMenu(pMenu);
         }
      }
      else
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
            pCallback->OnAlignmentContextMenu(pMenu);
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

void CAlignmentDisplayObjectEvents::OnChanged(std::shared_ptr<iDisplayObject> pDO)
{
}

void CAlignmentDisplayObjectEvents::OnDragMoved(std::shared_ptr<iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
}

void CAlignmentDisplayObjectEvents::OnMoved(std::shared_ptr<iDisplayObject> pDO)
{
}

void CAlignmentDisplayObjectEvents::OnCopied(std::shared_ptr<iDisplayObject> pDO)
{
}

void CAlignmentDisplayObjectEvents::OnSelect(std::shared_ptr<iDisplayObject> pDO)
{
   m_pFrame->SelectAlignment();
}

void CAlignmentDisplayObjectEvents::OnUnselect(std::shared_ptr<iDisplayObject> pDO)
{
}


DROPEFFECT CAlignmentDisplayObjectEvents::CanDrop(COleDataObject* pDataObject,DWORD dwKeyState,const WBFL::Geometry::Point2d& point)
{
   if ( pDataObject->IsDataAvailable(CBridgeSectionCutDisplayImpl::ms_Format) )
   {
      // need to peek at our object first and make sure it's coming from the local process
      // this is ugly because it breaks encapsulation of CBridgeSectionCutDisplayImpl
      auto source = WBFL::DManip::DragDataSource::Create();
      source->SetDataObject(pDataObject);
      source->PrepareFormat(CBridgeSectionCutDisplayImpl::ms_Format);

      CWinThread* thread = ::AfxGetThread( );
      DWORD threadid = thread->m_nThreadID;

      DWORD threadl;
      // know (by voodoo) that the first member of this data source is the thread id
      source->Read(CBridgeSectionCutDisplayImpl::ms_Format,&threadl,sizeof(DWORD));

      if (threadl == threadid)
      {
        return DROPEFFECT_MOVE;
      }
   }

   return DROPEFFECT_NONE;
}

void CAlignmentDisplayObjectEvents::OnDropped(COleDataObject* pDataObject,DROPEFFECT dropEffect,const WBFL::Geometry::Point2d& point)
{
   // Something was dropped on a display object that represents a Alignment
   AfxMessageBox(_T("Alignment drop"));
}

void CAlignmentDisplayObjectEvents::SetDisplayObject(std::weak_ptr<iDisplayObject> pDO)
{
   m_DispObj = pDO;
}

std::shared_ptr<iDisplayObject> CAlignmentDisplayObjectEvents::GetDisplayObject()
{
   return m_DispObj.lock();
}

void CAlignmentDisplayObjectEvents::Highlight(CDC* pDC, BOOL bHighlite)
{
   m_DispObj.lock()->Highlight(pDC, bHighlite);
}


