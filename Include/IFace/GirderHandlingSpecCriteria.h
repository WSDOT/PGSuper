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

#ifndef INCLUDED_IFACE_GIRDERHANDLINGSPECCRITERIA_H_
#define INCLUDED_IFACE_GIRDERHANDLINGSPECCRITERIA_H_

// SYSTEM INCLUDES
//
#if !defined INCLUDED_WBFLTYPES_H_
#include <WbflTypes.h>
#endif

#if !defined INCLUDED_PGSUPERTYPES_H_
#include <PGSuperTypes.h>
#endif

#if !defined INCLUDED_PGSEXT_POINTOFINTEREST_H_
#include <PgsExt\ReportPointOfInterest.h>
#endif

#include <Stability\Stability.h>

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   ISegmentLiftingSpecCriteria

   Interface to specification criteria for lifting

DESCRIPTION
   Interface to specification criteria for lifting
*****************************************************************************/
// {CA83DBFA-E62F-11d2-AD3D-00105A9AF985}
DEFINE_GUID(IID_ISegmentLiftingSpecCriteria, 
0xca83dbfa, 0xe62f, 0x11d2, 0xad, 0x3d, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface ISegmentLiftingSpecCriteria : IUnknown
{
   // do we analyze lifting or not?
   virtual bool IsLiftingAnalysisEnabled() const = 0;

   // impact
   virtual void GetLiftingImpact(Float64* pDownward, Float64* pUpward) const = 0;
   // factors of safety
   virtual Float64 GetLiftingCrackingFs() const = 0;
   virtual Float64 GetLiftingFailureFs() const = 0;
   // allowable concrete stresses
   virtual Float64 GetLiftingAllowableTensileConcreteStress(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetLiftingAllowableGlobalCompressiveConcreteStress(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetLiftingAllowableGlobalCompressionFactor() const = 0; // C*f'ci -> returns C
   virtual Float64 GetLiftingAllowablePeakCompressiveConcreteStress(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetLiftingAllowablePeakCompressionFactor() const = 0; // C*f'ci -> returns C
   virtual Float64 GetLiftingAllowableTensionFactor() const = 0; // T*Sqrt(f'ci) -> returns T
   virtual Float64 GetLiftingWithMildRebarAllowableStress(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetLiftingWithMildRebarAllowableStressFactor() const = 0;
   virtual void GetLiftingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax) const = 0;

   virtual Float64 GetLiftingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey,Float64 fci, bool includeRebar) const = 0;
   virtual Float64 GetLiftingAllowableGlobalCompressiveConcreteStressEx(const CSegmentKey& segmentKey, Float64 fci) const = 0;
   virtual Float64 GetLiftingAllowablePeakCompressiveConcreteStressEx(const CSegmentKey& segmentKey, Float64 fci) const = 0;

   // loop placement above top of girder
   virtual Float64 GetHeightOfPickPointAboveGirderTop() const = 0;

   // lateral tolerance for placement of lifting loop
   virtual Float64 GetLiftingLoopPlacementTolerance() const = 0;

   // minimum allowable inclination of lifting cable measured from horizontal
   virtual Float64 GetLiftingCableMinInclination() const = 0;

   // girder sweep tolerance
   virtual Float64 GetLiftingSweepTolerance() const = 0;

   virtual Float64 GetLiftingModulusOfRupture(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetLiftingModulusOfRupture(const CSegmentKey& segmentKey,Float64 fci,pgsTypes::ConcreteType concType) const = 0;
   virtual Float64 GetLiftingModulusOfRuptureFactor(pgsTypes::ConcreteType concType) const = 0;

   virtual Float64 GetMinimumLiftingPointLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const = 0;
   virtual Float64 GetLiftingPointLocationAccuracy() const = 0;

   // Factor to modify basic camber
   virtual Float64 GetLiftingCamberMultiplier() const = 0;

   virtual pgsTypes::WindType GetLiftingWindType() const = 0;
   virtual Float64 GetLiftingWindLoad() const = 0;

   virtual WBFL::Stability::LiftingCriteria GetLiftingStabilityCriteria(const CSegmentKey& segmentKey) const = 0;
   virtual WBFL::Stability::LiftingCriteria GetLiftingStabilityCriteria(const CSegmentKey& segmentKey,const HANDLINGCONFIG& liftConfig) const = 0;
};

/*****************************************************************************
INTERFACE
   ISegmentHaulingSpecCriteria

   Interface to  specification criteria for Hauling

DESCRIPTION
   Interface to  specification criteria for Hauling
*****************************************************************************/
// {DBEBE70C-E62F-11d2-AD3D-00105A9AF985}
DEFINE_GUID(IID_ISegmentHaulingSpecCriteria, 
0xdbebe70c, 0xe62f, 0x11d2, 0xad, 0x3d, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface ISegmentHaulingSpecCriteria : IUnknown
{
   // do we analyze hauling or not?
   virtual bool IsHaulingAnalysisEnabled() const = 0;

   virtual pgsTypes::HaulingAnalysisMethod GetHaulingAnalysisMethod() const = 0;
   
   // Impact
   virtual void GetHaulingImpact(Float64* pDownward, Float64* pUpward) const = 0;

   // Factors of safety
   virtual Float64 GetHaulingCrackingFs() const = 0;
   virtual Float64 GetHaulingRolloverFs() const = 0;

   // Allowable concrete stresses
   virtual Float64 GetHaulingAllowableTensileConcreteStress(const CSegmentKey& segmentKey, pgsTypes::HaulingSlope slope) const = 0;
   virtual Float64 GetHaulingAllowableGlobalCompressiveConcreteStress(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetHaulingAllowablePeakCompressiveConcreteStress(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetHaulingAllowableTensionFactor(pgsTypes::HaulingSlope slope) const = 0;
   virtual Float64 GetHaulingAllowableGlobalCompressionFactor() const = 0;
   virtual Float64 GetHaulingAllowablePeakCompressionFactor() const = 0;
   virtual Float64 GetHaulingWithMildRebarAllowableStress(const CSegmentKey& segmentKey, pgsTypes::HaulingSlope slope) const = 0;
   virtual Float64 GetHaulingWithMildRebarAllowableStressFactor(pgsTypes::HaulingSlope slope) const = 0;
   virtual void GetHaulingAllowableTensileConcreteStressParameters(pgsTypes::HaulingSlope slope,Float64* factor,bool* pbMax,Float64* fmax) const = 0;
   virtual Float64 GetHaulingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey,pgsTypes::HaulingSlope slope,Float64 fc, bool includeRebar) const = 0;
   virtual Float64 GetHaulingAllowableGlobalCompressiveConcreteStressEx(const CSegmentKey& segmentKey, Float64 fc) const = 0;
   virtual Float64 GetHaulingAllowablePeakCompressiveConcreteStressEx(const CSegmentKey& segmentKey, Float64 fc) const = 0;

   virtual Float64 GetHaulingModulusOfRupture(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetHaulingModulusOfRupture(const CSegmentKey& segmentKey,Float64 fci,pgsTypes::ConcreteType concType) const = 0;
   virtual Float64 GetHaulingModulusOfRuptureFactor(pgsTypes::ConcreteType concType) const = 0;

   // Truck parameters
   virtual Float64 GetRollStiffness(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetHeightOfGirderBottomAboveRoadway(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetHeightOfTruckRollCenterAboveRoadway(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetAxleWidth(const CSegmentKey& segmentKey) const = 0;

   virtual pgsTypes::HaulingImpact GetHaulingImpactUsage() const = 0;
   virtual Float64 GetNormalCrownSlope() const = 0;
   // Max expected roadway superelevation - slope
   virtual Float64 GetMaxSuperelevation() const = 0;

   // Girder sweep tolerance
   virtual Float64 GetHaulingSweepTolerance() const = 0;
   virtual Float64 GetHaulingSweepGrowth() const = 0;

   // Lateral tolerance for placement of girder onto supports
   virtual Float64 GetHaulingSupportPlacementTolerance() const = 0;

   // Factor to modify basic camber
   virtual Float64 GetHaulingCamberMultiplier() const = 0;

   virtual Float64 GetAllowableDistanceBetweenSupports(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetAllowableLeadingOverhang(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetMaxGirderWgt(const CSegmentKey& segmentKey) const = 0;

   virtual Float64 GetMinimumHaulingSupportLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const = 0;
   virtual Float64 GetHaulingSupportLocationAccuracy() const = 0;

   virtual pgsTypes::WindType GetHaulingWindType() const = 0;
   virtual Float64 GetHaulingWindLoad() const = 0;
   virtual pgsTypes::CFType GetCentrifugalForceType() const = 0;
   virtual Float64 GetHaulingSpeed() const = 0;
   virtual Float64 GetTurningRadius() const = 0;

   virtual WBFL::Stability::HaulingCriteria GetHaulingStabilityCriteria(const CSegmentKey& segmentKey) const = 0;
   virtual WBFL::Stability::HaulingCriteria GetHaulingStabilityCriteria(const CSegmentKey& segmentKey,const HANDLINGCONFIG& haulConfig) const = 0;
};

// {CA374433-127A-4850-AEC1-AB250D323724}
DEFINE_GUID(IID_IKdotGirderHaulingSpecCriteria, 
0xca374433, 0x127a, 0x4850, 0xae, 0xc1, 0xab, 0x25, 0xd, 0x32, 0x37, 0x24);
interface IKdotGirderHaulingSpecCriteria : IUnknown
{
   // Spec criteria for KDOT analyses
   // Allowable concrete stresses
   virtual Float64 GetKdotHaulingAllowableTensileConcreteStress(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetKdotHaulingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetKdotHaulingAllowableTensionFactor() const = 0;
   virtual Float64 GetKdotHaulingAllowableCompressionFactor() const = 0;
   virtual Float64 GetKdotHaulingWithMildRebarAllowableStress(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetKdotHaulingWithMildRebarAllowableStressFactor() const = 0;
   virtual void GetKdotHaulingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax) const = 0;
   virtual Float64 GetKdotHaulingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc, bool includeRebar) const = 0;
   virtual Float64 GetKdotHaulingAllowableCompressiveConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc) const = 0;

   virtual void GetMinimumHaulingSupportLocation(Float64* pHardDistance, bool* pUseFactoredLength, Float64* pLengthFactor) const = 0;
   virtual Float64 GetHaulingDesignLocationAccuracy() const = 0;

   virtual void GetHaulingGFactors(Float64* pOverhangFactor, Float64* pInteriorFactor) const = 0;
};

#endif // INCLUDED_IFACE_GIRDERHANDLINGSPECCRITERIA_H_

