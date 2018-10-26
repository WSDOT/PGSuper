///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// GMDisplayMgrEventsImpl.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "GMDisplayMgrEventsImpl.h"
#include "mfcdual.h"
#include "GirderModelChildFrame.h"
#include <IReportManager.h>

#include <IFace\EditByUI.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CGMDisplayMgrEventsImpl
CGMDisplayMgrEventsImpl::CGMDisplayMgrEventsImpl(CPGSuperDoc* pDoc,CGirderModelChildFrame* pFrame, CWnd* pParent)
{
   m_pDoc    = pDoc;
   m_pFrame  = pFrame;
   m_pParent = pParent;
}

CGMDisplayMgrEventsImpl::~CGMDisplayMgrEventsImpl()
{
}

BEGIN_INTERFACE_MAP(CGMDisplayMgrEventsImpl,CCmdTarget)
   INTERFACE_PART(CGMDisplayMgrEventsImpl,IID_iDisplayMgrEvents,Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CGMDisplayMgrEventsImpl,Events);


STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnLButtonDblClk(iDisplayMgr* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);

   SpanIndexType spanIdx;
   GirderIndexType gdrIdx;
   pThis->m_pFrame->GetSpanAndGirderSelection(&spanIdx,&gdrIdx);

   // if a span and girder isn't selected, do nothing
   if ( spanIdx == ALL_SPANS || gdrIdx == ALL_GIRDERS )
      return true;

   pThis->m_pDoc->EditGirderDescription(spanIdx,gdrIdx,EGD_GENERAL);

   return true;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnLButtonDown(iDisplayMgr* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);

   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnRButtonDblClk(iDisplayMgr* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnRButtonDown(iDisplayMgr* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnRButtonUp(iDisplayMgr* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnLButtonUp(iDisplayMgr* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnMouseMove(iDisplayMgr* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnMouseWheel(iDisplayMgr* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnKeyDown(iDisplayMgr* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnContextMenu(iDisplayMgr* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE_(CGMDisplayMgrEventsImpl,Events);
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CMenu menu;
   menu.LoadMenu(IDR_GIRDER_CTX);
   CEAFMenu contextMenu(menu.Detach(),pThis->m_pDoc->GetPluginCommandManager());

   CEAFMenu* pmnuReport = contextMenu.GetSubMenu(0);
   pThis->m_pDoc->BuildReportMenu(pmnuReport,true);

   if ( point.x < 0 || point.y < 0 )
   {
      // the context menu key or Shift+F10 was pressed
      // need some real coordinates (how about the center of the client area)
      CRect rClient;
      pThis->m_pParent->GetClientRect(&rClient);
      CPoint center = rClient.TopLeft();
      pThis->m_pParent->ClientToScreen(&center);
      point = center;
   }


   pmnuReport->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x,point.y, pThis->m_pFrame );
   return true;
}
