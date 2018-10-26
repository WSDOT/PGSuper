///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "GirderDisplayObjectEvents.h"
#include "mfcdual.h"
#include "pgsuperdoc.h"
#include <IFace\Bridge.h>
#include <IFace\EditByUI.h>
#include <IReportManager.h>

#include "BridgePlanView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewGirderDisplayObjectEvents

CBridgePlanViewGirderDisplayObjectEvents::CBridgePlanViewGirderDisplayObjectEvents(SpanIndexType spanIdx,GirderIndexType gdrIdx,SpanIndexType nSpans,GirderIndexType nGirderThisSpan,CBridgeModelViewChildFrame* pFrame)
{
   m_SpanIdx          = spanIdx;
   m_GirderIdx        = gdrIdx;
   m_nSpans           = nSpans;
   m_nGirdersThisSpan = nGirderThisSpan;
   m_pFrame           = pFrame;
}

BEGIN_INTERFACE_MAP(CBridgePlanViewGirderDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CBridgePlanViewGirderDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CBridgePlanViewGirderDisplayObjectEvents,Events);

void CBridgePlanViewGirderDisplayObjectEvents::EditGirder(iDisplayObject* pDO)
{
   CComPtr<iDisplayList> pList;
   pDO->GetDisplayList(&pList);

   CComPtr<iDisplayMgr> pDispMgr;
   pList->GetDisplayMgr(&pDispMgr);

   CDisplayView* pView = pDispMgr->GetView();
   CDocument* pDoc = pView->GetDocument();

   ((CPGSuperDoc*)pDoc)->EditGirderDescription(m_SpanIdx,m_GirderIdx,EGD_GENERAL);
}

void CBridgePlanViewGirderDisplayObjectEvents::SelectGirder(iDisplayObject* pDO)
{
   // do the selection at the frame level because it will do this view
   // and the other view
   m_pFrame->SelectGirder(m_SpanIdx,m_GirderIdx);
}

void CBridgePlanViewGirderDisplayObjectEvents::SelectPrevGirder()
{
   if ( m_GirderIdx == 0 )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker,IBridge,pBridge);

      // if this is the first girder in this span
      if ( m_SpanIdx == 0 ) // and this is the first span
      {
         // select the alignment
         m_pFrame->GetBridgePlanView()->SelectAlignment(true);
      }
      else
      {
         GirderIndexType nGirders = pBridge->GetGirderCount(m_SpanIdx-1);
         m_pFrame->SelectGirder(m_SpanIdx-1,nGirders-1); // select the last girder in the previous span
      }
   }
   else
   {
      m_pFrame->SelectGirder(m_SpanIdx,m_GirderIdx-1); // select the previous girder in this span
   }
}

void CBridgePlanViewGirderDisplayObjectEvents::SelectNextGirder()
{
   if ( m_GirderIdx == m_nGirdersThisSpan-1 )
   {
      // if this is the last girder in the span
      if ( m_SpanIdx == m_nSpans-1 ) // and this is the last span
         m_pFrame->GetBridgePlanView()->SelectAlignment(true); // select the alignment
      else
         m_pFrame->SelectGirder(m_SpanIdx+1,0); // otherwise select the first girder in the next span
   }
   else
   {
      m_pFrame->SelectGirder(m_SpanIdx,m_GirderIdx+1); // otherwise select the next girder in this span
   }
}

void CBridgePlanViewGirderDisplayObjectEvents::SelectSpan()
{
   m_pFrame->SelectSpan(m_SpanIdx);
}

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewGirderDisplayObjectEvents message handlers
STDMETHODIMP_(bool) CBridgePlanViewGirderDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewGirderDisplayObjectEvents,Events);
   pThis->EditGirder(pDO);
   return true;
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

      CPGSuperDoc* pPGSuperDoc = (CPGSuperDoc*)pDoc;

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pPGSuperDoc->GetPluginCommandManager());
      pMenu->LoadMenu(IDR_SELECTED_GIRDER_CONTEXT,NULL);
      pPGSuperDoc->BuildReportMenu(pMenu,true);

      std::map<IDType,IBridgePlanViewEventCallback*> callbacks = pPGSuperDoc->GetBridgePlanViewCallbacks();
      std::map<IDType,IBridgePlanViewEventCallback*>::iterator iter;
      for ( iter = callbacks.begin(); iter != callbacks.end(); iter++ )
      {
         IBridgePlanViewEventCallback* callback = iter->second;
         callback->OnGirderContextMenu(pThis->m_SpanIdx,pThis->m_GirderIdx,pMenu);
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


// CBridgeSectionViewGirderDisplayObjectEvents

CBridgeSectionViewGirderDisplayObjectEvents::CBridgeSectionViewGirderDisplayObjectEvents(SpanIndexType spanIdx,GirderIndexType gdrIdx,SpanIndexType nSpans,GirderIndexType nGirderThisSpan,CBridgeModelViewChildFrame* pFrame)
{
   m_SpanIdx          = spanIdx;
   m_GirderIdx        = gdrIdx;
   m_nSpans           = nSpans;
   m_nGirdersThisSpan = nGirderThisSpan;
   m_pFrame           = pFrame;
}

BEGIN_INTERFACE_MAP(CBridgeSectionViewGirderDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CBridgeSectionViewGirderDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CBridgeSectionViewGirderDisplayObjectEvents,Events);

void CBridgeSectionViewGirderDisplayObjectEvents::EditGirder(iDisplayObject* pDO)
{
   CComPtr<iDisplayList> pList;
   pDO->GetDisplayList(&pList);

   CComPtr<iDisplayMgr> pDispMgr;
   pList->GetDisplayMgr(&pDispMgr);

   CDisplayView* pView = pDispMgr->GetView();
   CDocument* pDoc = pView->GetDocument();

   ((CPGSuperDoc*)pDoc)->EditGirderDescription(m_SpanIdx,m_GirderIdx,EGD_GENERAL);
}

void CBridgeSectionViewGirderDisplayObjectEvents::SelectGirder(iDisplayObject* pDO)
{
   // do the selection at the frame level because it will do this view
   // and the other view
   m_pFrame->SelectGirder(m_SpanIdx,m_GirderIdx);
}

void CBridgeSectionViewGirderDisplayObjectEvents::SelectPrevGirder()
{
   if ( m_GirderIdx == 0 )
   {
      m_pFrame->SelectDeck();
   }
   else
   {
      m_pFrame->SelectGirder(m_SpanIdx,m_GirderIdx-1);
   }
}

void CBridgeSectionViewGirderDisplayObjectEvents::SelectNextGirder()
{
   if ( m_GirderIdx == m_nGirdersThisSpan-1 )
   {
      m_pFrame->SelectDeck();
   }
   else
   {
      m_pFrame->SelectGirder(m_SpanIdx,m_GirderIdx+1); // otherwise select the next girder in this span
   }
}

void CBridgeSectionViewGirderDisplayObjectEvents::SelectSpan()
{
   m_pFrame->SelectSpan(m_SpanIdx);
}

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionViewGirderDisplayObjectEvents message handlers
STDMETHODIMP_(bool) CBridgeSectionViewGirderDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewGirderDisplayObjectEvents,Events);
   pThis->EditGirder(pDO);
   return true;
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
      CComPtr<iDisplayList> pList;
      pDO->GetDisplayList(&pList);

      CComPtr<iDisplayMgr> pDispMgr;
      pList->GetDisplayMgr(&pDispMgr);

      CDisplayView* pView = pDispMgr->GetView();
      CDocument* pDoc = pView->GetDocument();

      CPGSuperDoc* pPGSuperDoc = (CPGSuperDoc*)pDoc;

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pPGSuperDoc->GetPluginCommandManager());
      pMenu->LoadMenu(IDR_SELECTED_GIRDER_CONTEXT,NULL);
      pPGSuperDoc->BuildReportMenu(pMenu,true);

      std::map<IDType,IBridgeSectionViewEventCallback*> callbacks = pPGSuperDoc->GetBridgeSectionViewCallbacks();
      std::map<IDType,IBridgeSectionViewEventCallback*>::iterator iter;
      for ( iter = callbacks.begin(); iter != callbacks.end(); iter++ )
      {
         IBridgeSectionViewEventCallback* callback = iter->second;
         callback->OnGirderContextMenu(pThis->m_SpanIdx,pThis->m_GirderIdx,pMenu);
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

