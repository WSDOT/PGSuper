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


#include "psgLibLib.h"
#include <LRFD/RefinedLosses2005.h>
#include <LRFD/BDSManager.h>

class rptChapter;
interface IEAFDisplayUnits;
class pgsLibraryEntryDifferenceItem;
class SpecLibraryEntryImpl;

struct PSGLIBCLASS PrestressLossCriteria
{
   enum class LossMethodType
   {
      AASHTO_REFINED = 0,
      AASHTO_LUMPSUM = 1,
      GENERAL_LUMPSUM = 3, // this option is depreciated - general lump sum was moved from the library to a user input
      WSDOT_LUMPSUM = 4, // same as PPR = 1.0 in aashto eqn's
      AASHTO_LUMPSUM_2005 = 5, // 2005 AASHTO code
      AASHTO_REFINED_2005 = 6, // 2005 AASHTO code
      WSDOT_LUMPSUM_2005 = 7, // 2005 AASHTO, WSDOT (includes initial relaxation loss)
      WSDOT_REFINED_2005 = 8, // 2005 AASHTO, WSDOT (includes initial relaxation loss)
      WSDOT_REFINED = 9,
      TXDOT_REFINED_2004 = 10, // TxDOT's May, 09 decision is to use refined losses from AASHTO 2004
      TXDOT_REFINED_2013 = 11, // TxDOT's Method based on Report No. FHWA/TX-12/0-6374-2
      TIME_STEP = 12, // Losses are computed with a time-step method   
   };

   enum class FcgpMethodType
   {
      // method for computing fcgp - the concrete stress at the center of gravity of prestressing tendons at transfer
      // this method is only used for the TxDOT 2013 loss method
      Assume07fpu, // assume stress is 0.7fpu 
      Iterative, // iterate to find value
      Hybrid // iterate unless initial stress is 0.75fpu, then use 0.7fpu
   };

   enum class TimeDependentConcreteModelType
   {
      AASHTO,
      ACI209,
      CEBFIP
   };

   LossMethodType LossMethod = LossMethodType::AASHTO_REFINED;
   WBFL::LRFD::RefinedLosses2005::RelaxationLossMethod RelaxationLossMethod = WBFL::LRFD::RefinedLosses2005::RelaxationLossMethod::Refined;  // method for computing relaxation losses for LRFD 2005 and later, refined method
   TimeDependentConcreteModelType TimeDependentConcreteModel = TimeDependentConcreteModelType::AASHTO;
   FcgpMethodType FcgpComputationMethod = FcgpMethodType::Assume07fpu; // method for computing fcgp for losses. only used for txdot 2013

   Float64 ShippingLosses = WBFL::Units::ConvertToSysUnits(20, WBFL::Units::Measure::KSI);  // if between -1.0 and 0, shipping loss is fraction of final loss. Fraction is abs(m_ShippingLoss)
   Float64 ShippingTime = WBFL::Units::ConvertToSysUnits(10, WBFL::Units::Measure::Day);


   Float64 SlabElasticGain = 1.0;
   Float64 SlabPadElasticGain = 1.0;
   Float64 DiaphragmElasticGain = 1.0;
   Float64 UserDCElasticGain_BeforeDeckPlacement = 1.0;
   Float64 UserDWElasticGain_BeforeDeckPlacement = 1.0;
   Float64 UserDCElasticGain_AfterDeckPlacement = 1.0;
   Float64 UserDWElasticGain_AfterDeckPlacement = 1.0;
   Float64 RailingSystemElasticGain = 1.0;
   Float64 OverlayElasticGain = 1.0;
   Float64 SlabShrinkageElasticGain = 1.0;
   Float64 LiveLoadElasticGain = 0.0;


   bool Compare(const PrestressLossCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences,bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);

   bool AreElasticGainsApplicable(WBFL::LRFD::BDSManager::Edition edition) const;
   bool IsDeckShrinkageApplicable(WBFL::LRFD::BDSManager::Edition edition) const;
};


inline constexpr auto operator+(PrestressLossCriteria::LossMethodType a) noexcept { return std::underlying_type<PrestressLossCriteria::LossMethodType>::type(a); }
inline constexpr auto operator+(PrestressLossCriteria::TimeDependentConcreteModelType a) noexcept { return std::underlying_type<PrestressLossCriteria::TimeDependentConcreteModelType>::type(a); }
inline constexpr auto operator+(PrestressLossCriteria::FcgpMethodType a) noexcept { return std::underlying_type<PrestressLossCriteria::FcgpMethodType>::type(a); }
