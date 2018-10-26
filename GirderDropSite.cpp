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

// GirderDropSite.cpp: implementation of the CGirderDropSite class.
//
//////////////////////////////////////////////////////////////////////

#include "PGSuperAppPlugin\stdafx.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "GirderDropSite.h"
#include "GirderModelChildFrame.h"
#include "EditPointLoadDlg.h"
#include "EditDistributedLoadDlg.h"
#include "EditMomentLoadDlg.h"
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include "mfcdual.h"

#include <PgsExt\InsertDeleteLoad.h>
#include <WBFLDManip.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGirderDropSite::CGirderDropSite(CPGSuperDocBase* pDoc, const CSpanGirderKey& spanGirderKey, CGirderModelChildFrame* pFrame) :
m_SpanGirderKey(spanGirderKey)
{
   m_pDoc   = pDoc;
   m_pFrame = pFrame;
}

CGirderDropSite::~CGirderDropSite()
{

}

BEGIN_INTERFACE_MAP(CGirderDropSite,CCmdTarget)
   INTERFACE_PART(CGirderDropSite,IID_iDropSite,DropSite)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CGirderDropSite,DropSite);


STDMETHODIMP_(DROPEFFECT) CGirderDropSite::XDropSite::CanDrop(COleDataObject* pDataObject,DWORD dwKeyState,IPoint2d* point)
{
   METHOD_PROLOGUE(CGirderDropSite,DropSite);

   // Was a tool dragged over the Girder?
   CComPtr<iTool> tool;
   ::CoCreateInstance(CLSID_Tool,NULL,CLSCTX_ALL,IID_iTool,(void**)&tool);
   CComQIPtr<iDraggable,&IID_iDraggable> draggable(tool);

   if ( pDataObject->IsDataAvailable( draggable->Format() ) )
   {
      CComPtr<iDragDataSource> source;
      ::CoCreateInstance(CLSID_DragDataSource,NULL,CLSCTX_ALL,IID_iDragDataSource,(void**)&source);
      source->SetDataObject(pDataObject);
      draggable->OnDrop(source); // Rebuild the tool object from the data object

      DWORD id = tool->GetID();

      if ( id == IDC_POINT_LOAD_DRAG       || 
           id == IDC_DISTRIBUTED_LOAD_DRAG ||
           id == IDC_MOMENT_LOAD_DRAG 
         )
         return DROPEFFECT_MOVE;
   }

   return DROPEFFECT_NONE;
}

STDMETHODIMP_(void) CGirderDropSite::XDropSite::OnDropped(COleDataObject* pDataObject,DROPEFFECT dropEffect,IPoint2d* point)
{
   METHOD_PROLOGUE_(CGirderDropSite,DropSite);
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Something was dropped on a display object that represents a Girder

   // Let's see what was dropped

   // Was it a tool from the palette?
   CComPtr<iTool> tool;
   ::CoCreateInstance(CLSID_Tool,NULL,CLSCTX_ALL,IID_iTool,(void**)&tool);
   CComQIPtr<iDraggable,&IID_iDraggable> draggable(tool);
   if ( pDataObject->IsDataAvailable(draggable->Format()) )
   {
      CComPtr<iDragDataSource> source;
      ::CoCreateInstance(CLSID_DragDataSource,NULL,CLSCTX_ALL,IID_iDragDataSource,(void**)&source);
      source->SetDataObject(pDataObject);
      draggable->OnDrop(source); // Rebuild the tool object from the data object

      // Was it the concentrated load tool?
      if ( tool->GetID() == IDC_POINT_LOAD_DRAG )
      {
         // set data to that of view
         CPointLoadData data;
         data.m_SpanGirderKey = pThis->m_SpanGirderKey;

         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
         EventIndexType liveLoadEventIdx = pIBridgeDesc->GetLiveLoadEventIndex();

         data.m_EventIdx = pThis->m_pFrame->GetEvent();
         if (pThis->m_pFrame->GetEvent() == liveLoadEventIdx )
            data.m_LoadCase = UserLoads::LL_IM;

         CComPtr<IPoint2d> pntStart, pntEnd;
         CComQIPtr<iLineDisplayObject> doLine(pThis->m_DispObj);
         doLine->GetEndPoints(&pntStart,&pntEnd);
         Float64 segX, wx;
         pntStart->get_X(&segX);
         point->get_X(&wx);

         Float64 load_loc = wx - segX;

         //// estimate where we are at
         //CComPtr<IBroker> pBroker;
         //pThis->m_pDoc->GetBroker(&pBroker);
         //GET_IFACE2(pBroker,IBridge,pBridge);
         //Float64 gdr_length = pBridge->GetSegmentLength(data.m_SegmentKey);
         //Float64 start_lgth = pBridge->GetSegmentStartEndDistance(data.m_SegmentKey);
         //Float64 end_lgth   = pBridge->GetSegmentEndEndDistance(data.m_SegmentKey);
         //Float64 span_lgth  = gdr_length - start_lgth - end_lgth;

         //Float64 wx;
         //point->get_X(&wx);

         //wx = wx - start_lgth;  // span coordinates

         //Float64 load_loc;
         //if (wx < 0.0)
         //   load_loc = 0.0;
         //else if (wx > span_lgth)
         //   load_loc = span_lgth;
         //else
         //   load_loc = wx;

         data.m_Fractional = false;
         data.m_Location   = load_loc;

	      CEditPointLoadDlg dlg(data);
         if (dlg.DoModal() == IDOK)
         {
            txnInsertPointLoad* pTxn = new txnInsertPointLoad(dlg.m_Load);
            txnTxnManager::GetInstance()->Execute(pTxn);
         }
      }
      // perhaps a distributed load?
      else if ( tool->GetID() == IDC_DISTRIBUTED_LOAD_DRAG )
      {
         // set data to that of view
         CDistributedLoadData data;
         data.m_SpanGirderKey = pThis->m_SpanGirderKey;

         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
         EventIndexType liveLoadEventIdx = pIBridgeDesc->GetLiveLoadEventIndex();

         data.m_EventIdx = pThis->m_pFrame->GetEvent();
         if (pThis->m_pFrame->GetEvent() == liveLoadEventIdx )
            data.m_LoadCase = UserLoads::LL_IM;

         // estimate where we are at
         GET_IFACE2(pBroker,IBridge,pBridge);
         Float64 span_length = pBridge->GetSpanLength(data.m_SpanGirderKey.spanIndex,data.m_SpanGirderKey.girderIndex);
         //Float64 start_lgth = pBridge->GetSegmentStartEndDistance(data.m_SegmentKey);
         //Float64 end_lgth   = pBridge->GetSegmentEndEndDistance(data.m_SegmentKey);
         //Float64 span_lgth  = gdr_length - start_lgth - end_lgth;

         // set length of load to 1/10 span length
         Float64 load_length = span_length/10;

         Float64 wx;
         point->get_X(&wx);

         wx = wx - span_length;  // span coordinates

         Float64 load_loc;
         if (wx < 0.0)
            load_loc = 0.0;
         else if (wx > span_length-load_length)
            load_loc = span_length-load_length;
         else
            load_loc = wx;

         // start with 1/4 of span length and shorten if we are near the end
         data.m_Fractional = false;
         data.m_StartLocation = load_loc;
         data.m_EndLocation = load_loc+load_length;

	      CEditDistributedLoadDlg dlg(data);
         if (dlg.DoModal() == IDOK)
         {
            txnInsertDistributedLoad* pTxn = new txnInsertDistributedLoad(dlg.m_Load);
            txnTxnManager::GetInstance()->Execute(pTxn);
         }
      }
      else if ( tool->GetID() == IDC_MOMENT_LOAD_DRAG )
      {
         // set data to that of view
         CMomentLoadData data;
         data.m_SpanGirderKey = pThis->m_SpanGirderKey;

         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
         EventIndexType liveLoadEventIdx = pIBridgeDesc->GetLiveLoadEventIndex();

         data.m_EventIdx = pThis->m_pFrame->GetEvent();
         if (pThis->m_pFrame->GetEvent() == liveLoadEventIdx )
            data.m_LoadCase = UserLoads::LL_IM;

         // estimate where we are at
         GET_IFACE2(pBroker,IBridge,pBridge);
         Float64 span_length = pBridge->GetSpanLength(data.m_SpanGirderKey.spanIndex,data.m_SpanGirderKey.girderIndex);
         //Float64 start_lgth = pBridge->GetSegmentStartEndDistance(data.m_SegmentKey);
         //Float64 end_lgth   = pBridge->GetSegmentEndEndDistance(data.m_SegmentKey);
         //Float64 span_lgth  = gdr_length - start_lgth - end_lgth;

         Float64 wx;
         point->get_X(&wx);

         wx = wx - span_length;  // span coordinates

         Float64 load_loc;
         if (wx < 0.0)
            load_loc = 0.0;
         else if (wx > span_length)
            load_loc = span_length;
         else
            load_loc = wx;

         data.m_Fractional = false;
         data.m_Location   = load_loc;

	      CEditMomentLoadDlg dlg(data);
         if (dlg.DoModal() == IDOK)
         {
            txnInsertMomentLoad* pTxn = new txnInsertMomentLoad(dlg.m_Load);
            txnTxnManager::GetInstance()->Execute(pTxn);
         }
      }
   }
}

STDMETHODIMP_(void) CGirderDropSite::XDropSite::SetDisplayObject(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CGirderDropSite,DropSite);
   pThis->m_DispObj = pDO;
}

STDMETHODIMP_(void) CGirderDropSite::XDropSite::GetDisplayObject(iDisplayObject** dispObj)
{
   METHOD_PROLOGUE(CGirderDropSite,DropSite);
   *dispObj = pThis->m_DispObj;
   (*dispObj)->AddRef();
}

STDMETHODIMP_(void) CGirderDropSite::XDropSite::Highlite(CDC* pDC,BOOL bHighlite)
{
   METHOD_PROLOGUE(CGirderDropSite,DropSite);
   pThis->m_DispObj->Highlite(pDC,bHighlite);
}


