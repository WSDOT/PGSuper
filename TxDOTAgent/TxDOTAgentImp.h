///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// TxDOTAgentImp.h : Declaration of the CTxDOTAgentImp

#pragma once
#include "resource.h"       // main symbols

#include "CLSID.h"

#include <EAF\EAFInterfaceCache.h>
#include <EAF\EAFUIIntegration.h>

#include <IFace\TxDOTCadExport.h>
#include "TxDOTCommandLineInfo.h"

#include <PgsExt\GirderDesignArtifact.h>


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif



// CTxDOTAgentImp

class ATL_NO_VTABLE CTxDOTAgentImp :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTxDOTAgentImp, &CLSID_TxDOTAgent>,
	public IAgentEx,
   public IEAFProcessCommandLine,
   public ITxDOTCadExport
{
public:
	CTxDOTAgentImp()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TXDOTAGENTIMP)

DECLARE_NOT_AGGREGATABLE(CTxDOTAgentImp)

BEGIN_COM_MAP(CTxDOTAgentImp)
	COM_INTERFACE_ENTRY(IAgentEx)
   COM_INTERFACE_ENTRY(ITxDOTCadExport)
   COM_INTERFACE_ENTRY(IEAFProcessCommandLine)
END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

// IAgentEx
public:
	STDMETHOD(SetBroker)(IBroker* pBroker) override;
	STDMETHOD(RegInterfaces)() override;
	STDMETHOD(Init)() override;
	STDMETHOD(Reset)() override;
	STDMETHOD(ShutDown)() override;
   STDMETHOD(Init2)() override;
   STDMETHOD(GetClassID)(CLSID* pCLSID) override;

// ITxDOTCadExport
public:
   virtual int WriteCADDataToFile (FILE *fp, IBroker* pBroker, const CSegmentKey& segmentKey, TxDOTCadExportFormatType format, bool designSucceeded) override;
   virtual int WriteDistributionFactorsToFile (FILE *fp, IBroker* pBroker, const CSegmentKey& segmentKey) override;

// IEAFProcessCommandLine
public:
   virtual BOOL ProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo) override;

protected:
   void ProcessTxDotCad(const CTxDOTCommandLineInfo& rCmdInfo);
   bool CreateTxDOTFileNames(const CString& output, CString* pErrFileName);
   bool DoTxDotCadReport(const CString& outputFileName, const CString& errorFileName, const CTxDOTCommandLineInfo& txInfo);
   void SaveFlexureDesign(const CSegmentKey& segmentKey,const pgsSegmentDesignArtifact* pArtifact);

   void ProcessTOGAReport(const CTxDOTCommandLineInfo& rCmdInfo);
   bool DoTOGAReport(const CString& outputFileName, const CTxDOTCommandLineInfo& txInfo);

private:
   DECLARE_EAF_AGENT_DATA;
   DECLARE_LOGFILE;
};

OBJECT_ENTRY_AUTO(__uuidof(TxDOTAgent), CTxDOTAgentImp)
