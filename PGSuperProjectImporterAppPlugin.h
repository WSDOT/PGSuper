#pragma once
#include <EAF\EAFAppPlugin.h>
#include "PGSuperBaseAppPlugin.h"

// {552AB673-3D3D-47e6-AFD6-4EA033A8A7F2}
DEFINE_GUID(CLSID_PGSuperProjectImporterDocumentPlugin, 
0x552ab673, 0x3d3d, 0x47e6, 0xaf, 0xd6, 0x4e, 0xa0, 0x33, 0xa8, 0xa7, 0xf2);

class ATL_NO_VTABLE CPGSuperProjectImporterAppPlugin : 
   public CPGSuperBaseAppPlugin,
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CPGSuperProjectImporterAppPlugin, &CLSID_PGSuperProjectImporterDocumentPlugin>,
   public IEAFAppPlugin
{
public:
   CPGSuperProjectImporterAppPlugin()
   {
   }

   HRESULT FinalConstruct();
   void FinalRelease();

BEGIN_COM_MAP(CPGSuperProjectImporterAppPlugin)
   COM_INTERFACE_ENTRY(IEAFAppPlugin)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CPGSuperProjectImporterAppPlugin)
END_CONNECTION_POINT_MAP()

   HMENU m_hMenuShared;

// IEAFAppPlugin
public:
   virtual BOOL Init(CEAFApp* pParent);
   virtual void Terminate();
   virtual void IntegrateWithUI(BOOL bIntegrate);
   virtual CEAFDocTemplate* CreateDocTemplate();
   virtual HMENU GetSharedMenuHandle();
   virtual UINT GetDocumentResourceID();
   virtual CString GetName();
};
