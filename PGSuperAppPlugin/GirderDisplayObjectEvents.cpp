///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewGirderDisplayObjectEvents

CBridgePlanViewGirderDisplayObjectEvents::CBridgePlanViewGirderDisplayObjectEvents(const CGirderKey& girderKey,GroupIndexType nGroups,GirderIndexType nGirderThisGroup,CBridgeModelViewChildFrame* pFrame)
{
   m_GirderKey         = girderKey;
   m_nGroups           = nGroups;
   m_nGirdersThisGroup = nGirderThisGroup;
   m_pFrame            = pFrame;
}

BEGIN_INTERFACE_MAP(CBridgePlanViewGirderDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CBridgePlanViewGirderDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CBridgePlanViewGirderDisplayObjectEvents,Events);

void CBridgePlanViewGirderDisplayObjectEvents::EditGirder(iDisplayObject* pDO)
{
   m_pFrame->SendMessage(WM_COMMAND,ID_EDIT_GIRDER,0);
}

void CBridgePlanViewGirderDisplayObjectEvents::SelectGirder(iDisplayObject* pDO)
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
STDMETHODIMP_(bool) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);
   if (pDO->IsSelected())
   {
      pThis->EditGirder(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

STDMETHODIMP_(bool) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);

   if ( nChar == VK_RETURN )
   {
      pThis->EditGirder(pDO);
      return true;
   }
   else if ( nChar == VK_LEFT || nChar == VK_RIGHT )
   {
      pThis->SelectSpan();
      return true;
   }
   else if ( nChar == VK_UP )
   {
      pThis->SelectPrevGirder();
      return true;
   }
   else if ( nChar == VK_DOWN )
   {
      pThis->SelectNextGirder();
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE_(CBridgePlanViewGirderDisplayObjectEvents,Events);
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( pDO->IsSelected() )
   {
      CComPtr<iDisplayList> pList;
      pDO->GetDisplayList(&pList);

      CComPtr<iDisplayMgr> pDispMgr;
      pList->GetDisplayMgr(&pDispMgr);

      CDisplayView* pView = pDispMgr->GetView();
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
         pMenu->RemoveMenu(ID_PROJECT_DESIGNGIRDERDIRECTHOLDSLABOFFSET,MF_BYCOMMAND,nullptr);
      }

      pPGSDoc->BuildReportMenu(pMenu,true);

      const std::map<IDType,IBridgePlanViewEventCallback*>& callbacks = pPGSDoc->GetBridgePlanViewCallbacks();
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
      for ( ; callbackIter != callbackIterEnd; callbackIter++ )
      {
         IBridgePlanViewEventCallback* pCallback = callbackIter->second;
         pCallback->OnGirderContextMenu(pThis->m_GirderKey,pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,pThis->m_pFrame);

      delete pMenu;

      return true;
   }

   return false;
}

STDMETHODIMP_(void) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);
   pThis->SelectGirder(pDO);
}

STDMETHODIMP_(void) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);
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

BEGIN_INTERFACE_MAP(CBridgePlanViewSegmentDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CBridgePlanViewSegmentDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CBridgePlanViewSegmentDisplayObjectEvents,Events);

void CBridgePlanViewSegmentDisplayObjectEvents::EditSegment(iDisplayObject* pDO)
{
   m_pFrame->SendMessage(WM_COMMAND,ID_EDIT_SEGMENT,0);
}

void CBridgePlanViewSegmentDisplayObjectEvents::SelectSegment(iDisplayObject* pDO)
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
STDMETHODIMP_(bool) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSegmentDisplayObjectEvents,Events);
   CComPtr<iDisplayObject> pParent;
   pDO->GetParent(&pParent);
   if (pParent && pParent->IsSelected())
   {
      // we are part of a composite display object and the parent is selected
      // defer dbl-click handling to the parent
      return false;
   }

   if (pDO->IsSelected())
   {
      pThis->EditSegment(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

STDMETHODIMP_(bool) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSegmentDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSegmentDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSegmentDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSegmentDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSegmentDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSegmentDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSegmentDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CBridgePlanViewSegmentDisplayObjectEvents,Events);

   if ( nChar == VK_RETURN )
   {
      pThis->EditSegment(pDO);
      return true;
   }
   else if ( nChar == VK_LEFT )
   {
      pThis->SelectPrevSegment();
      return true;
   }
   else if ( nChar == VK_RIGHT )
   {
      pThis->SelectNextSegment();
      return true;
   }
   else if ( nChar == VK_UP )
   {
      pThis->SelectAdjacentPrevSegment();
      return true;
   }
   else if ( nChar == VK_DOWN )
   {
      pThis->SelectAdjacentNextSegment();
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE_(CBridgePlanViewSegmentDisplayObjectEvents,Events);
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( pDO->IsSelected() )
   {
      CComPtr<iDisplayList> pList;
      pDO->GetDisplayList(&pList);

      CComPtr<iDisplayMgr> pDispMgr;
      pList->GetDisplayMgr(&pDispMgr);

      CDisplayView* pView = pDispMgr->GetView();
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
         callback->OnGirderSegmentContextMenu(pThis->m_SegmentKey,pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,pThis->m_pFrame);

      delete pMenu;

      return true;
   }

   return false;
}

STDMETHODIMP_(void) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewSegmentDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CBridgePlanViewSegmentDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewSegmentDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewSegmentDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewSegmentDisplayObjectEvents,Events);
   pThis->SelectSegment(pDO);
}

STDMETHODIMP_(void) CBridgePlanViewSegmentDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewSegmentDisplayObjectEvents,Events);
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

BEGIN_INTERFACE_MAP(CBridgeSectionViewGirderDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CBridgeSectionViewGirderDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CBridgeSectionViewGirderDisplayObjectEvents,Events);

void CBridgeSectionViewGirderDisplayObjectEvents::EditGirder(iDisplayObject* pDO)
{
   m_pFrame->SendMessage(WM_COMMAND,ID_EDIT_GIRDER,0);
}

void CBridgeSectionViewGirderDisplayObjectEvents::SelectGirder(iDisplayObject* pDO)
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
STDMETHODIMP_(bool) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);
   if (pDO->IsSelected())
   {
      pThis->EditGirder(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

STDMETHODIMP_(bool) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDetla,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);

   bool bEventHandled = false;
   if ( nChar == VK_RETURN )
   {
      pThis->EditGirder(pDO);
      bEventHandled = true;
   }
   else if ( nChar == VK_LEFT )
   {
      pThis->SelectPrevGirder();
      bEventHandled = true;
   }
   else if ( nChar == VK_RIGHT )
   {
      pThis->SelectNextGirder();
      bEventHandled = true;
   }

   return bEventHandled;
}

STDMETHODIMP_(bool) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE_(CBridgeSectionViewGirderDisplayObjectEvents,Events);
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
         SegmentIndexType nSegments = pIBridgeDesc->GetBridgeDescription()->GetGirderGroup(pThis->m_GirderKey.groupIndex)->GetGirder(pThis->m_GirderKey.girderIndex)->GetSegmentCount();
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
         pCallback->OnGirderContextMenu(pThis->m_GirderKey,pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,pThis->m_pFrame);

      delete pMenu;

      return true;
   }

   return false;
}

STDMETHODIMP_(void) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);
   pThis->SelectGirder(pDO);
}

STDMETHODIMP_(void) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);
}
