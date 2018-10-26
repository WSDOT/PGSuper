#pragma once
#include <EAF\EAFAppPlugin.h>
#include "resource.h"

class ATL_NO_VTABLE CLibraryAppPlugin : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CLibraryAppPlugin, &CLSID_LibraryAppPlugin>,
   public IEAFAppPlugin
{
public:
   CLibraryAppPlugin()
   {
   }

DECLARE_REGISTRY_RESOURCEID(IDR_LIBAPPPLUGIN)

BEGIN_COM_MAP(CLibraryAppPlugin)
   COM_INTERFACE_ENTRY(IEAFAppPlugin)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CLibraryAppPlugin)
END_CONNECTION_POINT_MAP()

// IEAFAppPlugin
public:
   virtual BOOL Init(CEAFApp* pParent);
   virtual void Terminate();
   virtual CEAFDocTemplate* CreateDocTemplate();
   virtual CCmdTarget* GetCommandTarget();
   virtual HMENU GetSharedMenuHandle();
   virtual UINT GetDocumentResourceID();
   virtual CString GetName();
};


OBJECT_ENTRY_AUTO(__uuidof(LibraryAppPlugin), CLibraryAppPlugin)
