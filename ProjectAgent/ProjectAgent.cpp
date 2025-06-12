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

// ProjectAgent.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f ProjectAgentps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "ProjectAgent.h"

#include "CLSID.h"


#include <WBFLGenericBridge_i.c>

#include <WBFLCogo_i.c>

#include "ProjectAgentImp.h"

#include "PGSuperCatCom.h"
#include "PGSpliceCatCom.h"

#include <IFace/PointOfInterest.h>
#include <IFace/Alignment.h>
#include <IFace\PrestressForce.h>
#include <EAF/EAFStatusCenter.h>
#include <IFace\UpdateTemplates.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Transactions.h>
#include <IFace\DocumentType.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFUIIntegration.h>
#include <EAF/EAFProgress.h>
#include <EAF\EAFStatusCenter.h>

#include <EAF\ComponentModule.h>
WBFL::EAF::ComponentModule Module_;

EAF_BEGIN_OBJECT_MAP(ObjectMap)
	EAF_OBJECT_ENTRY(CLSID_ProjectAgent, CProjectAgentImp)
EAF_END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		Module_.Init(ObjectMap);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
		Module_.Term();
	return TRUE;    // ok
}
