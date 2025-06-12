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

#pragma once

/*****************************************************************************
PsgLib
   Library Management

   Support library that provides library services.

DESCRIPTION
   Support library that provides library services.
*****************************************************************************/

// Define BUILDPsgLibLIB when building this library
// For static builds, define PsgLibLIB
// For static binding, define PsgLibLIB
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

#include <PsgLib\AutoLib.h>


#include <atlbase.h>

#if defined PGS_ASSERT_VALID
#undef PGS_ASSERT_VALID
#endif

#if defined _DEBUG
#define PGS_ASSERT_VALID AssertValid()
#else
#define PGS_ASSERT_VALID
#endif
