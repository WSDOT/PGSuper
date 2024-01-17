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

// TogaDisplayObjectFactory.cpp : implementation file
//

#include "stdafx.h"
#include "TogaDisplayObjectFactory.h"
#include "TxDOTOptionalDesignDoc.h"

#include "mfcdual.h"
#include "TogaSectionCutDisplayImpl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTogaDisplayObjectFactory

CTogaDisplayObjectFactory::CTogaDisplayObjectFactory(CTxDOTOptionalDesignDoc* pDoc)
{
   m_pDoc = pDoc;
   ::CoCreateInstance(CLSID_DisplayObjectFactory,nullptr,CLSCTX_ALL,IID_iDisplayObjectFactory,(void**)&m_Factory);
}

CTogaDisplayObjectFactory::~CTogaDisplayObjectFactory()
{
}

BEGIN_INTERFACE_MAP(CTogaDisplayObjectFactory,CCmdTarget)
   INTERFACE_PART(CTogaDisplayObjectFactory,IID_iDisplayObjectFactory,Factory)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CTogaDisplayObjectFactory,Factory);


/////////////////////////////////////////////////////////////////////////////
// CTogaDisplayObjectFactory message handlers
STDMETHODIMP_(void) CTogaDisplayObjectFactory::XFactory::Create(CLIPFORMAT cfFormat,COleDataObject* pDataObject,iDisplayObject** dispObj)
{
   METHOD_PROLOGUE(CTogaDisplayObjectFactory,Factory);

   if ( cfFormat == CTogaSectionCutDisplayImpl::ms_Format )
   {
      CComPtr<iPointDisplayObject> doSectionCut;
      ::CoCreateInstance(CLSID_PointDisplayObject,nullptr,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doSectionCut);

      doSectionCut->SetSelectionType(stAll);

      CTogaSectionCutDisplayImpl* pDisplayImpl = new CTogaSectionCutDisplayImpl();
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
         ::CoCreateInstance(CLSID_DragDataSource,nullptr,CLSCTX_ALL,IID_iDragDataSource,(void**)&source);
         source->SetDataObject(pDataObject);

         // rebuild the display object from the data source
         draggable->OnDrop(source);
      }

      (*dispObj) = doSectionCut;
      (*dispObj)->AddRef();
   }
   else
   {
      pThis->m_Factory->Create(cfFormat,pDataObject,dispObj);
   }

}
