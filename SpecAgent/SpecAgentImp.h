///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

// IAllowableStrandStress
public:
   virtual bool CheckStressAtJacking() override;
   virtual bool CheckStressBeforeXfer() override;
   virtual bool CheckStressAfterXfer() override;
   virtual bool CheckStressAfterLosses() override;
   virtual Float64 GetAllowableAtJacking(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) override;
   virtual Float64 GetAllowableBeforeXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) override;
   virtual Float64 GetAllowableAfterXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) override;
   virtual Float64 GetAllowableAfterLosses(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) override;

// IAllowableTendonStress
public:
   virtual bool CheckTendonStressAtJacking() override;
   virtual bool CheckTendonStressPriorToSeating() override;
   virtual Float64 GetAllowableAtJacking(const CGirderKey& girderKey) override;
   virtual Float64 GetAllowablePriorToSeating(const CGirderKey& girderKey) override;
   virtual Float64 GetAllowableAfterAnchorSetAtAnchorage(const CGirderKey& girderKey) override;
   virtual Float64 GetAllowableAfterAnchorSet(const CGirderKey& girderKey) override;
   virtual Float64 GetAllowableAfterLosses(const CGirderKey& girderKey) override;
   virtual Float64 GetAllowableCoefficientAtJacking(const CGirderKey& girderKey) override;
   virtual Float64 GetAllowableCoefficientPriorToSeating(const CGirderKey& girderKey) override;
   virtual Float64 GetAllowableCoefficientAfterAnchorSetAtAnchorage(const CGirderKey& girderKey) override;
   virtual Float64 GetAllowableCoefficientAfterAnchorSet(const CGirderKey& girderKey) override;
   virtual Float64 GetAllowableCoefficientAfterLosses(const CGirderKey& girderKey) override;

// IAllowableConcreteStress
public:
   virtual Float64 GetAllowableCompressionStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) override;
   virtual Float64 GetAllowableTensionStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) override;

   virtual Float64 GetAllowableTensionStress(pgsTypes::LoadRatingType ratingType,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation) override;

   virtual Float64 GetAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) override;
   virtual void GetAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) override;

   virtual std::vector<Float64> CSpecAgentImp::GetGirderAllowableCompressionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) override;
   virtual std::vector<Float64> CSpecAgentImp::GetDeckAllowableCompressionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) override;
   virtual std::vector<Float64> CSpecAgentImp::GetGirderAllowableTensionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondededReinforcement,bool bInPrecompressedTensileZone) override;
   virtual std::vector<Float64> CSpecAgentImp::GetDeckAllowableTensionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondededReinforcement) override;

   virtual Float64 GetSegmentAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) override;
   virtual Float64 GetClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) override;
   virtual Float64 GetDeckAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) override;

   virtual Float64 GetSegmentAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc) override;
   virtual Float64 GetClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc) override;
   virtual Float64 GetDeckAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc) override;

   virtual Float64 GetSegmentAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement) override;
   virtual Float64 GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) override;
   virtual Float64 GetDeckAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement) override;

   virtual Float64 GetSegmentAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc,bool bWithBondedReinforcement) override;
   virtual Float64 GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) override;
   virtual Float64 GetDeckAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc,bool bWithBondedReinforcement) override;

   virtual Float64 GetSegmentAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) override;
   virtual Float64 GetClosureJointAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) override;
   virtual Float64 GetDeckAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) override;

   virtual void GetSegmentAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) override;
   virtual void GetClosureJointAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) override;
   virtual void GetDeckAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) override;

   virtual std::vector<pgsTypes::LimitState> GetStressCheckLimitStates(IntervalIndexType intervalIdx) override;
   virtual bool IsStressCheckApplicable(const CGirderKey& girderKey,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::StressType stressType) override;
   virtual bool HasAllowableTensionWithRebarOption(IntervalIndexType intervalIdx,bool bInPTZ,bool bSegment,const CSegmentKey& segmentKey) override;

   virtual bool CheckTemporaryStresses() override;
   virtual bool CheckFinalDeadLoadTensionStress() override;

// ITransverseReinforcementSpec
public:
   virtual Float64 GetMaxSplittingStress(Float64 fyRebar) override;
   virtual Float64 GetSplittingZoneLength( Float64 girderHeight ) override;
   virtual Float64 CSpecAgentImp::GetSplittingZoneLengthFactor() override;
   virtual matRebar::Size GetMinConfinmentBarSize() override;
   virtual Float64 GetMaxConfinmentBarSpacing() override;
   virtual Float64 GetMinConfinmentAvS() override;
   virtual void GetMaxStirrupSpacing(Float64 dv,Float64* sUnderLimit, Float64* sOverLimit) override;
   virtual Float64 GetMinStirrupSpacing(Float64 maxAggregateSize, Float64 barDiameter) override;

// IPrecastIGirderDetailsSpec
public:
   virtual Float64 GetMinTopFlangeThickness() override;
   virtual Float64 GetMinWebThickness() override;
   virtual Float64 GetMinBottomFlangeThickness() override;

// ISegmentLiftingSpecCriteria
public:
   virtual bool IsLiftingAnalysisEnabled() override;
   virtual void GetLiftingImpact(Float64* pDownward, Float64* pUpward) override;
   virtual Float64 GetLiftingCrackingFs() override;
   virtual Float64 GetLiftingFailureFs() override;
   virtual Float64 GetLiftingAllowableTensileConcreteStress(const CSegmentKey& segmentKey) override;
   virtual Float64 GetLiftingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey) override;
   virtual Float64 GetLiftingAllowableCompressionFactor() override;
   virtual Float64 GetLiftingAllowableTensionFactor() override;
   virtual Float64 GetHeightOfPickPointAboveGirderTop() override;
   virtual Float64 GetLiftingLoopPlacementTolerance() override;
   virtual Float64 GetLiftingCableMinInclination() override;
   virtual Float64 GetLiftingSweepTolerance() override;
   virtual Float64 GetLiftingWithMildRebarAllowableStress(const CSegmentKey& segmentKey) override;
   virtual Float64 GetLiftingWithMildRebarAllowableStressFactor() override;
   virtual void GetLiftingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax) override;
   virtual Float64 GetLiftingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey,Float64 fci, bool includeRebar) override;
   virtual Float64 GetLiftingAllowableCompressiveConcreteStressEx(const CSegmentKey& segmentKey,Float64 fci) override;
   virtual Float64 GetLiftingModulusOfRupture(const CSegmentKey& segmentKey) override;
   virtual Float64 GetLiftingModulusOfRupture(const CSegmentKey& segmentKey,Float64 fci,pgsTypes::ConcreteType concType) override;
   virtual Float64 GetLiftingModulusOfRuptureFactor(pgsTypes::ConcreteType concType) override;
   virtual Float64 GetMinimumLiftingPointLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) override;
   virtual Float64 GetLiftingPointLocationAccuracy() override;
   virtual pgsTypes::CamberMethod GetLiftingCamberMethod() override;
   virtual Float64 GetLiftingIncreaseInCgForCamber() override;
   virtual Float64 GetLiftingCamberMultiplier() override;
   virtual pgsTypes::WindType GetLiftingWindType() override;
   virtual Float64 GetLiftingWindLoad() override;
   virtual bool EvaluateLiftingStressesAtEquilibriumAngle() override;
   virtual stbLiftingCriteria GetLiftingStabilityCriteria(const CSegmentKey& segmentKey) override;
   virtual stbLiftingCriteria GetLiftingStabilityCriteria(const CSegmentKey& segmentKey,const HANDLINGCONFIG& liftConfig) override;

// ISegmentHaulingSpecCriteria
public:
   virtual bool IsHaulingAnalysisEnabled() override;
   virtual pgsTypes::HaulingAnalysisMethod GetHaulingAnalysisMethod() override;
   virtual void GetHaulingImpact(Float64* pDownward, Float64* pUpward) override;
   virtual Float64 GetHaulingCrackingFs() override;
   virtual Float64 GetHaulingRolloverFs() override;
   virtual void GetHaulingAllowableTensileConcreteStressParametersNormalCrown(Float64* factor,bool* pbMax,Float64* fmax) override;
   virtual Float64 GetHaulingAllowableTensileConcreteStressNormalCrown(const CSegmentKey& segmentKey) override;
   virtual void GetHaulingAllowableTensileConcreteStressParametersMaxSuper(Float64* factor,bool* pbMax,Float64* fmax) override;
   virtual Float64 GetHaulingAllowableTensileConcreteStressMaxSuper(const CSegmentKey& segmentKey) override;
   virtual Float64 GetHaulingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey) override;
   virtual Float64 GetHaulingAllowableTensionFactorNormalCrown() override;
   virtual Float64 GetHaulingAllowableTensionFactorMaxSuper() override;
   virtual Float64 GetHaulingAllowableCompressionFactor() override;
   virtual Float64 GetHaulingAllowableTensileConcreteStressExNormalCrown(const CSegmentKey& segmentKey,Float64 fc, bool includeRebar) override;
   virtual Float64 GetHaulingAllowableTensileConcreteStressExMaxSuper(const CSegmentKey& segmentKey,Float64 fc, bool includeRebar) override;
   virtual Float64 GetHaulingAllowableCompressiveConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc) override;
   virtual pgsTypes::HaulingImpact GetHaulingImpactUsage() override;
   virtual Float64 GetNormalCrownSlope() override;
   virtual Float64 GetMaxSuperelevation() override;
   virtual Float64 GetHaulingSweepTolerance() override;
   virtual Float64 GetHaulingSupportPlacementTolerance() override;
   virtual pgsTypes::CamberMethod GetHaulingCamberMethod() override;
   virtual Float64 GetHaulingIncreaseInCgForCamber() override;
   virtual Float64 GetHaulingCamberMultiplier() override;
   virtual Float64 GetRollStiffness(const CSegmentKey& segmentKey) override;
   virtual Float64 GetHeightOfGirderBottomAboveRoadway(const CSegmentKey& segmentKey) override;
   virtual Float64 GetHeightOfTruckRollCenterAboveRoadway(const CSegmentKey& segmentKey) override;
   virtual Float64 GetAxleWidth(const CSegmentKey& segmentKey) override;
   virtual Float64 GetAllowableDistanceBetweenSupports(const CSegmentKey& segmentKey) override;
   virtual Float64 GetAllowableLeadingOverhang(const CSegmentKey& segmentKey) override;
   virtual Float64 GetMaxGirderWgt(const CSegmentKey& segmentKey) override;
   virtual Float64 GetHaulingWithMildRebarAllowableStressNormalCrown(const CSegmentKey& segmentKey) override;
   virtual Float64 GetHaulingWithMildRebarAllowableStressFactorNormalCrown() override;
   virtual Float64 GetHaulingWithMildRebarAllowableStressMaxSuper(const CSegmentKey& segmentKey) override;
   virtual Float64 GetHaulingWithMildRebarAllowableStressFactorMaxSuper(const CSegmentKey& segmentKey) override;
   virtual Float64 GetHaulingModulusOfRupture(const CSegmentKey& segmentKey) override;
   virtual Float64 GetHaulingModulusOfRupture(const CSegmentKey& segmentKey,Float64 fci,pgsTypes::ConcreteType concType) override;
   virtual Float64 GetHaulingModulusOfRuptureFactor(pgsTypes::ConcreteType concType) override;
   virtual Float64 GetMinimumHaulingSupportLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) override;
   virtual Float64 GetHaulingSupportLocationAccuracy() override;
   virtual pgsTypes::WindType GetHaulingWindType() override;
   virtual Float64 GetHaulingWindLoad() override;
   virtual pgsTypes::CFType GetCentrifugalForceType() override;
   virtual Float64 GetHaulingSpeed() override;
   virtual Float64 GetTurningRadius() override;
   virtual bool EvaluateHaulingStressesAtEquilibriumAngle() override;
   virtual stbHaulingCriteria GetHaulingStabilityCriteria(const CSegmentKey& segmentKey) override;
   virtual stbHaulingCriteria GetHaulingStabilityCriteria(const CSegmentKey& segmentKey,const HANDLINGCONFIG& haulConfig) override;

// IKdotGirderHaulingSpecCriteria
public:
   // Spec criteria for KDOT analyses
   virtual Float64 GetKdotHaulingAllowableTensileConcreteStress(const CSegmentKey& segmentKey) override;
   virtual Float64 GetKdotHaulingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey) override;
   virtual Float64 GetKdotHaulingAllowableTensionFactor() override;
   virtual Float64 GetKdotHaulingAllowableCompressionFactor() override;
   virtual Float64 GetKdotHaulingWithMildRebarAllowableStress(const CSegmentKey& segmentKey) override;
   virtual Float64 GetKdotHaulingWithMildRebarAllowableStressFactor() override;
   virtual void GetKdotHaulingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax) override;
   virtual Float64 GetKdotHaulingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc, bool includeRebar) override;
   virtual Float64 GetKdotHaulingAllowableCompressiveConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc) override;
   virtual void GetMinimumHaulingSupportLocation(Float64* pHardDistance, bool* pUseFactoredLength, Float64* pLengthFactor) override;
   virtual Float64 GetHaulingDesignLocationAccuracy() override;
   virtual void GetHaulingGFactors(Float64* pOverhangFactor, Float64* pInteriorFactor) override;

// IDebondLimits
public:
   virtual Float64 GetMaxDebondedStrands(const CSegmentKey& segmentKey) override;
   virtual Float64 GetMaxDebondedStrandsPerRow(const CSegmentKey& segmentKey) override;
   virtual Float64 GetMaxDebondedStrandsPerSection(const CSegmentKey& segmentKey) override;
   virtual StrandIndexType GetMaxNumDebondedStrandsPerSection(const CSegmentKey& segmentKey) override;
   virtual void GetMaxDebondLength(const CSegmentKey& segmentKey, Float64* pLen, pgsTypes::DebondLengthControl* pControl) override;
   virtual Float64 GetMinDebondSectionDistance(const CSegmentKey& segmentKey) override;

// IResistanceFactors
public:
   virtual void GetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64* phiTensionPS,Float64* phiTensionRC,Float64* phiTensionSpliced,Float64* phiCompression) override;
   virtual void GetFlexuralStrainLimits(matPsStrand::Grade grade,matPsStrand::Type type,Float64* pecl,Float64* petl) override;
   virtual void GetFlexuralStrainLimits(matRebar::Grade rebarGrade,Float64* pecl,Float64* petl) override;
   virtual Float64 GetShearResistanceFactor(pgsTypes::ConcreteType type) override;
   virtual Float64 GetClosureJointFlexureResistanceFactor(pgsTypes::ConcreteType type) override;
   virtual Float64 GetClosureJointShearResistanceFactor(pgsTypes::ConcreteType type) override;

// IInterfaceShearRequirements 
public:
   virtual ShearFlowMethod GetShearFlowMethod() override;
   virtual Float64 GetMaxShearConnectorSpacing(const pgsPointOfInterest& poi) override;

// IDuctLimits
public:
   virtual Float64 GetRadiusOfCurvatureLimit(const CGirderKey& girderKey) override;
   virtual Float64 GetTendonAreaLimit(const CGirderKey& girderKey) override;
   virtual Float64 GetDuctSizeLimit(const CGirderKey& girderKey) override;

private:
   DECLARE_EAF_AGENT_DATA;

   DWORD m_dwBridgeDescCookie;

   StatusCallbackIDType m_scidHaulTruckError;

   const GirderLibraryEntry* GetGirderEntry(const CSegmentKey& segmentKey);
   const SpecLibraryEntry* GetSpec();

   bool IsLoadRatingServiceIIILimitState(pgsTypes::LimitState ls);
   void ValidateHaulTruck(const CPrecastSegmentData* pSegment);
   void Invalidate();
};

#endif //__SPECAGENT_H_
