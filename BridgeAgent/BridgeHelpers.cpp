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
#include "BridgeHelpers.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HRESULT GetSuperstructureMember(IGenericBridge* pBridge,SpanIndexType spanIdx,GirderIndexType gdrIdx,ISuperstructureMember* *ssmbr)
{
   CComPtr<ISpanCollection> spans;
   pBridge->get_Spans(&spans);

   CComPtr<ISpan> span;
   spans->get_Item(spanIdx,&span);

   // assumes one superstructure member per span
   return span->get_SuperstructureMember(gdrIdx,ssmbr);
}

HRESULT GetGirder(IGenericBridge* pBridge,SpanIndexType spanIdx,GirderIndexType gdrIdx,IPrecastGirder** girder)
{
   CComPtr<ISuperstructureMember> ssmbr;
   GetSuperstructureMember(pBridge,spanIdx,gdrIdx,&ssmbr);

   CComQIPtr<IItemData> itemdata(ssmbr);
   CComPtr<IUnknown> punk;
   itemdata->GetItemData(CComBSTR("Precast Girder"),&punk);
   CComQIPtr<IPrecastGirder> gdr(punk);

   (*girder) = gdr;
   (*girder)->AddRef();

   return S_OK;
}
