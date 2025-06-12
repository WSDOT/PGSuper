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
#include "BridgeSectionCutDisplayImpl.h"
#include "BridgeModelViewChildFrame.h"
#include <IFace\Alignment.h>
#include <IFace\Bridge.h>
#include <MathEx.h>

#include <WBFLGeometry/GeomHelpers.h>

#include <DManip/PointDisplayObject.h>
#include <DManip/DisplayMgr.h>
#include <DManip/TaskFactory.h>

// height of section cut above/below girder
static const Float64 SSIZE = 1440 * 3/8; // (twips)


UINT CBridgeSectionCutDisplayImpl::ms_Format = ::RegisterClipboardFormat(_T("BridgeSectionCutData"));

void CBridgeSectionCutDisplayImpl::Init(CBridgeModelViewChildFrame* pFrame,std::shared_ptr<WBFL::DManip::iPointDisplayObject> pDO, std::weak_ptr<IRoadway> pRoadway,std::weak_ptr<IBridge> pBridge, iCutLocation* pCutLoc)
{
   m_pFrame = pFrame;
   m_pRoadway = pRoadway;
   m_pBridge = pBridge;
   m_pCutLocation = pCutLoc;

   Float64 pos = m_pCutLocation->GetCurrentCutLocation();
   
   WBFL::Geometry::Point2d pnt(pos, 0.0);
   pDO->SetPosition(pnt, false, false);
}


void CBridgeSectionCutDisplayImpl::SetColor(COLORREF color)
{
   m_Color = color;
}

void CBridgeSectionCutDisplayImpl::Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC) const
{
   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();

   COLORREF color;

   if ( pDO->IsSelected() )
      color = pDispMgr->GetSelectionLineColor();
   else
      color = m_Color;

   auto pos = pDO->GetPosition();

   Draw(pDO,pDC,color,pos);
}

void CBridgeSectionCutDisplayImpl::DrawHighlight(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC,bool bHighlite) const
{
   Draw(pDO,pDC);
}

void CBridgeSectionCutDisplayImpl::DrawDragImage(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC, std::shared_ptr<const WBFL::DManip::iCoordinateMap> map, const POINT& dragStart, const POINT& dragPoint) const 
{
   Float64 wx, wy;
   map->LPtoWP(dragPoint.x, dragPoint.y, &wx, &wy);
   map->WPtoMP(wx, wy, &wx, &wy);
   m_CachePoint.Move(wx, wy);
   Draw(pDO,pDC,RGB(150,150,150),m_CachePoint);
}


WBFL::Geometry::Rect2d CBridgeSectionCutDisplayImpl::GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO) const
{
   auto pnt = pDO->GetPosition();
   
   Float64 xpos = pnt.X();

   Float64 top, left, right, bottom;
   GetBoundingBox(pDO, xpos, &top, &left, &right, &bottom);

   return { left,bottom,right,top };
}

void CBridgeSectionCutDisplayImpl::GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, Float64 position, Float64* top, Float64* left, Float64* right, Float64* bottom) const
{
   auto pos = pDO->GetPosition();

   auto [p1,p2] = GetSectionCutPointsInWorldSpace(pDO,pos);

   auto [x1, y1] = p1.GetLocation();
   auto [x2, y2] = p2.GetLocation();

   POINT left_arrow[3], right_arrow[3];
   GetArrowHeadPoints(pDO,pos,left_arrow,right_arrow);

   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();
   auto pMap = pDispMgr->GetCoordinateMap();

   Float64 lx, ly, rx, ry;
   pMap->LPtoWP(left_arrow[1].x, left_arrow[1].y,  &lx,&ly);
   pMap->LPtoWP(right_arrow[1].x,right_arrow[1].y, &rx,&ry);

   *top    = Max( Max(y1,y2), Max(ly,ry) );
   *bottom = Min( Min(y1,y2), Min(ly,ry) );
   *left   = Min( Min(x1,x2), Min(lx,rx) );
   *right  = Max( Max(x1,x2), Max(lx,rx) );
}

std::pair<WBFL::Geometry::Point2d,WBFL::Geometry::Point2d> CBridgeSectionCutDisplayImpl::GetSectionCutPointsInWorldSpace(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,const WBFL::Geometry::Point2d& userLoc) const
{
   auto roadway = m_pRoadway.lock();
   auto bridge = m_pBridge.lock();

   CComPtr<IPoint2d> p;
   p.CoCreateInstance(CLSID_Point2d);
   p->Move(userLoc.X(), userLoc.Y());
   Float64 station, offset;
   roadway->GetStationAndOffset(pgsTypes::pcGlobal,p,&station,&offset);
   
   CComPtr<IDirection> normal;
   roadway->GetBearingNormal(station,&normal);

   Float64 start_station = bridge->GetPierStation(0);
   Float64 Xb = station - start_station;

   Float64 left  = bridge->GetLeftSlabEdgeOffset(Xb);
   Float64 right = bridge->GetRightSlabEdgeOffset(Xb);

   CComPtr<IPoint2d> p1, p2;
   roadway->GetPoint(station, left,  normal, pgsTypes::pcGlobal, &p1);
   roadway->GetPoint(station, right, normal, pgsTypes::pcGlobal, &p2);

   Float64 x1, y1; p1->Location(&x1, &y1);
   Float64 x2, y2; p2->Location(&x2, &y2);

   Float64 dx = x2 - x1;
   Float64 dy = y2 - y1;

   Float64 extension_factor = 0.1;

   p1->Offset(-extension_factor*dx,-extension_factor*dy);
   p2->Offset( extension_factor*dx, extension_factor*dy);

   // map to world space
   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();
   auto map = pDispMgr->GetCoordinateMap();

   p1->Location(&x1, &y1);
   p2->Location(&x2, &y2);

   map->MPtoWP(x1,y1,&x1,&y1);
   map->MPtoWP(x2,y2,&x2,&y2);

   return { {x1,y1}, {x2,y2} };
}

void CBridgeSectionCutDisplayImpl::GetSectionCutPointsInLogicalSpace(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,const WBFL::Geometry::Point2d& userLoc,POINT& p1,POINT& p2) const
{
   auto [left_point,right_point] = GetSectionCutPointsInWorldSpace(pDO,userLoc);

   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();
   auto pMap = pDispMgr->GetCoordinateMap();

   long x1,y1, x2,y2;
   pMap->WPtoLP(left_point,&x1,&y1);
   pMap->WPtoLP(right_point,&x2,&y2);

   p1.x = x1;
   p1.y = y1;

   p2.x = x2;
   p2.y = y2;
}

void CBridgeSectionCutDisplayImpl::GetArrowHeadPoints(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,const WBFL::Geometry::Point2d& userLoc, POINT* left, POINT* right) const
{
   CPoint p1,p2;
   GetSectionCutPointsInLogicalSpace(pDO,userLoc,p1,p2);
   GetArrowHeadPoints(pDO,userLoc,p1,p2,left,right);
}

void CBridgeSectionCutDisplayImpl::GetArrowHeadPoints(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, const WBFL::Geometry::Point2d& userLoc,CPoint p1,CPoint p2,POINT* left,POINT* right) const
{
   // arrows
   Float64 dx,dy;
   dx = p2.x - p1.x;
   dy = p2.y - p1.y;

   Float64 angle = atan2(dy,dx);

   Float64 xs,ys;
   xs = 10;
   ys = 20;
   left[0].x = p1.x;
   left[0].y = p1.y;
   left[1].x = LONG(p1.x + ys*sin(angle));
   left[1].y = LONG(p1.y - ys*cos(angle));
   left[2].x = LONG(p1.x + xs*cos(angle));
   left[2].y = LONG(p1.y + xs*sin(angle));

   right[0].x = p2.x;
   right[0].y = p2.y;
   right[1].x = LONG(p2.x + ys*sin(angle));
   right[1].y = LONG(p2.y - ys*cos(angle));
   right[2].x = LONG(p2.x - xs*cos(angle));
   right[2].y = LONG(p2.y - xs*sin(angle));
}

void CBridgeSectionCutDisplayImpl::Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC,COLORREF color,const WBFL::Geometry::Point2d& userLoc) const
{
   CPoint p1,p2;
   GetSectionCutPointsInLogicalSpace(pDO,userLoc,p1,p2);

   CPen   pen(PS_SOLID,2,color);
   CPen* old_pen = pDC->SelectObject(&pen);
   CBrush brush(color);
   CBrush* old_brush = pDC->SelectObject(&brush);

   pDC->MoveTo(p1);
   pDC->LineTo(p2);

   // arrows
   POINT left[3], right[3];
   GetArrowHeadPoints(pDO,userLoc,p1,p2,left,right);

   pDC->Polygon(left,  3);
   pDC->Polygon(right, 3);

   pDC->SelectObject(old_pen);
   pDC->SelectObject(old_brush);
}

void CBridgeSectionCutDisplayImpl::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgeSectionCutDisplayImpl::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
   auto roadway = m_pRoadway.lock();

   Float64 pos =  m_pCutLocation->GetCurrentCutLocation();

   CComPtr<IDirection> direction;
   roadway->GetBearing(pos,&direction);

   CComPtr<IPoint2d> point;
   roadway->GetPoint(pos,0.00,direction,pgsTypes::pcGlobal,&point);

   point->Offset(offset.Dx(), offset.Dy());

   Float64 station, alignment_offset;
   roadway->GetStationAndOffset(pgsTypes::pcGlobal,point,&station,&alignment_offset);

   PutPosition(station);
}

void CBridgeSectionCutDisplayImpl::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CBridgeSectionCutDisplayImpl::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   ASSERT(FALSE); 
}

bool CBridgeSectionCutDisplayImpl::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   if (pDO->IsSelected())
   {
      m_pCutLocation->ShowCutDlg();
      return true;
   }
   else
   {
      return false;
   }
}

bool CBridgeSectionCutDisplayImpl::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   auto list = pDO->GetDisplayList();
   auto dispMgr = list->GetDisplayMgr();

   dispMgr->SelectObject(pDO,true);

   // d&d task
   auto factory = dispMgr->GetTaskFactory();
   auto task = factory->CreateLocalDragDropTask(dispMgr,point);
   dispMgr->SetTask(task);

   return true;
}

bool CBridgeSectionCutDisplayImpl::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeSectionCutDisplayImpl::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}


bool CBridgeSectionCutDisplayImpl::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeSectionCutDisplayImpl::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeSectionCutDisplayImpl::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CBridgeSectionCutDisplayImpl::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   return false;
}

bool CBridgeSectionCutDisplayImpl::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   switch(nChar)
   {
      case VK_RETURN:
         m_pCutLocation->ShowCutDlg();
         return true;
      break;
   }

   return false;
}

bool CBridgeSectionCutDisplayImpl::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   return false;
}

void CBridgeSectionCutDisplayImpl::PutPosition(Float64 pos)
{
   m_pCutLocation->CutAt(pos);
}

void CBridgeSectionCutDisplayImpl::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   m_pFrame->ClearSelection();
}

void CBridgeSectionCutDisplayImpl::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

UINT CBridgeSectionCutDisplayImpl::Format()
{
   return ms_Format;
}

bool CBridgeSectionCutDisplayImpl::PrepareForDrag(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,std::shared_ptr<WBFL::DManip::iDragDataSink> pSink)
{
   // Create a place to store the drag data for this object
   pSink->CreateFormat(ms_Format);

   CWinThread* thread = ::AfxGetThread( );
   DWORD threadid = thread->m_nThreadID;

   pSink->Write(ms_Format,&threadid,sizeof(DWORD));
   pSink->Write(ms_Format,&m_pFrame,sizeof(CBridgeModelViewChildFrame*));
   pSink->Write(ms_Format,&m_Color,sizeof(COLORREF));
   pSink->Write(ms_Format,&m_pRoadway,sizeof(IRoadway));
   pSink->Write(ms_Format,&m_pBridge,sizeof(IBridge));
   pSink->Write(ms_Format,&m_pCutLocation,sizeof(iCutLocation*));

   return TRUE;
}

void CBridgeSectionCutDisplayImpl::OnDrop(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,std::shared_ptr<WBFL::DManip::iDragDataSource> pSource)
{
   // Tell the source we are about to read from our format
   pSource->PrepareFormat(ms_Format);

   CWinThread* thread = ::AfxGetThread( );
   DWORD threadid = thread->m_nThreadID;

   DWORD threadl;
   pSource->Read(ms_Format,&threadl,sizeof(DWORD));
   ATLASSERT(threadid == threadl);
   pSource->Read(ms_Format,&m_pFrame,sizeof(CBridgeModelViewChildFrame*));
   pSource->Read(ms_Format,&m_Color,sizeof(COLORREF));
   pSource->Read(ms_Format,&m_pRoadway,sizeof(IRoadway));
   pSource->Read(ms_Format,&m_pBridge ,sizeof(IBridge));
   pSource->Read(ms_Format,&m_pCutLocation,sizeof(iCutLocation*));
}

