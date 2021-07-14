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

// SpecAgentImp.h : Declaration of the CSpecAgentImp

#ifndef __SPECAGENT_H_
#define __SPECAGENT_H_

#include "CLSID.h"
#include "resource.h"       // main symbols

#include <EAF\EAFInterfaceCache.h>
#include <IFace\ResistanceFactors.h>
#include <IFace\InterfaceShearRequirements.h>

class GirderLibraryEntry;
class SpecLibraryEntry;

/////////////////////////////////////////////////////////////////////////////
// CSpecAgentImp
class ATL_NO_VTABLE CSpecAgentImp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSpecAgentImp, &CLSID_SpecAgent>,
	public IConnectionPointContainerImpl<CSpecAgentImp>,
	public IAgentEx,
   public IBridgeDescriptionEventSink,
   public IStressCheck,
   public IAllowableStrandStress,
   public IAllowableTendonStress,
   public IAllowableConcreteStress,
   public ITransverseReinforcementSpec,
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

DECLARE_REGISTRY_RESOURCEID(IDR_SPECAGENT)
DECLARE_NOT_AGGREGATABLE(CSpecAgentImp)

BEGIN_COM_MAP(CSpecAgentImp)
	COM_INTERFACE_ENTRY(IAgent)
   COM_INTERFACE_ENTRY(IAgentEx)
   COM_INTERFACE_ENTRY(IBridgeDescriptionEventSink)
   COM_INTERFACE_ENTRY(IStressCheck)
   COM_INTERFACE_ENTRY(IAllowableStrandStress)
   COM_INTERFACE_ENTRY(IAllowableTendonStress)
   COM_INTERFACE_ENTRY(IAllowableConcreteStress)
   COM_INTERFACE_ENTRY(ITransverseReinforcementSpec)
   COM_INTERFACE_ENTRY(IPrecastIGirderDetailsSpec)
   COM_INTERFACE_ENTRY(ISegmentLiftingSpecCriteria)
   COM_INTERFACE_ENTRY(ISegmentHaulingSpecCriteria)
   COM_INTERFACE_ENTRY(IKdotGirderHaulingSpecCriteria)
   COM_INTERFACE_ENTRY(IDebondLimits)
   COM_INTERFACE_ENTRY(IResistanceFactors)
   COM_INTERFACE_ENTRY(IInterfaceShearRequirements)
   COM_INTERFACE_ENTRY(IDuctLimits)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CSpecAgentImp)
END_CONNECTION_POINT_MAP()


// IAgent
public:
	STDMETHOD(SetBroker)(IBroker* pBroker) override;
	STDMETHOD(RegInterfaces)() override;
	STDMETHOD(Init)() override;
	STDMETHOD(Reset)() override;
	STDMETHOD(ShutDown)() override;
   STDMETHOD(Init2)() override;
   STDMETHOD(GetClassID)(CLSID* pCLSID) override;

// IBridgeDescriptionEventSink
public:
   virtual HRESULT OnBridgeChanged(CBridgeChangedHint* pHint) override;
   virtual HRESULT OnGirderFamilyChanged() override;
   virtual HRESULT OnGirderChanged(const CGirderKey& girderKey, Uint32 lHint) override;
   virtual HRESULT OnLiveLoadChanged() override;
   virtual HRESULT OnLiveLoadNameChanged(LPCTSTR strOldName, LPCTSTR strNewName) override;
   virtual HRESULT OnConstructionLoadChanged() override;

// IStressCheck
public:
   virtual std::vector<StressCheckTask> GetStressCheckTasks(const CGirderKey& girderKey,bool bDesign = false) const override;
   virtual std::vector<StressCheckTask> GetStressCheckTasks(const CSegmentKey& segmentKey, bool bDesign = false) const override;
   virtual std::vector<IntervalIndexType> GetStressCheckIntervals(const CGirderKey& girderKey, bool bDesign = false) const override;

// IAllowableStrandStress
public:
   virtual bool CheckStressAtJacking() const override;;
   virtual bool CheckStressBeforeXfer() const override;;
   virtual bool CheckStressAfterXfer() const override;;
   virtual bool CheckStressAfterLosses() const override;;
   virtual Float64 GetAllowableAtJacking(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const override;;
   virtual Float64 GetAllowableBeforeXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const override;;
   virtual Float64 GetAllowableAfterXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const override;;
   virtual Float64 GetAllowableAfterLosses(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const override;;

// IAllowableTendonStress
public:
   virtual bool CheckTendonStressAtJacking() const override;;
   virtual bool CheckTendonStressPriorToSeating() const override;;
   virtual Float64 GetSegmentTendonAllowableAtJacking(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentTendonAllowablePriorToSeating(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentTendonAllowableAfterAnchorSetAtAnchorage(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentTendonAllowableAfterAnchorSet(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentTendonAllowableAfterLosses(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentTendonAllowableCoefficientAtJacking(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentTendonAllowableCoefficientPriorToSeating(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentTendonAllowableCoefficientAfterAnchorSetAtAnchorage(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentTendonAllowableCoefficientAfterAnchorSet(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentTendonAllowableCoefficientAfterLosses(const CSegmentKey& segmentKey) const override;

   virtual Float64 GetGirderTendonAllowableAtJacking(const CGirderKey& girderKey) const override;
   virtual Float64 GetGirderTendonAllowablePriorToSeating(const CGirderKey& girderKey) const override;
   virtual Float64 GetGirderTendonAllowableAfterAnchorSetAtAnchorage(const CGirderKey& girderKey) const override;
   virtual Float64 GetGirderTendonAllowableAfterAnchorSet(const CGirderKey& girderKey) const override;
   virtual Float64 GetGirderTendonAllowableAfterLosses(const CGirderKey& girderKey) const override;
   virtual Float64 GetGirderTendonAllowableCoefficientAtJacking(const CGirderKey& girderKey) const override;
   virtual Float64 GetGirderTendonAllowableCoefficientPriorToSeating(const CGirderKey& girderKey) const override;
   virtual Float64 GetGirderTendonAllowableCoefficientAfterAnchorSetAtAnchorage(const CGirderKey& girderKey) const override;
   virtual Float64 GetGirderTendonAllowableCoefficientAfterAnchorSet(const CGirderKey& girderKey) const override;
   virtual Float64 GetGirderTendonAllowableCoefficientAfterLosses(const CGirderKey& girderKey) const override;

// IAllowableConcreteStress
public:
   virtual Float64 GetAllowableCompressionStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task) const override;
   virtual Float64 GetAllowableTensionStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const override;

   virtual Float64 GetAllowableTensionStress(pgsTypes::LoadRatingType ratingType,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation) const override;

   virtual Float64 GetAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task) const override;
   virtual void GetAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) const override;

   virtual std::vector<Float64> CSpecAgentImp::GetGirderAllowableCompressionStress(const PoiList& vPoi, const StressCheckTask& task) const override;
   virtual std::vector<Float64> CSpecAgentImp::GetDeckAllowableCompressionStress(const PoiList& vPoi, const StressCheckTask& task) const override;
   virtual std::vector<Float64> CSpecAgentImp::GetGirderAllowableTensionStress(const PoiList& vPoi, const StressCheckTask& task,bool bWithBondededReinforcement,bool bInPrecompressedTensileZone) const override;
   virtual std::vector<Float64> CSpecAgentImp::GetDeckAllowableTensionStress(const PoiList& vPoi, const StressCheckTask& task,bool bWithBondededReinforcement) const override;

   virtual Float64 GetSegmentAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task) const override;
   virtual Float64 GetClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task) const override;
   virtual Float64 GetDeckAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task) const override;

   virtual Float64 GetSegmentAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const override;
   virtual Float64 GetClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const override;
   virtual Float64 GetDeckAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const override;

   virtual Float64 GetSegmentAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement) const override;
   virtual Float64 GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const override;
   virtual Float64 GetDeckAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement) const override;

   virtual Float64 GetSegmentAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement) const override;
   virtual Float64 GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const override;
   virtual Float64 GetDeckAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement) const override;

   virtual Float64 GetSegmentAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,const StressCheckTask& task) const override;
   virtual Float64 GetClosureJointAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task) const override;
   virtual Float64 GetDeckAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task) const override;

   virtual void GetSegmentAllowableTensionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) const override;
   virtual void GetClosureJointAllowableTensionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) const override;
   virtual void GetDeckAllowableTensionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) const override;

   virtual bool IsStressCheckApplicable(const CGirderKey& girderKey, const StressCheckTask& task) const override;
   virtual bool HasAllowableTensionWithRebarOption(IntervalIndexType intervalIdx,bool bInPTZ,bool bSegment,const CSegmentKey& segmentKey) const override;

   virtual bool CheckTemporaryStresses() const override;
   virtual bool CheckFinalDeadLoadTensionStress() const override;

   virtual Float64 GetAllowableSegmentPrincipalWebTensionStress(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetAllowableClosureJointPrincipalWebTensionStress(const CClosureKey& closureKey) const override;
   virtual Float64 GetAllowablePrincipalWebTensionStress(const pgsPointOfInterest& poi) const override;
   virtual Float64 GetAllowablePrincipalWebTensionStressCoefficient() const override;
   virtual Float64 GetprincipalTensileStressFcThreshold() const override;

// ITransverseReinforcementSpec
public:
   virtual Float64 GetMaxSplittingStress(Float64 fyRebar) const override;
   virtual Float64 GetSplittingZoneLength( Float64 girderHeight ) const override;
   virtual Float64 CSpecAgentImp::GetSplittingZoneLengthFactor() const override;
   virtual Float64 GetUHPCStrengthAtFirstCrack() const override;
   virtual matRebar::Size GetMinConfinmentBarSize() const override;
   virtual Float64 GetMaxConfinmentBarSpacing() const override;
   virtual Float64 GetMinConfinmentAvS() const override;
   virtual void GetMaxStirrupSpacing(Float64 dv,Float64* sUnderLimit, Float64* sOverLimit) const override;
   virtual Float64 GetMinStirrupSpacing(Float64 maxAggregateSize, Float64 barDiameter) const override;

// IPrecastIGirderDetailsSpec
public:
   virtual Float64 GetMinTopFlangeThickness() const override;
   virtual Float64 GetMinWebThickness() const override;
   virtual Float64 GetMinBottomFlangeThickness() const override;

// ISegmentLiftingSpecCriteria
public:
   virtual bool IsLiftingAnalysisEnabled() const override;
   virtual void GetLiftingImpact(Float64* pDownward, Float64* pUpward) const override;
   virtual Float64 GetLiftingCrackingFs() const override;
   virtual Float64 GetLiftingFailureFs() const override;
   virtual Float64 GetLiftingAllowableTensileConcreteStress(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetLiftingAllowableGlobalCompressiveConcreteStress(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetLiftingAllowableGlobalCompressionFactor() const override;
   virtual Float64 GetLiftingAllowablePeakCompressiveConcreteStress(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetLiftingAllowablePeakCompressionFactor() const override;
   virtual Float64 GetLiftingAllowableTensionFactor() const override;
   virtual Float64 GetHeightOfPickPointAboveGirderTop() const override;
   virtual Float64 GetLiftingLoopPlacementTolerance() const override;
   virtual Float64 GetLiftingCableMinInclination() const override;
   virtual Float64 GetLiftingSweepTolerance() const override;
   virtual Float64 GetLiftingWithMildRebarAllowableStress(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetLiftingWithMildRebarAllowableStressFactor() const override;
   virtual void GetLiftingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax) const override;
   virtual Float64 GetLiftingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey,Float64 fci, bool includeRebar) const override;
   virtual Float64 GetLiftingAllowableGlobalCompressiveConcreteStressEx(const CSegmentKey& segmentKey, Float64 fci) const override;
   virtual Float64 GetLiftingAllowablePeakCompressiveConcreteStressEx(const CSegmentKey& segmentKey, Float64 fci) const override;
   virtual Float64 GetLiftingModulusOfRupture(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetLiftingModulusOfRupture(const CSegmentKey& segmentKey,Float64 fci,pgsTypes::ConcreteType concType) const override;
   virtual Float64 GetLiftingModulusOfRuptureFactor(pgsTypes::ConcreteType concType) const override;
   virtual Float64 GetMinimumLiftingPointLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const override;
   virtual Float64 GetLiftingPointLocationAccuracy() const override;
   virtual Float64 GetLiftingCamberMultiplier() const override;
   virtual pgsTypes::WindType GetLiftingWindType() const override;
   virtual Float64 GetLiftingWindLoad() const override;
   virtual WBFL::Stability::LiftingCriteria GetLiftingStabilityCriteria(const CSegmentKey& segmentKey) const override;
   virtual WBFL::Stability::LiftingCriteria GetLiftingStabilityCriteria(const CSegmentKey& segmentKey,const HANDLINGCONFIG& liftConfig) const override;

// ISegmentHaulingSpecCriteria
public:
   virtual bool IsHaulingAnalysisEnabled() const override;
   virtual pgsTypes::HaulingAnalysisMethod GetHaulingAnalysisMethod() const override;
   virtual void GetHaulingImpact(Float64* pDownward, Float64* pUpward) const override;
   virtual Float64 GetHaulingCrackingFs() const override;
   virtual Float64 GetHaulingRolloverFs() const override;
   virtual void GetHaulingAllowableTensileConcreteStressParametersNormalCrown(Float64* factor,bool* pbMax,Float64* fmax) const override;
   virtual Float64 GetHaulingAllowableTensileConcreteStressNormalCrown(const CSegmentKey& segmentKey) const override;
   virtual void GetHaulingAllowableTensileConcreteStressParametersMaxSuper(Float64* factor,bool* pbMax,Float64* fmax) const override;
   virtual Float64 GetHaulingAllowableTensileConcreteStressMaxSuper(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetHaulingAllowableGlobalCompressiveConcreteStress(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetHaulingAllowablePeakCompressiveConcreteStress(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetHaulingAllowableTensionFactorNormalCrown() const override;
   virtual Float64 GetHaulingAllowableTensionFactorMaxSuper() const override;
   virtual Float64 GetHaulingAllowableGlobalCompressionFactor() const override;
   virtual Float64 GetHaulingAllowablePeakCompressionFactor() const override;
   virtual Float64 GetHaulingAllowableTensileConcreteStressExNormalCrown(const CSegmentKey& segmentKey,Float64 fc, bool includeRebar) const override;
   virtual Float64 GetHaulingAllowableTensileConcreteStressExMaxSuper(const CSegmentKey& segmentKey,Float64 fc, bool includeRebar) const override;
   virtual Float64 GetHaulingAllowableGlobalCompressiveConcreteStressEx(const CSegmentKey& segmentKey, Float64 fc) const override;
   virtual Float64 GetHaulingAllowablePeakCompressiveConcreteStressEx(const CSegmentKey& segmentKey, Float64 fc) const override;
   virtual pgsTypes::HaulingImpact GetHaulingImpactUsage() const override;
   virtual Float64 GetNormalCrownSlope() const override;
   virtual Float64 GetMaxSuperelevation() const override;
   virtual Float64 GetHaulingSweepTolerance() const override;
   virtual Float64 GetHaulingSweepGrowth() const override;
   virtual Float64 GetHaulingSupportPlacementTolerance() const override;
   virtual Float64 GetHaulingCamberMultiplier() const override;
   virtual Float64 GetRollStiffness(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetHeightOfGirderBottomAboveRoadway(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetHeightOfTruckRollCenterAboveRoadway(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetAxleWidth(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetAllowableDistanceBetweenSupports(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetAllowableLeadingOverhang(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetMaxGirderWgt(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetHaulingWithMildRebarAllowableStressNormalCrown(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetHaulingWithMildRebarAllowableStressFactorNormalCrown() const override;
   virtual Float64 GetHaulingWithMildRebarAllowableStressMaxSuper(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetHaulingWithMildRebarAllowableStressFactorMaxSuper(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetHaulingModulusOfRupture(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetHaulingModulusOfRupture(const CSegmentKey& segmentKey,Float64 fci,pgsTypes::ConcreteType concType) const override;
   virtual Float64 GetHaulingModulusOfRuptureFactor(pgsTypes::ConcreteType concType) const override;
   virtual Float64 GetMinimumHaulingSupportLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const override;
   virtual Float64 GetHaulingSupportLocationAccuracy() const override;
   virtual pgsTypes::WindType GetHaulingWindType() const override;
   virtual Float64 GetHaulingWindLoad() const override;
   virtual pgsTypes::CFType GetCentrifugalForceType() const override;
   virtual Float64 GetHaulingSpeed() const override;
   virtual Float64 GetTurningRadius() const override;
   virtual WBFL::Stability::HaulingCriteria GetHaulingStabilityCriteria(const CSegmentKey& segmentKey) const override;
   virtual WBFL::Stability::HaulingCriteria GetHaulingStabilityCriteria(const CSegmentKey& segmentKey,const HANDLINGCONFIG& haulConfig) const override;

// IKdotGirderHaulingSpecCriteria
public:
   // Spec criteria for KDOT analyses
   virtual Float64 GetKdotHaulingAllowableTensileConcreteStress(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetKdotHaulingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetKdotHaulingAllowableTensionFactor() const override;
   virtual Float64 GetKdotHaulingAllowableCompressionFactor() const override;
   virtual Float64 GetKdotHaulingWithMildRebarAllowableStress(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetKdotHaulingWithMildRebarAllowableStressFactor() const override;
   virtual void GetKdotHaulingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax) const override;
   virtual Float64 GetKdotHaulingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc, bool includeRebar) const override;
   virtual Float64 GetKdotHaulingAllowableCompressiveConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc) const override;
   virtual void GetMinimumHaulingSupportLocation(Float64* pHardDistance, bool* pUseFactoredLength, Float64* pLengthFactor) const override;
   virtual Float64 GetHaulingDesignLocationAccuracy() const override;
   virtual void GetHaulingGFactors(Float64* pOverhangFactor, Float64* pInteriorFactor) const override;

// IDebondLimits
public:
   virtual bool CheckMaxDebondedStrands(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetMaxDebondedStrands(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetMaxDebondedStrandsPerRow(const CSegmentKey& segmentKey) const override;
   virtual void GetMaxDebondedStrandsPerSection(const CSegmentKey& segmentKey, StrandIndexType* p10orLess, StrandIndexType* pNS, bool* pbCheckMax, Float64* pMaxFraction) const override;
   virtual void GetMaxDebondLength(const CSegmentKey& segmentKey, Float64* pLen, pgsTypes::DebondLengthControl* pControl) const override;
   virtual void GetMinDistanceBetweenDebondSections(const CSegmentKey& segmentKey, Float64* pndb, bool* pbUseMinDistance, Float64* pMinDistance) const override;
   virtual Float64 GetMinDistanceBetweenDebondSections(const CSegmentKey& segmentKey) const override;
   virtual bool CheckDebondingSymmetry(const CSegmentKey& segmentKey) const override;
   virtual bool CheckAdjacentDebonding(const CSegmentKey& segmentKey) const override;
   virtual bool CheckDebondingInWebWidthProjections(const CSegmentKey& segmentKey) const override;
   virtual bool IsExteriorStrandBondingRequiredInRow(const CSegmentKey& segmentKey, pgsTypes::MemberEndType endType, RowIndexType rowIdx) const override;

// IResistanceFactors
public:
   virtual void GetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64* phiTensionPS,Float64* phiTensionRC,Float64* phiTensionSpliced,Float64* phiCompression) const override;
   virtual void GetFlexuralStrainLimits(matPsStrand::Grade grade,matPsStrand::Type type,Float64* pecl,Float64* petl) const override;
   virtual void GetFlexuralStrainLimits(matRebar::Grade rebarGrade,Float64* pecl,Float64* petl) const override;
   virtual Float64 GetShearResistanceFactor(const pgsPointOfInterest& poi, pgsTypes::ConcreteType type) const override;
   virtual Float64 GetShearResistanceFactor(bool isDebonded, pgsTypes::ConcreteType type) const override;
   virtual Float64 GetClosureJointFlexureResistanceFactor(pgsTypes::ConcreteType type) const override;
   virtual Float64 GetClosureJointShearResistanceFactor(pgsTypes::ConcreteType type) const override;

// IInterfaceShearRequirements 
public:
   virtual ShearFlowMethod GetShearFlowMethod() const override;
   virtual Float64 GetMaxShearConnectorSpacing(const pgsPointOfInterest& poi) const override;

// IDuctLimits
public:
   virtual Float64 GetSegmentTendonRadiusOfCurvatureLimit(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentTendonAreaLimit(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSegmentTendonDuctSizeLimit(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetGirderTendonRadiusOfCurvatureLimit(const CGirderKey& girderKey) const override;
   virtual Float64 GetGirderTendonAreaLimit(const CGirderKey& girderKey) const override;
   virtual Float64 GetGirderTendonDuctSizeLimit(const CGirderKey& girderKey) const override;
   virtual Float64 GetTendonAreaLimit(pgsTypes::StrandInstallationType installationType) const override;
   virtual Float64 GetSegmentDuctDeductionFactor(const CSegmentKey& segmentKey, IntervalIndexType intervalIdx) const override;
   virtual Float64 GetGirderDuctDeductionFactor(const CGirderKey& girderKey, DuctIndexType ductIdx, IntervalIndexType intervalIdx) const override;

private:
   DECLARE_EAF_AGENT_DATA;
   DECLARE_LOGFILE;

   DWORD m_dwBridgeDescCookie;

   StatusCallbackIDType m_scidHaulTruckError;

   const GirderLibraryEntry* GetGirderEntry(const CSegmentKey& segmentKey) const;
   const SpecLibraryEntry* GetSpec() const;

   Float64 GetRadiusOfCurvatureLimit(pgsTypes::DuctType ductType) const;
   Float64 GetDuctDeductFactor(IntervalIndexType intervalIdx, IntervalIndexType groutDuctIntervalIdx) const;

   bool IsLoadRatingServiceIIILimitState(pgsTypes::LimitState ls) const;
   void ValidateHaulTruck(const CPrecastSegmentData* pSegment) const;
   void Invalidate();
};

#endif //__SPECAGENT_H_
