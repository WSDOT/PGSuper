///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include "mfcdual.h"
#include "PGSuperDocBase.h"
#include <IFace\EditByUI.h>

#include <PgsExt\SpanData2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

BEGIN_INTERFACE_MAP(CBridgePlanViewSpanDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CBridgePlanViewSpanDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CBridgePlanViewSpanDisplayObjectEvents,Events);

void CBridgePlanViewSpanDisplayObjectEvents::EditSpan(iDisplayObject* pDO)
{
   CComPtr<iDisplayList> pList;
   pDO->GetDisplayList(&pList);

   CComPtr<iDisplayMgr> pDispMgr;
   pList->GetDisplayMgr(&pDispMgr);

   CDisplayView* pView = pDispMgr->GetView();
   CDocument* pDoc = pView->GetDocument();

   ((CPGSDocBase*)pDoc)->EditSpanDescription(m_SpanIdx,ESD_GENERAL);
}

void CBridgePlanViewSpanDisplayObjectEvents::SelectSpan(iDisplayObject* pDO)
{
   // do the selection at the frame level because it will do this view
   // and the other view
   m_pFrame->SelectSpan(m_SpanIdx);
}

void CBridgePlanViewSpanDisplayObjectEvents::SelectPrev(iDisplayObject* pDO)
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

void CBridgePlanViewSpanDisplayObjectEvents::SelectNext(iDisplayObject* pDO)
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
STDMETHODIMP_(bool) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSpanDisplayObjectEvents,Events);
   if (pDO->IsSelected())
   {
      pThis->EditSpan(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

STDMETHODIMP_(bool) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSpanDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSpanDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSpanDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSpanDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSpanDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSpanDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSpanDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CBridgePlanViewSpanDisplayObjectEvents,Events);

   if ( nChar == VK_RETURN )
   {
      pThis->EditSpan(pDO);
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
   else if ( nChar == VK_UP || nChar == VK_DOWN )
   {
      pThis->m_pFrame->SelectGirder( CSegmentKey(pThis->m_SpanIdx,0,INVALID_INDEX) );
      return true;
   }
   else if ( nChar == VK_DELETE )
   {
      pThis->m_pFrame->PostMessage(WM_COMMAND,ID_DELETE,0);
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE_(CBridgePlanViewSpanDisplayObjectEvents,Events);
   AFX_MANAGE_STATE(AfxGetStaticModuleState());


   if ( pDO->IsSelected() )
   {
      CComPtr<iDisplayList> pList;
      pDO->GetDisplayList(&pList);

      CComPtr<iDisplayMgr> pDispMgr;
      pList->GetDisplayMgr(&pDispMgr);

      CDisplayView* pView = pDispMgr->GetView();
      CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
      pMenu->LoadMenu(IDR_SELECTED_SPAN_CONTEXT,nullptr);

      const std::map<IDType,IBridgePlanViewEventCallback*>& callbacks = pDoc->GetBridgePlanViewCallbacks();
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
      for ( ; callbackIter != callbackIterEnd; callbackIter++ )
      {
         IBridgePlanViewEventCallback* pCallback = callbackIter->second;
         pCallback->OnSpanContextMenu(pThis->m_SpanIdx,pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,pThis->m_pFrame);

      return true;
   }


   return false;
}

STDMETHODIMP_(void) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewSpanDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CBridgePlanViewSpanDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewSpanDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewSpanDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewSpanDisplayObjectEvents,Events);
   pThis->SelectSpan(pDO);
}

STDMETHODIMP_(void) CBridgePlanViewSpanDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewSpanDisplayObjectEvents,Events);
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

BEGIN_INTERFACE_MAP(CBridgeSectionViewSpanDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CBridgeSectionViewSpanDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CBridgeSectionViewSpanDisplayObjectEvents,Events);

void CBridgeSectionViewSpanDisplayObjectEvents::EditSpan(iDisplayObject* pDO)
{
   CComPtr<iDisplayList> pList;
   pDO->GetDisplayList(&pList);

   CComPtr<iDisplayMgr> pDispMgr;
   pList->GetDisplayMgr(&pDispMgr);

   CDisplayView* pView = pDispMgr->GetView();
   CDocument* pDoc = pView->GetDocument();

   ((CPGSDocBase*)pDoc)->EditSpanDescription(m_SpanIdx,ESD_GENERAL);
}

void CBridgeSectionViewSpanDisplayObjectEvents::SelectSpan(iDisplayObject* pDO)
{
   // do the selection at the frame level because it will do this view
   // and the other view
   m_pFrame->SelectSpan(m_SpanIdx);
}

void CBridgeSectionViewSpanDisplayObjectEvents::SelectPrev(iDisplayObject* pDO)
{
   m_pFrame->SelectPier(m_SpanIdx);
}

void CBridgeSectionViewSpanDisplayObjectEvents::SelectNext(iDisplayObject* pDO)
{
   m_pFrame->SelectPier(m_SpanIdx+1);
}

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionViewSpanDisplayObjectEvents message handlers
STDMETHODIMP_(bool) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO, UINT nFlags, CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSpanDisplayObjectEvents, Events);
   if (pDO->IsSelected())
   {
      pThis->EditSpan(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

STDMETHODIMP_(bool) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSpanDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSpanDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSpanDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSpanDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSpanDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSpanDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSpanDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CBridgeSectionViewSpanDisplayObjectEvents,Events);

   if ( nChar == VK_RETURN )
   {
      pThis->EditSpan(pDO);
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
   else if ( nChar == VK_UP || nChar == VK_DOWN )
   {
      pThis->m_pFrame->SelectGirder( CSegmentKey(pThis->m_SpanIdx,0,INVALID_INDEX) );
      return true;
   }
   else if ( nChar == VK_DELETE )
   {
      pThis->m_pFrame->PostMessage(WM_COMMAND,ID_DELETE,0);
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE_(CBridgeSectionViewSpanDisplayObjectEvents,Events);
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

STDMETHODIMP_(void) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionViewSpanDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CBridgeSectionViewSpanDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionViewSpanDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionViewSpanDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionViewSpanDisplayObjectEvents,Events);
   pThis->SelectSpan(pDO);
}

STDMETHODIMP_(void) CBridgeSectionViewSpanDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionViewSpanDisplayObjectEvents,Events);
}
