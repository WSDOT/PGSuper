///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

// TrafficBarrierDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TrafficBarrierDisplayObjectEvents.h"
#include "BridgeModelViewChildFrame.h"
#include "mfcdual.h"
#include "PGSuperDocBase.h"
#include "BridgeSectionView.h"

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTrafficBarrierDisplayObjectEvents

CTrafficBarrierDisplayObjectEvents::CTrafficBarrierDisplayObjectEvents(IBroker* pBroker, CBridgeModelViewChildFrame* pFrame,pgsTypes::TrafficBarrierOrientation orientation)
{
   m_pBroker = pBroker;
   m_pFrame = pFrame;
   m_TrafficBarrierOrientation = orientation;
}

CTrafficBarrierDisplayObjectEvents::~CTrafficBarrierDisplayObjectEvents()
{
}

BEGIN_INTERFACE_MAP(CTrafficBarrierDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CTrafficBarrierDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CTrafficBarrierDisplayObjectEvents,Events);


void CTrafficBarrierDisplayObjectEvents::EditBarrier()
{
   m_pFrame->SendMessage(WM_COMMAND,ID_PROJECT_BARRIER,0);
}

void CTrafficBarrierDisplayObjectEvents::SelectPrev()
{
   if (m_TrafficBarrierOrientation == pgsTypes::tboLeft)
   {
      // select last girder
      GET_IFACE(IBridgeDescription, pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      GroupIndexType grpIdx = m_pFrame->GetBridgeSectionView()->GetGroupIndex();
      GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
      m_pFrame->SelectGirder(CSegmentKey(grpIdx, nGirders - 1, INVALID_INDEX));
   }
   else
   {
      m_pFrame->SelectDeck();
   }
}

void CTrafficBarrierDisplayObjectEvents::SelectNext()
{
   if (m_TrafficBarrierOrientation == pgsTypes::tboLeft)
   {
      m_pFrame->SelectDeck();
   }
   else
   {
      // select first girder
      GroupIndexType grpIdx = m_pFrame->GetBridgeSectionView()->GetGroupIndex();
      m_pFrame->SelectGirder(CSegmentKey(grpIdx, 0, INVALID_INDEX));
   }
}

/////////////////////////////////////////////////////////////////////////////
// CTrafficBarrierDisplayObjectEvents message handlers
STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO, UINT nFlags, CPoint point)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents, Events);

   if (pDO->IsSelected())
   {
      pThis->EditBarrier();
      return true;
   }
   else
   {
      return false;
   }
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents,Events);
   return true;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents,Events);

   if ( nChar == VK_RETURN )
   {
      pThis->EditBarrier();
      return true;
   }
   else if (nChar == VK_LEFT)
   {
      pThis->SelectPrev();
      return true;
   }
   else if (nChar == VK_RIGHT)
   {
      pThis->SelectNext();
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CTrafficBarrierDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents,Events);
   if ( pDO->IsSelected() )
   {
      CComPtr<iDisplayList> pList;
      pDO->GetDisplayList(&pList);

      CComPtr<iDisplayMgr> pDispMgr;
      pList->GetDisplayMgr(&pDispMgr);

      CDisplayView* pView = pDispMgr->GetView();
      CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
      pMenu->AppendMenu(ID_PROJECT_BARRIER, _T("Edit Railing System"), nullptr);

      bool bResult = false;
      if ( 0 < pMenu->GetMenuItemCount() )
      {
         pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,pThis->m_pFrame);
         bResult = true;
      }

      delete pMenu;

      return bResult;
   }

   return false;
}

STDMETHODIMP_(void) CTrafficBarrierDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CTrafficBarrierDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CTrafficBarrierDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CTrafficBarrierDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CTrafficBarrierDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents,Events);
   pThis->m_pFrame->ClearSelection();
   pThis->m_pFrame->SelectTrafficBarrier(pThis->m_TrafficBarrierOrientation);
}

STDMETHODIMP_(void) CTrafficBarrierDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CTrafficBarrierDisplayObjectEvents,Events);
}
