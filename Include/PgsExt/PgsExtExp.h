///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSEXT_PGSEXTEXP_H_
#define INCLUDED_PGSEXT_PGSEXTEXP_H_

/*****************************************************************************
LIBRARY
   PGSExt

   Extension class for PGSuper.

DESCRIPTION
   Extension class for PGSuper.  Primarily wrapper class for the structs that
   get passed between interfaces.

COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

// Define BUILDPGSEXTLIB when building this library
// For static builds, define PGSEXTLIB
// For static binding, define PGSEXTLIB
// For dynamic binding, nothing is required to be defined

#if defined (BUILDPGSEXTLIB) && !defined(PGSEXTLIB)
#define PGSEXTCLASS __declspec(dllexport)
#define PGSEXTFUNC  __declspec(dllexport)
#define PGSEXTTPL   template class PGSEXTCLASS
#elif defined(PGSEXTLIB)
#define PGSEXTCLASS
#define PGSEXTFUNC
#define PGSEXTTPL
#else
#define PGSEXTCLASS __declspec(dllimport)
#define PGSEXTFUNC
#define PGSEXTTPL   extern template class PGSEXTCLASS
#endif

#include <WbflAll.h>
#include <PGSuperTypes.h>
#include <PgsExt\AutoLib.h>


#endif // INCLUDED_PGSEXT_PGSEXTEXP_H_