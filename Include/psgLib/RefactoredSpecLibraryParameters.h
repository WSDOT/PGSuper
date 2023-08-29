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
#include <psgLib\OldHaulTruck.h>
#include <LRFD\ILiveLoadDistributionFactor.h>

class SpecLibraryEntryImpl;

/// @brief This class contains parameters that were once part of the 
/// SpecLibraryEntry, but were removed and made part of the main
/// project data. When old SpecLibraryEntry data is loaded, this class
/// captures the refactored data and makes it available to the ProjectAgent
/// so the ProjectAgent can initialize the project data with these parameters.
class PSGLIBCLASS RefactoredSpecLibraryParameters
{
   friend SpecLibraryEntryImpl;

public:
   RefactoredSpecLibraryParameters();

   bool HasLoadFactors() const;
   std::pair<Float64, Float64> GetDCLoadFactors(pgsTypes::LimitState ls) const;
   std::pair<Float64, Float64> GetDWLoadFactors(pgsTypes::LimitState ls) const;
   std::pair<Float64, Float64> GetLLIMLoadFactors(pgsTypes::LimitState ls) const;

   bool HasLumpSumLosses() const;
   Float64 GetBeforeXferLosses() const;
   Float64 GetAfterXferLosses() const;
   Float64 GetLiftingLosses() const;
   Float64 GetShippingLosses() const;
   Float64 GetBeforeTempStrandRemovalLosses() const;
   Float64 GetAfterTempStrandRemovalLosses() const;
   Float64 GetAfterDeckPlacementLosses() const;
   Float64 GetAfterSIDLLosses() const;
   Float64 GetFinalLosses() const;

   bool HasPTParameters() const;
   Float64 GetAnchorSet() const;
   Float64 GetWobbleFrictionCoefficient() const;
   Float64 GetFrictionCoefficient() const;

   bool HasAnalysisType() const;
   pgsTypes::AnalysisType GetAnalysisType() const;

   bool HasLLDFRangeOfApplicabilityAction() const;
   WBFL::LRFD::RangeOfApplicabilityAction GetRangeOfApplicabilityAction() const;

   bool HasOldHaulTruck() const;
   const COldHaulTruck* GetOldHaulTruck() const;

private:
   bool m_bUpdateLoadFactors = false; // true if the load factors are from an old library entry
   std::array<Float64, 6> m_DCmin;   // index is one of pgsTypes::LimitState constants (except for CLLIM)
   std::array<Float64, 6> m_DWmin;
   std::array<Float64, 6> m_LLIMmin;
   std::array<Float64, 6> m_DCmax;
   std::array<Float64, 6> m_DWmax;
   std::array<Float64, 6> m_LLIMmax;

   bool m_bUpdatePTParameters = false;
   Float64 m_Dset = WBFL::Units::ConvertToSysUnits(0.375, WBFL::Units::Measure::Inch); // anchor set
   Float64 m_WobbleFriction = WBFL::Units::ConvertToSysUnits(0.0002, WBFL::Units::Measure::PerFeet); // wobble friction, K
   Float64 m_FrictionCoefficient = 0.25; // mu

   bool m_bUpdateLumpSumLosses = false;
   Float64 m_FinalLosses = 0;
   Float64 m_LiftingLosses = 0;
   Float64 m_BeforeXferLosses = 0;
   Float64 m_AfterXferLosses = 0;
   Float64 m_ShippingLosses = WBFL::Units::ConvertToSysUnits(20, WBFL::Units::Measure::KSI);
   Float64 m_BeforeTempStrandRemovalLosses = 0;
   Float64 m_AfterTempStrandRemovalLosses = 0;
   Float64 m_AfterDeckPlacementLosses = 0;
   Float64 m_AfterSIDLLosses = 0;

   bool m_bHasAnalysisType = false;
   pgsTypes::AnalysisType m_AnalysisType = pgsTypes::Envelope; // this data will be in old library entries (version < 28)

   bool m_bIgnoreRangeOfApplicability = false;  // this will only be found in library entries older than version 29

   bool m_bHasOldHaulTruck = false; // if true, an old spec library entry was read and the hauling truck information is stored in m_OldHaulTruck
   COldHaulTruck m_OldHaulTruck;
};
