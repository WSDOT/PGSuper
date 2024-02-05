///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include "SectionCutDisplayImpl.h"
#include <MathEx.h>
#include <PGSuperColors.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>
#include "PGSuperDocBase.h"

#include <PgsExt\BridgeDescription2.h>
#include "PGSuperDoc.h"

#include <DManip/PointDisplayObject.h>
#include <DManip/DisplayMgr.h>
#include <DManip/TaskFactory.h>
#include <DManip/DisplayView.h>

// height of section cut above/below girder
static const Uint32 SSIZE = 1440 * 3/8; // (twips)


UINT CSectionCutDisplayImpl::ms_Format = ::RegisterClipboardFormat(_T("SectionCutData"));

CSectionCutDisplayImpl::CSectionCutDisplayImpl():
m_pCutLocation(nullptr),
m_Color(CUT_COLOR)
{
}

CSectionCutDisplayImpl::~CSectionCutDisplayImpl()
{
}

void CSectionCutDisplayImpl::Init(std::shared_ptr<WBFL::DManip::iPointDisplayObject> pDO, IBroker* pBroker, const CGirderKey& girderKey, iCutLocation* pCutLoc)
{
   m_pBroker = pBroker;

   m_GirderKey = girderKey;

   pCutLoc->GetCutRange(&(m_MinCutLocation), &(m_MaxCutLocation));
  
   m_pCutLocation = pCutLoc;

   Float64 Xgl = m_pCutLocation->GetCurrentCutLocation();

   WBFL::Geometry::Point2d pnt(Xgl, 0.0);
   pDO->SetPosition(pnt, false, false);
}

void CSectionCutDisplayImpl::SetColor(COLORREF color)
{
   m_Color = color;
}

pgsPointOfInterest CSectionCutDisplayImpl::GetCutPOI(Float64 Xgl) const
{
   GET_IFACE(IPointOfInterest, pPoi);
   if (m_GirderKey.groupIndex == ALL_GROUPS)
   {
      return pPoi->ConvertGirderlineCoordinateToPoi(m_GirderKey.girderIndex, Xgl);
   }
   else
   {
      return pPoi->ConvertGirderPathCoordinateToPoi(m_GirderKey, Xgl);
   }
}

void CSectionCutDisplayImpl::Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, CDC* pDC) const
{
   auto pDL = pDO->GetDisplayList();
   auto pDispMgr = pDL->GetDisplayMgr();

   COLORREF color;

   if ( pDO->IsSelected() )
   {
      color = pDispMgr->GetSelectionLineColor();
   }
   else
   {
      color = m_Color;
   }

   auto pos = pDO->GetPosition();

   Draw(pDO,pDC,color,pos);
}

void CSectionCutDisplayImpl::DrawHighlight(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC,bool bHighlite) const
{
   Draw(pDO,pDC);
}

void CSectionCutDisplayImpl::DrawDragImage(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC, std::shared_ptr<const WBFL::DManip::iCoordinateMap> map, const POINT& dragStart, const POINT& dragPoint) const
{
   Float64 wx, wy;
   map->LPtoWP(dragPoint.x, dragPoint.y, &wx, &wy);
   m_CachePoint.Move(wx, wy);

   Draw(pDO,pDC,SELECTED_OBJECT_LINE_COLOR,m_CachePoint);
}


WBFL::Geometry::Rect2d CSectionCutDisplayImpl::GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO) const
{
   auto pnt = pDO->GetPosition();
   
   Float64 Xgl = pnt.X();

   Float64 top, left, right, bottom;
   GetBoundingBox(pDO, Xgl, &top, &left, &right, &bottom);

   return { left,bottom,right,top };
}

void CSectionCutDisplayImpl::GetBoundingBox(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO, Float64 Xgl, 
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

   Float64 dx = (x2 - xo) / 2.0;  // width is half of height
   Float64 dy = y2-yo;

   pgsPointOfInterest poi = GetCutPOI(Xgl);

   GET_IFACE(IGirder, pGirder);
   Float64 Hg = pGirder->GetHeight(poi);
   Float64 top_flange_thickening = pGirder->GetTopFlangeThickening(poi);
   
   GET_IFACE(ICamber, pCamber);
   Float64 precamber = pCamber->GetPrecamber(poi,pgsTypes::pddErected);

   *top = dy + top_flange_thickening + precamber;
   *bottom = *top - Hg - 2*dy;
   *left   = Xgl;
   *right  = *left + dx;
}

void CSectionCutDisplayImpl::Draw(std::shared_ptr<const WBFL::DManip::iPointDisplayObject> pDO,CDC* pDC,COLORREF color,const WBFL::Geometry::Point2d& userLoc) const
{
   Float64 Xgl = userLoc.X(); // userLoc is in view coordinates... this has something to do with why we can't draw a section cut in the start cantilever
   Xgl = ::ForceIntoRange(m_MinCutLocation,Xgl,m_MaxCutLocation);

   Float64 wtop, wleft, wright, wbottom;
   GetBoundingBox(pDO, Xgl, &wtop, &wleft, &wright, &wbottom);

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

void CSectionCutDisplayImpl::OnChanged(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
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


void CSectionCutDisplayImpl::OnDragMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,const WBFL::Geometry::Size2d& offset)
{
   Float64 Xgl =  m_pCutLocation->GetCurrentCutLocation();

   Float64 xOffset = offset.Dx();

   Xgl += xOffset;

   PutPosition(Xgl);
}

void CSectionCutDisplayImpl::OnMoved(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   ASSERT(FALSE); 
}

void CSectionCutDisplayImpl::OnCopied(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
   ASSERT(FALSE); 
}

bool CSectionCutDisplayImpl::OnLButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   if (pDO->IsSelected())
   {
      m_pCutLocation->ShowCutDlg();
      return true;
   }
   else
   {
      return false;
   }
}

bool CSectionCutDisplayImpl::OnLButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   auto list = pDO->GetDisplayList();
   auto dispMgr = list->GetDisplayMgr();

   // If control key is pressed, don't clear current selection
   // (i.e. we want multi-select)
   BOOL bMultiSelect = nFlags & MK_CONTROL ? TRUE : FALSE;

   dispMgr->SelectObject(pDO,!bMultiSelect);

   // d&d task
   auto factory = dispMgr->GetTaskFactory();
   auto task = factory->CreateLocalDragDropTask(dispMgr,point);
   dispMgr->SetTask(task);

   return true;
}

bool CSectionCutDisplayImpl::OnRButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CSectionCutDisplayImpl::OnLButtonUp(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}


bool CSectionCutDisplayImpl::OnRButtonDblClk(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CSectionCutDisplayImpl::OnRButtonDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CSectionCutDisplayImpl::OnMouseMove(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,const POINT& point)
{
   return false;
}

bool CSectionCutDisplayImpl::OnMouseWheel(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nFlags,short zDelta,const POINT& point)
{
   if ( zDelta < 0 )
   {
      m_pCutLocation->CutAtPrev();
   }
   else
   {
      m_pCutLocation->CutAtNext();
   }
   return true;
}

bool CSectionCutDisplayImpl::OnKeyDown(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   switch(nChar)
   {
   case VK_RIGHT:
      m_pCutLocation->CutAtNext();
      break;

   case VK_LEFT:
      m_pCutLocation->CutAtPrev();
      break;

   case VK_RETURN:
      m_pCutLocation->ShowCutDlg();
      break;
   }

   return true;
}

bool CSectionCutDisplayImpl::OnContextMenu(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,CWnd* pWnd,const POINT& point)
{
   if ( pDO->IsSelected() )
   {
      auto pList = pDO->GetDisplayList();
      auto pDispMgr =  pList->GetDisplayMgr();
      auto pView = pDispMgr->GetView();
      CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();

      const std::map<IDType,IBridgePlanViewEventCallback*>& callbacks = pDoc->GetBridgePlanViewCallbacks();
      if ( callbacks.size() == 0 )
         return false;

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
      std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
      for ( ; callbackIter != callbackIterEnd; callbackIter++ )
      {
         IBridgePlanViewEventCallback* pCallback = callbackIter->second;
         pCallback->OnAlignmentContextMenu(pMenu);
      }

      bool bResult = false;
      if ( 0 < pMenu->GetMenuItemCount() )
      {
         pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,pWnd);
         bResult = true;
      }

      delete pMenu;

      return bResult;
   }

   return false;
}

void CSectionCutDisplayImpl::PutPosition(Float64 X)
{
   m_pCutLocation->CutAt(X);
}


void CSectionCutDisplayImpl::OnSelect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

void CSectionCutDisplayImpl::OnUnselect(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO)
{
}

UINT CSectionCutDisplayImpl::Format()
{
   return ms_Format;
}

bool CSectionCutDisplayImpl::PrepareForDrag(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,std::shared_ptr<WBFL::DManip::iDragDataSink> pSink)
{
   // Create a place to store the drag data for this object
   pSink->CreateFormat(ms_Format);

   CWinThread* thread = ::AfxGetThread( );
   DWORD threadid = thread->m_nThreadID;

   pSink->Write(ms_Format,&threadid,sizeof(DWORD));
   pSink->Write(ms_Format,&m_Color,sizeof(COLORREF));
   pSink->Write(ms_Format,&m_pBroker,sizeof(IBroker*));
   pSink->Write(ms_Format,&m_GirderKey,sizeof(CGirderKey));
   pSink->Write(ms_Format,&m_MinCutLocation,sizeof(Float64));
   pSink->Write(ms_Format,&m_MaxCutLocation,sizeof(Float64));
   pSink->Write(ms_Format,&m_pCutLocation,sizeof(iCutLocation*));

   return TRUE;
}

void CSectionCutDisplayImpl::OnDrop(std::shared_ptr<WBFL::DManip::iDisplayObject> pDO,std::shared_ptr<WBFL::DManip::iDragDataSource> pSource)
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
   pSource->Read(ms_Format,&m_GirderKey,sizeof(CGirderKey));
   pSource->Read(ms_Format,&m_MinCutLocation,sizeof(Float64));
   pSource->Read(ms_Format,&m_MaxCutLocation,sizeof(Float64));
   pSource->Read(ms_Format,&m_pCutLocation,sizeof(iCutLocation*));
}

