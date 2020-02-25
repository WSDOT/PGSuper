///////////////////////////////////////////////////////////////////////
// IEPluginExample
// Copyright © 1999-2020  Washington State Department of Transportation
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

// PGSpliceProjectImporter.h : Declaration of the CPGSpliceProjectImporter

#pragma once

#include <Plugins\PGSuperIEPlugin.h>
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPGSpliceProjectImporter
class ATL_NO_VTABLE CPGSpliceProjectImporter : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPGSpliceProjectImporter, &CLSID_PGSpliceProjectImporter>,
   public IPGSProjectImporter
{
public:
	CPGSpliceProjectImporter()
	{
	}

   HRESULT FinalConstruct();

   CBitmap m_Bitmap;

DECLARE_REGISTRY_RESOURCEID(IDR_PGSPLICEPROJECTIMPORTER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPGSpliceProjectImporter)
	COM_INTERFACE_ENTRY(IPGSProjectImporter)
END_COM_MAP()

// IPGSProjectImporter
public:
   STDMETHOD(GetItemText)(/*[out,retval]*/BSTR*  bstrText) const override;
   STDMETHOD(GetIcon)(/*[out]*/HICON* phIcon) const override;
   STDMETHOD(Import)(/*[in]*/IBroker* pBroker) override;
};
