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

void CBridgePlanViewBearingDisplayObjectEvents::SelectBearingAbove()
{
    ReactionLocation aboveLocation = m_ReactionLocation;

    if (m_ReactionLocation.GirderKey.girderIndex == 0)
    {
        if (m_ReactionLocation.GirderKey.groupIndex == 0)
        {
            if (m_ReactionLocation.Face == rftAhead)
            {
                aboveLocation.GirderKey = CGirderKey(m_nGroups - 1, m_nGirdersThisGroup - 1);
                aboveLocation.PierIdx = m_nPiers - 1;
                aboveLocation.Face = rftBack;
            }
            else
            {
                aboveLocation.PierIdx--;
				aboveLocation.GirderKey.girderIndex = m_nGirdersThisGroup - 1;
                auto pBroker = EAFGetBroker();
                GET_IFACE2(pBroker, IBridge, pBridge);
                bool bInterior = pBridge->IsInteriorPier(m_ReactionLocation.PierIdx - 1);
				if (bInterior)
                {
                    aboveLocation.Face = rftMid;
                }
                else
                {
                    aboveLocation.Face = rftAhead;
                }
            }
        }
        else
        {
            if (aboveLocation.Face == rftAhead)
            {
                aboveLocation.GirderKey = CGirderKey(aboveLocation.GirderKey.groupIndex - 1, m_nGirdersThisGroup - 1);
                aboveLocation.Face = rftBack;
            }
            else
            {
                aboveLocation.GirderKey.girderIndex = m_nGirdersThisGroup - 1;
				aboveLocation.PierIdx--;
                aboveLocation.Face = rftAhead;
            }
        }
    }
    else
    {
        aboveLocation.GirderKey.girderIndex--;
    }

    m_pFrame->SelectBearing(aboveLocation);
}

void CBridgePlanViewBearingDisplayObjectEvents::SelectBearingBelow()
{
    ReactionLocation belowLocation = m_ReactionLocation;

    if (m_ReactionLocation.GirderKey.girderIndex == m_nGirdersThisGroup - 1)
    {
        if (m_ReactionLocation.GirderKey.groupIndex == m_nGroups - 1)
        {
            if (belowLocation.Face == rftBack)
            {
                belowLocation.GirderKey.girderIndex = 0;
                belowLocation.GirderKey.groupIndex = 0;
                belowLocation.PierIdx = 0;
                belowLocation.Face = rftAhead;
            }
            else
            {
                belowLocation.GirderKey.girderIndex = 0;
                belowLocation.PierIdx++;
                belowLocation.Face = rftBack;
            }
        }
        else
        {
            if (belowLocation.Face == rftAhead)
            {
                belowLocation.GirderKey.girderIndex = 0;
                belowLocation.PierIdx++;
                belowLocation.Face = rftBack;
            }
            else
            {
                belowLocation.GirderKey.girderIndex = 0;
                belowLocation.GirderKey.groupIndex++;
                belowLocation.Face = rftAhead;
            }
        }
    }
    else
    {
        belowLocation.GirderKey.girderIndex++;
    }

    m_pFrame->SelectBearing(belowLocation);
}

void CBridgePlanViewBearingDisplayObjectEvents::SelectRightBearing()
{
    ReactionLocation rightLocation = m_ReactionLocation;

    bool bInterior = false;
	if (m_ReactionLocation.PierIdx + 1 < m_nPiers)
    {
        auto pBroker = EAFGetBroker();
        GET_IFACE2(pBroker, IBridge, pBridge);
        bInterior = pBridge->IsInteriorPier(m_ReactionLocation.PierIdx + 1);
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
            rightLocation.PierIdx++;
        }
        else
        {
            rightLocation.Face = rftBack;
            rightLocation.PierIdx++;
        }
	}
    else
    {
        if (m_ReactionLocation.PierIdx == m_nPiers - 1)
        {
            if (m_ReactionLocation.GirderKey.girderIndex == m_nGirdersThisGroup - 1)
            {
                rightLocation.Face = rftAhead;
                rightLocation.PierIdx = 0;
                rightLocation.GirderKey.groupIndex = 0;
                rightLocation.GirderKey.girderIndex = 0;
            }
            else
            {
                rightLocation.Face = rftAhead;
                rightLocation.PierIdx = 0;
                rightLocation.GirderKey.groupIndex = 0;
                rightLocation.GirderKey.girderIndex++;
            }
        }
        else
        {
            rightLocation.Face = rftAhead;
            rightLocation.GirderKey.groupIndex++;
        }
    }
    
    m_pFrame->SelectBearing(rightLocation);

}

void CBridgePlanViewBearingDisplayObjectEvents::SelectLeftBearing()
{
    ReactionLocation leftLocation = m_ReactionLocation;

    if (m_ReactionLocation.Face == rftBack)
    {
        leftLocation.Face = rftAhead;
		leftLocation.PierIdx--;
    }
    else
    {
        if (m_ReactionLocation.PierIdx == 0)
        {
            if (m_ReactionLocation.GirderKey.girderIndex == 0)
            {
                leftLocation.Face = rftBack;
                leftLocation.PierIdx = m_nPiers - 1;
                leftLocation.GirderKey.groupIndex = m_nGroups - 1;
                leftLocation.GirderKey.girderIndex = m_nGirdersThisGroup - 1;
            }
            else
            {
                leftLocation.Face = rftBack;
                leftLocation.PierIdx = m_nPiers - 1;
				leftLocation.GirderKey.girderIndex--;
                leftLocation.GirderKey.groupIndex = m_nGroups - 1;
            }
        }
		else
        {
            leftLocation.Face = rftBack;
            leftLocation.GirderKey.groupIndex--;
        }
    }
    
    m_pFrame->SelectBearing(leftLocation);

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
       SelectLeftBearing();
       return true;
   }
   else if (nChar == VK_RIGHT)
   {
       SelectRightBearing();
       return true;
   }
   else if ( nChar == VK_UP )
   {
      SelectBearingAbove();
      return true;
   }
   else if ( nChar == VK_DOWN )
   {
      SelectBearingBelow();
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

