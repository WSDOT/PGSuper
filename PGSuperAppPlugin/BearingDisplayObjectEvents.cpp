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

CBridgePlanViewBearingDisplayObjectEvents::CBridgePlanViewBearingDisplayObjectEvents(const ReactionLocation& reactionLocation,CBridgeModelViewChildFrame* pFrame)
{
   m_ReactionLocation = reactionLocation;
   m_pFrame     = pFrame;

   
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(m_ReactionLocation.GirderKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(m_ReactionLocation.GirderKey.girderIndex);
   m_nGirders = pGroup->GetGirderCount();
}

void CBridgePlanViewBearingDisplayObjectEvents::EditBearing(std::shared_ptr<iDisplayObject> pDO)
{
   m_pFrame->PostMessage(WM_COMMAND,ID_EDIT_BEARING,0);
}

void CBridgePlanViewBearingDisplayObjectEvents::SelectBearing(std::shared_ptr<iDisplayObject> pDO)
{
   m_pFrame->SelectBearing(m_ReactionLocation);
}

void CBridgePlanViewBearingDisplayObjectEvents::SelectPrevBearing()
{
   if ( m_ReactionLocation.GirderKey.groupIndex == 0 && m_ReactionLocation.GirderKey.girderIndex == 0 && m_ReactionLocation.Face == 0 )
   {
      // this is the first segment in the first girder in the first group... select the alignment
      m_pFrame->GetBridgePlanView()->SelectAlignment(true);
   }
   else
   {
      if ( m_ReactionLocation.Face == 0 )
      {
         // select previous spliced girder
         CGirderKey girderKey(m_ReactionLocation.GirderKey.groupIndex, m_ReactionLocation.GirderKey.girderIndex-1);
         m_pFrame->SelectGirder(girderKey);
      }
      else
      {
         // select closure joint at left end of this segment
         CSegmentKey closureKey(m_ReactionLocation.GirderKey.groupIndex, m_ReactionLocation.GirderKey.girderIndex,m_ReactionLocation.Face-1);
         m_pFrame->SelectClosureJoint(closureKey);
      }
   }
}

void CBridgePlanViewBearingDisplayObjectEvents::SelectNextBearing()
{
   if ( m_ReactionLocation.Face == m_nReactionLocations-1 )
   {
      // this is the last segment, select the entire girder line
      CGirderKey girderKey(m_ReactionLocation.GirderKey.groupIndex, m_ReactionLocation.GirderKey.girderIndex);
      m_pFrame->SelectGirder(girderKey);
   }
   else
   {
      // select closure joint at the right end of this segment
      CSegmentKey closureKey(m_ReactionLocation.GirderKey.groupIndex, m_ReactionLocation.GirderKey.girderIndex,m_ReactionLocation.Face);
      m_pFrame->SelectClosureJoint(closureKey);
   }
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
   else if ( nChar == VK_LEFT )
   {
      SelectPrevBearing();
      return true;
   }
   else if ( nChar == VK_RIGHT )
   {
      SelectNextBearing();
      return true;
   }

   return false;
}

bool CBridgePlanViewBearingDisplayObjectEvents::OnContextMenu(std::shared_ptr<iDisplayObject> pDO, CWnd* pWnd, const POINT& point)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (pDO->IsSelected())
    {
        auto pList = pDO->GetDisplayList();
        auto pDispMgr = pList->GetDisplayMgr();
        auto pView = pDispMgr->GetView();
        CDocument* pDoc = pView->GetDocument();

        CPGSDocBase* pPGSDoc = (CPGSDocBase*)pDoc;

        auto pMenu = WBFL::EAF::Menu::CreateContextMenu(pPGSDoc->GetPluginCommandManager());
        pMenu->LoadMenu(IDR_SELECTED_GIRDER_CONTEXT, nullptr);

        if (pPGSDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)))
        {
            // PGSplice doesn't do design
            CString strDesignGirder;
            pMenu->GetMenuString(ID_PROJECT_DESIGNGIRDERDIRECT, strDesignGirder, MF_BYCOMMAND);
            UINT nPos = pMenu->FindMenuItem(strDesignGirder);
            pMenu->RemoveMenu(nPos - 1, MF_BYPOSITION, nullptr); // remove the separater before "Design Girder"
            pMenu->RemoveMenu(ID_PROJECT_DESIGNGIRDERDIRECT, MF_BYCOMMAND, nullptr);
            pMenu->RemoveMenu(ID_PROJECT_DESIGNGIRDERDIRECT_PRESERVEHAUNCH, MF_BYCOMMAND, nullptr);
        }

        pPGSDoc->BuildReportMenu(pMenu, true);

        const std::map<IDType, IBridgePlanViewEventCallback*>& callbacks = pPGSDoc->GetBridgePlanViewCallbacks();
        std::map<IDType, IBridgePlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
        std::map<IDType, IBridgePlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
        for (; callbackIter != callbackIterEnd; callbackIter++)
        {
            IBridgePlanViewEventCallback* pCallback = callbackIter->second;
            pCallback->OnGirderContextMenu(m_ReactionLocation.GirderKey, pMenu);
        }

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

