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

// TogaGMDisplayMgrEventsImpl.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
//#include "PGSuperAppPlugin\PGSuperApp.h"
#include "TogaGMDisplayMgrEventsImpl.h"
#include "mfcdual.h"

#include <IFace\EditByUI.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CTogaGMDisplayMgrEventsImpl
CTogaGMDisplayMgrEventsImpl::CTogaGMDisplayMgrEventsImpl(CTxDOTOptionalDesignDoc* pDoc,CTxDOTOptionalDesignGirderViewPage* pFrame, CWnd* pParent)
{
   m_pDoc    = pDoc;
   m_pFrame  = pFrame;
   m_pParent = pParent;
}

CTogaGMDisplayMgrEventsImpl::~CTogaGMDisplayMgrEventsImpl()
{
}

BEGIN_INTERFACE_MAP(CTogaGMDisplayMgrEventsImpl,CCmdTarget)
   INTERFACE_PART(CTogaGMDisplayMgrEventsImpl,IID_iDisplayMgrEvents,Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CTogaGMDisplayMgrEventsImpl,Events);


STDMETHODIMP_(bool) CTogaGMDisplayMgrEventsImpl::XEvents::OnLButtonDblClk(iDisplayMgr* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTogaGMDisplayMgrEventsImpl,Events);

   pThis->m_pFrame->ShowCutDlg();

   return true;
}

STDMETHODIMP_(bool) CTogaGMDisplayMgrEventsImpl::XEvents::OnLButtonDown(iDisplayMgr* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTogaGMDisplayMgrEventsImpl,Events);

   pThis->m_pParent->SetFocus();

   return false;
}

STDMETHODIMP_(bool) CTogaGMDisplayMgrEventsImpl::XEvents::OnRButtonDblClk(iDisplayMgr* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTogaGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CTogaGMDisplayMgrEventsImpl::XEvents::OnRButtonDown(iDisplayMgr* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTogaGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CTogaGMDisplayMgrEventsImpl::XEvents::OnRButtonUp(iDisplayMgr* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTogaGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CTogaGMDisplayMgrEventsImpl::XEvents::OnLButtonUp(iDisplayMgr* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTogaGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CTogaGMDisplayMgrEventsImpl::XEvents::OnMouseMove(iDisplayMgr* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTogaGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CTogaGMDisplayMgrEventsImpl::XEvents::OnMouseWheel(iDisplayMgr* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CTogaGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CTogaGMDisplayMgrEventsImpl::XEvents::OnKeyDown(iDisplayMgr* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CTogaGMDisplayMgrEventsImpl,Events);

   if ( nChar == VK_LEFT )
   {
      pThis->m_pFrame->CutAtPrev();
      return true;
   }
   else if ( nChar == VK_RIGHT )
   {
      pThis->m_pFrame->CutAtNext();
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CTogaGMDisplayMgrEventsImpl::XEvents::OnContextMenu(iDisplayMgr* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE_(CTogaGMDisplayMgrEventsImpl,Events);
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CMenu menu;
   menu.LoadMenu(IDR_GIRDER_VIEW_CTX);
   CMenu* pcontext = menu.GetSubMenu(0);

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

   pcontext->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x,point.y, pThis->m_pParent );
   return true;
}
