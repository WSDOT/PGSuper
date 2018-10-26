///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

class GirderLibraryEntry;
class SpecLibraryEntry;

/////////////////////////////////////////////////////////////////////////////
// CSpecAgentImp
class ATL_NO_VTABLE CSpecAgentImp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSpecAgentImp, &CLSID_SpecAgent>,
	public IConnectionPointContainerImpl<CSpecAgentImp>,
	public IAgentEx,
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
   COM_INTERFACE_ENTRY(IDuctLimits)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CSpecAgentImp)
END_CONNECTION_POINT_MAP()


// IAgent
public:
	STDMETHOD(SetBroker)(IBroker* pBroker);
	STDMETHOD(RegInterfaces)();
	STDMETHOD(Init)();
	STDMETHOD(Reset)();
	STDMETHOD(ShutDown)();
   STDMETHOD(Init2)();
   STDMETHOD(GetClassID)(CLSID* pCLSID);

// IAllowableStrandStress
public:
   virtual bool CheckStressAtJacking();
   virtual bool CheckStressBeforeXfer();
   virtual bool CheckStressAfterXfer();
   virtual bool CheckStressAfterLosses();
   virtual Float64 GetAllowableAtJacking(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType);
   virtual Float64 GetAllowableBeforeXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType);
   virtual Float64 GetAllowableAfterXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType);
   virtual Float64 GetAllowableAfterLosses(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType);

// IAllowableTendonStress
public:
   virtual bool CheckTendonStressAtJacking();
   virtual bool CheckTendonStressPriorToSeating();
   virtual Float64 GetAllowableAtJacking(const CGirderKey& girderKey);
   virtual Float64 GetAllowablePriorToSeating(const CGirderKey& girderKey);
   virtual Float64 GetAllowableAfterAnchorSetAtAnchorage(const CGirderKey& girderKey);
   virtual Float64 GetAllowableAfterAnchorSet(const CGirderKey& girderKey);
   virtual Float64 GetAllowableAfterLosses(const CGirderKey& girderKey);

// IAllowableConcreteStress
public:
   virtual Float64 GetAllowableCompressionStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls);
   virtual Float64 GetAllowableTensionStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone);

   virtual Float64 GetAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls);
   virtual void GetAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone,Float64* pCoeff,bool* pbMax,Float64* pMaxValue);

   virtual std::vector<Float64> CSpecAgentImp::GetGirderAllowableCompressionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls);
   virtual std::vector<Float64> CSpecAgentImp::GetDeckAllowableCompressionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls);
   virtual std::vector<Float64> CSpecAgentImp::GetGirderAllowableTensionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondededReinforcement,bool bInPrecompressedTensileZone);
   virtual std::vector<Float64> CSpecAgentImp::GetDeckAllowableTensionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondededReinforcement);

   virtual Float64 GetSegmentAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls);
   virtual Float64 GetClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls);
   virtual Float64 GetDeckAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls);

   virtual Float64 GetSegmentAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc);
   virtual Float64 GetClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc);
   virtual Float64 GetDeckAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc);

   virtual Float64 GetSegmentAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement);
   virtual Float64 GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone);
   virtual Float64 GetDeckAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement);

   virtual Float64 GetSegmentAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc,bool bWithBondedReinforcement);
   virtual Float64 GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone);
   virtual Float64 GetDeckAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc,bool bWithBondedReinforcement);

   virtual Float64 GetSegmentAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls);
   virtual Float64 GetClosureJointAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls);
   virtual Float64 GetDeckAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls);

   virtual void GetSegmentAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,Float64* pCoeff,bool* pbMax,Float64* pMaxValue);
   virtual void GetClosureJointAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone,Float64* pCoeff,bool* pbMax,Float64* pMaxValue);
   virtual void GetDeckAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,Float64* pCoeff,bool* pbMax,Float64* pMaxValue);

   virtual std::vector<pgsTypes::LimitState> GetStressCheckLimitStates();
   virtual bool IsStressCheckApplicable(const CGirderKey& girderKey,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::StressType stressType);
   virtual bool HasAllowableTensionWithRebarOption(IntervalIndexType intervalIdx,bool bInPTZ,bool bSegment,const CSegmentKey& segmentKey);

// ITransverseReinforcementSpec
public:
   virtual Float64 GetMaxSplittingStress(Float64 fyRebar);
   virtual Float64 GetSplittingZoneLength( Float64 girderHeight );
   virtual Float64 CSpecAgentImp::GetSplittingZoneLengthFactor();
   virtual matRebar::Size GetMinConfinmentBarSize();
   virtual Float64 GetMaxConfinmentBarSpacing();
   virtual Float64 GetMinConfinmentAvS();
   virtual void GetMaxStirrupSpacing(Float64* sUnderLimit, Float64* sOverLimit);
   virtual Float64 GetMinStirrupSpacing(Float64 maxAggregateSize, Float64 barDiameter);

// IPrecastIGirderDetailsSpec
public:
   virtual Float64 GetMinTopFlangeThickness() const;
   virtual Float64 GetMinWebThickness() const;
   virtual Float64 GetMinBottomFlangeThickness() const;

// ISegmentLiftingSpecCriteria
public:
   virtual bool IsLiftingAnalysisEnabled() const;
   virtual void GetLiftingImpact(Float64* pDownward, Float64* pUpward) const;
   virtual Float64 GetLiftingCrackingFs() const;
   virtual Float64 GetLiftingFailureFs() const;
   virtual Float64 GetLiftingAllowableTensileConcreteStress(const CSegmentKey& segmentKey);
   virtual Float64 GetLiftingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey);
   virtual Float64 GetLiftingAllowableCompressionFactor();
   virtual Float64 GetLiftingAllowableTensionFactor();
   virtual Float64 GetHeightOfPickPointAboveGirderTop() const;
   virtual Float64 GetLiftingLoopPlacementTolerance() const;
   virtual Float64 GetLiftingCableMinInclination() const;
   virtual Float64 GetLiftingSweepTolerance()const;
   virtual Float64 GetLiftingWithMildRebarAllowableStress(const CSegmentKey& segmentKey);
   virtual Float64 GetLiftingWithMildRebarAllowableStressFactor();
   virtual void GetLiftingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax);
   virtual Float64 GetLiftingAllowableTensileConcreteStressEx(Float64 fci, bool includeRebar);
   virtual Float64 GetLiftingAllowableCompressiveConcreteStressEx(Float64 fci);
   virtual Float64 GetLiftingModulusOfRupture(const CSegmentKey& segmentKey);
   virtual Float64 GetLiftingModulusOfRupture(Float64 fci,pgsTypes::ConcreteType concType);
   virtual Float64 GetLiftingModulusOfRuptureFactor(pgsTypes::ConcreteType concType);
   virtual Float64 GetMinimumLiftingPointLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const;
   virtual Float64 GetLiftingPointLocationAccuracy() const;

// ISegmentHaulingSpecCriteria
public:
   virtual bool IsHaulingAnalysisEnabled() const;
   virtual pgsTypes::HaulingAnalysisMethod GetHaulingAnalysisMethod() const;
   virtual void GetHaulingImpact(Float64* pDownward, Float64* pUpward) const;
   virtual Float64 GetHaulingCrackingFs() const;
   virtual Float64 GetHaulingRolloverFs() const;
   virtual void GetHaulingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax);
   virtual Float64 GetHaulingAllowableTensileConcreteStress(const CSegmentKey& segmentKey);
   virtual Float64 GetHaulingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey);
   virtual Float64 GetHaulingAllowableTensionFactor();
   virtual Float64 GetHaulingAllowableCompressionFactor();
   virtual Float64 GetHaulingAllowableTensileConcreteStressEx(Float64 fc, bool includeRebar);
   virtual Float64 GetHaulingAllowableCompressiveConcreteStressEx(Float64 fc);
   virtual ISegmentHaulingSpecCriteria::RollStiffnessMethod GetRollStiffnessMethod() const;
   virtual Float64 GetLumpSumRollStiffness() const;
   virtual Float64 GetAxleWeightLimit() const;
   virtual Float64 GetAxleStiffness() const;
   virtual Float64 GetMinimumRollStiffness() const;
   virtual Float64 GetHeightOfGirderBottomAboveRoadway() const;
   virtual Float64 GetHeightOfTruckRollCenterAboveRoadway() const;
   virtual Float64 GetAxleWidth() const;
   virtual Float64 GetMaxSuperelevation() const;
   virtual Float64 GetHaulingSweepTolerance()const;
   virtual Float64 GetHaulingSupportPlacementTolerance() const;
   virtual Float64 GetIncreaseInCgForCamber() const;
   virtual Float64 GetAllowableDistanceBetweenSupports() const;
   virtual Float64 GetAllowableLeadingOverhang() const;
   virtual Float64 GetMaxGirderWgt() const;
   virtual Float64 GetHaulingWithMildRebarAllowableStress(const CSegmentKey& segmentKey);
   virtual Float64 GetHaulingWithMildRebarAllowableStressFactor();
   virtual Float64 GetHaulingModulusOfRupture(const CSegmentKey& segmentKey);
   virtual Float64 GetHaulingModulusOfRupture(Float64 fci,pgsTypes::ConcreteType concType);
   virtual Float64 GetHaulingModulusOfRuptureFactor(pgsTypes::ConcreteType concType);
   virtual Float64 GetMinimumHaulingSupportLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const;
   virtual Float64 GetHaulingSupportLocationAccuracy() const;

// IKdotGirderHaulingSpecCriteria
public:
   // Spec criteria for KDOT analyses
   virtual Float64 GetKdotHaulingAllowableTensileConcreteStress(const CSegmentKey& segmentKey);
   virtual Float64 GetKdotHaulingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey);
   virtual Float64 GetKdotHaulingAllowableTensionFactor();
   virtual Float64 GetKdotHaulingAllowableCompressionFactor();
   virtual Float64 GetKdotHaulingWithMildRebarAllowableStress(const CSegmentKey& segmentKey);
   virtual Float64 GetKdotHaulingWithMildRebarAllowableStressFactor();
   virtual void GetKdotHaulingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax);
   virtual Float64 GetKdotHaulingAllowableTensileConcreteStressEx(Float64 fc, bool includeRebar);
   virtual Float64 GetKdotHaulingAllowableCompressiveConcreteStressEx(Float64 fc);
   virtual void GetMinimumHaulingSupportLocation(Float64* pHardDistance, bool* pUseFactoredLength, Float64* pLengthFactor) const;
   virtual Float64 GetHaulingDesignLocationAccuracy() const;
   virtual void GetHaulingGFactors(Float64* pOverhangFactor, Float64* pInteriorFactor) const;

// IDebondLimits
public:
   virtual Float64 GetMaxDebondedStrands(const CSegmentKey& segmentKey);
   virtual Float64 GetMaxDebondedStrandsPerRow(const CSegmentKey& segmentKey);
   virtual Float64 GetMaxDebondedStrandsPerSection(const CSegmentKey& segmentKey);
   virtual StrandIndexType GetMaxNumDebondedStrandsPerSection(const CSegmentKey& segmentKey);
   virtual void GetMaxDebondLength(const CSegmentKey& segmentKey, Float64* pLen, pgsTypes::DebondLengthControl* pControl); 
   virtual Float64 GetMinDebondSectionDistance(const CSegmentKey& segmentKey); 

// IResistanceFactors
public:
   virtual void GetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64* phiTensionPS,Float64* phiTensionRC,Float64* phiTensionSpliced,Float64* phiCompression);
   virtual void GetFlexuralStrainLimits(matPsStrand::Grade grade,matPsStrand::Type type,Float64* pecl,Float64* petl);
   virtual void GetFlexuralStrainLimits(matRebar::Grade rebarGrade,Float64* pecl,Float64* petl);
   virtual Float64 GetShearResistanceFactor(pgsTypes::ConcreteType type);
   virtual Float64 GetClosureJointFlexureResistanceFactor(pgsTypes::ConcreteType type);
   virtual Float64 GetClosureJointShearResistanceFactor(pgsTypes::ConcreteType type);

// IDuctLimits
public:
   virtual Float64 GetRadiusOfCurvatureLimit(const CGirderKey& girderKey);
   virtual Float64 GetTendonAreaLimit(const CGirderKey& girderKey);
   virtual Float64 GetDuctSizeLimit(const CGirderKey& girderKey);

private:
   DECLARE_EAF_AGENT_DATA;

   const GirderLibraryEntry* GetGirderEntry(const CSegmentKey& segmentKey) const;
   const SpecLibraryEntry* GetSpec() const;

   bool IsLoadRatingServiceIIILimitState(pgsTypes::LimitState ls);
   Float64 GetLoadRatingAllowableTension(const CSegmentKey& segmentKey,pgsTypes::LimitState ls);
};

#endif //__SPECAGENT_H_
