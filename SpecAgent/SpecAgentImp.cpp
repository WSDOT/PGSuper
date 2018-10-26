///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#include <PgsExt\BridgeDescription.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <Lrfd\PsStrand.h>
#include <Lrfd\Rebar.h>

#include <IFace\StatusCenter.h>
#include <IFace\PrestressForce.h>
#include <IFace\RatingSpecification.h>

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

   pBrokerInit->RegInterface( IID_IAllowableStrandStress,   this );
   pBrokerInit->RegInterface( IID_IAllowableConcreteStress, this );
   pBrokerInit->RegInterface( IID_ITransverseReinforcementSpec, this );
   pBrokerInit->RegInterface( IID_IPrecastIGirderDetailsSpec, this );
   pBrokerInit->RegInterface( IID_IGirderLiftingSpecCriteria, this );
   pBrokerInit->RegInterface( IID_IGirderLiftingSpecCriteriaEx, this );
   pBrokerInit->RegInterface( IID_IGirderHaulingSpecCriteria, this );
   pBrokerInit->RegInterface( IID_IGirderHaulingSpecCriteriaEx, this );
   pBrokerInit->RegInterface( IID_IDebondLimits, this );
   pBrokerInit->RegInterface( IID_IResistanceFactors, this );

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
// IAgent
//
bool CSpecAgentImp::CheckStressAtJacking()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckStrandStress(AT_JACKING);
   //return ( pSpec->GetSpecificationType() == lrfdVersionMgr::FirstEdition ? true : false );
}

bool CSpecAgentImp::CheckStressBeforeXfer()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckStrandStress(BEFORE_TRANSFER);
//   return ( pSpec->GetSpecificationType() == lrfdVersionMgr::FirstEdition ? false : true );
}

bool CSpecAgentImp::CheckStressAfterXfer()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckStrandStress(AFTER_TRANSFER);
//   return ( pSpec->GetSpecificationType() == lrfdVersionMgr::FirstEdition ? true : false );
}

bool CSpecAgentImp::CheckStressAfterLosses()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckStrandStress(AFTER_ALL_LOSSES);
//   return true;
}

Float64 CSpecAgentImp::GetAllowableAtJacking(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType)
{
   if ( !CheckStressAtJacking() )
      return 0.0;

   GET_IFACE(IBridgeMaterial,pMat);
   const matPsStrand* pStrand = pMat->GetStrand(span,gdr,strandType);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetStrandStressCoefficient(AT_JACKING,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpu;

//   return lrfdPsStrand::GetStressLimit( pStrand->GetGrade(), pStrand->GetType(), lrfdPsStrand::Jacking );
}

Float64 CSpecAgentImp::GetAllowableBeforeXfer(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType)
{
   if ( !CheckStressBeforeXfer() )
      return 0.0;

   GET_IFACE(IBridgeMaterial,pMat);
   const matPsStrand* pStrand = pMat->GetStrand(span,gdr,strandType);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetStrandStressCoefficient(BEFORE_TRANSFER,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpu;

//   return lrfdPsStrand::GetStressLimit( pStrand->GetGrade(), pStrand->GetType(), lrfdPsStrand::BeforeTransfer );
}

Float64 CSpecAgentImp::GetAllowableAfterXfer(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType)
{
   if ( !CheckStressAfterXfer() )
      return 0.0;

   GET_IFACE(IBridgeMaterial,pMat);
   const matPsStrand* pStrand = pMat->GetStrand(span,gdr,strandType);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetStrandStressCoefficient(AFTER_TRANSFER,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpu;

//   return lrfdPsStrand::GetStressLimit( pStrand->GetGrade(), pStrand->GetType(), lrfdPsStrand::AfterTransfer );
}

Float64 CSpecAgentImp::GetAllowableAfterLosses(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType)
{
   if ( !CheckStressAfterLosses() )
      return 0.0;

   GET_IFACE(IBridgeMaterial,pMat);
   const matPsStrand* pStrand = pMat->GetStrand(span,gdr,strandType);

   Float64 fpy = lrfdPsStrand::GetYieldStrength(pStrand->GetGrade(),pStrand->GetType());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetStrandStressCoefficient(AFTER_ALL_LOSSES,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpy;

//   return lrfdPsStrand::GetStressLimit( pStrand->GetGrade(), pStrand->GetType(), lrfdPsStrand::AfterAllLosses );
}

const SpecLibraryEntry* CSpecAgentImp::GetSpec() const
{
   GET_IFACE( ISpecification, pSpec );
   GET_IFACE( ILibrary,       pLib );

   std::_tstring specName = pSpec->GetSpecification();
   return pLib->GetSpecEntry( specName.c_str() );
}

const GirderLibraryEntry* CSpecAgentImp::GetGirderEntry(SpanIndexType spanIdx,GirderIndexType gdrIdx) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const GirderLibraryEntry* pGirderEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
   return pGirderEntry;
}


/////////////////////////////////////////////////////////////////////////////
// IAllowableConcreteStress
//
Float64 CSpecAgentImp::GetAllowableStress(const pgsPointOfInterest& poi, pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType type)
{
   SpanIndexType spanIdx = poi.GetSpan();
   GirderIndexType gdrIdx = poi.GetGirder();

   if ( stage == pgsTypes::BridgeSite3 && ( ls == pgsTypes::ServiceIII_Inventory ||
                                            ls == pgsTypes::ServiceIII_Operating ||
                                            ls == pgsTypes::ServiceIII_LegalRoutine ||
                                            ls == pgsTypes::ServiceIII_LegalSpecial ) )
   {
      GET_IFACE(IRatingSpecification,pRatingSpec);
      if ( ls == pgsTypes::ServiceIII_Inventory )
         return pRatingSpec->GetAllowableTension(pgsTypes::lrDesign_Inventory,spanIdx,gdrIdx);

      if ( ls == pgsTypes::ServiceIII_Operating )
         return pRatingSpec->GetAllowableTension(pgsTypes::lrDesign_Operating,spanIdx,gdrIdx);

      if ( ls == pgsTypes::ServiceIII_LegalRoutine )
         return pRatingSpec->GetAllowableTension(pgsTypes::lrLegal_Routine,spanIdx,gdrIdx);

      if ( ls == pgsTypes::ServiceIII_LegalSpecial )
         return pRatingSpec->GetAllowableTension(pgsTypes::lrLegal_Special,spanIdx,gdrIdx);

      ATLASSERT(false); // should never get here
      return -1;
   }
   else
   {
      GET_IFACE(IBridgeMaterial,pMat);
      Float64 fc;
      
      switch( stage )
      {
      case pgsTypes::CastingYard:
         fc = pMat->GetFciGdr(spanIdx,gdrIdx);
         break;

   //   case pgsTypes::GirderPlacement:
      case pgsTypes::TemporaryStrandRemoval:
      case pgsTypes::BridgeSite1:
      case pgsTypes::BridgeSite2:
      case pgsTypes::BridgeSite3:
         fc = pMat->GetFcGdr(spanIdx,gdrIdx);
         break;

      default:
         ATLASSERT(false);// should never get here
      }

      return GetAllowableStress(stage,ls,type,fc);
   }
}

std::vector<Float64> CSpecAgentImp::GetAllowableStress(const std::vector<pgsPointOfInterest>& vPoi, pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType type)
{
   std::vector<Float64> vStress;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      vStress.push_back( GetAllowableStress(poi,stage,ls,type));
   }

   return vStress;
}

Float64 CSpecAgentImp::GetAllowableStress(pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType type,Float64 fc)
{
   Float64 fAllow;

   switch( stage )
   {
   case pgsTypes::CastingYard:
      fAllow = GetCastingYardAllowableStress(ls,type,fc);
      break;

//   case pgsTypes::GirderPlacement:
   case pgsTypes::TemporaryStrandRemoval:
   case pgsTypes::BridgeSite1:
   case pgsTypes::BridgeSite2:
   case pgsTypes::BridgeSite3:
      fAllow = GetBridgeSiteAllowableStress(stage,ls,type,fc);
      break;

   default:
      ATLASSERT(false);// should never get here
   }

   return fAllow;
}

Float64 CSpecAgentImp::GetCastingYardAllowableStress(pgsTypes::LimitState ls,pgsTypes::StressType type,Float64 fc)
{
   // limit state must be Service I
   PRECONDITION( ls == pgsTypes::ServiceI );

   Float64 fAllow;

   switch( type )
   {
   case pgsTypes::Tension:
      fAllow = GetInitialAllowableTensileStress(fc,false);
      break;

   case pgsTypes::Compression:
      fAllow = GetInitialAllowableCompressiveStress(fc);
      break;
   }

   return fAllow;
}

Float64 CSpecAgentImp::GetCastingYardWithMildRebarAllowableStress(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(IBridgeMaterial,pMat);

   Float64 fci = pMat->GetFciGdr(span,gdr);

   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 x = pSpec->GetCyMaxConcreteTensWithRebar();

   return x*sqrt(fci);
}

Float64 CSpecAgentImp::GetLiftingWithMildRebarAllowableStressFactor()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = pSpec->GetMaxConcreteTensWithRebarLifting();
   return x;
}

Float64 CSpecAgentImp::GetLiftingWithMildRebarAllowableStress(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(IBridgeMaterial,pMat);

   Float64 fci = pMat->GetFciGdr(span,gdr);

   Float64 x = GetLiftingWithMildRebarAllowableStressFactor();

   return x*sqrt(fci);
}

Float64 CSpecAgentImp::GetHaulingWithMildRebarAllowableStressFactor()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = pSpec->GetMaxConcreteTensWithRebarHauling();
   return x;
}

Float64 CSpecAgentImp::GetHaulingWithMildRebarAllowableStress(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(IBridgeMaterial,pMat);

   Float64 fc = pMat->GetFcGdr(span,gdr);

   Float64 x = GetHaulingWithMildRebarAllowableStressFactor();

   return x*sqrt(fc);
}

Float64 CSpecAgentImp::GetHaulingModulusOfRuptureCoefficient()
{
   ATLASSERT(false); // obsolete
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingModulusOfRuptureCoefficient(pgsTypes::Normal);
}

Float64 CSpecAgentImp::GetHaulingModulusOfRupture(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(IBridgeMaterialEx,pMat);

   Float64 fc = pMat->GetFcGdr(span,gdr);
   pgsTypes::ConcreteType type = pMat->GetGdrConcreteType(span,gdr);

   return GetHaulingModulusOfRupture(fc,type);
}

Float64 CSpecAgentImp::GetHaulingModulusOfRupture(Float64 fc)
{
   ATLASSERT(false); // obsolete
   double x = GetHaulingModulusOfRuptureCoefficient();
   return x*sqrt(fc);
}

Float64 CSpecAgentImp::GetBridgeSiteAllowableStress(pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType type,Float64 fc)
{
   Float64 fAllow;

   switch( type )
   {
   case pgsTypes::Tension:
      fAllow = GetFinalAllowableTensileStress(stage, fc);
      break;

   case pgsTypes::Compression:
      fAllow = GetFinalAllowableCompressiveStress(stage,ls,fc);
      break;
   }

   return fAllow;
}

Float64 CSpecAgentImp::GetAllowableCompressiveStressCoefficient(pgsTypes::Stage stage,pgsTypes::LimitState ls)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   double x = -999999;

   switch( stage )
   {
   case pgsTypes::CastingYard:
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetCyCompStressService();
      break;

//   case pgsTypes::GirderPlacement:
   case pgsTypes::TemporaryStrandRemoval:
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetTempStrandRemovalCompStress();
      break;

   case pgsTypes::BridgeSite1:
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetBs1CompStress();
      break;

   case pgsTypes::BridgeSite2:
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetBs2CompStress();
      break;

   case pgsTypes::BridgeSite3:
      ATLASSERT( (ls == pgsTypes::ServiceI) || (ls == pgsTypes::ServiceIA) || (ls == pgsTypes::FatigueI));
      x = (ls == pgsTypes::ServiceI ? pSpec->GetBs3CompStressService() : pSpec->GetBs3CompStressService1A());
      break;

   case pgsTypes::Lifting:
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetCyCompStressLifting();
      break;

   case pgsTypes::Hauling:
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetHaulingCompStress();
      break;

   default:
      ATLASSERT(false);
   }

   return x;
}

void CSpecAgentImp::GetAllowableTensionStressCoefficient(pgsTypes::Stage stage,pgsTypes::LimitState ls,double* pCoeff,bool* pbMax,double* pMaxValue)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   double x = -999999;
   bool bCheckMax;
   double fmax;

   GET_IFACE(IEnvironment,pEnv);

   switch( stage )
   {
   case pgsTypes::CastingYard:
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetCyMaxConcreteTens();
      pSpec->GetCyAbsMaxConcreteTens(&bCheckMax,&fmax);
      break;

//   case pgsTypes::GirderPlacement:
   case pgsTypes::TemporaryStrandRemoval:
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetTempStrandRemovalMaxConcreteTens();
      pSpec->GetTempStrandRemovalAbsMaxConcreteTens(&bCheckMax,&fmax);
      break;

   case pgsTypes::BridgeSite1:
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetBs1MaxConcreteTens();
      pSpec->GetBs1AbsMaxConcreteTens(&bCheckMax,&fmax);
      break;

   case pgsTypes::BridgeSite2:
      ATLASSERT( false ); // should never get here!!!
      break;

   case pgsTypes::BridgeSite3:
      ATLASSERT( (ls == pgsTypes::ServiceIII)  );
      if ( pEnv->GetExposureCondition() == expNormal )
      {
         x = pSpec->GetBs3MaxConcreteTensNc();
         pSpec->GetBs3AbsMaxConcreteTensNc(&bCheckMax,&fmax);
      }
      else
      {
         x = pSpec->GetBs3MaxConcreteTensSc();
         pSpec->GetBs3AbsMaxConcreteTensSc(&bCheckMax,&fmax);
      }
      break;

   case pgsTypes::Lifting:
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetCyMaxConcreteTensLifting();
      pSpec->GetCyAbsMaxConcreteTensLifting(&bCheckMax,&fmax);
      break;

   case pgsTypes::Hauling:
      ATLASSERT( ls == pgsTypes::ServiceI );
      x = pSpec->GetMaxConcreteTensHauling();
      pSpec->GetAbsMaxConcreteTensHauling(&bCheckMax,&fmax);
      break;


   default:
      ATLASSERT(false);
   }

   *pCoeff    = x;
   *pbMax     = bCheckMax;
   *pMaxValue = fmax;
}

Float64 CSpecAgentImp::GetInitialAllowableCompressiveStress(Float64 fci)
{
   double x = GetAllowableCompressiveStressCoefficient(pgsTypes::CastingYard,pgsTypes::ServiceI);

   // Add a minus sign because compression is negative
   return -x*fci;
}

Float64 CSpecAgentImp::GetInitialAllowableTensileStress(Float64 fci, bool withMinRebar)
{
   if (withMinRebar)
   {
      Float64 x = GetCastingYardAllowableTensionStressCoefficientWithRebar();

      Float64 f = x * sqrt( fci );
      return f;
   }
   else
   {
      Float64 x; 
      bool bCheckMax;
      Float64 fmax;

      GetAllowableTensionStressCoefficient(pgsTypes::CastingYard,pgsTypes::ServiceI,&x,&bCheckMax,&fmax);

      Float64 f = x * sqrt( fci );

      if ( bCheckMax )
         f = min(f,fmax);

      return f;
   }
}

Float64 CSpecAgentImp::GetFinalAllowableCompressiveStress(pgsTypes::Stage stage,pgsTypes::LimitState ls,Float64 fc)
{
   double x = GetAllowableCompressiveStressCoefficient(stage,ls);
   // Add a minus sign because compression is negative
   return -x*fc;
}

Float64 CSpecAgentImp::GetFinalAllowableTensileStress(pgsTypes::Stage stage, Float64 fc)
{
   GET_IFACE(IEnvironment,pEnv);

   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 x;
   bool bCheckMax;
   Float64 fmax; // In system units
   pgsTypes::LimitState ls = (stage == pgsTypes::BridgeSite3 ? pgsTypes::ServiceIII : pgsTypes::ServiceI);
   GetAllowableTensionStressCoefficient(stage,ls,&x,&bCheckMax,&fmax);

   Float64 f = x * sqrt( fc );
   if ( bCheckMax )
      f = min(f,fmax);

   return f;
}

Float64 CSpecAgentImp::GetCastingYardAllowableTensionStressCoefficientWithRebar()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetCyMaxConcreteTensWithRebar();
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

Float64 CSpecAgentImp::GetAvOverSMin(Float64 fc, Float64 bv, Float64 fy)
{
   ATLASSERT(false); // this method is obsolete and should not be used
   // Lightweight concrete makes this method obsolete
   return lrfdRebar::GetAvOverSMin(fc,bv,fy);
}

void CSpecAgentImp::GetMaxStirrupSpacing(Float64* sUnderLimit, Float64* sOverLimit)
{
   lrfdRebar::GetMaxStirrupSpacing(sUnderLimit, sOverLimit);

   // check to see if this has been overridden by spec library entry.
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 max_spac = pSpec->GetMaxStirrupSpacing();

   *sUnderLimit = min(*sUnderLimit, max_spac);
   *sOverLimit = min(*sOverLimit, max_spac);
}

Float64 CSpecAgentImp::GetMinStirrupSpacing(Float64 maxAggregateSize, Float64 barDiameter)
{
   CHECK(maxAggregateSize>0.0);
   CHECK(barDiameter>0.0);

   Float64 min_spc = max(1.33*maxAggregateSize, barDiameter);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 abs_min_spc;
   if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
      abs_min_spc = ::ConvertToSysUnits(25., unitMeasure::Millimeter);
   else
      abs_min_spc = ::ConvertToSysUnits(1., unitMeasure::Inch);

   // lrfd requirements are for clear distance, we want cl-to-cl spacing
   min_spc += barDiameter;
   abs_min_spc += barDiameter;

   return max(min_spc, abs_min_spc);
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

   Float64 dim;
   if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
      dim = ::ConvertToSysUnits(125., unitMeasure::Millimeter);
   else
      dim = ::ConvertToSysUnits(5., unitMeasure::Inch);

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
bool CSpecAgentImp::IsLiftingCheckEnabled() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->IsLiftingCheckEnabled();
}

void  CSpecAgentImp::GetLiftingImpact(Float64* pDownward, Float64* pUpward) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *pDownward = pSpec->GetCyLiftingDownwardImpact();
   *pUpward   = pSpec->GetCyLiftingUpwardImpact();
}

Float64 CSpecAgentImp::GetLiftingCrackingFs() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetCyLiftingCrackFs();
}

Float64 CSpecAgentImp::GetLiftingFailureFs() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetCyLiftingFailFs();
}

void CSpecAgentImp::GetLiftingAllowableTensileConcreteStressParameters(double* factor,bool* pbMax,double* fmax)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *factor = GetLiftingAllowableTensionFactor();
   pSpec->GetCyAbsMaxConcreteTensLifting(pbMax,fmax);
}

Float64 CSpecAgentImp::GetLiftingAllowableTensileConcreteStress(SpanIndexType span,GirderIndexType gdr)
{
   Float64 factor = GetLiftingAllowableTensionFactor();
   GET_IFACE(IBridgeMaterial,pMat);
   Float64 fci = pMat->GetFciGdr(span,gdr);

   Float64 f = factor * sqrt( fci );

   bool is_max;
   Float64 maxval;
   const SpecLibraryEntry* pSpec = GetSpec();

   pSpec->GetCyAbsMaxConcreteTensLifting(&is_max,&maxval);
   if (is_max)
      f = min(f, maxval);

   return f;
}

Float64 CSpecAgentImp::GetLiftingAllowableTensionFactor()
{
   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 factor = pSpec->GetCyMaxConcreteTensLifting();
   return factor;
}

Float64 CSpecAgentImp::GetLiftingAllowableCompressiveConcreteStress(SpanIndexType span,GirderIndexType gdr)
{
   Float64 factor = GetLiftingAllowableCompressionFactor();
   GET_IFACE(IBridgeMaterial,pMat);
   Float64 fci = pMat->GetFciGdr(span,gdr);
   Float64 all = factor * fci;

   return all;
}

Float64 CSpecAgentImp::GetLiftingAllowableCompressionFactor()
{
   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 factor = pSpec->GetCyCompStressLifting();
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
         f = min(f,fmax);

      return f;
   }
}

Float64 CSpecAgentImp::GetLiftingAllowableCompressiveConcreteStressEx(Float64 fci)
{
   double x = GetLiftingAllowableCompressionFactor();

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

   return pSpec->GetMaxGirderSweepLifting();
}

Float64 CSpecAgentImp::GetLiftingModulusOfRuptureCoefficient()
{
   ATLASSERT(false); // obsolete...
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingModulusOfRuptureCoefficient(pgsTypes::Normal);
}

Float64 CSpecAgentImp::GetLiftingModulusOfRupture(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(IBridgeMaterialEx,pMat);

   Float64 fci = pMat->GetFciGdr(span,gdr);
   pgsTypes::ConcreteType type = pMat->GetGdrConcreteType(span,gdr);

   return GetLiftingModulusOfRupture(fci,type);
}

Float64 CSpecAgentImp::GetLiftingModulusOfRupture(Float64 fci)
{
   ATLASSERT(false); // obsolete...
   double x = GetLiftingModulusOfRuptureCoefficient();
   return x*sqrt(fci);
}

Float64 CSpecAgentImp::GetMinimumLiftingPointLocation(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType end) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 min_lift_point = pSpec->GetMininumLiftingPointLocation();

   // if less than zero, then use H from the end of the girder
   if ( min_lift_point < 0 )
   {
      GET_IFACE(IBridge,pBridge);
      pgsPointOfInterest poi(spanIdx,gdrIdx,0.0);
      if ( end == pgsTypes::metEnd )
      {
         poi.SetDistFromStart( pBridge->GetGirderLength(spanIdx,gdrIdx) );
      }
      GET_IFACE(ISectProp2,pSectProp);
      min_lift_point = pSectProp->GetHg( pgsTypes::CastingYard, poi );
   }

   return min_lift_point;
}

Float64 CSpecAgentImp::GetLiftingPointLocationAccuracy() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingPointLocationAccuracy();
}

/////////////////////////////////////////////////////////////////////
//  IGirderLiftingSpecCriteriaEx
Float64 CSpecAgentImp::GetLiftingModulusOfRuptureCoefficient(pgsTypes::ConcreteType concType)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingModulusOfRuptureCoefficient(concType);
}

Float64 CSpecAgentImp::GetLiftingModulusOfRupture(Float64 fci,pgsTypes::ConcreteType concType)
{
   double x = GetLiftingModulusOfRuptureCoefficient(concType);
   return x*sqrt(fci);
}

// IGirderHaulingSpecCriteria
bool CSpecAgentImp::IsHaulingCheckEnabled() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->IsHaulingCheckEnabled();
}

void CSpecAgentImp::GetHaulingImpact(Float64* pDownward, Float64* pUpward) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *pDownward = pSpec->GetHaulingDownwardImpact();
   *pUpward   = pSpec->GetHaulingUpwardImpact();
}

Float64 CSpecAgentImp::GetHaulingCrackingFs() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingCrackFs();
}

Float64 CSpecAgentImp::GetHaulingRolloverFs() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingFailFs();
}

void CSpecAgentImp::GetHaulingAllowableTensileConcreteStressParameters(double* factor,bool* pbMax,double* fmax)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *factor = GetHaulingAllowableTensionFactor();
   pSpec->GetAbsMaxConcreteTensHauling(pbMax,fmax);
}

Float64 CSpecAgentImp::GetHaulingAllowableTensileConcreteStress(SpanIndexType span,GirderIndexType gdr)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = GetHaulingAllowableTensionFactor();
   GET_IFACE(IBridgeMaterial,pMat);
   Float64 fc = pMat->GetFcGdr(span,gdr);

   Float64 allow = factor * sqrt( fc );

   bool is_max;
   Float64 maxval;
   pSpec->GetAbsMaxConcreteTensHauling(&is_max,&maxval);
   if (is_max)
      allow = min(allow, maxval);

   return allow;
}

Float64 CSpecAgentImp::GetHaulingAllowableCompressiveConcreteStress(SpanIndexType span,GirderIndexType gdr)
{
   Float64 factor = GetHaulingAllowableCompressionFactor();
   GET_IFACE(IBridgeMaterial,pMat);
   Float64 fc = pMat->GetFcGdr(span,gdr);
   Float64 allow = factor * fc;
   return allow;
}

Float64 CSpecAgentImp::GetHaulingAllowableTensionFactor()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = pSpec->GetMaxConcreteTensHauling();
   return factor;
}

Float64 CSpecAgentImp::GetHaulingAllowableCompressionFactor()
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = pSpec->GetHaulingCompStress();
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
         f = min(f,fmax);

      return f;
   }
}

Float64 CSpecAgentImp::GetHaulingAllowableCompressiveConcreteStressEx(Float64 fc)
{
   double x = GetHaulingAllowableCompressionFactor();

   return x*fc;
}

IGirderHaulingSpecCriteria::RollStiffnessMethod CSpecAgentImp::GetRollStiffnessMethod() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return (pSpec->GetTruckRollStiffnessMethod() == 0 ? IGirderHaulingSpecCriteria::LumpSum : IGirderHaulingSpecCriteria::PerAxle );
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
   return pSpec->GetMaxGirderSweepHauling();
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
   return pSpec->GetMaxHaulingOverhang();
}

Float64 CSpecAgentImp::GetMaxGirderWgt() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetMaxGirderWeight();
}

Float64 CSpecAgentImp::GetMinimumHaulingSupportLocation(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType end) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 min_pick_point = pSpec->GetMininumTruckSupportLocation();

   // if less than zero, then use H from the end of the girder
   if ( min_pick_point < 0 )
   {
      GET_IFACE(IBridge,pBridge);
      pgsPointOfInterest poi(spanIdx,gdrIdx,0.0);
      if ( end == pgsTypes::metEnd )
      {
         poi.SetDistFromStart( pBridge->GetGirderLength(spanIdx,gdrIdx) );
      }
      GET_IFACE(ISectProp2,pSectProp);
      min_pick_point = pSectProp->GetHg( pgsTypes::CastingYard, poi );
   }

   return min_pick_point;
}

Float64 CSpecAgentImp::GetHaulingSupportLocationAccuracy() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetTruckSupportLocationAccuracy();
}

/////////////////////////////////////////////////////////////////////
//  IGirderLiftingSpecCriteriaEx
Float64 CSpecAgentImp::GetHaulingModulusOfRuptureCoefficient(pgsTypes::ConcreteType concType)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingModulusOfRuptureCoefficient(concType);
}

Float64 CSpecAgentImp::GetHaulingModulusOfRupture(Float64 fc,pgsTypes::ConcreteType concType)
{
   double x = GetHaulingModulusOfRuptureCoefficient(concType);
   return x*sqrt(fc);
}

/////////////////////////////////////////////////////////////////////
// IDebondLimits
Float64 CSpecAgentImp::GetMaxDebondedStrands(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(spanIdx,gdrIdx);

   return pGirderEntry->GetMaxTotalFractionDebondedStrands();
}

Float64 CSpecAgentImp::GetMaxDebondedStrandsPerRow(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(spanIdx,gdrIdx);
   return pGirderEntry->GetMaxFractionDebondedStrandsPerRow();
}

Float64 CSpecAgentImp::GetMaxDebondedStrandsPerSection(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   StrandIndexType nMax;
   double fMax;

   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(spanIdx,gdrIdx);
   pGirderEntry->GetMaxDebondedStrandsPerSection(&nMax,&fMax);

   return fMax;
}

StrandIndexType CSpecAgentImp::GetMaxNumDebondedStrandsPerSection(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   StrandIndexType nMax;
   double fMax;

   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(spanIdx,gdrIdx);
   pGirderEntry->GetMaxDebondedStrandsPerSection(&nMax,&fMax);

   return nMax;
}

void CSpecAgentImp::GetMaxDebondLength(SpanIndexType span, GirderIndexType gdr, Float64* pLen, pgsTypes::DebondLengthControl* pControl)
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(span,gdr);

   bool bSpanFraction, buseHard;
   Float64 spanFraction, hardDistance;
   pGirderEntry->GetMaxDebondedLength(&bSpanFraction, &spanFraction, &buseHard, &hardDistance);

   GET_IFACE(IBridge,pBridge);

   Float64 gdrlength = pBridge->GetGirderLength(span, gdr);

   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_MIDSPAN);
   pgsPointOfInterest poi = vPOI[0];

   // always use half girder length - development length
   GET_IFACE(IPrestressForce, pPrestressForce ); 
   Float64 dev_len = pPrestressForce->GetDevLength(poi,true); // set debonding to true to get max length

   Float64 min_len = gdrlength/2.0 - dev_len;
   *pControl = pgsTypes::mdbDefault;

   if (bSpanFraction)
   {
      Float64 sflen = gdrlength * spanFraction;
      min_len = min(min_len, sflen);
      *pControl = pgsTypes::mbdFractional;
   }

   if (buseHard)
   {
      min_len = min(min_len, hardDistance);
      *pControl = pgsTypes::mdbHardLength;
   }

   *pLen = min_len>0.0 ? min_len : 0.0; // don't return less than zero
}

Float64 CSpecAgentImp::GetMinDebondSectionDistance(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(spanIdx,gdrIdx);
   return pGirderEntry->GetMinDebondSectionLength();
}

/////////////////////////////////////////////////////////////////////
// IResistanceFactors
void CSpecAgentImp::GetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64* phiTensionPS,Float64* phiTensionRC,Float64* phiCompression)
{
   const SpecLibraryEntry* pSpec = GetSpec();
   pSpec->GetFlexureResistanceFactors(type,phiTensionPS,phiTensionRC,phiCompression);
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
      *pecl = 0.0026;
      *petl = 0.0054;
      break;

   case matRebar::Grade80:
      *pecl = 0.0028;
      *petl = 0.0056;
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
