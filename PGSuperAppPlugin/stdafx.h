///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef STRICT
#define STRICT
#endif

#include <PGSuperAll.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>

#include <IFace\Tools.h>

#if defined _NOGRID
#include <NoGrid.h>
#else
#include <grid\gxall.h>
#endif

#include <WBFLAtlExt.h>

#include <WBFLCore.h>
#include <WBFLTools.h>
#include <WBFLGeometry.h>
#include <WBFLCogo.h>
#include <WBFLGenericBridge.h>
#include <WBFLSTL.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderLabel.h>
#include <afxwin.h>

#include <EAF\EAFUtilities.h> // so all files have EAFGetBroker
#include <EAF\EAFResources.h> // so all files have EAF resource identifiers
#include <EAF\EAFHints.h>     // so all files have EAF Doc/View hints
#include <EAF\EAFHelp.h>

#include <afxdlgs.h>

#include "..\MakePgz\zip.h"
#include "..\MakePgz\unzip.h"

#include "..\Documentation\PGSuper.hh"

#include <MFCTools\MFCTools.h>

using namespace ATL;
