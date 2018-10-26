///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include "PointLoadDrawStrategyImpl.h"
#include "mfcdual.h"
#include <IFace\Project.h> 
#include "EditPointLoadDlg.h"
#include <PgsExt\InsertDeleteLoad.h>
#include "PGSuperColors.h"

#include <MathEx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// height of maximum loads
static const long SSIZE = 1440 * 3/4; // (twips)


UINT CPointLoadDrawStrategyImpl::ms_Format = ::RegisterClipboardFormat(_T("PointLoadDrawStrategyImpl"));

CPointLoadDrawStrategyImpl::CPointLoadDrawStrategyImpl()
{
   m_CachePoint.CoCreateInstance(CLSID_Point2d);
}

BEGIN_INTERFACE_MAP(CPointLoadDrawStrategyImpl,CCmdTarget)
   INTERFACE_PART(CPointLoadDrawStrategyImpl,IID_iDrawPointStrategy,DrawPointStrategy)
   INTERFACE_PART(CPointLoadDrawStrategyImpl,IID_iPointLoadDrawStrategy,Strategy)
   INTERFACE_PART(CPointLoadDrawStrategyImpl,IID_iDisplayObjectEvents,DisplayObjectEvents)
   INTERFACE_PART(CPointLoadDrawStrategyImpl,IID_iGevEditLoad,EditLoad)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CPointLoadDrawStrategyImpl,DrawPointStrategy);
DELEGATE_CUSTOM_INTERFACE(CPointLoadDrawStrategyImpl,Strategy);
DELEGATE_CUSTOM_INTERFACE(CPointLoadDrawStrategyImpl,DisplayObjectEvents);
DELEGATE_CUSTOM_INTERFACE(CPointLoadDrawStrategyImpl,EditLoad);


void CPointLoadDrawStrategyImpl::XStrategy::Init(iPointDisplayObject* pDO, IBroker* pBroker, const CPointLoadData& load,
                                                 CollectionIndexType loadIndex, Float64 spanLength, 
                                                 Float64 maxMagnitude, COLORREF color)
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl,Strategy);

   pThis->m_Load          = load;
   pThis->m_LoadIndex     = loadIndex;
   pThis->m_pBroker       = pBroker;
   pThis->m_Color         = color;
   pThis->m_MaxMagnitude  = maxMagnitude;
   pThis->m_SpanLength    = spanLength;
}


STDMETHODIMP_(void) CPointLoadDrawStrategyImpl::XDrawPointStrategy::Draw(iPointDisplayObject* pDO,CDC* pDC)
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl,DrawPointStrategy);

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);

   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);

   COLORREF color = pDO->IsSelected() ? pDispMgr->GetSelectionLineColor() : pThis->m_Color;

   CComPtr<IPoint2d> pos;
   pDO->GetPosition(&pos);

   pThis->Draw(pDO,pDC,color, pos);
}

STDMETHODIMP_(void) CPointLoadDrawStrategyImpl::XDrawPointStrategy::DrawHighlite(iPointDisplayObject* pDO,CDC* pDC,BOOL bHighlite)
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl,DrawPointStrategy);
   Draw(pDO,pDC);
}

STDMETHODIMP_(void) CPointLoadDrawStrategyImpl::XDrawPointStrategy::DrawDragImage(iPointDisplayObject* pDO,CDC* pDC, iCoordinateMap* map, const CPoint& dragStart, const CPoint& dragPoint)
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl,DrawPointStrategy);

   Float64 wx, wy;
   map->LPtoWP(dragPoint.x, dragPoint.y, &wx, &wy);
   pThis->m_CachePoint->put_X(wx);
   pThis->m_CachePoint->put_Y(wy);

   pThis->Draw(pDO,pDC,SELECTED_OBJECT_LINE_COLOR, pThis->m_CachePoint);
}

STDMETHODIMP_(void) CPointLoadDrawStrategyImpl::XDrawPointStrategy::GetBoundingBox(iPointDisplayObject* pDO,IRect2d** rect)
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl,DrawPointStrategy);

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

   Float64 height, width;
   pThis->GetWSymbolSize(pMap, &width, &height);

   CComPtr<IRect2d> bounding_box;
   bounding_box.CoCreateInstance(CLSID_Rect2d);

   bounding_box->put_Top(height);
   bounding_box->put_Left(xpos-width);
   bounding_box->put_Bottom(0);
   bounding_box->put_Right(xpos+width);

   (*rect) = bounding_box;
   (*rect)->AddRef();
}


void CPointLoadDrawStrategyImpl::Draw(iPointDisplayObject* pDO,CDC* pDC,COLORREF color, IPoint2d* loc)
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

   // height and width of load
   Float64 wid, hgt;
   GetWSymbolSize(pMap, &wid, &hgt);

   // line style and width
   Float64 location = (m_Load.m_Fractional ? m_SpanLength*m_Load.m_Location : m_Load.m_Location);
   bool funky_load = m_Load.m_Magnitude==0.0 || m_SpanLength < location;

   int nWidth    = funky_load ? 1 : 2;
   int nPenStyle = funky_load ? PS_DOT : PS_SOLID;

   CPen pen(nPenStyle,nWidth,color);
   CPen* pOldPen = pDC->SelectObject(&pen);

   // top of load
   long cxt, cyt;
   pMap->WPtoLP(wx,wyb+hgt,&cxt,&cyt);

   // draw shaft
   pDC->MoveTo(cxb,cyb);
   pDC->LineTo(cxt,cyt);

   long w,h;
   GetLSymbolSize(pMap, &w, &h);
   POINT points[3];
   // draw arrow head
   if (m_Load.m_Magnitude >= 0.0)
   {
      // down arrow
      points[0].x = cxb-w;
      points[0].y = cyb-w;
      points[1].x = cxb;
      points[1].y = cyb;
      points[2].x = cxb+w;
      points[2].y = cyb-w;
   }
   else
   {
      // up arrow
      points[0].x = cxt-w;
      points[0].y = cyt+w;
      points[1].x = cxt;
      points[1].y = cyt;
      points[2].x = cxt+w;
      points[2].y = cyt+w;
   }
 
   // have to draw like this in order to make dots symmetrical
   pDC->MoveTo(points[1]);
   pDC->LineTo(points[0]);
   pDC->MoveTo(points[1]);
   pDC->LineTo(points[2]);

   pDC->SelectObject(pOldPen);
}


STDMETHODIMP_(void) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnChanged(iDisplayObject* pDO)
{
}

STDMETHODIMP_(void) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   ASSERT(FALSE); // Points must be dropped on a member. This event should never occur
}

STDMETHODIMP_(void) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl,DisplayObjectEvents);
}

STDMETHODIMP_(void) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnCopied(iDisplayObject* pDO)
{
   // No big deal...
}

STDMETHODIMP_(bool) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl,DisplayObjectEvents);
   pThis->EditLoad();
   return true;
}

STDMETHODIMP_(bool) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   CComPtr<iDisplayList> list;
   pDO->GetDisplayList(&list);

   CComPtr<iDisplayMgr> dispMgr;
   list->GetDisplayMgr(&dispMgr);

   dispMgr->SelectObject(pDO, true);

   return true;
}

STDMETHODIMP_(bool) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE_(CPointLoadDrawStrategyImpl,DisplayObjectEvents);
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

STDMETHODIMP_(bool) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl,DisplayObjectEvents);
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

STDMETHODIMP_(bool) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl,DisplayObjectEvents);

   return false;
}

STDMETHODIMP_(void) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnSelect(iDisplayObject* pDO)
{
}

STDMETHODIMP_(void) CPointLoadDrawStrategyImpl::XDisplayObjectEvents::OnUnselect(iDisplayObject* pDO)
{
}


STDMETHODIMP_(HRESULT) CPointLoadDrawStrategyImpl::XEditLoad::EditLoad()
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl, EditLoad);
   pThis->EditLoad();
   return S_OK;
}

STDMETHODIMP_(HRESULT) CPointLoadDrawStrategyImpl::XEditLoad::DeleteLoad()
{
   METHOD_PROLOGUE(CPointLoadDrawStrategyImpl, EditLoad);
   pThis->DeleteLoad();
   return S_OK;
}

void CPointLoadDrawStrategyImpl::GetTSymbolSize(iCoordinateMap* pMap, long* psx, long* psy)
{
   Float64 frac = Max( (fabs(m_Load.m_Magnitude)/m_MaxMagnitude), 1.0/6.0); // minimum symbol size
   if (frac!=0.0)
      *psy = long(floor(frac * SSIZE));
   else
      *psy = SSIZE;

   *psx = SSIZE/6;
}

void CPointLoadDrawStrategyImpl::GetWSymbolSize(iCoordinateMap* pMap, Float64* psx, Float64* psy)
{
   long hgt, wid2;
   GetTSymbolSize(pMap, &wid2, &hgt);

   Float64 xo,yo;
   pMap->TPtoWP(0,0,&xo,&yo);
   Float64 x2,y2;
   pMap->TPtoWP(wid2,hgt,&x2,&y2);

   *psx = x2-xo;
   *psy = y2-yo;
}

void CPointLoadDrawStrategyImpl::GetLSymbolSize(iCoordinateMap* pMap, long* psx, long* psy)
{
   long hgt, wid2;
   GetTSymbolSize(pMap, &wid2, &hgt);

   long xo,yo;
   pMap->TPtoLP(0,0,&xo,&yo);
   long x2,y2;
   pMap->TPtoLP(wid2,hgt,&x2,&y2);

   *psx = x2-xo;
   *psy = y2-yo;
}


void CPointLoadDrawStrategyImpl::EditLoad()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   GET_IFACE(IUserDefinedLoadData, pUdl);

   ATLASSERT(0 <= m_LoadIndex && m_LoadIndex < pUdl->GetPointLoadCount());

   const CPointLoadData* pLoad = pUdl->GetPointLoad(m_LoadIndex);

	CEditPointLoadDlg dlg(*pLoad);
   if (dlg.DoModal() == IDOK)
   {
      if (*pLoad != dlg.m_Load)
      {
         txnEditPointLoad* pTxn = new txnEditPointLoad(m_LoadIndex,*pLoad,dlg.m_Load);
         txnTxnManager::GetInstance()->Execute(pTxn);
      }
   }
}

void CPointLoadDrawStrategyImpl::DeleteLoad()
{
   txnDeletePointLoad* pTxn = new txnDeletePointLoad(m_LoadIndex);
   txnTxnManager::GetInstance()->Execute(pTxn);
}
