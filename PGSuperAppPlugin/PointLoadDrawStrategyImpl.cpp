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

#include "stdafx.h"
#include "resource.h"
#include "PointLoadDrawStrategyImpl.h"

#include <IFace/Tools.h>
#include <IFace\EditByUI.h> 

#include "PGSuperColors.h"

#include <MathEx.h>

#include <DManip/DisplayMgr.h>
#include <DManip/DisplayView.h>

// height of maximum loads
static const long SSIZE = 1440 * 3/4; // (twips)


UINT CPointLoadDrawStrategyImpl::ms_Format = ::RegisterClipboardFormat(_T("PointLoadDrawStrategyImpl"));

CPointLoadDrawStrategyImpl::CPointLoadDrawStrategyImpl()
{
}

void CPointLoadDrawStrategyImpl::Init(std::shared_ptr<WBFL::DManip::iPointDisplayObject> pDO, std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPointLoadData& load,
                                                 IndexType loadIndex, Float64 spanLength, 
                                                 Float64 maxMagnitude, COLORREF color)
{
   m_Load          = load;
   m_LoadIndex     = loadIndex;
   m_pBroker       = pBroker;
   m_Color         = color;
   m_MaxMagnitude  = maxMagnitude;
   m_SpanLength    = spanLength;
}


void CPointLoadDrawStrategyImpl::Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC) const
{
   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();

   COLORREF color = pDO->IsSelected() ? pDispMgr->GetSelectionLineColor() : m_Color;

   auto pos = pDO->GetPosition();

   Draw(pDO,pDC,color, pos);
}

void CPointLoadDrawStrategyImpl::DrawHighlight(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC,bool bHighlite) const
{
   Draw(pDO,pDC);
}

void CPointLoadDrawStrategyImpl::DrawDragImage(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC, std::shared_ptr<const WBFL::DManip::iCoordinateMap> map, const POINT& dragStart, const POINT& dragPoint) const
{
   Float64 wx, wy;
   map->LPtoWP(dragPoint.x, dragPoint.y, &wx, &wy);
   m_CachePoint.Move(wx,wy);

   Draw(pDO,pDC,SELECTED_OBJECT_LINE_COLOR, m_CachePoint);
}

WBFL::Geometry::Rect2d CPointLoadDrawStrategyImpl::GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO) const
{
   auto point = pDO->GetPosition();

   Float64 xpos = point.X();

   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();
   auto pMap = pDispMgr->GetCoordinateMap();

   Float64 height, width;
   GetWSymbolSize(pMap, &width, &height);

   WBFL::Geometry::Rect2d bounding_box;
   bounding_box.Top() = height;
   bounding_box.Left() = xpos-width;
   bounding_box.Bottom() = 0;
   bounding_box.Right() = xpos+width;

   return bounding_box;
}


void CPointLoadDrawStrategyImpl::Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC,COLORREF color, const WBFL::Geometry::Point2d& loc) const
{
   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();
   auto pMap = pDispMgr->GetCoordinateMap();

   // bottom of load is at top of girder
   Float64 wx, wyb;
   wx = loc.X();
   wyb = 0;

   long cxb,cyb;
   pMap->WPtoLP(wx, wyb, &cxb, &cyb);

   // height and width of load
   Float64 wid, hgt;
   GetWSymbolSize(pMap, &wid, &hgt);

   // line style and width
   Float64 location = (m_Load.m_Fractional ? m_SpanLength*m_Load.m_Location : m_Load.m_Location);
   bool funky_load = ::IsZero(m_Load.m_Magnitude) || m_SpanLength < location;

   int nWidth    = funky_load ? 1 : 2;
   int nPenStyle = funky_load ? PS_DOT : PS_SOLID;

   CPen pen(nPenStyle,nWidth,color);
   CPen* pOldPen = pDC->SelectObject(&pen);

   // top of load
   long cxt, cyt;
   pMap->WPtoLP(wx,wyb+hgt,&cxt,&cyt);

   // draw shaft
   pDC->MoveTo(cxb,cyb);
   pDC->LineTo(cxt,cyt);

   long w,h;
   GetLSymbolSize(pMap, &w, &h);
   POINT points[3];
   // draw arrow head
   if (m_Load.m_Magnitude >= 0.0)
   {
      // down arrow
      points[0].x = cxb-w;
      points[0].y = cyb-w;
      points[1].x = cxb;
      points[1].y = cyb;
      points[2].x = cxb+w;
      points[2].y = cyb-w;
   }
   else
   {
      // up arrow
      points[0].x = cxt-w;
      points[0].y = cyt+w;
      points[1].x = cxt;
      points[1].y = cyt;
      points[2].x = cxt+w;
      points[2].y = cyt+w;
   }
 
   // have to draw like this in order to make dots symmetrical
   pDC->MoveTo(points[1]);
   pDC->LineTo(points[0]);
   pDC->MoveTo(points[1]);
   pDC->LineTo(points[2]);

   pDC->SelectObject(pOldPen);
}


void CPointLoadDrawStrategyImpl::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CPointLoadDrawStrategyImpl::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
   ASSERT(FALSE); // Points must be dropped on a member. This event should never occur
}

void CPointLoadDrawStrategyImpl::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CPointLoadDrawStrategyImpl::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   // No big deal...
}

bool CPointLoadDrawStrategyImpl::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   if (pDO->IsSelected())
   {
      EditLoad();
      return true;
   }
   else
   {
      return false;
   }
}

bool CPointLoadDrawStrategyImpl::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   auto list = pDO->GetDisplayList();
   auto dispMgr = list->GetDisplayMgr();

   dispMgr->SelectObject(pDO, true);

   return true;
}

bool CPointLoadDrawStrategyImpl::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CPointLoadDrawStrategyImpl::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   auto list = pDO->GetDisplayList();
   auto dispMgr = list->GetDisplayMgr();

   dispMgr->SelectObject(pDO, true);

   auto view = dispMgr->GetView();
   POINT screen_point = point;
   view->ClientToScreen(&screen_point);

   CMenu menu;
   VERIFY( menu.LoadMenu(IDR_LOADS_CTX) );
   menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, screen_point.x, screen_point.y, view);

   return true;
}

bool CPointLoadDrawStrategyImpl::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CPointLoadDrawStrategyImpl::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CPointLoadDrawStrategyImpl::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CPointLoadDrawStrategyImpl::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CPointLoadDrawStrategyImpl::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   switch(nChar)
   {
   case VK_RETURN:
      EditLoad();
      return true;
      
   case VK_DELETE:
      DeleteLoad();
      return true;
   }
   return false;
}

bool CPointLoadDrawStrategyImpl::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   return false;
}

void CPointLoadDrawStrategyImpl::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CPointLoadDrawStrategyImpl::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}


void CPointLoadDrawStrategyImpl::EditLoad()
{
   GET_IFACE(IEditByUI, pEditByUI);
   pEditByUI->EditPointLoad(m_LoadIndex);
}

void CPointLoadDrawStrategyImpl::DeleteLoad()
{
   GET_IFACE(IEditByUI, pEditByUI);
   pEditByUI->DeletePointLoad(m_LoadIndex);
}

void CPointLoadDrawStrategyImpl::GetTSymbolSize(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, long* psx, long* psy) const
{
   Float64 frac = Max( (fabs(m_Load.m_Magnitude)/m_MaxMagnitude), 1.0/6.0); // minimum symbol size
   if (frac!=0.0)
      *psy = long(floor(frac * SSIZE));
   else
      *psy = SSIZE;

   *psx = SSIZE/6;
}

void CPointLoadDrawStrategyImpl::GetWSymbolSize(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, Float64* psx, Float64* psy) const
{
   long hgt, wid2;
   GetTSymbolSize(pMap, &wid2, &hgt);

   Float64 xo,yo;
   pMap->TPtoWP(0,0,&xo,&yo);
   Float64 x2,y2;
   pMap->TPtoWP(wid2,hgt,&x2,&y2);

   *psx = x2-xo;
   *psy = y2-yo;
}

void CPointLoadDrawStrategyImpl::GetLSymbolSize(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, long* psx, long* psy) const
{
   long hgt, wid2;
   GetTSymbolSize(pMap, &wid2, &hgt);

   long xo,yo;
   pMap->TPtoLP(0,0,&xo,&yo);
   long x2,y2;
   pMap->TPtoLP(wid2,hgt,&x2,&y2);

   *psx = x2-xo;
   *psy = y2-yo;
}
