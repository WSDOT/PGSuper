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
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#pragma once

#include <WbflTypes.h>

/*****************************************************************************
INTERFACE
   IReportOptions


DESCRIPTION
   Interface for getting report options
*****************************************************************************/
// {F840E147-8C06-402B-A02E-EBC0C577EA76}
DEFINE_GUID(IID_IReportOptions,
   0xf840e147,0x8c06,0x402b,0xa0,0x2e,0xeb,0xc0,0xc5,0x77,0xea,0x76);
struct __declspec(uuid("{F840E147-8C06-402B-A02E-EBC0C577EA76}")) IReportOptions;
interface IReportOptions : IUnknown
{
   // global option to include span and girder information when reporting POI locations
   virtual bool IncludeSpanAndGirder4Pois(const CGirderKey& rKey) = 0;
};
