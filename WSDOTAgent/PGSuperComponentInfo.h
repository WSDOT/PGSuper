///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include "CLSID.h"

class ATL_NO_VTABLE CPGSuperComponentInfo : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CPGSuperComponentInfo, &CLSID_PGSuperComponentInfo>,
   public IPGSuperComponentInfo
{
public:
   CPGSuperComponentInfo()
   {
   }

DECLARE_REGISTRY_RESOURCEID(IDR_PGSUPERCOMPONENTINFO)
DECLARE_CLASSFACTORY_SINGLETON(CPGSuperComponentInfo)

BEGIN_COM_MAP(CPGSuperComponentInfo)
   COM_INTERFACE_ENTRY(IPGSuperComponentInfo)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CPGSuperComponentInfo)
END_CONNECTION_POINT_MAP()

   HRESULT FinalConstruct();
   void FinalRelease();

// IPGSuperComponentInfo
public:
   virtual BOOL Init(CPGSuperDoc* pDoc) override;
   virtual void Terminate() override;
   virtual CString GetName() const override;
   virtual CString GetDescription() const override;
   virtual HICON GetIcon() const override;
   virtual bool HasMoreInfo() const override;
   virtual void OnMoreInfo() const override;
};

OBJECT_ENTRY_AUTO(__uuidof(PGSuperComponentInfo), CPGSuperComponentInfo)
