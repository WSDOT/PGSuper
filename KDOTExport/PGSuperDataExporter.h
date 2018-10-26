// PGSuperExporter.h : Declaration of the CPGSuperExporter

#ifndef __KDOTEXPORTER_H_
#define __KDOTEXPORTER_H_

#include "resource.h"       // main symbols

#include <PgsExt\BridgeDescription2.h>
#include <IFace\AnalysisResults.h>
#include "KDOTExporter.hxx"


interface IShapes;
class LiveLoadLibraryEntry;

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDataExporter
class ATL_NO_VTABLE CPGSuperDataExporter : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPGSuperDataExporter, &CLSID_PGSuperDataExporter>,
   public IPGSDataExporter,
   public IPGSDocumentation
{
public:
	CPGSuperDataExporter()
	{
	}

   HRESULT FinalConstruct();

   CBitmap m_bmpLogo;

DECLARE_REGISTRY_RESOURCEID(IDR_PGSUPERDATAEXPORTER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPGSuperDataExporter)
   COM_INTERFACE_ENTRY(IPGSDataExporter)
   COM_INTERFACE_ENTRY(IPGSDocumentation)
END_COM_MAP()

// IPGSDataExporter
public:
   STDMETHOD(Init)(UINT nCmdID) override;
   STDMETHOD(GetMenuText)(/*[out,retval]*/BSTR*  bstrText) override;
   STDMETHOD(GetBitmapHandle)(/*[out]*/HBITMAP* phBmp) override;
   STDMETHOD(GetCommandHintText)(BSTR*  bstrText) override;
   STDMETHOD(Export)(/*[in]*/IBroker* pBroker) override;

// IPGSDocumentation
public:
   STDMETHOD(GetDocumentationSetName)(BSTR* pbstrName) override;
   STDMETHOD(LoadDocumentationMap)() override;
   STDMETHOD(GetDocumentLocation)(UINT nHID,BSTR* pbstrURL) override;

private:
   Float64 m_BearingHeight;
   HRESULT Export(IBroker* pBroker,CString& strFileName, const std::vector<CGirderKey>& girderKeys);

   std::map<UINT,CString> m_HelpTopics;
   CString GetDocumentationURL();
};

#endif //__PGSUPEREXPORTER_H_
