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

#include "PGSuperAppPlugin\stdafx.h"
#include "resource.h"
#include "MomentLoadDrawStrategyImpl.h"
#include "mfcdual.h"
#include <IFace\Project.h> 
#include "EditMomentLoadDlg.h"
#include <PgsExt\InsertDeleteLoad.h>
#include <MathEx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// height of maximum loads
static const Uint32 SSIZE = 1440 * 1/2; // (twips)


UINT CMomentLoadDrawStrategyImpl::ms_Format = ::RegisterClipboardFormat(_T("MomentLoadDrawStrategyImpl"));

CMomentLoadDrawStrategyImpl::CMomentLoadDrawStrategyImpl()
{
   m_CachePoint.CoCreateInstance(CLSID_Point2d);
}

BEGIN_INTERFACE_MAP(CMomentLoadDrawStrategyImpl,CCmdTarget)
   INTERFACE_PART(CMomentLoadDrawStrategyImpl,IID_iDrawPointStrategy,DrawPointStrategy)
   INTERFACE_PART(CMomentLoadDrawStrategyImpl,IID_iMomentLoadDrawStrategy,Strategy)
   INTERFACE_PART(CMomentLoadDrawStrategyImpl,IID_iDisplayObjectEvents,DisplayObjectEvents)
   INTERFACE_PART(CMomentLoadDrawStrategyImpl,IID_iGevEditLoad,EditLoad)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CMomentLoadDrawStrategyImpl,DrawPointStrategy);
DELEGATE_CUSTOM_INTERFACE(CMomentLoadDrawStrategyImpl,Strategy);
DELEGATE_CUSTOM_INTERFACE(CMomentLoadDrawStrategyImpl,DisplayObjectEvents);
DELEGATE_CUSTOM_INTERFACE(CMomentLoadDrawStrategyImpl,EditLoad);


void CMomentLoadDrawStrategyImpl::XStrategy::Init(iPointDisplayObject* pDO, IBroker* pBroker, CMomentLoadData load,
                                                 IndexType loadIndex, Float64 girderDepth, Float64 spanLength, 
                                                 Float64 maxMagnitude, COLORREF color)
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl,Strategy);

   pThis->m_Load = load;
   pThis->m_LoadIndex = loadIndex;
   pThis->m_pBroker = pBroker;
   pThis->m_Color = color;
   pThis->m_MaxMagnitude = maxMagnitude;
   pThis->m_GirderDepth = girderDepth;
   pThis->m_SpanLength = spanLength;
}


STDMETHODIMP_(void) CMomentLoadDrawStrategyImpl::XDrawPointStrategy::Draw(iPointDisplayObject* pDO,CDC* pDC)
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl,DrawPointStrategy);

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);

   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);

   COLORREF color = pDO->IsSelected() ? pDispMgr->GetSelectionLineColor() : pThis->m_Color;

   CComPtr<IPoint2d> pos;
   pDO->GetPosition(&pos);

   pThis->Draw(pDO,pDC,color, pos);
}

STDMETHODIMP_(void) CMomentLoadDrawStrategyImpl::XDrawPointStrategy::DrawHighlite(iPointDisplayObject* pDO,CDC* pDC,BOOL bHighlite)
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl,DrawPointStrategy);
   Draw(pDO,pDC);
}

STDMETHODIMP_(void) CMomentLoadDrawStrategyImpl::XDrawPointStrategy::DrawDragImage(iPointDisplayObject* pDO,CDC* pDC, iCoordinateMap* map, const CPoint& dragStart, const CPoint& dragPoint)
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl,DrawPointStrategy);

   double wx, wy;
   map->LPtoWP(dragPoint.x, dragPoint.y, &wx, &wy);
   pThis->m_CachePoint->put_X(wx);
   pThis->m_CachePoint->put_Y(wy);

   pThis->Draw(pDO,pDC,RGB(255,0,0), pThis->m_CachePoint);
}

STDMETHODIMP_(void) CMomentLoadDrawStrategyImpl::XDrawPointStrategy::GetBoundingBox(iPointDisplayObject* pDO,IRect2d** rect)
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl,DrawPointStrategy);

   CComPtr<IPoint2d> point;
   pDO->GetPosition(&point);

   double xpos;
   point->get_X(&xpos);

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);

   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);

   CComPtr<iCoordinateMap> pMap;
   pDispMgr->GetCoordinateMap(&pMap);

   double diameter;
   pThis->GetWSymbolSize(pMap, &diameter);

   CComPtr<IRect2d> bounding_box;
   bounding_box.CoCreateInstance(CLSID_Rect2d);

   bounding_box->put_Top(3*diameter/4);
   bounding_box->put_Left(xpos-diameter/2);
   bounding_box->put_Bottom(-3*diameter/4);
   bounding_box->put_Right(xpos+diameter/2);

   (*rect) = bounding_box;
   (*rect)->AddRef();
}


void CMomentLoadDrawStrategyImpl::Draw(iPointDisplayObject* pDO,CDC* pDC,COLORREF color, IPoint2d* loc)
{
   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);

   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);

   CComPtr<iCoordinateMap> pMap;
   pDispMgr->GetCoordinateMap(&pMap);

   // bottom of load is at top of girder
   Float64 wx, wyb;
   loc->get_X(&wx);
   wyb = 0;

   long cxb,cyb;
   pMap->WPtoLP(wx, wyb, &cxb, &cyb);

   // diameter of moment symbol
   Uint32 diameter;
   GetLSymbolSize(pMap, &diameter);
   Uint32 radius = diameter / 2;

   // line style and width
   bool funky_load = m_Load.m_Magnitude==0.0 || wx>m_SpanLength;

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


STDMETHODIMP_(void) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnChanged(iDisplayObject* pDO)
{
}

STDMETHODIMP_(void) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   ASSERT(FALSE); // Points must be dropped on a member. This event should never occur
}

STDMETHODIMP_(void) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl,DisplayObjectEvents);
}

STDMETHODIMP_(void) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnCopied(iDisplayObject* pDO)
{
   // No big deal...
}

STDMETHODIMP_(bool) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl,DisplayObjectEvents);
   pThis->EditLoad();
   return true;
}

STDMETHODIMP_(bool) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   CComPtr<iDisplayList> list;
   pDO->GetDisplayList(&list);

   CComPtr<iDisplayMgr> dispMgr;
   list->GetDisplayMgr(&dispMgr);

   dispMgr->SelectObject(pDO, true);

   return true;
}

STDMETHODIMP_(bool) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE_(CMomentLoadDrawStrategyImpl,DisplayObjectEvents);
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

STDMETHODIMP_(bool) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl,DisplayObjectEvents);
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

STDMETHODIMP_(bool) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl,DisplayObjectEvents);

   return false;
}

STDMETHODIMP_(void) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnSelect(iDisplayObject* pDO)
{
}

STDMETHODIMP_(void) CMomentLoadDrawStrategyImpl::XDisplayObjectEvents::OnUnselect(iDisplayObject* pDO)
{
}


STDMETHODIMP_(HRESULT) CMomentLoadDrawStrategyImpl::XEditLoad::EditLoad()
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl, EditLoad);
   pThis->EditLoad();
   return S_OK;
}

STDMETHODIMP_(HRESULT) CMomentLoadDrawStrategyImpl::XEditLoad::DeleteLoad()
{
   METHOD_PROLOGUE(CMomentLoadDrawStrategyImpl, EditLoad);
   pThis->DeleteLoad();
   return S_OK;
}

void CMomentLoadDrawStrategyImpl::GetTSymbolSize(iCoordinateMap* pMap, Uint32* pd)
{
   Float64 frac = max( (fabs(m_Load.m_Magnitude)/m_MaxMagnitude), 1.0/6.0); // minimum symbol size
   if (frac!=0.0)
      *pd = Uint32(frac * Float64(SSIZE));
   else
      *pd = SSIZE;

//   *psx = SSIZE/6.;
}

void CMomentLoadDrawStrategyImpl::GetWSymbolSize(iCoordinateMap* pMap, double* pd)
{
   Uint32 d;
   GetTSymbolSize(pMap, &d);

   double xo,yo;
   pMap->TPtoWP(0,0,&xo,&yo);

   double x2,y2;
   pMap->TPtoWP(d,d,&x2,&y2);

   *pd = x2-xo;
   //*psy = y2-yo;
}

void CMomentLoadDrawStrategyImpl::GetLSymbolSize(iCoordinateMap* pMap, Uint32* pd)
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


void CMomentLoadDrawStrategyImpl::EditLoad()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   GET_IFACE( IUserDefinedLoadData, pUdl);

   CHECK(0 <= m_LoadIndex && m_LoadIndex < pUdl->GetMomentLoadCount());

   CMomentLoadData rld = pUdl->GetMomentLoad(m_LoadIndex);

	CEditMomentLoadDlg dlg(rld,m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      if (rld!=dlg.m_Load)
      {
         txnEditMomentLoad* pTxn = new txnEditMomentLoad(m_LoadIndex,rld,dlg.m_Load);
         txnTxnManager::GetInstance()->Execute(pTxn);
      }
   }
}

void CMomentLoadDrawStrategyImpl::DeleteLoad()
{
   txnDeleteMomentLoad* pTxn = new txnDeleteMomentLoad(m_LoadIndex);
   txnTxnManager::GetInstance()->Execute(pTxn);
}
