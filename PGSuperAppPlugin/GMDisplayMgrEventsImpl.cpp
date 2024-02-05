///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"
#include "GMDisplayMgrEventsImpl.h"
#include "GirderModelChildFrame.h"
#include <IReportManager.h>
#include <IFace\EditByUI.h>
#include "PGSpliceDoc.h"

#include <DManip/DisplayMgr.h>
#include <DManip/DisplayView.h>

std::shared_ptr<CGMDisplayMgrEventsImpl> CGMDisplayMgrEventsImpl::Create(CPGSDocBase* pDoc, CGirderModelChildFrame* pFrame, CWnd* pParent, bool bGirderElevation)
{
   return std::shared_ptr<CGMDisplayMgrEventsImpl>(new CGMDisplayMgrEventsImpl(pDoc, pFrame, pParent, bGirderElevation));
}


CGMDisplayMgrEventsImpl::CGMDisplayMgrEventsImpl(CPGSDocBase* pDoc,CGirderModelChildFrame* pFrame, CWnd* pParent,bool bGirderElevation)
{
   m_pDoc    = pDoc;
   m_pFrame  = pFrame;
   m_pParent = pParent;
   m_bGirderElevation = bGirderElevation;
}

bool CGMDisplayMgrEventsImpl::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDisplayMgr,UINT nFlags,const POINT& point)
{
   // only handle left button double click in the field of the section view
   if (m_bGirderElevation) return false;
   
   auto poi = m_pFrame->GetCutLocation();
   m_pDoc->EditGirderDescription(poi.GetSegmentKey(), EGD_GENERAL);

   return true;
}

bool CGMDisplayMgrEventsImpl::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDisplayMgr,UINT nFlags,const POINT& point)
{
   return false;
}

bool CGMDisplayMgrEventsImpl::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDisplayMgr,UINT nFlags,const POINT& point)
{
   return false;
}

bool CGMDisplayMgrEventsImpl::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDisplayMgr,UINT nFlags,const POINT& point)
{
   return false;
}

bool CGMDisplayMgrEventsImpl::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDisplayMgr,UINT nFlags,const POINT& point)
{
   return false;
}

bool CGMDisplayMgrEventsImpl::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDisplayMgr,UINT nFlags,const POINT& point)
{
   return false;
}

bool CGMDisplayMgrEventsImpl::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDisplayMgr,UINT nFlags,const POINT& point)
{
   return false;
}

bool CGMDisplayMgrEventsImpl::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDisplayMgr,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CGMDisplayMgrEventsImpl::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDisplayMgr,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   return false;
}

bool CGMDisplayMgrEventsImpl::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDisplayMgr,CWnd* pWnd,const POINT& point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CDisplayView* pView = pDisplayMgr->GetView();
   CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();

   CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
   pMenu->LoadMenu(IDR_GIRDER_CTX,nullptr);

   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
   {
      // PGSplice doesn't do design
      CString strDesignGirder;
      pMenu->GetMenuString(ID_GIRDERVIEW_DESIGNGIRDERDIRECT,strDesignGirder,MF_BYCOMMAND);
      UINT nPos = pMenu->FindMenuItem(strDesignGirder);
      pMenu->RemoveMenu(nPos-1,MF_BYPOSITION,nullptr); // remove the separater before "Design Girder"
      pMenu->RemoveMenu(ID_GIRDERVIEW_DESIGNGIRDERDIRECT,MF_BYCOMMAND,nullptr);
      pMenu->RemoveMenu(ID_GIRDERVIEW_DESIGNGIRDERDIRECTHOLDSLABOFFSET,MF_BYCOMMAND,nullptr);

      // PGSplice does not use moment loads
      pMenu->RemoveMenu(ID_ADD_MOMENT_LOAD,MF_BYCOMMAND,nullptr);

      // In the context of the whole view, harp points don't make sense for spliced girder bridges
      // Each segment can have harp points... to which segment are we referring? Each segment
      // can have a different number of harp points as well.
      pMenu->RemoveMenu(ID_LEFT_HP,MF_BYCOMMAND,nullptr);
      pMenu->RemoveMenu(ID_RIGHT_HP,MF_BYCOMMAND,nullptr);
   }

#pragma Reminder("REVIEW: context menu items incorrect if less that two harp points")
   // If a PGSuper girder/segment has less than two harp points, the context menu will not be correct
   // There isn't a left/right harp points.

   pDoc->BuildReportMenu(pMenu,true);

   CPoint p = point;
   if ( p.x < 0 || p.y < 0 )
   {
      // the context menu key or Shift+F10 was pressed
      // need some real coordinates (how about the center of the client area)
      CRect rClient;
      m_pParent->GetClientRect(&rClient);
      CPoint center = rClient.TopLeft();
      m_pParent->ClientToScreen(&center);
      p = center;
   }

   if ( m_bGirderElevation )
   {
      const auto& callbacks = pDoc->GetGirderElevationViewCallbacks();
      for ( const auto& callback : callbacks )
      {
         IGirderElevationViewEventCallback* pCallback = callback.second;
         pCallback->OnBackgroundContextMenu(pMenu);
      }
   }
   else
   {
      const auto& callbacks = pDoc->GetGirderSectionViewCallbacks();
      for ( const auto& callback : callbacks)
      {
         IGirderSectionViewEventCallback* pCallback = callback.second;
         pCallback->OnBackgroundContextMenu(pMenu);
      }
   }

   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, p.x,p.y, m_pFrame );
   return true;
}
