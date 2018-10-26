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

#include "PGSuperAppPlugin\stdafx.h"
#include "BridgeSectionCutDisplayImpl.h"
#include "BridgeModelViewChildFrame.h"
#include <IFace\Alignment.h>
#include <IFace\Bridge.h>
#include "mfcdual.h"
#include <MathEx.h>
#include <MfcTools\MfcTools.h> 

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// height of section cut above/below girder
static const Float64 SSIZE = 1440 * 3/8; // (twips)


UINT CBridgeSectionCutDisplayImpl::ms_Format = ::RegisterClipboardFormat(_T("BridgeSectionCutData"));

CBridgeSectionCutDisplayImpl::CBridgeSectionCutDisplayImpl():
m_pCutLocation(NULL),
m_Color(RGB(0,0,220)),
m_pFrame(0)
{
   EnableAutomation ();

   m_CachePoint.CoCreateInstance(CLSID_Point2d);
}

CBridgeSectionCutDisplayImpl::~CBridgeSectionCutDisplayImpl()
{
}

BEGIN_MESSAGE_MAP(CBridgeSectionCutDisplayImpl, CCmdTarget)
	//{{AFX_MSG_MAP(CBridgeSectionCutDisplayImpl)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_INTERFACE_MAP(CBridgeSectionCutDisplayImpl,CCmdTarget)
   INTERFACE_PART(CBridgeSectionCutDisplayImpl,IID_iDrawPointStrategy,DrawPointStrategy)
   INTERFACE_PART(CBridgeSectionCutDisplayImpl,IID_iBridgeSectionCutDrawStrategy,Strategy)
//   INTERFACE_PART(CBridgeSectionCutDisplayImpl,IID_iSectionCutEvents,Events)
   INTERFACE_PART(CBridgeSectionCutDisplayImpl,IID_iDisplayObjectEvents,DisplayObjectEvents)
   INTERFACE_PART(CBridgeSectionCutDisplayImpl,IID_iDragData,DragData)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CBridgeSectionCutDisplayImpl,DrawPointStrategy);
DELEGATE_CUSTOM_INTERFACE(CBridgeSectionCutDisplayImpl,Strategy);
//DELEGATE_CUSTOM_INTERFACE(CBridgeSectionCutDisplayImpl,Events);
DELEGATE_CUSTOM_INTERFACE(CBridgeSectionCutDisplayImpl,DisplayObjectEvents);
DELEGATE_CUSTOM_INTERFACE(CBridgeSectionCutDisplayImpl,DragData);

// This goes in the source code file
 // Note: ClassWizard looks for these comments:
 BEGIN_DISPATCH_MAP(CBridgeSectionCutDisplayImpl, CCmdTarget)
     //{{AFX_DISPATCH_MAP(AClassWithAutomation)
        // NOTE - the ClassWizard will add and remove mapping macros here.
     //}}AFX_DISPATCH_MAP
 END_DISPATCH_MAP()
 


STDMETHODIMP_(void) CBridgeSectionCutDisplayImpl::XStrategy::Init(CBridgeModelViewChildFrame* pFrame,iPointDisplayObject* pDO, IRoadway* pRoadway,IBridge* pBridge, iCutLocation* pCutLoc)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,Strategy);

   pThis->m_pFrame = pFrame;
   pThis->m_pRoadway = pRoadway;
   pThis->m_pBridge = pBridge;
   pThis->m_pCutLocation = pCutLoc;

   Float64 pos = pThis->m_pCutLocation->GetCurrentCutLocation();

   CComPtr<IPoint2d> pnt;
   pnt.CoCreateInstance(CLSID_Point2d);
   pnt->put_X(pos);
   pnt->put_Y( 0.0 );
   pDO->SetPosition(pnt, FALSE, FALSE);

}


STDMETHODIMP_(void) CBridgeSectionCutDisplayImpl::XStrategy::SetColor(COLORREF color)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,Strategy);
   pThis->m_Color = color;
}


STDMETHODIMP_(void) CBridgeSectionCutDisplayImpl::XDrawPointStrategy::Draw(iPointDisplayObject* pDO,CDC* pDC)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DrawPointStrategy);

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

STDMETHODIMP_(void) CBridgeSectionCutDisplayImpl::XDrawPointStrategy::DrawHighlite(iPointDisplayObject* pDO,CDC* pDC,BOOL bHighlite)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DrawPointStrategy);
   Draw(pDO,pDC);
}

STDMETHODIMP_(void) CBridgeSectionCutDisplayImpl::XDrawPointStrategy::DrawDragImage(iPointDisplayObject* pDO,CDC* pDC, iCoordinateMap* map, const CPoint& dragStart, const CPoint& dragPoint)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DrawPointStrategy);

   Float64 wx, wy;
   map->LPtoWP(dragPoint.x, dragPoint.y, &wx, &wy);
   map->WPtoMP(wx, wy, &wx, &wy);
   pThis->m_CachePoint->put_X(wx);
   pThis->m_CachePoint->put_Y(wy);

   pThis->Draw(pDO,pDC,RGB(150,150,150),pThis->m_CachePoint);
}


STDMETHODIMP_(void) CBridgeSectionCutDisplayImpl::XDrawPointStrategy::GetBoundingBox(iPointDisplayObject* pDO, IRect2d** rect)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DrawPointStrategy);

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

void CBridgeSectionCutDisplayImpl::GetBoundingBox(iPointDisplayObject* pDO, Float64 position, 
                                            Float64* top, Float64* left, Float64* right, Float64* bottom)
{
   CComPtr<IPoint2d> pos;
   pDO->GetPosition(&pos);

   CComPtr<IPoint2d> p1,p2;
   GetSectionCutPointsInWorldSpace(pDO,pos,&p1,&p2);

   Float64 x1,y1,x2,y2;
   p1->get_X(&x1);
   p1->get_Y(&y1);
   p2->get_X(&x2);
   p2->get_Y(&y2);

   POINT left_arrow[3], right_arrow[3];
   GetArrowHeadPoints(pDO,pos,left_arrow,right_arrow);

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);
   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);
   CComPtr<iCoordinateMap> pMap;
   pDispMgr->GetCoordinateMap(&pMap);

   Float64 lx, ly, rx, ry;
   pMap->LPtoWP(left_arrow[1].x, left_arrow[1].y,  &lx,&ly);
   pMap->LPtoWP(right_arrow[1].x,right_arrow[1].y, &rx,&ry);

   *top    = _cpp_max( _cpp_max(y1,y2), _cpp_max(ly,ry) );
   *bottom = _cpp_min( _cpp_min(y1,y2), _cpp_min(ly,ry) );
   *left   = _cpp_min( _cpp_min(x1,x2), _cpp_min(lx,rx) );
   *right  = _cpp_max( _cpp_max(x1,x2), _cpp_max(lx,rx) );
}

void CBridgeSectionCutDisplayImpl::GetSectionCutPointsInWorldSpace(iPointDisplayObject* pDO,IPoint2d* userLoc,IPoint2d** p1,IPoint2d** p2)
{
   Float64 station, offset;
   m_pRoadway->GetStationAndOffset(userLoc,&station,&offset);
   
   CComPtr<IDirection> normal;
   m_pRoadway->GetBearingNormal(station,&normal);

   Float64 start_station = m_pBridge->GetPierStation(0);
   Float64 distFromStartOfBridge = station - start_station;

   Float64 left  = m_pBridge->GetLeftSlabEdgeOffset(distFromStartOfBridge);
   Float64 right = m_pBridge->GetRightSlabEdgeOffset(distFromStartOfBridge);

   m_pRoadway->GetPoint(station, left, normal,p1);
   m_pRoadway->GetPoint(station, right,normal,p2);

   Float64 x1,y1;
   (*p1)->get_X(&x1);
   (*p1)->get_Y(&y1);

   Float64 x2,y2;
   (*p2)->get_X(&x2);
   (*p2)->get_Y(&y2);

   Float64 dx = x2 - x1;
   Float64 dy = y2 - y1;

   Float64 extension_factor = 0.1;

   (*p1)->Offset(-extension_factor*dx,-extension_factor*dy);
   (*p2)->Offset( extension_factor*dx, extension_factor*dy);

   // map to world space
   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);
   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);

   CComPtr<iCoordinateMap> map;
   pDispMgr->GetCoordinateMap(&map);

   (*p1)->get_X(&x1);
   (*p1)->get_Y(&y1);

   (*p2)->get_X(&x2);
   (*p2)->get_Y(&y2);

   map->MPtoWP(x1,y1,&x1,&y1);
   map->MPtoWP(x2,y2,&x2,&y2);

   (*p1)->Move(x1,y1);
   (*p2)->Move(x2,y2);
}

void CBridgeSectionCutDisplayImpl::GetSectionCutPointsInLogicalSpace(iPointDisplayObject* pDO,IPoint2d* userLoc,POINT& p1,POINT& p2)
{
   CComPtr<IPoint2d> left_point,right_point;
   GetSectionCutPointsInWorldSpace(pDO,userLoc,&left_point,&right_point);

   CComPtr<iDisplayList> pDL;
   pDO->GetDisplayList(&pDL);
   CComPtr<iDisplayMgr> pDispMgr;
   pDL->GetDisplayMgr(&pDispMgr);
   CComPtr<iCoordinateMap> pMap;
   pDispMgr->GetCoordinateMap(&pMap);

   long x1,y1, x2,y2;
   pMap->WPtoLP(left_point,&x1,&y1);
   pMap->WPtoLP(right_point,&x2,&y2);

   p1.x = x1;
   p1.y = y1;

   p2.x = x2;
   p2.y = y2;
}

void CBridgeSectionCutDisplayImpl::GetArrowHeadPoints(iPointDisplayObject* pDO,IPoint2d* userLoc,POINT* left,POINT* right)
{
   CPoint p1,p2;
   GetSectionCutPointsInLogicalSpace(pDO,userLoc,p1,p2);
   GetArrowHeadPoints(pDO,userLoc,p1,p2,left,right);
}

void CBridgeSectionCutDisplayImpl::GetArrowHeadPoints(iPointDisplayObject* pDO,IPoint2d* userLoc,CPoint p1,CPoint p2,POINT* left,POINT* right)
{
   // arrows
   Float64 dx,dy;
   dx = p2.x - p1.x;
   dy = p2.y - p1.y;

   Float64 angle = atan2(dy,dx);

   Float64 xs,ys;
   xs = 10;
   ys = 20;
   left[0].x = p1.x;
   left[0].y = p1.y;
   left[1].x = LONG(p1.x + ys*sin(angle));
   left[1].y = LONG(p1.y - ys*cos(angle));
   left[2].x = LONG(p1.x + xs*cos(angle));
   left[2].y = LONG(p1.y + xs*sin(angle));

   right[0].x = p2.x;
   right[0].y = p2.y;
   right[1].x = LONG(p2.x + ys*sin(angle));
   right[1].y = LONG(p2.y - ys*cos(angle));
   right[2].x = LONG(p2.x - xs*cos(angle));
   right[2].y = LONG(p2.y - xs*sin(angle));
}

void CBridgeSectionCutDisplayImpl::Draw(iPointDisplayObject* pDO,CDC* pDC,COLORREF color,IPoint2d* userLoc)
{
   CPoint p1,p2;
   GetSectionCutPointsInLogicalSpace(pDO,userLoc,p1,p2);

   CPen   pen(PS_SOLID,2,color);
   CPen* old_pen = pDC->SelectObject(&pen);
   CBrush brush(color);
   CBrush* old_brush = pDC->SelectObject(&brush);

   pDC->MoveTo(p1);
   pDC->LineTo(p2);

   // arrows
   POINT left[3], right[3];
   GetArrowHeadPoints(pDO,userLoc,p1,p2,left,right);

   pDC->Polygon(left,  3);
   pDC->Polygon(right, 3);

   pDC->SelectObject(old_pen);
   pDC->SelectObject(old_brush);
}

STDMETHODIMP_(void) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DisplayObjectEvents);
}


STDMETHODIMP_(void) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DisplayObjectEvents);

   Float64 pos =  pThis->m_pCutLocation->GetCurrentCutLocation();

   CComPtr<IDirection> direction;
   pThis->m_pRoadway->GetBearing(pos,&direction);

   CComPtr<IPoint2d> point;
   pThis->m_pRoadway->GetPoint(pos,0.00,direction,&point);

   point->OffsetEx(offset);

   Float64 station, alignment_offset;
   pThis->m_pRoadway->GetStationAndOffset(point,&station,&alignment_offset);

   pThis->PutPosition(station);
}

STDMETHODIMP_(void) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DisplayObjectEvents);
}

STDMETHODIMP_(void) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnCopied(iDisplayObject* pDO)
{
   ASSERT(FALSE); 
}

STDMETHODIMP_(bool) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DisplayObjectEvents);

   pThis->m_pCutLocation->ShowCutDlg();

   return true;
}

STDMETHODIMP_(bool) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   CComPtr<iDisplayList> list;
   pDO->GetDisplayList(&list);

   CComPtr<iDisplayMgr> dispMgr;
   list->GetDisplayMgr(&dispMgr);

   dispMgr->SelectObject(pDO,true);

   // d&d task
   CComPtr<iTaskFactory> factory;
   dispMgr->GetTaskFactory(&factory);
   CComPtr<iTask> task;
   factory->CreateLocalDragDropTask(dispMgr,point,&task);
   dispMgr->SetTask(task);

   return true;
}

STDMETHODIMP_(bool) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
}


STDMETHODIMP_(bool) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
  
}

STDMETHODIMP_(bool) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DisplayObjectEvents);

   switch(nChar)
   {
      case VK_RETURN:
         pThis->m_pCutLocation->ShowCutDlg();
         return true;
      break;
   }

   return false;
}

STDMETHODIMP_(bool) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DisplayObjectEvents);

   return false;
}

void CBridgeSectionCutDisplayImpl::PutPosition(Float64 pos)
{
   m_pCutLocation->CutAt(pos);
}


STDMETHODIMP_(void) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DisplayObjectEvents);
   pThis->m_pFrame->ClearSelection();
}

STDMETHODIMP_(void) CBridgeSectionCutDisplayImpl::XDisplayObjectEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DisplayObjectEvents);
}

STDMETHODIMP_(UINT) CBridgeSectionCutDisplayImpl::XDragData::Format()
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DragData);
   return ms_Format;
}

STDMETHODIMP_(BOOL) CBridgeSectionCutDisplayImpl::XDragData::PrepareForDrag(iDisplayObject* pDO,iDragDataSink* pSink)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DragData);

   // Create a place to store the drag data for this object
   pSink->CreateFormat(ms_Format);

   CWinThread* thread = ::AfxGetThread( );
   DWORD threadid = thread->m_nThreadID;

   pSink->Write(ms_Format,&threadid,sizeof(DWORD));
   pSink->Write(ms_Format,&pThis->m_pFrame,sizeof(CBridgeModelViewChildFrame*));
   pSink->Write(ms_Format,&pThis->m_Color,sizeof(COLORREF));
   pSink->Write(ms_Format,&pThis->m_pRoadway,sizeof(IRoadway));
   pSink->Write(ms_Format,&pThis->m_pBridge,sizeof(IBridge));
   pSink->Write(ms_Format,&pThis->m_pCutLocation,sizeof(iCutLocation*));

   return TRUE;
}

STDMETHODIMP_(void) CBridgeSectionCutDisplayImpl::XDragData::OnDrop(iDisplayObject* pDO,iDragDataSource* pSource)
{
   METHOD_PROLOGUE(CBridgeSectionCutDisplayImpl,DragData);

   // Tell the source we are about to read from our format
   pSource->PrepareFormat(ms_Format);

   CWinThread* thread = ::AfxGetThread( );
   DWORD threadid = thread->m_nThreadID;

   DWORD threadl;
   pSource->Read(ms_Format,&threadl,sizeof(DWORD));
   ATLASSERT(threadid == threadl);
   pSource->Read(ms_Format,&pThis->m_pFrame,sizeof(CBridgeModelViewChildFrame*));
   pSource->Read(ms_Format,&pThis->m_Color,sizeof(COLORREF));
   pSource->Read(ms_Format,&pThis->m_pRoadway,sizeof(IRoadway));
   pSource->Read(ms_Format,&pThis->m_pBridge ,sizeof(IBridge));
   pSource->Read(ms_Format,&pThis->m_pCutLocation,sizeof(iCutLocation*));

}

