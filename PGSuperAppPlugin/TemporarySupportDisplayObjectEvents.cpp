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

// TemporarySupportDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"
#include "TemporarySupportDisplayObjectEvents.h"
#include "PGSpliceDoc.h"
#include <IFace\Project.h>
#include <PsgLib\BridgeDescription2.h>

#include <DManip/DisplayObject.h>
#include <DManip/DisplayList.h>
#include <DManip/DisplayView.h>

CTemporarySupportDisplayObjectEvents::CTemporarySupportDisplayObjectEvents(const CTemporarySupportData* pTS, CBridgeModelViewChildFrame* pFrame)
{
   m_pTS = pTS;
   m_pFrame = pFrame;

   m_pPrevTS = nullptr;
   m_pNextTS = nullptr;

   m_pPrevPier = nullptr;
   m_pNextPier = nullptr;

   SupportIndexType tsIdx = pTS->GetIndex();

   m_pNextTS = m_pTS->GetBridgeDescription()->GetTemporarySupport(tsIdx - 1);
   m_pPrevTS = m_pTS->GetBridgeDescription()->GetTemporarySupport(tsIdx + 1);

   if ( (m_pPrevTS && (m_pPrevTS->GetSpan() != pTS->GetSpan())) || m_pPrevTS == nullptr)
   {
      m_pPrevTS = nullptr;
      m_pPrevPier = pTS->GetSpan()->GetPrevPier();
   }

   if ( (m_pNextTS && (m_pNextTS->GetSpan() != pTS->GetSpan())) || m_pNextTS == nullptr )
   {
      m_pNextTS = nullptr;
      m_pNextPier = pTS->GetSpan()->GetNextPier();
   }
}

void CTemporarySupportDisplayObjectEvents::EditTemporarySupport(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   m_pFrame->PostMessage(WM_COMMAND,ID_EDIT_TEMPORARY_SUPPORT,0);
}

void CTemporarySupportDisplayObjectEvents::SelectTemporarySupport(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   // do the selection at the frame level because it will do this view
   // and the other view
   m_pFrame->SelectTemporarySupport(m_pTS->GetID());
}

void CTemporarySupportDisplayObjectEvents::SelectPrev(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   if ( m_pPrevTS )
   {
      m_pFrame->SelectTemporarySupport(m_pPrevTS->GetID());
   }
   else
   {
      m_pFrame->SelectPier(m_pPrevPier->GetIndex());
   }
}

void CTemporarySupportDisplayObjectEvents::SelectNext(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   if ( m_pNextTS )
   {
      m_pFrame->SelectTemporarySupport(m_pNextTS->GetID());
   }
   else
   {
      m_pFrame->SelectPier(m_pNextPier->GetIndex());
   }
}

/////////////////////////////////////////////////////////////////////////////
// CTemporarySupportDisplayObjectEvents message handlers
bool CTemporarySupportDisplayObjectEvents::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   if (pDO->IsSelected())
   {
      EditTemporarySupport(pDO);
      return true;
   }
   else
   {
      return false;
   }
}

bool CTemporarySupportDisplayObjectEvents::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CTemporarySupportDisplayObjectEvents::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CTemporarySupportDisplayObjectEvents::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CTemporarySupportDisplayObjectEvents::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return true; // acknowledge the event so that the object can become selected
}

bool CTemporarySupportDisplayObjectEvents::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CTemporarySupportDisplayObjectEvents::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CTemporarySupportDisplayObjectEvents::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CTemporarySupportDisplayObjectEvents::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_RETURN )
   {
      EditTemporarySupport(pDO);
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
#pragma Reminder("UPDATE")
   //else if ( nChar == VK_UP || nChar == VK_DOWN )
   //{
   //   m_pFrame->SelectGirder(m_SpanIdx,0);
   //   return true;
   //}
   else if ( nChar == VK_DELETE )
   {
      m_pFrame->PostMessage(WM_COMMAND,ID_DELETE,0);
      return true;
   }

   return false;
}

bool CTemporarySupportDisplayObjectEvents::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());


   if ( pDO->IsSelected() )
   {
      auto pView = pDO->GetDisplayList()->GetDisplayMgr()->GetView();
      CPGSpliceDoc* pDoc = (CPGSpliceDoc*)pView->GetDocument();

      auto pMenu = WBFL::EAF::Menu::CreateContextMenu(pDoc->GetPluginCommandManager());
      pMenu->LoadMenu(IDR_SELECTED_TEMPORARY_SUPPORT_CONTEXT,nullptr);

      pMenu->AppendSeparator();
      pMenu->AppendMenu(IDM_ERECTION_TOWER, CTemporarySupportData::AsString(pgsTypes::ErectionTower), nullptr);
      pMenu->AppendMenu(IDM_STRONG_BACK, CTemporarySupportData::AsString(pgsTypes::StrongBack), nullptr);

      if (m_pTS->GetSupportType() == pgsTypes::ErectionTower)
      {
         // toggling of segment connection type is only applicable to erection towers... segments must be joined with a closure joint at strongbacks
         pMenu->AppendSeparator();
         pMenu->AppendMenu(IDM_CONTINUOUS_SEGMENT, CTemporarySupportData::AsString(pgsTypes::tsctContinuousSegment), nullptr);
         pMenu->AppendMenu(IDM_CONTINUOUS_CLOSURE, CTemporarySupportData::AsString(pgsTypes::tsctClosureJoint), nullptr);
      }

      std::map<IDType,IBridgePlanViewEventCallback*> callbacks = pDoc->GetBridgePlanViewCallbacks();
      std::map<IDType,IBridgePlanViewEventCallback*>::iterator iter;
      for ( iter = callbacks.begin(); iter != callbacks.end(); iter++ )
      {
         IBridgePlanViewEventCallback* callback = iter->second;
         callback->OnTemporarySupportContextMenu(m_pTS->GetID(),pMenu);
      }

      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,m_pFrame);

      return true;
   }

   return false;
}

void CTemporarySupportDisplayObjectEvents::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CTemporarySupportDisplayObjectEvents::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
}

void CTemporarySupportDisplayObjectEvents::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CTemporarySupportDisplayObjectEvents::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CTemporarySupportDisplayObjectEvents::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   SelectTemporarySupport(pDO);
}

void CTemporarySupportDisplayObjectEvents::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

