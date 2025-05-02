///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
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

// AnalysisAgent.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f AnalysisAgentps.mk in the project directory.

#include "stdafx.h"
#include <initguid.h>
#include "AnalysisAgent.h"
#include "CLSID.h"

#include <WBFLFem2d_i.c>
#include <WBFLGeometry_i.c>
#include <WBFLCogo_i.c>

#include <WBFLLBAMAnalysisUtility_i.c>
#include <WBFLLBAMAnalysis_i.c>
#include <WBFLLBAMLiveLoader_i.c>
#include <WBFLLBAMLoadCombiner_i.c>
#include <WBFLLBAMUtility_i.c>
#include <WBFLLBAM_i.c>

#include <WBFLTools_i.c>
#include <WBFLUnitServer_i.c>


#include "AnalysisAgentImp.h"

#include "PGSuperCatCom.h"
#include "PGSpliceCatCom.h"
#include <System\ComCatMgr.h>

#include <EAF/EAFStatusCenter.h>
#include <IFace\PointOfInterest.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>
#include <IFace\GirderHandling.h>
#include <IFace\Constructability.h>

#include <EAF/EAFProgress.h>
#include <EAF/EAFStatusCenter.h>

#include <EAF\ComponentModule.h>
WBFL::EAF::ComponentModule _Module;

EAF_BEGIN_OBJECT_MAP(ObjectMap)
	EAF_OBJECT_ENTRY(CLSID_AnalysisAgent, CAnalysisAgentImp)
EAF_END_OBJECT_MAP()


class CAnalysisAgentApp : public CWinApp
{
public:
	virtual BOOL InitInstance() override;
	virtual int ExitInstance() override;
};

CAnalysisAgentApp theApp;

BOOL CAnalysisAgentApp::InitInstance()
{
	_Module.Init(ObjectMap);
	return CWinApp::InitInstance();
}

int CAnalysisAgentApp::ExitInstance()
{
	_Module.Term();
	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry
#pragma Reminder("WORKING HERE - Removing COM - Register Analysis Agent with agent component categories")
//HRESULT RegisterAgent(bool bRegister)
//{
//   HRESULT hr = S_OK;
//   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_AnalysisAgent,CATID_PGSuperAgent,bRegister);
//   if ( FAILED(hr) )
//   {
//      return hr;
//   }
//
//   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_AnalysisAgent,CATID_PGSpliceAgent,bRegister);
//   if ( FAILED(hr) )
//   {
//      return hr;
//   }
//
//   return S_OK;
//}
