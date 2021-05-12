///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include "psgLibLib.h"

class HaulTruckLibraryEntry;

class PSGLIBCLASS COldHaulTruck
{
public:
   COldHaulTruck();

   bool IsEqual(const HaulTruckLibraryEntry* pEntry) const;
   bool IsEqual(Float64 Ktheta,const HaulTruckLibraryEntry* pEntry) const;
   void InitEntry(HaulTruckLibraryEntry* pEntry) const;
   void InitEntry(Float64 Ktheta,HaulTruckLibraryEntry* pEntry) const;

   Float64 m_Hbg; // height from roadway to bottom of girder
   Float64 m_Hrc; // height of roll center above roadway
   Float64 m_Wcc; // center-to-center width of wheels
   Float64 m_Lmax; // maximum span between bunk points
   Float64 m_MaxOH; // maximum leading overhang
   Float64 m_MaxWeight; // maximum weight of girder that can be hauled for this truck

   int m_TruckRollStiffnessMethod; // 0 means direct m_TruckRollStiffness holds the value, otherwise computed
   Float64 m_TruckRollStiffness;
   Float64 m_AxleWeightLimit;
   Float64 m_AxleStiffness;
   Float64 m_MinRollStiffness;
};
