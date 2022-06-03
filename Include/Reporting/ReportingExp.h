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

#pragma once

/*****************************************************************************
LIBRARY
   PGSuperReporting

DESCRIPTION
   This library encapsulates the report chapters and utility classes and exports
   them so that Extension Agents can incorporate standard chapters, paragraphs, and tables
   into their custom reports.
*****************************************************************************/

// Define BUILDREPORTINGLIB when building this library
// For static builds, define REPORTINGLIB
// For static binding, define REPORTINGLIB
// For dynamic binding, nothing is required to be defined

#if defined (BUILDREPORTINGLIB) && !defined(REPORTINGLIB)
#define REPORTINGCLASS __declspec(dllexport)
#define REPORTINGFUNC  __declspec(dllexport)
#define REPORTINGTPL   template class REPORTINGCLASS
#elif defined(REPORTINGLIB)
#define REPORTINGCLASS
#define REPORTINGFUNC
#define REPORTINGTPL
#else
#define REPORTINGCLASS __declspec(dllimport)
#define REPORTINGFUNC
#define REPORTINGTPL   extern template class REPORTINGCLASS
#endif

#include <Reporting\AutoLib.h>

#define RPT_E_INVALID_SPAN        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,512)
#define RPT_E_INVALID_GROUP       MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,513)
#define RPT_E_INVALID_GIRDER      MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,514)
#define RPT_E_INVALID_SEGMENT     MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,515)
