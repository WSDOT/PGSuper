///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// Camber multiplication factors
typedef struct CamberMultipliers
{
   Float64 ErectionFactor;   // instantaneous prestress + dead loads at release to erection
   Float64 CreepFactor;      // Factor for all stages of creep deflection
   Float64 DiaphragmFactor;  // Diaphraghm + ShearKey + Construction
   Float64 DeckPanelFactor;  // Deck Panel
   Float64 SlabUser1Factor;  // Slab + User 1
   Float64 SlabPadLoadFactor; // Haunch loading
   Float64 BarrierSwOverlayUser2Factor; // Barrier + Sidewalk + Overlay + User2

   CamberMultipliers(): 
      ErectionFactor(1.0), CreepFactor(1.0), DiaphragmFactor(1.0), DeckPanelFactor(1.0), SlabUser1Factor(1.0), 
      SlabPadLoadFactor(1.0), BarrierSwOverlayUser2Factor(1.0)
   {;}

   bool operator == (const CamberMultipliers& rOther) const
   {
      bool test = true;
      test &= ::IsEqual(ErectionFactor, rOther.ErectionFactor);
      test &= ::IsEqual(CreepFactor,                 rOther.CreepFactor);
      test &= ::IsEqual(DiaphragmFactor,             rOther.DiaphragmFactor);
      test &= ::IsEqual(DeckPanelFactor,             rOther.DeckPanelFactor);
      test &= ::IsEqual(SlabUser1Factor,             rOther.SlabUser1Factor);
      test &= ::IsEqual(SlabPadLoadFactor,            rOther.SlabPadLoadFactor);
      test &= ::IsEqual(BarrierSwOverlayUser2Factor, rOther.BarrierSwOverlayUser2Factor);

      return test;
   }
} CamberMultipliers;
