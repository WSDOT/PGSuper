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

#include "PsgLibLib.h"
#include <MathEx.h>
#include <StrData.h>
#include <Materials/Rebar.h>

class GirderLibraryEntry;

/*****************************************************************************
CLASS 
   CLongitudinalRebarData

   Utility class for longitudinal rebar description data.

DESCRIPTION
   Utility class for longitudinal rebar. This class encapsulates 
   the input data a row of rebar and implements the IStructuredLoad 
   and IStructuredSave persistence interfaces.

LOG
   rab : 02.08.2007 : Created file
*****************************************************************************/

class PSGLIBCLASS CLongitudinalRebarData
{
public:
   class PSGLIBCLASS RebarRow 
   {
   public:
      pgsTypes::RebarLayoutType BarLayout;
      Float64 DistFromEnd; // Only applicable to blFromLeft, blFromRight
      Float64 BarLength; //   Applicable to blFromLeft, blFromRight, blMidGirderLength, and blMidGirderEnds

      bool bExtendedLeft, bExtendedRight; // indicates the bars are extended out of the face of the girder. extended bars are assumed to be fully developed
      // only applicable for BarLayout = blFullLength and blFromLeft and blFromRight when DistFromEnd is 0.

      pgsTypes::FaceType  Face;
      WBFL::Materials::Rebar::Size BarSize;
      IndexType NumberOfBars;
      Float64     Cover;
      Float64     BarSpacing;

      RebarRow():
         Face(pgsTypes::TopFace), BarSize(WBFL::Materials::Rebar::Size::bsNone), NumberOfBars(0), Cover(0), BarSpacing(0),
         BarLayout(pgsTypes::blFullLength), DistFromEnd(0), BarLength(0), bExtendedLeft(false), bExtendedRight(false)
      {;}

      bool operator==(const RebarRow& other) const;

      // Get locations of rebar start and end measured from left end of segment
      // given a segment length. Return false if entire bar is outside of segment.
      bool GetRebarStartEnd(Float64 segmentLength, Float64* pBarStart, Float64* pBarEnd) const;

      bool IsLeftEndExtended() const;
      bool IsRightEndExtended() const;
   };

   WBFL::Materials::Rebar::Type BarType;
   WBFL::Materials::Rebar::Grade BarGrade;
   std::vector<RebarRow> RebarRows;


   CLongitudinalRebarData();
   CLongitudinalRebarData(const CLongitudinalRebarData& rOther) = default;
   ~CLongitudinalRebarData();

   CLongitudinalRebarData& operator = (const CLongitudinalRebarData& rOther) = default;
   bool operator == (const CLongitudinalRebarData& rOther) const;
   bool operator != (const CLongitudinalRebarData& rOther) const;

	HRESULT Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress);
	HRESULT Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress);

   // copy shear data from a girder entry
   void CopyGirderEntryData(const GirderLibraryEntry* pGirderEntry);
};
