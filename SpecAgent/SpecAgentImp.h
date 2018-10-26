///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
   public IAllowableConcreteStress,
   public ITransverseReinforcementSpec,
   public IPrecastIGirderDetailsSpec,
   public IGirderHaulingSpecCriteriaEx,
   public IGirderLiftingSpecCriteriaEx,
   public IDebondLimits,
   public IResistanceFactors
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
   COM_INTERFACE_ENTRY(IAllowableConcreteStress)
   COM_INTERFACE_ENTRY(ITransverseReinforcementSpec)
   COM_INTERFACE_ENTRY(IPrecastIGirderDetailsSpec)
   COM_INTERFACE_ENTRY(IGirderLiftingSpecCriteria)
   COM_INTERFACE_ENTRY(IGirderLiftingSpecCriteriaEx)
   COM_INTERFACE_ENTRY(IGirderHaulingSpecCriteria)
   COM_INTERFACE_ENTRY(IGirderHaulingSpecCriteriaEx)
   COM_INTERFACE_ENTRY(IDebondLimits)
   COM_INTERFACE_ENTRY(IResistanceFactors)
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
   virtual Float64 GetAllowableAtJacking(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType);
   virtual Float64 GetAllowableBeforeXfer(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType);
   virtual Float64 GetAllowableAfterXfer(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType);
   virtual Float64 GetAllowableAfterLosses(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType);

// IAllowableConcreteStress
public:
   virtual Float64 GetAllowableStress(const pgsPointOfInterest& poi, pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType type);
   virtual std::vector<Float64> GetAllowableStress(const std::vector<pgsPointOfInterest>& vPoi, pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType type);
   virtual Float64 GetAllowableStress(pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType type,Float64 fc);
   virtual Float64 GetCastingYardWithMildRebarAllowableStress(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetAllowableCompressiveStressCoefficient(pgsTypes::Stage stage,pgsTypes::LimitState ls);
   virtual void GetAllowableTensionStressCoefficient(pgsTypes::Stage stage,pgsTypes::LimitState ls,double* pCoeff,bool* pbMax,double* pMaxValue);
   virtual Float64 GetCastingYardAllowableStress(pgsTypes::LimitState ls,pgsTypes::StressType type,Float64 fc);
   virtual Float64 GetBridgeSiteAllowableStress(pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType type,Float64 fc);
   virtual Float64 GetInitialAllowableCompressiveStress(Float64 fci);
   virtual Float64 GetInitialAllowableTensileStress(Float64 fci, bool useMinRebar);
   virtual Float64 GetFinalAllowableCompressiveStress(pgsTypes::Stage stage,pgsTypes::LimitState ls,Float64 fc);
   virtual Float64 GetFinalAllowableTensileStress(pgsTypes::Stage stage, Float64 fc);
   virtual Float64 GetCastingYardAllowableTensionStressCoefficientWithRebar();

// ITransverseReinforcementSpec
public:
   virtual Float64 GetMaxSplittingStress(Float64 fyRebar);
   virtual Float64 GetSplittingZoneLength( Float64 girderHeight );
   virtual Float64 CSpecAgentImp::GetSplittingZoneLengthFactor();
   virtual matRebar::Size GetMinConfinmentBarSize();
   virtual Float64 GetMaxConfinmentBarSpacing();
   virtual Float64 GetMinConfinmentAvS();
   virtual Float64 GetAvOverSMin(Float64 fc, Float64 bv, Float64 fy); // obsolete
   virtual void GetMaxStirrupSpacing(Float64* sUnderLimit, Float64* sOverLimit);
   virtual Float64 GetMinStirrupSpacing(Float64 maxAggregateSize, Float64 barDiameter);

// IPrecastIGirderDetailsSpec
public:
   virtual Float64 GetMinTopFlangeThickness() const;
   virtual Float64 GetMinWebThickness() const;
   virtual Float64 GetMinBottomFlangeThickness() const;

// IGirderLiftingSpecCriteria
public:
   virtual bool IsLiftingCheckEnabled() const;
   virtual void GetLiftingImpact(Float64* pDownward, Float64* pUpward) const;
   virtual Float64 GetLiftingCrackingFs() const;
   virtual Float64 GetLiftingFailureFs() const;
   virtual Float64 GetLiftingAllowableTensileConcreteStress(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetLiftingAllowableCompressiveConcreteStress(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetLiftingAllowableCompressionFactor();
   virtual Float64 GetLiftingAllowableTensionFactor();
   virtual Float64 GetHeightOfPickPointAboveGirderTop() const;
   virtual Float64 GetLiftingLoopPlacementTolerance() const;
   virtual Float64 GetLiftingCableMinInclination() const;
   virtual Float64 GetLiftingSweepTolerance()const;
   virtual Float64 GetLiftingWithMildRebarAllowableStress(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetLiftingWithMildRebarAllowableStressFactor();
   virtual void GetLiftingAllowableTensileConcreteStressParameters(double* factor,bool* pbMax,double* fmax);
   virtual Float64 GetLiftingAllowableTensileConcreteStressEx(Float64 fci, bool includeRebar);
   virtual Float64 GetLiftingAllowableCompressiveConcreteStressEx(Float64 fci);
   virtual Float64 GetLiftingModulusOfRupture(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetLiftingModulusOfRupture(Float64 fci);
   virtual Float64 GetLiftingModulusOfRuptureCoefficient();
   virtual Float64 GetMinimumLiftingPointLocation(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType end) const;
   virtual Float64 GetLiftingPointLocationAccuracy() const;

// IGirderLiftingSpecCriteriaEx
public:
   virtual Float64 GetLiftingModulusOfRuptureCoefficient(pgsTypes::ConcreteType concType);
   virtual Float64 GetLiftingModulusOfRupture(Float64 fci,pgsTypes::ConcreteType concType);

// IGirderHaulingSpecCriteria
public:
   virtual bool IsHaulingCheckEnabled() const;
   virtual void GetHaulingImpact(Float64* pDownward, Float64* pUpward) const;
   virtual Float64 GetHaulingCrackingFs() const;
   virtual Float64 GetHaulingRolloverFs() const;
   virtual void GetHaulingAllowableTensileConcreteStressParameters(double* factor,bool* pbMax,double* fmax);
   virtual Float64 GetHaulingAllowableTensileConcreteStress(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetHaulingAllowableCompressiveConcreteStress(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetHaulingAllowableTensionFactor();
   virtual Float64 GetHaulingAllowableCompressionFactor();
   virtual Float64 GetHaulingAllowableTensileConcreteStressEx(Float64 fc, bool includeRebar);
   virtual Float64 GetHaulingAllowableCompressiveConcreteStressEx(Float64 fc);
   virtual IGirderHaulingSpecCriteria::RollStiffnessMethod GetRollStiffnessMethod() const;
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
   virtual Float64 GetHaulingWithMildRebarAllowableStress(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetHaulingWithMildRebarAllowableStressFactor();
   virtual Float64 GetHaulingModulusOfRuptureCoefficient();
   virtual Float64 GetHaulingModulusOfRupture(Float64 fc);
   virtual Float64 GetHaulingModulusOfRupture(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetMinimumHaulingSupportLocation(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType end) const;
   virtual Float64 GetHaulingSupportLocationAccuracy() const;

// IGirderHaulingSpecCriteriaEx
public:
   virtual Float64 GetHaulingModulusOfRuptureCoefficient(pgsTypes::ConcreteType concType);
   virtual Float64 GetHaulingModulusOfRupture(Float64 fci,pgsTypes::ConcreteType concType);

// IDebondLimits
public:
   virtual Float64 GetMaxDebondedStrands(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetMaxDebondedStrandsPerRow(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetMaxDebondedStrandsPerSection(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual StrandIndexType GetMaxNumDebondedStrandsPerSection(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual void GetMaxDebondLength(SpanIndexType spanIdx, GirderIndexType gdrIdx, Float64* pLen, pgsTypes::DebondLengthControl* pControl); 
   virtual Float64 GetMinDebondSectionDistance(SpanIndexType spanIdx,GirderIndexType gdrIdx); 

// IResistanceFactors
public:
   virtual void GetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64* phiTensionPS,Float64* phiTensionRC,Float64* phiCompression);
   virtual Float64 GetShearResistanceFactor(pgsTypes::ConcreteType type);


private:
   DECLARE_AGENT_DATA;

   const GirderLibraryEntry* GetGirderEntry(SpanIndexType spanIdx,GirderIndexType gdrIdx) const;
   const SpecLibraryEntry* GetSpec() const;
};

#endif //__SPECAGENT_H_
