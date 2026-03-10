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
#include <StrData.h>
#include <Materials/Rebar.h>

/*****************************************************************************
CLASS 
   CDeckRebarData

   Utility class for deck reinforcement description data.

DESCRIPTION
   Utility class for deck reinforcement description data. This class encapsulates all
   the input data for deck reinforcement and implements the IStructuredLoad and 
   IStructuredSave persistence interfaces.

LOG
   rab : 01.11.2007 : Created file
*****************************************************************************/
class PSGLIBCLASS CDeckRebarData
{
public:
   Float64 TopCover;
   Float64 BottomCover;

   WBFL::Materials::Rebar::Type TopRebarType;
   WBFL::Materials::Rebar::Grade TopRebarGrade;
   WBFL::Materials::Rebar::Size TopRebarSize;

   WBFL::Materials::Rebar::Type BottomRebarType;
   WBFL::Materials::Rebar::Grade BottomRebarGrade;
   WBFL::Materials::Rebar::Size BottomRebarSize;

   Float64 TopSpacing;
   Float64 BottomSpacing;

   Float64 TopLumpSum;
   Float64 BottomLumpSum;

   enum RebarMat { TopMat, BottomMat };
   struct NegMomentRebarData
   {
      WBFL::Materials::Rebar::Type RebarType;
      WBFL::Materials::Rebar::Grade RebarGrade;
      WBFL::Materials::Rebar::Size RebarSize;

      PierIndexType PierIdx;
      RebarMat Mat;
      Float64 LumpSum;
      Float64 Spacing;
      Float64 LeftCutoff;
      Float64 RightCutoff;

      bool operator!=(const NegMomentRebarData& rOther) const;
      bool operator==(const NegMomentRebarData& rOther) const;
   };
   std::vector<NegMomentRebarData> NegMomentRebar;

   CDeckRebarData();
   CDeckRebarData(const CDeckRebarData& rOther) = default;
   ~CDeckRebarData() = default;

   CDeckRebarData& operator = (const CDeckRebarData& rOther) = default;

   bool operator!=(const CDeckRebarData& rOther) const;
   bool operator==(const CDeckRebarData& rOther) const;

	HRESULT Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress);
	HRESULT Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress);

   std::vector<CDeckRebarData::NegMomentRebarData> GetSupplementalReinforcement(PierIndexType pierIdx) const;
};
