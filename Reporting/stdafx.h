///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#include <PGSuperAll.h>
#include <WBFLSTL.h>

#include <Reporter\Reporter.h>


#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Tools.h>

#include <PgsExt\GirderLabel.h>
#include <PgsExt\ReportPointOfInterest.h>

#include <WBFLGenericBridge.h>
#include <WBFLGenericBridgeTools.h>

#include <initguid.h>
#include <EAF\EAFDisplayUnits.h>

#if defined _NOGRID
#include <NoGrid.h>
#else
#include <grid\gxall.h>
#endif

#include <afxwin.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFHelp.h>
