///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSEXT_HANDLINGDATA_H_
#define INCLUDED_PGSEXT_HANDLINGDATA_H_
#pragma once

#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <WBFLCore.h>
#include <PsgLib\HaulTruckLibraryEntry.h>

/*****************************************************************************
CLASS 
   CHandlingData

   Utility class for lifting and transportation handling data.

DESCRIPTION
   Utility class for lifting and transportation handling data.

LOG
   rab : 05.27.208 : Created file
*****************************************************************************/

class PGSEXTCLASS CHandlingData
{
public:
   CHandlingData();
   CHandlingData(const CHandlingData& rOther);
   ~CHandlingData();

   CHandlingData& operator = (const CHandlingData& rOther);
   bool operator==(const CHandlingData& rOther) const;
   bool operator!=(const CHandlingData& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   Float64 LeftReleasePoint, RightReleasePoint; // dist from left/right end of segment to support point at release (-1==END, -2==BRG)
   Float64 LeftStoragePoint, RightStoragePoint; // distance from left/right end of segment to storage support point (-1==BRG)
   Float64 LeftLiftPoint, RightLiftPoint; // distance from left/right end of segment to lift point
   Float64 LeadingSupportPoint, TrailingSupportPoint;
   std::_tstring HaulTruckName;
   const HaulTruckLibraryEntry* pHaulTruckLibraryEntry;


#if defined _DEBUG
   void AssertValid();
#endif

protected:
   void MakeCopy(const CHandlingData& rOther);
   void MakeAssignment(const CHandlingData& rOther);
};

#endif // INCLUDED_PGSEXT_HANDLINGDATA_H_
