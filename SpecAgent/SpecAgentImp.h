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

#include <EAF\Agent.h>
#include "CLSID.h"


#include <IFace\ResistanceFactors.h>
#include <IFace\InterfaceShearRequirements.h>
#include <IFace\SplittingChecks.h>

#include "LRFDSplittingCheckEngineer.h"

class GirderLibraryEntry;
class SpecLibraryEntry;

class CSpecAgentImp : public WBFL::EAF::Agent,
   public IBridgeDescriptionEventSink,
   public IStressCheck,
   public IStrandStressLimit,
   public ITendonStressLimit,
   public IConcreteStressLimits,
   public ITransverseReinforcementSpec,
   public ISplittingChecks,
   public IPrecastIGirderDetailsSpec,
   public ISegmentHaulingSpecCriteria,
   public IKdotGirderHaulingSpecCriteria,
   public ISegmentLiftingSpecCriteria,
   public IDebondLimits,
   public IResistanceFactors,
   public IInterfaceShearRequirements,
   public IDuctLimits
{
public:
	CSpecAgentImp()
	{
	}


// IAgent
public:
   std::_tstring GetName() const override { return _T("SpecAgent"); }
   bool RegisterInterfaces() override;
   bool Init() override;
   bool Reset() override;
   bool ShutDown() override;
   CLSID GetCLSID() const override;

// IBridgeDescriptionEventSink
public:
   HRESULT OnBridgeChanged(CBridgeChangedHint* pHint) override;
   HRESULT OnGirderFamilyChanged() override;
   HRESULT OnGirderChanged(const CGirderKey& girderKey, Uint32 lHint) override;
   HRESULT OnLiveLoadChanged() override;
   HRESULT OnLiveLoadNameChanged(LPCTSTR strOldName, LPCTSTR strNewName) override;
   HRESULT OnConstructionLoadChanged() override;

// IStressCheck
public:
   std::vector<StressCheckTask> GetStressCheckTasks(const CGirderKey& girderKey,bool bDesign = false) const override;
   std::vector<StressCheckTask> GetStressCheckTasks(const CSegmentKey& segmentKey, bool bDesign = false) const override;
   std::vector<IntervalIndexType> GetStressCheckIntervals(const CGirderKey& girderKey, bool bDesign = false) const override;

// IAllowableStrandStress
public:
   bool CheckStrandStressAtJacking() const override;;
   bool CheckStrandStressBeforeXfer() const override;;
   bool CheckStrandStressAfterXfer() const override;;
   bool CheckStrandStressAfterLosses() const override;;
   Float64 GetStrandStressLimitAtJacking(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const override;;
   Float64 GetStrandStressLimitBeforeXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const override;;
   Float64 GetStrandStressLimitAfterXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const override;;
   Float64 GetStrandStressLimitAfterLosses(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const override;;

// IAllowableTendonStress
public:
   bool CheckTendonStressAtJacking() const override;;
   bool CheckTendonStressPriorToSeating() const override;;
   Float64 GetSegmentTendonStressLimitAtJacking(const CSegmentKey& segmentKey) const override;
   Float64 GetSegmentTendonStressLimitPriorToSeating(const CSegmentKey& segmentKey) const override;
   Float64 GetSegmentTendonStressLimitAfterAnchorSetAtAnchorage(const CSegmentKey& segmentKey) const override;
   Float64 GetSegmentTendonStressLimitAfterAnchorSet(const CSegmentKey& segmentKey) const override;
   Float64 GetSegmentTendonStressLimitAfterLosses(const CSegmentKey& segmentKey) const override;
   Float64 GetSegmentTendonStressLimitCoefficientAtJacking(const CSegmentKey& segmentKey) const override;
   Float64 GetSegmentTendonStressLimitCoefficientPriorToSeating(const CSegmentKey& segmentKey) const override;
   Float64 GetSegmentTendonStressLimitCoefficientAfterAnchorSetAtAnchorage(const CSegmentKey& segmentKey) const override;
   Float64 GetSegmentTendonStressLimitCoefficientAfterAnchorSet(const CSegmentKey& segmentKey) const override;
   Float64 GetSegmentTendonStressLimitCoefficientAfterLosses(const CSegmentKey& segmentKey) const override;
   Float64 GetGirderTendonStressLimitAtJacking(const CGirderKey& girderKey) const override;
   Float64 GetGirderTendonStressLimitPriorToSeating(const CGirderKey& girderKey) const override;
   Float64 GetGirderTendonStressLimitAfterAnchorSetAtAnchorage(const CGirderKey& girderKey) const override;
   Float64 GetGirderTendonStressLimitAfterAnchorSet(const CGirderKey& girderKey) const override;
   Float64 GetGirderTendonStressLimitAfterLosses(const CGirderKey& girderKey) const override;
   Float64 GetGirderTendonStressLimitCoefficientAtJacking(const CGirderKey& girderKey) const override;
   Float64 GetGirderTendonStressLimitCoefficientPriorToSeating(const CGirderKey& girderKey) const override;
   Float64 GetGirderTendonStressLimitCoefficientAfterAnchorSetAtAnchorage(const CGirderKey& girderKey) const override;
   Float64 GetGirderTendonStressLimitCoefficientAfterAnchorSet(const CGirderKey& girderKey) const override;
   Float64 GetGirderTendonStressLimitCoefficientAfterLosses(const CGirderKey& girderKey) const override;

// IAllowableConcreteStress
public:
   Float64 GetConcreteCompressionStressLimit(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task) const override;
   Float64 GetConcreteTensionStressLimit(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const override;
   void ReportSegmentConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const override;
   void ReportSegmentConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task, const pgsSegmentArtifact* pSegmentArtifact, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const override;
   void ReportClosureJointConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const override;
   void ReportClosureJointConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task, const pgsSegmentArtifact* pSegmentArtifact, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const override;

   Float64 GetConcreteTensionStressLimit(pgsTypes::LoadRatingType ratingType,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation) const override;

   Float64 GetConcreteCompressionStressLimitCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task) const override;
   TensionStressLimit GetConcreteTensionStressLimitParameters(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const override;

   std::vector<Float64> GetDeckConcreteCompressionStressLimit(const PoiList& vPoi, const StressCheckTask& task) const override;
   std::vector<Float64> GetGirderConcreteTensionStressLimit(const PoiList& vPoi, const StressCheckTask& task,bool bWithBondededReinforcement,bool bInPrecompressedTensileZone) const override;
   std::vector<Float64> GetDeckConcreteTensionStressLimit(const PoiList& vPoi, const StressCheckTask& task,bool bWithBondededReinforcement) const override;
   std::vector<Float64> GetGirderConcreteCompressionStressLimit(const PoiList& vPoi, const StressCheckTask& task) const override;

   Float64 GetSegmentConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task) const override;
   Float64 GetClosureJointConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task) const override;
   Float64 GetDeckConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task) const override;

   Float64 GetSegmentConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const override;
   Float64 GetClosureJointConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const override;
   Float64 GetDeckConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const override;

   Float64 GetSegmentConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement) const override;
   Float64 GetClosureJointConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const override;
   Float64 GetDeckConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement) const override;

   Float64 GetSegmentConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement) const override;
   Float64 GetClosureJointConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const override;
   Float64 GetDeckConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement) const override;

   Float64 GetSegmentConcreteCompressionStressLimitCoefficient(const pgsPointOfInterest& poi,const StressCheckTask& task) const override;
   Float64 GetClosureJointConcreteCompressionStressLimitCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task) const override;
   Float64 GetDeckConcreteCompressionStressLimitCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task) const override;

   TensionStressLimit GetSegmentConcreteTensionStressLimitParameters(const pgsPointOfInterest& poi, const StressCheckTask& task, bool bWithBondedReinforcement) const override;
   TensionStressLimit GetClosureJointConcreteTensionStressLimitParameters(const pgsPointOfInterest& poi, const StressCheckTask& task, bool bWithBondedReinforcement, bool bInPrecompressedTensileZone) const override;
   TensionStressLimit GetDeckConcreteTensionStressLimitParameters(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement) const override;

   bool IsConcreteStressLimitApplicable(const CSegmentKey& segmentKey, const StressCheckTask& task) const override;
   bool IsConcreteStressLimitApplicable(const CGirderKey& girderKey, const StressCheckTask& task) const override;
   bool HasConcreteTensionStressLimitWithRebarOption(IntervalIndexType intervalIdx,bool bInPTZ,bool bSegment,const CSegmentKey& segmentKey) const override;
   Float64 GetMaxCoverToUseHigherTensionStressLimit() const override;

   bool CheckTemporaryStresses() const override;
   bool CheckFinalDeadLoadTensionStress() const override;

   Float64 GetSegmentConcreteWebPrincipalTensionStressLimit(const CSegmentKey& segmentKey) const override;
   void ReportSegmentConcreteWebPrincipalTensionStressLimit(const CSegmentKey& segmentKey, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const override;
   Float64 GetClosureJointConcreteWebPrincipalTensionStressLimit(const CClosureKey& closureKey) const override;
   void ReportClosureJointConcreteWebPrincipalTensionStressLimit(const CClosureKey& closureKey, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const override;
   Float64 GetConcreteWebPrincipalTensionStressLimit(const pgsPointOfInterest& poi) const override;
   Float64 GetConcreteWebPrincipalTensionStressLimitCoefficient() const override;
   Float64 GetPrincipalTensileStressFcThreshold() const override;
   Float64 GetPrincipalTensileStressRequiredConcreteStrength(const pgsPointOfInterest& poi, Float64 stress) const override;

   Float64 GetPCIUHPCTensionStressLimitCoefficient() const override;
   Float64 GetUHPCTensionStressLimitCoefficient(const CSegmentKey& segmentKey) const override;
   Float64 GetUHPCFatigueTensionStressLimitModifier() const override;

   Float64 ComputeRequiredConcreteStrength(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,Float64 stressDemand,const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const override;
   std::_tstring GetConcreteStressLimitParameterName(pgsTypes::StressType stressType, pgsTypes::ConcreteType concreteType) const override;

// ITransverseReinforcementSpec
public:
   WBFL::Materials::Rebar::Size GetMinConfinementBarSize() const override;
   Float64 GetMaxConfinementBarSpacing() const override;
   Float64 GetMinConfinementAvS() const override;
   void GetMaxStirrupSpacing(Float64 dv,Float64* sUnderLimit, Float64* sOverLimit) const override;
   Float64 GetMinStirrupSpacing(Float64 maxAggregateSize, Float64 barDiameter) const override;

// ISplittingChecks
public:
   Float64 GetSplittingZoneLength(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType) const override;
   std::shared_ptr<pgsSplittingCheckArtifact> CheckSplitting(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetAsRequired(const pgsSplittingCheckArtifact* pArtifact) const override;
   void ReportSplittingChecks(const pgsGirderArtifact* pGirderArtifact, rptChapter* pChapter) const override;
   void ReportSplittingCheckDetails(const pgsGirderArtifact* pGirderArtifact, rptChapter* pChapter) const override;
   std::_tstring GetSplittingCheckName() const override;

// IPrecastIGirderDetailsSpec
public:
   Float64 GetMinTopFlangeThickness() const override;
   Float64 GetMinWebThickness() const override;
   Float64 GetMinBottomFlangeThickness() const override;

// ISegmentLiftingSpecCriteria
public:
   bool IsLiftingAnalysisEnabled() const override;
   void GetLiftingImpact(Float64* pDownward, Float64* pUpward) const override;
   Float64 GetLiftingCrackingFs() const override;
   Float64 GetLiftingFailureFs() const override;
   Float64 GetLiftingAllowableTensileConcreteStress(const CSegmentKey& segmentKey) const override;
   Float64 GetLiftingAllowableGlobalCompressiveConcreteStress(const CSegmentKey& segmentKey) const override;
   Float64 GetLiftingAllowableGlobalCompressionFactor() const override;
   Float64 GetLiftingAllowablePeakCompressiveConcreteStress(const CSegmentKey& segmentKey) const override;
   Float64 GetLiftingAllowablePeakCompressionFactor() const override;
   Float64 GetLiftingAllowableTensionFactor() const override;
   Float64 GetHeightOfPickPointAboveGirderTop() const override;
   Float64 GetLiftingLoopPlacementTolerance() const override;
   Float64 GetLiftingCableMinInclination() const override;
   Float64 GetLiftingSweepTolerance() const override;
   Float64 GetLiftingWithMildRebarAllowableStress(const CSegmentKey& segmentKey) const override;
   Float64 GetLiftingWithMildRebarAllowableStressFactor() const override;
   void GetLiftingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax) const override;
   Float64 GetLiftingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey,Float64 fci, bool includeRebar) const override;
   Float64 GetLiftingAllowableGlobalCompressiveConcreteStressEx(const CSegmentKey& segmentKey, Float64 fci) const override;
   Float64 GetLiftingAllowablePeakCompressiveConcreteStressEx(const CSegmentKey& segmentKey, Float64 fci) const override;
   Float64 GetLiftingModulusOfRupture(const CSegmentKey& segmentKey) const override;
   Float64 GetLiftingModulusOfRupture(const CSegmentKey& segmentKey,Float64 fci,pgsTypes::ConcreteType concType) const override;
   Float64 GetLiftingModulusOfRuptureFactor(pgsTypes::ConcreteType concType) const override;
   Float64 GetMinimumLiftingPointLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const override;
   Float64 GetLiftingPointLocationAccuracy() const override;
   Float64 GetLiftingCamberMultiplier() const override;
   WBFL::Stability::WindLoadType GetLiftingWindType() const override;
   Float64 GetLiftingWindLoad() const override;
   WBFL::Stability::LiftingCriteria GetLiftingStabilityCriteria(const CSegmentKey& segmentKey, const HANDLINGCONFIG* pLiftConfig = nullptr) const override;

// ISegmentHaulingSpecCriteria
public:
   bool IsHaulingAnalysisEnabled() const override;
   pgsTypes::HaulingAnalysisMethod GetHaulingAnalysisMethod() const override;
   void GetHaulingImpact(Float64* pDownward, Float64* pUpward) const override;
   Float64 GetHaulingCrackingFs() const override;
   Float64 GetHaulingRolloverFs() const override;
   void GetHaulingAllowableTensileConcreteStressParameters(WBFL::Stability::HaulingSlope slope,Float64* factor,bool* pbMax,Float64* fmax) const override;
   Float64 GetHaulingAllowableTensileConcreteStress(const CSegmentKey& segmentKey, WBFL::Stability::HaulingSlope slope) const override;
   Float64 GetHaulingAllowableGlobalCompressiveConcreteStress(const CSegmentKey& segmentKey) const override;
   Float64 GetHaulingAllowablePeakCompressiveConcreteStress(const CSegmentKey& segmentKey) const override;
   Float64 GetHaulingAllowableTensionFactor(WBFL::Stability::HaulingSlope slope) const override;
   Float64 GetHaulingAllowableGlobalCompressionFactor() const override;
   Float64 GetHaulingAllowablePeakCompressionFactor() const override;
   Float64 GetHaulingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey, WBFL::Stability::HaulingSlope slope,Float64 fc, bool includeRebar) const override;
   Float64 GetHaulingAllowableGlobalCompressiveConcreteStressEx(const CSegmentKey& segmentKey, Float64 fc) const override;
   Float64 GetHaulingAllowablePeakCompressiveConcreteStressEx(const CSegmentKey& segmentKey, Float64 fc) const override;
   WBFL::Stability::HaulingImpact GetHaulingImpactUsage() const override;
   Float64 GetNormalCrownSlope() const override;
   Float64 GetMaxSuperelevation() const override;
   Float64 GetHaulingSweepTolerance() const override;
   Float64 GetHaulingSweepGrowth() const override;
   Float64 GetHaulingSupportPlacementTolerance() const override;
   Float64 GetHaulingCamberMultiplier() const override;
   Float64 GetRollStiffness(const CSegmentKey& segmentKey) const override;
   Float64 GetHeightOfGirderBottomAboveRoadway(const CSegmentKey& segmentKey) const override;
   Float64 GetHeightOfTruckRollCenterAboveRoadway(const CSegmentKey& segmentKey) const override;
   Float64 GetAxleWidth(const CSegmentKey& segmentKey) const override;
   Float64 GetAllowableDistanceBetweenSupports(const CSegmentKey& segmentKey) const override;
   Float64 GetAllowableLeadingOverhang(const CSegmentKey& segmentKey) const override;
   Float64 GetMaxGirderWgt(const CSegmentKey& segmentKey) const override;
   Float64 GetHaulingWithMildRebarAllowableStress(const CSegmentKey& segmentKey, WBFL::Stability::HaulingSlope slope) const override;
   Float64 GetHaulingWithMildRebarAllowableStressFactor(WBFL::Stability::HaulingSlope slope) const override;
   Float64 GetHaulingModulusOfRupture(const CSegmentKey& segmentKey) const override;
   Float64 GetHaulingModulusOfRupture(const CSegmentKey& segmentKey,Float64 fci,pgsTypes::ConcreteType concType) const override;
   Float64 GetHaulingModulusOfRuptureFactor(pgsTypes::ConcreteType concType) const override;
   Float64 GetMinimumHaulingSupportLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const override;
   Float64 GetHaulingSupportLocationAccuracy() const override;
   WBFL::Stability::WindLoadType GetHaulingWindType() const override;
   Float64 GetHaulingWindLoad() const override;
   WBFL::Stability::CFType GetCentrifugalForceType() const override;
   Float64 GetHaulingSpeed() const override;
   Float64 GetTurningRadius() const override;
   WBFL::Stability::HaulingCriteria GetHaulingStabilityCriteria(const CSegmentKey& segmentKey,const HANDLINGCONFIG* pHaulConfig = nullptr) const override;

// IKdotGirderHaulingSpecCriteria
public:
   // Spec criteria for KDOT analyses
   Float64 GetKdotHaulingAllowableTensileConcreteStress(const CSegmentKey& segmentKey) const override;
   Float64 GetKdotHaulingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey) const override;
   Float64 GetKdotHaulingAllowableTensionFactor() const override;
   Float64 GetKdotHaulingAllowableCompressionFactor() const override;
   Float64 GetKdotHaulingWithMildRebarAllowableStress(const CSegmentKey& segmentKey) const override;
   Float64 GetKdotHaulingWithMildRebarAllowableStressFactor() const override;
   void GetKdotHaulingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax) const override;
   Float64 GetKdotHaulingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc, bool includeRebar) const override;
   Float64 GetKdotHaulingAllowableCompressiveConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc) const override;
   void GetMinimumHaulingSupportLocation(Float64* pHardDistance, bool* pUseFactoredLength, Float64* pLengthFactor) const override;
   Float64 GetHaulingDesignLocationAccuracy() const override;
   void GetHaulingGFactors(Float64* pOverhangFactor, Float64* pInteriorFactor) const override;

// IDebondLimits
public:
   bool CheckMaxDebondedStrands(const CSegmentKey& segmentKey) const override;
   Float64 GetMaxDebondedStrands(const CSegmentKey& segmentKey) const override;
   Float64 GetMaxDebondedStrandsPerRow(const CSegmentKey& segmentKey) const override;
   void GetMaxDebondedStrandsPerSection(const CSegmentKey& segmentKey, StrandIndexType* p10orLess, StrandIndexType* pNS, bool* pbCheckMax, Float64* pMaxFraction) const override;
   void GetMaxDebondLength(const CSegmentKey& segmentKey, Float64* pLen, pgsTypes::DebondLengthControl* pControl) const override;
   void GetMinDistanceBetweenDebondSections(const CSegmentKey& segmentKey, Float64* pndb, bool* pbUseMinDistance, Float64* pMinDistance) const override;
   Float64 GetMinDistanceBetweenDebondSections(const CSegmentKey& segmentKey) const override;
   bool CheckDebondingSymmetry(const CSegmentKey& segmentKey) const override;
   bool CheckAdjacentDebonding(const CSegmentKey& segmentKey) const override;
   bool CheckDebondingInWebWidthProjections(const CSegmentKey& segmentKey) const override;
   bool IsExteriorStrandBondingRequiredInRow(const CSegmentKey& segmentKey, pgsTypes::MemberEndType endType, RowIndexType rowIdx) const override;

// IResistanceFactors
public:
   void GetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64* phiTensionPS,Float64* phiTensionRC,Float64* phiTensionSpliced,Float64* phiCompression) const override;
   void GetFlexuralStrainLimits(WBFL::Materials::PsStrand::Grade grade,WBFL::Materials::PsStrand::Type type,Float64* pecl,Float64* petl) const override;
   void GetFlexuralStrainLimits(WBFL::Materials::Rebar::Grade rebarGrade,Float64* pecl,Float64* petl) const override;
   Float64 GetShearResistanceFactor(const pgsPointOfInterest& poi, pgsTypes::ConcreteType type) const override;
   Float64 GetShearResistanceFactor(bool isDebonded, pgsTypes::ConcreteType type) const override;
   Float64 GetClosureJointFlexureResistanceFactor(pgsTypes::ConcreteType type) const override;
   Float64 GetClosureJointShearResistanceFactor(pgsTypes::ConcreteType type) const override;
   Float64 GetDuctilityCurvatureRatioLimit() const override;

// IInterfaceShearRequirements 
public:
   pgsTypes::ShearFlowMethod GetShearFlowMethod() const override;
   Float64 GetMaxShearConnectorSpacing(const pgsPointOfInterest& poi) const override;

// IDuctLimits
public:
   Float64 GetSegmentTendonRadiusOfCurvatureLimit(const CSegmentKey& segmentKey) const override;
   Float64 GetSegmentTendonAreaLimit(const CSegmentKey& segmentKey) const override;
   Float64 GetSegmentTendonDuctSizeLimit(const CSegmentKey& segmentKey) const override;
   Float64 GetGirderTendonRadiusOfCurvatureLimit(const CGirderKey& girderKey) const override;
   Float64 GetGirderTendonAreaLimit(const CGirderKey& girderKey) const override;
   Float64 GetGirderTendonDuctSizeLimit(const CGirderKey& girderKey) const override;
   Float64 GetTendonAreaLimit(pgsTypes::StrandInstallationType installationType) const override;
   Float64 GetSegmentDuctDeductionFactor(const CSegmentKey& segmentKey, IntervalIndexType intervalIdx) const override;
   Float64 GetGirderDuctDeductionFactor(const CGirderKey& girderKey, DuctIndexType ductIdx, IntervalIndexType intervalIdx) const override;

private:
   EAF_DECLARE_AGENT_DATA;
   DECLARE_LOGFILE;

   IDType m_dwBridgeDescCookie;

   StatusCallbackIDType m_scidHaulTruckError;

   const pgsSplittingCheckEngineer& GetSplittingCheckEngineer(const CSegmentKey& segmentKey) const;
   pgsLRFDSplittingCheckEngineer m_LRFDSplittingCheckEngineer;
   pgsPCIUHPCSplittingCheckEngineer m_PCIUHPCSplittingCheckEngineer;
   pgsUHPCSplittingCheckEngineer m_UHPCSplittingCheckEngineer;

   const GirderLibraryEntry* GetGirderEntry(const CSegmentKey& segmentKey) const;
   const SpecLibraryEntry* GetSpec() const;

   Float64 GetRadiusOfCurvatureLimit(pgsTypes::DuctType ductType) const;
   Float64 GetDuctDeductFactor(IntervalIndexType intervalIdx, IntervalIndexType groutDuctIntervalIdx) const;

   bool IsLoadRatingServiceIIILimitState(pgsTypes::LimitState ls) const;
   void ValidateHaulTruck(const CPrecastSegmentData* pSegment) const;
   void Invalidate();
};
