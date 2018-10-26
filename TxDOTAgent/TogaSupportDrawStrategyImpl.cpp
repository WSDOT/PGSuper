///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
#include "mfcdual.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// make our symbols 3/8" in size
static const long SSIZE = 1440 * 3/8; // (twips)


CTogaSupportDrawStrategyImpl::CTogaSupportDrawStrategyImpl(CTxDOTOptionalDesignDoc* pDoc)
{
   m_pDoc = pDoc;

   m_CachePoint.CoCreateInstance(CLSID_Point2d);
}

BEGIN_INTERFACE_MAP(CTogaSupportDrawStrategyImpl,CCmdTarget)
   INTERFACE_PART(CTogaSupportDrawStrategyImpl,IID_iDrawPointStrategy,DrawPointStrategy)
   INTERFACE_PART(CTogaSupportDrawStrategyImpl,IID_iTogaSupportDrawStrategy,Strategy)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CTogaSupportDrawStrategyImpl,DrawPointStrategy);
DELEGATE_CUSTOM_INTERFACE(CTogaSupportDrawStrategyImpl,Strategy);

STDMETHODIMP_(void) CTogaSupportDrawStrategyImpl::XDrawPointStrategy::Draw(iPointDisplayObject* pDO,CDC* pDC)
{
   METHOD_PROLOGUE(CTogaSupportDrawStrategyImpl,DrawPointStrategy);

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);

   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);

   COLORREF color;

   if ( pDO->IsSelected() )
      color = pDispMgr->GetSelectionLineColor();
   else
      color = RGB(140,70,0);

   CComPtr<IPoint2d> pos;
   pDO->GetPosition(&pos);
   pThis->Draw(pDO,pDC,color,pos);
}

STDMETHODIMP_(void) CTogaSupportDrawStrategyImpl::XDrawPointStrategy::DrawHighlite(iPointDisplayObject* pDO,CDC* pDC,BOOL bHighlite)
{
   METHOD_PROLOGUE(CTogaSupportDrawStrategyImpl,DrawPointStrategy);
   Draw(pDO,pDC);
}

STDMETHODIMP_(void) CTogaSupportDrawStrategyImpl::XDrawPointStrategy::DrawDragImage(iPointDisplayObject* pDO,CDC* pDC, iCoordinateMap* map, const CPoint& dragStart, const CPoint& dragPoint)
{
   METHOD_PROLOGUE(CTogaSupportDrawStrategyImpl,DrawPointStrategy);

   double wx, wy;
   map->LPtoWP(dragPoint.x, dragPoint.y, &wx, &wy);
   pThis->m_CachePoint->put_X(wx);
   pThis->m_CachePoint->put_Y(wy);

   // Draw the support
   pThis->Draw(pDO,pDC,RGB(255,0,0),pThis->m_CachePoint);
}

STDMETHODIMP_(void) CTogaSupportDrawStrategyImpl::XDrawPointStrategy::GetBoundingBox(iPointDisplayObject* pDO,IRect2d** rect)
{
   METHOD_PROLOGUE(CTogaSupportDrawStrategyImpl,DrawPointStrategy);

   CComPtr<IPoint2d> point;
   pDO->GetPosition(&point);

   double px, py;
   point->get_X(&px);
   point->get_Y(&py);

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);
   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);
   CComPtr<iCoordinateMap> pMap;
   pDispMgr->GetCoordinateMap(&pMap);

   double wid,hgt;
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

void CTogaSupportDrawStrategyImpl::GetWSymbolSize(iCoordinateMap* pMap, double* psx, double* psy)
{

   double xo,yo;
   pMap->TPtoWP(0,0,&xo,&yo);
   double x2,y2;
   pMap->TPtoWP(SSIZE,SSIZE,&x2,&y2);

   *psx = fabs(x2-xo)/2.0;
   *psy = fabs(y2-yo)/2.0;
}

void CTogaSupportDrawStrategyImpl::GetLSymbolSize(iCoordinateMap* pMap, long* psx, long* psy)
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


void CTogaSupportDrawStrategyImpl::Draw(iPointDisplayObject* pDO,CDC* pDC,COLORREF color, IPoint2d* loc)
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

   CBrush brush(color);
   CBrush* pOldBrush = pDC->SelectObject(&brush);

   DrawPinnedSupport( pDC, topx, topy, wid, hgt);

   pDC->SelectObject(pOldPen);
   pDC->SelectObject(pOldBrush);
}


