///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

// TestAgentImp.h : Declaration of the CTestAgentImp

#ifndef __TESTAGENT_H_
#define __TESTAGENT_H_

#include "resource.h"       // main symbols

#include <IFace\Project.h>
#include <IFace\DistributionFactors.h>
#include <IFace\GirderHandlingSpecCriteria.h>

#include <EAF\EAFInterfaceCache.h>

/////////////////////////////////////////////////////////////////////////////
// CTestAgentImp
class ATL_NO_VTABLE CTestAgentImp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTestAgentImp, &CLSID_TestAgent>,
	public IConnectionPointContainerImpl<CTestAgentImp>,
	public IAgentEx,
   public ITest1250
{
public:
	CTestAgentImp()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TESTAGENT)
DECLARE_NOT_AGGREGATABLE(CTestAgentImp)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTestAgentImp)
	COM_INTERFACE_ENTRY(IAgent)
   COM_INTERFACE_ENTRY(IAgentEx)
	COM_INTERFACE_ENTRY(ITest1250)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CTestAgentImp)
END_CONNECTION_POINT_MAP()


// IAgentEx
public:
	STDMETHOD(SetBroker)(IBroker* pBroker);
   STDMETHOD(RegInterfaces)();
	STDMETHOD(Init)();
	STDMETHOD(Reset)();
	STDMETHOD(ShutDown)();
   STDMETHOD(Init2)();
   STDMETHOD(GetClassID)(CLSID* pCLSID);

// ITest1250
   virtual bool RunTest(long  type,
                        const std::_tstring& outputFileName,
                        const std::_tstring& poiFileName);

   virtual bool RunTestEx(long  type,const std::vector<SpanGirderHashType>& girderList,
                          const std::_tstring& outputFileName,
                          const std::_tstring& poiFileName);

private:
   DECLARE_EAF_AGENT_DATA;

   std::_tstring GetBridgeID();
   std::_tstring GetProcessID();

   bool RunHaunchTest(std::_tofstream& resultsFile, std::_tofstream& poiFile,const CSegmentKey& segmentKey);
   bool RunGeometryTest(std::_tofstream& resultsFile, std::_tofstream& poiFile,const CSegmentKey& segmentKey);
   bool RunDistFactorTest(std::_tofstream& resultsFile, std::_tofstream& poiFile,const CSegmentKey& segmentKey);
   bool RunHL93Test(std::_tofstream& resultsFile, std::_tofstream& poiFile,const CSegmentKey& segmentKey);
   bool RunCrossSectionTest(std::_tofstream& resultsFile, std::_tofstream& poiFile,const CSegmentKey& segmentKey);
   bool RunDeadLoadActionTest(std::_tofstream& resultsFile, std::_tofstream& poiFile,const CSegmentKey& segmentKey);
   bool RunCombinedLoadActionTest(std::_tofstream& resultsFile, std::_tofstream& poiFile,const CSegmentKey& segmentKey);
   bool RunPrestressedISectionTest(std::_tofstream& resultsFile, std::_tofstream& poiFile,const CSegmentKey& segmentKey);
   bool RunHandlingTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CSegmentKey& segmentKey);
   bool RunWsdotGirderScheduleTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CSegmentKey& segmentKey);
   bool RunDesignTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CSegmentKey& segmentKey);
   bool RunCamberTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CSegmentKey& segmentKey);
   bool RunFabOptimizationTest(std::_tofstream& resultsFile,std::_tofstream& poiFile,const CSegmentKey& segmentKey);
   bool RunLoadRatingTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CGirderKey& girderKey);
};

#endif //__TESTAGENT_H_
