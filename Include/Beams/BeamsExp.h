///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

// Define BUILDBEAMSLIB when building this library
// For static builds, define BEAMSLIB
// For static binding, define BEAMSLIB
// For dynamic binding, nothing is required to be defined

#if defined BUILDBEAMSLIB && !defined BEAMSLIB
#define BEAMSCLASS __declspec(dllexport)
#define BEAMSFUNC  __declspec(dllexport)
#define BEAMSTPL   template class BEAMSCLASS 
#elif defined(BEAMSLIB)
#define BEAMSCLASS
#define BEAMSFUNC
#define BEAMSTPL
#else
#define BEAMSCLASS __declspec(dllimport)
#define BEAMSFUNC
#define BEAMSTPL   extern template class BEAMSCLASS 
#endif

#include <Beams\AutoLib.h>
