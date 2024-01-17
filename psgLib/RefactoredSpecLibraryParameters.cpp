///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include "StdAfx.h"
#include <psgLib/RefactoredSpecLibraryParameters.h>

RefactoredSpecLibraryParameters::RefactoredSpecLibraryParameters()
{
   m_bUpdateLoadFactors = false;
   m_DCmin[pgsTypes::ServiceI] = 1.0;         m_DCmax[pgsTypes::ServiceI] = 1.0;
   m_DWmin[pgsTypes::ServiceI] = 1.0;         m_DWmax[pgsTypes::ServiceI] = 1.0;
   m_LLIMmin[pgsTypes::ServiceI] = 1.0;         m_LLIMmax[pgsTypes::ServiceI] = 1.0;

   m_DCmin[pgsTypes::ServiceIA] = 0.5;        m_DCmax[pgsTypes::ServiceIA] = 0.5;
   m_DWmin[pgsTypes::ServiceIA] = 0.5;        m_DWmax[pgsTypes::ServiceIA] = 0.5;
   m_LLIMmin[pgsTypes::ServiceIA] = 1.0;        m_LLIMmax[pgsTypes::ServiceIA] = 1.0;

   m_DCmin[pgsTypes::ServiceIII] = 1.0;       m_DCmax[pgsTypes::ServiceIII] = 1.0;
   m_DWmin[pgsTypes::ServiceIII] = 1.0;       m_DWmax[pgsTypes::ServiceIII] = 1.0;
   m_LLIMmin[pgsTypes::ServiceIII] = 0.8;       m_LLIMmax[pgsTypes::ServiceIII] = 0.8;

   m_DCmin[pgsTypes::StrengthI] = 0.90;       m_DCmax[pgsTypes::StrengthI] = 1.25;
   m_DWmin[pgsTypes::StrengthI] = 0.65;       m_DWmax[pgsTypes::StrengthI] = 1.50;
   m_LLIMmin[pgsTypes::StrengthI] = 1.75;       m_LLIMmax[pgsTypes::StrengthI] = 1.75;

   m_DCmin[pgsTypes::StrengthII] = 0.90;      m_DCmax[pgsTypes::StrengthII] = 1.25;
   m_DWmin[pgsTypes::StrengthII] = 0.65;      m_DWmax[pgsTypes::StrengthII] = 1.50;
   m_LLIMmin[pgsTypes::StrengthII] = 1.35;      m_LLIMmax[pgsTypes::StrengthII] = 1.35;

   m_DCmin[pgsTypes::FatigueI] = 0.5;        m_DCmax[pgsTypes::FatigueI] = 0.5;
   m_DWmin[pgsTypes::FatigueI] = 0.5;        m_DWmax[pgsTypes::FatigueI] = 0.5;
   m_LLIMmin[pgsTypes::FatigueI] = 1.5;        m_LLIMmax[pgsTypes::FatigueI] = 1.5;
}

bool RefactoredSpecLibraryParameters::HasLoadFactors() const
{
   return m_bUpdateLoadFactors;
}

std::pair<Float64, Float64> RefactoredSpecLibraryParameters::GetDCLoadFactors(pgsTypes::LimitState ls) const
{
   return std::make_pair(m_DCmin[ls], m_DCmax[ls]);
}

std::pair<Float64, Float64> RefactoredSpecLibraryParameters::GetDWLoadFactors(pgsTypes::LimitState ls) const
{
   return std::make_pair(m_DWmin[ls], m_DWmax[ls]);
}

std::pair<Float64, Float64> RefactoredSpecLibraryParameters::GetLLIMLoadFactors(pgsTypes::LimitState ls) const
{
   return std::make_pair(m_LLIMmin[ls], m_LLIMmax[ls]);
}

bool RefactoredSpecLibraryParameters::HasLumpSumLosses() const
{
   return m_bUpdateLumpSumLosses;
}

Float64 RefactoredSpecLibraryParameters::GetBeforeXferLosses() const
{
   return m_BeforeXferLosses;
}

Float64 RefactoredSpecLibraryParameters::GetAfterXferLosses() const
{
   return m_AfterXferLosses;
}

Float64 RefactoredSpecLibraryParameters::GetLiftingLosses() const
{
   return m_LiftingLosses;
}

Float64 RefactoredSpecLibraryParameters::GetShippingLosses() const
{
   return m_ShippingLosses;
}

Float64 RefactoredSpecLibraryParameters::GetBeforeTempStrandRemovalLosses() const
{
   return m_BeforeTempStrandRemovalLosses;
}

Float64 RefactoredSpecLibraryParameters::GetAfterTempStrandRemovalLosses() const
{
   return m_AfterTempStrandRemovalLosses;
}

Float64 RefactoredSpecLibraryParameters::GetAfterDeckPlacementLosses() const
{
   return m_AfterDeckPlacementLosses;
}

Float64 RefactoredSpecLibraryParameters::GetAfterSIDLLosses() const
{
   return m_AfterSIDLLosses;
}

Float64 RefactoredSpecLibraryParameters::GetFinalLosses() const
{
   return m_FinalLosses;
}

bool RefactoredSpecLibraryParameters::HasPTParameters() const
{
   return m_bUpdatePTParameters;
}

Float64 RefactoredSpecLibraryParameters::GetAnchorSet() const
{
   return m_Dset;
}

Float64 RefactoredSpecLibraryParameters::GetWobbleFrictionCoefficient() const
{
   return m_WobbleFriction;
}

Float64 RefactoredSpecLibraryParameters::GetFrictionCoefficient() const
{
   return m_FrictionCoefficient;
}

bool RefactoredSpecLibraryParameters::HasAnalysisType() const
{
   return m_bHasAnalysisType;
}

pgsTypes::AnalysisType RefactoredSpecLibraryParameters::GetAnalysisType() const
{
   return m_AnalysisType;
}

bool RefactoredSpecLibraryParameters::HasLLDFRangeOfApplicabilityAction() const
{
   return m_bIgnoreRangeOfApplicability;
}

WBFL::LRFD::RangeOfApplicabilityAction RefactoredSpecLibraryParameters::GetRangeOfApplicabilityAction() const
{
   return m_bIgnoreRangeOfApplicability ? WBFL::LRFD::RangeOfApplicabilityAction::Ignore : WBFL::LRFD::RangeOfApplicabilityAction::Enforce;
}

bool RefactoredSpecLibraryParameters::HasOldHaulTruck() const
{
   return m_bHasOldHaulTruck;
}

const COldHaulTruck* RefactoredSpecLibraryParameters::GetOldHaulTruck() const
{
   return m_bHasOldHaulTruck ? &m_OldHaulTruck : nullptr;
}
