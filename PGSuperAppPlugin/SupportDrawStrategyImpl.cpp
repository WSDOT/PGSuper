///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include "mfcdual.h"

#include <PgsExt\PierData2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// make our symbols 1/4" in size
static const long SSIZE = 1440 * 1/4; // (twips)


CSupportDrawStrategyImpl::CSupportDrawStrategyImpl(const CPierData2* pPier)
{
   m_pPier = pPier;
   m_CachePoint.CoCreateInstance(CLSID_Point2d);
}

BEGIN_INTERFACE_MAP(CSupportDrawStrategyImpl,CCmdTarget)
   INTERFACE_PART(CSupportDrawStrategyImpl,IID_iDrawPointStrategy,DrawPointStrategy)
   INTERFACE_PART(CSupportDrawStrategyImpl,IID_iSupportDrawStrategy,Strategy)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CSupportDrawStrategyImpl,DrawPointStrategy);
DELEGATE_CUSTOM_INTERFACE(CSupportDrawStrategyImpl,Strategy);

STDMETHODIMP_(void) CSupportDrawStrategyImpl::XDrawPointStrategy::Draw(iPointDisplayObject* pDO,CDC* pDC)
{
   METHOD_PROLOGUE(CSupportDrawStrategyImpl,DrawPointStrategy);

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);

   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);

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

   CComPtr<IPoint2d> pos;
   pDO->GetPosition(&pos);
   pThis->Draw(pDO,pDC,pen_color,brush_color,pos);
}

STDMETHODIMP_(void) CSupportDrawStrategyImpl::XDrawPointStrategy::DrawHighlite(iPointDisplayObject* pDO,CDC* pDC,BOOL bHighlite)
{
   METHOD_PROLOGUE(CSupportDrawStrategyImpl,DrawPointStrategy);
   Draw(pDO,pDC);
}

STDMETHODIMP_(void) CSupportDrawStrategyImpl::XDrawPointStrategy::DrawDragImage(iPointDisplayObject* pDO,CDC* pDC, iCoordinateMap* map, const CPoint& dragStart, const CPoint& dragPoint)
{
   METHOD_PROLOGUE(CSupportDrawStrategyImpl,DrawPointStrategy);

   Float64 wx, wy;
   map->LPtoWP(dragPoint.x, dragPoint.y, &wx, &wy);
   pThis->m_CachePoint->put_X(wx);
   pThis->m_CachePoint->put_Y(wy);

   // Draw the support
   pThis->Draw(pDO,pDC,PIER_BORDER_COLOR,PIER_FILL_COLOR,pThis->m_CachePoint);
}

STDMETHODIMP_(void) CSupportDrawStrategyImpl::XDrawPointStrategy::GetBoundingBox(iPointDisplayObject* pDO,IRect2d** rect)
{
   METHOD_PROLOGUE(CSupportDrawStrategyImpl,DrawPointStrategy);

   CComPtr<IPoint2d> point;
   pDO->GetPosition(&point);

   Float64 px, py;
   point->get_X(&px);
   point->get_Y(&py);

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);
   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);
   CComPtr<iCoordinateMap> pMap;
   pDispMgr->GetCoordinateMap(&pMap);

   Float64 wid,hgt;
   pThis->GetWSymbolSize(pMap, &wid, &hgt);

   CComPtr<IRect2d> bounding_box;
   bounding_box.CoCreateInstance(CLSID_Rect2d);

   bounding_box->put_Left(px-wid);
   bounding_box->put_Bottom(py-hgt);
   bounding_box->put_Right(px+wid);
   bounding_box->put_Top(py+hgt);

   (*rect) = bounding_box;
   (*rect)->AddRef();
}

void CSupportDrawStrategyImpl::GetWSymbolSize(iCoordinateMap* pMap, Float64* psx, Float64* psy)
{

   Float64 xo,yo;
   pMap->TPtoWP(0,0,&xo,&yo);
   Float64 x2,y2;
   pMap->TPtoWP(SSIZE,SSIZE,&x2,&y2);

   *psx = fabs(x2-xo)/2.0;
   *psy = fabs(y2-yo)/2.0;
}

void CSupportDrawStrategyImpl::GetLSymbolSize(iCoordinateMap* pMap, long* psx, long* psy)
{
   long xo,yo;
   pMap->TPtoLP(0,0,&xo,&yo);
   long x2,y2;
   pMap->TPtoLP(SSIZE,SSIZE,&x2,&y2);

   *psx = abs(x2-xo)/2;
   *psy = abs(y2-yo)/2;
}


void CSupportDrawStrategyImpl::DrawGround(CDC* pDC, long cx, long cy, long wid, long hgt)
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

void CSupportDrawStrategyImpl::DrawFixedSupport(CDC* pDC, long cx, long cy, long wid, long hgt)
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

void CSupportDrawStrategyImpl::DrawPinnedSupport(CDC* pDC, long cx, long cy, long wid, long hgt)
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

void CSupportDrawStrategyImpl::DrawRollerSupport(CDC* pDC, long cx, long cy, long wid, long hgt)
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

void CSupportDrawStrategyImpl::Draw(iPointDisplayObject* pDO,CDC* pDC,COLORREF outline_color,COLORREF fill_color,IPoint2d* loc)
{
   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);

   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);

   CComPtr<iCoordinateMap> pMap;
   pDispMgr->GetCoordinateMap(&pMap);

   long wid,hgt;
   GetLSymbolSize(pMap, &wid, &hgt);

   long topx,topy; // location of top
   pMap->WPtoLP(loc,&topx,&topy);

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


