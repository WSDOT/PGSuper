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
#include "TxDOTOptionalDesignDoc.h"
#include "TogaSupportDrawStrategyImpl.h"
#include <DManip/PointDisplayObject.h>
#include <DManip/DisplayMgr.h>

// make our symbols 3/8" in size
static const long SSIZE = 1440 * 3/8; // (twips)


CTogaSupportDrawStrategyImpl::CTogaSupportDrawStrategyImpl(CTxDOTOptionalDesignDoc* pDoc)
{
   m_pDoc = pDoc;
}

void CTogaSupportDrawStrategyImpl::Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC) const
{
   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();

   COLORREF color;

   if ( pDO->IsSelected() )
      color = pDispMgr->GetSelectionLineColor();
   else
      color = RGB(140,70,0);

   auto pos = pDO->GetPosition();
   Draw(pDO,pDC,color,pos);
}

void CTogaSupportDrawStrategyImpl::DrawHighlight(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC, bool bHighlite) const
{
   Draw(pDO,pDC);
}

void CTogaSupportDrawStrategyImpl::DrawDragImage(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC, std::shared_ptr<const WBFL::DManip::iCoordinateMap> map, const POINT& dragStart, const POINT& dragPoint) const
{
   m_CachePoint = map->LPtoWP(dragPoint.x, dragPoint.y);
   Draw(pDO, pDC, RGB(255,0,0), m_CachePoint);
}

WBFL::Geometry::Rect2d CTogaSupportDrawStrategyImpl::GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO) const
{
   auto point = pDO->GetPosition();
   auto [px, py] = point.GetLocation();

   auto map = pDO->GetDisplayList()->GetDisplayMgr()->GetCoordinateMap();

   Float64 wid, hgt;
   GetWSymbolSize(map, &wid, &hgt);

   return { (px - wid), (py - hgt), (px + wid), (py + hgt) };
}

void CTogaSupportDrawStrategyImpl::GetWSymbolSize(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, Float64* psx, Float64* psy) const
{

   Float64 xo,yo;
   pMap->TPtoWP(0,0,&xo,&yo);
   Float64 x2,y2;
   pMap->TPtoWP(SSIZE,SSIZE,&x2,&y2);

   *psx = fabs(x2-xo)/2.0;
   *psy = fabs(y2-yo)/2.0;
}

void CTogaSupportDrawStrategyImpl::GetLSymbolSize(std::shared_ptr<const WBFL::DManip::iCoordinateMap> pMap, long* psx, long* psy) const
{
   long xo,yo;
   pMap->TPtoLP(0,0,&xo,&yo);
   long x2,y2;
   pMap->TPtoLP(SSIZE,SSIZE,&x2,&y2);

   *psx = abs(x2-xo)/2;
   *psy = abs(y2-yo)/2;
}


void DrawGround(CDC* pDC, long cx, long cy, long wid, long hgt)
{
   // base
   pDC->MoveTo(cx-wid,cy);
   pDC->LineTo(cx+wid,cy);

   // tics
   long num_tics=6;
   long tic_size = (2*hgt)/(num_tics+2);
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


void DrawPinnedSupport(CDC* pDC, long cx, long cy, long wid, long hgt)
{
      // pinned support
      DrawGround(pDC, cx, cy+hgt, wid, hgt);

      // triangle
      POINT points[4];
      points[0].x = cx;
      points[0].y = cy;
      points[1].x = cx-wid/2; 
      points[1].y = cy+hgt;
      points[2].x = cx+wid/2;
      points[2].y = cy+hgt;
      points[3].x = cx;
      points[3].y = cy;
      pDC->Polygon(points,4);

      // tip circle
      long es=wid/5;
      pDC->Ellipse(cx-es, cy-es, cx+es, cy+es);
}


void CTogaSupportDrawStrategyImpl::Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC,COLORREF color, const WBFL::Geometry::Point2d& loc) const
{
   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();
   auto pMap = pDispMgr->GetCoordinateMap();

   long wid,hgt;
   GetLSymbolSize(pMap, &wid, &hgt);

   long topx,topy; // location of top
   pMap->WPtoLP(loc,&topx,&topy);

   CPen pen(PS_SOLID,1,color);
   CPen* pOldPen = pDC->SelectObject(&pen);

   CBrush brush(color);
   CBrush* pOldBrush = pDC->SelectObject(&brush);

   DrawPinnedSupport( pDC, topx, topy, wid, hgt);

   pDC->SelectObject(pOldPen);
   pDC->SelectObject(pOldBrush);
}


