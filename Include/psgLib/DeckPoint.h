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
#include <PGSuperTypes.h>
#include <PsgLib\DeckRebarData.h>

/*****************************************************************************
CLASS 
   CDeckPoint

   Utility class for describing points along the edge of deck.

DESCRIPTION
   Utility class for describing points along the edge of deck.

LOG
   rab : 06.18.2008 : Created file
*****************************************************************************/
class PSGLIBCLASS CDeckPoint
{
public:
   CDeckPoint();
   CDeckPoint(const CDeckPoint& rOther) = default;
   ~CDeckPoint() = default;

   CDeckPoint& operator = (const CDeckPoint& rOther) = default;
   bool operator == (const CDeckPoint& rOther) const;
   bool operator != (const CDeckPoint& rOther) const;

   bool operator< (const CDeckPoint& rOther) const; // for sorting purposes


   HRESULT Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress);
   HRESULT Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress);

   Float64 Station;   // station where this edge point is measured
   Float64 LeftEdge;  // + = left of measurement datum (even thought this is technically an offset, we are using absolute values to keep things simple for the user)
   Float64 RightEdge; // + = right of measurement datum
   pgsTypes::OffsetMeasurementType MeasurementType; // datum of measurement
   pgsTypes::DeckPointTransitionType LeftTransitionType; // how the deck edge transitions to the next point
   pgsTypes::DeckPointTransitionType RightTransitionType; // how the deck edge transitions to the next point

   Float64 GetWidth() const;
};
