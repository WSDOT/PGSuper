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
#include "PGSuperApp.h"
#include "PGSuperColors.h"
#include "TemporarySupportDrawStrategyImpl.h"

#include <DManip/PointDisplayObject.h>
#include <DManip/DisplayList.h>
#include <DManip/DisplayMgr.h>

using namespace WBFL::DManip;

// make our symbols 1/4" in size
static const long SSIZE = 1440 * 1/4; // (twips)

CTemporarySupportDrawStrategyImpl::CTemporarySupportDrawStrategyImpl(pgsTypes::TemporarySupportType supportType,Float64 leftBrgOffset,Float64 rightBrgOffset)
{
   m_SupportType = supportType;

   m_LeftBrgOffset  = leftBrgOffset;
   m_RightBrgOffset = rightBrgOffset;
}

void CTemporarySupportDrawStrategyImpl::Draw(std::shared_ptr<const iPointDisplayObject> pDO,CDC* pDC) const
{
   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();

   COLORREF color;

   if ( pDO->IsSelected() )
   {
      color = pDispMgr->GetSelectionLineColor();
   }
   else
   {
      color = m_SupportType == pgsTypes::StrongBack ? SB_FILL_COLOR : TS_FILL_COLOR;
   }

   const auto& pos = pDO->GetPosition();

   if ( m_SupportType == pgsTypes::StrongBack )
   {
      DrawStrongBack(pDO,pDC,color,pos);
   }
   else
   {
      Draw(pDO,pDC,color,pos);
   }
}

void CTemporarySupportDrawStrategyImpl::DrawHighlight(std::shared_ptr<const iPointDisplayObject> pDO,CDC* pDC,bool bHighlite) const
{
   Draw(pDO,pDC);
}

void CTemporarySupportDrawStrategyImpl::DrawDragImage(std::shared_ptr<const iPointDisplayObject> pDO,CDC* pDC, std::shared_ptr<const iCoordinateMap> map, const POINT& dragStart, const POINT& dragPoint) const
{
   m_CachePoint = map->LPtoWP(dragPoint.x, dragPoint.y);

   // Draw the support
   Draw(pDO,pDC,PIER_FILL_COLOR,m_CachePoint);
}

WBFL::Geometry::Rect2d CTemporarySupportDrawStrategyImpl::GetBoundingBox(std::shared_ptr<const iPointDisplayObject> pDO) const
{
   const auto& point = pDO->GetPosition();

   auto [px, py] = point.GetLocation();

   auto map = pDO->GetDisplayList()->GetDisplayMgr()->GetCoordinateMap();

   Float64 wid = m_LeftBrgOffset + m_RightBrgOffset;
   Float64 hgt = 2*wid;

   return { (px - wid), (py - hgt), (px + wid), (py + hgt) };
}

void CTemporarySupportDrawStrategyImpl::GetLSymbolSize(std::shared_ptr<const iCoordinateMap> pMap, long* psx, long* psy) const
{
   long xo,yo;
   pMap->TPtoLP(0,0,&xo,&yo);
   long x2,y2;
   pMap->TPtoLP(SSIZE,2*SSIZE,&x2,&y2);
   
   long minX = abs(x2-xo)/2;
   long minY = abs(y2-yo)/2;

   pMap->WPtoLP(0,0,&xo,&yo);
   Float64 w = m_LeftBrgOffset + m_RightBrgOffset;
   pMap->WPtoLP(w,2*w,&x2,&y2);

   *psx = Max(minX,abs(x2-xo));
   *psy = Max(minY,abs(y2-yo));
}


void CTemporarySupportDrawStrategyImpl::DrawGround(CDC* pDC, long cx, long cy, long wid, long hgt) const
{
   // base
   pDC->MoveTo(cx-wid,cy);
   pDC->LineTo(cx+wid,cy);

   // tics
   long num_tics=6;
   long tic_size = (2*wid)/(num_tics+2);
   long yt = cy;
   long yb = yt + tic_size;
   long xl = cx-wid+tic_size;
   for (long i=0; i<=num_tics+1; i++)
   {
      pDC->MoveTo(xl, yt);
      pDC->LineTo(xl-tic_size, yb);

      xl+=tic_size;
   }
}


void CTemporarySupportDrawStrategyImpl::DrawTowerSupport(CDC* pDC, long cx, long cy, long wid, long hgt) const
{
   DrawGround(pDC, cx, cy+hgt, wid, hgt);

   // rectangle
   POINT points[5];
   points[0].x = cx-wid/2;
   points[0].y = cy;

   points[1].x = cx-wid/2; 
   points[1].y = cy+hgt;

   points[2].x = cx+wid/2;
   points[2].y = cy+hgt;

   points[3].x = cx+wid/2;
   points[3].y = cy;

   points[4].x = cx-wid/2;
   points[4].y = cy;
   pDC->Polygon(points,5);
}


void CTemporarySupportDrawStrategyImpl::Draw(std::shared_ptr<const iPointDisplayObject> pDO,CDC* pDC,COLORREF color, const WBFL::Geometry::Point2d& loc) const
{
   auto map = pDO->GetDisplayList()->GetDisplayMgr()->GetCoordinateMap();

   long wid,hgt;
   GetLSymbolSize(map, &wid, &hgt);

   long topx,topy; // location of top
   map->WPtoLP(loc,&topx,&topy);

   CPen pen(PS_SOLID,1,color);
   CPen* pOldPen = pDC->SelectObject(&pen);

   CBrush brush(HS_DIAGCROSS,color);
   CBrush* pOldBrush = pDC->SelectObject(&brush);

   DrawTowerSupport( pDC, topx, topy, wid, hgt);

   pDC->SelectObject(pOldPen);
   pDC->SelectObject(pOldBrush);
}

void CTemporarySupportDrawStrategyImpl::DrawStrongBack(std::shared_ptr<const iPointDisplayObject> pDO,CDC* pDC,COLORREF color,const WBFL::Geometry::Point2d& loc) const
{
   auto map = pDO->GetDisplayList()->GetDisplayMgr()->GetCoordinateMap();

   CPen pen(PS_SOLID,2,color);
   CPen* pOldPen = pDC->SelectObject(&pen);

   auto [x, y] = loc.GetLocation();

   long wx1,wy1;
   long wx2,wy2;

   Float64 strong_back_height_above_girder = WBFL::Units::ConvertToSysUnits(1.0,WBFL::Units::Measure::Feet);

   map->WPtoLP(x-m_LeftBrgOffset,y,&wx1,&wy1);
   map->WPtoLP(x-m_LeftBrgOffset,strong_back_height_above_girder,&wx2,&wy2);

   pDC->MoveTo(wx1,wy1);
   pDC->LineTo(wx2,wy2);

   map->WPtoLP(x+m_RightBrgOffset,strong_back_height_above_girder,&wx1,&wy1);
   pDC->LineTo(wx1,wy1);

   map->WPtoLP(x+m_RightBrgOffset,y,&wx2,&wy2);
   pDC->LineTo(wx2,wy2);

   pDC->SelectObject(pOldPen);
}