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

// SpecAgentImp.cpp : Implementation of CSpecAgentImp
#include "stdafx.h"
#include "SpecAgent.h"
#include "SpecAgent_i.h"
#include "SpecAgentImp.h"

#include <PgsExt\BridgeDescription2.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <Lrfd\PsStrand.h>
#include <Lrfd\Rebar.h>

#include <IFace\StatusCenter.h>
#include <IFace\PrestressForce.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>

#include <Units\SysUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DECLARE_LOGFILE;

//#define MATCH_OLD_ANALYSIS // set to true to match the old stability analysis with the new stability code

/////////////////////////////////////////////////////////////////////////////
// CSpecAgentImp


/////////////////////////////////////////////////////////////////////////////
// IAgent
//
STDMETHODIMP CSpecAgentImp::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CSpecAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   pBrokerInit->RegInterface( IID_IAllowableStrandStress,         this );
   pBrokerInit->RegInterface( IID_IAllowableTendonStress,         this );
   pBrokerInit->RegInterface( IID_IAllowableConcreteStress,       this );
   pBrokerInit->RegInterface( IID_ITransverseReinforcementSpec,   this );
   pBrokerInit->RegInterface( IID_IPrecastIGirderDetailsSpec,     this );
   pBrokerInit->RegInterface( IID_ISegmentLiftingSpecCriteria,    this );
   pBrokerInit->RegInterface( IID_ISegmentHaulingSpecCriteria,    this );
   pBrokerInit->RegInterface( IID_IKdotGirderHaulingSpecCriteria, this );
   pBrokerInit->RegInterface( IID_IDebondLimits,                  this );
   pBrokerInit->RegInterface( IID_IResistanceFactors,             this );
   pBrokerInit->RegInterface( IID_IInterfaceShearRequirements,    this );
   pBrokerInit->RegInterface( IID_IDuctLimits,                    this );

   return S_OK;
}

STDMETHODIMP CSpecAgentImp::Init()
{
   CREATE_LOGFILE("SpecAgent");
   EAF_AGENT_INIT;
   return S_OK;
}

STDMETHODIMP CSpecAgentImp::Init2()
{
   return S_OK;
}

STDMETHODIMP CSpecAgentImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_SpecAgent;
   return S_OK;
}

STDMETHODIMP CSpecAgentImp::Reset()
{
   return S_OK;
}

STDMETHODIMP CSpecAgentImp::ShutDown()
{
   EAF_AGENT_CLEAR_INTERFACE_CACHE;
   CLOSE_LOGFILE;
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IAllowableStrandStress
//
bool CSpecAgentImp::CheckStressAtJacking()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckStrandStress(CSS_AT_JACKING);
}

bool CSpecAgentImp::CheckStressBeforeXfer()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckStrandStress(CSS_BEFORE_TRANSFER);
}

bool CSpecAgentImp::CheckStressAfterXfer()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckStrandStress(CSS_AFTER_TRANSFER);
}

bool CSpecAgentImp::CheckStressAfterLosses()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckStrandStress(CSS_AFTER_ALL_LOSSES);
}

Float64 CSpecAgentImp::GetAllowableAtJacking(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType)
{
   if ( !CheckStressAtJacking() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,strandType);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetStrandStressCoefficient(CSS_AT_JACKING,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetAllowableBeforeXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType)
{
   if ( !CheckStressBeforeXfer() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,strandType);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetStrandStressCoefficient(CSS_BEFORE_TRANSFER,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetAllowableAfterXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType)
{
   if ( !CheckStressAfterXfer() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,strandType);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetStrandStressCoefficient(CSS_AFTER_TRANSFER,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetAllowableAfterLosses(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType)
{
   if ( !CheckStressAfterLosses() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,strandType);

   Float64 fpy = lrfdPsStrand::GetYieldStrength(pStrand->GetGrade(),pStrand->GetType());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetStrandStressCoefficient(CSS_AFTER_ALL_LOSSES,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpy;
}

/////////////////////////////////////////////////////////
// IAllowableTendonStress
//
bool CSpecAgentImp::CheckTendonStressAtJacking()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckTendonStressAtJacking();
}

bool CSpecAgentImp::CheckTendonStressPriorToSeating()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckTendonStressPriorToSeating();
}

Float64 CSpecAgentImp::GetAllowableAtJacking(const CGirderKey& girderKey)
{
   if ( !CheckTendonStressAtJacking() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetTendonMaterial(girderKey);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   Float64 coeff = GetAllowableCoefficientAtJacking(girderKey);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetAllowablePriorToSeating(const CGirderKey& girderKey)
{
   if ( !CheckTendonStressPriorToSeating() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetTendonMaterial(girderKey);

   Float64 fpy = lrfdPsStrand::GetYieldStrength(pStrand->GetGrade(),pStrand->GetType());

   Float64 coeff = GetAllowableCoefficientPriorToSeating(girderKey);

   return coeff*fpy;
}

Float64 CSpecAgentImp::GetAllowableAfterAnchorSetAtAnchorage(const CGirderKey& girderKey)
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetTendonMaterial(girderKey);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   Float64 coeff = GetAllowableCoefficientAfterAnchorSetAtAnchorage(girderKey);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetAllowableAfterAnchorSet(const CGirderKey& girderKey)
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetTendonMaterial(girderKey);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   Float64 coeff = GetAllowableCoefficientAfterAnchorSet(girderKey);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetAllowableAfterLosses(const CGirderKey& girderKey)
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetTendonMaterial(girderKey);

   Float64 fpy = lrfdPsStrand::GetYieldStrength(pStrand->GetGrade(),pStrand->GetType());

   Float64 coeff = GetAllowableCoefficientAfterLosses(girderKey);

   return coeff*fpy;
}

Float64 CSpecAgentImp::GetAllowableCoefficientAtJacking(const CGirderKey& girderKey)
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetTendonMaterial(girderKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_AT_JACKING,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);
   return coeff;
}

Float64 CSpecAgentImp::GetAllowableCoefficientPriorToSeating(const CGirderKey& girderKey)
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetTendonMaterial(girderKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_PRIOR_TO_SEATING,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);
   return coeff;
}

Float64 CSpecAgentImp::GetAllowableCoefficientAfterAnchorSetAtAnchorage(const CGirderKey& girderKey)
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetTendonMaterial(girderKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_ANCHORAGES_AFTER_SEATING,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);
   return coeff;
}

Float64 CSpecAgentImp::GetAllowableCoefficientAfterAnchorSet(const CGirderKey& girderKey)
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetTendonMaterial(girderKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_ELSEWHERE_AFTER_SEATING,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);
   return coeff;
}

Float64 CSpecAgentImp::GetAllowableCoefficientAfterLosses(const CGirderKey& girderKey)
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetTendonMaterial(girderKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_AFTER_ALL_LOSSES,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff;
}

/////////////////////////////////////////////////////////////////////////////
// IAllowableConcreteStress
//
Float64 CSpecAgentImp::GetAllowableCompressionStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   if ( IsGirderStressLocation(stressLocation) )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         return GetClosureJointAllowableCompressionStress(poi,intervalIdx,ls);
      }
      else
      {
         return GetSegmentAllowableCompressionStress(poi,intervalIdx,ls);
      }
   }
   else
   {
      ATLASSERT(IsDeckStressLocation(stressLocation));
      return GetDeckAllowableCompressionStress(poi,intervalIdx,ls);
   }
}

Float64 CSpecAgentImp::GetAllowableTensionStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone)
{
   if ( IsGirderStressLocation(stressLocation) )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         return GetClosureJointAllowableTensionStress(poi,intervalIdx,ls,bWithBondedReinforcement,bInPrecompressedTensileZone);
      }
      else
      {
         return GetSegmentAllowableTensionStress(poi,intervalIdx,ls,bWithBondedReinforcement);
      }
   }
   else
   {
      ATLASSERT(IsDeckStressLocation(stressLocation));
      return GetDeckAllowableTensionStress(poi,intervalIdx,ls,bWithBondedReinforcement);
   }
}

Float64 CSpecAgentImp::GetAllowableTensionStress(pgsTypes::LoadRatingType ratingType,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType ratingIntervalIdx = pIntervals->GetLoadRatingInterval();

   Float64 fc;
   Float64 lambda;
   GET_IFACE(IMaterials,pMaterials);
   if ( IsGirderStressLocation(stressLocation) )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         fc = pMaterials->GetClosureJointDesignFc(closureKey,ratingIntervalIdx);
         lambda = pMaterials->GetClosureJointLambda(closureKey);
      }
      else
      {
         fc = pMaterials->GetSegmentDesignFc(segmentKey,ratingIntervalIdx);
         lambda = pMaterials->GetSegmentLambda(segmentKey);
      }
   }
   else
   {
      ATLASSERT(IsDeckStressLocation(stressLocation));
      fc = pMaterials->GetDeckDesignFc(ratingIntervalIdx);
      lambda = pMaterials->GetDeckLambda();
   }

   GET_IFACE(IRatingSpecification,pRatingSpec);
   Float64 x = pRatingSpec->GetAllowableTensionCoefficient(ratingType);

   Float64 fallow = x*lambda*sqrt(fc);
   return fallow;
}

Float64 CSpecAgentImp::GetAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   if ( IsGirderStressLocation(stressLocation) )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         return GetClosureJointAllowableCompressionStressCoefficient(poi,intervalIdx,ls);
      }
      else
      {
         return GetSegmentAllowableCompressionStressCoefficient(poi,intervalIdx,ls);
      }
   }
   else
   {
      ATLASSERT(IsDeckStressLocation(stressLocation));
      return GetDeckAllowableCompressionStressCoefficient(poi,intervalIdx,ls);
   }
}

void CSpecAgentImp::GetAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone,Float64* pCoeff,bool* pbMax,Float64* pMaxValue)
{
   if ( IsGirderStressLocation(stressLocation) )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         GetClosureJointAllowableTensionStressCoefficient(poi,intervalIdx,ls,bWithBondedReinforcement,bInPrecompressedTensileZone,pCoeff,pbMax,pMaxValue);
      }
      else
      {
         GetSegmentAllowableTensionStressCoefficient(poi,intervalIdx,ls,bWithBondedReinforcement,pCoeff,pbMax,pMaxValue);
      }
   }
   else
   {
      ATLASSERT(IsDeckStressLocation(stressLocation));
      GetDeckAllowableTensionStressCoefficient(poi,intervalIdx,ls,bWithBondedReinforcement,pCoeff,pbMax,pMaxValue);
   }
}

Float64 CSpecAgentImp::GetSegmentAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,intervalIdx,ls,pgsTypes::Compression));

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetSegmentDesignFc(segmentKey,intervalIdx);

   Float64 fAllow = GetSegmentAllowableCompressionStress(poi,intervalIdx,ls,fc);
   return fAllow;
}

Float64 CSpecAgentImp::GetClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   GET_IFACE(IPointOfInterest,pPoi);
   CClosureKey closureKey;
   VERIFY(pPoi->IsInClosureJoint(poi,&closureKey));

   ATLASSERT(IsStressCheckApplicable(closureKey,intervalIdx,ls,pgsTypes::Compression));

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetClosureJointDesignFc(closureKey,intervalIdx);

   Float64 fAllow = GetClosureJointAllowableCompressionStress(poi,intervalIdx,ls,fc);
   return fAllow;
}

Float64 CSpecAgentImp::GetDeckAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,intervalIdx,ls,pgsTypes::Compression));

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetDeckDesignFc(intervalIdx);

   Float64 fAllow = GetDeckAllowableCompressionStress(poi,intervalIdx,ls,fc);
   return fAllow;
}

Float64 CSpecAgentImp::GetSegmentAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,intervalIdx,ls,pgsTypes::Tension));

   if ( IsLoadRatingServiceIIILimitState(ls) )
   {
#if defined _DEBUG
      // allowable stresses during load rating only make sense if live load is applied
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      ATLASSERT(liveLoadIntervalIdx <= intervalIdx );
#endif
      pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(ls);
      return GetAllowableTensionStress(ratingType,poi,pgsTypes::BottomGirder);
   }

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetSegmentDesignFc(segmentKey,intervalIdx);

   Float64 fAllow = GetSegmentAllowableTensionStress(poi,intervalIdx,ls,fc,bWithBondedReinforcement);
   return fAllow;
}

Float64 CSpecAgentImp::GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,intervalIdx,ls,pgsTypes::Tension));

   if ( IsLoadRatingServiceIIILimitState(ls) )
   {
#if defined _DEBUG
      // allowable stresses during load rating only make sense if live load is applied
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      ATLASSERT(liveLoadIntervalIdx <= intervalIdx );
#endif
      pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(ls);
      return GetAllowableTensionStress(ratingType,poi,pgsTypes::BottomGirder);
   }

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetClosureJointDesignFc(segmentKey,intervalIdx);

   Float64 fAllow = GetClosureJointAllowableTensionStress(poi,intervalIdx,ls,fc,bWithBondedReinforcement,bInPrecompressedTensileZone);
   return fAllow;
}

Float64 CSpecAgentImp::GetDeckAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,intervalIdx,ls,pgsTypes::Tension));

   if ( IsLoadRatingServiceIIILimitState(ls) )
   {
#if defined _DEBUG
      // allowable stresses during load rating only make sense if live load is applied
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      ATLASSERT(liveLoadIntervalIdx <= intervalIdx );
#endif
      pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(ls);
      return GetAllowableTensionStress(ratingType,poi,pgsTypes::TopDeck);
   }

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetDeckDesignFc(intervalIdx);

   Float64 fAllow = GetDeckAllowableTensionStress(poi,intervalIdx,ls,fc,bWithBondedReinforcement);
   return fAllow;
}

std::vector<Float64> CSpecAgentImp::GetGirderAllowableCompressionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   ATLASSERT(IsStressCheckApplicable(vPoi.front().GetSegmentKey(),intervalIdx,ls,pgsTypes::Compression));

   GET_IFACE(IPointOfInterest,pPoi);

   std::vector<Float64> vStress;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         vStress.push_back( GetClosureJointAllowableCompressionStress(poi,intervalIdx,ls));
      }
      else
      {
         vStress.push_back( GetSegmentAllowableCompressionStress(poi,intervalIdx,ls));
      }
   }

   return vStress;
}

std::vector<Float64> CSpecAgentImp::GetDeckAllowableCompressionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   ATLASSERT(IsStressCheckApplicable(vPoi.front().GetSegmentKey(),intervalIdx,ls,pgsTypes::Compression));

   std::vector<Float64> vStress;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi(*iter);
      vStress.push_back( GetDeckAllowableCompressionStress(poi,intervalIdx,ls));
   }

   return vStress;
}

std::vector<Float64> CSpecAgentImp::GetGirderAllowableTensionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondededReinforcement,bool bInPrecompressedTensileZone)
{
   ATLASSERT(IsStressCheckApplicable(vPoi.front().GetSegmentKey(),intervalIdx,ls,pgsTypes::Tension));

   GET_IFACE(IPointOfInterest,pPoi);

   std::vector<Float64> vStress;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         vStress.push_back( GetClosureJointAllowableTensionStress(poi,intervalIdx,ls,bWithBondededReinforcement,bInPrecompressedTensileZone));
      }
      else
      {
         vStress.push_back( GetSegmentAllowableTensionStress(poi,intervalIdx,ls,bWithBondededReinforcement));
      }
   }

   return vStress;
}

std::vector<Float64> CSpecAgentImp::GetDeckAllowableTensionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondededReinforcement)
{
   ATLASSERT(IsStressCheckApplicable(vPoi.front().GetSegmentKey(),intervalIdx,ls,pgsTypes::Tension));

   std::vector<Float64> vStress;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      vStress.push_back( GetDeckAllowableTensionStress(poi,intervalIdx,ls,bWithBondededReinforcement));
   }

   return vStress;
}

Float64 CSpecAgentImp::GetSegmentAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc)
{
   Float64 x = GetSegmentAllowableCompressionStressCoefficient(poi,intervalIdx,ls);

   // Add a minus sign because compression is negative
   return -x*fc;
}

Float64 CSpecAgentImp::GetClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc)
{
   Float64 x = GetClosureJointAllowableCompressionStressCoefficient(poi,intervalIdx,ls);

   GET_IFACE(IPointOfInterest,pPoi);
   CClosureKey closureKey;
   VERIFY( pPoi->IsInClosureJoint(poi,&closureKey) );

   // Add a minus sign because compression is negative
   return -x*fc;
}

Float64 CSpecAgentImp::GetDeckAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc)
{
   Float64 x = GetDeckAllowableCompressionStressCoefficient(poi,intervalIdx,ls);

   // Add a minus sign because compression is negative
   return -x*fc;
}

Float64 CSpecAgentImp::GetSegmentAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc,bool bWithBondedReinforcement)
{
   Float64 x;
   bool bCheckMax;
   Float64 fmax; // In system units
   GetSegmentAllowableTensionStressCoefficient(poi,intervalIdx,ls,bWithBondedReinforcement,&x,&bCheckMax,&fmax);

   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetSegmentLambda(poi.GetSegmentKey());

   Float64 f = x * lambda * sqrt( fc );
   if ( bCheckMax )
   {
      f = Min(f,fmax);
   }

   return f;
}

Float64 CSpecAgentImp::GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone)
{
   Float64 x;
   bool bCheckMax;
   Float64 fmax; // In system units
   GetClosureJointAllowableTensionStressCoefficient(poi,intervalIdx,ls,bWithBondedReinforcement,bInPrecompressedTensileZone,&x,&bCheckMax,&fmax);

   GET_IFACE(IPointOfInterest,pPoi);
   CClosureKey closureKey;
   VERIFY( pPoi->IsInClosureJoint(poi,&closureKey) );

   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetClosureJointLambda(closureKey);

   Float64 f = x * lambda * sqrt( fc );
   if ( bCheckMax )
   {
      f = Min(f,fmax);
   }

   return f;
}

Float64 CSpecAgentImp::GetDeckAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc,bool bWithBondedReinforcement)
{
   Float64 x;
   bool bCheckMax;
   Float64 fmax; // In system units
   GetDeckAllowableTensionStressCoefficient(poi,intervalIdx,ls,bWithBondedReinforcement,&x,&bCheckMax,&fmax);

   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetDeckLambda();

   Float64 f = x * lambda * sqrt( fc );
   if ( bCheckMax )
   {
      f = Min(f,fmax);
   }

   return f;
}

Float64 CSpecAgentImp::GetLiftingWithMildRebarAllowableStressFactor()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = pSpec->GetLiftingTensionStressFactorWithRebar();
   return x;
}

Float64 CSpecAgentImp::GetLiftingWithMildRebarAllowableStress(const CSegmentKey& segmentKey)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fci = pMaterial->GetSegmentDesignFc(segmentKey,liftSegmentIntervalIdx);
   Float64 lambda = pMaterial->GetSegmentLambda(segmentKey);

   Float64 x = GetLiftingWithMildRebarAllowableStressFactor();

   return x*lambda*sqrt(fci);
}

Float64 CSpecAgentImp::GetHaulingWithMildRebarAllowableStressFactorNormalCrown()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = pSpec->GetHaulingTensionStressFactorWithRebarNormalCrown();
   return x;
}

Float64 CSpecAgentImp::GetHaulingWithMildRebarAllowableStressNormalCrown(const CSegmentKey& segmentKey)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fc = pMaterial->GetSegmentDesignFc(segmentKey,haulSegmentIntervalIdx);
   Float64 lambda = pMaterial->GetSegmentLambda(segmentKey);

   Float64 x = GetHaulingWithMildRebarAllowableStressFactorNormalCrown();

   return x*lambda*sqrt(fc);
}

Float64 CSpecAgentImp::GetHaulingWithMildRebarAllowableStressFactorMaxSuper(const CSegmentKey& segmentKey)
{
#if defined MATCH_OLD_ANALYSIS
   GET_IFACE(IMaterials,pMaterial);
   pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
   Float64 x = GetHaulingModulusOfRuptureFactor(concType);
#else
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = pSpec->GetHaulingTensionStressFactorWithRebarMaxSuper();
#endif
   return x;
}

Float64 CSpecAgentImp::GetHaulingWithMildRebarAllowableStressMaxSuper(const CSegmentKey& segmentKey)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fc = pMaterial->GetSegmentDesignFc(segmentKey,haulSegmentIntervalIdx);
   Float64 lambda = pMaterial->GetSegmentLambda(segmentKey);

   Float64 x = GetHaulingWithMildRebarAllowableStressFactorMaxSuper(segmentKey);

   return x*lambda*sqrt(fc);
}

Float64 CSpecAgentImp::GetHaulingModulusOfRupture(const CSegmentKey& segmentKey)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetSegmentDesignFc(segmentKey,intervalIdx);
   pgsTypes::ConcreteType type = pMaterials->GetSegmentConcreteType(segmentKey);

   return GetHaulingModulusOfRupture(segmentKey,fc,type);
}

Float64 CSpecAgentImp::GetHaulingModulusOfRupture(const CSegmentKey& segmentKey,Float64 fc,pgsTypes::ConcreteType concType)
{
   Float64 x = GetHaulingModulusOfRuptureFactor(concType);

   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

   return x*lambda*sqrt(fc);
}

Float64 CSpecAgentImp::GetHaulingModulusOfRuptureFactor(pgsTypes::ConcreteType concType)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingModulusOfRuptureFactor(concType);
}

Float64 CSpecAgentImp::GetSegmentAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,intervalIdx,ls,pgsTypes::Compression));

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liftIntervalIdx          = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType haulIntervalIdx          = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType erectSegmentIdx          = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tempStrandRemovalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   // first the special cases
   if ( intervalIdx == liftIntervalIdx )
   {
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetLiftingCompressionStressFactor();
   }
   else if ( storageIntervalIdx <= intervalIdx && intervalIdx < haulIntervalIdx )
   {
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetAtReleaseCompressionStressFactor();
   }
   else if ( intervalIdx == haulIntervalIdx )
   {
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetHaulingCompressionStressFactor();
   }
   else if ( intervalIdx == tempStrandRemovalIdx )
   {
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetTempStrandRemovalCompressionStressFactor();
   }
   else
   {
      // now for the normal cases
      bool bIsStressingInterval = pIntervals->IsStressingInterval(segmentKey,intervalIdx);

      if ( bIsStressingInterval )
      {
         // stressing interval
         ATLASSERT( ls == pgsTypes::ServiceI );
         x = pSpec->GetAtReleaseCompressionStressFactor();
      }
      else
      {
         // non-stressing interval
         if ( intervalIdx < liveLoadIntervalIdx )
         {
            if ( intervalIdx < railingSystemIntervalIdx )
            {
               // before the deck is composite (this is for temporary loading conditions)
               // this is basically the wet slab on girder case
               ATLASSERT( ls == pgsTypes::ServiceI );
               x = pSpec->GetErectionCompressionStressFactor();
            }
            else
            {
               // the deck is now composite so this is the Effective Prestress + Permanent Loads case
               // (basically the case when the railing system has been installed, but no live load)
               ATLASSERT( ls == pgsTypes::ServiceI );
               x = pSpec->GetFinalWithoutLiveLoadCompressionStressFactor();
            }
         }
         else
         {
            // live load is on the structure so this is the "at service limit states" "after all losses"
            // case
            ATLASSERT( (ls == pgsTypes::ServiceI) || (ls == pgsTypes::ServiceIA) || (ls == pgsTypes::FatigueI));
            x = (ls == pgsTypes::ServiceI ? pSpec->GetFinalWithLiveLoadCompressionStressFactor() : pSpec->GetFatigueCompressionStressFactor());
         }
      }
   }

   return x;
}

Float64 CSpecAgentImp::GetClosureJointAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,intervalIdx,ls,pgsTypes::Compression));

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   bool bIsTendonStressingInterval = pIntervals->IsTendonStressingInterval(segmentKey,intervalIdx);

   if ( bIsTendonStressingInterval )
   {
      // stressing interval
      x = pSpec->GetAtStressingCompressingStressFactor();
   }
   else
   {
      // non-stressing interval
      if ( intervalIdx < liveLoadIntervalIdx )
      {
         // Effective Prestress + Permanent Loads
         x = pSpec->GetAtServiceCompressingStressFactor();
      }
      else
      {
         // Effective Prestress + Permanent Loads + Transient Loads
         if (ls == pgsTypes::ServiceIA || ls == pgsTypes::FatigueI )
         {
            x = pSpec->GetClosureFatigueCompressionStressFactor();
         }
         else
         {
            x = pSpec->GetAtServiceWithLiveLoadCompressingStressFactor();
         }
      }
   }

   return x;
}

Float64 CSpecAgentImp::GetDeckAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,intervalIdx,ls,pgsTypes::Compression));

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   bool bIsTendonStressingInterval = pIntervals->IsTendonStressingInterval(segmentKey,intervalIdx);

   ATLASSERT(compositeDeckIntervalIdx <= intervalIdx); // why are you asking for allowable deck stresses before the deck can take load?

   if ( bIsTendonStressingInterval )
   {
      // stressing interval
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetAtReleaseCompressionStressFactor();
   }
   else
   {
      // non-stressing interval
      if ( intervalIdx < liveLoadIntervalIdx )
      {
         // before the deck is composite (this is for temporary loading conditions)
         // this is basically the wet slab on girder case
         if ( intervalIdx < compositeDeckIntervalIdx )
         {
            ATLASSERT( ls == pgsTypes::ServiceI );
            x = pSpec->GetErectionCompressionStressFactor();
         }
         else
         {
            // the deck is now composite so this is the Effective Prestress + Permanent Loads case
            // (basically the case when the railing system has been installed, but no live load)
            ATLASSERT( ls == pgsTypes::ServiceI );
            x = pSpec->GetFinalWithoutLiveLoadCompressionStressFactor();
         }
      }
      else
      {
         // live load is on the structure so this is the "at service limit states" "after all losses"
         // case
         ATLASSERT( (ls == pgsTypes::ServiceI) || (ls == pgsTypes::ServiceIA) || (ls == pgsTypes::FatigueI));
         x = (ls == pgsTypes::ServiceI ? pSpec->GetFinalWithLiveLoadCompressionStressFactor() : pSpec->GetFatigueCompressionStressFactor());
      }
   }

   return x;
}

void CSpecAgentImp::GetSegmentAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,Float64* pCoeff,bool* pbMax,Float64* pMaxValue)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;
   bool bCheckMax = false;
   Float64 fmax = -99999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,intervalIdx,ls,pgsTypes::Tension));


   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liftingIntervalIdx       = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType haulingIntervalIdx       = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType erectSegmentIdx          = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tempStrandRemovalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   bool bIsStressingInterval = pIntervals->IsStressingInterval(segmentKey,intervalIdx);

   // first deal with the special cases
   if ( intervalIdx == liftingIntervalIdx )
   {
      ATLASSERT( ls == pgsTypes::ServiceI );
      if ( bWithBondedReinforcement )
      {
         x = pSpec->GetLiftingTensionStressFactorWithRebar();
      }
      else
      {
         x = pSpec->GetLiftingTensionStressFactor();
         pSpec->GetLiftingMaximumTensionStress(&bCheckMax,&fmax);
      }
   }
   else if ( storageIntervalIdx <= intervalIdx && intervalIdx < haulingIntervalIdx )
   {
      ATLASSERT( ls == pgsTypes::ServiceI );
      if ( bWithBondedReinforcement )
      {
         x = pSpec->GetAtReleaseTensionStressFactorWithRebar();
      }
      else
      {
         x = pSpec->GetAtReleaseTensionStressFactor();
         pSpec->GetAtReleaseMaximumTensionStress(&bCheckMax,&fmax);
      }
   }
   else if ( intervalIdx == haulingIntervalIdx )
   {
      ATLASSERT(false); // can't use this method for hauling because we don't know
      // if the caller wants the allowable tension factor for normal crown or max super

      //ATLASSERT( ls == pgsTypes::ServiceI );
      //if ( bWithBondedReinforcement )
      //{
      //   x = pSpec->GetHaulingTensionStressFactorWithRebarNormalCrown();
      //}
      //else
      //{
      //   x = pSpec->GetHaulingTensionStressFactorNormalCrown();
      //   pSpec->GetHaulingMaximumTensionStressNormalCrown(&bCheckMax,&fmax);
      //}
   } 
   else if ( intervalIdx == tempStrandRemovalIdx )
   {
      ATLASSERT( ls == pgsTypes::ServiceI );
      ATLASSERT( CheckTemporaryStresses() ); // if this fires, why are you asking for this if they aren't being used?
      if ( bWithBondedReinforcement )
      {
         x = pSpec->GetTempStrandRemovalTensionStressFactorWithRebar();
      }
      else
      {
         x = pSpec->GetTempStrandRemovalTensionStressFactor();
         pSpec->GetTempStrandRemovalMaximumTensionStress(&bCheckMax,&fmax);
      }
   }
   else if ( intervalIdx == railingSystemIntervalIdx )
   {
      ATLASSERT( ls == pgsTypes::ServiceI );
      ATLASSERT( CheckFinalDeadLoadTensionStress() ); // if this fires, why are you asking for this if they aren't being used?
      x = pSpec->GetBs2MaxConcreteTens();
      pSpec->GetBs2AbsMaxConcreteTens(&bCheckMax,&fmax);
   }
   else
   {
      // now for the "normal" cases...
      if ( bIsStressingInterval )
      {
         // if this is a stressing interval, use allowables from Table 5.9.4.2.1-1
         ATLASSERT( ls == pgsTypes::ServiceI );
         if ( bWithBondedReinforcement )
         {
            x = pSpec->GetAtReleaseTensionStressFactorWithRebar();
         }
         else
         {
            x = pSpec->GetAtReleaseTensionStressFactor();
            pSpec->GetAtReleaseMaximumTensionStress(&bCheckMax,&fmax);
         }
      }
      else
      {
         // if this is a non-stressing interval, use allowables from Table 5.9.4.2.2-1
         if ( intervalIdx < railingSystemIntervalIdx )
         {

            ATLASSERT( ls == pgsTypes::ServiceI );
            x = pSpec->GetErectionTensionStressFactor();
            pSpec->GetErectionMaximumTensionStress(&bCheckMax,&fmax);
         }
         else 
         {
            if ( ls == pgsTypes::ServiceI )
            {
               x = pSpec->GetBs2MaxConcreteTens();
               pSpec->GetBs2AbsMaxConcreteTens(&bCheckMax,&fmax);
            }
            else
            {
               ATLASSERT( ls == pgsTypes::ServiceIII  );
               GET_IFACE(IEnvironment,pEnv);
               int exposureCondition = pEnv->GetExposureCondition() == expNormal ? EXPOSURE_NORMAL : EXPOSURE_SEVERE;
               x = pSpec->GetFinalTensionStressFactor(exposureCondition);
               pSpec->GetFinalTensionStressFactor(exposureCondition,&bCheckMax,&fmax);
            }
         }
      } // end if bIsStressingInterval
   }// end if,else-if 


   *pCoeff    = x;
   *pbMax     = bCheckMax;
   *pMaxValue = fmax;
}

void CSpecAgentImp::GetClosureJointAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone,Float64* pCoeff,bool* pbMax,Float64* pMaxValue)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;
   bool bCheckMax = false;
   Float64 fmax = -99999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,intervalIdx,ls,pgsTypes::Tension));

   GET_IFACE(IIntervals,pIntervals);
   bool bIsTendonStressingInterval = pIntervals->IsTendonStressingInterval(segmentKey,intervalIdx);

   // closure joints have allowables for both "in the precompressed tensile zone" and "in areas other than
   // the precompressed tensile zone" (See Table 5.9.4.1.2-1 and -5.9.4.2.2-1) for both during stressing
   // and non-stressing intervals

   if ( bIsTendonStressingInterval )
   {
      // stressing interval, use Table 5.9.4.1.2-1
      if ( bInPrecompressedTensileZone )
      {
         if ( bWithBondedReinforcement )
         {
            x = pSpec->GetAtStressingPrecompressedTensileZoneTensionStressFactorWithRebar();
         }
         else
         {
            x = pSpec->GetAtStressingPrecompressedTensileZoneTensionStressFactor();
         }
      }
      else
      {
         if ( bWithBondedReinforcement )
         {
            x = pSpec->GetAtStressingOtherLocationTensionStressFactorWithRebar();
         }
         else
         {
            x = pSpec->GetAtStressingOtherLocationTensionStressFactor();
         }
      }
   }
   else
   {
      // non-stressing interval, use Table 5.9.4.2.2-1
      if ( bInPrecompressedTensileZone )
      {
         if ( bWithBondedReinforcement )
         {
            x = pSpec->GetAtServicePrecompressedTensileZoneTensionStressFactorWithRebar();
         }
         else
         {
            x = pSpec->GetAtServicePrecompressedTensileZoneTensionStressFactor();
         }
      }
      else
      {
         if ( bWithBondedReinforcement )
         {
            x = pSpec->GetAtServiceOtherLocationTensionStressFactorWithRebar();
         }
         else
         {
            x = pSpec->GetAtServiceOtherLocationTensionStressFactor();
         }
      }
   }

   *pCoeff    = x;
   *pbMax     = bCheckMax;
   *pMaxValue = fmax;
}

void CSpecAgentImp::GetDeckAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,Float64* pCoeff,bool* pbMax,Float64* pMaxValue)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;
   bool bCheckMax = false;
   Float64 fmax = -99999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,intervalIdx,ls,pgsTypes::Tension));

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();

   ATLASSERT(pIntervals->GetCompositeDeckInterval() <= intervalIdx);

   bool bIsTendonStressingInterval = pIntervals->IsTendonStressingInterval(segmentKey,intervalIdx);

   if ( bIsTendonStressingInterval )
   {
      // if this is a stressing interval, use allowables from Table 5.9.4.2.1-1
      ATLASSERT( ls == pgsTypes::ServiceI );
      if ( bWithBondedReinforcement )
      {
         x = pSpec->GetAtReleaseTensionStressFactorWithRebar();
      }
      else
      {
         x = pSpec->GetAtReleaseTensionStressFactor();
         pSpec->GetAtReleaseMaximumTensionStress(&bCheckMax,&fmax);
      }
   }
   else
   {
      // if this is a non-stressing interval, use allowables from Table 5.9.4.2.2-1
      GET_IFACE(IEnvironment,pEnv);
      int exposureCondition = pEnv->GetExposureCondition() == expNormal ? EXPOSURE_NORMAL : EXPOSURE_SEVERE;
      x = pSpec->GetFinalTensionStressFactor(exposureCondition);
      pSpec->GetFinalTensionStressFactor(exposureCondition,&bCheckMax,&fmax);
   }

   *pCoeff    = x;
   *pbMax     = bCheckMax;
   *pMaxValue = fmax;
}

std::vector<pgsTypes::LimitState> CSpecAgentImp::GetStressCheckLimitStates(IntervalIndexType intervalIdx)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   std::vector<pgsTypes::LimitState> vLimitStates;
   vLimitStates.push_back(pgsTypes::ServiceI);
   if ( liveLoadIntervalIdx <= intervalIdx || intervalIdx == INVALID_INDEX )
   {
      vLimitStates.push_back(pgsTypes::ServiceIII);
      vLimitStates.push_back(lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? pgsTypes::ServiceIA : pgsTypes::FatigueI);
   }
   return vLimitStates;
}

bool CSpecAgentImp::IsStressCheckApplicable(const CGirderKey& girderKey,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::StressType stressType)
{
   ATLASSERT(::IsServiceLimitState(limitState) || ::IsFatigueLimitState(limitState) ); // must be a service limit state
   if ( (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims && limitState == pgsTypes::FatigueI) || 
        (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion()&& limitState == pgsTypes::ServiceIA)
        )
   {
      // if before LRFD 2009 and Fatigue I 
      // - OR -
      // LRFD 2009 and later and Service IA
      //
      // ... don't evaluate this case
      return false;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();

   if ( stressType == pgsTypes::Tension )
   {
      switch(limitState)
      {
      case pgsTypes::ServiceI:
         if ( (erectSegmentIntervalIdx <= intervalIdx && intervalIdx <= castDeckIntervalIdx && !CheckTemporaryStresses()) 
              ||
              (railingSystemIntervalIdx <= intervalIdx && !CheckFinalDeadLoadTensionStress())
              ||
              (liveLoadIntervalIdx <= intervalIdx)
            )
         {
            return false;
         }
         else
         {
            return true;
         }

      case pgsTypes::ServiceI_PermitRoutine:
      case pgsTypes::ServiceI_PermitSpecial:
         return (liveLoadIntervalIdx <= intervalIdx ? true : false);

      case pgsTypes::ServiceIA:
      case pgsTypes::FatigueI:
         return false; // these are compression only limit states

      case pgsTypes::ServiceIII:
      case pgsTypes::ServiceIII_Inventory:
      case pgsTypes::ServiceIII_Operating:
      case pgsTypes::ServiceIII_LegalRoutine:
      case pgsTypes::ServiceIII_LegalSpecial:
         return (liveLoadIntervalIdx <= intervalIdx ? true : false);

      default:
         ATLASSERT(false); // either a new service limit state or a non-service limit state was passed in
      }
   }
   else
   {
      ATLASSERT(stressType == pgsTypes::Compression);

      switch(limitState)
      {
      case pgsTypes::ServiceI:
         if ( erectSegmentIntervalIdx <= intervalIdx && intervalIdx <= castDeckIntervalIdx && !CheckTemporaryStresses() )
         {
            return false;
         }
         else
         {
            return true;
         }

      case pgsTypes::ServiceI_PermitRoutine:
      case pgsTypes::ServiceI_PermitSpecial:
         return (liveLoadIntervalIdx <= intervalIdx ? true : false);

      case pgsTypes::ServiceIA:
      case pgsTypes::FatigueI:
         if ( liveLoadIntervalIdx <= intervalIdx )
         {
            return true; // these are compression only limit states
         }
         else
         {
            return false; // only check if there is live load
         }

      case pgsTypes::ServiceIII:
      case pgsTypes::ServiceIII_Inventory:
      case pgsTypes::ServiceIII_Operating:
      case pgsTypes::ServiceIII_LegalRoutine:
      case pgsTypes::ServiceIII_LegalSpecial:
         return false;

      default:
         ATLASSERT(false); // either a new service limit state or a non-service limit state was passed in
      }
   }

   ATLASSERT(false); // I think the code above should have covered all possible cases... why did we get here?

   // not other stress check are applicable
   return false;
}

bool CSpecAgentImp::HasAllowableTensionWithRebarOption(IntervalIndexType intervalIdx,bool bInPTZ,bool bSegment,const CSegmentKey& segmentKey)
{
   if ( !bSegment )
   {
      // At closure joints, there is always a "with rebar" option in both the PTZ and other areas
      return true;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   if ( intervalIdx < erectionIntervalIdx ||
        intervalIdx == tsRemovalIntervalIdx ||
        intervalIdx == castDeckIntervalIdx )
   {
      // LRFD Table 5.9.4.1.2-1, third bullet... there is always a "with rebar" option
      // in both the PTZ and other areas.

      // by looking at intervals before segment erection, we cover release, lifting, storage, and hauling
      // all of which have a "with rebar" option.

      // Temporary conditions for deck casting and temporary strand removal are also "with rebar" cases
      return true;
   }

   // there is no "with rebar" in the other intervals
   return false;
}

bool CSpecAgentImp::CheckTemporaryStresses()
{
   // I hate using the IDocumentType interface, but I don't
   // think there is a better way to figure out if we have a PGSuper or PGSplice file
   // The temporary stress checks are always required for spliced girders
   GET_IFACE(IDocumentType,pDocType);
   if ( pDocType->IsPGSpliceDocument() )
   {
      // always checking for spliced girders (See LRFD 5.14.1.3.3)
      return true;
   }
   else
   {
      const SpecLibraryEntry* pSpec = GetSpec();
      return pSpec->CheckTemporaryStresses();
   }
}

bool CSpecAgentImp::CheckFinalDeadLoadTensionStress()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckBs2Tension();
}

/////////////////////////////////////////////////////////////////////////////
// ITransverseReinforcementSpec
//
Float64 CSpecAgentImp::GetMaxSplittingStress(Float64 fyRebar)
{
   return lrfdRebar::GetMaxBurstingStress(fyRebar);
}

Float64 CSpecAgentImp::GetSplittingZoneLengthFactor()
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   Float64 bzlf = pSpecEntry->GetSplittingZoneLengthFactor();
   return bzlf;
}

Float64 CSpecAgentImp::GetSplittingZoneLength( Float64 girderHeight )
{
   return girderHeight / GetSplittingZoneLengthFactor();
}

matRebar::Size CSpecAgentImp::GetMinConfinmentBarSize()
{
   return lrfdRebar::GetMinConfinmentBarSize();
}

Float64 CSpecAgentImp::GetMaxConfinmentBarSpacing()
{
   return lrfdRebar::GetMaxConfinmentBarSpacing();
}

Float64 CSpecAgentImp::GetMinConfinmentAvS()
{
   return lrfdRebar::GetMinConfinmentAvS();
}

void CSpecAgentImp::GetMaxStirrupSpacing(Float64 dv,Float64* pSmax1, Float64* pSmax2)
{
   Float64 k1,k2,s1,s2;
   const SpecLibraryEntry* pSpec = GetSpec();
   pSpec->GetMaxStirrupSpacing(&k1,&s1,&k2,&s2);
   *pSmax1 = min(k1*dv,s1); // LRFD equation 5.8.2.7-1
   *pSmax2 = min(k2*dv,s2); // LRFD equation 5.8.2.7-2
}

Float64 CSpecAgentImp::GetMinStirrupSpacing(Float64 maxAggregateSize, Float64 barDiameter)
{
   ATLASSERT(maxAggregateSize>0.0);
   ATLASSERT(barDiameter>0.0);

   Float64 min_spc = Max(1.33*maxAggregateSize, barDiameter);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 abs_min_spc;
   if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
   {
      abs_min_spc = ::ConvertToSysUnits(25., unitMeasure::Millimeter);
   }
   else
   {
      abs_min_spc = ::ConvertToSysUnits(1., unitMeasure::Inch);
   }

   // lrfd requirements are for clear distance, we want cl-to-cl spacing
   min_spc += barDiameter;
   abs_min_spc += barDiameter;

   return Max(min_spc, abs_min_spc);
}


Float64 CSpecAgentImp::GetMinTopFlangeThickness()
{
   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 dim;
   if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
   {
      dim = ::ConvertToSysUnits(50., unitMeasure::Millimeter);
   }
   else
   {
      dim = ::ConvertToSysUnits(2., unitMeasure::Inch);
   }

   return dim;
}

Float64 CSpecAgentImp::GetMinWebThickness()
{
   const SpecLibraryEntry* pSpec = GetSpec();

   bool bPostTension = false;
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ITendonGeometry,pTendonGeom);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups && !bPostTension; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         DuctIndexType nDucts = pTendonGeom->GetDuctCount(CGirderKey(grpIdx,gdrIdx));
         if ( 0 < nDucts )
         {
            bPostTension = true;
            break;
         }
      }
   }

   Float64 dim;
   if ( bPostTension )
   {
      if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
      {
         dim = ::ConvertToSysUnits(165., unitMeasure::Millimeter);
      }
      else
      {
         dim = ::ConvertToSysUnits(6.5, unitMeasure::Inch);
      }
   }
   else
   {
      if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
      {
         dim = ::ConvertToSysUnits(125., unitMeasure::Millimeter);
      }
      else
      {
         dim = ::ConvertToSysUnits(5., unitMeasure::Inch);
      }
   }

   return dim;
}

Float64 CSpecAgentImp::GetMinBottomFlangeThickness()
{
   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 dim;
   if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
   {
      dim = ::ConvertToSysUnits(125., unitMeasure::Millimeter);
   }
   else
   {
      dim = ::ConvertToSysUnits(5., unitMeasure::Inch);
   }

   return dim;
}

/////////////////////////////////////////////////////////////////////
//  ISegmentLiftingSpecCriteria
bool CSpecAgentImp::IsLiftingAnalysisEnabled()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->IsLiftingAnalysisEnabled();
}

void  CSpecAgentImp::GetLiftingImpact(Float64* pDownward, Float64* pUpward)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *pDownward = pSpec->GetLiftingDownwardImpactFactor();
   *pUpward   = pSpec->GetLiftingUpwardImpactFactor();
}

Float64 CSpecAgentImp::GetLiftingCrackingFs()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetCrackingFOSLifting();
}

Float64 CSpecAgentImp::GetLiftingFailureFs()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingFailureFOS();
}

void CSpecAgentImp::GetLiftingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *factor = GetLiftingAllowableTensionFactor();
   pSpec->GetLiftingMaximumTensionStress(pbMax,fmax);
}

Float64 CSpecAgentImp::GetLiftingAllowableTensileConcreteStress(const CSegmentKey& segmentKey)
{
   Float64 factor = GetLiftingAllowableTensionFactor();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fci = pMaterial->GetSegmentDesignFc(segmentKey,liftSegmentIntervalIdx);
   Float64 lambda = pMaterial->GetSegmentLambda(segmentKey);

   Float64 f = factor * lambda * sqrt( fci );

   bool is_max;
   Float64 maxval;
   const SpecLibraryEntry* pSpec = GetSpec();

   pSpec->GetLiftingMaximumTensionStress(&is_max,&maxval);
   if (is_max)
   {
      f = Min(f, maxval);
   }

   return f;
}

Float64 CSpecAgentImp::GetLiftingAllowableTensionFactor()
{
   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 factor = pSpec->GetLiftingTensionStressFactor();
   return factor;
}

Float64 CSpecAgentImp::GetLiftingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey)
{
   Float64 factor = GetLiftingAllowableCompressionFactor();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fci = pMaterial->GetSegmentDesignFc(segmentKey,liftSegmentIntervalIdx);

   Float64 allowable = factor * fci;

   return allowable;
}

Float64 CSpecAgentImp::GetLiftingAllowableCompressionFactor()
{
   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 factor = pSpec->GetLiftingCompressionStressFactor();
   return -factor;
}

Float64 CSpecAgentImp::GetLiftingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey,Float64 fci, bool withMinRebar)
{
   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

   if (withMinRebar)
   {
      Float64 x = GetLiftingWithMildRebarAllowableStressFactor();

      Float64 f = x * lambda * sqrt( fci );
      return f;
   }
   else
   {
      Float64 x; 
      bool bCheckMax;
      Float64 fmax;

      GetLiftingAllowableTensileConcreteStressParameters(&x,&bCheckMax,&fmax);

      Float64 f = x * lambda * sqrt( fci );

      if ( bCheckMax )
      {
         f = Min(f,fmax);
      }

      return f;
   }
}

Float64 CSpecAgentImp::GetLiftingAllowableCompressiveConcreteStressEx(const CSegmentKey &segmentKey,Float64 fci)
{
   Float64 x = GetLiftingAllowableCompressionFactor();

   return x*fci;
}

Float64 CSpecAgentImp::GetHeightOfPickPointAboveGirderTop()
{
   const SpecLibraryEntry* pSpec = GetSpec();

   return pSpec->GetPickPointHeight();
}

Float64 CSpecAgentImp::GetLiftingLoopPlacementTolerance()
{
   const SpecLibraryEntry* pSpec = GetSpec();

   return pSpec->GetLiftingLoopTolerance();
}

Float64 CSpecAgentImp::GetLiftingCableMinInclination()
{
   const SpecLibraryEntry* pSpec = GetSpec();

   return pSpec->GetMinCableInclination();
}

Float64 CSpecAgentImp::GetLiftingSweepTolerance()
{
   const SpecLibraryEntry* pSpec = GetSpec();

   return pSpec->GetLiftingMaximumGirderSweepTolerance();
}

Float64 CSpecAgentImp::GetLiftingModulusOfRupture(const CSegmentKey& segmentKey)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterials);
   Float64 fci = pMaterials->GetSegmentDesignFc(segmentKey,intervalIdx);
   pgsTypes::ConcreteType type = pMaterials->GetSegmentConcreteType(segmentKey);

   return GetLiftingModulusOfRupture(segmentKey,fci,type);
}

Float64 CSpecAgentImp::GetLiftingModulusOfRupture(const CSegmentKey& segmentKey,Float64 fci,pgsTypes::ConcreteType concType)
{
   Float64 x = GetLiftingModulusOfRuptureFactor(concType);

   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

   return x*lambda*sqrt(fci);
}

Float64 CSpecAgentImp::GetLiftingModulusOfRuptureFactor(pgsTypes::ConcreteType concType)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingModulusOfRuptureFactor(concType);
}

Float64 CSpecAgentImp::GetMinimumLiftingPointLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 min_lift_point = pSpec->GetMininumLiftingPointLocation();

   // if less than zero, then use H from the end of the girder
   if ( min_lift_point < 0 )
   {
      GET_IFACE(IPointOfInterest,pIPoi);

      PoiAttributeType searchAttribute = POI_RELEASED_SEGMENT;
      if ( end == pgsTypes::metStart )
      {
         searchAttribute |= POI_0L;
      }
      else
      {
         searchAttribute |= POI_10L;
      }

      std::vector<pgsPointOfInterest> vPoi(pIPoi->GetPointsOfInterest(segmentKey,searchAttribute,POIFIND_AND));
      ATLASSERT(vPoi.size() == 1);
      pgsPointOfInterest poi(vPoi.front());

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      GET_IFACE(ISectionProperties,pSectProp);
      min_lift_point = pSectProp->GetHg( releaseIntervalIdx, poi );
   }

   return min_lift_point;
}

Float64 CSpecAgentImp::GetLiftingPointLocationAccuracy()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingPointLocationAccuracy();
}

stbLiftingCriteria CSpecAgentImp::GetLiftingStabilityCriteria(const CSegmentKey& segmentKey)
{
   stbLiftingCriteria criteria;

   GET_IFACE(IMaterials,pMaterial);
   criteria.Lambda = pMaterial->GetSegmentLambda(segmentKey);
   criteria.CompressionCoefficient = -GetLiftingAllowableCompressionFactor();
   GetLiftingAllowableTensileConcreteStressParameters(&criteria.TensionCoefficient,&criteria.bMaxTension,&criteria.MaxTension);
   criteria.TensionCoefficientWithRebar = GetLiftingWithMildRebarAllowableStressFactor();
   criteria.AllowableCompression = GetLiftingAllowableCompressiveConcreteStress(segmentKey);
   criteria.AllowableTension = GetLiftingAllowableTensileConcreteStress(segmentKey);
   criteria.AllowableTensionWithRebar = GetLiftingWithMildRebarAllowableStress(segmentKey);
   criteria.MinFScr = GetLiftingCrackingFs();
   criteria.MinFSf = GetLiftingFailureFs();

   return criteria;
}

stbLiftingCriteria CSpecAgentImp::GetLiftingStabilityCriteria(const CSegmentKey& segmentKey,const HANDLINGCONFIG& liftConfig)
{
   stbLiftingCriteria criteria;

   GET_IFACE(IMaterials,pMaterial);
   criteria.Lambda = pMaterial->GetSegmentLambda(segmentKey);
   criteria.CompressionCoefficient = -GetLiftingAllowableCompressionFactor();
   GetLiftingAllowableTensileConcreteStressParameters(&criteria.TensionCoefficient,&criteria.bMaxTension,&criteria.MaxTension);
   criteria.TensionCoefficientWithRebar = GetLiftingWithMildRebarAllowableStressFactor();
   criteria.AllowableCompression = GetLiftingAllowableCompressiveConcreteStressEx(segmentKey,liftConfig.GdrConfig.Fci);
   criteria.AllowableTension = GetLiftingAllowableTensileConcreteStressEx(segmentKey,liftConfig.GdrConfig.Fci,false);
   criteria.AllowableTensionWithRebar = GetLiftingAllowableTensileConcreteStressEx(segmentKey,liftConfig.GdrConfig.Fci,true);
   criteria.MinFScr = GetLiftingCrackingFs();
   criteria.MinFSf = GetLiftingFailureFs();

   return criteria;
}

pgsTypes::CamberMethod CSpecAgentImp::GetLiftingCamberMethod()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingCamberMethod();
}

Float64 CSpecAgentImp::GetLiftingIncreaseInCgForCamber()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingCamberPercentEstimate();
}

Float64 CSpecAgentImp::GetLiftingCamberMultiplier()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingCamberMultiplier();
}

pgsTypes::WindType CSpecAgentImp::GetLiftingWindType()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingWindType();
}

Float64 CSpecAgentImp::GetLiftingWindLoad()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingWindLoad();
}

bool CSpecAgentImp::EvaluateLiftingStressesAtEquilibriumAngle()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->EvaluateLiftingStressesAtEquilibriumAngle();
}

//////////////////////////////////////////////////////////////////////
// ISegmentHaulingSpecCriteria
bool CSpecAgentImp::IsHaulingAnalysisEnabled()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->IsHaulingAnalysisEnabled();
}

pgsTypes::HaulingAnalysisMethod CSpecAgentImp::GetHaulingAnalysisMethod()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingAnalysisMethod();
}

void CSpecAgentImp::GetHaulingImpact(Float64* pDownward, Float64* pUpward)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *pDownward = pSpec->GetHaulingDownwardImpactFactor();
   *pUpward   = pSpec->GetHaulingUpwardImpactFactor();
}

Float64 CSpecAgentImp::GetHaulingCrackingFs()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingCrackingFOS();
}

Float64 CSpecAgentImp::GetHaulingRolloverFs()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingFailureFOS();
}

void CSpecAgentImp::GetHaulingAllowableTensileConcreteStressParametersNormalCrown(Float64* factor,bool* pbMax,Float64* fmax)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *factor = GetHaulingAllowableTensionFactorNormalCrown();
   pSpec->GetHaulingMaximumTensionStressNormalCrown(pbMax,fmax);
}

Float64 CSpecAgentImp::GetHaulingAllowableTensileConcreteStressNormalCrown(const CSegmentKey& segmentKey)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = GetHaulingAllowableTensionFactorNormalCrown();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fc = pMaterial->GetSegmentDesignFc(segmentKey,haulSegmentIntervalIdx);
   Float64 lambda = pMaterial->GetSegmentLambda(segmentKey);

   Float64 allow = factor * lambda * sqrt( fc );

   bool is_max;
   Float64 maxval;
   pSpec->GetHaulingMaximumTensionStressNormalCrown(&is_max,&maxval);
   if (is_max)
   {
      allow = Min(allow, maxval);
   }

   return allow;
}

void CSpecAgentImp::GetHaulingAllowableTensileConcreteStressParametersMaxSuper(Float64* factor,bool* pbMax,Float64* fmax)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *factor = GetHaulingAllowableTensionFactorMaxSuper();
   pSpec->GetHaulingMaximumTensionStressMaxSuper(pbMax,fmax);
}

Float64 CSpecAgentImp::GetHaulingAllowableTensileConcreteStressMaxSuper(const CSegmentKey& segmentKey)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = GetHaulingAllowableTensionFactorMaxSuper();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fc = pMaterial->GetSegmentDesignFc(segmentKey,haulSegmentIntervalIdx);
   Float64 lambda = pMaterial->GetSegmentLambda(segmentKey);

   Float64 allow = factor * lambda * sqrt( fc );

   bool is_max;
   Float64 maxval;
   pSpec->GetHaulingMaximumTensionStressMaxSuper(&is_max,&maxval);
   if (is_max)
   {
      allow = Min(allow, maxval);
   }

   return allow;
}

Float64 CSpecAgentImp::GetHaulingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey)
{
   Float64 factor = GetHaulingAllowableCompressionFactor();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fc = pMaterial->GetSegmentDesignFc(segmentKey,haulSegmentIntervalIdx);

   Float64 allowable = factor * fc;
   return allowable;
}

Float64 CSpecAgentImp::GetHaulingAllowableTensionFactorNormalCrown()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = pSpec->GetHaulingTensionStressFactorNormalCrown();
   return factor;
}

Float64 CSpecAgentImp::GetHaulingAllowableTensionFactorMaxSuper()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = pSpec->GetHaulingTensionStressFactorMaxSuper();
   return factor;
}

Float64 CSpecAgentImp::GetHaulingAllowableCompressionFactor()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = pSpec->GetHaulingCompressionStressFactor();
   return -factor;
}

Float64 CSpecAgentImp::GetHaulingAllowableTensileConcreteStressExNormalCrown(const CSegmentKey& segmentKey,Float64 fc, bool includeRebar)
{
   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

   if (includeRebar)
   {
      Float64 x = GetHaulingWithMildRebarAllowableStressFactorNormalCrown();

      Float64 f = x * lambda * sqrt( fc );
      return f;
   }
   else
   {
      Float64 x; 
      bool bCheckMax;
      Float64 fmax;

      GetHaulingAllowableTensileConcreteStressParametersNormalCrown(&x,&bCheckMax,&fmax);

      Float64 f = x * lambda * sqrt( fc );

      if ( bCheckMax )
      {
         f = Min(f,fmax);
      }

      return f;
   }
}

Float64 CSpecAgentImp::GetHaulingAllowableTensileConcreteStressExMaxSuper(const CSegmentKey& segmentKey,Float64 fc, bool includeRebar)
{
   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

   if (includeRebar)
   {
      Float64 x = GetHaulingWithMildRebarAllowableStressFactorMaxSuper(segmentKey);

      Float64 f = x * lambda * sqrt( fc );
      return f;
   }
   else
   {
      Float64 x; 
      bool bCheckMax;
      Float64 fmax;

      GetHaulingAllowableTensileConcreteStressParametersMaxSuper(&x,&bCheckMax,&fmax);

      Float64 f = x * lambda * sqrt( fc );

      if ( bCheckMax )
      {
         f = Min(f,fmax);
      }

      return f;
   }
}

Float64 CSpecAgentImp::GetHaulingAllowableCompressiveConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc)
{
   Float64 x = GetHaulingAllowableCompressionFactor();

   return x*fc;
}

pgsTypes::HaulingImpact CSpecAgentImp::GetHaulingImpactUsage()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingImpactUsage();
}

Float64 CSpecAgentImp::GetNormalCrownSlope()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetRoadwayCrownSlope();
}

Float64 CSpecAgentImp::GetMaxSuperelevation()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetRoadwaySuperelevation();
}

Float64 CSpecAgentImp::GetHaulingSweepTolerance()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingMaximumGirderSweepTolerance();
}

Float64 CSpecAgentImp::GetHaulingSupportPlacementTolerance()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingSupportPlacementTolerance();
}

pgsTypes::CamberMethod CSpecAgentImp::GetHaulingCamberMethod()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingCamberMethod();
}

Float64 CSpecAgentImp::GetHaulingIncreaseInCgForCamber()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingCamberPercentEstimate();
}

Float64 CSpecAgentImp::GetHaulingCamberMultiplier()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingCamberMultiplier();
}

Float64 CSpecAgentImp::GetRollStiffness(const CSegmentKey& segmentKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   return pSegment->HandlingData.pHaulTruckLibraryEntry->GetRollStiffness();
}

Float64 CSpecAgentImp::GetHeightOfGirderBottomAboveRoadway(const CSegmentKey& segmentKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   return pSegment->HandlingData.pHaulTruckLibraryEntry->GetBottomOfGirderHeight();
}

Float64 CSpecAgentImp::GetHeightOfTruckRollCenterAboveRoadway(const CSegmentKey& segmentKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   return pSegment->HandlingData.pHaulTruckLibraryEntry->GetRollCenterHeight();
}

Float64 CSpecAgentImp::GetAxleWidth(const CSegmentKey& segmentKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   return pSegment->HandlingData.pHaulTruckLibraryEntry->GetAxleWidth();
}

Float64 CSpecAgentImp::GetAllowableDistanceBetweenSupports(const CSegmentKey& segmentKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   return pSegment->HandlingData.pHaulTruckLibraryEntry->GetMaxDistanceBetweenBunkPoints();
}

Float64 CSpecAgentImp::GetAllowableLeadingOverhang(const CSegmentKey& segmentKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   return pSegment->HandlingData.pHaulTruckLibraryEntry->GetMaximumLeadingOverhang();
}

Float64 CSpecAgentImp::GetMaxGirderWgt(const CSegmentKey& segmentKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   return pSegment->HandlingData.pHaulTruckLibraryEntry->GetMaxGirderWeight();
}

Float64 CSpecAgentImp::GetMinimumHaulingSupportLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 min_pick_point = pSpec->GetMininumTruckSupportLocation();

   // if less than zero, then use H from the end of the girder
   if ( min_pick_point < 0 )
   {
      GET_IFACE(IPointOfInterest,pIPoi);

      PoiAttributeType searchAttribute = POI_RELEASED_SEGMENT;
      if ( end == pgsTypes::metStart )
      {
         searchAttribute |= POI_0L;
      }
      else
      {
         searchAttribute |= POI_10L;
      }

      std::vector<pgsPointOfInterest> vPoi(pIPoi->GetPointsOfInterest(segmentKey,searchAttribute,POIFIND_AND));
      ATLASSERT(vPoi.size() == 1);
      pgsPointOfInterest poi(vPoi.front());

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      GET_IFACE(ISectionProperties,pSectProp);
      min_pick_point = pSectProp->GetHg( releaseIntervalIdx, poi );
   }

   return min_pick_point;
}

Float64 CSpecAgentImp::GetHaulingSupportLocationAccuracy()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetTruckSupportLocationAccuracy();
}

pgsTypes::WindType CSpecAgentImp::GetHaulingWindType()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingWindType();
}

Float64 CSpecAgentImp::GetHaulingWindLoad()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingWindLoad();
}

pgsTypes::CFType CSpecAgentImp::GetCentrifugalForceType()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetCentrifugalForceType();
}

Float64 CSpecAgentImp::GetHaulingSpeed()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingSpeed();
}

Float64 CSpecAgentImp::GetTurningRadius()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetTurningRadius();
}

bool CSpecAgentImp::EvaluateHaulingStressesAtEquilibriumAngle()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->EvaluateHaulingStressesAtEquilibriumAngle();
}

stbHaulingCriteria CSpecAgentImp::GetHaulingStabilityCriteria(const CSegmentKey& segmentKey)
{
   stbHaulingCriteria criteria;

   GET_IFACE(IMaterials,pMaterial);
   pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
   criteria.Lambda = pMaterial->GetSegmentLambda(segmentKey);

   criteria.CompressionCoefficient         = -GetHaulingAllowableCompressionFactor();
   criteria.AllowableCompression           = GetHaulingAllowableCompressiveConcreteStress(segmentKey);

   GetHaulingAllowableTensileConcreteStressParametersNormalCrown(&criteria.TensionCoefficient[stbTypes::CrownSlope],&criteria.bMaxTension[stbTypes::CrownSlope],&criteria.MaxTension[stbTypes::CrownSlope]);
   criteria.TensionCoefficientWithRebar[stbTypes::CrownSlope]    = GetHaulingWithMildRebarAllowableStressFactorNormalCrown();
   criteria.AllowableTension[stbTypes::CrownSlope]               = GetHaulingAllowableTensileConcreteStressNormalCrown(segmentKey);
   criteria.AllowableTensionWithRebar[stbTypes::CrownSlope]      = GetHaulingWithMildRebarAllowableStressNormalCrown(segmentKey);

   GetHaulingAllowableTensileConcreteStressParametersMaxSuper(&criteria.TensionCoefficient[stbTypes::MaxSuper],&criteria.bMaxTension[stbTypes::MaxSuper],&criteria.MaxTension[stbTypes::MaxSuper]);
   criteria.TensionCoefficientWithRebar[stbTypes::MaxSuper]    = GetHaulingWithMildRebarAllowableStressFactorMaxSuper(segmentKey);
   criteria.AllowableTension[stbTypes::MaxSuper]               = GetHaulingAllowableTensileConcreteStressMaxSuper(segmentKey);
   criteria.AllowableTensionWithRebar[stbTypes::MaxSuper]      = GetHaulingWithMildRebarAllowableStressMaxSuper(segmentKey);

   criteria.MinFScr = GetHaulingCrackingFs();
   criteria.MinFSf  = GetHaulingRolloverFs();

   criteria.MaxClearSpan       = GetAllowableDistanceBetweenSupports(segmentKey);
   criteria.MaxLeadingOverhang = GetAllowableLeadingOverhang(segmentKey);
   criteria.MaxGirderWeight    = GetMaxGirderWgt(segmentKey);


   return criteria;
}

stbHaulingCriteria CSpecAgentImp::GetHaulingStabilityCriteria(const CSegmentKey& segmentKey,const HANDLINGCONFIG& haulConfig)
{
   stbHaulingCriteria criteria;

   GET_IFACE(IMaterials,pMaterial);
   pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
   criteria.Lambda = pMaterial->GetSegmentLambda(segmentKey);

   criteria.CompressionCoefficient = -GetHaulingAllowableCompressionFactor();
   criteria.AllowableCompression = GetHaulingAllowableCompressiveConcreteStressEx(segmentKey,haulConfig.GdrConfig.Fc);

   GetHaulingAllowableTensileConcreteStressParametersNormalCrown(&criteria.TensionCoefficient[stbTypes::CrownSlope],&criteria.bMaxTension[stbTypes::CrownSlope],&criteria.MaxTension[stbTypes::CrownSlope]);
   criteria.TensionCoefficientWithRebar[stbTypes::CrownSlope] = GetHaulingWithMildRebarAllowableStressFactorNormalCrown();
#if defined MATCH_OLD_ANALYSIS
   // this is a bug in the older version... it doesn't take into account the concrete strength in the configuration
   criteria.AllowableTension[stbTypes::CrownSlope]               = GetHaulingAllowableTensileConcreteStressNormalCrown(segmentKey);
   criteria.AllowableTensionWithRebar[stbTypes::CrownSlope]      = GetHaulingWithMildRebarAllowableStressNormalCrown(segmentKey);
#else
   if (haulConfig.bIgnoreGirderConfig)
   {
      criteria.AllowableTension[stbTypes::CrownSlope]               = GetHaulingAllowableTensileConcreteStressNormalCrown(segmentKey);
      criteria.AllowableTensionWithRebar[stbTypes::CrownSlope]      = GetHaulingWithMildRebarAllowableStressNormalCrown(segmentKey);
   }
   else
   {
      criteria.AllowableTension[stbTypes::CrownSlope] = GetHaulingAllowableTensileConcreteStressExNormalCrown(segmentKey,haulConfig.GdrConfig.Fc,false);
      criteria.AllowableTensionWithRebar[stbTypes::CrownSlope] = GetHaulingAllowableTensileConcreteStressExNormalCrown(segmentKey,haulConfig.GdrConfig.Fc,true);
   }
#endif

   GetHaulingAllowableTensileConcreteStressParametersMaxSuper(&criteria.TensionCoefficient[stbTypes::MaxSuper],&criteria.bMaxTension[stbTypes::MaxSuper],&criteria.MaxTension[stbTypes::MaxSuper]);
   criteria.TensionCoefficientWithRebar[stbTypes::MaxSuper]    = GetHaulingWithMildRebarAllowableStressFactorMaxSuper(segmentKey);
#if defined MATCH_OLD_ANALYSIS
   criteria.AllowableTension[stbTypes::MaxSuper]          = GetHaulingModulusOfRupture(segmentKey,haulConfig.GdrConfig.Fc,haulConfig.GdrConfig.ConcType);
   criteria.AllowableTensionWithRebar[stbTypes::MaxSuper] = GetHaulingModulusOfRupture(segmentKey,haulConfig.GdrConfig.Fc,haulConfig.GdrConfig.ConcType);
#else
   if (haulConfig.bIgnoreGirderConfig)
   {
      criteria.AllowableTension[stbTypes::MaxSuper]               = GetHaulingAllowableTensileConcreteStressMaxSuper(segmentKey);
      criteria.AllowableTensionWithRebar[stbTypes::MaxSuper]      = GetHaulingWithMildRebarAllowableStressMaxSuper(segmentKey);
   }
   else
   {
      criteria.AllowableTension[stbTypes::MaxSuper]          = GetHaulingAllowableTensileConcreteStressExMaxSuper(segmentKey,haulConfig.GdrConfig.Fc,false);
      criteria.AllowableTensionWithRebar[stbTypes::MaxSuper] = GetHaulingAllowableTensileConcreteStressExMaxSuper(segmentKey,haulConfig.GdrConfig.Fc,true);
   }
#endif

   criteria.MinFScr = GetHaulingCrackingFs();
   criteria.MinFSf  = GetHaulingRolloverFs();

   if (haulConfig.pHaulTruckEntry == nullptr )
   {
      criteria.MaxClearSpan       = GetAllowableDistanceBetweenSupports(segmentKey);
      criteria.MaxLeadingOverhang = GetAllowableLeadingOverhang(segmentKey);
      criteria.MaxGirderWeight    = GetMaxGirderWgt(segmentKey);
   }
   else
   {
      criteria.MaxClearSpan       = haulConfig.pHaulTruckEntry->GetMaxDistanceBetweenBunkPoints();
      criteria.MaxLeadingOverhang = haulConfig.pHaulTruckEntry->GetMaximumLeadingOverhang();
      criteria.MaxGirderWeight    = haulConfig.pHaulTruckEntry->GetMaxGirderWeight();
   }

   return criteria;
}

/////////////////////////////////////////////////////////////////////
//  IKdotGirderLiftingSpecCriteria
// Spec criteria for KDOT analyses
Float64 CSpecAgentImp::GetKdotHaulingAllowableTensileConcreteStress(const CSegmentKey& segmentKey)
{
   return GetHaulingAllowableTensileConcreteStressNormalCrown(segmentKey);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey)
{
   return GetHaulingAllowableCompressiveConcreteStress(segmentKey);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableTensionFactor()
{
   return GetHaulingAllowableTensionFactorNormalCrown();
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableCompressionFactor()
{
   return GetHaulingAllowableCompressionFactor();
}

Float64 CSpecAgentImp::GetKdotHaulingWithMildRebarAllowableStress(const CSegmentKey& segmentKey)

{
   return GetHaulingWithMildRebarAllowableStressNormalCrown(segmentKey);
}

Float64 CSpecAgentImp::GetKdotHaulingWithMildRebarAllowableStressFactor()
{
   return GetHaulingWithMildRebarAllowableStressFactorNormalCrown();
}

void CSpecAgentImp::GetKdotHaulingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax)
{
   GetHaulingAllowableTensileConcreteStressParametersNormalCrown(factor, pbMax, fmax);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc, bool includeRebar)
{
   return GetHaulingAllowableTensileConcreteStressExNormalCrown(segmentKey, fc, includeRebar);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableCompressiveConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc)
{
   return GetHaulingAllowableCompressiveConcreteStressEx(segmentKey,fc);
}

void CSpecAgentImp::GetMinimumHaulingSupportLocation(Float64* pHardDistance, bool* pUseFactoredLength, Float64* pLengthFactor)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *pHardDistance = pSpec->GetMininumTruckSupportLocation();
   *pUseFactoredLength = pSpec->GetUseMinTruckSupportLocationFactor();
   *pLengthFactor = pSpec->GetMinTruckSupportLocationFactor();
}

Float64 CSpecAgentImp::GetHaulingDesignLocationAccuracy()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetTruckSupportLocationAccuracy();
}

void CSpecAgentImp::GetHaulingGFactors(Float64* pOverhangFactor, Float64* pInteriorFactor)
{
   const SpecLibraryEntry* pSpec = GetSpec();

   *pOverhangFactor = pSpec->GetOverhangGFactor();
   *pInteriorFactor = pSpec->GetInteriorGFactor();
}

/////////////////////////////////////////////////////////////////////
// IDebondLimits
Float64 CSpecAgentImp::GetMaxDebondedStrands(const CSegmentKey& segmentKey)
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);

   return pGirderEntry->GetMaxTotalFractionDebondedStrands();
}

Float64 CSpecAgentImp::GetMaxDebondedStrandsPerRow(const CSegmentKey& segmentKey)
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);
   return pGirderEntry->GetMaxFractionDebondedStrandsPerRow();
}

Float64 CSpecAgentImp::GetMaxDebondedStrandsPerSection(const CSegmentKey& segmentKey)
{
   StrandIndexType nMax;
   Float64 fMax;

   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);
   pGirderEntry->GetMaxDebondedStrandsPerSection(&nMax,&fMax);

   return fMax;
}

StrandIndexType CSpecAgentImp::GetMaxNumDebondedStrandsPerSection(const CSegmentKey& segmentKey)
{
   StrandIndexType nMax;
   Float64 fMax;

   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);
   pGirderEntry->GetMaxDebondedStrandsPerSection(&nMax,&fMax);

   return nMax;
}

void CSpecAgentImp::GetMaxDebondLength(const CSegmentKey& segmentKey, Float64* pLen, pgsTypes::DebondLengthControl* pControl)
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);

   bool bSpanFraction, buseHard;
   Float64 spanFraction, hardDistance;
   pGirderEntry->GetMaxDebondedLength(&bSpanFraction, &spanFraction, &buseHard, &hardDistance);

   GET_IFACE(IBridge,pBridge);

   Float64 gdrlength = pBridge->GetSegmentLength(segmentKey);

   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_5L) );
   ATLASSERT(vPOI.size() == 1);
   pgsPointOfInterest poi( vPOI[0] );

   // always use half girder length - development length
   GET_IFACE(IPretensionForce, pPrestressForce ); 
   Float64 dev_len = pPrestressForce->GetDevLength(poi,true); // set debonding to true to get max length

   Float64 min_len = gdrlength/2.0 - dev_len;
   *pControl = pgsTypes::mdbDefault;

   if (bSpanFraction)
   {
      Float64 sflen = gdrlength * spanFraction;
      if (sflen < min_len)
      {
         min_len = sflen;
         *pControl = pgsTypes::mbdFractional;
      }
   }

   if (buseHard)
   {
      if (hardDistance < min_len )
      {
         min_len = hardDistance;
         *pControl = pgsTypes::mdbHardLength;
      }
   }

   *pLen = min_len>0.0 ? min_len : 0.0; // don't return less than zero
}

Float64 CSpecAgentImp::GetMinDebondSectionDistance(const CSegmentKey& segmentKey)
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);
   return pGirderEntry->GetMinDebondSectionLength();
}

/////////////////////////////////////////////////////////////////////
// IResistanceFactors
void CSpecAgentImp::GetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64* phiTensionPS,Float64* phiTensionRC,Float64* phiTensionSpliced,Float64* phiCompression)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   pSpec->GetFlexureResistanceFactors(type,phiTensionPS,phiTensionRC,phiTensionSpliced,phiCompression);
}

void CSpecAgentImp::GetFlexuralStrainLimits(matPsStrand::Grade grade,matPsStrand::Type type,Float64* pecl,Float64* petl)
{
   // The values for Grade 60 are the same as for all types of strand
   GetFlexuralStrainLimits(matRebar::Grade60,pecl,petl);
}

void CSpecAgentImp::GetFlexuralStrainLimits(matRebar::Grade rebarGrade,Float64* pecl,Float64* petl)
{
   *pecl = lrfdRebar::GetCompressionControlledStrainLimit(rebarGrade);
   *petl = lrfdRebar::GetTensionControlledStrainLimit(rebarGrade);

#if defined _DEBUG
   Float64 ecl, etl;
   switch (rebarGrade )
   {
   case matRebar::Grade40:
      ecl = 0.0014;
      etl = 0.005;
      break;

   case matRebar::Grade60:
      ecl = 0.002;
      etl = 0.005;
      break;

   case matRebar::Grade75:
      ecl = 0.0028;
      etl = 0.0050;
      break;

   case matRebar::Grade80:
      ecl = 0.0030;
      etl = 0.0056;
      break;

   case matRebar::Grade100:
      ecl = 0.0040;
      etl = 0.0080;
      break;

   default:
      ATLASSERT(false); // new rebar grade?
   }
   ATLASSERT(IsEqual(*pecl,ecl,0.0001));
   ATLASSERT(IsEqual(*petl,etl,0.0001));
#endif
}

Float64 CSpecAgentImp::GetShearResistanceFactor(pgsTypes::ConcreteType type)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetShearResistanceFactor(type);
}

Float64 CSpecAgentImp::GetClosureJointFlexureResistanceFactor(pgsTypes::ConcreteType type)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetClosureJointFlexureResistanceFactor(type);
}

Float64 CSpecAgentImp::GetClosureJointShearResistanceFactor(pgsTypes::ConcreteType type)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetClosureJointShearResistanceFactor(type);
}

///////////////////////////////////////////////////
// IInterfaceShearRequirements 
ShearFlowMethod CSpecAgentImp::GetShearFlowMethod()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetShearFlowMethod();
}

Float64 CSpecAgentImp::GetMaxShearConnectorSpacing(const pgsPointOfInterest& poi)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 sMax = pSpec->GetMaxInterfaceShearConnectorSpacing();
   if ( lrfdVersionMgr::SeventhEdition2014 <= lrfdVersionMgr::GetVersion() )
   {
      GET_IFACE(ISectionProperties,pSectProp);
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;
      Float64 Hg = pSectProp->GetHg(intervalIdx,poi);
      sMax = min(Hg,sMax);
   }
   return sMax;
}

////////////////////
// IDuctLimits
Float64 CSpecAgentImp::GetRadiusOfCurvatureLimit(const CGirderKey& girderKey)
{
   // LRFD 5.4.6.1
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPTData* pPTData = pIBridgeDesc->GetPostTensioning(girderKey);
   return ::ConvertToSysUnits(pPTData->DuctType == pgsTypes::dtPlastic ? 30.0 : 20.0,unitMeasure::Feet);
}

Float64 CSpecAgentImp::GetTendonAreaLimit(const CGirderKey& girderKey)
{
   // LRFD 5.4.6.2
   const SpecLibraryEntry* pSpecEntry = GetSpec();
   Float64 pushRatio, pullRatio;
   pSpecEntry->GetDuctAreaRatio(&pushRatio,&pullRatio);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPTData* pPTData = pIBridgeDesc->GetPostTensioning(girderKey);
   return (pPTData->InstallationType == pgsTypes::sitPush ? pushRatio : pullRatio);
}

Float64 CSpecAgentImp::GetDuctSizeLimit(const CGirderKey& girderKey)
{
   // LRFD 5.4.6.2
   const SpecLibraryEntry* pSpecEntry = GetSpec();
   return pSpecEntry->GetDuctDiameterRatio();
}

////////////////////
// Private methods

const SpecLibraryEntry* CSpecAgentImp::GetSpec()
{
   GET_IFACE( ISpecification, pSpec );
   GET_IFACE( ILibrary,       pLib );

   std::_tstring specName = pSpec->GetSpecification();
   return pLib->GetSpecEntry( specName.c_str() );
}

const GirderLibraryEntry* CSpecAgentImp::GetGirderEntry(const CSegmentKey& segmentKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   return pGirderEntry;
}

bool CSpecAgentImp::IsLoadRatingServiceIIILimitState(pgsTypes::LimitState ls)
{
   return ( ls == pgsTypes::ServiceIII_Inventory ||
            ls == pgsTypes::ServiceIII_Operating ||
            ls == pgsTypes::ServiceIII_LegalRoutine ||
            ls == pgsTypes::ServiceIII_LegalSpecial ) ? true : false;
}
