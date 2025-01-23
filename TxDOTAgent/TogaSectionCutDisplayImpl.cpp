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
#include "TogaSectionCutDisplayImpl.h"
#include <MathEx.h>
#include <MfcTools\MfcTools.h> 
#include <PGSuperColors.h>
#include <IFace\Bridge.h>
#include "TxDOTOptionalDesignDoc.h"

#include <DManip/PointDisplayObject.h>
#include <DManip/DisplayMgr.h>
#include <DManip/TaskFactory.h>
#include <DManip/DisplayView.h>

// height of section cut above/below girder
static const Uint32 SSIZE = 1440 * 3/8; // (twips)


UINT CTogaSectionCutDisplayImpl::ms_Format = ::RegisterClipboardFormat(_T("TogaSectionCutData"));

CTogaSectionCutDisplayImpl::CTogaSectionCutDisplayImpl():
m_pCutLocation(nullptr),
m_Color(CUT_COLOR)
{
}

CTogaSectionCutDisplayImpl::~CTogaSectionCutDisplayImpl()
{
}

void CTogaSectionCutDisplayImpl::Init(std::shared_ptr<WBFL::DManip::iPointDisplayObject> pDO, IBroker* pBroker, const CSegmentKey& segmentKey, iCutLocation* pCutLoc)
{
   m_pBroker = pBroker;

   m_SegmentKey = segmentKey;
   
   GET_IFACE(IBridge, pBridge);
   m_gdrLength = pBridge->GetSegmentLength(segmentKey);

   m_pCutLocation = pCutLoc;

   Float64 Xgl = m_pCutLocation->GetCurrentCutLocation();

   WBFL::Geometry::Point2d pnt(Xgl, 0.0);
   pDO->SetPosition(pnt, false, false);
}

void CTogaSectionCutDisplayImpl::SetColor(COLORREF color)
{
   m_Color = color;
}

void CTogaSectionCutDisplayImpl::Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC) const
{
   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();

   COLORREF color;

   if (pDO->IsSelected())
   {
      color = pDispMgr->GetSelectionLineColor();
   }
   else
   {
      color = m_Color;
   }

   auto pos = pDO->GetPosition();

   Draw(pDO, pDC, color, pos);
}

void CTogaSectionCutDisplayImpl::DrawHighlight(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC, bool bHighlite) const
{
   Draw(pDO, pDC);
}

void CTogaSectionCutDisplayImpl::DrawDragImage(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC, std::shared_ptr<const WBFL::DManip::iCoordinateMap> map, const POINT& dragStart, const POINT& dragPoint) const
{
   Float64 wx, wy;
   map->LPtoWP(dragPoint.x, dragPoint.y, &wx, &wy);
   m_CachePoint.Move(wx, wy);

   Draw(pDO, pDC, SELECTED_OBJECT_LINE_COLOR, m_CachePoint);
}


WBFL::Geometry::Rect2d CTogaSectionCutDisplayImpl::GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO) const
{
   auto pnt = pDO->GetPosition();

   Float64 Xgl = pnt.X();

   Float64 top, left, right, bottom;
   GetBoundingBox(pDO, Xgl, &top, &left, &right, &bottom);

   return { left,bottom,right,top };
}

void CTogaSectionCutDisplayImpl::GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, Float64 Xgl,
   Float64* top, Float64* left, Float64* right, Float64* bottom) const
{
   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();
   auto pMap = pDispMgr->GetCoordinateMap();

   // height of cut above/below girder
   Float64 xo,yo;
   pMap->TPtoWP(0,0,&xo,&yo);
   Float64 x2,y2;
   pMap->TPtoWP(SSIZE,SSIZE,&x2,&y2);

   Float64 width = (x2-xo)/2.0;  // width is half of height
   Float64 height = y2-yo;

   *top    = height;
   *bottom = -(GetGirderHeight(Xgl) + height);
   *left   = Xgl;
   *right  = *left + width;
}

void CTogaSectionCutDisplayImpl::Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC, COLORREF color, const WBFL::Geometry::Point2d& userLoc) const
{
   auto x = userLoc.X();
   x = ::ForceIntoRange(0.0,x,m_gdrLength);

   Float64 wtop, wleft, wright, wbottom;
   GetBoundingBox(pDO, x, &wtop, &wleft, &wright, &wbottom);

   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();
   auto pMap = pDispMgr->GetCoordinateMap();

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

Float64 CTogaSectionCutDisplayImpl::GetGirderHeight(Float64 distFromStartOfGirder) const
{
   GET_IFACE(IGirder,pGirder);
   GET_IFACE(IPointOfInterest,pPOI);

   pgsPointOfInterest poi( pPOI->GetPointOfInterest(m_SegmentKey,distFromStartOfGirder) );
   if ( poi.GetID() < 0 )
      poi.SetDistFromStart(distFromStartOfGirder);

   Float64 gdrHeight = pGirder->GetHeight(poi);
   return gdrHeight;
}

void CTogaSectionCutDisplayImpl::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   auto pPointDO = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(pDO);
   ATLASSERT(pPointDO);

   if (pPointDO)
   {
      Float64 Xgl = m_pCutLocation->GetCurrentCutLocation();
      WBFL::Geometry::Point2d pnt(Xgl, 0.0);
      pPointDO->SetPosition(pnt, true, false);
   }
}

void CTogaSectionCutDisplayImpl::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, const WBFL::Geometry::Size2d& offset)
{
   Float64 Xgl = m_pCutLocation->GetCurrentCutLocation();

   Float64 xOffset = offset.Dx();

   Xgl += xOffset;

   PutPosition(Xgl);
}

void CTogaSectionCutDisplayImpl::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   ASSERT(FALSE);
}

void CTogaSectionCutDisplayImpl::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   ASSERT(FALSE);
}

bool CTogaSectionCutDisplayImpl::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   m_pCutLocation->ShowCutDlg();
   return true;
}

bool CTogaSectionCutDisplayImpl::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   auto list = pDO->GetDisplayList();
   auto dispMgr = list->GetDisplayMgr();

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
   auto factory = dispMgr->GetTaskFactory();
   auto task = factory->CreateLocalDragDropTask(dispMgr, point);
   dispMgr->SetTask(task);

   return true;
}

bool CTogaSectionCutDisplayImpl::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CTogaSectionCutDisplayImpl::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}


bool CTogaSectionCutDisplayImpl::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CTogaSectionCutDisplayImpl::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CTogaSectionCutDisplayImpl::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, const POINT& point)
{
   return false;
}

bool CTogaSectionCutDisplayImpl::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nFlags, short zDelta, const POINT& point)
{
   Float64 pos = m_pCutLocation->GetCurrentCutLocation();
   Float64 xoff = BinarySign(zDelta)*1.0;
   pos += xoff * m_gdrLength/100.0;
   PutPosition(pos);
   return true;
}

bool CTogaSectionCutDisplayImpl::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, UINT nChar, UINT nRepCnt, UINT nFlags)
{
   switch(nChar)
   {
   case VK_RIGHT:
   case VK_LEFT:
      {
         Float64 pos =  m_pCutLocation->GetCurrentCutLocation();

         Float64 xoff = nChar==VK_RIGHT ? 1.0 : -1.0;
         pos += xoff * m_gdrLength/100.0;

         PutPosition(pos);

      }
      break;

   case VK_RETURN:
      m_pCutLocation->ShowCutDlg();
      break;
   }

   return true;
}

bool CTogaSectionCutDisplayImpl::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, CWnd* pWnd, const POINT& point)
{
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


void CTogaSectionCutDisplayImpl::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CTogaSectionCutDisplayImpl::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

UINT CTogaSectionCutDisplayImpl::Format()
{
   return ms_Format;
}

bool CTogaSectionCutDisplayImpl::PrepareForDrag(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, std::shared_ptr<WBFL::DManip::iDragDataSink> pSink)
{
   // Create a place to store the drag data for this object
   pSink->CreateFormat(ms_Format);

   CWinThread* thread = ::AfxGetThread( );
   DWORD threadid = thread->m_nThreadID;

   pSink->Write(ms_Format,&threadid,sizeof(DWORD));
   pSink->Write(ms_Format,&m_Color,sizeof(COLORREF));
   pSink->Write(ms_Format,&m_pBroker,sizeof(IBroker*));
   pSink->Write(ms_Format,&m_SegmentKey,sizeof(CSegmentKey));
   pSink->Write(ms_Format,&m_gdrLength,sizeof(Float64));
   pSink->Write(ms_Format,&m_pCutLocation,sizeof(iCutLocation*));

   return TRUE;
}

void CTogaSectionCutDisplayImpl::OnDrop(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO, std::shared_ptr<WBFL::DManip::iDragDataSource> pSource)
{
   // Tell the source we are about to read from our format
   pSource->PrepareFormat(ms_Format);

   CWinThread* thread = ::AfxGetThread( );
   DWORD threadid = thread->m_nThreadID;

   DWORD threadl;
   pSource->Read(ms_Format,&threadl,sizeof(DWORD));

   ATLASSERT(threadid == threadl);

   pSource->Read(ms_Format,&m_Color,sizeof(COLORREF));
   pSource->Read(ms_Format,&m_pBroker,sizeof(IBroker*));
   pSource->Read(ms_Format,&m_SegmentKey,sizeof(CSegmentKey));
   pSource->Read(ms_Format,&m_gdrLength,sizeof(Float64));
   pSource->Read(ms_Format,&m_pCutLocation,sizeof(iCutLocation*));
}

