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
#include "SectionCutDisplayImpl.h"
#include "mfcdual.h"
#include <MathEx.h>
#include <PGSuperColors.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include "PGSuperDocBase.h"

#include <PgsExt\BridgeDescription2.h>
#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// height of section cut above/below girder
static const Uint32 SSIZE = 1440 * 3/8; // (twips)


UINT CSectionCutDisplayImpl::ms_Format = ::RegisterClipboardFormat(_T("SectionCutData"));

CSectionCutDisplayImpl::CSectionCutDisplayImpl():
m_pCutLocation(NULL),
m_Color(CUT_COLOR)
{
   EnableAutomation ();

   m_CachePoint.CoCreateInstance(CLSID_Point2d);
}

CSectionCutDisplayImpl::~CSectionCutDisplayImpl()
{
}

BEGIN_MESSAGE_MAP(CSectionCutDisplayImpl, CCmdTarget)
	//{{AFX_MSG_MAP(CSectionCutDisplayImpl)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_INTERFACE_MAP(CSectionCutDisplayImpl,CCmdTarget)
   INTERFACE_PART(CSectionCutDisplayImpl,IID_iDrawPointStrategy,DrawPointStrategy)
   INTERFACE_PART(CSectionCutDisplayImpl,IID_iSectionCutDrawStrategy,Strategy)
//   INTERFACE_PART(CSectionCutDisplayImpl,IID_iSectionCutEvents,Events)
   INTERFACE_PART(CSectionCutDisplayImpl,IID_iDisplayObjectEvents,DisplayObjectEvents)
   INTERFACE_PART(CSectionCutDisplayImpl,IID_iDragData,DragData)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CSectionCutDisplayImpl,DrawPointStrategy);
DELEGATE_CUSTOM_INTERFACE(CSectionCutDisplayImpl,Strategy);
//DELEGATE_CUSTOM_INTERFACE(CSectionCutDisplayImpl,Events);
DELEGATE_CUSTOM_INTERFACE(CSectionCutDisplayImpl,DisplayObjectEvents);
DELEGATE_CUSTOM_INTERFACE(CSectionCutDisplayImpl,DragData);

// This goes in the source code file
 // Note: ClassWizard looks for these comments:
 BEGIN_DISPATCH_MAP(CSectionCutDisplayImpl, CCmdTarget)
     //{{AFX_DISPATCH_MAP(AClassWithAutomation)
        // NOTE - the ClassWizard will add and remove mapping macros here.
     //}}AFX_DISPATCH_MAP
 END_DISPATCH_MAP()
 

STDMETHODIMP_(void) CSectionCutDisplayImpl::XStrategy::Init(iPointDisplayObject* pDO, IBroker* pBroker, const CGirderKey& girderKey, iCutLocation* pCutLoc)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,Strategy);

   pThis->m_pBroker = pBroker;

   pThis->m_GirderKey = girderKey;

   pThis->m_MinCutLocation = pCutLoc->GetMinCutLocation();
   pThis->m_MaxCutLocation = pCutLoc->GetMaxCutLocation();

   pThis->m_pCutLocation = pCutLoc;

   Float64 pos = pThis->m_pCutLocation->GetCurrentCutLocation();

   CComPtr<IPoint2d> pnt;
   pnt.CoCreateInstance(CLSID_Point2d);
   pnt->put_X(pos);
   pnt->put_Y( 0.0 );
   pDO->SetPosition(pnt, FALSE, FALSE);
}


STDMETHODIMP_(void) CSectionCutDisplayImpl::XStrategy::SetColor(COLORREF color)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,Strategy);
   pThis->m_Color = color;
}


STDMETHODIMP_(pgsPointOfInterest) CSectionCutDisplayImpl::XStrategy::GetCutPOI(Float64 distFromStartOfGirder)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,Strategy);
   return pThis->GetCutPOI(distFromStartOfGirder);
}

STDMETHODIMP_(void) CSectionCutDisplayImpl::XDrawPointStrategy::Draw(iPointDisplayObject* pDO,CDC* pDC)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DrawPointStrategy);

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

STDMETHODIMP_(void) CSectionCutDisplayImpl::XDrawPointStrategy::DrawHighlite(iPointDisplayObject* pDO,CDC* pDC,BOOL bHighlite)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DrawPointStrategy);
   Draw(pDO,pDC);
}

STDMETHODIMP_(void) CSectionCutDisplayImpl::XDrawPointStrategy::DrawDragImage(iPointDisplayObject* pDO,CDC* pDC, iCoordinateMap* map, const CPoint& dragStart, const CPoint& dragPoint)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DrawPointStrategy);

   Float64 wx, wy;
   map->LPtoWP(dragPoint.x, dragPoint.y, &wx, &wy);
   pThis->m_CachePoint->put_X(wx);
   pThis->m_CachePoint->put_Y(wy);

   pThis->Draw(pDO,pDC,SELECTED_OBJECT_LINE_COLOR,pThis->m_CachePoint);
}


STDMETHODIMP_(void) CSectionCutDisplayImpl::XDrawPointStrategy::GetBoundingBox(iPointDisplayObject* pDO, IRect2d** rect)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DrawPointStrategy);

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

void CSectionCutDisplayImpl::GetBoundingBox(iPointDisplayObject* pDO, Float64 position, 
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

void CSectionCutDisplayImpl::Draw(iPointDisplayObject* pDO,CDC* pDC,COLORREF color,IPoint2d* userLoc)
{
   Float64 x;
   userLoc->get_X(&x);
   x = ::ForceIntoRange(m_MinCutLocation,x,m_MaxCutLocation);

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

pgsPointOfInterest CSectionCutDisplayImpl::GetCutPOI(Float64 distFromStart)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE(IBridge,pBridge);

   Float64 distFromStartOfGirder = 0;
   GroupIndexType groupIndex = INVALID_INDEX;
   Float64 group_offset = 0;
   GroupIndexType startGroupIdx = (m_GirderKey.groupIndex == ALL_GROUPS ? 0 : m_GirderKey.groupIndex);
   GroupIndexType endGroupIdx   = (m_GirderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      Float64 girder_layout_length = pBridge->GetGirderLayoutLength(CSegmentKey(grpIdx,m_GirderKey.girderIndex,INVALID_INDEX));
      if ( ::InRange(group_offset,distFromStart,group_offset+girder_layout_length) )
      {
         distFromStartOfGirder = distFromStart - group_offset;
         groupIndex = grpIdx;
         break;
      }
      else
      {
         group_offset += girder_layout_length;
      }
   }

   SegmentIndexType segmentIndex = INVALID_INDEX;
   SegmentIndexType nSegments = pBridgeDesc->GetGirderGroup(groupIndex)->GetGirder(m_GirderKey.girderIndex)->GetSegmentCount();
   Float64 running_segment_length = 0;
   Float64 distFromStartOfSegment = 0;
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      Float64 segment_layout_length = pBridge->GetSegmentLayoutLength(CSegmentKey(groupIndex,m_GirderKey.girderIndex,segIdx));
      if ( ::InRange(running_segment_length,distFromStartOfGirder,running_segment_length+segment_layout_length))
      {
         distFromStartOfSegment = distFromStartOfGirder - running_segment_length;
         distFromStartOfSegment = ::ForceIntoRange(0.0,distFromStartOfSegment,segment_layout_length);
         segmentIndex = segIdx;
         break;
      }
      else
      {
         running_segment_length += segment_layout_length;
      }
   }

   GET_IFACE(IPointOfInterest,pPoi);
   pgsPointOfInterest poi = pPoi->GetPointOfInterest(CSegmentKey(groupIndex,m_GirderKey.girderIndex,segmentIndex),distFromStartOfSegment);
   return poi;
}

Float64 CSectionCutDisplayImpl::GetGirderHeight(Float64 distFromStart)
{
   pgsPointOfInterest poi = GetCutPOI(distFromStart);

   GET_IFACE(IGirder,pGirder);
   Float64 height = pGirder->GetHeight(poi);
   return height;
}

STDMETHODIMP_(void) CSectionCutDisplayImpl::XDisplayObjectEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DisplayObjectEvents);

   iPointDisplayObject* pPointDO = dynamic_cast<iPointDisplayObject*>(pDO);
   ATLASSERT(pPointDO);

   if (pPointDO)
   {
      Float64 pos = pThis->m_pCutLocation->GetCurrentCutLocation();
   
      CComPtr<IPoint2d> pnt;
      pnt.CoCreateInstance(CLSID_Point2d);
      pnt->put_X(pos);
      pnt->put_Y( 0.0 );
      pPointDO->SetPosition(pnt, TRUE, FALSE);
   }
}


STDMETHODIMP_(void) CSectionCutDisplayImpl::XDisplayObjectEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DisplayObjectEvents);

   Float64 pos =  pThis->m_pCutLocation->GetCurrentCutLocation();

   Float64 xoff;
   offset->get_Dx(&xoff);

   pos += xoff;

   pThis->PutPosition(pos);
}

STDMETHODIMP_(void) CSectionCutDisplayImpl::XDisplayObjectEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DisplayObjectEvents);

   ASSERT(FALSE); 
}

STDMETHODIMP_(void) CSectionCutDisplayImpl::XDisplayObjectEvents::OnCopied(iDisplayObject* pDO)
{
   ASSERT(FALSE); 
}

STDMETHODIMP_(bool) CSectionCutDisplayImpl::XDisplayObjectEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DisplayObjectEvents);

   pThis->m_pCutLocation->ShowCutDlg();

   return true;
}

STDMETHODIMP_(bool) CSectionCutDisplayImpl::XDisplayObjectEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   CComPtr<iDisplayList> list;
   pDO->GetDisplayList(&list);

   CComPtr<iDisplayMgr> dispMgr;
   list->GetDisplayMgr(&dispMgr);

   // If control key is pressed, don't clear current selection
   // (i.e. we want multi-select)
   BOOL bMultiSelect = nFlags & MK_CONTROL ? TRUE : FALSE;

   dispMgr->SelectObject(pDO,!bMultiSelect);

   // d&d task
   CComPtr<iTaskFactory> factory;
   dispMgr->GetTaskFactory(&factory);
   CComPtr<iTask> task;
   factory->CreateLocalDragDropTask(dispMgr,point,&task);
   dispMgr->SetTask(task);

   return true;
}

STDMETHODIMP_(bool) CSectionCutDisplayImpl::XDisplayObjectEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CSectionCutDisplayImpl::XDisplayObjectEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
}


STDMETHODIMP_(bool) CSectionCutDisplayImpl::XDisplayObjectEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CSectionCutDisplayImpl::XDisplayObjectEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
  
}

STDMETHODIMP_(bool) CSectionCutDisplayImpl::XDisplayObjectEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DisplayObjectEvents);
   return false;
}

STDMETHODIMP_(bool) CSectionCutDisplayImpl::XDisplayObjectEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DisplayObjectEvents);
   Float64 pos = pThis->m_pCutLocation->GetCurrentCutLocation();
   Float64 xoff = BinarySign(zDelta)*1.0;
   pos += xoff * (pThis->m_MaxCutLocation - pThis->m_MinCutLocation)/100.0;
   pThis->PutPosition(pos);
   return true;
}

STDMETHODIMP_(bool) CSectionCutDisplayImpl::XDisplayObjectEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DisplayObjectEvents);

   switch(nChar)
   {
   case VK_RIGHT:
   case VK_LEFT:
      {
         Float64 pos =  pThis->m_pCutLocation->GetCurrentCutLocation();

         Float64 xoff = nChar==VK_RIGHT ? 1.0 : -1.0;
         pos += xoff * (pThis->m_MaxCutLocation - pThis->m_MinCutLocation)/100.0;

         pThis->PutPosition(pos);

      }
      break;

   case VK_RETURN:
      pThis->m_pCutLocation->ShowCutDlg();
      break;
   }

   return true;
}

STDMETHODIMP_(bool) CSectionCutDisplayImpl::XDisplayObjectEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DisplayObjectEvents);

   if ( pDO->IsSelected() )
   {
      CComPtr<iDisplayList> pList;
      pDO->GetDisplayList(&pList);

      CComPtr<iDisplayMgr> pDispMgr;
      pList->GetDisplayMgr(&pDispMgr);

      CDisplayView* pView = pDispMgr->GetView();
      CPGSuperDocBase* pDoc = (CPGSuperDocBase*)pView->GetDocument();

      std::map<IDType,IBridgePlanViewEventCallback*> callbacks = pDoc->GetBridgePlanViewCallbacks();
      if ( callbacks.size() == 0 )
         return false;

      CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
      std::map<IDType,IBridgePlanViewEventCallback*>::iterator iter;
      for ( iter = callbacks.begin(); iter != callbacks.end(); iter++ )
      {
         IBridgePlanViewEventCallback* callback = iter->second;
         callback->OnAlignmentContextMenu(pMenu);
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

void CSectionCutDisplayImpl::PutPosition(Float64 pos)
{
   pos = ::ForceIntoRange(m_MinCutLocation,pos,m_MaxCutLocation);
   m_pCutLocation->CutAt(pos);
}


STDMETHODIMP_(void) CSectionCutDisplayImpl::XDisplayObjectEvents::OnSelect(iDisplayObject* pDO)
{

}

STDMETHODIMP_(void) CSectionCutDisplayImpl::XDisplayObjectEvents::OnUnselect(iDisplayObject* pDO)
{
}

STDMETHODIMP_(UINT) CSectionCutDisplayImpl::XDragData::Format()
{
   return ms_Format;
}

STDMETHODIMP_(BOOL) CSectionCutDisplayImpl::XDragData::PrepareForDrag(iDisplayObject* pDO,iDragDataSink* pSink)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DragData);

   // Create a place to store the drag data for this object
   pSink->CreateFormat(ms_Format);

   CWinThread* thread = ::AfxGetThread( );
   DWORD threadid = thread->m_nThreadID;

   pSink->Write(ms_Format,&threadid,sizeof(DWORD));
   pSink->Write(ms_Format,&pThis->m_Color,sizeof(COLORREF));
   pSink->Write(ms_Format,&pThis->m_pBroker,sizeof(IBroker*));
   pSink->Write(ms_Format,&pThis->m_GirderKey,sizeof(CSegmentKey));
   pSink->Write(ms_Format,&pThis->m_MinCutLocation,sizeof(Float64));
   pSink->Write(ms_Format,&pThis->m_MaxCutLocation,sizeof(Float64));
   pSink->Write(ms_Format,&pThis->m_pCutLocation,sizeof(iCutLocation*));

   return TRUE;
}

STDMETHODIMP_(void) CSectionCutDisplayImpl::XDragData::OnDrop(iDisplayObject* pDO,iDragDataSource* pSource)
{
   METHOD_PROLOGUE(CSectionCutDisplayImpl,DragData);

   // Tell the source we are about to read from our format
   pSource->PrepareFormat(ms_Format);

   CWinThread* thread = ::AfxGetThread( );
   DWORD threadid = thread->m_nThreadID;

   DWORD threadl;
   pSource->Read(ms_Format,&threadl,sizeof(DWORD));

   ATLASSERT(threadid == threadl);

   pSource->Read(ms_Format,&pThis->m_Color,sizeof(COLORREF));
   pSource->Read(ms_Format,&pThis->m_pBroker,sizeof(IBroker*));
   pSource->Read(ms_Format,&pThis->m_GirderKey,sizeof(CSegmentKey));
   pSource->Read(ms_Format,&pThis->m_MinCutLocation,sizeof(Float64));
   pSource->Read(ms_Format,&pThis->m_MaxCutLocation,sizeof(Float64));
   pSource->Read(ms_Format,&pThis->m_pCutLocation,sizeof(iCutLocation*));
}

