///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

// AlignmentDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "AlignmentDisplayObjectEvents.h"
#include "BridgeSectionCutDisplayImpl.h"
#include "BridgeModelViewChildFrame.h"
#include "mfcdual.h"
#include "PGSuperDocBase.h"
#include <IFace\Bridge.h>
#include <IFace\EditByUI.h>
#include <WBFLDManip.h>

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAlignmentDisplayObjectEvents

CAlignmentDisplayObjectEvents::CAlignmentDisplayObjectEvents(IBroker* pBroker, CBridgeModelViewChildFrame* pFrame,ViewType viewType,iDisplayObject* pDO)
{
   m_ViewType = viewType;
   m_pBroker = pBroker;
   m_pFrame = pFrame;
   
   if ( pDO )
   {
      m_DispObj.Attach(pDO);
   }
}

CAlignmentDisplayObjectEvents::~CAlignmentDisplayObjectEvents()
{
}

void CAlignmentDisplayObjectEvents::OnFinalRelease()
{
   m_DispObj.Detach();
   CCmdTarget::OnFinalRelease();
}

BEGIN_INTERFACE_MAP(CAlignmentDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CAlignmentDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
   INTERFACE_PART(CAlignmentDisplayObjectEvents,IID_iDropSite,DropSite)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CAlignmentDisplayObjectEvents,Events);
DELEGATE_CUSTOM_INTERFACE(CAlignmentDisplayObjectEvents,DropSite);

void CAlignmentDisplayObjectEvents::EditAlignment()
{
   GET_IFACE(IEditByUI, pEditByUI);
   int page = (m_ViewType == BridgePlan || m_ViewType == Alignment ? EAD_ROADWAY : EAD_SECTION);
   pEditByUI->EditAlignmentDescription(page);
}

/////////////////////////////////////////////////////////////////////////////
// CAlignmentDisplayObjectEvents message handlers
STDMETHODIMP_(bool) CAlignmentDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);

   if (pDO->IsSelected())
   {
      pThis->EditAlignment();
      return true;
   }
   else
   {
      return false;
   }
}

STDMETHODIMP_(bool) CAlignmentDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);
   return true;
}

STDMETHODIMP_(bool) CAlignmentDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CAlignmentDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CAlignmentDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CAlignmentDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CAlignmentDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CAlignmentDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CAlignmentDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);

   if ( nChar == VK_RETURN )
   {
      pThis->EditAlignment();
      return true;
   }

   if (pThis->m_ViewType == BridgePlan || pThis->m_ViewType == BridgeSection)
   {
      if ( nChar == VK_DOWN )
      {
         pThis->m_pFrame->SelectGirder(CSegmentKey(0,0,0));
         return true;
      }
      else if ( nChar == VK_UP )
      {
         GET_IFACE2(pThis->m_pBroker,IBridgeDescription,pIBridgeDesc);
         const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
         GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
         GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(nGroups-1)->GetGirderCount();
         SegmentIndexType nSegments = pBridgeDesc->GetGirderGroup(nGroups-1)->GetGirder(nGirders-1)->GetSegmentCount();
         pThis->m_pFrame->SelectGirder(CSegmentKey(nGroups-1,nGirders-1,nSegments-1));

         return true;
      }
      else if ( nChar == VK_LEFT )
      {
         GET_IFACE2(pThis->m_pBroker,IBridgeDescription,pIBridgeDesc);
         const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
         pThis->m_pFrame->SelectPier(pBridgeDesc->GetPierCount()-1);
         return true;
      }
      else if ( nChar == VK_RIGHT )
      {
         GET_IFACE2(pThis->m_pBroker,IBridgeDescription,pIBridgeDesc);
         const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
         pThis->m_pFrame->SelectPier(0);
         return true;
      }
   }


   return false;
}

STDMETHODIMP_(bool) CAlignmentDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);
   if ( pDO->IsSelected() )
   {
      CComPtr<iDisplayList> pList;
      pDO->GetDisplayList(&pList);

      CComPtr<iDisplayMgr> pDispMgr;
      pList->GetDisplayMgr(&pDispMgr);

      CDisplayView* pView = pDispMgr->GetView();
      CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();

      CEAFMenu* pMenu;

      if ( pThis->m_ViewType == BridgePlan || pThis->m_ViewType == BridgeSection )
      {
         const std::map<IDType,IBridgePlanViewEventCallback*>& callbacks = pDoc->GetBridgePlanViewCallbacks();

         // the alignment doesn't have its own context menu, so if there aren't callbacks to add anything
         // then just return
         if ( callbacks.size() == 0 )
         {
            return false;
         }

         pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
         std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
         std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
         for ( ; callbackIter != callbackIterEnd; callbackIter++ )
         {
            IBridgePlanViewEventCallback* pCallback = callbackIter->second;
            pCallback->OnAlignmentContextMenu(pMenu);
         }
      }
      else
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
            pCallback->OnAlignmentContextMenu(pMenu);
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

STDMETHODIMP_(void) CAlignmentDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CAlignmentDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CAlignmentDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CAlignmentDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CAlignmentDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);
   pThis->m_pFrame->SelectAlignment();
}

STDMETHODIMP_(void) CAlignmentDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,Events);
}


STDMETHODIMP_(DROPEFFECT) CAlignmentDisplayObjectEvents::XDropSite::CanDrop(COleDataObject* pDataObject,DWORD dwKeyState,IPoint2d* point)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,DropSite);

   if ( pDataObject->IsDataAvailable(CBridgeSectionCutDisplayImpl::ms_Format) )
   {
      // need to peek at our object first and make sure it's coming from the local process
      // this is ugly because it breaks encapsulation of CBridgeSectionCutDisplayImpl
      CComPtr<iDragDataSource> source;               
      ::CoCreateInstance(CLSID_DragDataSource,nullptr,CLSCTX_ALL,IID_iDragDataSource,(void**)&source);
      source->SetDataObject(pDataObject);
      source->PrepareFormat(CBridgeSectionCutDisplayImpl::ms_Format);

      CWinThread* thread = ::AfxGetThread( );
      DWORD threadid = thread->m_nThreadID;

      DWORD threadl;
      // know (by voodoo) that the first member of this data source is the thread id
      source->Read(CBridgeSectionCutDisplayImpl::ms_Format,&threadl,sizeof(DWORD));

      if (threadl == threadid)
      {
        return DROPEFFECT_MOVE;
      }
   }

   return DROPEFFECT_NONE;
}

STDMETHODIMP_(void) CAlignmentDisplayObjectEvents::XDropSite::OnDropped(COleDataObject* pDataObject,DROPEFFECT dropEffect,IPoint2d* point)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,DropSite);

   // Something was dropped on a display object that represents a Alignment
   AfxMessageBox(_T("Alignment drop"));
}

STDMETHODIMP_(void) CAlignmentDisplayObjectEvents::XDropSite::SetDisplayObject(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,DropSite);
   pThis->m_DispObj.Detach();
   pThis->m_DispObj.Attach(pDO);
}

STDMETHODIMP_(void) CAlignmentDisplayObjectEvents::XDropSite::GetDisplayObject(iDisplayObject** dispObj)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,DropSite);
   *dispObj = pThis->m_DispObj;
   (*dispObj)->AddRef();
}

STDMETHODIMP_(void) CAlignmentDisplayObjectEvents::XDropSite::Highlite(CDC* pDC,BOOL bHighlite)
{
   METHOD_PROLOGUE(CAlignmentDisplayObjectEvents,DropSite);
   pThis->m_DispObj->Highlite(pDC,bHighlite);
}


