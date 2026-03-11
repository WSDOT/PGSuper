///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

// GirderDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"
#include "BearingDisplayObjectEvents.h"
#include "mfcdual.h"
#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"
#include <IFace\Bridge.h>
#include <IFace\EditByUI.h>
#include <EAF/EAFReportManager.h>

#include "BridgePlanView.h"

#include <DManip/DisplayObjectEvents.h>
#include <DManip/DisplayList.h>
#include <DManip/DisplayMgr.h>
#include <DManip/DisplayView.h>

#include <PsgLib\BridgeDescription2.h>

using namespace WBFL::DManip;


/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewBearingDisplayObjectEvents

CBridgePlanViewBearingDisplayObjectEvents::CBridgePlanViewBearingDisplayObjectEvents(
    const ReactionLocation& reactionLocation,
	PierIndexType nPiers,
    GroupIndexType nGroups, 
    GirderIndexType nGirderThisGroup,
    CBridgeModelViewChildFrame* pFrame)
{
   m_ReactionLocation  = reactionLocation;
   m_nPiers            = nPiers;
   m_nGroups           = nGroups;
   m_nGirdersThisGroup = nGirderThisGroup;
   m_pFrame            = pFrame;
}

void CBridgePlanViewBearingDisplayObjectEvents::EditBearing(std::shared_ptr<iDisplayObject> pDO)
{
   m_pFrame->PostMessage(WM_COMMAND,ID_EDIT_BEARING,0);
}

void CBridgePlanViewBearingDisplayObjectEvents::SelectBearing(std::shared_ptr<iDisplayObject> pDO)
{
   m_pFrame->SelectBearing(m_ReactionLocation);
}

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanViewSegmentDisplayObjectEvents message handlers
bool CBridgePlanViewBearingDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   auto pParent = pDO->GetParent();
   if (pParent && pParent->IsSelected())
   {
      // we are part of a composite display object and the parent is selected
      // defer dbl-click handling to the parent
      return false;
   }

   if (pDO->IsSelected())
   {
      EditBearing(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

bool CBridgePlanViewBearingDisplayObjectEvents::OnLButtonDown(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CBridgePlanViewBearingDisplayObjectEvents::OnLButtonUp(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgePlanViewBearingDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgePlanViewBearingDisplayObjectEvents::OnRButtonDown(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CBridgePlanViewBearingDisplayObjectEvents::OnRButtonUp(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgePlanViewBearingDisplayObjectEvents::OnMouseMove(std::shared_ptr<iDisplayObject> pDO,UINT nFlags, const POINT& point)
{
   return false;
}

bool CBridgePlanViewBearingDisplayObjectEvents::OnMouseWheel(std::shared_ptr<iDisplayObject> pDO,UINT nFlags,short zDelta, const POINT& point)
{
   return false;
}

bool CBridgePlanViewBearingDisplayObjectEvents::OnKeyDown(std::shared_ptr<iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_RETURN )
   {
      EditBearing(pDO);
      return true;
   }
   else if (nChar == VK_LEFT)
   {
       auto pBroker = EAFGetBroker();
       GET_IFACE2(pBroker, IBridgeDescription, pBridgeDesc);
       const auto& bearingType = pBridgeDesc->GetBearingType();

       if (bearingType == pgsTypes::brtBridge)
       {
           m_pFrame->SelectDeck();
       }
       else if (bearingType == pgsTypes::brtGirder)
       {
           m_pFrame->SelectGirder(m_ReactionLocation.GirderKey);
       }
       else if (bearingType == pgsTypes::brtPier && m_ReactionLocation.Face == rftAhead)
       {
           m_pFrame->SelectPier(m_ReactionLocation.PierIdx);
       }
       else if (bearingType == pgsTypes::brtPier && (m_ReactionLocation.Face == rftBack || m_ReactionLocation.Face == rftMid))
       {
           m_pFrame->SelectSpan(m_ReactionLocation.PierIdx - 1);
       }
       else
       {
           m_pFrame->SelectGirder(m_ReactionLocation.GirderKey);
       }
       return true;
   }
   else if (nChar == VK_RIGHT)
   {
       auto pBroker = EAFGetBroker();
       GET_IFACE2(pBroker, IBridgeDescription, pBridgeDesc);
       const auto& bearingType = pBridgeDesc->GetBearingType();

       if (bearingType == pgsTypes::brtBridge)
       {
           m_pFrame->SelectAlignment();
           return true;
       }
       else if (bearingType == pgsTypes::brtGirder)
       {
           m_pFrame->SelectGirder(m_ReactionLocation.GirderKey);
           return true;
       }

       ReactionLocation rightLocation = m_ReactionLocation;

       bool bInterior = false;
       if (m_ReactionLocation.PierIdx + 1 < m_nPiers)
       {
           auto pBroker = EAFGetBroker();
           GET_IFACE2(pBroker, IBridge, pBridge);
           bInterior = pBridge->IsInteriorPier(m_ReactionLocation.PierIdx);
       }

       if (m_ReactionLocation.Face == rftAhead)
       {
           if (bInterior)
           {
               rightLocation.Face = rftMid;
               rightLocation.PierIdx++;
           }
           else
           {
               rightLocation.Face = rftBack;
               rightLocation.PierIdx++;
           }
       }
       else if (m_ReactionLocation.Face == rftMid)
       {
           if (bInterior)
           {
               m_pFrame->SelectPier(rightLocation.PierIdx);
               return true;
           }
           else
           {
               rightLocation.Face = rftBack;
               rightLocation.PierIdx++;
           }
       }
       else
       {
           if (m_ReactionLocation.PierIdx == m_nPiers - 1 && bearingType != pgsTypes::brtPier)
           {
               if (m_ReactionLocation.GirderKey.girderIndex == m_nGirdersThisGroup - 1)
               {
                   m_pFrame->SelectAlignment();
                   return true;
               }
               else
               {
                   if (bearingType == pgsTypes::brtGirder)
                   {
                       rightLocation.Face = rftBack;
                       rightLocation.PierIdx = 0;
                       rightLocation.GirderKey.groupIndex = 0;
                       rightLocation.GirderKey.girderIndex++;
                   }
                   else
                   {
                       m_pFrame->SelectAlignment();
                       return true;
                   }
               }
           }
           else
           {
               rightLocation.Face = rftAhead;
			   m_pFrame->SelectPier(rightLocation.PierIdx);
               return true;
           }
       }

       m_pFrame->SelectSpan(rightLocation.PierIdx - 1);
       return true;
   }
   else if ( nChar == VK_UP  || nChar == VK_DOWN)
   {
       auto pBroker = EAFGetBroker();
       GET_IFACE2(pBroker, IBridgeDescription, pBridgeDesc);
       const auto& bearingType = pBridgeDesc->GetBearingType();

       if (bearingType == pgsTypes::brtBridge)
       {
           m_pFrame->SelectGirder(CGirderKey(0,0));
       }
       else
       {
           m_pFrame->SelectGirder(m_ReactionLocation.GirderKey);
       }
      return true;
   }

   return false;
}

bool CBridgePlanViewBearingDisplayObjectEvents::OnContextMenu(std::shared_ptr<iDisplayObject> pDO, CWnd* pWnd, const POINT& point)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (pDO->IsSelected())
    {
        auto pView = pDO->GetDisplayList()->GetDisplayMgr()->GetView();
        CPGSuperDoc* pDoc = (CPGSuperDoc*)pView->GetDocument();


        auto pMenu = WBFL::EAF::Menu::CreateContextMenu(pDoc->GetPluginCommandManager());
        pMenu->LoadMenu(IDR_SELECTED_BEARING_CONTEXT, nullptr);

        /// For future use if needed
        //
        //CPGSDocBase* pPGSDoc = (CPGSDocBase*)pDoc;
        //const std::map<IDType, IBridgePlanViewEventCallback*>& callbacks = pPGSDoc->GetBridgePlanViewCallbacks();
        //std::map<IDType, IBridgePlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
        //std::map<IDType, IBridgePlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
        //for (; callbackIter != callbackIterEnd; callbackIter++)
        //{
        //    IBridgePlanViewEventCallback* pCallback = callbackIter->second;
        //    pCallback->OnGirderBearingMenu(m_ReactionLocation.GirderKey, pMenu);
        //}

        pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, m_pFrame);

        return true;
    }

    return false;
}

void CBridgePlanViewBearingDisplayObjectEvents::OnChanged(std::shared_ptr<iDisplayObject> pDO)
{
}

void CBridgePlanViewBearingDisplayObjectEvents::OnDragMoved(std::shared_ptr<iDisplayObject> pDO,const WBFL::Geometry::Size2d& size)
{
}

void CBridgePlanViewBearingDisplayObjectEvents::OnMoved(std::shared_ptr<iDisplayObject> pDO)
{
}

void CBridgePlanViewBearingDisplayObjectEvents::OnCopied(std::shared_ptr<iDisplayObject> pDO)
{
}

void CBridgePlanViewBearingDisplayObjectEvents::OnSelect(std::shared_ptr<iDisplayObject> pDO)
{
   SelectBearing(pDO);
}

void CBridgePlanViewBearingDisplayObjectEvents::OnUnselect(std::shared_ptr<iDisplayObject> pDO)
{
}

