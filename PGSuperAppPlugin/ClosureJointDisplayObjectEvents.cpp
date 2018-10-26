///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "ClosureJointDisplayObjectEvents.h"
#include "mfcdual.h"
#include "PGSpliceDoc.h"
#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\SpanData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClosureJointDisplayObjectEvents

CClosureJointDisplayObjectEvents::CClosureJointDisplayObjectEvents(const CSegmentKey& closureKey,const CSegmentKey& leftSegmentKey,const CSegmentKey& rightSegmentKey,CBridgeModelViewChildFrame* pFrame)
{
   m_ClosureKey = closureKey;
   m_pFrame = pFrame;

   m_LeftSegmentKey  = leftSegmentKey;
   m_RightSegmentKey = rightSegmentKey;
}

BEGIN_INTERFACE_MAP(CClosureJointDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CClosureJointDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CClosureJointDisplayObjectEvents,Events);

void CClosureJointDisplayObjectEvents::EditClosureJoint(iDisplayObject* pDO)
{
   m_pFrame->SendMessage(WM_COMMAND,ID_EDIT_CLOSURE,0);
}

void CClosureJointDisplayObjectEvents::SelectClosureJoint(iDisplayObject* pDO)
{
   // do the selection at the frame level because it will do this view
   // and the other view
   m_pFrame->SelectClosureJoint(m_ClosureKey);
}

void CClosureJointDisplayObjectEvents::SelectPrev(iDisplayObject* pDO)
{
   m_pFrame->SelectSegment(m_LeftSegmentKey);
}

void CClosureJointDisplayObjectEvents::SelectNext(iDisplayObject* pDO)
{
   m_pFrame->SelectSegment(m_RightSegmentKey);
}

void CClosureJointDisplayObjectEvents::SelectPrevAdjacent(iDisplayObject* pDO)
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

void CClosureJointDisplayObjectEvents::SelectNextAdjacent(iDisplayObject* pDO)
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
STDMETHODIMP_(bool) CClosureJointDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CClosureJointDisplayObjectEvents,Events);
   pThis->EditClosureJoint(pDO);
   return true;
}

STDMETHODIMP_(bool) CClosureJointDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CClosureJointDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CClosureJointDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CClosureJointDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CClosureJointDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CClosureJointDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CClosureJointDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CClosureJointDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CClosureJointDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CClosureJointDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CClosureJointDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CClosureJointDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CClosureJointDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CClosureJointDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CClosureJointDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CClosureJointDisplayObjectEvents,Events);

   if ( nChar == VK_RETURN )
   {
      pThis->EditClosureJoint(pDO);
      return true;
   }
   else if ( nChar == VK_LEFT )
   {
      pThis->SelectPrev(pDO);
      return true;
   }
   else if ( nChar == VK_RIGHT )
   {
      pThis->SelectNext(pDO);
      return true;
   }
   else if ( nChar == VK_UP )
   {
      pThis->SelectPrevAdjacent(pDO);
      return true;
   }
   else if ( nChar == VK_DOWN )
   {
      pThis->SelectNextAdjacent(pDO);
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CClosureJointDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE_(CClosureJointDisplayObjectEvents,Events);
   AFX_MANAGE_STATE(AfxGetStaticModuleState());


   if ( pDO->IsSelected() )
   {
      CComPtr<iDisplayList> pList;
      pDO->GetDisplayList(&pList);

      CComPtr<iDisplayMgr> pDispMgr;
      pList->GetDisplayMgr(&pDispMgr);

      CDisplayView* pView = pDispMgr->GetView();
      CPGSpliceDoc* pDoc = (CPGSpliceDoc*)pView->GetDocument();

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
      pMenu->AppendMenu(ID_EDIT_CLOSURE,_T("Edit"),NULL);

      std::map<IDType,IBridgePlanViewEventCallback*> callbacks = pDoc->GetBridgePlanViewCallbacks();
      std::map<IDType,IBridgePlanViewEventCallback*>::iterator iter;
      for ( iter = callbacks.begin(); iter != callbacks.end(); iter++ )
      {
         IBridgePlanViewEventCallback* callback = iter->second;
         callback->OnClosureJointContextMenu(pThis->m_ClosureKey,pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,pThis->m_pFrame);

      delete pMenu;

      return true;
   }

   return false;
}

STDMETHODIMP_(void) CClosureJointDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CClosureJointDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CClosureJointDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CClosureJointDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CClosureJointDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CClosureJointDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CClosureJointDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CClosureJointDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CClosureJointDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CClosureJointDisplayObjectEvents,Events);
   pThis->SelectClosureJoint(pDO);
}

STDMETHODIMP_(void) CClosureJointDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CClosureJointDisplayObjectEvents,Events);
}

