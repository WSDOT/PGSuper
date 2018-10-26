///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperColors.h"
#include "PGSuperAppPlugin\TemporarySupportDrawStrategyImpl.h"
#include "mfcdual.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// make our symbols 1/4" in size
static const long SSIZE = 1440 * 1/4; // (twips)


CTemporarySupportDrawStrategyImpl::CTemporarySupportDrawStrategyImpl(pgsTypes::TemporarySupportType supportType,Float64 leftBrgOffset,Float64 rightBrgOffset)
{
   m_CachePoint.CoCreateInstance(CLSID_Point2d);
   m_SupportType = supportType;

   m_LeftBrgOffset  = leftBrgOffset;
   m_RightBrgOffset = rightBrgOffset;
}

BEGIN_INTERFACE_MAP(CTemporarySupportDrawStrategyImpl,CCmdTarget)
   INTERFACE_PART(CTemporarySupportDrawStrategyImpl,IID_iDrawPointStrategy,DrawPointStrategy)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CTemporarySupportDrawStrategyImpl,DrawPointStrategy);

STDMETHODIMP_(void) CTemporarySupportDrawStrategyImpl::XDrawPointStrategy::Draw(iPointDisplayObject* pDO,CDC* pDC)
{
   METHOD_PROLOGUE(CTemporarySupportDrawStrategyImpl,DrawPointStrategy);

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);

   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);

   COLORREF color;

   if ( pDO->IsSelected() )
   {
      color = pDispMgr->GetSelectionLineColor();
   }
   else
   {
      color = pThis->m_SupportType == pgsTypes::StrongBack ? SB_FILL_COLOR : TS_FILL_COLOR;
   }

   CComPtr<IPoint2d> pos;
   pDO->GetPosition(&pos);

   if ( pThis->m_SupportType == pgsTypes::StrongBack )
   {
      pThis->DrawStrongBack(pDO,pDC,color,pos);
   }
   else
   {
      pThis->Draw(pDO,pDC,color,pos);
   }
}

STDMETHODIMP_(void) CTemporarySupportDrawStrategyImpl::XDrawPointStrategy::DrawHighlite(iPointDisplayObject* pDO,CDC* pDC,BOOL bHighlite)
{
   METHOD_PROLOGUE(CTemporarySupportDrawStrategyImpl,DrawPointStrategy);
   Draw(pDO,pDC);
}

STDMETHODIMP_(void) CTemporarySupportDrawStrategyImpl::XDrawPointStrategy::DrawDragImage(iPointDisplayObject* pDO,CDC* pDC, iCoordinateMap* map, const CPoint& dragStart, const CPoint& dragPoint)
{
   METHOD_PROLOGUE(CTemporarySupportDrawStrategyImpl,DrawPointStrategy);

   Float64 wx, wy;
   map->LPtoWP(dragPoint.x, dragPoint.y, &wx, &wy);
   pThis->m_CachePoint->put_X(wx);
   pThis->m_CachePoint->put_Y(wy);

   // Draw the support
   pThis->Draw(pDO,pDC,PIER_FILL_COLOR,pThis->m_CachePoint);
}

STDMETHODIMP_(void) CTemporarySupportDrawStrategyImpl::XDrawPointStrategy::GetBoundingBox(iPointDisplayObject* pDO,IRect2d** rect)
{
   METHOD_PROLOGUE(CTemporarySupportDrawStrategyImpl,DrawPointStrategy);

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

   Float64 wid = pThis->m_LeftBrgOffset + pThis->m_RightBrgOffset;
   Float64 hgt = 2*wid;
#pragma Reminder("UPDATE: is the correct height used for strongbacks")
   CComPtr<IRect2d> bounding_box;
   bounding_box.CoCreateInstance(CLSID_Rect2d);

   bounding_box->put_Left(px-wid);
   bounding_box->put_Bottom(py-hgt);
   bounding_box->put_Right(px+wid);
   bounding_box->put_Top(py+hgt);

   (*rect) = bounding_box;
   (*rect)->AddRef();
}

void CTemporarySupportDrawStrategyImpl::GetLSymbolSize(iCoordinateMap* pMap, long* psx, long* psy)
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


void CTemporarySupportDrawStrategyImpl::DrawGround(CDC* pDC, long cx, long cy, long wid, long hgt)
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


void CTemporarySupportDrawStrategyImpl::DrawTowerSupport(CDC* pDC, long cx, long cy, long wid, long hgt)
{
   DrawGround(pDC, cx, cy+hgt, wid, hgt);

   // rectagle
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


void CTemporarySupportDrawStrategyImpl::Draw(iPointDisplayObject* pDO,CDC* pDC,COLORREF color, IPoint2d* loc)
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

   CPen pen(PS_SOLID,1,color);
   CPen* pOldPen = pDC->SelectObject(&pen);

   CBrush brush(HS_DIAGCROSS,color);
   CBrush* pOldBrush = pDC->SelectObject(&brush);

   DrawTowerSupport( pDC, topx, topy, wid, hgt);

   pDC->SelectObject(pOldPen);
   pDC->SelectObject(pOldBrush);
}

void CTemporarySupportDrawStrategyImpl::DrawStrongBack(iPointDisplayObject* pDO,CDC* pDC,COLORREF color,IPoint2d* loc)
{
   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);

   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);

   CComPtr<iCoordinateMap> pMap;
   pDispMgr->GetCoordinateMap(&pMap);

   CPen pen(PS_SOLID,2,color);
   CPen* pOldPen = pDC->SelectObject(&pen);

   Float64 x,y;
   loc->Location(&x,&y);

   long wx1,wy1;
   long wx2,wy2;

   Float64 strong_back_height_above_girder = ::ConvertToSysUnits(1.0,unitMeasure::Feet);

   pMap->WPtoLP(x-m_LeftBrgOffset,y,&wx1,&wy1);
   pMap->WPtoLP(x-m_LeftBrgOffset,strong_back_height_above_girder,&wx2,&wy2);

   pDC->MoveTo(wx1,wy1);
   pDC->LineTo(wx2,wy2);

   pMap->WPtoLP(x+m_RightBrgOffset,strong_back_height_above_girder,&wx1,&wy1);
   pDC->LineTo(wx1,wy1);

   pMap->WPtoLP(x+m_RightBrgOffset,y,&wx2,&wy2);
   pDC->LineTo(wx2,wy2);

   pDC->SelectObject(pOldPen);
}