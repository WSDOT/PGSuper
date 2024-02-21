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
#include "MomentLoadDrawStrategyImpl.h"
#include <IFace\EditByUI.h> 
#include <MathEx.h>

#include <DManip/PointDisplayObject.h>
#include <DManip/DisplayMgr.h>
#include <DManip/DisplayView.h>

// height of maximum loads
static const Uint32 SSIZE = 1440 * 1/2; // (twips)


UINT CMomentLoadDrawStrategyImpl::ms_Format = ::RegisterClipboardFormat(_T("MomentLoadDrawStrategyImpl"));

CMomentLoadDrawStrategyImpl::CMomentLoadDrawStrategyImpl()
{
}

void CMomentLoadDrawStrategyImpl::Init(std::shared_ptr<WBFL::DManip::iPointDisplayObject> pDO, IBroker* pBroker, CMomentLoadData load,
                                                 IndexType loadIndex, Float64 spanLength, 
                                                 Float64 maxMagnitude, COLORREF color)
{
   m_Load = load;
   m_LoadIndex = loadIndex;
   m_pBroker = pBroker;
   m_Color = color;
   m_MaxMagnitude = maxMagnitude;
   m_SpanLength = spanLength;
}


void CMomentLoadDrawStrategyImpl::Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC) const
{
   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();

   COLORREF color = pDO->IsSelected() ? pDispMgr->GetSelectionLineColor() : m_Color;

   auto pos = pDO->GetPosition();

   Draw(pDO,pDC,color, pos);
}

void CMomentLoadDrawStrategyImpl::DrawHighlight(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC,bool bHighlite) const
{
   Draw(pDO,pDC);
}

void CMomentLoadDrawStrategyImpl::DrawDragImage(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC, std::shared_ptr<const WBFL::DManip::iCoordinateMap> map, const POINT& dragStart, const POINT& dragPoint) const
{
   Float64 wx, wy;
   map->LPtoWP(dragPoint.x, dragPoint.y, &wx, &wy);
   m_CachePoint.Move(wx,wy);
   Draw(pDO,pDC,RGB(255,0,0), m_CachePoint);
}

WBFL::Geometry::Rect2d CMomentLoadDrawStrategyImpl::GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO) const
{
   auto point = pDO->GetPosition();

   Float64 xpos = point.X();

   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();
   auto pMap = pDispMgr->GetCoordinateMap();

   Float64 diameter;
   GetWSymbolSize(pMap, &diameter);

   WBFL::Geometry::Rect2d bounding_box;

   bounding_box.Top() = 3*diameter/4;
   bounding_box.Left() = xpos-diameter/2;
   bounding_box.Bottom() = -3*diameter/4;
   bounding_box.Right() = xpos+diameter/2;

   return bounding_box;
}


void CMomentLoadDrawStrategyImpl::Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC,COLORREF color, const WBFL::Geometry::Point2d& loc) const
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

   // diameter of moment symbol
   Uint32 diameter;
   GetLSymbolSize(pMap, &diameter);
   Uint32 radius = diameter / 2;

   // line style and width
   Float64 location = (m_Load.m_Fractional ? m_SpanLength*m_Load.m_Location : m_Load.m_Location);
   bool funky_load = ::IsZero(m_Load.m_Magnitude) || m_SpanLength < location;

   int nWidth    = funky_load ? 1 : 2;
   int nPenStyle = funky_load ? PS_DOT : PS_SOLID;

   CPen pen(nPenStyle,nWidth,color);
   CPen* pOldPen = pDC->SelectObject(&pen);

   // top of load
   long cxt = cxb;
   long cyt = cyb + diameter;

   // draw arc
   CRect rect(cxb-radius,cyb-radius,cxb+radius,cyb+radius);
   pDC->Arc(rect,CPoint(cxb,cyb+diameter),CPoint(cxt,cyb-diameter));

   POINT points[3];
   // draw arrow head
   if (0.0 <= m_Load.m_Magnitude)
   {
      // arrowhead at top
      points[0].x = cxb+radius/2;
      points[0].y = cyb-radius/2;
      points[1].x = cxb;
      points[1].y = cyb-radius;
      points[2].x = cxb+radius/2;
      points[2].y = cyb-3*radius/2;
   }
   else
   {
      // arrowhead at bottom
      points[0].x = cxb+radius/2;
      points[0].y = cyb+3*radius/2;
      points[1].x = cxb;
      points[1].y = cyb+radius;
      points[2].x = cxb+radius/2;
      points[2].y = cyb+radius/2;
   }
 
   // have to draw like this in order to make dots symmetrical
   pDC->MoveTo(points[1]);
   pDC->LineTo(points[0]);
   pDC->MoveTo(points[1]);
   pDC->LineTo(points[2]);

   // draw a little dot where the load is applied
   CRect dot(cxb,cyb,cxb,cyb);
   dot.InflateRect(1,1,1,1);
   pDC->Ellipse(dot);

   pDC->SelectObject(pOldPen);
}


void CMomentLoadDrawStrategyImpl::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CMomentLoadDrawStrategyImpl::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
   ASSERT(FALSE); // Points must be dropped on a member. This event should never occur
}

void CMomentLoadDrawStrategyImpl::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CMomentLoadDrawStrategyImpl::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   // No big deal...
}

bool CMomentLoadDrawStrategyImpl::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
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

bool CMomentLoadDrawStrategyImpl::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   auto list = pDO->GetDisplayList();
   auto dispMgr = list->GetDisplayMgr();

   dispMgr->SelectObject(pDO, true);

   return true;
}

bool CMomentLoadDrawStrategyImpl::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CMomentLoadDrawStrategyImpl::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
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

bool CMomentLoadDrawStrategyImpl::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CMomentLoadDrawStrategyImpl::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CMomentLoadDrawStrategyImpl::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CMomentLoadDrawStrategyImpl::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CMomentLoadDrawStrategyImpl::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
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

bool CMomentLoadDrawStrategyImpl::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   return false;
}

void CMomentLoadDrawStrategyImpl::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CMomentLoadDrawStrategyImpl::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}


void CMomentLoadDrawStrategyImpl::EditLoad()
{
   GET_IFACE(IEditByUI, pEditByUI);
   pEditByUI->EditMomentLoad(m_LoadIndex);
}

void CMomentLoadDrawStrategyImpl::DeleteLoad()
{
   GET_IFACE(IEditByUI, pEditByUI);
   pEditByUI->DeleteMomentLoad(m_LoadIndex);

}

void CMomentLoadDrawStrategyImpl::GetTSymbolSize(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, Uint32* pd) const
{
   Float64 frac = Max( (fabs(m_Load.m_Magnitude)/m_MaxMagnitude), 1.0/6.0); // minimum symbol size
   if (frac!=0.0)
   {
      *pd = Uint32(frac * Float64(SSIZE));
   }
   else
   {
      *pd = SSIZE;
   }

//   *psx = SSIZE/6.;
}

void CMomentLoadDrawStrategyImpl::GetWSymbolSize(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, Float64* pd) const
{
   Uint32 d;
   GetTSymbolSize(pMap, &d);

   Float64 xo,yo;
   pMap->TPtoWP(0,0,&xo,&yo);

   Float64 x2,y2;
   pMap->TPtoWP(d,d,&x2,&y2);

   *pd = x2-xo;
   //*psy = y2-yo;
}

void CMomentLoadDrawStrategyImpl::GetLSymbolSize(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, Uint32* pd) const
{
   Uint32 d;
   GetTSymbolSize(pMap, &d);

   long xo,yo;
   pMap->TPtoLP(0,0,&xo,&yo);
   long x2,y2;
   pMap->TPtoLP(d,d,&x2,&y2);

   *pd = x2-xo;
//   *psy = y2-yo;
}
