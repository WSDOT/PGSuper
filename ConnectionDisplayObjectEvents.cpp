///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// ConnectionDisplayObjectEvents.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "ConnectionDisplayObjectEvents.h"
#include "mfcdual.h"
#include "pgsuperdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConnectionDisplayObjectEvents

CConnectionDisplayObjectEvents::CConnectionDisplayObjectEvents(PierIndexType pierIdx)
{
   m_PierIdx = pierIdx;
}

BEGIN_INTERFACE_MAP(CConnectionDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CConnectionDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CConnectionDisplayObjectEvents,Events);

void CConnectionDisplayObjectEvents::EditPier(iDisplayObject* pDO)
{
   CComPtr<iDisplayList> pList;
   pDO->GetDisplayList(&pList);

   CComPtr<iDisplayMgr> pDispMgr;
   pList->GetDisplayMgr(&pDispMgr);

   CDisplayView* pView = pDispMgr->GetView();
   CDocument* pDoc = pView->GetDocument();

   ((CPGSuperDoc*)pDoc)->EditPierDescription(m_PierIdx,EPD_CONNECTION);
}

/////////////////////////////////////////////////////////////////////////////
// CConnectionDisplayObjectEvents message handlers
STDMETHODIMP_(bool) CConnectionDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);
   pThis->EditPier(pDO);
   return true;
}

STDMETHODIMP_(bool) CConnectionDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CConnectionDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CConnectionDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CConnectionDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CConnectionDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CConnectionDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CConnectionDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CConnectionDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);

   if ( nChar == VK_RETURN )
   {
      pThis->EditPier(pDO);
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CConnectionDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(void) CConnectionDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CConnectionDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CConnectionDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CConnectionDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CConnectionDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CConnectionDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CConnectionDisplayObjectEvents,Events);
}

