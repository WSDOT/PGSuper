///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <System\dllTest.h>

#include <initguid.h>
#include <IFace\Project.h>
#include <IFace\VersionInfo.h>
#include <IFace\StatusCenter.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\EditByUI.h>
#include <EAF\EAFTransactions.h>
#include <IFace\Intervals.h>
#include <IFace\BeamFactory.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\GirderHandling.h>
#include <IFace\Allowables.h>
#include <IFace\DocumentType.h>
#include <IFace\TransverseReinforcementSpec.h>
#include <IFace\PrestressForce.h>
#include <IFace\SplittingChecks.h>
#include <IFace/Alignment.h>

#include <EAF\EAFDisplayUnits.h>

#include <WBFLCogo.h>
#include <WBFLCogo_i.c>

#include <WBFLFem2d.h>
#include <WBFLFem2d_i.c>

#include <WBFLTools.h>
#include <WBFLTools_i.c>

// must include these files. these files defined abstract class
// and they must be included so the compiler generated default
// methods (constructors, etc.) get generated
#include <PgsExt\TransferLength.h>
#include <PgsExt\DevelopmentLength.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//// Every DLL has an entry point DllEntryPoint
//BOOL WINAPI DllEntryPoint( HINSTANCE /*hinstDll*/,
//                           DWORD     /*fdwRreason*/,
//                           LPVOID    /*plvReserved*/)
//{
//    return 1;   // Indicate that the DLL was initialized successfully.
//}
///*
//// call unit test routines for all packages belonging to this dll.
//bool WINAPI UnitTest(WBFL::Debug::Log& rlog)
//{
//   return CUnitTest::TestMe(rlog);
//}
//*/


/////////////////////////////////////////////////////////////////////////////
// CPGSuperExt
// See main.cpp for the implementation of this class
//

class CPGSuperExt : public CWinApp
{
public:
	CPGSuperExt();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPGSuperExt)
	public:
	virtual BOOL InitInstance() override;
	virtual int ExitInstance() override;
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CPGSuperExt)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CPGSuperExt

BEGIN_MESSAGE_MAP(CPGSuperExt, CWinApp)
	//{{AFX_MSG_MAP(CPGSuperExt)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPGSuperExt construction

CPGSuperExt::CPGSuperExt()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPGSuperExt object

CPGSuperExt theApp;

/////////////////////////////////////////////////////////////////////////////
// CPGSuperExt initialization

BOOL CPGSuperExt::InitInstance()
{
   ::GXInit();
   CWinApp::InitInstance();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Special entry points required for inproc servers

int CPGSuperExt::ExitInstance() 
{
	return CWinApp::ExitInstance();
}
