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

// TogaGMDisplayMgrEventsImpl.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TogaGMDisplayMgrEventsImpl.h"

#include <IFace\EditByUI.h>


/////////////////////////////////////////////////////////////////////////////
// CTogaGMDisplayMgrEventsImpl
CTogaGMDisplayMgrEventsImpl::CTogaGMDisplayMgrEventsImpl(CTxDOTOptionalDesignDoc* pDoc,CTxDOTOptionalDesignGirderViewPage* pFrame, CWnd* pParent)
{
   m_pDoc    = pDoc;
   m_pFrame  = pFrame;
   m_pParent = pParent;
}

CTogaGMDisplayMgrEventsImpl::~CTogaGMDisplayMgrEventsImpl()
{
}

bool CTogaGMDisplayMgrEventsImpl::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO,UINT nFlags,const POINT& point)
{
   m_pFrame->ShowCutDlg();
   return true;
}

bool CTogaGMDisplayMgrEventsImpl::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO,UINT nFlags,const POINT& point)
{
   m_pParent->SetFocus();
   return false;
}

bool CTogaGMDisplayMgrEventsImpl::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CTogaGMDisplayMgrEventsImpl::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CTogaGMDisplayMgrEventsImpl::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CTogaGMDisplayMgrEventsImpl::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CTogaGMDisplayMgrEventsImpl::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CTogaGMDisplayMgrEventsImpl::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CTogaGMDisplayMgrEventsImpl::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_LEFT )
   {
      m_pFrame->CutAtPrev();
      return true;
   }
   else if ( nChar == VK_RIGHT )
   {
      m_pFrame->CutAtNext();
      return true;
   }

   return false;
}

bool CTogaGMDisplayMgrEventsImpl::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayMgr> pDO,CWnd* pWnd,const POINT& point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CMenu menu;
   menu.LoadMenu(IDR_GIRDER_VIEW_CTX);
   CMenu* pcontext = menu.GetSubMenu(0);

   POINT client_point = point;
   if ( point.x < 0 || point.y < 0 )
   {
      // the context menu key or Shift+F10 was pressed
      // need some real coordinates (how about the center of the client area)
      CRect rClient;
      m_pParent->GetClientRect(&rClient);
      CPoint center = rClient.TopLeft();
      m_pParent->ClientToScreen(&center);
      client_point = center;
   }

   pcontext->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, client_point.x, client_point.y, m_pParent );
   return true;
}
