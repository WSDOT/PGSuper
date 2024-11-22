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
#include "AnalysisResults.h"




/*****************************************************************************
INTERFACE
   IBearingDesignParameters

   <<Summary here>>

DESCRIPTION
   <<Detailed description here>>
*****************************************************************************/




struct BEARINGPARAMETERS
{
	bool bSegments;
	bool bConstruction;
	bool bDeck;
	bool bHasOverlay;
	bool bFutureOverlay;
	bool bDeckPanels;
	bool bPedLoading;
	bool bSidewalk;
	bool bShearKey;
	bool bLongitudinalJoint;
	bool bContinuousBeforeDeckCasting;
	bool bTimeStep;
	DuctIndexType nDucts;
	GroupIndexType startGroup;
	GroupIndexType endGroup;
	pgsPointOfInterest poi_fixity;
	PierIndexType fixityPier;
};



struct DESIGNPROPERTIES
{
	Float64 Fy{ WBFL::Units::ConvertToSysUnits(36, WBFL::Units::Measure::KSI) };///< steel yield strength
	Float64 Fth{ WBFL::Units::ConvertToSysUnits(24, WBFL::Units::Measure::KSI) };///< steel fatigue threshold
	Float64 Gmin50{ WBFL::Units::ConvertToSysUnits(0.095, WBFL::Units::Measure::KSI) };///< elastomer minimum shear modulus @ 50 hardness
	Float64 Gmax50{ WBFL::Units::ConvertToSysUnits(0.130, WBFL::Units::Measure::KSI) };///< elastomer maximum shear modulus @ 50 hardness
	Float64 Gmin60{ WBFL::Units::ConvertToSysUnits(0.130, WBFL::Units::Measure::KSI) };///< elastomer minimum shear modulus @ 60 hardness
	Float64 Gmax60{ WBFL::Units::ConvertToSysUnits(0.200, WBFL::Units::Measure::KSI) };///< elastomer maximum shear modulus @ 60 hardness
	Float64 Gmin70{ WBFL::Units::ConvertToSysUnits(0.200, WBFL::Units::Measure::KSI) };///< elastomer minimum shear modulus @ 70 hardness
	Float64 Gmax70{ WBFL::Units::ConvertToSysUnits(0.300, WBFL::Units::Measure::KSI) };///< elastomer maximum shear modulus @ 70 hardness
};


struct ROTATIONDETAILS : public BEARINGPARAMETERS
{
	Float64 skewFactor;
	Float64 staticRotation;
	Float64 cyclicRotation;
	Float64 totalRotation;
	Float64 erectedSegmentRotation;
	Float64 maxGirderRotation;
	Float64 minGirderRotation;
	Float64 diaphragmRotation;
	Float64 maxShearKeyRotation;
	Float64 minShearKeyRotation;
	Float64 maxSlabRotation;
	Float64 minSlabRotation;
	Float64 maxHaunchRotation;
	Float64 minHaunchRotation;
	Float64 maxRailingSystemRotation;
	Float64 minRailingSystemRotation;
	Float64 maxFutureOverlayRotation;
	Float64 minFutureOverlayRotation;
	Float64 maxUserDCRotation;
	Float64 minUserDCRotation;
	Float64 maxUserDWRotation;
	Float64 minUserDWRotation;
	Float64 maxDesignLLrotation;
	VehicleIndexType maxConfigRotation;
	Float64 minDesignLLrotation;
	VehicleIndexType minConfigRotation;
	Float64 maxUserLLrotation;
	Float64 minUserLLrotation;
	Float64 maxLongitudinalJointRotation;
	Float64 minLongitudinalJointRotation;
	Float64 maxConstructionRotation;
	Float64 minConstructionRotation;
	Float64 maxSlabPanelRotation;
	Float64 minSlabPanelRotation;
	Float64 maxSidewalkRotation;
	Float64 minSidewalkRotation;
	Float64 maxPedRotation;
	Float64 minPedRotation;
	Float64 preTensionRotation;
	Float64 postTensionRotation;
	Float64 creepRotation;
	Float64 shrinkageRotation;
	Float64 relaxationRotation;
};

struct REACTIONDETAILS : public BEARINGPARAMETERS
{
	Float64 totalDLreaction;
	Float64 maxComboDesignLLReaction;
	Float64 minComboDesignLLReaction;
	Float64 totalReaction;
	Float64 erectedSegmentReaction;
	Float64 maxGirderReaction;
	Float64 minGirderReaction;
	Float64 diaphragmReaction;
	Float64 maxShearKeyReaction;
	Float64 minShearKeyReaction;
	Float64 maxSlabReaction;
	Float64 minSlabReaction;
	Float64 maxHaunchReaction;
	Float64 minHaunchReaction;
	Float64 maxRailingSystemReaction;
	Float64 minRailingSystemReaction;
	Float64 maxFutureOverlayReaction;
	Float64 minFutureOverlayReaction;
	Float64 maxUserDCReaction;
	Float64 minUserDCReaction;
	Float64 maxUserDWReaction;
	Float64 minUserDWReaction;
	Float64 maxDesignLLReaction;
	VehicleIndexType maxConfigReaction;
	Float64 minDesignLLReaction;
	VehicleIndexType minConfigReaction;
	Float64 maxUserLLReaction;
	Float64 minUserLLReaction;
	Float64 maxLongitudinalJointReaction;
	Float64 minLongitudinalJointReaction;
	Float64 maxConstructionReaction;
	Float64 minConstructionReaction;
	Float64 maxSlabPanelReaction;
	Float64 minSlabPanelReaction;
	Float64 maxSidewalkReaction;
	Float64 minSidewalkReaction;
	Float64 maxPedReaction;
	Float64 minPedReaction;
	Float64 preTensionReaction;
	Float64 postTensionReaction;
	Float64 creepReaction;
	Float64 shrinkageReaction;
	Float64 relaxationReaction;
};


struct TSSHEARDEFORMATION_DIFF_ELEMS
{
	pgsPointOfInterest poi;
	Float64 delta_d;
	//index 0 = incremental strain at girder bottom face (minus erect interval)
	//index 1 = cumulative strain at girder bottom face (minus erect interval)
	//index 2 = average cumulative strain at girder bottom face (minus erect interval)
	//index 3 = effective cumulative shear deformation at bearing
	std::array<Float64, 4> creep;
	std::array<Float64, 4> shrinkage;
	std::array<Float64, 4> relaxation;
};

struct TSSHEARDEFORMATIONDETAILS
{
	IntervalIndexType interval;
	std::vector<TSSHEARDEFORMATION_DIFF_ELEMS> ts_diff_elems;
	Float64 interval_creep;
	Float64 interval_shrinkage;
	Float64 interval_relaxation;
};


struct BEARINGSHEARDEFORMATIONDETAILS  //results per bearing
{
	ReactionLocation reactionLocation;
	pgsPointOfInterest rPoi;
	Float64 creep;
	Float64 tendon_creep;
	Float64 shrinkage;
	Float64 tendon_shrinkage;
	Float64 relaxation;
	Float64 tendon_relaxation;
	std::vector<TSSHEARDEFORMATIONDETAILS> timestep_details;  /// timestep method for td losses
	Float64 ep;
	Float64 yb;
	Float64 r;
	Float64 Ixx;
	Float64 Ag;
	Float64 length_pf;
	Float64 percentExpansion;
	Float64 thermal_expansion_coefficient;
	Float64 max_design_temperature_cold;
	Float64 min_design_temperature_cold;
	Float64 max_design_temperature_moderate;
	Float64 min_design_temperature_moderate;
	Float64 thermal_expansion_cold;
	Float64 thermal_expansion_moderate;
	Float64 tendon_shortening;
	Float64 time_dependent;
	Float64 total_shear_deformation_cold;
	Float64 total_shear_deformation_moderate;
};


struct SHEARDEFORMATIONDETAILS : public BEARINGPARAMETERS  // results per girder
{
	std::vector <BEARINGSHEARDEFORMATIONDETAILS> brg_details;
};





// {D88670F0-3B83-11d2-8EC5-006097DF3C68}
DEFINE_GUID(IID_IBearingDesignParameters,
	0xD88670F0, 0x3B83, 0x11d2, 0x8E, 0xC5, 0x00, 0x60, 0x97, 0xDF, 0x3C, 0x68);
interface IBearingDesignParameters : IUnknown
{
	virtual void GetBearingParameters(CGirderKey girderKey, BEARINGPARAMETERS* pDetails) const = 0;

	virtual void GetBearingRotationDetails(pgsTypes::AnalysisType analysisType, const pgsPointOfInterest& poi, 
		const ReactionLocation& reactionLocation, CGirderKey girderKey, bool bIncludeImpact, bool bIncludeLLDF,
		bool isFlexural, ROTATIONDETAILS* pDetails) const = 0;

	virtual void GetBearingReactionDetails(const ReactionLocation& reactionLocation,
		CGirderKey girderKey, pgsTypes::AnalysisType analysisType, bool bIncludeImpact,
		bool bIncludeLLDF, REACTIONDETAILS* pDetails) const = 0;

	virtual void GetThermalExpansionDetails(CGirderKey girderKey, BEARINGSHEARDEFORMATIONDETAILS* bearing) const = 0;

	virtual Float64 GetDistanceToPointOfFixity(const pgsPointOfInterest& poi, SHEARDEFORMATIONDETAILS* pDetails) const = 0;

	virtual void GetTimeDependentShearDeformation(CGirderKey girderKey, SHEARDEFORMATIONDETAILS* pDetails) const = 0;

	virtual void GetBearingDesignProperties(DESIGNPROPERTIES* pDetails) const = 0;

};





