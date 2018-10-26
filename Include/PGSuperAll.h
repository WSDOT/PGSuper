///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSUPERALL_H_
#define INCLUDED_PGSUPERALL_H_
#pragma once

#ifndef _PGSUPER_VERSION
#define _PGSUPER_VERSION 230 // version 2.3.x
#endif

// This header file must be included in the precompiled header
// for all PGSuper packages

// target platform is Win2000 or later
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#define VC_EXTRALEAN
#define STRICT

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>
#include <afxpriv.h>        // Private MFC extensions

#include <PGSuperTypes.h>
#include <PGSuperDebug.h>

#endif // INCLUDED_PGSUPERDEBUG_H_