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
   PGSuperGraphing

DESCRIPTION
   This library encapsulates the graph builders and exports
   them so that Extension Agents can incorporate standard graphs into graphing views.
*****************************************************************************/

// Define BUILDGRAPHINGLIB when building this library
// For static builds, define GRAPHINGLIB
// For static binding, define GRAPHINGLIB
// For dynamic binding, nothing is required to be defined

#if defined (BUILDGRAPHINGLIB) && !defined(GRAPHGLIB)
#define GRAPHINGCLASS __declspec(dllexport)
#define GRAPHINGFUNC  __declspec(dllexport)
#define GRAPHINGTPL   template class GRAPHINGCLASS
#elif defined(GRAPHINGLIB)
#define GRAPHINGCLASS
#define GRAPHINGFUNC
#define GRAPHINGTPL
#else
#define GRAPHINGCLASS __declspec(dllimport)
#define GRAPHINGFUNC
#define GRAPHINGTPL   extern template class GRAPHINGCLASS
#endif

#include <Graphing\AutoLib.h>
