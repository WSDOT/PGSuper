///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// DisplayObjectFactory.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "DisplayObjectFactory.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperDoc.h"

#include "mfcdual.h"
#include "SectionCutDisplayImpl.h"
#include "BridgeSectionCutDisplayImpl.h"
#include "PointLoadDrawStrategyImpl.h"
#include "DistributedLoadDrawStrategyImpl.h"
#include "MomentLoadDrawStrategyImpl.h"
#include <WBFLDManip.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDisplayObjectFactory

CDisplayObjectFactory::CDisplayObjectFactory(CPGSuperDoc* pDoc)
{
   m_pDoc = pDoc;
   ::CoCreateInstance(CLSID_DisplayObjectFactory,NULL,CLSCTX_ALL,IID_iDisplayObjectFactory,(void**)&m_Factory);
}


CDisplayObjectFactory::~CDisplayObjectFactory()
{
}




BEGIN_INTERFACE_MAP(CDisplayObjectFactory,CCmdTarget)
   INTERFACE_PART(CDisplayObjectFactory,IID_iDisplayObjectFactory,Factory)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CDisplayObjectFactory,Factory);


/////////////////////////////////////////////////////////////////////////////
// CDisplayObjectFactory message handlers
STDMETHODIMP_(void) CDisplayObjectFactory::XFactory::Create(CLIPFORMAT cfFormat,COleDataObject* pDataObject,iDisplayObject** dispObj)
{
   METHOD_PROLOGUE(CDisplayObjectFactory,Factory);

   if ( cfFormat == CSectionCutDisplayImpl::ms_Format )
   {
      CComPtr<iPointDisplayObject> doSectionCut;
      ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doSectionCut);

      doSectionCut->SetSelectionType(stAll);

      CSectionCutDisplayImpl* pDisplayImpl = new CSectionCutDisplayImpl();
      IUnknown* unk = pDisplayImpl->GetInterface(&IID_iDrawPointStrategy);
      doSectionCut->SetDrawingStrategy((iDrawPointStrategy*)unk);

      unk = pDisplayImpl->GetInterface(&IID_iDisplayObjectEvents);
      doSectionCut->RegisterEventSink((iDisplayObjectEvents*)unk);

      unk = pDisplayImpl->GetInterface(&IID_IUnknown);
      CComQIPtr<iDragData,&IID_iDragData> dd(unk);
      CComQIPtr<iDraggable,&IID_iDraggable> draggable(doSectionCut);
      draggable->SetDragData(dd);

      if ( pDataObject )
      {
         // Initialize from data object
         CComPtr<iDragDataSource> source;
         ::CoCreateInstance(CLSID_DragDataSource,NULL,CLSCTX_ALL,IID_iDragDataSource,(void**)&source);
         source->SetDataObject(pDataObject);

         // rebuild the display object from the data source
         draggable->OnDrop(source);
      }

      (*dispObj) = doSectionCut;
      (*dispObj)->AddRef();
   }
   else if ( cfFormat == CBridgeSectionCutDisplayImpl::ms_Format )
   {
      CComPtr<iPointDisplayObject> doSectionCut;
      ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doSectionCut);

      CBridgeSectionCutDisplayImpl* pDisplayImpl = new CBridgeSectionCutDisplayImpl(); // ref count = 1
      IUnknown* unk = pDisplayImpl->GetInterface(&IID_iDrawPointStrategy);
      doSectionCut->SetDrawingStrategy((iDrawPointStrategy*)unk);
      unk->Release(); // balances the AddRef for new above

      unk = pDisplayImpl->GetInterface(&IID_iDisplayObjectEvents);
      doSectionCut->RegisterEventSink((iDisplayObjectEvents*)unk);

      unk = pDisplayImpl->GetInterface(&IID_IUnknown);
      CComQIPtr<iDragData,&IID_iDragData> dd(unk);
      CComQIPtr<iDraggable,&IID_iDraggable> draggable(doSectionCut);
      draggable->SetDragData(dd);

      if ( pDataObject )
      {
         // Initialize from data object
         CComPtr<iDragDataSource> source;
         ::CoCreateInstance(CLSID_DragDataSource,NULL,CLSCTX_ALL,IID_iDragDataSource,(void**)&source);
         source->SetDataObject(pDataObject);

         // rebuild the display object from the data source
         draggable->OnDrop(source);
      }

      (*dispObj) = doSectionCut;
      (*dispObj)->AddRef();
   }
   else if ( cfFormat == CPointLoadDrawStrategyImpl::ms_Format )
   {
      CComPtr<iPointDisplayObject> LoadRep;
      ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&LoadRep);

      LoadRep->SetSelectionType(stAll);

      CPointLoadDrawStrategyImpl* pDisplayImpl = new CPointLoadDrawStrategyImpl();
      IUnknown*  unk = pDisplayImpl->GetInterface(&IID_iDrawPointStrategy);
      LoadRep->SetDrawingStrategy((iDrawPointStrategy*)unk);

      unk = pDisplayImpl->GetInterface(&IID_iDisplayObjectEvents);
      LoadRep->RegisterEventSink((iDisplayObjectEvents*)unk);
      
      unk = pDisplayImpl->GetInterface(&IID_IUnknown);
      CComQIPtr<iDragData,&IID_iDragData> dd(unk);
      CComQIPtr<iDraggable,&IID_iDraggable> draggable(LoadRep);
      draggable->SetDragData(dd);

      if ( pDataObject )
      {
         // Initialize from data object
         CComPtr<iDragDataSource> source;
         ::CoCreateInstance(CLSID_DragDataSource,NULL,CLSCTX_ALL,IID_iDragDataSource,(void**)&source);
         source->SetDataObject(pDataObject);

         // rebuild the display object from the data source
         draggable->OnDrop(source);
      }

      (*dispObj) = LoadRep;
      (*dispObj)->AddRef();
   }
   else if ( cfFormat == CDistributedLoadDrawStrategyImpl::ms_Format )
   {
      CComPtr<iPointDisplayObject> LoadRep;
      ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&LoadRep);

      LoadRep->SetSelectionType(stAll);

      CDistributedLoadDrawStrategyImpl* pDisplayImpl = new CDistributedLoadDrawStrategyImpl();
      IUnknown*  unk = pDisplayImpl->GetInterface(&IID_iDrawPointStrategy);
      LoadRep->SetDrawingStrategy((iDrawPointStrategy*)unk);

      unk = pDisplayImpl->GetInterface(&IID_iDisplayObjectEvents);
      LoadRep->RegisterEventSink((iDisplayObjectEvents*)unk);

      unk = pDisplayImpl->GetInterface(&IID_iGravityWellStrategy);
      LoadRep->SetGravityWellStrategy((iGravityWellStrategy*)unk);

      unk = pDisplayImpl->GetInterface(&IID_IUnknown);
      CComQIPtr<iDragData,&IID_iDragData> dd(unk);
      CComQIPtr<iDraggable,&IID_iDraggable> draggable(LoadRep);
      draggable->SetDragData(dd);

      if ( pDataObject )
      {
         // Initialize from data object
         CComPtr<iDragDataSource> source;
         ::CoCreateInstance(CLSID_DragDataSource,NULL,CLSCTX_ALL,IID_iDragDataSource,(void**)&source);
         source->SetDataObject(pDataObject);

         // rebuild the display object from the data source
         draggable->OnDrop(source);
      }

      (*dispObj) = LoadRep;
      (*dispObj)->AddRef();
   }
   else if ( cfFormat == CMomentLoadDrawStrategyImpl::ms_Format )
   {
      CComPtr<iPointDisplayObject> LoadRep;
      ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&LoadRep);

      LoadRep->SetSelectionType(stAll);

      CMomentLoadDrawStrategyImpl* pDisplayImpl = new CMomentLoadDrawStrategyImpl();
      IUnknown*  unk = pDisplayImpl->GetInterface(&IID_iDrawPointStrategy);
      LoadRep->SetDrawingStrategy((iDrawPointStrategy*)unk);

      unk = pDisplayImpl->GetInterface(&IID_iDisplayObjectEvents);
      LoadRep->RegisterEventSink((iDisplayObjectEvents*)unk);
      
      unk = pDisplayImpl->GetInterface(&IID_IUnknown);
      CComQIPtr<iDragData,&IID_iDragData> dd(unk);
      CComQIPtr<iDraggable,&IID_iDraggable> draggable(LoadRep);
      draggable->SetDragData(dd);

      if ( pDataObject )
      {
         // Initialize from data object
         CComPtr<iDragDataSource> source;
         ::CoCreateInstance(CLSID_DragDataSource,NULL,CLSCTX_ALL,IID_iDragDataSource,(void**)&source);
         source->SetDataObject(pDataObject);

         // rebuild the display object from the data source
         draggable->OnDrop(source);
      }

      (*dispObj) = LoadRep;
      (*dispObj)->AddRef();
   }
   else
   {
      pThis->m_Factory->Create(cfFormat,pDataObject,dispObj);
   }

}
