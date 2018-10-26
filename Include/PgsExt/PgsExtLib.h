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

#ifndef INCLUDED_PGSEXT_PGSEXTLIB_H_
#define INCLUDED_PGSEXT_PGSEXTLIB_H_

#include <PGSuperAll.h>
#include <WBFLAtlExt.h>

#include <atlbase.h>

#include <Units\Units.h>
#include <System\System.h>
#include <LRFD\LRFD.h>

#include <IFace\Tools.h>
#include <EAF\EAFUtilities.h>
#include <EAF\EAFHelp.h>

#include <grid\gxall.h>
#include <MFCTools\MFCTools.h>

#include <MathEx.h>

#if defined PGS_ASSERT_VALID
#undef PGS_ASSERT_VALID
#endif

#if defined _DEBUG
#define PGS_ASSERT_VALID AssertValid()
#else
#define PGS_ASSERT_VALID
#endif

#endif // INCLUDED_PGSEXT_PGSEXTLIB_H_