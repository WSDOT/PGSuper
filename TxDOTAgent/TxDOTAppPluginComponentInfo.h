///////////////////////////////////////////////////////////////////////
// BEToolbox
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

#pragma once
#include <EAF\EAFComponentInfo.h>
#include "CLSID.h"
#include "resource.h"

class ATL_NO_VTABLE CTxDOTAppPluginComponentInfo : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CTxDOTAppPluginComponentInfo, &CLSID_TxDOTAppPluginComponentInfo>,
   public IEAFComponentInfo
{
public:
   CTxDOTAppPluginComponentInfo()
   {
   }

DECLARE_REGISTRY_RESOURCEID(IDR_TXDOTAPPPLUGINCOMPONENTINFO)
DECLARE_CLASSFACTORY_SINGLETON(CTxDOTAppPluginComponentInfo)

BEGIN_COM_MAP(CTxDOTAppPluginComponentInfo)
   COM_INTERFACE_ENTRY(IEAFComponentInfo)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CTxDOTAppPluginComponentInfo)
END_CONNECTION_POINT_MAP()

   HRESULT FinalConstruct();
   void FinalRelease();

// IEAFComponentInfo
public:
   virtual BOOL Init(CEAFApp* pApp) override;
   virtual void Terminate() override;
   virtual CString GetName() override;
   virtual CString GetDescription() override;
   virtual HICON GetIcon() override;
   virtual bool HasMoreInfo() override;
   virtual void OnMoreInfo() override;
};

OBJECT_ENTRY_AUTO(__uuidof(TxDOTAppPluginComponentInfo), CTxDOTAppPluginComponentInfo)
