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
// 4500 3rd AVE SE - P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#pragma once

#ifndef STRICT
#define STRICT
#endif

#define VC_EXTRALEAN
#define COM_STDMETHOD_CAN_THROW

#define _USE_MULTITHREADING    // When defined, multi-threading is used
//#define _REDUCE_POI            // When defined, the number of POIs is reduced

#include <PGSuperVersion.h>

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxpriv.h>        // Private MFC extensions
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC OLE automation classes

#include <atlbase.h>
#include <atlcom.h>
//#include <atlctl.h>

using namespace ATL;

// If this is defined, average tendon forces, after friction and anchor set
// are used for the stress analysis
//#define USE_AVERAGE_TENDON_FORCE

#include <WBFLAll.h>
#include <WBFLAtlExt.h>
#include <PGSuperTypes.h>
#include <PGSuperDebug.h>
#include <PGSuperUnits.h>

