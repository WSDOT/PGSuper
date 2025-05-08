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

#include <PsgLib\Keys.h>

#define CAD_SUCCESS  0
#define CAD_FAIL    -1

/*****************************************************************************
INTERFACE
   ITestFileExport

DESCRIPTION
   This interface exposes the test file Export functions
*****************************************************************************/
// {8D0B5EB4-B255-4fba-8ADB-4DB2E3A6B1D5}
DEFINE_GUID(IID_ITestFileExport, 
0x8d0b5eb4, 0xb255, 0x4fba, 0x8a, 0xdb, 0x4d, 0xb2, 0xe3, 0xa6, 0xb1, 0xd5);
class ITestFileExport
{
public:
   virtual int WriteCADDataToFile (FILE *fp, const CSegmentKey& segmentKey, bool designSucceeded) = 0;
   virtual int WriteDistributionFactorsToFile (FILE *fp, const CSegmentKey& segmentKey) = 0;
};

