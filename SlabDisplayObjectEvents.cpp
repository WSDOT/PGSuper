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

// SlabDisplayObjectEvents.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "SlabDisplayObjectEvents.h"
#include "BridgeSectionCutDisplayImpl.h"
#include "mfcdual.h"
#include "pgsuperdoc.h"
#include <IFace\Bridge.h>
#include <IFace\EditByUI.h>
#include "BridgeSectionView.h"

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewSlabDisplayObjectEvents

CBridgePlanViewSlabDisplayObjectEvents::CBridgePlanViewSlabDisplayObjectEvents(CPGSuperDocBase* pDoc,IBroker* pBroker, CBridgeModelViewChildFrame* pFrame,bool bFillIfNotSelected)
{
   m_pDoc = pDoc;
   m_pBroker = pBroker;
   m_pFrame = pFrame;
   m_bFillIfNotSelected = bFillIfNotSelected;

   GET_IFACE(IBridge,pBridge);
   m_nPiers = pBridge->GetPierCount();
}

BEGIN_INTERFACE_MAP(CBridgePlanViewSlabDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CBridgePlanViewSlabDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CBridgePlanViewSlabDisplayObjectEvents,Events);

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
STDMETHODIMP_(bool) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSlabDisplayObjectEvents,Events);

   pThis->EditSlab();
   
   return true;
}

STDMETHODIMP_(bool) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSlabDisplayObjectEvents,Events);
   return true;
}

STDMETHODIMP_(bool) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSlabDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSlabDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSlabDisplayObjectEvents,Events);
   return true;
}

STDMETHODIMP_(bool) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSlabDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSlabDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CBridgePlanViewSlabDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CBridgePlanViewSlabDisplayObjectEvents,Events);

   if ( nChar == VK_RETURN )
   {
      pThis->EditSlab();
      return true;
   }
   else if ( nChar == VK_LEFT )
   {
      pThis->SelectPrev();
      return true;
   }
   else if ( nChar == VK_RIGHT )
   {
      pThis->SelectNext();
      return true;
   }
   else if ( nChar == VK_UP || nChar == VK_DOWN )
   {
      pThis->m_pFrame->SelectGirder( CSegmentKey(0,0,INVALID_INDEX) );
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE_(CBridgePlanViewSlabDisplayObjectEvents,Events);
   AFX_MANAGE_STATE(AfxGetStaticModuleState());


   if ( pDO->IsSelected() )
   {
      CComPtr<iDisplayList> pList;
      pDO->GetDisplayList(&pList);

      CComPtr<iDisplayMgr> pDispMgr;
      pList->GetDisplayMgr(&pDispMgr);

      CDisplayView* pView = pDispMgr->GetView();
      CPGSuperDoc* pDoc = (CPGSuperDoc*)pView->GetDocument();

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
      pMenu->LoadMenu(IDR_SELECTED_DECK_CONTEXT,NULL);

      const std::map<IDType,IBridgePlanViewEventCallback*>& callbacks = pDoc->GetBridgePlanViewCallbacks();
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
      for ( ; callbackIter != callbackIterEnd; callbackIter++ )
      {
         IBridgePlanViewEventCallback* pCallback = callbackIter->second;
         pCallback->OnDeckContextMenu(pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,pThis->m_pFrame);

      return true;
   }

   return false;
}

STDMETHODIMP_(void) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewSlabDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CBridgePlanViewSlabDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewSlabDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewSlabDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewSlabDisplayObjectEvents,Events);
   
   IDType id = pDO->GetID();
   if ( id == DECK_ID ) // plan view
   {
      // fill when selected
      CComQIPtr<iPointDisplayObject> pntDO(pDO);
   
      CComPtr<iDrawPointStrategy> strategy;
      pntDO->GetDrawingStrategy(&strategy);

      CComQIPtr<iShapeDrawStrategy> shape_draw(strategy);

      shape_draw->DoFill(true);
   }

   pThis->m_pFrame->SelectDeck();
}

STDMETHODIMP_(void) CBridgePlanViewSlabDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgePlanViewSlabDisplayObjectEvents,Events);

   IDType id = pDO->GetID();
   if ( id == DECK_ID )
   {
      // don't fill when not selected
      CComQIPtr<iPointDisplayObject> pntDO(pDO);
   
      CComPtr<iDrawPointStrategy> strategy;
      pntDO->GetDrawingStrategy(&strategy);

      CComQIPtr<iShapeDrawStrategy> shape_draw(strategy);

      shape_draw->DoFill(pThis->m_bFillIfNotSelected);
   }
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionViewSlabDisplayObjectEvents

CBridgeSectionViewSlabDisplayObjectEvents::CBridgeSectionViewSlabDisplayObjectEvents(CPGSuperDocBase* pDoc,IBroker* pBroker, CBridgeModelViewChildFrame* pFrame,bool bFillIfNotSelected)
{
   m_pDoc = pDoc;
   m_pBroker = pBroker;
   m_pFrame = pFrame;
   m_bFillIfNotSelected = bFillIfNotSelected;
}

BEGIN_INTERFACE_MAP(CBridgeSectionViewSlabDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CBridgeSectionViewSlabDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CBridgeSectionViewSlabDisplayObjectEvents,Events);

void CBridgeSectionViewSlabDisplayObjectEvents::EditSlab()
{
   m_pDoc->EditBridgeDescription(EBD_DECK);
}

void CBridgeSectionViewSlabDisplayObjectEvents::SelectPrev()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType grpIdx = m_pFrame->GetBridgeSectionView()->GetGroupIndex();
   GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
   m_pFrame->SelectGirder( CSegmentKey(grpIdx,nGirders-1,INVALID_INDEX) );
}

void CBridgeSectionViewSlabDisplayObjectEvents::SelectNext()
{
   GroupIndexType grpIdx = m_pFrame->GetBridgeSectionView()->GetGroupIndex();
   m_pFrame->SelectGirder( CSegmentKey(grpIdx,0,INVALID_INDEX) );
}

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionViewSlabDisplayObjectEvents message handlers
STDMETHODIMP_(bool) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSlabDisplayObjectEvents,Events);

   pThis->EditSlab();
   
   return true;
}

STDMETHODIMP_(bool) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSlabDisplayObjectEvents,Events);
   return true;
}

STDMETHODIMP_(bool) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSlabDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSlabDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSlabDisplayObjectEvents,Events);
   return true;
}

STDMETHODIMP_(bool) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSlabDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSlabDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionViewSlabDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CBridgeSectionViewSlabDisplayObjectEvents,Events);

   if ( nChar == VK_RETURN )
   {
      pThis->EditSlab();
      return true;
   }
   else if ( nChar == VK_LEFT )
   {
      pThis->SelectPrev();
      return true;
   }
   else if ( nChar == VK_RIGHT )
   {
      pThis->SelectNext();
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE_(CBridgeSectionViewSlabDisplayObjectEvents,Events);
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( pDO->IsSelected() )
   {
      CComPtr<iDisplayList> pList;
      pDO->GetDisplayList(&pList);

      CComPtr<iDisplayMgr> pDispMgr;
      pList->GetDisplayMgr(&pDispMgr);

      CDisplayView* pView = pDispMgr->GetView();
      CPGSuperDoc* pDoc = (CPGSuperDoc*)pView->GetDocument();

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
      pMenu->LoadMenu(IDR_SELECTED_DECK_CONTEXT,NULL);

      const std::map<IDType,IBridgeSectionViewEventCallback*>& callbacks = pDoc->GetBridgeSectionViewCallbacks();
      std::map<IDType,IBridgeSectionViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
      std::map<IDType,IBridgeSectionViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
      for ( ; callbackIter != callbackIterEnd; callbackIter++ )
      {
         IBridgeSectionViewEventCallback* pCallback = callbackIter->second;
         pCallback->OnDeckContextMenu(pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,pThis->m_pFrame);

      return true;
   }

   return false;
}

STDMETHODIMP_(void) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionViewSlabDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CBridgeSectionViewSlabDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionViewSlabDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionViewSlabDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionViewSlabDisplayObjectEvents,Events);
   
   IDType id = pDO->GetID();
   if ( id == DECK_ID ) // plan view
   {
      // fill when selected
      CComQIPtr<iPointDisplayObject> pntDO(pDO);
   
      CComPtr<iDrawPointStrategy> strategy;
      pntDO->GetDrawingStrategy(&strategy);

      CComQIPtr<iShapeDrawStrategy> shape_draw(strategy);

      shape_draw->DoFill(true);
   }

   pThis->m_pFrame->SelectDeck();
}

STDMETHODIMP_(void) CBridgeSectionViewSlabDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionViewSlabDisplayObjectEvents,Events);

   IDType id = pDO->GetID();
   if ( id == DECK_ID )
   {
      // don't fill when not selected
      CComQIPtr<iPointDisplayObject> pntDO(pDO);
   
      CComPtr<iDrawPointStrategy> strategy;
      pntDO->GetDrawingStrategy(&strategy);

      CComQIPtr<iShapeDrawStrategy> shape_draw(strategy);

      shape_draw->DoFill(pThis->m_bFillIfNotSelected);
   }
}
