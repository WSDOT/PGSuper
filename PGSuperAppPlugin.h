///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include "PGSuperAppPlugin\PGSuperAppPlugin_i.h"

#include <EAF\EAFAppPlugin.h>
#include <EAF\EAFUIIntegration.h>
#include "PGSuperBaseAppPlugin.h"

#include "PGSuperCommandLineInfo.h"

class CPGSuperAppPlugin;

class CMyCmdTarget : public CCmdTarget
{
public:
   CMyCmdTarget() {};

   afx_msg void OnConfigurePlugins();
   afx_msg void OnUpdateTemplates();
   afx_msg void OnProgramSettings();

   CPGSuperAppPlugin* m_pMyAppPlugin;

   DECLARE_MESSAGE_MAP()
};

//// {C1731920-FBCA-40fa-959D-9B749F03DEC0}
//DEFINE_GUID(CLSID_PGSuperAppPlugin, 
//0xc1731920, 0xfbca, 0x40fa, 0x95, 0x9d, 0x9b, 0x74, 0x9f, 0x3, 0xde, 0xc0);

class ATL_NO_VTABLE CPGSuperAppPlugin : 
   public CPGSuperBaseAppPlugin,
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CPGSuperAppPlugin, &CLSID_PGSuperAppPlugin>,
   public IEAFAppPlugin,
   public IEAFAppCommandLine,
   public IEAFCommandCallback
{
public:
   CPGSuperAppPlugin()
   {
      m_MyCmdTarget.m_pMyAppPlugin = this;

      m_bUpdatingTemplate = false;
   }

BEGIN_COM_MAP(CPGSuperAppPlugin)
   COM_INTERFACE_ENTRY(IEAFAppPlugin)
   COM_INTERFACE_ENTRY(IEAFAppCommandLine)
   COM_INTERFACE_ENTRY(IEAFCommandCallback)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CPGSuperAppPlugin)
END_CONNECTION_POINT_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_PGSUPERAPPPLUGINIMPL)

   HRESULT FinalConstruct();
   void FinalRelease();

private:
   HMENU m_hMenuShared;
   CMyCmdTarget m_MyCmdTarget;
   bool m_bUpdatingTemplate;

public:
   void ConfigurePlugins();
   void UpdateTemplates();
   bool UpdatingTemplates();
   void OnProgramSettings();

protected:
   void Process1250Testing(const CPGSuperCommandLineInfo& rCmdInfo);

// IEAFAppPlugin
public:
   virtual BOOL Init(CEAFApp* pParent);
   virtual void Terminate();
   virtual void IntegrateWithUI(BOOL bIntegrate);
   virtual CEAFDocTemplate* CreateDocTemplate();
   virtual HMENU GetSharedMenuHandle();
   virtual UINT GetDocumentResourceID();
   virtual CString GetName();

// IEAFAppCommandLine
public:
   virtual CString GetUsageMessage();
   virtual BOOL ProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo);

// IEAFCommandCallback
public:
   virtual BOOL OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo);
   virtual void GetStatusBarMessageString(UINT nID, CString& rMessage) const;
   virtual void GetToolTipMessageString(UINT nID, CString& rMessage) const;

};

OBJECT_ENTRY_AUTO(__uuidof(PGSuperAppPlugin), CPGSuperAppPlugin)
