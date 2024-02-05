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

// GirderDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"
#include "GirderDisplayObjectEvents.h"
#include "mfcdual.h"
#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"
#include <IFace\Bridge.h>
#include <IFace\EditByUI.h>
#include <IReportManager.h>

#include "BridgePlanView.h"

#include <DManip/DisplayObjectEvents.h>
#include <DManip/DisplayList.h>
#include <DManip/DisplayMgr.h>
#include <DManip/DisplayView.h>

#include <PgsExt\BridgeDescription2.h>

using namespace WBFL::DManip;

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewGirderDisplayObjectEvents

CBridgePlanViewGirderDisplayObjectEvents::CBridgePlanViewGirderDisplayObjectEvents(const CGirderKey& girderKey,GroupIndexType nGroups,GirderIndexType nGirderThisGroup,CBridgeModelViewChildFrame* pFrame)
{
   m_GirderKey         = girderKey;
   m_nGroups           = nGroups;
   m_nGirdersThisGroup = nGirderThisGroup;
   m_pFrame            = pFrame;
}

void CBridgePlanViewGirderDisplayObjectEvents::EditGirder(std::shared_ptr<iDisplayObject> pDO)
{
   m_pFrame->PostMessage(WM_COMMAND,ID_EDIT_GIRDER,0);
}

void CBridgePlanViewGirderDisplayObjectEvents::SelectGirder(std::shared_ptr<iDisplayObject> pDO)
{
   m_pFrame->SelectGirder(m_GirderKey);
}

void CBridgePlanViewGirderDisplayObjectEvents::SelectPrevGirder()
{
   if ( m_GirderKey.girderIndex == 0 )
   {
      // if this is the first girder in this group
      if ( m_GirderKey.groupIndex == 0 ) // and this is the first group
      {
         // select the alignment
         m_pFrame->GetBridgePlanView()->SelectAlignment(true);
      }
      else
      {
         // select the last girder in the previous group
         CGirderKey girderKey(m_GirderKey);
         girderKey.groupIndex--;

         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
         GirderIndexType nGirders = pIBridgeDesc->GetBridgeDescription()->GetGirderGroup(girderKey.groupIndex)->GetGirderCount();
         girderKey.girderIndex = nGirders-1;
         m_pFrame->SelectGirder(girderKey);
      }
   }
   else
   {
      // select the previous girder in this group
      CGirderKey girderKey(m_GirderKey);
      girderKey.girderIndex--;
      m_pFrame->SelectGirder(girderKey);
   }
}

void CBridgePlanViewGirderDisplayObjectEvents::SelectNextGirder()
{
   if ( m_GirderKey.girderIndex == m_nGirdersThisGroup-1 )
   {
      // if this is the last girder in this group
      if ( m_GirderKey.groupIndex == m_nGroups-1 ) // and this is the last group
      {
         // select the first segment
         m_pFrame->SelectSegment(CSegmentKey(0,0,0));
      }
      else
      {
         // select the first girder in the next group
         CGirderKey girderKey(m_GirderKey.groupIndex+1,0);
         m_pFrame->SelectGirder(girderKey);
      }
   }
   else
   {
      // select the next girder in this group
      CGirderKey girderKey(m_GirderKey);
      girderKey.girderIndex++;
      m_pFrame->SelectGirder(girderKey);
   }
}

void CBridgePlanViewGirderDisplayObjectEvents::SelectSpan()
{
   m_pFrame->SelectSpan(m_GirderKey.groupIndex);
}

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewGirderDisplayObjectEvents message handlers
bool CBridgePlanViewGirderDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   if (pDO->IsSelected())
   {
      EditGirder(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

bool CBridgePlanViewGirderDisplayObjectEvents::OnLButtonDown(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CBridgePlanViewGirderDisplayObjectEvents::OnLButtonUp(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgePlanViewGirderDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgePlanViewGirderDisplayObjectEvents::OnRButtonDown(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CBridgePlanViewGirderDisplayObjectEvents::OnRButtonUp(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgePlanViewGirderDisplayObjectEvents::OnMouseMove(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgePlanViewGirderDisplayObjectEvents::OnMouseWheel(std::shared_ptr<iDisplayObject> pDO,UINT nFlags,short zDelta, const POINT& point)
{
   return false;
}

bool CBridgePlanViewGirderDisplayObjectEvents::OnKeyDown(std::shared_ptr<iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_RETURN )
   {
      EditGirder(pDO);
      return true;
   }
   else if ( nChar == VK_LEFT || nChar == VK_RIGHT )
   {
      SelectSpan();
      return true;
   }
   else if ( nChar == VK_UP )
   {
      SelectPrevGirder();
      return true;
   }
   else if ( nChar == VK_DOWN )
   {
      SelectNextGirder();
      return true;
   }

   return false;
}

bool CBridgePlanViewGirderDisplayObjectEvents::OnContextMenu(std::shared_ptr<iDisplayObject> pDO,CWnd* pWnd, const POINT& point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( pDO->IsSelected() )
   {
      auto pList = pDO->GetDisplayList();
      auto pDispMgr = pList->GetDisplayMgr();
      auto pView = pDispMgr->GetView();
      CDocument* pDoc = pView->GetDocument();

      CPGSDocBase* pPGSDoc = (CPGSDocBase*)pDoc;

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pPGSDoc->GetPluginCommandManager());
      pMenu->LoadMenu(IDR_SELECTED_GIRDER_CONTEXT,nullptr);

      if ( pPGSDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
      {
         // PGSplice doesn't do design
         CString strDesignGirder;
         pMenu->GetMenuString(ID_PROJECT_DESIGNGIRDERDIRECT,strDesignGirder,MF_BYCOMMAND);
         UINT nPos = pMenu->FindMenuItem(strDesignGirder);
         pMenu->RemoveMenu(nPos-1,MF_BYPOSITION,nullptr); // remove the separater before "Design Girder"
         pMenu->RemoveMenu(ID_PROJECT_DESIGNGIRDERDIRECT,MF_BYCOMMAND,nullptr);
         pMenu->RemoveMenu(ID_PROJECT_DESIGNGIRDERDIRECT_PRESERVEHAUNCH,MF_BYCOMMAND,nullptr);
      }

      pPGSDoc->BuildReportMenu(pMenu,true);

      const std::map<IDType,IBridgePlanViewEventCallback*>& callbacks = pPGSDoc->GetBridgePlanViewCallbacks();
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
      for ( ; callbackIter != callbackIterEnd; callbackIter++ )
      {
         IBridgePlanViewEventCallback* pCallback = callbackIter->second;
         pCallback->OnGirderContextMenu(m_GirderKey,pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,m_pFrame);

      delete pMenu;

      return true;
   }

   return false;
}

void CBridgePlanViewGirderDisplayObjectEvents::OnChanged(std::shared_ptr<iDisplayObject> pDO)
{
}

void CBridgePlanViewGirderDisplayObjectEvents::OnDragMoved(std::shared_ptr<iDisplayObject> pDO,const WBFL::Geometry::Size2d& size)
{
}

void CBridgePlanViewGirderDisplayObjectEvents::OnMoved(std::shared_ptr<iDisplayObject> pDO)
{
}

void CBridgePlanViewGirderDisplayObjectEvents::OnCopied(std::shared_ptr<iDisplayObject> pDO)
{
}

void CBridgePlanViewGirderDisplayObjectEvents::OnSelect(std::shared_ptr<iDisplayObject> pDO)
{
   SelectGirder(pDO);
}

void CBridgePlanViewGirderDisplayObjectEvents::OnUnselect(std::shared_ptr<iDisplayObject> pDO)
{
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewSegmentDisplayObjectEvents

CBridgePlanViewSegmentDisplayObjectEvents::CBridgePlanViewSegmentDisplayObjectEvents(const CSegmentKey& segmentKey,CBridgeModelViewChildFrame* pFrame)
{
   m_SegmentKey = segmentKey;
   m_pFrame     = pFrame;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(m_SegmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(m_SegmentKey.girderIndex);
   m_nSegments = pGirder->GetSegmentCount();
   m_nGirders = pGroup->GetGirderCount();
}

void CBridgePlanViewSegmentDisplayObjectEvents::EditSegment(std::shared_ptr<iDisplayObject> pDO)
{
   m_pFrame->PostMessage(WM_COMMAND,ID_EDIT_SEGMENT,0);
}

void CBridgePlanViewSegmentDisplayObjectEvents::SelectSegment(std::shared_ptr<iDisplayObject> pDO)
{
   m_pFrame->SelectSegment(m_SegmentKey);
}

void CBridgePlanViewSegmentDisplayObjectEvents::SelectPrevSegment()
{
   if ( m_SegmentKey.groupIndex == 0 && m_SegmentKey.girderIndex == 0 && m_SegmentKey.segmentIndex == 0 )
   {
      // this is the first segment in the first girder in the first group... select the alignment
      m_pFrame->GetBridgePlanView()->SelectAlignment(true);
   }
   else
   {
      if ( m_SegmentKey.segmentIndex == 0 )
      {
         // select previous spliced girder
         CGirderKey girderKey(m_SegmentKey.groupIndex,m_SegmentKey.girderIndex-1);
         m_pFrame->SelectGirder(girderKey);
      }
      else
      {
         // select closure joint at left end of this segment
         CSegmentKey closureKey(m_SegmentKey.groupIndex,m_SegmentKey.girderIndex,m_SegmentKey.segmentIndex-1);
         m_pFrame->SelectClosureJoint(closureKey);
      }
   }
}

void CBridgePlanViewSegmentDisplayObjectEvents::SelectNextSegment()
{
   if ( m_SegmentKey.segmentIndex == m_nSegments-1 )
   {
      // this is the last segment, select the entire girderline
      CGirderKey girderKey(m_SegmentKey.groupIndex,m_SegmentKey.girderIndex);
      m_pFrame->SelectGirder(girderKey);
   }
   else
   {
      // select closure joint at the right end of this segment
      CSegmentKey closureKey(m_SegmentKey.groupIndex,m_SegmentKey.girderIndex,m_SegmentKey.segmentIndex);
      m_pFrame->SelectClosureJoint(closureKey);
   }
}

void CBridgePlanViewSegmentDisplayObjectEvents::SelectAdjacentPrevSegment()
{
   if ( m_SegmentKey.girderIndex == 0 )
   {
      // this is the left-most girderline... 
      if ( m_SegmentKey.segmentIndex == 0 )
      {
         //there are no more segments to select... select the alignment
         m_pFrame->GetBridgePlanView()->SelectAlignment(true);
      }
      else
      {
         // select the closure joint at the right end of the previous segment in the right-most girderline
         CSegmentKey closureKey(m_SegmentKey.groupIndex,m_nGirders-1,m_SegmentKey.segmentIndex-1);
         m_pFrame->SelectClosureJoint(closureKey);
      }
   }
   else
   {
      // select the corresponding segment in the previous girder line
      CSegmentKey segmentKey(m_SegmentKey.groupIndex,m_SegmentKey.girderIndex-1,m_SegmentKey.segmentIndex);
      m_pFrame->SelectSegment(segmentKey);
   }
}

void CBridgePlanViewSegmentDisplayObjectEvents::SelectAdjacentNextSegment()
{
   if ( m_SegmentKey.girderIndex == m_nGirders-1 )
   {
      // this segment is in the right-most girderline... 
      if ( m_SegmentKey.segmentIndex == m_nSegments-1 )
      {
         // there are no more segments to select... select the left-most girderline
         CGirderKey girderKey(m_SegmentKey.groupIndex,0);
         m_pFrame->SelectGirder(girderKey);
      }
      else
      {
         // select the closure joint at the end of the this segment in the left-most girderline
         CSegmentKey closureKey(m_SegmentKey.groupIndex,0,m_SegmentKey.segmentIndex);
         m_pFrame->SelectClosureJoint(closureKey);
      }
   }
   else
   {
      // select corresponding segment in next girderline to the right
      CSegmentKey segmentKey(m_SegmentKey.groupIndex,m_SegmentKey.girderIndex+1,m_SegmentKey.segmentIndex);
      m_pFrame->SelectSegment(segmentKey);
   }
}

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewSegmentDisplayObjectEvents message handlers
bool CBridgePlanViewSegmentDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   auto pParent = pDO->GetParent();
   if (pParent && pParent->IsSelected())
   {
      // we are part of a composite display object and the parent is selected
      // defer dbl-click handling to the parent
      return false;
   }

   if (pDO->IsSelected())
   {
      EditSegment(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

bool CBridgePlanViewSegmentDisplayObjectEvents::OnLButtonDown(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CBridgePlanViewSegmentDisplayObjectEvents::OnLButtonUp(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgePlanViewSegmentDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgePlanViewSegmentDisplayObjectEvents::OnRButtonDown(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CBridgePlanViewSegmentDisplayObjectEvents::OnRButtonUp(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgePlanViewSegmentDisplayObjectEvents::OnMouseMove(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgePlanViewSegmentDisplayObjectEvents::OnMouseWheel(std::shared_ptr<iDisplayObject> pDO,UINT nFlags,short zDelta, const POINT& point)
{
   return false;
}

bool CBridgePlanViewSegmentDisplayObjectEvents::OnKeyDown(std::shared_ptr<iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_RETURN )
   {
      EditSegment(pDO);
      return true;
   }
   else if ( nChar == VK_LEFT )
   {
      SelectPrevSegment();
      return true;
   }
   else if ( nChar == VK_RIGHT )
   {
      SelectNextSegment();
      return true;
   }
   else if ( nChar == VK_UP )
   {
      SelectAdjacentPrevSegment();
      return true;
   }
   else if ( nChar == VK_DOWN )
   {
      SelectAdjacentNextSegment();
      return true;
   }

   return false;
}

bool CBridgePlanViewSegmentDisplayObjectEvents::OnContextMenu(std::shared_ptr<iDisplayObject> pDO,CWnd* pWnd, const POINT& point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( pDO->IsSelected() )
   {
      auto pList = pDO->GetDisplayList();
      auto pDispMgr = pList->GetDisplayMgr();
      auto pView = pDispMgr->GetView();
      CDocument* pDoc = pView->GetDocument();

      CPGSDocBase* pPGSuperDoc = (CPGSDocBase*)pDoc;

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pPGSuperDoc->GetPluginCommandManager());
      if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
      {
         pMenu->LoadMenu(IDR_SELECTED_GIRDER_CONTEXT,nullptr);
      }
      else
      {
         pMenu->LoadMenu(IDR_SELECTED_GIRDER_SEGMENT_CONTEXT,nullptr);
      }

      pPGSuperDoc->BuildReportMenu(pMenu,true);

      std::map<IDType,IBridgePlanViewEventCallback*> callbacks = pPGSuperDoc->GetBridgePlanViewCallbacks();
      std::map<IDType,IBridgePlanViewEventCallback*>::iterator iter;
      for ( iter = callbacks.begin(); iter != callbacks.end(); iter++ )
      {
         IBridgePlanViewEventCallback* callback = iter->second;
         callback->OnGirderSegmentContextMenu(m_SegmentKey,pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,m_pFrame);

      delete pMenu;

      return true;
   }

   return false;
}

void CBridgePlanViewSegmentDisplayObjectEvents::OnChanged(std::shared_ptr<iDisplayObject> pDO)
{
}

void CBridgePlanViewSegmentDisplayObjectEvents::OnDragMoved(std::shared_ptr<iDisplayObject> pDO,const WBFL::Geometry::Size2d& size)
{
}

void CBridgePlanViewSegmentDisplayObjectEvents::OnMoved(std::shared_ptr<iDisplayObject> pDO)
{
}

void CBridgePlanViewSegmentDisplayObjectEvents::OnCopied(std::shared_ptr<iDisplayObject> pDO)
{
}

void CBridgePlanViewSegmentDisplayObjectEvents::OnSelect(std::shared_ptr<iDisplayObject> pDO)
{
   SelectSegment(pDO);
}

void CBridgePlanViewSegmentDisplayObjectEvents::OnUnselect(std::shared_ptr<iDisplayObject> pDO)
{
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// CBridgeSectionViewGirderDisplayObjectEvents

CBridgeSectionViewGirderDisplayObjectEvents::CBridgeSectionViewGirderDisplayObjectEvents(const CGirderKey& girderKey,GroupIndexType nGroups,GirderIndexType nGirdersThisGroup,CBridgeModelViewChildFrame* pFrame)
{
   m_GirderKey         = girderKey;
   m_nGroups           = nGroups;
   m_nGirdersThisGroup = nGirdersThisGroup;
   m_pFrame            = pFrame;
}

void CBridgeSectionViewGirderDisplayObjectEvents::EditGirder(std::shared_ptr<iDisplayObject> pDO)
{
   m_pFrame->PostMessage(WM_COMMAND,ID_EDIT_GIRDER,0);
}

void CBridgeSectionViewGirderDisplayObjectEvents::SelectGirder(std::shared_ptr<iDisplayObject> pDO)
{
   // do the selection at the frame level because it will do this view
   // and the other view
   m_pFrame->SelectGirder(m_GirderKey);
}

void CBridgeSectionViewGirderDisplayObjectEvents::SelectPrevGirder()
{
   if ( m_GirderKey.girderIndex == 0 )
   {
      m_pFrame->SelectTrafficBarrier(pgsTypes::tboRight);
   }
   else
   {
      CGirderKey girderKey(m_GirderKey);
      girderKey.girderIndex--;
      m_pFrame->SelectGirder(girderKey);
   }
}

void CBridgeSectionViewGirderDisplayObjectEvents::SelectNextGirder()
{
   if ( m_GirderKey.girderIndex == m_nGirdersThisGroup-1 )
   {
      m_pFrame->SelectTrafficBarrier(pgsTypes::tboLeft);
   }
   else
   {
      CGirderKey girderKey(m_GirderKey);
      girderKey.girderIndex++;
      m_pFrame->SelectGirder(girderKey);
   }
}

void CBridgeSectionViewGirderDisplayObjectEvents::SelectSpan()
{
   ATLASSERT(false); // implement
   //m_pFrame->SelectSpan(m_SpanIdx);
}

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionViewGirderDisplayObjectEvents message handlers
bool CBridgeSectionViewGirderDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   if (pDO->IsSelected())
   {
      EditGirder(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

bool CBridgeSectionViewGirderDisplayObjectEvents::OnLButtonDown(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CBridgeSectionViewGirderDisplayObjectEvents::OnLButtonUp(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgeSectionViewGirderDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgeSectionViewGirderDisplayObjectEvents::OnRButtonDown(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CBridgeSectionViewGirderDisplayObjectEvents::OnRButtonUp(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgeSectionViewGirderDisplayObjectEvents::OnMouseMove(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgeSectionViewGirderDisplayObjectEvents::OnMouseWheel(std::shared_ptr<iDisplayObject> pDO,UINT nFlags,short zDetla, const POINT& point)
{
   return false;
}

bool CBridgeSectionViewGirderDisplayObjectEvents::OnKeyDown(std::shared_ptr<iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   bool bEventHandled = false;
   if ( nChar == VK_RETURN )
   {
      EditGirder(pDO);
      bEventHandled = true;
   }
   else if ( nChar == VK_LEFT )
   {
      SelectPrevGirder();
      bEventHandled = true;
   }
   else if ( nChar == VK_RIGHT )
   {
      SelectNextGirder();
      bEventHandled = true;
   }

   return bEventHandled;
}

bool CBridgeSectionViewGirderDisplayObjectEvents::OnContextMenu(std::shared_ptr<iDisplayObject> pDO,CWnd* pWnd, const POINT& point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( pDO->IsSelected() )
   {
      CEAFDocument* pDoc = EAFGetDocument();

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());

      if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
      {
         pMenu->LoadMenu(IDR_SELECTED_GIRDER_CONTEXT,nullptr);
      }
      else if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
      {
         pMenu->LoadMenu(IDR_SELECTED_GIRDERLINE_CONTEXT,nullptr);

         CEAFMenu* pSegmentMenu = pMenu->CreatePopupMenu(0,_T("Edit Segment"));
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
         SegmentIndexType nSegments = pIBridgeDesc->GetBridgeDescription()->GetGirderGroup(m_GirderKey.groupIndex)->GetGirder(m_GirderKey.girderIndex)->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CString strMenuItem;
            strMenuItem.Format(_T("Segment %d"),LABEL_SEGMENT(segIdx));
            UINT uID = ID_EDIT_SEGMENT_MIN + (UINT)segIdx;
            if ( uID <= ID_EDIT_SEGMENT_MAX )
            {
               pSegmentMenu->AppendMenu(uID,strMenuItem,nullptr);
            }
         }
      }

      CPGSDocBase* pBaseDoc = (CPGSDocBase*)(pDoc);
      pBaseDoc->BuildReportMenu(pMenu,true);

      const std::map<IDType,IBridgeSectionViewEventCallback*>& callbacks = pBaseDoc->GetBridgeSectionViewCallbacks();
      std::map<IDType,IBridgeSectionViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
      std::map<IDType,IBridgeSectionViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
      for ( ; callbackIter != callbackIterEnd; callbackIter++ )
      {
         IBridgeSectionViewEventCallback* pCallback = callbackIter->second;
         pCallback->OnGirderContextMenu(m_GirderKey,pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,m_pFrame);

      delete pMenu;

      return true;
   }

   return false;
}

void CBridgeSectionViewGirderDisplayObjectEvents::OnChanged(std::shared_ptr<iDisplayObject> pDO)
{
}

void CBridgeSectionViewGirderDisplayObjectEvents::OnDragMoved(std::shared_ptr<iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
}

void CBridgeSectionViewGirderDisplayObjectEvents::OnMoved(std::shared_ptr<iDisplayObject> pDO)
{
}

void CBridgeSectionViewGirderDisplayObjectEvents::OnCopied(std::shared_ptr<iDisplayObject> pDO)
{
}

void CBridgeSectionViewGirderDisplayObjectEvents::OnSelect(std::shared_ptr<iDisplayObject> pDO)
{
   SelectGirder(pDO);
}

void CBridgeSectionViewGirderDisplayObjectEvents::OnUnselect(std::shared_ptr<iDisplayObject> pDO)
{
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CGirderElevationViewSegmentDisplayObjectEvents

CGirderElevationViewSegmentDisplayObjectEvents::CGirderElevationViewSegmentDisplayObjectEvents(const CSegmentKey& segmentKey, CGirderModelChildFrame* pFrame)
{
   m_SegmentKey = segmentKey;
   m_pFrame = pFrame;
}

void CGirderElevationViewSegmentDisplayObjectEvents::EditSegment(std::shared_ptr<iDisplayObject> pDO)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEditByUI, pEdit);
   pEdit->EditSegmentDescription(m_SegmentKey, EGD_GENERAL);
}

/////////////////////////////////////////////////////////////////////////////
// CGirderElevationViewSegmentDisplayObjectEvents message handlers
bool CGirderElevationViewSegmentDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   EditSegment(pDO);
   return true;
}

bool CGirderElevationViewSegmentDisplayObjectEvents::OnLButtonDown(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CGirderElevationViewSegmentDisplayObjectEvents::OnLButtonUp(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CGirderElevationViewSegmentDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CGirderElevationViewSegmentDisplayObjectEvents::OnRButtonDown(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CGirderElevationViewSegmentDisplayObjectEvents::OnRButtonUp(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CGirderElevationViewSegmentDisplayObjectEvents::OnMouseMove(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CGirderElevationViewSegmentDisplayObjectEvents::OnMouseWheel(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, short zDelta, const POINT& point)
{
   return false;
}

bool CGirderElevationViewSegmentDisplayObjectEvents::OnKeyDown(std::shared_ptr<iDisplayObject> pDO, UINT nChar, UINT nRepCnt, UINT nFlags)
{
   return false;
}

bool CGirderElevationViewSegmentDisplayObjectEvents::OnContextMenu(std::shared_ptr<iDisplayObject> pDO, CWnd* pWnd, const POINT& point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if (pDO->IsSelected())
   {
      auto pDispMgr = pDO->GetDisplayList()->GetDisplayMgr();

      CDisplayView* pView = pDispMgr->GetView();
      CDocument* pDoc = pView->GetDocument();

      CPGSDocBase* pPGSuperDoc = (CPGSDocBase*)pDoc;

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pPGSuperDoc->GetPluginCommandManager());
      if (pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)))
      {
         pMenu->LoadMenu(IDR_SELECTED_GIRDER_CONTEXT, nullptr);
      }
      else
      {
         pMenu->LoadMenu(IDR_SELECTED_GIRDER_SEGMENT_CONTEXT, nullptr);
      }

      pPGSuperDoc->BuildReportMenu(pMenu, true);

      std::map<IDType, IGirderElevationViewEventCallback*> callbacks = pPGSuperDoc->GetGirderElevationViewCallbacks();
      std::map<IDType, IGirderElevationViewEventCallback*>::iterator iter;
      for (iter = callbacks.begin(); iter != callbacks.end(); iter++)
      {
         IGirderElevationViewEventCallback* callback = iter->second;
         callback->OnGirderSegmentContextMenu(m_SegmentKey, pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, m_pFrame);

      delete pMenu;

      return true;
   }

   return false;
}

void CGirderElevationViewSegmentDisplayObjectEvents::OnChanged(std::shared_ptr<iDisplayObject> pDO)
{
}

void CGirderElevationViewSegmentDisplayObjectEvents::OnDragMoved(std::shared_ptr<iDisplayObject> pDO, const WBFL::Geometry::Size2d& offset)
{
}

void CGirderElevationViewSegmentDisplayObjectEvents::OnMoved(std::shared_ptr<iDisplayObject> pDO)
{
}

void CGirderElevationViewSegmentDisplayObjectEvents::OnCopied(std::shared_ptr<iDisplayObject> pDO)
{
}

void CGirderElevationViewSegmentDisplayObjectEvents::OnSelect(std::shared_ptr<iDisplayObject> pDO)
{
}

void CGirderElevationViewSegmentDisplayObjectEvents::OnUnselect(std::shared_ptr<iDisplayObject> pDO)
{
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CGirderSectionViewSegmentDisplayObjectEvents

CGirderSectionViewSegmentDisplayObjectEvents::CGirderSectionViewSegmentDisplayObjectEvents(const pgsPointOfInterest& poi, CGirderModelChildFrame* pFrame)
{
   m_POI = poi;
   m_pFrame = pFrame;
}


void CGirderSectionViewSegmentDisplayObjectEvents::EditSegment(std::shared_ptr<iDisplayObject> pDO)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEditByUI, pEdit);
   pEdit->EditGirderDescription(m_POI.GetSegmentKey(), EGD_GENERAL);
}

/////////////////////////////////////////////////////////////////////////////
// CGirderElevationViewSegmentDisplayObjectEvents message handlers
bool CGirderSectionViewSegmentDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   EditSegment(pDO);
   return true;
}

bool CGirderSectionViewSegmentDisplayObjectEvents::OnLButtonDown(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CGirderSectionViewSegmentDisplayObjectEvents::OnLButtonUp(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CGirderSectionViewSegmentDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CGirderSectionViewSegmentDisplayObjectEvents::OnRButtonDown(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CGirderSectionViewSegmentDisplayObjectEvents::OnRButtonUp(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CGirderSectionViewSegmentDisplayObjectEvents::OnMouseMove(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CGirderSectionViewSegmentDisplayObjectEvents::OnMouseWheel(std::shared_ptr<iDisplayObject> pDO, UINT nFlags, short zDelta, const POINT& point)
{
   return false;
}

bool CGirderSectionViewSegmentDisplayObjectEvents::OnKeyDown(std::shared_ptr<iDisplayObject> pDO, UINT nChar, UINT nRepCnt, UINT nFlags)
{
   return false;
}

bool CGirderSectionViewSegmentDisplayObjectEvents::OnContextMenu(std::shared_ptr<iDisplayObject> pDO, CWnd* pWnd, const POINT& point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if (pDO->IsSelected())
   {
      auto pList = pDO->GetDisplayList();
      auto pDispMgr = pList->GetDisplayMgr();

      CDisplayView* pView = pDispMgr->GetView();
      CDocument* pDoc = pView->GetDocument();

      CPGSDocBase* pPGSuperDoc = (CPGSDocBase*)pDoc;

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pPGSuperDoc->GetPluginCommandManager());
      if (pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)))
      {
         pMenu->LoadMenu(IDR_SELECTED_GIRDER_CONTEXT, nullptr);
      }
      else
      {
         pMenu->LoadMenu(IDR_SELECTED_GIRDER_SEGMENT_CONTEXT, nullptr);
      }

      pPGSuperDoc->BuildReportMenu(pMenu, true);

      std::map<IDType, IGirderSectionViewEventCallback*> callbacks = pPGSuperDoc->GetGirderSectionViewCallbacks();
      std::map<IDType, IGirderSectionViewEventCallback*>::iterator iter;
      for (iter = callbacks.begin(); iter != callbacks.end(); iter++)
      {
         IGirderSectionViewEventCallback* callback = iter->second;
         callback->OnGirderSectionContextMenu(m_POI, pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, m_pFrame);

      delete pMenu;

      return true;
   }

   return false;
}

void CGirderSectionViewSegmentDisplayObjectEvents::OnChanged(std::shared_ptr<iDisplayObject> pDO)
{
}

void CGirderSectionViewSegmentDisplayObjectEvents::OnDragMoved(std::shared_ptr<iDisplayObject> pDO, const WBFL::Geometry::Size2d& offset)
{
}

void CGirderSectionViewSegmentDisplayObjectEvents::OnMoved(std::shared_ptr<iDisplayObject> pDO)
{
}

void CGirderSectionViewSegmentDisplayObjectEvents::OnCopied(std::shared_ptr<iDisplayObject> pDO)
{
}

void CGirderSectionViewSegmentDisplayObjectEvents::OnSelect(std::shared_ptr<iDisplayObject> pDO)
{
}

void CGirderSectionViewSegmentDisplayObjectEvents::OnUnselect(std::shared_ptr<iDisplayObject> pDO)
{
}
