///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

// BridgeDisplayObjectEvents.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "ProfileDisplayObjectEvents.h"
#include "BridgeModelViewChildFrame.h"
#include "PGSuperDocBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProfileDisplayObjectEvents

CProfileDisplayObjectEvents::CProfileDisplayObjectEvents(IBroker* pBroker, CBridgeModelViewChildFrame* pFrame,iDisplayObject* pDO)
{
   m_pBroker = pBroker;
   m_pFrame = pFrame;
   m_DispObj.Attach(pDO);
}

CProfileDisplayObjectEvents::~CProfileDisplayObjectEvents()
{
}

void CProfileDisplayObjectEvents::OnFinalRelease()
{
   m_DispObj.Detach();
   CCmdTarget::OnFinalRelease();
}

BEGIN_INTERFACE_MAP(CProfileDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CProfileDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

// Not sure why, but the DELEGATE_CUSTOM_INTERFACE macro isn't working
// Since the is simple, I've just implemented it directly
//DELEGATE_CUSTOM_INTERFACE(CProfileDisplayObjectEvents,Events);
STDMETHODIMP_(ULONG) CProfileDisplayObjectEvents::XEvents::AddRef() 
{ 
   METHOD_PROLOGUE(CProfileDisplayObjectEvents, Events) 
   return pThis->ExternalAddRef(); 
} 
STDMETHODIMP_(ULONG) CProfileDisplayObjectEvents::XEvents::Release() 
{ 
   METHOD_PROLOGUE(CProfileDisplayObjectEvents, Events) 
   return pThis->ExternalRelease(); 
} 
STDMETHODIMP CProfileDisplayObjectEvents::XEvents::QueryInterface( 
   REFIID iid, LPVOID* ppvObj) 
{ 
   METHOD_PROLOGUE(CProfileDisplayObjectEvents, Events) 
   return pThis->ExternalQueryInterface( &iid, ppvObj ); 
}

void CProfileDisplayObjectEvents::EditProfile()
{
   m_pFrame->SendMessage(WM_COMMAND,ID_PROJECT_PROFILE,0);
}

/////////////////////////////////////////////////////////////////////////////
// CProfileDisplayObjectEvents message handlers
STDMETHODIMP_(bool) CProfileDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);

   if (pDO->IsSelected())
   {
      pThis->EditProfile();
      return true;
   }
   else
   {
      return false;
   }
}

STDMETHODIMP_(bool) CProfileDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);
   return true;
}

STDMETHODIMP_(bool) CProfileDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CProfileDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CProfileDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CProfileDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CProfileDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CProfileDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CProfileDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);

   if ( nChar == VK_RETURN )
   {
      pThis->EditProfile();
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CProfileDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);
   if ( pDO->IsSelected() )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
      const std::map<IDType,IAlignmentProfileViewEventCallback*>& callbacks = pDoc->GetAlignmentProfileViewCallbacks();

      // the alignment doesn't have its own context menu, so if there aren't callbacks to add anything
      // then just return
      if ( callbacks.size() == 0 )
      {
         return false;
      }

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
      std::map<IDType,IAlignmentProfileViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
      std::map<IDType,IAlignmentProfileViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
      for ( ; callbackIter != callbackIterEnd; callbackIter++ )
      {
         IAlignmentProfileViewEventCallback* pCallback = callbackIter->second;
         pCallback->OnBridgeContextMenu(pMenu);
      }

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

STDMETHODIMP_(void) CProfileDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CProfileDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CProfileDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CProfileDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CProfileDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);
   pThis->m_pFrame->ClearSelection();
   pThis->m_pFrame->SelectAlignment();
}

STDMETHODIMP_(void) CProfileDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CProfileDisplayObjectEvents,Events);
}
