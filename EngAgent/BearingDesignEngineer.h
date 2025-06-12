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

#include <Details.h>
#include <PsgLib\GirderGroupData.h>
#include <IFace\AnalysisResults.h>
#include <IFace\BearingDesignParameters.h>
#include <EngTools\Bearing.h>
#include <EngTools\BearingLoads.h>
#include <EngTools\BearingCalculator.h>


class pgsBearingDesignEngineer
{
public:
   pgsBearingDesignEngineer(std::weak_ptr<WBFL::EAF::Broker> pBroker);
   pgsBearingDesignEngineer(const pgsBearingDesignEngineer& other) = default;

   Float64 BearingSkewFactor(const ReactionLocation& reactionLocation, bool isFlexural) const;

   void GetBearingParameters(const CGirderKey& girderKey, BEARINGPARAMETERS* pDetails) const;

   void GetLongitudinalPointOfFixity(const CGirderKey& girderKey, BEARINGPARAMETERS* pDetails) const;

   void GetBearingRotationDetails(pgsTypes::AnalysisType analysisType, const pgsPointOfInterest& poi,
	   const ReactionLocation& reactionLocation, CGirderKey girderKey, bool bIncludeImpact, bool bIncludeLLDF, bool isFlexural, ROTATIONDETAILS* pDetails) const;

   void GetBearingDesignProperties(DESIGNPROPERTIES* pDetails) const;

   void GetBearingReactionDetails(const ReactionLocation& reactionLocation,
	   CGirderKey girderKey, pgsTypes::AnalysisType analysisType, 
	   bool bIncludeImpact, bool bIncludeLLDF, REACTIONDETAILS* pDetails) const;

   void GetThermalExpansionDetails(CGirderKey girderKey, BEARINGSHEARDEFORMATIONDETAILS* bearing) const;

   Float64 GetDistanceToPointOfFixity(const pgsPointOfInterest& poi, SHEARDEFORMATIONDETAILS* pDetails) const;

   std::array<Float64,2> GetTimeDependentComponentShearDeformation(Float64 tdNetLoss, BEARINGSHEARDEFORMATIONDETAILS* pDetails) const;

   Float64 GetBearingTimeDependentLosses(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, const GDRCONFIG* pConfig, const LOSSDETAILS* pDetails, TDCOMPONENTS* tdComponents) const;

   void GetTimeDependentShearDeformation(CGirderKey girderKey, SHEARDEFORMATIONDETAILS* pDetails) const;

   void CheckBearing(WBFL::EngTools::Bearing* brg, WBFL::EngTools::BearingLoads* brg_loads, WBFL::EngTools::BearingCalculator* brg_calc) const;

private:
   std::weak_ptr<WBFL::EAF::Broker> m_pBroker;
   inline std::shared_ptr<WBFL::EAF::Broker> GetBroker() const { return m_pBroker.lock(); }
};
