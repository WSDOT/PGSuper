// CurvelExporter.h : Declaration of the CCurvelExporter

#pragma once

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CCurvelExporter
class ATL_NO_VTABLE CCurvelExporter : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCurvelExporter, &CLSID_CurvelExporter>,
   public IPGSuperDataExporter
{
public:
	CCurvelExporter()
	{
	}

   HRESULT FinalConstruct();

   CBitmap m_Bitmap;

DECLARE_REGISTRY_RESOURCEID(IDR_CURVELEXPORTER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCurvelExporter)
   COM_INTERFACE_ENTRY(IPGSuperDataExporter)
END_COM_MAP()

// IPGSuperDataExporter
public:
   STDMETHOD(Init)(UINT nCmdID);
   STDMETHOD(GetMenuText)(/*[out,retval]*/BSTR*  bstrText);
   STDMETHOD(GetBitmapHandle)(/*[out]*/HBITMAP* phBmp);
   STDMETHOD(GetCommandHintText)(BSTR*  bstrText);
   STDMETHOD(Export)(/*[in]*/IBroker* pBroker);
};

OBJECT_ENTRY_AUTO(__uuidof(CurvelExporter), CCurvelExporter)
