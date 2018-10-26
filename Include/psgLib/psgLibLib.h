///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#ifndef INCLUDED_PSGLIB_PSGLIBLIB_H_
#define INCLUDED_PSGLIB_PSGLIBLIB_H_

/*****************************************************************************
PSGLIB
   Library Management

   Support library that provides library services.

DESCRIPTION
   Support library that provides library services.

COPYRIGHT
   Copyright (c) 1997
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

// Define BUILDPSGLIBLIB when building this library
// For static builds, define PSGLIBLIB
// For static binding, define PSGLIBLIB
// For dynamic binding, nothing is required to be defined

#if defined (BUILDPSGLIBLIB) && !defined(PSGLIBLIB)
#define PSGLIBCLASS __declspec(dllexport)
#define PSGLIBFUNC  __declspec(dllexport)
#define PSGLIBTPL   template class PSGLIBCLASS
#elif defined(PSGLIBLIB)
#define PSGLIBCLASS
#define PSGLIBFUNC
#define PSGLIBTPL
#else
#define PSGLIBCLASS __declspec(dllimport)
#define PSGLIBFUNC
#define PSGLIBTPL   extern template class PSGLIBCLASS
#endif

#if !defined INCLUDED_PSGLIB_AUTOLIB_H_
#include <PsgLib\AutoLib.h>
#endif


#include <atlbase.h>

#endif // INCLUDED_PSGLIB_PSGLIBLIB_H_