///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include "TogaSectionCutDisplayImpl.h"
#include "mfcdual.h"
#include <MathEx.h>
#include <MfcTools\MfcTools.h> 
#include <PGSuperColors.h>
#include <IFace\Bridge.h>
#include "TxDOTOptionalDesignDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// height of section cut above/below girder
static const Uint32 SSIZE = 1440 * 3/8; // (twips)


UINT CTogaSectionCutDisplayImpl::ms_Format = ::RegisterClipboardFormat(_T("TogaSectionCutData"));

CTogaSectionCutDisplayImpl::CTogaSectionCutDisplayImpl():
m_pCutLocation(NULL),
m_Color(CUT_COLOR)
{
   EnableAutomation ();

   m_CachePoint.CoCreateInstance(CLSID_Point2d);
}

CTogaSectionCutDisplayImpl::~CTogaSectionCutDisplayImpl()
{
}

BEGIN_MESSAGE_MAP(CTogaSectionCutDisplayImpl, CCmdTarget)
	//{{AFX_MSG_MAP(CTogaSectionCutDisplayImpl)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_INTERFACE_MAP(CTogaSectionCutDisplayImpl,CCmdTarget)
   INTERFACE_PART(CTogaSectionCutDisplayImpl,IID_iDrawPointStrategy,DrawPointStrategy)
   INTERFACE_PART(CTogaSectionCutDisplayImpl,IID_iTogaSectionCutDrawStrategy,Strategy)
   INTERFACE_PART(CTogaSectionCutDisplayImpl,IID_iDisplayObjectEvents,DisplayObjectEvents)
   INTERFACE_PART(CTogaSectionCutDisplayImpl,IID_iDragData,DragData)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CTogaSectionCutDisplayImpl,DrawPointStrategy);
DELEGATE_CUSTOM_INTERFACE(CTogaSectionCutDisplayImpl,Strategy);
DELEGATE_CUSTOM_INTERFACE(CTogaSectionCutDisplayImpl,DisplayObjectEvents);
DELEGATE_CUSTOM_INTERFACE(CTogaSectionCutDisplayImpl,DragData);

// This goes in the source code file
 // Note: ClassWizard looks for these comments:
 BEGIN_DISPATCH_MAP(CTogaSectionCutDisplayImpl, CCmdTarget)
     //{{AFX_DISPATCH_MAP(AClassWithAutomation)
        // NOTE - the ClassWizard will add and remove mapping macros here.
     //}}AFX_DISPATCH_MAP
 END_DISPATCH_MAP()
 


STDMETHODIMP_(void) CTogaSectionCutDisplayImpl::XStrategy::Init(iPointDisplayObject* pDO, IBroker* pBroker, SpanIndexType spanIdx,GirderIndexType gdrIdx, iCutLocation* pCutLoc)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,Strategy);

   pThis->m_pBroker = pBroker;
   pThis->m_SpanIdx = spanIdx;
   pThis->m_GirderIdx = gdrIdx;

   GET_IFACE2(pThis->m_pBroker,IBridge,pBridge);
   pThis->m_gdrLength = pBridge->GetGirderLength(spanIdx,gdrIdx);

   pThis->m_pCutLocation = pCutLoc;

   Float64 pos = pThis->m_pCutLocation->GetCurrentCutLocation();

   CComPtr<IPoint2d> pnt;
   pnt.CoCreateInstance(CLSID_Point2d);
   pnt->put_X(pos);
   pnt->put_Y( 0.0 );
   pDO->SetPosition(pnt, FALSE, FALSE);
}


STDMETHODIMP_(void) CTogaSectionCutDisplayImpl::XStrategy::SetColor(COLORREF color)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,Strategy);
   pThis->m_Color = color;
}


STDMETHODIMP_(void) CTogaSectionCutDisplayImpl::XDrawPointStrategy::Draw(iPointDisplayObject* pDO,CDC* pDC)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DrawPointStrategy);

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);

   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);

   COLORREF color;

   if ( pDO->IsSelected() )
      color = pDispMgr->GetSelectionLineColor();
   else
      color = pThis->m_Color;

   CComPtr<IPoint2d> pos;
   pDO->GetPosition(&pos);

   pThis->Draw(pDO,pDC,color,pos);
}

STDMETHODIMP_(void) CTogaSectionCutDisplayImpl::XDrawPointStrategy::DrawHighlite(iPointDisplayObject* pDO,CDC* pDC,BOOL bHighlite)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DrawPointStrategy);
   Draw(pDO,pDC);
}

STDMETHODIMP_(void) CTogaSectionCutDisplayImpl::XDrawPointStrategy::DrawDragImage(iPointDisplayObject* pDO,CDC* pDC, iCoordinateMap* map, const CPoint& dragStart, const CPoint& dragPoint)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DrawPointStrategy);

   Float64 wx, wy;
   map->LPtoWP(dragPoint.x, dragPoint.y, &wx, &wy);
   pThis->m_CachePoint->put_X(wx);
   pThis->m_CachePoint->put_Y(wy);

   pThis->Draw(pDO,pDC,SELECTED_OBJECT_LINE_COLOR,pThis->m_CachePoint);
}


STDMETHODIMP_(void) CTogaSectionCutDisplayImpl::XDrawPointStrategy::GetBoundingBox(iPointDisplayObject* pDO, IRect2d** rect)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DrawPointStrategy);

   CComPtr<IPoint2d> pnt;
   pDO->GetPosition(&pnt);
   
   Float64 xpos;
   pnt->get_X(&xpos);

   Float64 top, left, right, bottom;
   pThis->GetBoundingBox(pDO, xpos, &top, &left, &right, &bottom);


   CComPtr<IRect2d> bounding_box;
   bounding_box.CoCreateInstance(CLSID_Rect2d);

   bounding_box->put_Top(top);
   bounding_box->put_Bottom(bottom);
   bounding_box->put_Left(left);
   bounding_box->put_Right(right);

   (*rect) = bounding_box;
   (*rect)->AddRef();
}

void CTogaSectionCutDisplayImpl::GetBoundingBox(iPointDisplayObject* pDO, Float64 position, 
                                            Float64* top, Float64* left, Float64* right, Float64* bottom)
{
   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);
   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);
   CComPtr<iCoordinateMap> pMap;
   pDispMgr->GetCoordinateMap(&pMap);

   // height of cut above/below girder
   Float64 xo,yo;
   pMap->TPtoWP(0,0,&xo,&yo);
   Float64 x2,y2;
   pMap->TPtoWP(SSIZE,SSIZE,&x2,&y2);

   Float64 width = (x2-xo)/2.0;  // width is half of height
   Float64 height = y2-yo;

   *top    = height;
   *bottom = -(GetGirderHeight(position) + height);
   *left   = position;
   *right  = *left + width;
}

void CTogaSectionCutDisplayImpl::Draw(iPointDisplayObject* pDO,CDC* pDC,COLORREF color,IPoint2d* userLoc)
{
   Float64 x;
   userLoc->get_X(&x);
   x = ::ForceIntoRange(0.0,x,m_gdrLength);

   Float64 wtop, wleft, wright, wbottom;
   GetBoundingBox(pDO, x, &wtop, &wleft, &wright, &wbottom);

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);
   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);
   CComPtr<iCoordinateMap> pMap;
   pDispMgr->GetCoordinateMap(&pMap);

   long ltop, lleft, lright, lbottom;
   pMap->WPtoLP(wleft, wtop, &lleft, &ltop);
   pMap->WPtoLP(wright, wbottom, &lright,  &lbottom);

   CRect rect(lleft, ltop, lright, lbottom);
   rect.NormalizeRect();
   rect.DeflateRect(1,1); // deflate slightly to prevent artifacts when dragging

   CPen   pen(PS_SOLID,2,color);
   CPen* old_pen = pDC->SelectObject(&pen);
   CBrush brush(color);
   CBrush* old_brush = pDC->SelectObject(&brush);

   LONG xs = rect.Width() /2;
   LONG dy = xs/2;
   rect.DeflateRect(0,dy);

   pDC->MoveTo(rect.right, rect.top);
   pDC->LineTo(rect.left, rect.top);
   pDC->LineTo(rect.left, rect.bottom);
   pDC->LineTo(rect.right, rect.bottom);

   // arrows
   POINT points[3];
   points[0].x = rect.right - xs;
   points[0].y = rect.top + dy;
   points[1].x = rect.right;
   points[1].y = rect.top;
   points[2].x = rect.right - xs;
   points[2].y = rect.top - dy;
   pDC->Polygon(points, 3);

   points[0].x = rect.right - xs;
   points[0].y = rect.bottom + dy;
   points[1].x = rect.right;
   points[1].y = rect.bottom;
   points[2].x = rect.right - xs;
   points[2].y = rect.bottom - dy;
   pDC->Polygon(points, 3);

   pDC->SelectObject(old_pen);
   pDC->SelectObject(old_brush);
}

Float64 CTogaSectionCutDisplayImpl::GetGirderHeight(Float64 distFromStartOfGirder)
{
   GET_IFACE(IGirder,pGirder);
   GET_IFACE(IPointOfInterest,pPOI);

   pgsPointOfInterest poi = pPOI->GetPointOfInterest(pgsTypes::CastingYard,m_SpanIdx,m_GirderIdx,distFromStartOfGirder);
   if ( poi.GetID() == INVALID_ID )
      poi.SetDistFromStart(distFromStartOfGirder);

   Float64 gdrHeight = pGirder->GetHeight(poi);
   return gdrHeight;
}

STDMETHODIMP_(void) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DisplayObjectEvents);

   iPointDisplayObject* ppdo = dynamic_cast<iPointDisplayObject*>(pDO);

   if (ppdo)
   {
      Float64 pos = pThis->m_pCutLocation->GetCurrentCutLocation();
   
      CComPtr<IPoint2d> pnt;
      pnt.CoCreateInstance(CLSID_Point2d);
      pnt->put_X(pos);
      pnt->put_Y( 0.0 );
      ppdo->SetPosition(pnt, TRUE, FALSE);
   }
   else
      ATLASSERT(0);
}


STDMETHODIMP_(void) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DisplayObjectEvents);

   Float64 pos =  pThis->m_pCutLocation->GetCurrentCutLocation();

   Float64 xoff;
   offset->get_Dx(&xoff);

   pos += xoff;

   pThis->PutPosition(pos);
}

STDMETHODIMP_(void) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DisplayObjectEvents);

   ASSERT(FALSE); 
}

STDMETHODIMP_(void) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnCopied(iDisplayObject* pDO)
{
   ASSERT(FALSE); 
}

STDMETHODIMP_(bool) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DisplayObjectEvents);

   pThis->m_pCutLocation->ShowCutDlg();

   return true;
}

STDMETHODIMP_(bool) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   CComPtr<iDisplayList> list;
   pDO->GetDisplayList(&list);

   CComPtr<iDisplayMgr> dispMgr;
   list->GetDisplayMgr(&dispMgr);

   // If control key is pressed, don't clear current selection
   // (i.e. we want multi-select)
   BOOL bMultiSelect = nFlags & MK_CONTROL ? TRUE : FALSE;
/*
   if ( bMultiSelect )
   {
      // clear all selected objects that aren't part of the load list
      dispMgr->ClearSelectedObjectsByList(LOAD_LIST,atByID,FALSE);
   }
*/
   dispMgr->SelectObject(pDO,!bMultiSelect);

   // d&d task
   CComPtr<iTaskFactory> factory;
   dispMgr->GetTaskFactory(&factory);
   CComPtr<iTask> task;
   factory->CreateLocalDragDropTask(dispMgr,point,&task);
   dispMgr->SetTask(task);

   return true;
}

STDMETHODIMP_(bool) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
}


STDMETHODIMP_(bool) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
  
}

STDMETHODIMP_(bool) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DisplayObjectEvents);
   Float64 pos = pThis->m_pCutLocation->GetCurrentCutLocation();
   Float64 xoff = BinarySign(zDelta)*1.0;
   pos += xoff * pThis->m_gdrLength/100.0;
   pThis->PutPosition(pos);
   return true;
}

STDMETHODIMP_(bool) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DisplayObjectEvents);

   switch(nChar)
   {
   case VK_RIGHT:
   case VK_LEFT:
      {
         Float64 pos =  pThis->m_pCutLocation->GetCurrentCutLocation();

         Float64 xoff = nChar==VK_RIGHT ? 1.0 : -1.0;
         pos += xoff * pThis->m_gdrLength/100.0;

         pThis->PutPosition(pos);

      }
      break;

   case VK_RETURN:
      pThis->m_pCutLocation->ShowCutDlg();
      break;
   }

   return true;
}

STDMETHODIMP_(bool) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DisplayObjectEvents);

   if ( pDO->IsSelected() )
   {
      CComPtr<iDisplayList> pList;
      pDO->GetDisplayList(&pList);

      CComPtr<iDisplayMgr> pDispMgr;
      pList->GetDisplayMgr(&pDispMgr);

      CDisplayView* pView = pDispMgr->GetView();
   }

   return false;
}

void CTogaSectionCutDisplayImpl::PutPosition(Float64 pos)
{
   if (pos < 0.0)
      pos = 0.0;
   else if (pos>m_gdrLength)
      pos = m_gdrLength;
      
   m_pCutLocation->CutAt(pos);
}


STDMETHODIMP_(void) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnSelect(iDisplayObject* pDO)
{

}

STDMETHODIMP_(void) CTogaSectionCutDisplayImpl::XDisplayObjectEvents::OnUnselect(iDisplayObject* pDO)
{
}

STDMETHODIMP_(UINT) CTogaSectionCutDisplayImpl::XDragData::Format()
{
   return ms_Format;
}

STDMETHODIMP_(BOOL) CTogaSectionCutDisplayImpl::XDragData::PrepareForDrag(iDisplayObject* pDO,iDragDataSink* pSink)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DragData);

   // Create a place to store the drag data for this object
   pSink->CreateFormat(ms_Format);

   CWinThread* thread = ::AfxGetThread( );
   DWORD threadid = thread->m_nThreadID;

   pSink->Write(ms_Format,&threadid,sizeof(DWORD));
   pSink->Write(ms_Format,&pThis->m_Color,sizeof(COLORREF));
   pSink->Write(ms_Format,&pThis->m_pBroker,sizeof(IBroker*));
   pSink->Write(ms_Format,&pThis->m_SpanIdx,sizeof(SpanIndexType));
   pSink->Write(ms_Format,&pThis->m_GirderIdx,sizeof(GirderIndexType));
   pSink->Write(ms_Format,&pThis->m_gdrLength,sizeof(Float64));
   pSink->Write(ms_Format,&pThis->m_pCutLocation,sizeof(iCutLocation*));

   return TRUE;
}

STDMETHODIMP_(void) CTogaSectionCutDisplayImpl::XDragData::OnDrop(iDisplayObject* pDO,iDragDataSource* pSource)
{
   METHOD_PROLOGUE(CTogaSectionCutDisplayImpl,DragData);

   // Tell the source we are about to read from our format
   pSource->PrepareFormat(ms_Format);

   CWinThread* thread = ::AfxGetThread( );
   DWORD threadid = thread->m_nThreadID;

   DWORD threadl;
   pSource->Read(ms_Format,&threadl,sizeof(DWORD));

   ATLASSERT(threadid == threadl);

   pSource->Read(ms_Format,&pThis->m_Color,sizeof(COLORREF));
   pSource->Read(ms_Format,&pThis->m_pBroker,sizeof(IBroker*));
   pSource->Read(ms_Format,&pThis->m_SpanIdx,sizeof(SpanIndexType));
   pSource->Read(ms_Format,&pThis->m_GirderIdx,sizeof(GirderIndexType));
   pSource->Read(ms_Format,&pThis->m_gdrLength,sizeof(Float64));
   pSource->Read(ms_Format,&pThis->m_pCutLocation,sizeof(iCutLocation*));

}

