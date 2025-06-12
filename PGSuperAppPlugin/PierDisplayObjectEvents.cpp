///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"
#include "PierDisplayObjectEvents.h"
#include "pgsuperdoc.h"

#include <IFace/Tools.h>
#include <IFace\Project.h>
#include <IFace\EditByUI.h>

#include <PsgLib\BridgeDescription2.h>

#include <DManip/DisplayObject.h>
#include <DManip/DisplayList.h>
#include <DManip/DisplayMgr.h>
#include <DManip/DisplayView.h>

/////////////////////////////////////////////////////////////////////////////
// CPierDisplayObjectEvents

CPierDisplayObjectEvents::CPierDisplayObjectEvents(PierIndexType pierIdx,const CBridgeDescription2* pBridgeDesc,CBridgeModelViewChildFrame* pFrame)
{
   m_PierIdx     = pierIdx;
   m_pBridgeDesc = pBridgeDesc;
   m_pFrame      = pFrame;
}

void CPierDisplayObjectEvents::EditPier(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   auto pList = pDO->GetDisplayList();
   auto pDispMgr = pList->GetDisplayMgr();
   auto pView = pDispMgr->GetView();
   CDocument* pDoc = pView->GetDocument();

   ((CPGSDocBase*)pDoc)->EditPierDescription(m_PierIdx,EPD_GENERAL);
}

void CPierDisplayObjectEvents::SelectPier(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   // do the selection at the frame level because it will do this view
   // and the other view
   m_pFrame->SelectPier(m_PierIdx);
}

void CPierDisplayObjectEvents::SelectPrev(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   if ( m_PierIdx == 0 )
   {
      // this is the first pier
      if (m_pBridgeDesc->GetDeckDescription()->GetDeckType() == pgsTypes::sdtNone)
      {
         m_pFrame->SelectPier(m_pBridgeDesc->GetPierCount() - 1); // no deck, select last pier
      }
      else
      {
         m_pFrame->SelectDeck();  // select deck if there is one
      }
   }
   else
   {
      m_pFrame->SelectSpan(m_PierIdx-1);
   }
}

void CPierDisplayObjectEvents::SelectNext(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   if ( m_PierIdx == m_pBridgeDesc->GetPierCount()-1 )
   {
      // this is the last pier
      if (m_pBridgeDesc->GetDeckDescription()->GetDeckType() == pgsTypes::sdtNone)
      {
         m_pFrame->SelectPier(0); // no deck, select first pier
      }
      else
      {
         m_pFrame->SelectDeck(); // select deck if there is one 
      }
   }
   else
   {
      m_pFrame->SelectSpan(m_PierIdx);
   }
}

/////////////////////////////////////////////////////////////////////////////
// CPierDisplayObjectEvents message handlers
bool CPierDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   if (pDO->IsSelected())
   {
      EditPier(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

bool CPierDisplayObjectEvents::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CPierDisplayObjectEvents::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{   
   return false;
}

bool CPierDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CPierDisplayObjectEvents::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CPierDisplayObjectEvents::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CPierDisplayObjectEvents::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CPierDisplayObjectEvents::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CPierDisplayObjectEvents::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_RETURN )
   {
      EditPier(pDO);
      return true;
   }
   else if ( nChar == VK_LEFT )
   {
      SelectPrev(pDO);
      return true;
   }
   else if ( nChar == VK_RIGHT )
   {
      SelectNext(pDO);
      return true;
   }
   else if ( nChar == VK_UP || nChar == VK_DOWN )
   {
      const CPierData2* pPier = m_pBridgeDesc->GetPier(m_PierIdx);
      const CSpanData2* pPrevSpan = pPier->GetPrevSpan();
      const CSpanData2* pNextSpan = pPier->GetNextSpan();
      const CGirderGroupData* pPrevGroup = m_pBridgeDesc->GetGirderGroup(pPrevSpan);
      const CGirderGroupData* pNextGroup = m_pBridgeDesc->GetGirderGroup(pNextSpan);
      const CGirderGroupData* pGroup = (pNextGroup ? pNextGroup : pPrevGroup);

      CGirderKey girderKey(pGroup->GetIndex(),0);
      m_pFrame->SelectGirder( girderKey );
      return true;
   }
   else if ( nChar == VK_DELETE )
   {
      m_pFrame->PostMessage(WM_COMMAND,ID_DELETE,0);
      return true;
   }

   return false;
}

bool CPierDisplayObjectEvents::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( pDO->IsSelected() )
   {
      auto pList = pDO->GetDisplayList();
      auto pDispMgr = pList->GetDisplayMgr();
      auto pView = pDispMgr->GetView();
      CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();

      auto pMenu = WBFL::EAF::Menu::CreateContextMenu(pDoc->GetPluginCommandManager());
      pMenu->LoadMenu(IDR_SELECTED_PIER_CONTEXT,nullptr);

      
      auto pBroker = pDoc->GetBroker();
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      const CPierData2* pPier = pBridgeDesc->GetPier(m_PierIdx);

      bool bNoDeck = IsNonstructuralDeck(pBridgeDesc->GetDeckDescription()->GetDeckType());

      if ( pPier->IsBoundaryPier() )
      {
         // get all valid connection types for the pier represented by this display object
         std::vector<pgsTypes::BoundaryConditionType> validConnectionTypes( pBridgeDesc->GetBoundaryConditionTypes(m_PierIdx) );

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
            pMenu->AppendMenu(nID ,CPierData2::AsString(*iter,bNoDeck),nullptr); // add it to the menu
         }
      }
      else
      {
         // get all valid connection types for the pier represented by this display object
         std::vector<pgsTypes::PierSegmentConnectionType> validConnectionTypes( pBridgeDesc->GetPierSegmentConnectionTypes(m_PierIdx) );

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
            pMenu->AppendMenu(nID ,CPierData2::AsString(*iter),nullptr); // add it to the menu
         }
      }

      const std::map<IDType,IBridgePlanViewEventCallback*>& callbacks = pDoc->GetBridgePlanViewCallbacks();
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
      for ( ; callbackIter != callbackIterEnd; callbackIter++ )
      {
         IBridgePlanViewEventCallback* pCallback = callbackIter->second;
         pCallback->OnPierContextMenu(m_PierIdx,pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,m_pFrame);

      return true;
   }

   return false;
}

void CPierDisplayObjectEvents::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{

}

void CPierDisplayObjectEvents::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{

}

void CPierDisplayObjectEvents::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{

}

void CPierDisplayObjectEvents::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{

}

void CPierDisplayObjectEvents::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{

   SelectPier(pDO);
}

void CPierDisplayObjectEvents::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{

}

