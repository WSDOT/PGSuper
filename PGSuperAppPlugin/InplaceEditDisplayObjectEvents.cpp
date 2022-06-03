///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

// InplaceEditDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "InplaceEditDisplayObjectEvents.h"
#include "mfcdual.h"
#include "pgsuperdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInplaceEditDisplayObjectEvents

CInplaceEditDisplayObjectEvents::CInplaceEditDisplayObjectEvents(IBroker* pBroker)
{
   m_pBroker = pBroker;
}

BEGIN_INTERFACE_MAP(CInplaceEditDisplayObjectEvents, CCmdTarget)
   INTERFACE_PART(CInplaceEditDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CInplaceEditDisplayObjectEvents,Events);

/////////////////////////////////////////////////////////////////////////////
// CInplaceEditDisplayObjectEvents message handlers
STDMETHODIMP_(bool) CInplaceEditDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // do nothing
   return false;
}

STDMETHODIMP_(bool) CInplaceEditDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // do nothing
   return false;
}

STDMETHODIMP_(bool) CInplaceEditDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // do nothing
   return false;
}

STDMETHODIMP_(bool) CInplaceEditDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // do nothing
   return false;
}

STDMETHODIMP_(bool) CInplaceEditDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // do nothing
   return false;
}

STDMETHODIMP_(bool) CInplaceEditDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // do nothing
   return false;
}

STDMETHODIMP_(bool) CInplaceEditDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // do nothing
   return false;
}

STDMETHODIMP_(bool) CInplaceEditDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // do nothing
   return false;
}

STDMETHODIMP_(bool) CInplaceEditDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // do nothing
   return false;
}

STDMETHODIMP_(bool) CInplaceEditDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // do nothing
   return false;
}

STDMETHODIMP_(void) CInplaceEditDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // delgate to a virtual function that subclasses can override
   pThis->Handle_OnChanged(pDO);
}

STDMETHODIMP_(void) CInplaceEditDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // do nothing
}

STDMETHODIMP_(void) CInplaceEditDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // do nothing
}

STDMETHODIMP_(void) CInplaceEditDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // do nothing
}

STDMETHODIMP_(void) CInplaceEditDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // do nothing
}

STDMETHODIMP_(void) CInplaceEditDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CInplaceEditDisplayObjectEvents,Events);
   // do nothing
}
