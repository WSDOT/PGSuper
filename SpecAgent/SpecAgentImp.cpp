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

#include <Units\SysUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DECLARE_LOGFILE;

/////////////////////////////////////////////////////////////////////////////
// CSpecAgentImp


/////////////////////////////////////////////////////////////////////////////
// IAgent
//
STDMETHODIMP CSpecAgentImp::SetBroker(IBroker* pBroker)
{
   AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CSpecAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   pBrokerInit->RegInterface( IID_IAllowableStrandStress,       this );
   pBrokerInit->RegInterface( IID_IAllowableTendonStress,       this );
   pBrokerInit->RegInterface( IID_IAllowableConcreteStress,     this );
   pBrokerInit->RegInterface( IID_ITransverseReinforcementSpec, this );
   pBrokerInit->RegInterface( IID_IPrecastIGirderDetailsSpec,   this );
   pBrokerInit->RegInterface( IID_IGirderLiftingSpecCriteria,   this );
   pBrokerInit->RegInterface( IID_IGirderHaulingSpecCriteria,   this );
   pBrokerInit->RegInterface( IID_IKdotGirderHaulingSpecCriteria, this );
   pBrokerInit->RegInterface( IID_IDebondLimits,                this );
   pBrokerInit->RegInterface( IID_IResistanceFactors,           this );

   return S_OK;
}

STDMETHODIMP CSpecAgentImp::Init()
{
   CREATE_LOGFILE("SpecAgent");
   AGENT_INIT;
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
   CLOSE_LOGFILE;
   AGENT_CLEAR_INTERFACE_CACHE;
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
      return 0.0;

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
      return 0.0;

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
      return 0.0;

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
      return 0.0;

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
      return 0.0;

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetTendonMaterial(girderKey);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_AT_JACKING,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetAllowablePriorToSeating(const CGirderKey& girderKey)
{
   if ( !CheckTendonStressPriorToSeating() )
      return 0.0;

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetTendonMaterial(girderKey);

   Float64 fpy = lrfdPsStrand::GetYieldStrength(pStrand->GetGrade(),pStrand->GetType());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_PRIOR_TO_SEATING,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpy;
}

Float64 CSpecAgentImp::GetAllowableAfterAnchorSetAtAnchorage(const CGirderKey& girderKey)
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetTendonMaterial(girderKey);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_ANCHORAGES_AFTER_SEATING,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetAllowableAfterAnchorSet(const CGirderKey& girderKey)
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetTendonMaterial(girderKey);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_ELSEWHERE_AFTER_SEATING,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetAllowableAfterLosses(const CGirderKey& girderKey)
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetTendonMaterial(girderKey);

   Float64 fpy = lrfdPsStrand::GetYieldStrength(pStrand->GetGrade(),pStrand->GetType());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_AFTER_ALL_LOSSES,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpy;
}

/////////////////////////////////////////////////////////////////////////////
// IAllowableConcreteStress
//
Float64 CSpecAgentImp::GetAllowableCompressionStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   if ( IsGirderStressLocation(stressLocation) )
   {
      if ( poi.HasAttribute(POI_CLOSURE) )
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
      if ( poi.HasAttribute(POI_CLOSURE) )
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

Float64 CSpecAgentImp::GetAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   if ( IsGirderStressLocation(stressLocation) )
   {
      if ( poi.HasAttribute(POI_CLOSURE) )
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
      if ( poi.HasAttribute(POI_CLOSURE) )
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
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Compression));

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetSegmentFc(segmentKey,intervalIdx);

   Float64 fAllow = GetSegmentAllowableCompressionStress(poi,intervalIdx,ls,fc);
   return fAllow;
}

Float64 CSpecAgentImp::GetClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Compression));

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetClosureJointFc(segmentKey,intervalIdx);

   Float64 fAllow = GetClosureJointAllowableCompressionStress(poi,intervalIdx,ls,fc);
   return fAllow;
}

Float64 CSpecAgentImp::GetDeckAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Compression));

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetDeckFc(intervalIdx);

   Float64 fAllow = GetDeckAllowableCompressionStress(poi,intervalIdx,ls,fc);
   return fAllow;
}

Float64 CSpecAgentImp::GetSegmentAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement)
{
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Tension));

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   if ( IsLoadRatingLimitState(ls) )
   {
#if defined _DEBUG
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      ATLASSERT(liveLoadIntervalIdx <= intervalIdx );
#endif
      return GetLoadRatingAllowableTension(segmentKey,ls);
   }

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetSegmentFc(segmentKey,intervalIdx);

   Float64 fAllow = GetSegmentAllowableTensionStress(poi,intervalIdx,ls,fc,bWithBondedReinforcement);
   return fAllow;
}

Float64 CSpecAgentImp::GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone)
{
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Tension));

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   if ( IsLoadRatingLimitState(ls) )
   {
#if defined _DEBUG
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      ATLASSERT(liveLoadIntervalIdx <= intervalIdx );
#endif
      return GetLoadRatingAllowableTension(segmentKey,ls);
   }

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetClosureJointFc(segmentKey,intervalIdx);

   Float64 fAllow = GetClosureJointAllowableTensionStress(poi,intervalIdx,ls,fc,bWithBondedReinforcement,bInPrecompressedTensileZone);
   return fAllow;
}

Float64 CSpecAgentImp::GetDeckAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement)
{
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Tension));

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   if ( IsLoadRatingLimitState(ls) )
   {
#if defined _DEBUG
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      ATLASSERT(liveLoadIntervalIdx <= intervalIdx );
#endif
      return GetLoadRatingAllowableTension(segmentKey,ls);
   }

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetDeckFc(intervalIdx);

   Float64 fAllow = GetDeckAllowableTensionStress(poi,intervalIdx,ls,fc,bWithBondedReinforcement);
   return fAllow;
}

std::vector<Float64> CSpecAgentImp::GetGirderAllowableCompressionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Compression));

   std::vector<Float64> vStress;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      if ( poi.HasAttribute(POI_CLOSURE) )
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
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Compression));

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
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Tension));

   std::vector<Float64> vStress;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      if ( poi.HasAttribute(POI_CLOSURE) )
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
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Tension));

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

   Float64 f = x * sqrt( fc );
   if ( bCheckMax )
      f = Min(f,fmax);

   return f;
}

Float64 CSpecAgentImp::GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone)
{
   Float64 x;
   bool bCheckMax;
   Float64 fmax; // In system units
   GetClosureJointAllowableTensionStressCoefficient(poi,intervalIdx,ls,bWithBondedReinforcement,bInPrecompressedTensileZone,&x,&bCheckMax,&fmax);

   Float64 f = x * sqrt( fc );
   if ( bCheckMax )
      f = Min(f,fmax);

   return f;
}

Float64 CSpecAgentImp::GetDeckAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc,bool bWithBondedReinforcement)
{
   Float64 x;
   bool bCheckMax;
   Float64 fmax; // In system units
   GetDeckAllowableTensionStressCoefficient(poi,intervalIdx,ls,bWithBondedReinforcement,&x,&bCheckMax,&fmax);

   Float64 f = x * sqrt( fc );
   if ( bCheckMax )
      f = Min(f,fmax);

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
#pragma Reminder("UPDATE: considate with other allowable stress method")
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fci = pMaterial->GetSegmentFc(segmentKey,liftSegmentIntervalIdx);

   Float64 x = GetLiftingWithMildRebarAllowableStressFactor();

   return x*sqrt(fci);
}

Float64 CSpecAgentImp::GetHaulingWithMildRebarAllowableStressFactor()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = pSpec->GetHaulingTensionStressFactorWithRebar();
   return x;
}

Float64 CSpecAgentImp::GetHaulingWithMildRebarAllowableStress(const CSegmentKey& segmentKey)
{
#pragma Reminder("UPDATE: considate with other allowable stress method")
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fc = pMaterial->GetSegmentFc(segmentKey,haulSegmentIntervalIdx);

   Float64 x = GetHaulingWithMildRebarAllowableStressFactor();

   return x*sqrt(fc);
}

Float64 CSpecAgentImp::GetHaulingModulusOfRupture(const CSegmentKey& segmentKey)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetSegmentFc(segmentKey,intervalIdx);

   GET_IFACE(IMaterials,pMaterial);
   pgsTypes::ConcreteType type = pMaterial->GetSegmentConcreteType(segmentKey);

   return GetHaulingModulusOfRupture(fc,type);
}

Float64 CSpecAgentImp::GetHaulingModulusOfRupture(Float64 fc,pgsTypes::ConcreteType concType)
{
   Float64 x = GetHaulingModulusOfRuptureFactor(concType);
   return x*sqrt(fc);
}

Float64 CSpecAgentImp::GetHaulingModulusOfRuptureFactor(pgsTypes::ConcreteType concType)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingModulusOfRuptureFactor(concType);
}

Float64 CSpecAgentImp::GetSegmentAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls)
{
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Compression));

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();


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
      bool bIsTendonStressingInterval = pIntervals->IsTendonStressingInterval(segmentKey,intervalIdx);
      bool bIsStressingInterval = (releaseIntervalIdx == intervalIdx || bIsTendonStressingInterval ? true : false);

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
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Compression));

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(poi.HasAttribute(POI_CLOSURE));

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
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Compression));

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();


   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   bool bIsTendonStressingInterval = pIntervals->IsTendonStressingInterval(segmentKey,intervalIdx);

   ATLASSERT(compositeDeckIntervalIdx <= intervalIdx); // why are you asking for allowable deck stresses before the deck can take load?

#pragma Reminder("REVIEW: consider having a separate set of allowable stress coefficients in the spec library for deck")
   // this is where those coefficients would be used

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
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Tension));

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;
   bool bCheckMax = false;
   Float64 fmax = -99999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IEnvironment,pEnv);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liftingIntervalIdx       = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType haulingIntervalIdx       = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType erectSegmentIdx          = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tempStrandRemovalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   bool bIsTendonStressingInterval = pIntervals->IsTendonStressingInterval(segmentKey,intervalIdx);
   bool bIsStressingInterval = (intervalIdx == releaseIntervalIdx || bIsTendonStressingInterval ? true : false);

   // first deal with the special cases
   if ( intervalIdx == liftingIntervalIdx )
   {
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetLiftingTensionStressFactor();
      pSpec->GetLiftingMaximumTensionStress(&bCheckMax,&fmax);
   }
   else if ( storageIntervalIdx <= intervalIdx && intervalIdx < haulingIntervalIdx )
   {
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetAtReleaseTensionStressFactor();
      pSpec->GetAtReleaseMaximumTensionStress(&bCheckMax,&fmax);
   }
   else if ( intervalIdx == haulingIntervalIdx )
   {
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetHaulingTensionStressFactor();
      pSpec->GetHaulingMaximumTensionStress(&bCheckMax,&fmax);
   } 
   else if ( intervalIdx == tempStrandRemovalIdx )
   {
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetTempStrandRemovalTensionStressFactor();
      pSpec->GetTempStrandRemovalMaximumTensionStress(&bCheckMax,&fmax);
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
            ATLASSERT( ls == pgsTypes::ServiceIII  );
            int exposureCondition = pEnv->GetExposureCondition() == expNormal ? EXPOSURE_NORMAL : EXPOSURE_SEVERE;
            x = pSpec->GetFinalTensionStressFactor(exposureCondition);
            pSpec->GetFinalTensionStressFactor(exposureCondition,&bCheckMax,&fmax);
         }
      } // end if bIsStressingInterval
   }// end if,else-if 


   *pCoeff    = x;
   *pbMax     = bCheckMax;
   *pMaxValue = fmax;
}

void CSpecAgentImp::GetClosureJointAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone,Float64* pCoeff,bool* pbMax,Float64* pMaxValue)
{
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Tension));

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;
   bool bCheckMax = false;
   Float64 fmax = -99999;

   ATLASSERT(poi.HasAttribute(POI_CLOSURE));

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

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
            x = pSpec->GetAtStressingPrecompressedTensileZoneTensionStressFactorWithRebar();
         else
            x = pSpec->GetAtStressingPrecompressedTensileZoneTensionStressFactor();
      }
      else
      {
         if ( bWithBondedReinforcement )
            x = pSpec->GetAtStressingOtherLocationTensionStressFactorWithRebar();
         else
            x = pSpec->GetAtStressingOtherLocationTensionStressFactor();
      }
   }
   else
   {
      // non-stressing interval, use Table 5.9.4.2.2-1
      if ( bInPrecompressedTensileZone )
      {
         if ( bWithBondedReinforcement )
            x = pSpec->GetAtServicePrecompressedTensileZoneTensionStressFactorWithRebar();
         else
            x = pSpec->GetAtServicePrecompressedTensileZoneTensionStressFactor();
      }
      else
      {
         if ( bWithBondedReinforcement )
            x = pSpec->GetAtServiceOtherLocationTensionStressFactorWithRebar();
         else
            x = pSpec->GetAtServiceOtherLocationTensionStressFactor();
      }
   }

   *pCoeff    = x;
   *pbMax     = bCheckMax;
   *pMaxValue = fmax;
}

void CSpecAgentImp::GetDeckAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,Float64* pCoeff,bool* pbMax,Float64* pMaxValue)
{
   ATLASSERT(IsStressCheckApplicable(intervalIdx,ls,pgsTypes::Tension));

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;
   bool bCheckMax = false;
   Float64 fmax = -99999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IEnvironment,pEnv);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx  = pIntervals->GetLiveLoadInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();

   ATLASSERT(pIntervals->GetCompositeDeckInterval() <= intervalIdx);

   bool bIsTendonStressingInterval = pIntervals->IsTendonStressingInterval(segmentKey,intervalIdx);

#pragma Reminder("REVIEW: consider having a separate set of allowable stress coefficients in the spec library for deck")
   // this is where those coefficients would be used

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
      if ( intervalIdx < railingSystemIntervalIdx )
      {
         ATLASSERT( ls == pgsTypes::ServiceI );
         x = pSpec->GetErectionTensionStressFactor();
         pSpec->GetErectionMaximumTensionStress(&bCheckMax,&fmax);
      }
      else
      {
         ATLASSERT( (ls == pgsTypes::ServiceIII)  );
         int exposureCondition = pEnv->GetExposureCondition() == expNormal ? EXPOSURE_NORMAL : EXPOSURE_SEVERE;
         x = pSpec->GetFinalTensionStressFactor(exposureCondition);
         pSpec->GetFinalTensionStressFactor(exposureCondition,&bCheckMax,&fmax);
      }
   }

   *pCoeff    = x;
   *pbMax     = bCheckMax;
   *pMaxValue = fmax;
}

std::vector<pgsTypes::LimitState> CSpecAgentImp::GetStressCheckLimitStates()
{
   std::vector<pgsTypes::LimitState> vLimitStates;
   vLimitStates.push_back(pgsTypes::ServiceI);
   vLimitStates.push_back(pgsTypes::ServiceIII);
   vLimitStates.push_back(lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? pgsTypes::ServiceIA : pgsTypes::FatigueI);
   return vLimitStates;
}

bool CSpecAgentImp::IsStressCheckApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::StressType stressType)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();

   if ( stressType == pgsTypes::Tension && limitState == pgsTypes::ServiceI && (railingSystemIntervalIdx <= intervalIdx && intervalIdx < liveLoadIntervalIdx) )
   {
      // tension is not checked in the Serivce I limit state after the railing system is installed... 
      // this is the "Effective Prestress + Permanent Loads" case only which is a compression check
      // tension is only checked in the Service III limit state after the railing system
      return false;
   }

   // Tension and Compression stresses are always checked in the Service I limit state
   // if this is before live load, except as noted above
   if ( limitState == pgsTypes::ServiceI && intervalIdx < liveLoadIntervalIdx )
   {
      return true;
   }

   if ( liveLoadIntervalIdx <= intervalIdx )
   {
      // After live load is applied...
      if ( stressType == pgsTypes::Tension && limitState == pgsTypes::ServiceIII )
      {
         // ... Service III tension check is applicable
         return true;
      }
      else if ( stressType == pgsTypes::Compression && 
               (limitState == pgsTypes::ServiceI || 
                limitState == (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? pgsTypes::ServiceIA : pgsTypes::FatigueI) )
          )
      {
         // ... Service I/Service IA/Fatigue I compression check is applicable
         return true;
      }
   }

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
   if ( intervalIdx < erectionIntervalIdx )
   {
      // LRFD Table 5.9.4.1.2-1, third bullet... there is always a "with rebar" option
      // in both the PTZ and other areas.

      // by looking at intervals before segment erection, we cover release, lifting, storage, and hauling
      // all of which have a "with rebar" option.
      return true;
   }

   // there is no "with rebar" in the other intervals
   return false;
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

void CSpecAgentImp::GetMaxStirrupSpacing(Float64* sUnderLimit, Float64* sOverLimit)
{
   lrfdRebar::GetMaxStirrupSpacing(sUnderLimit, sOverLimit);

   // check to see if this has been overridden by spec library entry.
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 max_spac = pSpec->GetMaxStirrupSpacing();

   *sUnderLimit = Min(*sUnderLimit, max_spac);
   *sOverLimit = Min(*sOverLimit, max_spac);
}

Float64 CSpecAgentImp::GetMinStirrupSpacing(Float64 maxAggregateSize, Float64 barDiameter)
{
   CHECK(maxAggregateSize>0.0);
   CHECK(barDiameter>0.0);

   Float64 min_spc = Max(1.33*maxAggregateSize, barDiameter);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 abs_min_spc;
   if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
      abs_min_spc = ::ConvertToSysUnits(25., unitMeasure::Millimeter);
   else
      abs_min_spc = ::ConvertToSysUnits(1., unitMeasure::Inch);

   // lrfd requirements are for clear distance, we want cl-to-cl spacing
   min_spc += barDiameter;
   abs_min_spc += barDiameter;

   return Max(min_spc, abs_min_spc);
}


Float64 CSpecAgentImp::GetMinTopFlangeThickness() const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 dim;
   if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
      dim = ::ConvertToSysUnits(50., unitMeasure::Millimeter);
   else
      dim = ::ConvertToSysUnits(2., unitMeasure::Inch);

   return dim;
}

Float64 CSpecAgentImp::GetMinWebThickness() const
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
         dim = ::ConvertToSysUnits(165., unitMeasure::Millimeter);
      else
         dim = ::ConvertToSysUnits(6.5, unitMeasure::Inch);
   }
   else
   {
      if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
         dim = ::ConvertToSysUnits(125., unitMeasure::Millimeter);
      else
         dim = ::ConvertToSysUnits(5., unitMeasure::Inch);
   }

   return dim;
}

Float64 CSpecAgentImp::GetMinBottomFlangeThickness() const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 dim;
   if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
      dim = ::ConvertToSysUnits(125., unitMeasure::Millimeter);
   else
      dim = ::ConvertToSysUnits(5., unitMeasure::Inch);

   return dim;
}

/////////////////////////////////////////////////////////////////////
//  IGirderLiftingSpecCriteria
bool CSpecAgentImp::IsLiftingAnalysisEnabled() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->IsLiftingAnalysisEnabled();
}

void  CSpecAgentImp::GetLiftingImpact(Float64* pDownward, Float64* pUpward) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *pDownward = pSpec->GetLiftingDownwardImpactFactor();
   *pUpward   = pSpec->GetLiftingUpwardImpactFactor();
}

Float64 CSpecAgentImp::GetLiftingCrackingFs() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetCrackingFOSLifting();
}

Float64 CSpecAgentImp::GetLiftingFailureFs() const
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
   Float64 fci = pMaterial->GetSegmentFc(segmentKey,liftSegmentIntervalIdx);

   Float64 f = factor * sqrt( fci );

   bool is_max;
   Float64 maxval;
   const SpecLibraryEntry* pSpec = GetSpec();

   pSpec->GetLiftingMaximumTensionStress(&is_max,&maxval);
   if (is_max)
      f = Min(f, maxval);

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
   Float64 fci = pMaterial->GetSegmentFc(segmentKey,liftSegmentIntervalIdx);

   Float64 allowable = factor * fci;

   return allowable;
}

Float64 CSpecAgentImp::GetLiftingAllowableCompressionFactor()
{
   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 factor = pSpec->GetLiftingCompressionStressFactor();
   return -factor;
}

Float64 CSpecAgentImp::GetLiftingAllowableTensileConcreteStressEx(Float64 fci, bool withMinRebar)
{
   if (withMinRebar)
   {
      Float64 x = GetLiftingWithMildRebarAllowableStressFactor();

      Float64 f = x * sqrt( fci );
      return f;
   }
   else
   {
      Float64 x; 
      bool bCheckMax;
      Float64 fmax;

      GetLiftingAllowableTensileConcreteStressParameters(&x,&bCheckMax,&fmax);

      Float64 f = x * sqrt( fci );

      if ( bCheckMax )
         f = Min(f,fmax);

      return f;
   }
}

Float64 CSpecAgentImp::GetLiftingAllowableCompressiveConcreteStressEx(Float64 fci)
{
   Float64 x = GetLiftingAllowableCompressionFactor();

   return x*fci;
}

Float64 CSpecAgentImp::GetHeightOfPickPointAboveGirderTop() const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   return pSpec->GetPickPointHeight();
}

Float64 CSpecAgentImp::GetLiftingLoopPlacementTolerance() const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   return pSpec->GetLiftingLoopTolerance();
}

Float64 CSpecAgentImp::GetLiftingCableMinInclination() const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   return pSpec->GetMinCableInclination();
}

Float64 CSpecAgentImp::GetLiftingSweepTolerance()const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   return pSpec->GetLiftingMaximumGirderSweepTolerance();
}

Float64 CSpecAgentImp::GetLiftingModulusOfRupture(const CSegmentKey& segmentKey)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterials);
   Float64 fci = pMaterials->GetSegmentFc(segmentKey,intervalIdx);
   pgsTypes::ConcreteType type = pMaterials->GetSegmentConcreteType(segmentKey);

   return GetLiftingModulusOfRupture(fci,type);
}

Float64 CSpecAgentImp::GetLiftingModulusOfRupture(Float64 fci,pgsTypes::ConcreteType concType)
{
   Float64 x = GetLiftingModulusOfRuptureFactor(concType);
   return x*sqrt(fci);
}

Float64 CSpecAgentImp::GetLiftingModulusOfRuptureFactor(pgsTypes::ConcreteType concType)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingModulusOfRuptureFactor(concType);
}

Float64 CSpecAgentImp::GetMinimumLiftingPointLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 min_lift_point = pSpec->GetMininumLiftingPointLocation();

   // if less than zero, then use H from the end of the girder
   if ( min_lift_point < 0 )
   {
      GET_IFACE(IBridge,pBridge);
      pgsPointOfInterest poi(segmentKey,0.0);

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      if ( end == pgsTypes::metEnd )
      {
         poi.SetDistFromStart( pBridge->GetSegmentLength(segmentKey) );
      }
      GET_IFACE(ISectionProperties,pSectProp);
      min_lift_point = pSectProp->GetHg( releaseIntervalIdx, poi );
   }

   return min_lift_point;
}

Float64 CSpecAgentImp::GetLiftingPointLocationAccuracy() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingPointLocationAccuracy();
}

//////////////////////////////////////////////////////////////////////
// IGirderHaulingSpecCriteria
bool CSpecAgentImp::IsHaulingAnalysisEnabled() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->IsHaulingAnalysisEnabled();
}

pgsTypes::HaulingAnalysisMethod CSpecAgentImp::GetHaulingAnalysisMethod() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingAnalysisMethod();
}

void CSpecAgentImp::GetHaulingImpact(Float64* pDownward, Float64* pUpward) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *pDownward = pSpec->GetHaulingDownwardImpactFactor();
   *pUpward   = pSpec->GetHaulingUpwardImpactFactor();
}

Float64 CSpecAgentImp::GetHaulingCrackingFs() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingCrackingFOS();
}

Float64 CSpecAgentImp::GetHaulingRolloverFs() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingFailureFOS();
}

void CSpecAgentImp::GetHaulingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *factor = GetHaulingAllowableTensionFactor();
   pSpec->GetHaulingMaximumTensionStress(pbMax,fmax);
}

Float64 CSpecAgentImp::GetHaulingAllowableTensileConcreteStress(const CSegmentKey& segmentKey)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = GetHaulingAllowableTensionFactor();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fc = pMaterial->GetSegmentFc(segmentKey,haulSegmentIntervalIdx);

   Float64 allow = factor * sqrt( fc );

   bool is_max;
   Float64 maxval;
   pSpec->GetHaulingMaximumTensionStress(&is_max,&maxval);
   if (is_max)
      allow = Min(allow, maxval);

   return allow;
}

Float64 CSpecAgentImp::GetHaulingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey)
{
   Float64 factor = GetHaulingAllowableCompressionFactor();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fc = pMaterial->GetSegmentFc(segmentKey,haulSegmentIntervalIdx);

   Float64 allowable = factor * fc;
   return allowable;
}

Float64 CSpecAgentImp::GetHaulingAllowableTensionFactor()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = pSpec->GetHaulingTensionStressFactor();
   return factor;
}

Float64 CSpecAgentImp::GetHaulingAllowableCompressionFactor()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = pSpec->GetHaulingCompressionStressFactor();
   return -factor;
}

Float64 CSpecAgentImp::GetHaulingAllowableTensileConcreteStressEx(Float64 fc, bool includeRebar)
{
   if (includeRebar)
   {
      Float64 x = GetHaulingWithMildRebarAllowableStressFactor();

      Float64 f = x * sqrt( fc );
      return f;
   }
   else
   {
      Float64 x; 
      bool bCheckMax;
      Float64 fmax;

      GetHaulingAllowableTensileConcreteStressParameters(&x,&bCheckMax,&fmax);

      Float64 f = x * sqrt( fc );

      if ( bCheckMax )
         f = Min(f,fmax);

      return f;
   }
}

Float64 CSpecAgentImp::GetHaulingAllowableCompressiveConcreteStressEx(Float64 fc)
{
   Float64 x = GetHaulingAllowableCompressionFactor();

   return x*fc;
}

IGirderHaulingSpecCriteria::RollStiffnessMethod CSpecAgentImp::GetRollStiffnessMethod() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return (pSpec->GetTruckRollStiffnessMethod() == ROLLSTIFFNESS_LUMPSUM ? IGirderHaulingSpecCriteria::LumpSum : IGirderHaulingSpecCriteria::PerAxle );
}

Float64 CSpecAgentImp::GetLumpSumRollStiffness() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetTruckRollStiffness();
}

Float64 CSpecAgentImp::GetAxleWeightLimit() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetAxleWeightLimit();
}

Float64 CSpecAgentImp::GetAxleStiffness() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetAxleStiffness();
}

Float64 CSpecAgentImp::GetMinimumRollStiffness() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetMinRollStiffness();
}

Float64 CSpecAgentImp::GetHeightOfGirderBottomAboveRoadway() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetTruckGirderHeight();
}

Float64 CSpecAgentImp::GetHeightOfTruckRollCenterAboveRoadway() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetTruckRollCenterHeight();
}

Float64 CSpecAgentImp::GetAxleWidth() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetTruckAxleWidth();
}

Float64 CSpecAgentImp::GetMaxSuperelevation() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetRoadwaySuperelevation();
}

Float64 CSpecAgentImp::GetHaulingSweepTolerance()const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingMaximumGirderSweepTolerance();
}

Float64 CSpecAgentImp::GetHaulingSupportPlacementTolerance() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingSupportPlacementTolerance();
}

Float64 CSpecAgentImp::GetIncreaseInCgForCamber() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingCamberPercentEstimate()/100.;
}

Float64 CSpecAgentImp::GetAllowableDistanceBetweenSupports() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingSupportDistance();
}

Float64 CSpecAgentImp::GetAllowableLeadingOverhang() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingMaximumLeadingOverhang();
}

Float64 CSpecAgentImp::GetMaxGirderWgt() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetMaxGirderWeight();
}

Float64 CSpecAgentImp::GetMinimumHaulingSupportLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 min_pick_point = pSpec->GetMininumTruckSupportLocation();

   // if less than zero, then use H from the end of the girder
   if ( min_pick_point < 0 )
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      GET_IFACE(IBridge,pBridge);
      pgsPointOfInterest poi(segmentKey,0.0);
      if ( end == pgsTypes::metEnd )
      {
         poi.SetDistFromStart( pBridge->GetSegmentLength(segmentKey) );
      }
      GET_IFACE(ISectionProperties,pSectProp);
      min_pick_point = pSectProp->GetHg( releaseIntervalIdx, poi );
   }

   return min_pick_point;
}

Float64 CSpecAgentImp::GetHaulingSupportLocationAccuracy() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetTruckSupportLocationAccuracy();
}

/////////////////////////////////////////////////////////////////////
//  IKdotGirderLiftingSpecCriteria
// Spec criteria for KDOT analyses
Float64 CSpecAgentImp::GetKdotHaulingAllowableTensileConcreteStress(const CSegmentKey& segmentKey)
{
   return GetHaulingAllowableTensileConcreteStress(segmentKey);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey)
{
   return GetHaulingAllowableCompressiveConcreteStress(segmentKey);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableTensionFactor()
{
   return GetHaulingAllowableTensionFactor();
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableCompressionFactor()
{
   return GetHaulingAllowableCompressionFactor();
}

Float64 CSpecAgentImp::GetKdotHaulingWithMildRebarAllowableStress(const CSegmentKey& segmentKey)

{
   return GetHaulingWithMildRebarAllowableStress(segmentKey);
}

Float64 CSpecAgentImp::GetKdotHaulingWithMildRebarAllowableStressFactor()
{
   return GetHaulingWithMildRebarAllowableStressFactor();
}

void CSpecAgentImp::GetKdotHaulingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax)
{
   GetHaulingAllowableTensileConcreteStressParameters(factor, pbMax, fmax);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableTensileConcreteStressEx(Float64 fc, bool includeRebar)
{
   return GetHaulingAllowableTensileConcreteStressEx(fc, includeRebar);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableCompressiveConcreteStressEx(Float64 fc)
{
   return GetHaulingAllowableCompressiveConcreteStressEx(fc);
}

void CSpecAgentImp::GetMinimumHaulingSupportLocation(Float64* pHardDistance, bool* pUseFactoredLength, Float64* pLengthFactor) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *pHardDistance = pSpec->GetMininumTruckSupportLocation();
   *pUseFactoredLength = pSpec->GetUseMinTruckSupportLocationFactor();
   *pLengthFactor = pSpec->GetMinTruckSupportLocationFactor();
}

Float64 CSpecAgentImp::GetHaulingDesignLocationAccuracy() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetTruckSupportLocationAccuracy();
}

void CSpecAgentImp::GetHaulingGFactors(Float64* pOverhangFactor, Float64* pInteriorFactor) const
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
   std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_MIDSPAN) );
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
   switch (rebarGrade )
   {
   case matRebar::Grade40:
      *pecl = 0.0014;
      *petl = 0.005;
      break;

   case matRebar::Grade60:
      *pecl = 0.002;
      *petl = 0.005;
      break;

   case matRebar::Grade75:
      *pecl = 0.0028;
      *petl = 0.0050;
      break;

   case matRebar::Grade80:
      *pecl = 0.0030;
      *petl = 0.0056;
      break;

   case matRebar::Grade100:
      *pecl = 0.0040;
      *petl = 0.0080;
      break;

   default:
      ATLASSERT(false); // new rebar grade?
   }
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

////////////////////
// Private methods

const SpecLibraryEntry* CSpecAgentImp::GetSpec() const
{
   GET_IFACE( ISpecification, pSpec );
   GET_IFACE( ILibrary,       pLib );

   std::_tstring specName = pSpec->GetSpecification();
   return pLib->GetSpecEntry( specName.c_str() );
}

const GirderLibraryEntry* CSpecAgentImp::GetGirderEntry(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   return pGirderEntry;
}

bool CSpecAgentImp::IsLoadRatingLimitState(pgsTypes::LimitState ls)
{
   return ( ls == pgsTypes::ServiceIII_Inventory ||
            ls == pgsTypes::ServiceIII_Operating ||
            ls == pgsTypes::ServiceIII_LegalRoutine ||
            ls == pgsTypes::ServiceIII_LegalSpecial ) ? true : false;
}

Float64 CSpecAgentImp::GetLoadRatingAllowableTension(const CSegmentKey& segmentKey,pgsTypes::LimitState ls)
{
   GET_IFACE(IRatingSpecification,pRatingSpec);
   if ( ls == pgsTypes::ServiceIII_Inventory )
      return pRatingSpec->GetAllowableTension(pgsTypes::lrDesign_Inventory,segmentKey);

   if ( ls == pgsTypes::ServiceIII_Operating )
      return pRatingSpec->GetAllowableTension(pgsTypes::lrDesign_Operating,segmentKey);

   if ( ls == pgsTypes::ServiceIII_LegalRoutine )
      return pRatingSpec->GetAllowableTension(pgsTypes::lrLegal_Routine,segmentKey);

   if ( ls == pgsTypes::ServiceIII_LegalSpecial )
      return pRatingSpec->GetAllowableTension(pgsTypes::lrLegal_Special,segmentKey);

   ATLASSERT(false); // should never get here
   return -1;
}
