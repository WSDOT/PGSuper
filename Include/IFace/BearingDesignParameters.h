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




struct TABLEPARAMETERS
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


struct ROTATIONDETAILS : public TABLEPARAMETERS
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

struct REACTIONDETAILS : public TABLEPARAMETERS
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


struct SHEARDEFORMATIONDETAILS : public TABLEPARAMETERS
{
	Float64 thermal_expansion_coefficient;
	Float64 length_pf;
	Float64 max_design_temperature_cold;
	Float64 min_design_temperature_cold;
	Float64 max_design_temperature_moderate;
	Float64 min_design_temperature_moderate;
	Float64 percentExpansion;
	Float64 thermal_expansion_cold;
	Float64 thermal_expansion_moderate;
	Float64 preTension;
	Float64 postTension;
	Float64 tendon_shortening;
	Float64 total_tendon_shortening;
	Float64 creep;
	Float64 tendon_creep;
	Float64 shrinkage;
	Float64 tendon_shrinkage;
	Float64 relaxation;
	Float64 tendon_relaxation;
	Float64 time_dependent;
	Float64 Ixx;
	Float64 Ag;
	Float64 ep;
	Float64 yb;
	Float64 r;
	Float64 flange_bottom_shortening;
	Float64 total_shear_deformation_cold;
	Float64 total_shear_deformation_moderate;
	pgsPointOfInterest poi_fixity;
};



// {D88670F0-3B83-11d2-8EC5-006097DF3C68}
DEFINE_GUID(IID_IBearingDesignParameters,
	0xD88670F0, 0x3B83, 0x11d2, 0x8E, 0xC5, 0x00, 0x60, 0x97, 0xDF, 0x3C, 0x68);
interface IBearingDesignParameters : IUnknown
{
	virtual void GetBearingTableParameters(CGirderKey girderKey, TABLEPARAMETERS* pDetails) const = 0;

	virtual void GetBearingRotationDetails(pgsTypes::AnalysisType analysisType, const pgsPointOfInterest& poi, 
		const ReactionLocation& reactionLocation, CGirderKey girderKey, bool bIncludeImpact, bool bIncludeLLDF,
		bool isFlexural, ROTATIONDETAILS* pDetails) const = 0;

	virtual void GetBearingReactionDetails(const ReactionLocation& reactionLocation,
		CGirderKey girderKey, pgsTypes::AnalysisType analysisType, bool bIncludeImpact,
		bool bIncludeLLDF, REACTIONDETAILS* pDetails) const = 0;

	virtual void GetThermalExpansionDetails(const pgsPointOfInterest& poi, SHEARDEFORMATIONDETAILS* pDetails) const = 0;

	virtual Float64 GetDistanceToPointOfFixity(const pgsPointOfInterest& poi, SHEARDEFORMATIONDETAILS* pDetails) const = 0;

	virtual Float64 GetTimeDependentComponentShearDeformation(const pgsPointOfInterest& poi, Float64 loss, SHEARDEFORMATIONDETAILS* pDetails) const = 0;

	virtual Float64 GetTimeDependentShearDeformation(
		const pgsPointOfInterest& poi, PierIndexType startPierIdx, SHEARDEFORMATIONDETAILS* pDetails) const = 0;

	virtual void GetBearingDesignProperties(DESIGNPROPERTIES* pDetails) const = 0;

};





