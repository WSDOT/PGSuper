///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#ifndef INCLUDED_IFACE_PRESTRESS_H_
#define INCLUDED_IFACE_PRESTRESS_H_

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

#include <Details.h>

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class pgsPointOfInterest;
class matPsStrand;
class rptChapter;
interface IEAFDisplayUnits;

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   IPretensionForce

   Interface to prestress force and stress information

DESCRIPTION
   Interface to prestress force and stress information.  This is computed
   information and not the raw input data.
*****************************************************************************/
// {381E19E0-6E82-11d2-8EEB-006097DF3C68}
DEFINE_GUID(IID_IPretensionForce, 
0x381e19e0, 0x6e82, 0x11d2, 0x8e, 0xeb, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface IPretensionForce : IUnknown
{
   virtual Float64 GetPjackMax(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,StrandIndexType nStrands) = 0;
   virtual Float64 GetPjackMax(const CSegmentKey& segmentKey,const matPsStrand& strand,StrandIndexType nStrands) = 0;

   virtual Float64 GetXferLength(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetDevLength(const pgsPointOfInterest& poi,bool bDebonded) = 0;
   virtual STRANDDEVLENGTHDETAILS GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded) = 0;
   virtual STRANDDEVLENGTHDETAILS GetDevLengthDetails(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bDebonded) = 0;
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,const GDRCONFIG& config,StrandIndexType strandIdx,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64 fps,Float64 fpe) = 0;
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,const GDRCONFIG& config,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64 fps,Float64 fpe) = 0;

   virtual Float64 GetHoldDownForce(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetHoldDownForce(const CSegmentKey& segmentKey,const GDRCONFIG& config) = 0;

   virtual Float64 GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime) = 0;
   virtual Float64 GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime) = 0;

   virtual Float64 GetVertHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime) = 0;
   virtual Float64 GetVertHarpedStrandForce(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime) = 0;

   virtual Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime) = 0;
   virtual Float64 GetPrestressForce(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime) = 0;
   virtual Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime) = 0;
   virtual Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime) = 0;
   virtual Float64 GetEffectivePrestress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime) = 0;
   virtual Float64 GetEffectivePrestress(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime) = 0;

   virtual Float64 GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) = 0;
};


/*****************************************************************************
INTERFACE
   IPosttensionForce

   Interface to post-tension force and stress information

DESCRIPTION
   Interface to post-tension force and stress information.  This is computed
   information and not the raw input data.
*****************************************************************************/
// {4E2D92B7-73B1-4dcd-8450-A4D18ED9F2B4}
DEFINE_GUID(IID_IPosttensionForce, 
0x4e2d92b7, 0x73b1, 0x4dcd, 0x84, 0x50, 0xa4, 0xd1, 0x8e, 0xd9, 0xf2, 0xb4);
interface IPosttensionForce : IUnknown
{
   // Returns the maximum jacking force for a given number of strands
   virtual Float64 GetPjackMax(const CGirderKey& girderKey,StrandIndexType nStrands) = 0;
   virtual Float64 GetPjackMax(const CGirderKey& girderKey,const matPsStrand& strand,StrandIndexType nStrands) = 0;

   // returns the force in a tendon in a particular interval. (use ALL_DUCTS for all tendons)
   // if bIncludeMinLiveLoad is true, the force related to the elastic stress due to the minimum Service III live load is included in the tendon force.
   // if bIncludeMaxLiveLoad is true, the force related to the elastic stress due to the maximum Service III live load is included in the tendon force.
   // if both bIncludeMinLiveLoad and bIncludeMaxLiveLoad are true, the live load that maximizes the tendon force is used.
   virtual Float64 GetTendonForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time,DuctIndexType ductIdx,bool bIncludeMinLiveLoad=false,bool bIncludeMaxLiveLoad=false) = 0;

   // returns the effective stress in a tendon in a particular interval.
   // if bIncludeMinLiveLoad is true, the elastic stress due to the minimum Service III live load is included in the tendon stress.
   // if bIncludeMaxLiveLoad is true, the elastic stress due to the maximum Service III live load is included in the tendon stress.
   // if both bIncludeMinLiveLoad and bIncludeMaxLiveLoad are true, the live load stress that maximizes the tendon stress is used.
   virtual Float64 GetTendonStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time,DuctIndexType ductIdx,bool bIncludeMinLiveLoad=false,bool bIncludeMaxLiveLoad=false) = 0;
};

/*****************************************************************************
INTERFACE
   ILosses

   Interface to get losses.

DESCRIPTION
   Interface to get losses. The losses returned by this interface
   are the effective prestress losses. When gross section properties are 
   used, any elastic gain/losses are included.
*****************************************************************************/
// {03D91150-6DBB-11d2-8EE9-006097DF3C68}
DEFINE_GUID(IID_ILosses, 
0x3d91150, 0x6dbb, 0x11d2, 0x8e, 0xe9, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface ILosses : IUnknown
{
   // losses based on current input
   virtual Float64 GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) = 0;
   virtual const LOSSDETAILS* GetLossDetails(const pgsPointOfInterest& poi) = 0;

   // losses based on a girder configuration and slab offset
   virtual Float64 GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config) = 0;
   virtual const LOSSDETAILS* GetLossDetails(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;
   virtual void ClearDesignLosses() = 0;

   virtual void ReportLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) = 0;
   virtual void ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) = 0;

   virtual Float64 GetPrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime) = 0;
   virtual Float64 GetPrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime) = 0;

   virtual Float64 GetPrestressLossWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) = 0;

   virtual Float64 GetFrictionLoss(const pgsPointOfInterest& poi,DuctIndexType ductIdx) = 0;
   virtual Float64 GetAnchorSetZoneLength(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType) = 0;
   virtual Float64 GetAnchorSetLoss(const pgsPointOfInterest& poi,DuctIndexType ductIdx) = 0;
};

#endif // INCLUDED_IFACE_PRESTRESS_H_

