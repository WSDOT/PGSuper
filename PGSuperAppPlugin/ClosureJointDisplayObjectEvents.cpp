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

// ClosureJointDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"
#include "ClosureJointDisplayObjectEvents.h"
#include "PGSpliceDoc.h"
#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>

#include <DManip/DisplayObject.h>
#include <DManip/DisplayList.h>
#include <DManip/DisplayMgr.h>
#include <DManip/DisplayView.h>


CClosureJointDisplayObjectEvents::CClosureJointDisplayObjectEvents(const CSegmentKey& closureKey,const CSegmentKey& leftSegmentKey,const CSegmentKey& rightSegmentKey,CBridgeModelViewChildFrame* pFrame)
{
   m_ClosureKey = closureKey;
   m_pFrame = pFrame;

   m_LeftSegmentKey  = leftSegmentKey;
   m_RightSegmentKey = rightSegmentKey;
}

void CClosureJointDisplayObjectEvents::EditClosureJoint(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   m_pFrame->PostMessage(WM_COMMAND,ID_EDIT_CLOSURE,0);
}

void CClosureJointDisplayObjectEvents::SelectClosureJoint(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   // do the selection at the frame level because it will do this view
   // and the other view
   m_pFrame->SelectClosureJoint(m_ClosureKey);
}

void CClosureJointDisplayObjectEvents::SelectPrev(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   m_pFrame->SelectSegment(m_LeftSegmentKey);
}

void CClosureJointDisplayObjectEvents::SelectNext(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   m_pFrame->SelectSegment(m_RightSegmentKey);
}

void CClosureJointDisplayObjectEvents::SelectPrevAdjacent(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   // select closure joint in adjacent girder
   if ( m_ClosureKey.girderIndex == 0 )
   {
      // if this is the first girder, wrap around to the previous segment of the last girder
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CGirderGroupData* pGroup = pIBridgeDesc->GetBridgeDescription()->GetGirderGroup(m_ClosureKey.groupIndex);
      m_pFrame->SelectSegment( CSegmentKey(m_ClosureKey.groupIndex,pGroup->GetGirderCount()-1,m_ClosureKey.segmentIndex) );
   }
   else
   {
      // select this same closure in the prev girder
      CSegmentKey closureKey(m_ClosureKey);
      closureKey.girderIndex--;
      m_pFrame->SelectClosureJoint(closureKey);
   }
}

void CClosureJointDisplayObjectEvents::SelectNextAdjacent(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CGirderGroupData* pGroup = pIBridgeDesc->GetBridgeDescription()->GetGirderGroup(m_ClosureKey.groupIndex);
   if ( m_ClosureKey.girderIndex == pGroup->GetGirderCount()-1 )
   {
      // this is the last girder in the group... wrap to the next segment in the first girder
      CSegmentKey segmentKey(m_ClosureKey);
      segmentKey.girderIndex = 0;
      segmentKey.segmentIndex++;
      m_pFrame->SelectSegment(segmentKey);
   }
   else
   {
      // select the same closure in the next girder
      CSegmentKey closureKey(m_ClosureKey);
      closureKey.girderIndex++;
      m_pFrame->SelectClosureJoint(closureKey);
   }
}

/////////////////////////////////////////////////////////////////////////////
// CClosureJointDisplayObjectEvents message handlers
bool CClosureJointDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   if (pDO->IsSelected())
   {
      EditClosureJoint(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

bool CClosureJointDisplayObjectEvents::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CClosureJointDisplayObjectEvents::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CClosureJointDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CClosureJointDisplayObjectEvents::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CClosureJointDisplayObjectEvents::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CClosureJointDisplayObjectEvents::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CClosureJointDisplayObjectEvents::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CClosureJointDisplayObjectEvents::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_RETURN )
   {
      EditClosureJoint(pDO);
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
   else if ( nChar == VK_UP )
   {
      SelectPrevAdjacent(pDO);
      return true;
   }
   else if ( nChar == VK_DOWN )
   {
      SelectNextAdjacent(pDO);
      return true;
   }

   return false;
}

bool CClosureJointDisplayObjectEvents::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());


   if ( pDO->IsSelected() )
   {
      auto pView = pDO->GetDisplayList()->GetDisplayMgr()->GetView();
      CPGSpliceDoc* pDoc = (CPGSpliceDoc*)pView->GetDocument();

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
      pMenu->AppendMenu(ID_EDIT_CLOSURE,_T("Edit"),nullptr);

      std::map<IDType,IBridgePlanViewEventCallback*> callbacks = pDoc->GetBridgePlanViewCallbacks();
      std::map<IDType,IBridgePlanViewEventCallback*>::iterator iter;
      for ( iter = callbacks.begin(); iter != callbacks.end(); iter++ )
      {
         IBridgePlanViewEventCallback* callback = iter->second;
         callback->OnClosureJointContextMenu(m_ClosureKey,pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,m_pFrame);

      delete pMenu;

      return true;
   }

   return false;
}

void CClosureJointDisplayObjectEvents::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CClosureJointDisplayObjectEvents::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
}

void CClosureJointDisplayObjectEvents::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CClosureJointDisplayObjectEvents::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CClosureJointDisplayObjectEvents::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   SelectClosureJoint(pDO);
}

void CClosureJointDisplayObjectEvents::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

