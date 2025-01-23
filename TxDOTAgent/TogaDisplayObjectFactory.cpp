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

// TogaDisplayObjectFactory.cpp : implementation file
//

#include "stdafx.h"
#include "TogaDisplayObjectFactory.h"
#include "TxDOTOptionalDesignDoc.h"

#include "TogaSectionCutDisplayImpl.h"

#include <DManip/DisplayObjectFactoryImpl.h>
#include <DManip/PointDisplayObjectImpl.h>
#include <DManip/DragDataImpl.h>

CTogaDisplayObjectFactory::CTogaDisplayObjectFactory(CTxDOTOptionalDesignDoc* pDoc)
{
   m_pDoc = pDoc;
   m_Factory = WBFL::DManip::DisplayObjectFactory::Create();
}

/////////////////////////////////////////////////////////////////////////////
// CTogaDisplayObjectFactory message handlers
std::shared_ptr<WBFL::DManip::iDisplayObject> CTogaDisplayObjectFactory::Create(CLIPFORMAT cfFormat,COleDataObject* pDataObject) const
{
   if ( cfFormat == CTogaSectionCutDisplayImpl::ms_Format )
   {
      auto doSectionCut = WBFL::DManip::PointDisplayObject::Create();
      doSectionCut->SetSelectionType(WBFL::DManip::SelectionType::All);

      auto pDisplayImpl = std::make_shared<CTogaSectionCutDisplayImpl>();
      auto strategy = std::dynamic_pointer_cast<WBFL::DManip::iDrawPointStrategy>(pDisplayImpl);
      doSectionCut->SetDrawingStrategy(strategy);

      auto events = std::dynamic_pointer_cast<WBFL::DManip::iDisplayObjectEvents>(pDisplayImpl);
      doSectionCut->RegisterEventSink(events);

      auto dd = std::dynamic_pointer_cast<WBFL::DManip::iDragData>(pDisplayImpl);
      auto draggable = std::dynamic_pointer_cast<WBFL::DManip::iDraggable>(doSectionCut);
      draggable->SetDragData(dd);

      if ( pDataObject )
      {
         // Initialize from data object
         auto source = WBFL::DManip::DragDataSource::Create();
         source->SetDataObject(pDataObject);

         // rebuild the display object from the data source
         draggable->OnDrop(source);
      }

      return doSectionCut;
   }

   return m_Factory->Create(cfFormat,pDataObject);
}
