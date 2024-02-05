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

#include "stdafx.h"
#include "resource.h"
#include "DistributedLoadDrawStrategyImpl.h"
#include <IFace\EditByUI.h> 
#include <MathEx.h>

#include <DManip/PointDisplayObject.h>
#include <DManip/DimensionLine.h>
#include <DManip/DisplayMgr.h>
#include <DManip/DisplayView.h>

// height of maximum loads
static const long SSIZE = 1440 / 2; // (twips)
// arrowhead in twips;
static const long ARROW_SIZE = 100;

// utilities
void DrawArrowLine(CDC* pDC, long x, long ydatum, long yend, CSize& arrowSize);
CSize GetLArrowSize(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap);

UINT CDistributedLoadDrawStrategyImpl::ms_Format = ::RegisterClipboardFormat(_T("DistributedLoadDrawStrategyImpl"));

CDistributedLoadDrawStrategyImpl::CDistributedLoadDrawStrategyImpl()
{
}


void CDistributedLoadDrawStrategyImpl::Init(std::shared_ptr<WBFL::DManip::iPointDisplayObject> pDO, IBroker* pBroker, CDistributedLoadData load, IndexType loadIndex, 
                                                       Float64 loadLength, Float64 spanLength, Float64 maxMagnitude, COLORREF color)
{
   m_Load = load;
   m_LoadIndex = loadIndex;
   m_pBroker = pBroker;
   m_Color = color;
   m_MaxMagnitude = maxMagnitude;
   m_SpanLength   = spanLength;
   m_LoadLength   = loadLength;

   // distance between vertical arrows
   m_ArrowSpacing = spanLength/50.0;
}


void CDistributedLoadDrawStrategyImpl::Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC) const
{
   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();

   COLORREF color = pDO->IsSelected() ? pDispMgr->GetSelectionLineColor() : m_Color;

   auto pos = pDO->GetPosition();

   Draw(pDO,pDC,color, pos);
}

void CDistributedLoadDrawStrategyImpl::DrawHighlight(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC,bool bHighlite) const
{
   Draw(pDO,pDC);
}

void CDistributedLoadDrawStrategyImpl::DrawDragImage(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC, std::shared_ptr<const WBFL::DManip::iCoordinateMap> map, const POINT& dragStart, const POINT& dragPoint) const
{
   Float64 wx, wy;
   map->LPtoWP(dragPoint.x, dragPoint.y, &wx, &wy);
   m_ReusablePoint.Move(wx,wy);

   Draw(pDO,pDC,RGB(255,0,0),m_ReusablePoint);
}

WBFL::Geometry::Rect2d CDistributedLoadDrawStrategyImpl::GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO) const
{
   auto point = pDO->GetPosition();

   Float64 xpos = point.X();

   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();
   auto pMap = pDispMgr->GetCoordinateMap();

   Float64 wystart, wyend;
   GetWLoadHeight(pMap, &wystart, &wyend);

   Float64 top = Max(0.0,wystart, wyend);
   Float64 bot = Min(0.0,wystart, wyend);

   WBFL::Geometry::Rect2d bounding_box;

   bounding_box.Top() = top;
   bounding_box.Left() = xpos;
   bounding_box.Bottom() = bot;
   bounding_box.Right() = xpos+m_LoadLength;

   return bounding_box;
}


void CDistributedLoadDrawStrategyImpl::Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC,COLORREF color, const WBFL::Geometry::Point2d& loc) const
{
   if (m_LoadLength<=0.0)
   {
      ATLASSERT(false);
      return;
   }

   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();
   auto pMap = pDispMgr->GetCoordinateMap();

   // bottom of load is at top of girder
   Float64 wyb_at_start;
   wyb_at_start = 0;

   Float64 wyb_at_end;
   wyb_at_end = 0;

   Float64 wx_start = loc.X();

   Float64 wx_end = wx_start + m_LoadLength;

   Float64 load_end   = (m_Load.m_Fractional ? m_Load.m_EndLocation*m_SpanLength : m_Load.m_EndLocation  );

   // pen style
   UINT nWidth = 1;
   UINT nPenStyle = PS_SOLID;
   if ( (m_Load.m_Type == UserLoads::Uniform && m_Load.m_WStart == 0.0) || 
        (m_Load.m_WStart == 0.0 && m_Load.m_WEnd == 0.0) ||
        (m_SpanLength < load_end))
   {
      nPenStyle = PS_DOT;
   }

   long lx_start, lx_end, ly_zero;
   pMap->WPtoLP(wx_start, wyb_at_start, &lx_start, &ly_zero);
   pMap->WPtoLP(wx_end  , wyb_at_end, &lx_end,   &ly_zero);

   // height of load at start and end
   long ly_start, ly_end;
   GetLLoadHeight(pMap, &ly_start, &ly_end);
   ly_start += ly_zero;
   ly_end   += ly_zero;

   CPen pen(nPenStyle,nWidth,color);
   CPen* pold_pen = pDC->SelectObject(&pen);
   CBrush brush(color);
   CBrush* pold_brush = pDC->SelectObject(&brush);

   // draw top chord of load
   pDC->MoveTo(lx_start,ly_start);
   pDC->LineTo(lx_end,ly_end);

   // spacing of verticals
   Uint32 num_spcs = Uint32(Max( Round(m_LoadLength/m_ArrowSpacing), 1.0));
   Float64 increment = Float64(lx_end-lx_start)/num_spcs; // use Float64 to preserve numerical accuracy

   // arrow size
   CSize ar_size = GetLArrowSize(pMap);

   // first line
   DrawArrowLine(pDC, lx_start, ly_zero, ly_start, ar_size);

   Float64 lxloc = lx_start;
   for (Uint32 it = 0; it < num_spcs; it++)
   {
      lxloc += increment;

      long delta = lx_end-lx_start; // avoid divide by zeros
      long val;
      if (0 < delta)
         val = LinInterp((long)(lxloc-lx_start), ly_start, ly_end, lx_end-lx_start);
      else
         val = ly_start;

      DrawArrowLine(pDC, long(lxloc), ly_zero, val, ar_size);
   }

   pDC->SelectObject(pold_pen);
   pDC->SelectObject(pold_brush);
}


void CDistributedLoadDrawStrategyImpl::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CDistributedLoadDrawStrategyImpl::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
   ASSERT(FALSE); // Points must be dropped on a member. This event should never occur
}

void CDistributedLoadDrawStrategyImpl::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CDistributedLoadDrawStrategyImpl::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   // No big deal...
}

bool CDistributedLoadDrawStrategyImpl::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
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

bool CDistributedLoadDrawStrategyImpl::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   auto list = pDO->GetDisplayList();
   auto dispMgr = list->GetDisplayMgr();

   dispMgr->SelectObject(pDO, true);

   return true;
}

bool CDistributedLoadDrawStrategyImpl::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CDistributedLoadDrawStrategyImpl::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
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

bool CDistributedLoadDrawStrategyImpl::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CDistributedLoadDrawStrategyImpl::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CDistributedLoadDrawStrategyImpl::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CDistributedLoadDrawStrategyImpl::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CDistributedLoadDrawStrategyImpl::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
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

bool CDistributedLoadDrawStrategyImpl::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   return false;
}

void CDistributedLoadDrawStrategyImpl::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CDistributedLoadDrawStrategyImpl::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}


void CDistributedLoadDrawStrategyImpl::EditLoad()
{
   GET_IFACE(IEditByUI, pEditByUI);
   pEditByUI->EditDistributedLoad(m_LoadIndex);
}

void CDistributedLoadDrawStrategyImpl::DeleteLoad()
{
   GET_IFACE(IEditByUI, pEditByUI);
   pEditByUI->DeleteDistributedLoad(m_LoadIndex);
}

/////////////////////////////////////////////////////////
// iGravityWellStrategy Implementation
void CDistributedLoadDrawStrategyImpl::GetGravityWell(std::shared_ptr<const WBFL::DManip::iDisplayObject> pDO,CRgn* pRgn)
{
   auto pPointDO = std::dynamic_pointer_cast<const WBFL::DManip::iPointDisplayObject>(pDO);

   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();
   auto pMap = pDispMgr->GetCoordinateMap();

   auto point = pPointDO->GetPosition();

   Float64 wxstart, wxend;
   wxstart = point.X();
   wxend = wxstart + m_LoadLength;

   Float64 wystart, wyend;
   GetWLoadHeight(pMap, &wystart, &wyend);

   // convert to logical
   long lxstart, lxend, lystart, lyend, lyzero;
   pMap->WPtoLP(wxstart, wystart, &lxstart, &lystart);
   pMap->WPtoLP(wxend, wyend, &lxend, &lyend);
   pMap->WPtoLP(wxend, 0.0, &lxend, &lyzero);

   CPoint p[4];
   p[0] = CPoint(lxstart, lystart);
   p[1] = CPoint(lxend, lyend);
   p[2] = CPoint(lxend, lyzero);
   p[3] = CPoint(lxstart, lyzero);
   VERIFY(pRgn->CreatePolygonRgn(p,4,WINDING));
}


void CDistributedLoadDrawStrategyImpl::GetTLoadHeight(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, long* startHgt, long* endHgt) const
{
   if (m_Load.m_Type==UserLoads::Uniform && m_Load.m_WStart==0.0 ||
       m_Load.m_WStart==0.0 && m_Load.m_WEnd==0.0)
   {
      *startHgt = SSIZE;
      *endHgt = SSIZE;
   }
   else
   {
      ATLASSERT(m_MaxMagnitude!=0.0);

      *startHgt = long(m_Load.m_WStart/m_MaxMagnitude * SSIZE);

      if (m_Load.m_Type==UserLoads::Uniform)
      {
         *endHgt = *startHgt;
      }
      else
      {
         *endHgt = long(m_Load.m_WEnd/m_MaxMagnitude * SSIZE);
      }
   }

   // don't allow loads to shrink out of sight
   long min_hgt = (long)Round(SSIZE/10.0);
   if (abs(*endHgt) < min_hgt && abs(*startHgt) < min_hgt)
   {
      *startHgt = ::BinarySign(*startHgt)*min_hgt;
      *endHgt   = ::BinarySign(*endHgt)*min_hgt;
   }
}

void CDistributedLoadDrawStrategyImpl::GetWLoadHeight(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, Float64* startHgt, Float64* endHgt) const
{
   long tystart, tyend;
   GetTLoadHeight(pMap, &tystart, &tyend);

   Float64 wxo,wyo;
   pMap->TPtoWP(0,0,&wxo,&wyo);
   Float64 wystart,wyend;
   pMap->TPtoWP(0,tystart,&wxo,&wystart);
   pMap->TPtoWP(0,tyend,  &wxo,&wyend);

   *startHgt = wystart-wyo;
   *endHgt   = wyend  -wyo;
}

void CDistributedLoadDrawStrategyImpl::GetLLoadHeight(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, long* startHgt, long* endHgt) const
{
   long tystart, tyend;
   GetTLoadHeight(pMap, &tystart, &tyend);

   long lxo,lyo;
   pMap->TPtoLP(0,0,&lxo,&lyo);
   long lystart,lyend;
   pMap->TPtoLP(0,tystart,&lxo, &lystart);
   pMap->TPtoLP(0,tyend,  &lxo, &lyend);

   *startHgt = lystart-lyo;
   *endHgt   = lyend  -lyo;
}

void DrawArrowLine(CDC* pDC, long x, long ydatum, long yend, CSize& arrowSize)
{
   // draw shaft
   pDC->MoveTo(x,ydatum);
   pDC->LineTo(x,yend);

   // draw arrow head only if arrow is shorter than line
   if (abs(yend-ydatum) > abs(arrowSize.cy) )
   {
      long cx = arrowSize.cx;
      long cy;
      if (yend>ydatum)
         cy = -arrowSize.cy;
      else
         cy = arrowSize.cy;

      POINT points[3];
      points[0].x = x-cx;
      points[0].y = ydatum+cy;
      points[1].x = x;
      points[1].y = ydatum;
      points[2].x = x+cx;
      points[2].y = ydatum+cy;

      pDC->Polygon(points, 3);
   }
}

CSize GetLArrowSize(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap)
{
   long lxo,lyo;
   pMap->TPtoLP(0,0,&lxo,&lyo);
   long lxe, lye;
   pMap->TPtoLP(ARROW_SIZE/4,ARROW_SIZE, &lxe, &lye);

   return CSize(lxe-lxo, lye-lyo);
}
