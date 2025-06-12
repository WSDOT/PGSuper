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

// EngAgent.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f EngAgentps.mk in the project directory.

#include "stdafx.h"
#include <initguid.h>
#include "EngAgent.h"

#include "CLSID.h"

#include "EngAgentImp.h"

#include "PGSuperCatCom.h"
#include "PGSpliceCatCom.h"

#include <IFace\PrecastIGirderDetailsSpec.h>
#include <IFace\Project.h>
#include <EAF/EAFStatusCenter.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h>
#include <IFace\RatingSpecification.h>
#include <IFace\ResistanceFactors.h>
#include <IFace\InterfaceShearRequirements.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>
#include <IFace\Views.h>
#include <IFace\SplittingChecks.h>


#include <WBFLGeometry_i.c>
#include <WBFLFem2d_i.c>
#include <WBFLRCCapacity_i.c> 

#include <EAF\ComponentModule.h>

WBFL::EAF::ComponentModule _Module;

EAF_BEGIN_OBJECT_MAP(ObjectMap)
	EAF_OBJECT_ENTRY(CLSID_EngAgent, CEngAgentImp)
EAF_END_OBJECT_MAP()

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
   if (dwReason == DLL_PROCESS_ATTACH)
   {
      _Module.Init(ObjectMap);
   }
   else if (dwReason == DLL_PROCESS_DETACH)
      _Module.Term();
   return TRUE;    // ok
}
