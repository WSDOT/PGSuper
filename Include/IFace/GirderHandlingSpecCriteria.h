///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

/*****************************************************************************
COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

// SYSTEM INCLUDES
//
#if !defined INCLUDED_WBFLTYPES_H_
#include <WbflTypes.h>
#endif

#if !defined INCLUDED_PGSUPERTYPES_H_
#include <PGSuperTypes.h>
#endif

#if !defined INCLUDED_PGSEXT_POINTOFINTEREST_H_
#include <PgsExt\PointOfInterest.h>
#endif

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
   IGirderLiftingSpecCriteria

   Interface to specification criteria for lifting

DESCRIPTION
   Interface to specification criteria for lifting
*****************************************************************************/
// {CA83DBFA-E62F-11d2-AD3D-00105A9AF985}
DEFINE_GUID(IID_IGirderLiftingSpecCriteria, 
0xca83dbfa, 0xe62f, 0x11d2, 0xad, 0x3d, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface IGirderLiftingSpecCriteria : IUnknown
{
   // do we analyze lifting or not?
   virtual bool IsLiftingCheckEnabled() const=0;

   // impact
   virtual void GetLiftingImpact(Float64* pDownward, Float64* pUpward) const=0;
   // factors of safety
   virtual Float64 GetLiftingCrackingFs() const=0;
   virtual Float64 GetLiftingFailureFs() const=0;
   // allowable concrete stresses
   virtual Float64 GetLiftingAllowableTensileConcreteStress(SpanIndexType span,GirderIndexType gdr)=0;
   virtual Float64 GetLiftingAllowableCompressiveConcreteStress(SpanIndexType span,GirderIndexType gdr)=0;
   virtual Float64 GetLiftingAllowableCompressionFactor() = 0; // C*f'ci -> returns C
   virtual Float64 GetLiftingAllowableTensionFactor() = 0; // T*Sqrt(f'ci) -> returns T
   virtual Float64 GetLiftingWithMildRebarAllowableStress(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual Float64 GetLiftingWithMildRebarAllowableStressFactor() = 0;
   virtual void GetLiftingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax) = 0;

   virtual Float64 GetLiftingAllowableTensileConcreteStressEx(Float64 fci, bool includeRebar)=0;
   virtual Float64 GetLiftingAllowableCompressiveConcreteStressEx(Float64 fci)=0;

   // loop placement above top of girder
   virtual Float64 GetHeightOfPickPointAboveGirderTop() const=0;

   // lateral tolerance for placement of lifting loop
   virtual Float64 GetLiftingLoopPlacementTolerance() const=0;

   // minimum allowable inclination of lifting cable measured from horizontal
   virtual Float64 GetLiftingCableMinInclination() const=0;

   // girder sweep tolerance
   virtual Float64 GetLiftingSweepTolerance()const=0;

   virtual Float64 GetLiftingModulusOfRupture(SpanIndexType span,GirderIndexType gdr)=0;
   virtual Float64 GetLiftingModulusOfRupture(Float64 fci)=0; // obsolete (only valid for NWC)
   virtual Float64 GetLiftingModulusOfRuptureCoefficient()=0; // obsolete (only valid for NWC)

   virtual Float64 GetMinimumLiftingPointLocation(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType end) const = 0;
   virtual Float64 GetLiftingPointLocationAccuracy() const = 0;
};

// {B0296081-AB1C-4587-A3EC-410645D0BCD2}
DEFINE_GUID(IID_IGirderLiftingSpecCriteriaEx, 
0xb0296081, 0xab1c, 0x4587, 0xa3, 0xec, 0x41, 0x6, 0x45, 0xd0, 0xbc, 0xd2);
interface IGirderLiftingSpecCriteriaEx : IGirderLiftingSpecCriteria
{
   virtual Float64 GetLiftingModulusOfRuptureCoefficient(pgsTypes::ConcreteType concType) = 0;
   virtual Float64 GetLiftingModulusOfRupture(Float64 fci,pgsTypes::ConcreteType concType) = 0;
};

/*****************************************************************************
INTERFACE
   IGirderHaulingSpecCriteria

   Interface to  specification criteria for Hauling

DESCRIPTION
   Interface to  specification criteria for Hauling
*****************************************************************************/
// {DBEBE70C-E62F-11d2-AD3D-00105A9AF985}
DEFINE_GUID(IID_IGirderHaulingSpecCriteria, 
0xdbebe70c, 0xe62f, 0x11d2, 0xad, 0x3d, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface IGirderHaulingSpecCriteria : IUnknown
{
   enum RollStiffnessMethod { LumpSum, PerAxle };

   // do we analyze hauling or not?
   virtual bool IsHaulingCheckEnabled() const=0;

   virtual pgsTypes::HaulingAnalysisMethod GetHaulingAnalysisMethod() const=0;
   
   // Impact
   virtual void GetHaulingImpact(Float64* pDownward, Float64* pUpward) const=0;

   // Factors of safety
   virtual Float64 GetHaulingCrackingFs() const=0;
   virtual Float64 GetHaulingRolloverFs() const=0;

   // Allowable concrete stresses
   virtual Float64 GetHaulingAllowableTensileConcreteStress(SpanIndexType span,GirderIndexType gdr)=0;
   virtual Float64 GetHaulingAllowableCompressiveConcreteStress(SpanIndexType span,GirderIndexType gdr)=0;
   virtual Float64 GetHaulingAllowableTensionFactor()=0;
   virtual Float64 GetHaulingAllowableCompressionFactor()=0;
   virtual Float64 GetHaulingWithMildRebarAllowableStress(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual Float64 GetHaulingWithMildRebarAllowableStressFactor() = 0;
   virtual void GetHaulingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax) = 0;
   virtual Float64 GetHaulingAllowableTensileConcreteStressEx(Float64 fc, bool includeRebar)=0;
   virtual Float64 GetHaulingAllowableCompressiveConcreteStressEx(Float64 fc)=0;

   virtual Float64 GetHaulingModulusOfRupture(SpanIndexType span,GirderIndexType gdr)=0;
   virtual Float64 GetHaulingModulusOfRupture(Float64 fc)=0; // obsolete (only valid for NWC)
   virtual Float64 GetHaulingModulusOfRuptureCoefficient()=0; // obsolete (only valud for NWC)

   // Truck parameters
   virtual RollStiffnessMethod GetRollStiffnessMethod() const = 0;
   virtual Float64 GetLumpSumRollStiffness() const =0;
   virtual Float64 GetAxleWeightLimit() const = 0;
   virtual Float64 GetAxleStiffness() const = 0;
   virtual Float64 GetMinimumRollStiffness() const = 0;
   virtual Float64 GetHeightOfGirderBottomAboveRoadway() const =0;
   virtual Float64 GetHeightOfTruckRollCenterAboveRoadway() const =0;
   virtual Float64 GetAxleWidth() const =0;

   // Max expected roadway superelevation - slope
   virtual Float64 GetMaxSuperelevation() const =0;

   // Girder sweep tolerance
   virtual Float64 GetHaulingSweepTolerance()const=0;

   // Lateral tolerance for placement of girder onto supports
   virtual Float64 GetHaulingSupportPlacementTolerance() const=0;

   // Factor to use to raise height of CG of beam above truck roll center by
   // to account for camber. (returns 1.02 for a 2% increase)
   virtual Float64 GetIncreaseInCgForCamber() const =0;

   // governed by max length of truck trailer
   virtual Float64 GetAllowableDistanceBetweenSupports() const =0;
   virtual Float64 GetAllowableLeadingOverhang() const = 0;

   virtual Float64 GetMaxGirderWgt() const = 0;

   virtual Float64 GetMinimumHaulingSupportLocation(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType end) const = 0;
   virtual Float64 GetHaulingSupportLocationAccuracy() const = 0;
};

// {6CA4EE87-969A-4c9f-BA2F-A64F3FFEF124}
DEFINE_GUID(IID_IGirderHaulingSpecCriteriaEx, 
0x6ca4ee87, 0x969a, 0x4c9f, 0xba, 0x2f, 0xa6, 0x4f, 0x3f, 0xfe, 0xf1, 0x24);
interface IGirderHaulingSpecCriteriaEx : IGirderHaulingSpecCriteria
{
   virtual Float64 GetHaulingModulusOfRuptureCoefficient(pgsTypes::ConcreteType concType) = 0;
   virtual Float64 GetHaulingModulusOfRupture(Float64 fci,pgsTypes::ConcreteType concType) = 0;
};

// {CA374433-127A-4850-AEC1-AB250D323724}
DEFINE_GUID(IID_IKdotGirderHaulingSpecCriteria, 
0xca374433, 0x127a, 0x4850, 0xae, 0xc1, 0xab, 0x25, 0xd, 0x32, 0x37, 0x24);
interface IKdotGirderHaulingSpecCriteria : IUnknown
{
   // Spec criteria for KDOT analyses
   // Allowable concrete stresses
   virtual Float64 GetKdotHaulingAllowableTensileConcreteStress(SpanIndexType span,GirderIndexType gdr)=0;
   virtual Float64 GetKdotHaulingAllowableCompressiveConcreteStress(SpanIndexType span,GirderIndexType gdr)=0;
   virtual Float64 GetKdotHaulingAllowableTensionFactor()=0;
   virtual Float64 GetKdotHaulingAllowableCompressionFactor()=0;
   virtual Float64 GetKdotHaulingWithMildRebarAllowableStress(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual Float64 GetKdotHaulingWithMildRebarAllowableStressFactor() = 0;
   virtual void GetKdotHaulingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax) = 0;
   virtual Float64 GetKdotHaulingAllowableTensileConcreteStressEx(Float64 fc, bool includeRebar)=0;
   virtual Float64 GetKdotHaulingAllowableCompressiveConcreteStressEx(Float64 fc)=0;

   virtual void GetMinimumHaulingSupportLocation(Float64* pHardDistance, bool* pUseFactoredLength, Float64* pLengthFactor) const = 0;
   virtual Float64 GetHaulingDesignLocationAccuracy() const = 0;

   virtual void GetHaulingGFactors(Float64* pOverhangFactor, Float64* pInteriorFactor) const = 0;
};


#endif // INCLUDED_IFACE_GIRDERHANDLINGSPECCRITERIA_H_

