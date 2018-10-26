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

// GMDisplayMgrEventsImpl.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "GMDisplayMgrEventsImpl.h"
#include "mfcdual.h"
#include "GirderModelChildFrame.h"
#include <IReportManager.h>

#include <IFace\EditByUI.h>

#include "PGSpliceDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CGMDisplayMgrEventsImpl
CGMDisplayMgrEventsImpl::CGMDisplayMgrEventsImpl(CPGSuperDocBase* pDoc,CGirderModelChildFrame* pFrame, CWnd* pParent,bool bGirderElevation)
{
   m_pDoc    = pDoc;
   m_pFrame  = pFrame;
   m_pParent = pParent;
   m_bGirderElevation = bGirderElevation;
}

CGMDisplayMgrEventsImpl::~CGMDisplayMgrEventsImpl()
{
}

BEGIN_INTERFACE_MAP(CGMDisplayMgrEventsImpl,CCmdTarget)
   INTERFACE_PART(CGMDisplayMgrEventsImpl,IID_iDisplayMgrEvents,Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CGMDisplayMgrEventsImpl,Events);


STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnLButtonDblClk(iDisplayMgr* pDisplayMgr,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);

   CGirderKey girderKey = pThis->m_pFrame->GetSelection();

   if ( girderKey.groupIndex == ALL_GROUPS )
      girderKey.groupIndex = 0;
   
   if ( girderKey.girderIndex == ALL_GIRDERS )
      girderKey.girderIndex = 0;

   pThis->m_pDoc->EditGirderDescription(girderKey,EGD_GENERAL);

   return true;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnLButtonDown(iDisplayMgr* pDisplayMgr,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);

   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnRButtonDblClk(iDisplayMgr* pDisplayMgr,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnRButtonDown(iDisplayMgr* pDisplayMgr,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnRButtonUp(iDisplayMgr* pDisplayMgr,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnLButtonUp(iDisplayMgr* pDisplayMgr,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnMouseMove(iDisplayMgr* pDisplayMgr,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnMouseWheel(iDisplayMgr* pDisplayMgr,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnKeyDown(iDisplayMgr* pDisplayMgr,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CGMDisplayMgrEventsImpl,Events);
   return false;
}

STDMETHODIMP_(bool) CGMDisplayMgrEventsImpl::XEvents::OnContextMenu(iDisplayMgr* pDisplayMgr,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE_(CGMDisplayMgrEventsImpl,Events);
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CDisplayView* pView = pDisplayMgr->GetView();
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)pView->GetDocument();

   CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
   pMenu->LoadMenu(IDR_GIRDER_CTX,NULL);

   // PGSplice does not use moment loads
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
   {
      pMenu->RemoveMenu(ID_ADD_MOMENT_LOAD,MF_BYCOMMAND,NULL);
   }

   pDoc->BuildReportMenu(pMenu,true);

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

   if ( pThis->m_bGirderElevation )
   {
      std::map<IDType,IGirderElevationViewEventCallback*> callbacks = pDoc->GetGirderElevationViewCallbacks();
      std::map<IDType,IGirderElevationViewEventCallback*>::iterator iter;
      for ( iter = callbacks.begin(); iter != callbacks.end(); iter++ )
      {
         IGirderElevationViewEventCallback* callback = iter->second;
         callback->OnBackgroundContextMenu(pMenu);
      }
   }
   else
   {
      std::map<IDType,IGirderSectionViewEventCallback*> callbacks = pDoc->GetGirderSectionViewCallbacks();
      std::map<IDType,IGirderSectionViewEventCallback*>::iterator iter;
      for ( iter = callbacks.begin(); iter != callbacks.end(); iter++ )
      {
         IGirderSectionViewEventCallback* callback = iter->second;
         callback->OnBackgroundContextMenu(pMenu);
      }
   }

   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x,point.y, pThis->m_pFrame );
   return true;
}
