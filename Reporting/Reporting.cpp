///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// Reporting.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "Reporting.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <initguid.h>
// Put an include statement here for every interface used in this agent.
// The #include "initguid.h" statement above will cause the IID's to be
// resolved in this DLL
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Alignment.h>

#include <IFace\AnalysisResults.h>
#include <IFace\PrestressForce.h>
#include <IFace\Artifact.h>
#include <IFace\DistributionFactors.h>
#include <IFace\MomentCapacity.h>
#include <IFace\ShearCapacity.h>
#include <IFace\Constructability.h>
#include <IFace\TransverseReinforcementSpec.h>
#include <IFace\PrecastIGirderDetailsSpec.h>
#include <IFace\VersionInfo.h>
#include <IFace\PointOfInterest.h>
#include <IFace\GirderHandling.h>
#include <IFace\Allowables.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\StatusCenter.h>
#include <IFace\Selection.h>
#include <IFace\RatingSpecification.h>
#include <IFace\CrackedSection.h>
#include <EAF\EAFUIIntegration.h>
#include <EAF\EAFDisplayUnits.h>

#include <WBFLCogo.h>
#include <WBFLCogo_i.c>
#include <WBFLCore_i.c>
#include <WBFLGenericBridge_i.c>
#include <WBFLGenericBridgeTools_i.c>

#include <PgsExt\ReportStyleHolder.h>

//
//TODO: If this DLL is dynamically linked against the MFC DLLs,
//		any functions exported from this DLL which call into
//		MFC must have the AFX_MANAGE_STATE macro added at the
//		very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

// CReportingApp

BEGIN_MESSAGE_MAP(CReportingApp, CWinApp)
END_MESSAGE_MAP()


// CReportingApp construction

CReportingApp::CReportingApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CReportingApp object

CReportingApp theApp;


// CReportingApp initialization

BOOL CReportingApp::InitInstance()
{
   GXInit();

   CWinApp::InitInstance();

   // Initialize the report styles when the DLL loads
   pgsReportStyleHolder::InitStyles();

	return TRUE;
}

int CReportingApp::ExitInstance() 
{
   GXForceTerminate();
	return CWinApp::ExitInstance();
}