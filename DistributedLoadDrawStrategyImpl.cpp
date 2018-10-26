///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include "PGSuperAppPlugin\resource.h"
#include "DistributedLoadDrawStrategyImpl.h"
#include "mfcdual.h"
#include <IFace\EditByUI.h> 
#include <MathEx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// height of maximum loads
static const long SSIZE = 1440 / 2; // (twips)
// arrowhead in twips;
static const long ARROW_SIZE = 100;

// utilities
void DrawArrowLine(CDC* pDC, long x, long ydatum, long yend, CSize& arrowSize);
CSize GetLArrowSize(iCoordinateMap* pMap);

UINT CDistributedLoadDrawStrategyImpl::ms_Format = ::RegisterClipboardFormat(_T("DistributedLoadDrawStrategyImpl"));

CDistributedLoadDrawStrategyImpl::CDistributedLoadDrawStrategyImpl()
{
   m_ReusablePoint.CoCreateInstance(CLSID_Point2d);
   m_ReusableRect.CoCreateInstance(CLSID_Rect2d);
}

BEGIN_INTERFACE_MAP(CDistributedLoadDrawStrategyImpl,CCmdTarget)
   INTERFACE_PART(CDistributedLoadDrawStrategyImpl,IID_iDrawPointStrategy,DrawPointStrategy)
   INTERFACE_PART(CDistributedLoadDrawStrategyImpl,IID_iDistributedLoadDrawStrategy,Strategy)
   INTERFACE_PART(CDistributedLoadDrawStrategyImpl,IID_iDisplayObjectEvents,DisplayObjectEvents)
   INTERFACE_PART(CDistributedLoadDrawStrategyImpl,IID_iGevEditLoad,EditLoad)
   INTERFACE_PART(CDistributedLoadDrawStrategyImpl,IID_iGravityWellStrategy,GravityWellStrategy)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CDistributedLoadDrawStrategyImpl,DrawPointStrategy);
DELEGATE_CUSTOM_INTERFACE(CDistributedLoadDrawStrategyImpl,Strategy);
DELEGATE_CUSTOM_INTERFACE(CDistributedLoadDrawStrategyImpl,DisplayObjectEvents);
DELEGATE_CUSTOM_INTERFACE(CDistributedLoadDrawStrategyImpl,EditLoad);
DELEGATE_CUSTOM_INTERFACE(CDistributedLoadDrawStrategyImpl,GravityWellStrategy);


void CDistributedLoadDrawStrategyImpl::XStrategy::Init(iPointDisplayObject* pDO, IBroker* pBroker, CDistributedLoadData load, CollectionIndexType loadIndex, 
                                                       Float64 loadLength, Float64 spanLength, Float64 maxMagnitude, COLORREF color)
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl,Strategy);

   pThis->m_Load = load;
   pThis->m_LoadIndex = loadIndex;
   pThis->m_pBroker = pBroker;
   pThis->m_Color = color;
   pThis->m_MaxMagnitude = maxMagnitude;
   pThis->m_SpanLength   = spanLength;
   pThis->m_LoadLength   = loadLength;

   // distance between vertical arrows
   pThis->m_ArrowSpacing = spanLength/50.0;
}


STDMETHODIMP_(void) CDistributedLoadDrawStrategyImpl::XDrawPointStrategy::Draw(iPointDisplayObject* pDO,CDC* pDC)
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl,DrawPointStrategy);

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);

   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);

   COLORREF color = pDO->IsSelected() ? pDispMgr->GetSelectionLineColor() : pThis->m_Color;

   CComPtr<IPoint2d> pos;
   pDO->GetPosition(&pos);

   pThis->Draw(pDO,pDC,color, pos);
}

STDMETHODIMP_(void) CDistributedLoadDrawStrategyImpl::XDrawPointStrategy::DrawHighlite(iPointDisplayObject* pDO,CDC* pDC,BOOL bHighlite)
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl,DrawPointStrategy);
   Draw(pDO,pDC);
}

STDMETHODIMP_(void) CDistributedLoadDrawStrategyImpl::XDrawPointStrategy::DrawDragImage(iPointDisplayObject* pDO,CDC* pDC, iCoordinateMap* map, const CPoint& dragStart, const CPoint& dragPoint)
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl,DrawPointStrategy);

   Float64 wx, wy;
   map->LPtoWP(dragPoint.x, dragPoint.y, &wx, &wy);
   pThis->m_ReusablePoint->put_X(wx);
   pThis->m_ReusablePoint->put_Y(wy);

   pThis->Draw(pDO,pDC,RGB(255,0,0),pThis->m_ReusablePoint);
}

STDMETHODIMP_(void) CDistributedLoadDrawStrategyImpl::XDrawPointStrategy::GetBoundingBox(iPointDisplayObject* pDO,IRect2d** rect)
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl,DrawPointStrategy);

   CComPtr<IPoint2d> point;
   pDO->GetPosition(&point);

   Float64 xpos;
   point->get_X(&xpos);

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);

   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);

   CComPtr<iCoordinateMap> pMap;
   pDispMgr->GetCoordinateMap(&pMap);

   Float64 wystart, wyend;
   pThis->GetWLoadHeight(pMap, &wystart, &wyend);

   Float64 top = Max(0.0,wystart, wyend);
   Float64 bot = Min(0.0,wystart, wyend);

   CComPtr<IRect2d> bounding_box;
   bounding_box.CoCreateInstance(CLSID_Rect2d);

   bounding_box->put_Top(top);
   bounding_box->put_Left(xpos);
   bounding_box->put_Bottom(bot);
   bounding_box->put_Right(xpos+pThis->m_LoadLength);

   (*rect) = bounding_box;
   (*rect)->AddRef();
}


void CDistributedLoadDrawStrategyImpl::Draw(iPointDisplayObject* pDO,CDC* pDC,COLORREF color, IPoint2d* loc)
{
   if (m_LoadLength<=0.0)
   {
      ATLASSERT(false);
      return;
   }

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);
   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);
   CComPtr<iCoordinateMap> pMap;
   pDispMgr->GetCoordinateMap(&pMap);

   // bottom of load is at top of girder
   Float64 wyb_at_start;
   wyb_at_start = 0;

   Float64 wyb_at_end;
   wyb_at_end = 0;

   Float64 wx_start;
   loc->get_X(&wx_start);

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


STDMETHODIMP_(void) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnChanged(iDisplayObject* pDO)
{
}

STDMETHODIMP_(void) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   ASSERT(FALSE); // Points must be dropped on a member. This event should never occur
}

STDMETHODIMP_(void) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl,DisplayObjectEvents);
}

STDMETHODIMP_(void) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnCopied(iDisplayObject* pDO)
{
   // No big deal...
}

STDMETHODIMP_(bool) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl,DisplayObjectEvents);
   if (pDO->IsSelected())
   {
      pThis->EditLoad();
      return true;
   }
   else
   {
      return false;
   }
}

STDMETHODIMP_(bool) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   CComPtr<iDisplayList> list;
   pDO->GetDisplayList(&list);

   CComPtr<iDisplayMgr> dispMgr;
   list->GetDisplayMgr(&dispMgr);

   dispMgr->SelectObject(pDO, true);

   return true;
}

STDMETHODIMP_(bool) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE_(CDistributedLoadDrawStrategyImpl,DisplayObjectEvents);
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComPtr<iDisplayList> list;
   pDO->GetDisplayList(&list);

   CComPtr<iDisplayMgr> dispMgr;
   list->GetDisplayMgr(&dispMgr);

   dispMgr->SelectObject(pDO, true);

   CDisplayView* view = dispMgr->GetView();
   view->ClientToScreen(&point);

   CMenu menu;
   VERIFY( menu.LoadMenu(IDR_LOADS_CTX) );
   menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x,point.y, view);

   return true;
}

STDMETHODIMP_(bool) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl,DisplayObjectEvents);
   switch(nChar)
   {
   case VK_RETURN:
      pThis->EditLoad();
      return true;

   case VK_DELETE:
      pThis->DeleteLoad();
      return true;
   }
   return false;
}

STDMETHODIMP_(bool) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl,DisplayObjectEvents);

   return false;
}

STDMETHODIMP_(void) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnSelect(iDisplayObject* pDO)
{
}

STDMETHODIMP_(void) CDistributedLoadDrawStrategyImpl::XDisplayObjectEvents::OnUnselect(iDisplayObject* pDO)
{
}


STDMETHODIMP_(HRESULT) CDistributedLoadDrawStrategyImpl::XEditLoad::EditLoad()
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl, EditLoad);
   pThis->EditLoad();
   return S_OK;
}

STDMETHODIMP_(HRESULT) CDistributedLoadDrawStrategyImpl::XEditLoad::DeleteLoad()
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl, EditLoad);
   pThis->DeleteLoad();
   return S_OK;
}

/////////////////////////////////////////////////////////
// iGravityWellStrategy Implementation
STDMETHODIMP_(void) CDistributedLoadDrawStrategyImpl::XGravityWellStrategy::GetGravityWell(iDisplayObject* pDO,CRgn* pRgn)
{
   METHOD_PROLOGUE(CDistributedLoadDrawStrategyImpl, GravityWellStrategy);

   CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> pPointDO(pDO);

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);
   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);
   CComPtr<iCoordinateMap> pMap;
   pDispMgr->GetCoordinateMap(&pMap);

   CComPtr<IPoint2d> point;
   pPointDO->GetPosition(&point);

   Float64 wxstart, wxend;
   point->get_X(&wxstart);
   wxend = wxstart + pThis->m_LoadLength;

   Float64 wystart, wyend;
   pThis->GetWLoadHeight(pMap, &wystart, &wyend);

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


void CDistributedLoadDrawStrategyImpl::GetTLoadHeight(iCoordinateMap* pMap, long* startHgt, long* endHgt)
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

void CDistributedLoadDrawStrategyImpl::GetWLoadHeight(iCoordinateMap* pMap, Float64* startHgt, Float64* endHgt)
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

void CDistributedLoadDrawStrategyImpl::GetLLoadHeight(iCoordinateMap* pMap, long* startHgt, long* endHgt)
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

CSize GetLArrowSize(iCoordinateMap* pMap)
{
   long lxo,lyo;
   pMap->TPtoLP(0,0,&lxo,&lyo);
   long lxe, lye;
   pMap->TPtoLP(ARROW_SIZE/4,ARROW_SIZE, &lxe, &lye);

   return CSize(lxe-lxo, lye-lyo);
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
