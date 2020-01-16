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

// BridgeDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "BridgeDisplayObjectEvents.h"
#include "BridgeModelViewChildFrame.h"
#include "PGSuperDocBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeDisplayObjectEvents

CBridgeDisplayObjectEvents::CBridgeDisplayObjectEvents(IBroker* pBroker, CBridgeModelViewChildFrame* pFrame,iDisplayObject* pDO,CBridgeDisplayObjectEvents::ViewType viewType)
{
   m_pBroker = pBroker;
   m_pFrame = pFrame;
   m_ViewType = viewType;
   m_DispObj.Attach(pDO);
}

CBridgeDisplayObjectEvents::~CBridgeDisplayObjectEvents()
{
}

void CBridgeDisplayObjectEvents::OnFinalRelease()
{
   m_DispObj.Detach();
   CCmdTarget::OnFinalRelease();
}

BEGIN_INTERFACE_MAP(CBridgeDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CBridgeDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

// Not sure why, but the DELEGATE_CUSTOM_INTERFACE macro isn't working
// Since the is simple, I've just implemented it directly
//DELEGATE_CUSTOM_INTERFACE(CBridgeDisplayObjectEvents,Events);
STDMETHODIMP_(ULONG) CBridgeDisplayObjectEvents::XEvents::AddRef() 
{ 
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents, Events) 
   return pThis->ExternalAddRef(); 
} 
STDMETHODIMP_(ULONG) CBridgeDisplayObjectEvents::XEvents::Release() 
{ 
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents, Events) 
   return pThis->ExternalRelease(); 
} 
STDMETHODIMP CBridgeDisplayObjectEvents::XEvents::QueryInterface( 
   REFIID iid, LPVOID* ppvObj) 
{ 
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents, Events) 
   return pThis->ExternalQueryInterface( &iid, ppvObj ); 
}

void CBridgeDisplayObjectEvents::EditBridge()
{
   m_pFrame->PostMessage(WM_COMMAND,ID_PROJECT_BRIDGEDESC,0);
}

/////////////////////////////////////////////////////////////////////////////
// CAlignmentDisplayObjectEvents message handlers
STDMETHODIMP_(bool) CBridgeDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);

   if (pDO->IsSelected())
   {
      pThis->EditBridge();
      return true;
   }
   else
   {
      return false;
   }
}

STDMETHODIMP_(bool) CBridgeDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);
   return true;
}

STDMETHODIMP_(bool) CBridgeDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CBridgeDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);

   if ( nChar == VK_RETURN )
   {
      pThis->EditBridge();
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CBridgeDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);
   if ( pDO->IsSelected() )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
      CEAFMenu* pMenu;
      if ( pThis->m_ViewType == CBridgeDisplayObjectEvents::Plan )
      {
         const std::map<IDType,IAlignmentPlanViewEventCallback*>& callbacks = pDoc->GetAlignmentPlanViewCallbacks();

         // the alignment doesn't have its own context menu, so if there aren't callbacks to add anything
         // then just return
         if ( callbacks.size() == 0 )
         {
            return false;
         }

         pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
         std::map<IDType,IAlignmentPlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
         std::map<IDType,IAlignmentPlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
         for ( ; callbackIter != callbackIterEnd; callbackIter++ )
         {
            IAlignmentPlanViewEventCallback* pCallback = callbackIter->second;
            pCallback->OnBridgeContextMenu(pMenu);
         }
      }
      else
      {
         const std::map<IDType,IAlignmentProfileViewEventCallback*>& callbacks = pDoc->GetAlignmentProfileViewCallbacks();

         // the alignment doesn't have its own context menu, so if there aren't callbacks to add anything
         // then just return
         if ( callbacks.size() == 0 )
         {
            return false;
         }

         pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
         std::map<IDType,IAlignmentProfileViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
         std::map<IDType,IAlignmentProfileViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
         for ( ; callbackIter != callbackIterEnd; callbackIter++ )
         {
            IAlignmentProfileViewEventCallback* pCallback = callbackIter->second;
            pCallback->OnBridgeContextMenu(pMenu);
         }
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

STDMETHODIMP_(void) CBridgeDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CBridgeDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);
   pThis->m_pFrame->ClearSelection();
   pThis->m_pFrame->SelectAlignment();
}

STDMETHODIMP_(void) CBridgeDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeDisplayObjectEvents,Events);
}
