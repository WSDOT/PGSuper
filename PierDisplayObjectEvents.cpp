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

// PierDisplayObjectEvents.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PierDisplayObjectEvents.h"
#include "mfcdual.h"
#include "pgsuperdoc.h"
#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPierDisplayObjectEvents

CPierDisplayObjectEvents::CPierDisplayObjectEvents(PierIndexType pierIdx,const CBridgeDescription2* pBridgeDesc,CBridgeModelViewChildFrame* pFrame)
{
   m_PierIdx     = pierIdx;
   m_pBridgeDesc = pBridgeDesc;
   m_pFrame      = pFrame;
}

BEGIN_INTERFACE_MAP(CPierDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CPierDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CPierDisplayObjectEvents,Events);

void CPierDisplayObjectEvents::EditPier(iDisplayObject* pDO)
{
   CComPtr<iDisplayList> pList;
   pDO->GetDisplayList(&pList);

   CComPtr<iDisplayMgr> pDispMgr;
   pList->GetDisplayMgr(&pDispMgr);

   CDisplayView* pView = pDispMgr->GetView();
   CDocument* pDoc = pView->GetDocument();

   ((CPGSDocBase*)pDoc)->EditPierDescription(m_PierIdx,EPD_GENERAL);
}

void CPierDisplayObjectEvents::SelectPier(iDisplayObject* pDO)
{
   // do the selection at the frame level because it will do this view
   // and the other view
   m_pFrame->SelectPier(m_PierIdx);
}

void CPierDisplayObjectEvents::SelectPrev(iDisplayObject* pDO)
{
   if ( m_PierIdx == 0 )
   {
      // this is the first pier
      if ( m_pBridgeDesc->GetDeckDescription()->DeckType == pgsTypes::sdtNone )
         m_pFrame->SelectPier(m_pBridgeDesc->GetPierCount()-1); // no deck, select last pier
      else
         m_pFrame->SelectDeck();  // select deck if there is one
   }
   else
   {
      m_pFrame->SelectSpan(m_PierIdx-1);
   }
}

void CPierDisplayObjectEvents::SelectNext(iDisplayObject* pDO)
{
   if ( m_PierIdx == m_pBridgeDesc->GetPierCount()-1 )
   {
      // this is the last pier
      if ( m_pBridgeDesc->GetDeckDescription()->DeckType == pgsTypes::sdtNone )
         m_pFrame->SelectPier(0); // no deck, select first pier
      else
         m_pFrame->SelectDeck(); // select deck if there is one 
   }
   else
   {
      m_pFrame->SelectSpan(m_PierIdx);
   }
}

/////////////////////////////////////////////////////////////////////////////
// CPierDisplayObjectEvents message handlers
STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   pThis->EditPier(pDO);
   return true;
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);

   if ( nChar == VK_RETURN )
   {
      pThis->EditPier(pDO);
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
   else if ( nChar == VK_UP || nChar == VK_DOWN )
   {
      const CPierData2* pPier = pThis->m_pBridgeDesc->GetPier(pThis->m_PierIdx);
      const CSpanData2* pPrevSpan = pPier->GetPrevSpan();
      const CSpanData2* pNextSpan = pPier->GetNextSpan();
      const CGirderGroupData* pPrevGroup = pThis->m_pBridgeDesc->GetGirderGroup(pPrevSpan);
      const CGirderGroupData* pNextGroup = pThis->m_pBridgeDesc->GetGirderGroup(pNextSpan);
      const CGirderGroupData* pGroup = (pNextGroup ? pNextGroup : pPrevGroup);

      CGirderKey girderKey(pGroup->GetIndex(),0);
      pThis->m_pFrame->SelectGirder( girderKey );
      return true;
   }
   else if ( nChar == VK_DELETE )
   {
      pThis->m_pFrame->SendMessage(WM_COMMAND,ID_DELETE,0);
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE_(CPierDisplayObjectEvents,Events);
   AFX_MANAGE_STATE(AfxGetStaticModuleState());


   if ( pDO->IsSelected() )
   {
      CComPtr<iDisplayList> pList;
      pDO->GetDisplayList(&pList);

      CComPtr<iDisplayMgr> pDispMgr;
      pList->GetDisplayMgr(&pDispMgr);

      CDisplayView* pView = pDispMgr->GetView();
      CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
      pMenu->LoadMenu(IDR_SELECTED_PIER_CONTEXT,NULL);

      CComPtr<IBroker> pBroker;
      pDoc->GetBroker(&pBroker);
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      const CPierData2* pPier = pBridgeDesc->GetPier(pThis->m_PierIdx);

      if ( pPier->IsBoundaryPier() )
      {
         // get all valid connection types for the pier represented by this display object
         std::vector<pgsTypes::BoundaryConditionType> validConnectionTypes( pBridgeDesc->GetBoundaryConditionTypes(pThis->m_PierIdx) );

         // Mapping between connection type and menu id
         std::map<pgsTypes::BoundaryConditionType,UINT> menuIDs;
         menuIDs.insert(std::make_pair(pgsTypes::bctHinge,IDM_HINGE));
         menuIDs.insert(std::make_pair(pgsTypes::bctRoller,IDM_ROLLER));
         menuIDs.insert(std::make_pair(pgsTypes::bctContinuousAfterDeck,IDM_CONTINUOUS_AFTERDECK));
         menuIDs.insert(std::make_pair(pgsTypes::bctContinuousBeforeDeck,IDM_CONTINUOUS_BEFOREDECK));
         menuIDs.insert(std::make_pair(pgsTypes::bctIntegralAfterDeck,IDM_INTEGRAL_AFTERDECK));
         menuIDs.insert(std::make_pair(pgsTypes::bctIntegralBeforeDeck,IDM_INTEGRAL_BEFOREDECK));
         menuIDs.insert(std::make_pair(pgsTypes::bctIntegralAfterDeckHingeBack,IDM_INTEGRAL_AFTERDECK_HINGEBACK));
         menuIDs.insert(std::make_pair(pgsTypes::bctIntegralBeforeDeckHingeBack,IDM_INTEGRAL_BEFOREDECK_HINGEBACK));
         menuIDs.insert(std::make_pair(pgsTypes::bctIntegralAfterDeckHingeAhead,IDM_INTEGRAL_AFTERDECK_HINGEAHEAD));
         menuIDs.insert(std::make_pair(pgsTypes::bctIntegralBeforeDeckHingeAhead,IDM_INTEGRAL_BEFOREDECK_HINGEAHEAD));

         pMenu->AppendSeparator();

         // Populate the menu
         std::vector<pgsTypes::BoundaryConditionType>::iterator iter(validConnectionTypes.begin());
         std::vector<pgsTypes::BoundaryConditionType>::iterator iterEnd(validConnectionTypes.end());
         for ( ; iter != iterEnd; iter++ )
         {
            UINT nID = menuIDs[*iter]; // look up the ID for each valid connection type
            pMenu->AppendMenu(nID ,CPierData2::AsString(*iter),NULL); // add it to the menu
         }
      }
      else
      {
         // get all valid connection types for the pier represented by this display object
         std::vector<pgsTypes::PierSegmentConnectionType> validConnectionTypes( pBridgeDesc->GetPierSegmentConnectionTypes(pThis->m_PierIdx) );

         // Mapping between connection type and menu id
         std::map<pgsTypes::PierSegmentConnectionType,UINT> menuIDs;
         menuIDs.insert(std::make_pair(pgsTypes::psctContinousClosureJoint,IDM_CONTINUOUS_CLOSURE));
         menuIDs.insert(std::make_pair(pgsTypes::psctIntegralClosureJoint,IDM_INTEGRAL_CLOSURE));
         menuIDs.insert(std::make_pair(pgsTypes::psctContinuousSegment,IDM_CONTINUOUS_SEGMENT_AT_PIER));
         menuIDs.insert(std::make_pair(pgsTypes::psctIntegralSegment,IDM_INTEGRAL_SEGMENT_AT_PIER));

         pMenu->AppendSeparator();

         // Populate the menu
         std::vector<pgsTypes::PierSegmentConnectionType>::iterator iter(validConnectionTypes.begin());
         std::vector<pgsTypes::PierSegmentConnectionType>::iterator iterEnd(validConnectionTypes.end());
         for ( ; iter != iterEnd; iter++ )
         {
            UINT nID = menuIDs[*iter]; // look up the ID for each valid connection type
            pMenu->AppendMenu(nID ,CPierData2::AsString(*iter),NULL); // add it to the menu
         }
      }

      const std::map<IDType,IBridgePlanViewEventCallback*>& callbacks = pDoc->GetBridgePlanViewCallbacks();
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
      for ( ; callbackIter != callbackIterEnd; callbackIter++ )
      {
         IBridgePlanViewEventCallback* pCallback = callbackIter->second;
         pCallback->OnPierContextMenu(pThis->m_PierIdx,pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,pThis->m_pFrame);

      delete pMenu;

      return true;
   }

   return false;
}

STDMETHODIMP_(void) CPierDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CPierDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CPierDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CPierDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CPierDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   pThis->SelectPier(pDO);
}

STDMETHODIMP_(void) CPierDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
}

