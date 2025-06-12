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
#include "PGSuperApp.h"
#include "PGSuperColors.h"
#include "SupportDrawStrategyImpl.h"

#include <PsgLib\PierData2.h>

#include <DManip/PointDisplayObject.h>
#include <DManip/DisplayMgr.h>

using namespace WBFL::DManip;


// make our symbols 1/4" in size
static const long SSIZE = 1440 * 1/4; // (twips)


CSupportDrawStrategyImpl::CSupportDrawStrategyImpl(const CPierData2* pPier)
{
   m_pPier = pPier;
}

void CSupportDrawStrategyImpl::Draw(std::shared_ptr<const iPointDisplayObject> pDO,CDC* pDC) const
{
   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();

   COLORREF pen_color;
   COLORREF brush_color;

   if ( pDO->IsSelected() )
   {
      pen_color = pDispMgr->GetSelectionLineColor();
      brush_color = pen_color;
   }
   else
   {
      pen_color = PIER_BORDER_COLOR;
      brush_color = PIER_FILL_COLOR;
   }

   auto pos = pDO->GetPosition();
   Draw(pDO,pDC,pen_color,brush_color,pos);
}

void CSupportDrawStrategyImpl::DrawHighlight(std::shared_ptr<const iPointDisplayObject> pDO,CDC* pDC,bool bHighlite) const
{
   Draw(pDO,pDC);
}

void CSupportDrawStrategyImpl::DrawDragImage(std::shared_ptr<const iPointDisplayObject> pDO,CDC* pDC, std::shared_ptr<const iCoordinateMap> map, const POINT& dragStart, const POINT& dragPoint) const
{
   m_CachePoint = map->LPtoWP(dragPoint.x, dragPoint.y);
   Draw(pDO,pDC,PIER_BORDER_COLOR,PIER_FILL_COLOR,m_CachePoint);
}

WBFL::Geometry::Rect2d CSupportDrawStrategyImpl::GetBoundingBox(std::shared_ptr<const iPointDisplayObject> pDO) const
{
   auto point = pDO->GetPosition();
   auto [px, py] = point.GetLocation();

   auto map = pDO->GetDisplayList()->GetDisplayMgr()->GetCoordinateMap();

   Float64 wid,hgt;
   GetWSymbolSize(map, &wid, &hgt);

   return { (px - wid), (py - hgt), (px + wid), (py + hgt) };
}

void CSupportDrawStrategyImpl::GetWSymbolSize(std::shared_ptr<const iCoordinateMap> pMap, Float64* psx, Float64* psy) const
{

   Float64 xo,yo;
   pMap->TPtoWP(0,0,&xo,&yo);
   Float64 x2,y2;
   pMap->TPtoWP(SSIZE,SSIZE,&x2,&y2);

   *psx = fabs(x2-xo)/2.0;
   *psy = fabs(y2-yo)/2.0;
}

void CSupportDrawStrategyImpl::GetLSymbolSize(std::shared_ptr<const iCoordinateMap> pMap, long* psx, long* psy) const
{
   long xo,yo;
   pMap->TPtoLP(0,0,&xo,&yo);
   long x2,y2;
   pMap->TPtoLP(SSIZE,SSIZE,&x2,&y2);

   *psx = abs(x2-xo)/2;
   *psy = abs(y2-yo)/2;
}


void CSupportDrawStrategyImpl::DrawGround(CDC* pDC, long cx, long cy, long wid, long hgt) const
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

void CSupportDrawStrategyImpl::DrawFixedSupport(CDC* pDC, long cx, long cy, long wid, long hgt) const
{
   // pinned support
   DrawGround(pDC, cx, cy+hgt, wid, hgt);

   // circle
   RECT rect;
   rect.top = cy;
   rect.bottom = cy+hgt;
   rect.left = cx-wid/2;
   rect.right = cx+wid/2;
   pDC->Rectangle(&rect);
}

void CSupportDrawStrategyImpl::DrawPinnedSupport(CDC* pDC, long cx, long cy, long wid, long hgt) const
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

   //// top circle
   //long es=wid/5;
   //pDC->Ellipse(cx-es, cy-es, cx+es, cy+es);
}

void CSupportDrawStrategyImpl::DrawRollerSupport(CDC* pDC, long cx, long cy, long wid, long hgt) const
{
   // pinned support
   DrawGround(pDC, cx, cy+hgt, wid, hgt);

   // circle
   RECT rect;
   rect.top = cy;
   rect.bottom = cy+hgt;
   rect.left = cx-wid/2;
   rect.right = cx+wid/2;
   pDC->Ellipse(&rect);
}

void CSupportDrawStrategyImpl::Draw(std::shared_ptr<const iPointDisplayObject> pDO,CDC* pDC,COLORREF outline_color,COLORREF fill_color,const WBFL::Geometry::Point2d& loc) const
{
   auto map = pDO->GetDisplayList()->GetDisplayMgr()->GetCoordinateMap();

   long wid,hgt;
   GetLSymbolSize(map, &wid, &hgt);

   long topx,topy; // location of top
   map->WPtoLP(loc,&topx,&topy);

   CPen pen(PS_SOLID,1,outline_color);
   CPen* pOldPen = pDC->SelectObject(&pen);

   CBrush brush(fill_color);
   CBrush* pOldBrush = pDC->SelectObject(&brush);

#pragma Reminder("UPDATE: need to draw physical pier if modeled")
   if ( m_pPier->IsBoundaryPier() )
   {
      pgsTypes::BoundaryConditionType connectionType = m_pPier->GetBoundaryConditionType();
      if ( connectionType == pgsTypes::bctRoller )
      {
         DrawRollerSupport(pDC,topx,topy,wid,hgt);
      }
      else if ( connectionType == pgsTypes::bctHinge || connectionType == pgsTypes::bctContinuousAfterDeck || connectionType == pgsTypes::bctContinuousBeforeDeck )
      {
         DrawPinnedSupport(pDC,topx,topy,wid,hgt);
      }
      else if ( connectionType == pgsTypes::bctIntegralAfterDeck  || connectionType == pgsTypes::bctIntegralBeforeDeck )
      {
         DrawFixedSupport(pDC,topx,topy,wid,hgt);
      }
      else if ( connectionType == pgsTypes::bctIntegralAfterDeckHingeBack || connectionType == pgsTypes::bctIntegralBeforeDeckHingeBack )
      {
         DrawPinnedSupport(pDC,topx-wid/2,topy,wid,hgt);
         DrawFixedSupport(pDC,topx+wid/2,topy,wid,hgt);
      }
      else if ( connectionType == pgsTypes::bctIntegralAfterDeckHingeAhead || connectionType == pgsTypes::bctIntegralBeforeDeckHingeAhead )
      {
         DrawFixedSupport(pDC,topx+wid/2,topy,wid,hgt);
         DrawPinnedSupport(pDC,topx-wid/2,topy,wid,hgt);
      }
      else
      {
         ATLASSERT(false); // is there a new connection type?
         DrawPinnedSupport(pDC,topx,topy,wid,hgt);
      }
   }
   else
   {
      pgsTypes::PierSegmentConnectionType connectionType = m_pPier->GetSegmentConnectionType();
      if ( connectionType == pgsTypes::psctContinousClosureJoint || connectionType == pgsTypes::psctContinuousSegment )
      {
         DrawRollerSupport(pDC,topx,topy,wid,hgt);
      }
      else
      {
         DrawFixedSupport(pDC,topx,topy,wid,hgt);
      }
   }

   pDC->SelectObject(pOldPen);
   pDC->SelectObject(pOldBrush);
}


