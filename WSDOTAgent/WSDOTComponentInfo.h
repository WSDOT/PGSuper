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


#pragma once
#include "PGSComponentInfo.h"
#include "resource.h"

class ATL_NO_VTABLE CWSDOTComponentInfo : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CWSDOTComponentInfo, &CLSID_WSDOTComponentInfo>,
   public IPGSuperComponentInfo
{
public:
   CWSDOTComponentInfo()
   {
   }

DECLARE_REGISTRY_RESOURCEID(IDR_WSDOTCOMPONENTINFO)
DECLARE_CLASSFACTORY_SINGLETON(CWSDOTComponentInfo)

BEGIN_COM_MAP(CWSDOTComponentInfo)
   COM_INTERFACE_ENTRY(IPGSuperComponentInfo)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CWSDOTComponentInfo)
END_CONNECTION_POINT_MAP()

   HRESULT FinalConstruct();
   void FinalRelease();

// IPGSuperComponentInfo
public:
   virtual BOOL Init(CPGSuperDoc* pDoc);
   virtual void Terminate();
   virtual CString GetName();
   virtual CString GetDescription();
   virtual HICON GetIcon();
   virtual bool HasMoreInfo();
   virtual void OnMoreInfo();
};

OBJECT_ENTRY_AUTO(__uuidof(WSDOTComponentInfo), CWSDOTComponentInfo)
