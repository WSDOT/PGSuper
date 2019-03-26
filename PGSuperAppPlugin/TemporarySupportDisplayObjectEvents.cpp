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

// TemporarySupportDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"
#include "TemporarySupportDisplayObjectEvents.h"
#include "mfcdual.h"
#include "PGSpliceDoc.h"
#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTemporarySupportDisplayObjectEvents

CTemporarySupportDisplayObjectEvents::CTemporarySupportDisplayObjectEvents(SupportIDType tsID,CBridgeModelViewChildFrame* pFrame)
{
   m_tsID   = tsID;
   m_pFrame = pFrame;

   m_pPrevTS = nullptr;
   m_pNextTS = nullptr;

   m_pPrevPier = nullptr;
   m_pNextPier = nullptr;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CTemporarySupportData* pTS = pIBridgeDesc->FindTemporarySupport(tsID);
   SupportIndexType tsIdx = pTS->GetIndex();

   m_pNextTS = pIBridgeDesc->GetTemporarySupport(tsIdx+1);
   m_pPrevTS = pIBridgeDesc->GetTemporarySupport(tsIdx-1);

   if ( (m_pPrevTS && (m_pPrevTS->GetSpan() != pTS->GetSpan())) || m_pPrevTS == nullptr)
   {
      m_pPrevTS = nullptr;
      m_pPrevPier = pTS->GetSpan()->GetPrevPier();
   }

   if ( (m_pNextTS && (m_pNextTS->GetSpan() != pTS->GetSpan())) || m_pNextTS == nullptr )
   {
      m_pNextTS = nullptr;
      m_pNextPier = pTS->GetSpan()->GetNextPier();
   }
}

BEGIN_INTERFACE_MAP(CTemporarySupportDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CTemporarySupportDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CTemporarySupportDisplayObjectEvents,Events);

void CTemporarySupportDisplayObjectEvents::EditTemporarySupport(iDisplayObject* pDO)
{
   m_pFrame->SendMessage(WM_COMMAND,ID_EDIT_TEMPORARY_SUPPORT,0);
}

void CTemporarySupportDisplayObjectEvents::SelectTemporarySupport(iDisplayObject* pDO)
{
   // do the selection at the frame level because it will do this view
   // and the other view
   m_pFrame->SelectTemporarySupport(m_tsID);
}

void CTemporarySupportDisplayObjectEvents::SelectPrev(iDisplayObject* pDO)
{
   if ( m_pPrevTS )
   {
      m_pFrame->SelectTemporarySupport(m_pPrevTS->GetID());
   }
   else
   {
      m_pFrame->SelectPier(m_pPrevPier->GetIndex());
   }
}

void CTemporarySupportDisplayObjectEvents::SelectNext(iDisplayObject* pDO)
{
   if ( m_pNextTS )
   {
      m_pFrame->SelectTemporarySupport(m_pNextTS->GetID());
   }
   else
   {
      m_pFrame->SelectPier(m_pNextPier->GetIndex());
   }
}

/////////////////////////////////////////////////////////////////////////////
// CTemporarySupportDisplayObjectEvents message handlers
STDMETHODIMP_(bool) CTemporarySupportDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTemporarySupportDisplayObjectEvents,Events);
   if (pDO->IsSelected())
   {
      pThis->EditTemporarySupport(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

STDMETHODIMP_(bool) CTemporarySupportDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTemporarySupportDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CTemporarySupportDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTemporarySupportDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CTemporarySupportDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTemporarySupportDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CTemporarySupportDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTemporarySupportDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CTemporarySupportDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTemporarySupportDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CTemporarySupportDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTemporarySupportDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CTemporarySupportDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CTemporarySupportDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CTemporarySupportDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CTemporarySupportDisplayObjectEvents,Events);

   if ( nChar == VK_RETURN )
   {
      pThis->EditTemporarySupport(pDO);
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
#pragma Reminder("UPDATE")
   //else if ( nChar == VK_UP || nChar == VK_DOWN )
   //{
   //   pThis->m_pFrame->SelectGirder(pThis->m_SpanIdx,0);
   //   return true;
   //}
   else if ( nChar == VK_DELETE )
   {
      pThis->m_pFrame->SendMessage(WM_COMMAND,ID_DELETE,0);
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CTemporarySupportDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE_(CTemporarySupportDisplayObjectEvents,Events);
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
      pMenu->LoadMenu(IDR_SELECTED_TEMPORARY_SUPPORT_CONTEXT,nullptr);

      std::map<IDType,IBridgePlanViewEventCallback*> callbacks = pDoc->GetBridgePlanViewCallbacks();
      std::map<IDType,IBridgePlanViewEventCallback*>::iterator iter;
      for ( iter = callbacks.begin(); iter != callbacks.end(); iter++ )
      {
         IBridgePlanViewEventCallback* callback = iter->second;
         callback->OnTemporarySupportContextMenu(pThis->m_tsID,pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,pThis->m_pFrame);

      delete pMenu;

      return true;
   }

   return false;
}

STDMETHODIMP_(void) CTemporarySupportDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CTemporarySupportDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CTemporarySupportDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CTemporarySupportDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CTemporarySupportDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CTemporarySupportDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CTemporarySupportDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CTemporarySupportDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CTemporarySupportDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CTemporarySupportDisplayObjectEvents,Events);
   pThis->SelectTemporarySupport(pDO);
}

STDMETHODIMP_(void) CTemporarySupportDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CTemporarySupportDisplayObjectEvents,Events);
}

