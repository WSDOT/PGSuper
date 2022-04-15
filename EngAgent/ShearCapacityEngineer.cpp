///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include "StdAfx.h"
#include "ShearCapacityEngineer.h"
#include <Units\SysUnits.h>
#include <PGSuperException.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\ShearCapacity.h>
#include <IFace\PrestressForce.h> 
#include <IFace\MomentCapacity.h>
#include <IFace\StatusCenter.h>
#include <IFace\ResistanceFactors.h>
#include <IFace\EditByUI.h>
#include <IFace\Intervals.h>
#include <IFace\Allowables.h>
#include <Lrfd\Rebar.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\statusitem.h>
#include <PgsExt\DesignConfigUtil.h>
#include <PgsExt\GirderLabel.h>

#if defined _DEBUG
#include <IFace\DocumentType.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   pgsShearCapacityEngineer
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsShearCapacityEngineer::pgsShearCapacityEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;
}

pgsShearCapacityEngineer::pgsShearCapacityEngineer(const pgsShearCapacityEngineer& rOther)
{
   MakeCopy(rOther);
}

pgsShearCapacityEngineer::~pgsShearCapacityEngineer()
{
}

//======================== OPERATORS  =======================================
pgsShearCapacityEngineer& pgsShearCapacityEngineer::operator= (const pgsShearCapacityEngineer& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void pgsShearCapacityEngineer::SetBroker(IBroker* pBroker)
{
   m_pBroker = pBroker;
}

void pgsShearCapacityEngineer::SetStatusGroupID(StatusGroupIDType statusGroupID)
{
   m_StatusGroupID = statusGroupID;

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   m_scidGirderDescriptionError   = pStatusCenter->RegisterCallback(new pgsGirderDescriptionStatusCallback(m_pBroker,eafTypes::statusError) );

}

void pgsShearCapacityEngineer::ComputeShearCapacity(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, SHEARCAPACITYDETAILS* pscd) const
{
   // get known information
   VERIFY(GetInformation(intervalIdx, limitState, poi, pConfig, pscd));
   ComputeShearCapacityDetails(intervalIdx,limitState,poi,pscd);
}

void pgsShearCapacityEngineer::ComputeShearCapacityDetails(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState, const pgsPointOfInterest& poi, SHEARCAPACITYDETAILS* pscd) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   CGirderKey girderKey(segmentKey);

#if defined _DEBUG
   // there is a built-in assumption that shear capacity is being computed at the strength limit state
   // which occurs after the bridge is open to traffic.
   //
   // this assumption has an impact on effective shear width (bv) when there are ducts/tendons 
   // in spliced girder bridges.
   //
   // since LRFD 9th Edition, bv, Vc, and Vs depend on if ducts are grouted or ungrouted and since
   // we assume the bridge is open to traffic are also assume ducts are grouted
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   ATLASSERT( liveLoadIntervalIdx <= intervalIdx );
#endif

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( lrfdVersionMgr::SecondEditionWith2000Interims <= pSpecEntry->GetSpecificationType() ? true : false );

   pgsTypes::ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   // some lrfd-related values
   GET_IFACE_NOCHECK(IMaterials,pMaterial);

   // Strands
   if ( bAfter1999 )
   {
      // ok to use Straight since we just want material properties
      const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Straight);

      //GET_IFACE(IPretensionForce,pPSForce);
      //Float64 xfer = pPSForce->GetTransferLengthAdjustment(poi);
      // Should we be using development length adjustment because this is an ultimate condition?
      // Logic says we should, but we never have and industry published examples don't seem to 
      // indicate that we need to.
      Float64 xfer = 1.0;

      if (0 < pscd->Aps)
      {
         Float64 fpu = pStrand->GetUltimateStrength();
         Float64 K = 0.70;
         pscd->fpops = xfer*K*fpu;
      }
      else
      {
         pscd->fpops = 0; // no prestressing on this side
      }
   }
   else
   {
      // C5.7.3.4.2-1 (pre2017: C5.8.3.4.2-1)
      pscd->fpops = pscd->fpeps + pscd->fpc*pscd->Eps/pscd->Ec;
   }

   // Tendons
   if ( bAfter1999 )
   {
      const matPsStrand* pSegmentTendon = pMaterial->GetSegmentTendonMaterial(segmentKey);
      const matPsStrand* pGirderTendon = pMaterial->GetGirderTendonMaterial(girderKey);

      if (0 < pscd->AptSegment)
      {
         Float64 fpu = pSegmentTendon->GetUltimateStrength();
         Float64 K = 0.70;
         pscd->fpoptSegment = K*fpu;
      }
      else
      {
         pscd->fpoptSegment = 0; // no prestressing on this side
      }

      if (0 < pscd->AptGirder)
      {
         Float64 fpu = pGirderTendon ->GetUltimateStrength();
         Float64 K = 0.70;
         pscd->fpoptGirder = K*fpu;
      }
      else
      {
         pscd->fpoptGirder = 0; // no prestressing on this side
      }
   }
   else
   {
      // C5.7.3.4.2-1 (pre2017: C5.8.3.4.2-1)
      pscd->fpoptSegment = pscd->fpeptSegment + pscd->fpc*pscd->EptSegment / pscd->Ec;
      pscd->fpoptGirder = pscd->fpeptGirder + pscd->fpc*pscd->EptGirder / pscd->Ec;
   }

   // concrete shear capacity
   if (!ComputeVc(poi,pscd))
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);

      std::_tstring msg =  std::_tstring(SEGMENT_LABEL(segmentKey)) + _T(": An error occured while computing shear capacity");
      pgsGirderDescriptionStatusItem* pStatusItem =
            new pgsGirderDescriptionStatusItem(segmentKey,EGD_STIRRUPS,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());

      pStatusCenter->Add(pStatusItem);

      msg += std::_tstring(_T("\nSee Status Center for Details"));
      THROW_UNWIND(msg.c_str(),-1);
   }

   if (pscd->ShearInRange)
   {
     // Vs - if Vc was calc'd successfully
      VERIFY(ComputeVs(poi, pscd));

      // final capacity
      // 5.7.3.3-1 (pre2017: 5.8.3.3-1)
      pscd->Vn1 = pscd->Vc + pscd->Vs  + (shear_capacity_method == pgsTypes::scmVciVcw ? 0 : pscd->Vp);
   }
   else
   {
      pscd->Vs = 0.0;
      pscd->Vn1 = 0.0;
   }

   // Max crushing capacity - 5.7.3.3-2 (pre2017: 5.8.3.3-2)
   pscd->Vn2 = 0.25 * pscd->fc * pscd->dv * pscd->bv + (shear_capacity_method == pgsTypes::scmVciVcw ? 0 : pscd->Vp);

   if (pscd->ShearInRange)
   {
      pscd->Vn = Min(pscd->Vn1,pscd->Vn2);
   }
   else
   {
      pscd->Vn = pscd->Vn2;
   }

   pscd->pVn = pscd->Phi * pscd->Vn;

 
   EvaluateStirrupRequirements(pscd);

   ComputeVsReqd(poi,pscd);
}

void pgsShearCapacityEngineer::EvaluateStirrupRequirements(SHEARCAPACITYDETAILS* pscd) const
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   pgsTypes::ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   // 5.7.2.3-1 (pre2017: 5.8.2.4-1)
   // transverse reinforcement required?
   pscd->VuLimit = 0.5 * pscd->Phi * ( pscd->Vc + (shear_capacity_method == pgsTypes::scmVciVcw ? 0 : pscd->Vp) );
   pscd->bStirrupsReqd = (pscd->VuLimit < pscd->Vu && pscd->ConcreteType != pgsTypes::PCI_UHPC ? true : false); // stirrups not required for PCI UHPC SDG E.7.2.2
}

void pgsShearCapacityEngineer::TweakShearCapacityOutboardOfCriticalSection(const pgsPointOfInterest& poiCS,SHEARCAPACITYDETAILS* pscd,const SHEARCAPACITYDETAILS* pscd_at_cs) const
{
   // assign Vs, Vc, and shear capacity details data from CS to the critical section that is outboard
   // of the CS
   pscd->ex    = pscd_at_cs->ex;
   pscd->Beta  = pscd_at_cs->Beta;
   pscd->Theta = pscd_at_cs->Theta;
   pscd->Vc    = pscd_at_cs->Vc;
   pscd->Vs    = pscd_at_cs->Vs;

   // Update values that have Vp because we need to use the Vp at the
   // actual section
   pscd->Vn1 = pscd->Vc + pscd->Vs + pscd->Vp;
   pscd->Vn  = Min(pscd->Vn1,pscd->Vn2);
   pscd->pVn = pscd->Phi * pscd->Vn;

   // Update the vertical reforcement requiremetns evaluation
   EvaluateStirrupRequirements(pscd);

   // Update the required Vs computation
   ComputeVsReqd(poiCS,pscd);
}

void pgsShearCapacityEngineer::ComputeFpc(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,FPCDETAILS* pd) const
{
   GET_IFACE(IPretensionForce,pPsForce);
   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IStrandGeometry,pStrandGeometry);
   GET_IFACE(IProductForces,pProdForces);

   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   CGirderKey girderKey(segmentKey);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType noncompsiteIntervalIdx = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType finalIntervalIdx = pIntervals->GetIntervalCount() - 1;
   IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();

   Float64 eps, Pps;
   eps = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poi, pgsTypes::Permanent, pConfig).Y();

   Pps = pPsForce->GetHorizHarpedStrandForce(poi, finalIntervalIdx, pgsTypes::End, pConfig)
      + pPsForce->GetPrestressForce(poi, pgsTypes::Straight, finalIntervalIdx, pgsTypes::End, pConfig);

   GET_IFACE_NOCHECK(IPosttensionForce,pPTForce); // only used if there are tendons

   GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
   Float64 PeSegment = 0;
   Float64 PptSegment = 0;
   IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);
   DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);
   for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
   {
      // only include tendons that are stressed prior to the section becoming composite
      if (pSegmentTendonGeometry->IsOnDuct(poi))
      {
         if (stressTendonIntervalIdx < compositeIntervalIdx)
         {
            Float64 eccX, eccY;
            pSegmentTendonGeometry->GetSegmentTendonEccentricity(finalIntervalIdx, poi, ductIdx, &eccX, &eccY);
            Float64 F = pPTForce->GetSegmentTendonForce(poi, finalIntervalIdx, pgsTypes::End, ductIdx, true, true);
            PptSegment += F;
            PeSegment += F*eccY;
         }
      }
   }
   Float64 eptSegment = (IsZero(PptSegment) ? 0 : PeSegment / PptSegment);

   GET_IFACE(IGirderTendonGeometry,pGirderTendonGeometry);
   Float64 PeGirder  = 0;
   Float64 PptGirder = 0;
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++ )
   {
      // only include tendons that are stressed prior to the section becoming composite
      if (pGirderTendonGeometry->IsOnDuct(poi, ductIdx))
      {
         IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressGirderTendonInterval(girderKey, ductIdx);
         if (stressTendonIntervalIdx < compositeIntervalIdx)
         {
            Float64 eccX, eccY;
            pGirderTendonGeometry->GetGirderTendonEccentricity(finalIntervalIdx, poi, ductIdx, &eccX, &eccY);
            Float64 F = pPTForce->GetGirderTendonForce(poi, finalIntervalIdx, pgsTypes::End, ductIdx, true, true);
            PptGirder += F;
            PeGirder += F*eccY;
         }
      }
   }
   Float64 eptGirder = (IsZero(PptGirder) ? 0 : PeGirder/PptGirder);

   // girder only props
   Float64 Ybg = pSectProp->GetY(noncompsiteIntervalIdx,poi,pgsTypes::BottomGirder);
   Float64 I   = pSectProp->GetIxx(noncompsiteIntervalIdx,poi);
   Float64 A   = pSectProp->GetAg(noncompsiteIntervalIdx,poi);

   Float64 Hg  = pSectProp->GetHg(noncompsiteIntervalIdx,poi);
   Float64 Ybc = pSectProp->GetY(finalIntervalIdx,poi,pgsTypes::BottomGirder);

   Float64 c   = Ybg - Min(Hg,Ybc);


   GET_IFACE(ICombinedForces,pCombinedForces);
   Float64 Mg = pCombinedForces->GetMoment(noncompsiteIntervalIdx, lcDC, poi, bat, rtCumulative);
   Mg        += pCombinedForces->GetMoment(noncompsiteIntervalIdx, lcDW, poi, bat, rtCumulative);

   Float64 fpc = -(Pps+PptSegment + PptGirder)/A - (Pps*eps + PptSegment*eptSegment + PptGirder*eptGirder)*c/I + Mg*c/I;
   fpc *= -1.0; // Need to make the compressive stress a positive value

   pd->eps = eps;
   pd->Pps = Pps;
   pd->eptSegment = eptSegment;
   pd->PptSegment = PptSegment;
   pd->eptGirder = eptGirder;
   pd->PptGirder = PptGirder;
   pd->Ag  = A;
   pd->Ig  = I;
   pd->Ybg = Ybg;
   pd->Ybc = Ybc;
   pd->c   = c;
   pd->Mg  = Mg;
   pd->fpc = fpc;
}


//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsShearCapacityEngineer::MakeCopy(const pgsShearCapacityEngineer& rOther)
{
   m_pBroker = rOther.m_pBroker;
   m_StatusGroupID = rOther.m_StatusGroupID;
}

void pgsShearCapacityEngineer::MakeAssignment(const pgsShearCapacityEngineer& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsShearCapacityEngineer::AssertValid() const
{
   return true;
}

void pgsShearCapacityEngineer::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for pgsShearCapacityEngineer") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsShearCapacityEngineer::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsShearCapacityEngineer");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsShearCapacityEngineer");

   TESTME_EPILOG("ShearCapacityEngineer");
}
#endif // _UNITTEST

bool pgsShearCapacityEngineer::GetGeneralInformation(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, SHEARCAPACITYDETAILS* pscd) const
{
   // The first thing we are going to do is fill in the SHEARCAPACITYDETAILS struct
   // with everything we know

   // general information to get started with
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(ILimitStateForces,pLsForces);
   GET_IFACE(ICombinedForces,pCombinedForces);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(ISectionProperties,pSectProp);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( lrfdVersionMgr::SecondEditionWith2000Interims <= pSpecEntry->GetSpecificationType() ? true : false );

   pgsTypes::ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   // Applied forces

   Float64 Mu_max, Mu_min;
   sysSectionValue Vu_min, Vu_max;
   sysSectionValue Vi_min, Vi_max; // shear that is concurrent with Mmin and Mmax
   sysSectionValue Vd_min, Vd_max;

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   if ( analysisType == pgsTypes::Envelope )
   {
      Float64 Mmin,Mmax;
      sysSectionValue Vmin, Vmax;
      sysSectionValue Vimin, Vimax; // shear that is concurrent with Mmin and Mmax

      pLsForces->GetMoment( intervalIdx, limitState, poi, pgsTypes::MaxSimpleContinuousEnvelope, &Mmin, &Mmax );
      pLsForces->GetShear(  intervalIdx, limitState, poi, pgsTypes::MaxSimpleContinuousEnvelope, &Vmin, &Vmax );
      pLsForces->GetConcurrentShear(  intervalIdx, limitState, poi, pgsTypes::MaxSimpleContinuousEnvelope, &Vimin, &Vimax );
      Mu_max = Mmax;
      Vu_max = Vmax;
      Vi_max = Vimax;

      pLsForces->GetMoment( intervalIdx, limitState, poi, pgsTypes::MinSimpleContinuousEnvelope, &Mmin, &Mmax );
      pLsForces->GetShear(  intervalIdx, limitState, poi, pgsTypes::MinSimpleContinuousEnvelope, &Vmin, &Vmax );
      pLsForces->GetConcurrentShear(  intervalIdx, limitState, poi, pgsTypes::MinSimpleContinuousEnvelope, &Vimin, &Vimax );
      Mu_min = Mmin;
      Vu_min = Vmin;
      Vi_min = Vimin;

      Vd_min =  pCombinedForces->GetShear(intervalIdx,lcDC,poi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative);
      Vd_min += pCombinedForces->GetShear(intervalIdx,lcDW,poi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative);
      Vd_max =  pCombinedForces->GetShear(intervalIdx,lcDC,poi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative);
      Vd_max += pCombinedForces->GetShear(intervalIdx,lcDW,poi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative);
   }
   else
   {
      pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
      pLsForces->GetMoment( intervalIdx, limitState, poi, bat, &Mu_min, &Mu_max );
      pLsForces->GetShear(  intervalIdx, limitState, poi, bat, &Vu_min, &Vu_max );
      pLsForces->GetConcurrentShear(  intervalIdx, limitState, poi, bat, &Vi_min, &Vi_max );

      Vd_min =  pCombinedForces->GetShear(intervalIdx,lcDC,poi,bat,rtCumulative);
      Vd_min += pCombinedForces->GetShear(intervalIdx,lcDW,poi,bat,rtCumulative);
      Vd_max = Vd_min;
   }

   Mu_max = IsZero(Mu_max) ? 0 : Mu_max;
   Mu_min = IsZero(Mu_min) ? 0 : Mu_min;

   // driving moment is the one with the greater magnitude
   Float64 Mu = Max(fabs(Mu_max),fabs(Mu_min));

   pscd->Mu = Mu;
   pscd->RealMu = Mu;

   // Determine if the tension side is on the top half or bottom half of the girder
   // The flexural tension side is on the bottom when the maximum (positive) bending moment
   // exceeds the minimum (negative) bending moment
   bool bTensionOnBottom = ( IsLE(fabs(Mu_min),fabs(Mu_max),0.0001) ? true : false);
   pscd->bTensionBottom = bTensionOnBottom;

   // axial force
   pscd->Nu = 0.0; // KLUDGE: Assume Axial Force is zero.

   // design shear
   Float64 vmax, vmin;
   vmax = Max(fabs(Vu_max.Left()), fabs(Vu_max.Right()));
   vmin = Max(fabs(Vu_min.Left()), fabs(Vu_min.Right()));
   pscd->Vu = Max(vmax, vmin);

   // Vi and Vd
   if ( IsEqual(Mu,fabs(Mu_max)) )
   {
      // magnitude of maximum moment is greater
      // use least of Left/Right Vi_max
      pscd->Vi = Min(fabs(Vi_max.Left()),fabs(Vi_max.Right()));
      pscd->Vd = Min(fabs(Vd_max.Left()),fabs(Vd_max.Right()));
   }
   else
   {
      // magnitude of minimum moment is greater
      // use least of Left/Right Vi_min
      pscd->Vi = Min(fabs(Vi_min.Left()),fabs(Vi_min.Right()));
      pscd->Vd = Min(fabs(Vd_min.Left()),fabs(Vd_min.Right()));
   }

   // phi factor for shear
   GET_IFACE(IResistanceFactors, pResistanceFactors);
   GET_IFACE(IMaterials,         pMaterial);
   GET_IFACE(IPointOfInterest,   pPoi);

   CClosureKey closureKey;
   bool bIsInClosureJoint = pPoi->IsInClosureJoint(poi,&closureKey);
   if ( bIsInClosureJoint )
   {
      pscd->Phi = pResistanceFactors->GetClosureJointShearResistanceFactor( pMaterial->GetClosureJointConcreteType(closureKey) );
   }
   else
   {
      pscd->Phi = pResistanceFactors->GetShearResistanceFactor( poi, pMaterial->GetSegmentConcreteType(segmentKey) );
   }

   // shear area (bv and dv)
   pscd->bv = pGdr->GetShearWidth(poi);
   pscd->bw = pGdr->GetWebWidth(poi); // needed for Vs for LRFD 9th edition and later


   // material props
   if ( pConfig == nullptr )
   {
      GET_IFACE(ILibrary, pLib);
      GET_IFACE(ISpecification, pSpec);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
      bool bUse90DayStrength;
      Float64 factor;
      pSpecEntry->Use90DayStrengthForSlowCuringConcrete(&bUse90DayStrength, &factor);

      if (bUse90DayStrength)
      {
         // 90 day strength isn't applicable to strength limit states (only stress limit states, LRFD 5.12.3.2.4)
         // so use 28day properties
         pscd->fc = pMaterial->GetSegmentFc28(segmentKey);
         pscd->Ec = pMaterial->GetSegmentEc28(segmentKey);
      }
      else
      {
         pscd->fc = pMaterial->GetSegmentDesignFc(segmentKey, intervalIdx);
         pscd->Ec = pMaterial->GetSegmentEc(segmentKey, intervalIdx);
      }
   }
   else
   {
      pscd->fc = pConfig->fc28;

      if ( pConfig->bUserEc )
      {
         pscd->Ec = pConfig->Ec;
      }
      else
      {
         pscd->Ec = pMaterial->GetEconc(pMaterial->GetSegmentConcreteType(segmentKey),pscd->fc,
                                        pMaterial->GetSegmentStrengthDensity(segmentKey),
                                        pMaterial->GetSegmentEccK1(segmentKey),
                                        pMaterial->GetSegmentEccK2(segmentKey));
      }
   }

   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Straight); // we just want E so straight strands is fine
   ATLASSERT(pStrand != nullptr);
   pscd->Eps = pStrand->GetE();

   const matPsStrand* pSegmentTendon = pMaterial->GetSegmentTendonMaterial(segmentKey);
   ATLASSERT(pSegmentTendon != nullptr);
   pscd->EptSegment = pSegmentTendon->GetE();

   const matPsStrand* pGirderTendon = pMaterial->GetGirderTendonMaterial(segmentKey);
   ATLASSERT(pGirderTendon != nullptr);
   pscd->EptGirder = pGirderTendon->GetE();

   // stirrup properties
   GET_IFACE(IStirrupGeometry,pStirrups);
   Float64 Es, fy, fu;
   if ( bIsInClosureJoint )
   {
      pMaterial->GetClosureJointTransverseRebarProperties(closureKey,&Es,&fy,&fu);
   }
   else
   {
      pMaterial->GetSegmentTransverseRebarProperties(segmentKey,&Es,&fy,&fu);
   }

   // added with LRFD 7th Edition 2014
   // fy <= 100 ksi
   // See LRFD 5.7.2.5 (pre2017: 5.8.2.5)
   if ( lrfdVersionMgr::SeventhEdition2014 <= lrfdVersionMgr::GetVersion() )
   {
      fy = min(fy,::ConvertToSysUnits(100.0,unitMeasure::KSI));
   }

   Float64 s;
   matRebar::Size size;
   Float64 nl;
   Float64 abar;
   Float64 avs;
   if ( pConfig == nullptr )
   {
      avs = pStirrups->GetVertStirrupAvs(poi, &size, &abar, &nl, &s);
   }
   else
   {
      GET_IFACE(IBridge, pBridge);
      Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
      Float64 location       = poi.GetDistFromStart();
      Float64 lft_supp_loc   = pBridge->GetSegmentStartEndDistance(segmentKey);
      Float64 rgt_sup_loc    = segment_length - pBridge->GetSegmentEndEndDistance(segmentKey);

      avs = GetPrimaryStirrupAvs(pConfig->StirrupConfig, getVerticalStirrup, location, segment_length, 
                                 lft_supp_loc, rgt_sup_loc, &size, &abar, &nl, &s);
   }

   pscd->Av    = abar*nl;
   pscd->fy    = fy;
   pscd->S     = s;
   pscd->Alpha = pStirrups->GetAlpha(poi);

   // long rebar
   GET_IFACE(ILongRebarGeometry,pLongRebarGeometry);

   if ( bTensionOnBottom )
   {
      pscd->As = pLongRebarGeometry->GetAsBottomHalf(poi,true);
   }
   else
   {
      pscd->As = pLongRebarGeometry->GetAsTopHalf(poi,true);
   }

   if ( bIsInClosureJoint )
   {
      pMaterial->GetClosureJointLongitudinalRebarProperties(closureKey,&Es,&fy,&fu);
   }
   else
   {
      pMaterial->GetSegmentLongitudinalRebarProperties(segmentKey,&Es,&fy,&fu);
   }
   pscd->Es = Es;

   // areas on tension side of axis
   if ( bTensionOnBottom )
   {
      pscd->Ac = pSectProp->GetAcBottomHalf(intervalIdx,poi); 
   }
   else
   {
      pscd->Ac = pSectProp->GetAcTopHalf(intervalIdx,poi); 
   }

   return true;
}

bool pgsShearCapacityEngineer::GetInformation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, SHEARCAPACITYDETAILS* pscd) const
{
   GetGeneralInformation(intervalIdx,limitState,poi,pConfig,pscd);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IPointOfInterest,pPoi);
   CClosureKey closureKey;
   bool bIsInClosureJoint = pPoi->IsInClosureJoint(poi,&closureKey);

   GET_IFACE(IPretensionForce,pPsForce);
   GET_IFACE_NOCHECK(IPosttensionForce,pPTForce); // not used when design
   GET_IFACE(IMomentCapacity,pMomentCapacity);
   GET_IFACE(IStrandGeometry,pStrandGeometry);
   GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
   GET_IFACE(IGirderTendonGeometry, pGirderTendonGeometry);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(ISectionProperties,pSectProps);

   // vertical component of prestress force
   pscd->Vps = pPsForce->GetVertHarpedStrandForce(poi, intervalIdx, pgsTypes::End, pConfig);
   if (pConfig == nullptr)
   {
      pscd->VptSegment = pPTForce->GetSegmentTendonVerticalForce(poi, intervalIdx, pgsTypes::End, ALL_DUCTS);
      pscd->VptGirder = pPTForce->GetGirderTendonVerticalForce(poi, intervalIdx, pgsTypes::End, ALL_DUCTS);
   }
   else
   {
      // if there is a config, this is a pretensioned bridge so no tendons.
      pscd->VptSegment = 0;
      pscd->VptGirder = 0;
#if defined _DEBUG
      GET_IFACE(IDocumentType, pDocType);
      ATLASSERT(pDocType->IsPGSuperDocument());
#endif
   }
   pscd->Vp = pscd->Vps + pscd->VptSegment + pscd->VptGirder;

   const MOMENTCAPACITYDETAILS* pCapdet = pMomentCapacity->GetMomentCapacityDetails(intervalIdx, poi, (pscd->bTensionBottom ? true : false), pConfig);

   pscd->de        = pCapdet->de_shear; // see PCI BDM 8.4.1.2
   pscd->MomentArm = pCapdet->MomentArm;
   pscd->PhiMu     = pCapdet->Phi;

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( lrfdVersionMgr::SecondEditionWith2000Interims <= pSpecEntry->GetSpecificationType() ? true : false );

   pgsTypes::ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   Float64 struct_slab_h = pBridge->GetStructuralSlabDepth(poi);

   pgsTypes::HaunchAnalysisSectionPropertiesType hatype = pSectProps->GetHaunchAnalysisSectionPropertiesType();
   Float64 haunch_depth = pSectProps->GetStructuralHaunchDepth(poi, hatype);

   pscd->h = pGdr->GetHeight(poi) + struct_slab_h + haunch_depth;

   // lrfd 5.7.2.6 (pre2017: 5.8.2.7)
   pscd->dv = Max( pscd->MomentArm, 0.9*pscd->de, 0.72*pscd->h );
   

   Float64 Mu = pscd->Mu;
   Float64 MuSign = BinarySign(Mu);

   if ( bAfter1999 && shear_capacity_method != pgsTypes::scmVciVcw )
   {
      // MuMin for Beta Theta equation (or WSDOT 2007) = |Vu - Vp|*dv
      // otherwise it is Vu*dv
      Float64 MuMin = (shear_capacity_method == pgsTypes::scmBTEquations || shear_capacity_method == pgsTypes::scmWSDOT2007) ?
         fabs(pscd->Vu - pscd->Vp)*pscd->dv : pscd->Vu*pscd->dv;

      if ( Mu < MuMin )
      {
         pscd->Mu          = MuSign*MuMin;
         pscd->RealMu      = MuSign*Mu;
         pscd->MuLimitUsed = true;
      }
      else
      {
         pscd->Mu          = MuSign*Mu;
         pscd->RealMu      = MuSign*Mu;
         pscd->MuLimitUsed = false;
      }
   }

   GET_IFACE(IShearCapacity,pShearCapacity);
   pscd->fpc = pShearCapacity->GetFpc(poi, pConfig);
   pscd->fpeps = pPsForce->GetEffectivePrestress(poi, pgsTypes::Permanent, intervalIdx, pgsTypes::End, pConfig);

   if ( bAfter1999 )
   {
      pscd->fpeptSegment = -999999; // not used so set parameter to a dummy value
      pscd->fpeptGirder = -999999; // not used so set parameter to a dummy value
   }
   else
   {
      // compute average stress in segment tendons
      DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);
      Float64 fpeAptSegment = 0;
      Float64 AptSegment = 0;
      for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
      {
         Float64 fpe = pPTForce->GetSegmentTendonStress(poi, intervalIdx, pgsTypes::End, ductIdx, true, true);
         Float64 apt = pSegmentTendonGeometry->GetSegmentTendonArea(segmentKey, intervalIdx, ductIdx);
         fpeAptSegment += fpe*apt;
         AptSegment += apt;
      }
      pscd->fpeptSegment = (IsZero(AptSegment) ? 0 : fpeAptSegment / AptSegment);

      // compute average stress in girder tendons
      DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(segmentKey);
      Float64 fpeAptGirder = 0;
      Float64 AptGirder = 0;
      for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++ )
      {
         Float64 fpe = pPTForce->GetGirderTendonStress(poi,intervalIdx,pgsTypes::End,ductIdx,true,true);
         Float64 apt = pGirderTendonGeometry->GetGirderTendonArea(segmentKey,intervalIdx,ductIdx);
         fpeAptGirder += fpe*apt;
         AptGirder += apt;
      }
      pscd->fpeptGirder = (IsZero(AptGirder) ? 0 : fpeAptGirder/AptGirder);
   }

   // prestress area - factor for development length per LRFD 5.7.3.4.2
   Float64 apsu = 0;
   if (pConfig == nullptr)
   {
      if ( pscd->bTensionBottom )
      {
         apsu = pStrandGeometry->GetApsBottomHalf(poi,dlaAccurate);
      }
      else
      {
         apsu = pStrandGeometry->GetApsTopHalf(poi,dlaAccurate);
      }
   }
   else
   {
      // Use approximate method during design. The performance gain is around 30%. However, this does
      // change results slightly in the end zones for some files. If this becomes problematic, check out the 
      // factor in CBridgeAgentImp::GetApsInHalfDepth where the accurate method is used only in end zones.
      if ( pscd->bTensionBottom )
      {
         apsu = pStrandGeometry->GetApsBottomHalf(poi,dlaApproximate,pConfig);
      }
      else                                                             
      {
         apsu = pStrandGeometry->GetApsTopHalf(poi,dlaApproximate,pConfig);
      }
   }

   pscd->Aps = apsu;

   if ( pscd->bTensionBottom )
   {
      pscd->AptSegment = pSegmentTendonGeometry->GetSegmentAptBottomHalf(poi);
      pscd->AptGirder = pGirderTendonGeometry->GetGirderAptBottomHalf(poi);
   }
   else
   {
      pscd->AptSegment = pSegmentTendonGeometry->GetSegmentAptTopHalf(poi);
      pscd->AptGirder = pGirderTendonGeometry->GetGirderAptTopHalf(poi);
   }

#if defined _DEBUG
   if ( pConfig != nullptr )
   {
      ATLASSERT(IsZero(pscd->AptSegment + pscd->AptGirder)); // if we have a config, we better not have a tendon
   }
#endif

   GET_IFACE(IMaterials,pMat);
   pscd->ag = pMat->GetSegmentMaxAggrSize(segmentKey);
   pscd->sx = pscd->dv; // We don't have input for this, so use dv. Assume area provided is > 0.003bvsx

   // cracking moment parameters for LRFD simplified method
   CRACKINGMOMENTDETAILS mcr_details;
   if ( pConfig == nullptr )
   {
      mcr_details = *(pMomentCapacity->GetCrackingMomentDetails(intervalIdx,poi,(pscd->bTensionBottom ? true : false)));
   }
   else
   {
      pMomentCapacity->GetCrackingMomentDetails(intervalIdx,poi,*pConfig,(pscd->bTensionBottom ? true : false),&mcr_details);
   }

   GET_IFACE(IMaterials,pMaterial);

   pscd->McrDetails = mcr_details;
   if ( (pscd->bTensionBottom ? true : false) )
   {
      pscd->McrDetails.fr = pMaterial->GetSegmentShearFr(segmentKey,intervalIdx);
   }
   else
   {
      IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
      if (deckCastingRegionIdx == INVALID_INDEX)
      {
         pscd->McrDetails.fr = pMaterial->GetSegmentShearFr(segmentKey, intervalIdx);
      }
      else
      {
         pscd->McrDetails.fr = pMaterial->GetDeckShearFr(deckCastingRegionIdx, intervalIdx);
      }
   }

   pscd->McrDetails.Mcr = pscd->McrDetails.Sbc*(pscd->McrDetails.fr + pscd->McrDetails.fcpe - pscd->McrDetails.Mdnc/pscd->McrDetails.Sb);

   if ( bIsInClosureJoint )
   {
      pscd->ConcreteType = pMaterial->GetClosureJointConcreteType(closureKey);
      switch( pscd->ConcreteType )
      {
      case pgsTypes::Normal:
      case pgsTypes::PCI_UHPC:
         pscd->bHasFct = false;
         pscd->fct = 0;
         break;

      case pgsTypes::AllLightweight:
         if ( pMaterial->DoesClosureJointConcreteHaveAggSplittingStrength(closureKey) )
         {
            pscd->bHasFct = true;
            pscd->fct = pMaterial->GetClosureJointConcreteAggSplittingStrength(closureKey);
         }
         else
         {
            pscd->bHasFct = false;
            pscd->fct = 0;
         }
         break;

      case pgsTypes::SandLightweight:
         if ( pMaterial->DoesClosureJointConcreteHaveAggSplittingStrength(closureKey) )
         {
            pscd->bHasFct = true;
            pscd->fct = pMaterial->GetClosureJointConcreteAggSplittingStrength(closureKey);
         }
         else
         {
            pscd->bHasFct = false;
            pscd->fct = 0;
         }
         break;

      default:
         ATLASSERT(false); // is there a new concrete type
         break;
      }
   }
   else
   {
      pscd->ConcreteType = pMaterial->GetSegmentConcreteType(segmentKey);
      switch( pscd->ConcreteType )
      {
      case pgsTypes::Normal:
      case pgsTypes::PCI_UHPC:
         pscd->bHasFct = false;
         pscd->fct = 0;
         break;

      case pgsTypes::AllLightweight:
         if ( pMaterial->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey) )
         {
            pscd->bHasFct = true;
            pscd->fct = pMaterial->GetSegmentConcreteAggSplittingStrength(segmentKey);
         }
         else
         {
            pscd->bHasFct = false;
            pscd->fct = 0;
         }
         break;

      case pgsTypes::SandLightweight:
         if ( pMaterial->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey) )
         {
            pscd->bHasFct = true;
            pscd->fct = pMaterial->GetSegmentConcreteAggSplittingStrength(segmentKey);
         }
         else
         {
            pscd->bHasFct = false;
            pscd->fct = 0;
         }
         break;

      default:
         ATLASSERT(false); // is there a new concrete type
         break;
      }
   }

   pscd->bLimitNetTensionStrainToPositiveValues = pSpecEntry->LimitNetTensionStrainToPositiveValues();
   pscd->bIgnoreMiniumStirrupRequirementForBeta = (pscd->ConcreteType == pgsTypes::PCI_UHPC); // we want to use the beta equation in all cases
   return true;
}

bool pgsShearCapacityEngineer::ComputeVc(const pgsPointOfInterest& poi, SHEARCAPACITYDETAILS* pscd) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   lrfdShearData data;
   data.Mu           = pscd->Mu;
   data.Nu           = pscd->Nu;
   data.Vu           = pscd->Vu;
   data.phi          = pscd->Phi;
   data.Vp           = pscd->Vp;
   data.dv           = pscd->dv;
   data.bv           = pscd->bv;
   data.Es           = pscd->Es;
   data.As           = pscd->As;
   data.Eps          = pscd->Eps;
   data.Aps          = pscd->Aps;
   data.EptSegment   = pscd->EptSegment;
   data.AptSegment   = pscd->AptSegment;
   data.EptGirder    = pscd->EptGirder;
   data.AptGirder    = pscd->AptGirder;
   data.Ec           = pscd->Ec;
   data.Ac           = pscd->Ac;
   data.fpops        = pscd->fpops;
   data.fpoptSegment = pscd->fpoptSegment;
   data.fpoptGirder  = pscd->fpoptGirder;
   data.fc           = pscd->fc;
   data.fy           = pscd->fy;
   data.AvS          = IsZero(pscd->S) ? 0 : pscd->Av/pscd->S;
   data.Vi           = pscd->Vi;
   data.Vd           = pscd->Vd;
   data.Mcre         = pscd->McrDetails.Mcr;
   data.fpc          = pscd->fpc;
   data.ConcreteType = (matConcrete::Type)pscd->ConcreteType;
   data.bHasfct      = pscd->bHasFct;
   data.fct          = pscd->fct;
   data.sx           = pscd->sx; // cracking
   data.ag           = pscd->ag;
   data.bLimitNetTensionStrainToPositiveValues = pscd->bLimitNetTensionStrainToPositiveValues;
   data.bIgnoreMiniumStirrupRequirementForBeta = pscd->bIgnoreMiniumStirrupRequirementForBeta;

   GET_IFACE(IMaterials,pMaterials);
   data.lambda = pMaterials->GetSegmentLambda(segmentKey);


   GET_IFACE(ISpecification, pSpec);
   GET_IFACE(ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );


   bool shear_in_range = true; // assume calc will be successful
   pgsTypes::ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   if(pscd->ConcreteType == pgsTypes::PCI_UHPC)
   {
      // PCI UHPC only uses beta-theta equations
      shear_capacity_method = pgsTypes::scmBTEquations;
   }

   try
   {
      pscd->Method = shear_capacity_method;
      if ( shear_capacity_method == pgsTypes::scmBTEquations || 
           shear_capacity_method == pgsTypes::scmBTTables    || 
           shear_capacity_method == pgsTypes::scmWSDOT2007 )
      {
         lrfdShear::ComputeThetaAndBeta( &data, shear_capacity_method == pgsTypes::scmBTTables ? lrfdShear::Tables : lrfdShear::Equations );
      }
      else if ( shear_capacity_method == pgsTypes::scmVciVcw )
      {
         lrfdShear::ComputeVciVcw( &data );
      }
      else
      {
         ATLASSERT(shear_capacity_method == pgsTypes::scmWSDOT2001);

         GET_IFACE(IPointOfInterest,pPOI);
         PoiList vPOI;
         pPOI->GetPointsOfInterest(segmentKey, POI_15H, &vPOI);
         ATLASSERT(vPOI.size() == 2);
         Float64 l1 = vPOI.front().get().GetDistFromStart();
         Float64 l2 = vPOI.back().get().GetDistFromStart();

         bool bEndRegion = InRange(l1,poi.GetDistFromStart(),l2) ? false : true;

         lrfdWsdotShear::ComputeThetaAndBeta( &data, bEndRegion );
      }
   }
   catch (lrfdXShear& rxs ) 
   {
      if (rxs.GetReason()==lrfdXShear::vfcOutOfRange)
      {
         shear_in_range=false;
      }
      else if (rxs.GetReason()==lrfdXShear::MaxIterExceeded)
      {
         GET_IFACE(IEAFStatusCenter,pStatusCenter);

         std::_tstring msg(std::_tstring(SEGMENT_LABEL(segmentKey)) + _T(": Error computing shear capacity - could not converge on a solution"));
         pgsGirderDescriptionStatusItem* pStatusItem =
            new pgsGirderDescriptionStatusItem(segmentKey,2,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());

         pStatusCenter->Add(pStatusItem);

         msg += std::_tstring(_T("\nSee Status Center for Details"));
         THROW_UNWIND(msg.c_str(),-1);
      }
      else 
      {
         return false;
      }
   }
#if defined _DEBUG
   catch(lrfdXCodeVersion& /*e*/)
   {
      ATLASSERT(false); // should never get here
      // Vci Vcw should not be used unless LRFD is 2007 or later
   }
#endif // _DEBUG

   if ( shear_capacity_method == pgsTypes::scmBTEquations || 
        shear_capacity_method == pgsTypes::scmWSDOT2001   || 
        shear_capacity_method == pgsTypes::scmWSDOT2007   || 
        shear_capacity_method == pgsTypes::scmBTTables )
   {
      if (shear_in_range)
      {
         pscd->ShearInRange = true;
         pscd->vu            = data.vu;
         pscd->vufc          = data.vufc;
         pscd->ex           = data.ex;
         pscd->Fe           = data.Fe;
         pscd->Beta         = data.Beta;
         pscd->BetaEqn      = data.BetaEqn;
         pscd->BetaThetaTable = data.BetaTheta_tbl;
         pscd->sxe          = data.sxe;
         pscd->sxe_tbl      = data.sxe_tbl;
         pscd->Theta        = data.Theta;
         pscd->Equation     = data.Eqn;
         pscd->ex_tbl       = data.ex_tbl;
         pscd->vfc_tbl     = data.vufc_tbl;

         Float64 Beta  = data.Beta;
         Float64 Theta = data.Theta;
         Float64 dv    = pscd->dv;
         Float64 bv    = pscd->bv;

         const unitLength* pLengthUnit = nullptr;
         const unitStress* pStressUnit = nullptr;
         const unitForce*  pForceUnit  = nullptr;
         Float64 K; // main coefficient in equaion 5.7.3.3-3 (pre2017: 5.8.3.3-3)
         Float64 Kfct; // coefficient for fct if LWC
         if ( lrfdVersionMgr::GetUnits() == lrfdVersionMgr::US )
         {
            pLengthUnit = &unitMeasure::Inch;
            pStressUnit = &unitMeasure::KSI;
            pForceUnit  = &unitMeasure::Kip;
            K = 0.0316;
            Kfct = 4.7;
         }
         else
         {
            pLengthUnit = &unitMeasure::Millimeter;
            pStressUnit = &unitMeasure::MPa;
            pForceUnit  = &unitMeasure::Newton;
            K = 0.083;
            Kfct = 1.8;
         }

         Float64 Vc = -99999;
         if (pscd->ConcreteType == pgsTypes::PCI_UHPC)
         {
            GET_IFACE(IIntervals, pIntervals);
            IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
            GET_IFACE(IAllowableConcreteStress, pAllowables);
            Float64 ft = pAllowables->GetAllowableTensionStress(poi, pgsTypes::BottomGirder, StressCheckTask(liveLoadIntervalIdx,pgsTypes::ServiceIII,pgsTypes::Tension), false, true);
            pscd->FiberStress = ft;

            Float64 cot_theta = 1 / tan(Theta);

            Vc = ft * bv * dv * cot_theta; // this is really Vcf
         }
         else
         {
            dv = ::ConvertFromSysUnits(dv, *pLengthUnit);
            bv = ::ConvertFromSysUnits(bv, *pLengthUnit);

            Float64 fc = ::ConvertFromSysUnits(pscd->fc, *pStressUnit);
            Float64 fct = ::ConvertFromSysUnits(pscd->fct, *pStressUnit);

            // 5.7.3.3-3 (pre2017: 5.8.3.3-3)
            Vc = K * data.lambda * Beta * bv * dv;
            if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SeventhEditionWith2016Interims)
            {
               switch (pscd->ConcreteType)
               {
               case pgsTypes::Normal:
                  Vc *= sqrt(fc);
                  break;

               case pgsTypes::AllLightweight:
                  if (pscd->bHasFct)
                  {
                     Vc *= Min(Kfct * fct, sqrt(fc));
                  }
                  else
                  {
                     Vc *= 0.75 * sqrt(fc);
                  }
                  break;

               case pgsTypes::SandLightweight:
                  if (pscd->bHasFct)
                  {
                     Vc *= Min(Kfct * fct, sqrt(fc));
                  }
                  else
                  {
                     Vc *= 0.85 * sqrt(fc);
                  }
                  break;

               default:
                  ATLASSERT(false); // is there a new concrete type
                  Vc *= sqrt(fc);
                  break;
               }
            }
            else
            {
               Vc *= sqrt(fc);
            }
            Vc = ::ConvertToSysUnits(Vc, *pForceUnit);
         }

         pscd->Vc = Vc;
      }
      else
      {
         // capacity calculation could not be done - section is too small
         // pick up the shreds
         pscd->ShearInRange = false;
         pscd->vu   = data.vu;
         pscd->vufc = data.vufc;
         pscd->ex   = 0.0;
         pscd->Beta = 0.0;
         pscd->Theta = 0.0;
         pscd->Vc = 0.0;
         pscd->Fe = -1; // Not applicable
         pscd->vfc_tbl  = 0.0;
         pscd->ex_tbl  = 0.0;
      }
   }
   else
   {
      ATLASSERT(shear_capacity_method == pgsTypes::scmVciVcw);

      pscd->ShearInRange = true;
      pscd->Vci = data.Vci;
      pscd->Vcw = data.Vcw;
      pscd->VciCalc = data.VciCalc;
      pscd->VciMin = data.VciMin;
      pscd->Vc = Min( data.Vci, data.Vcw );
   }

   return true;
}

bool pgsShearCapacityEngineer::ComputeVs(const pgsPointOfInterest& poi, SHEARCAPACITYDETAILS* pscd) const
{
   GET_IFACE(ISpecification, pSpec);
   GET_IFACE(ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );


   pgsTypes::ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   Float64 cot_theta;
   if ( shear_capacity_method == pgsTypes::scmVciVcw )
   {
      if ( IsLE(pscd->Vci,pscd->Vcw) )
      {
         cot_theta = 1.0;
      }
      else
      {
         Float64 fc, fpc, fct, Kfct, K;
         if (lrfdVersionMgr::GetUnits() == lrfdVersionMgr::SI )
         {
            fc  = ::ConvertFromSysUnits(pscd->fc,  unitMeasure::MPa);
            fpc = ::ConvertFromSysUnits(pscd->fpc, unitMeasure::MPa);
            fct = ::ConvertFromSysUnits(pscd->fct, unitMeasure::MPa);
            Kfct = 1.8;
            K = 1.14;
         }
         else
         {
            fc  = ::ConvertFromSysUnits(pscd->fc,  unitMeasure::KSI);
            fpc = ::ConvertFromSysUnits(pscd->fpc, unitMeasure::KSI);
            fct = ::ConvertFromSysUnits(pscd->fct, unitMeasure::KSI);
            Kfct = 4.7;
            K = 3.0;
         }

         Float64 sqrt_fc;
         if ( lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() )
         {
            GET_IFACE(IMaterials,pMaterials);
            Float64 lambda = pMaterials->GetSegmentLambda(poi.GetSegmentKey());
            sqrt_fc = lambda * sqrt(fc);
         }
         else
         {
            if ( pscd->ConcreteType == pgsTypes::Normal)
            {
               sqrt_fc = sqrt(fc);
            }
            else if ( (pscd->ConcreteType == pgsTypes::AllLightweight || pscd->ConcreteType == pgsTypes::SandLightweight) && pscd->bHasFct )
            {
               sqrt_fc = Min(Kfct*fct,sqrt(fc));
            }
            else if ( pscd->ConcreteType == pgsTypes::AllLightweight && !pscd->bHasFct )
            {
               sqrt_fc = 0.75*sqrt(fc);
            }
            else if ( pscd->ConcreteType == pgsTypes::SandLightweight && !pscd->bHasFct )
            {
               sqrt_fc = 0.85*sqrt(fc);
            }
#if defined _DEBUG
            else
            {
               ATLASSERT(false); // should never get here
               ATLASSERT(pscd->ConcreteType != pgsTypes::PCI_UHPC); // PCI UHPC is not compatible with the Vci/Vcw method.
            }
#endif
         }

         cot_theta = Min(1.0+K*(fpc/sqrt_fc),1.8);
      }

      pscd->Theta = atan(1/cot_theta);
      
   }
   else
   {
      Float64 Theta = pscd->Theta;
      Theta = ::ConvertFromSysUnits( Theta, unitMeasure::Radian );
      cot_theta = 1/tan(Theta);
   }

   Float64 dv = pscd->dv;

   Float64 Av = pscd->Av;
   Float64 S  = pscd->S;
   Float64 fy = pscd->fy;

   Float64 Vs = (IsZero(S) ? 0 : fy * dv * Av * cot_theta / S );

   if (lrfdVersionMgr::NinthEdition2020 <= lrfdVersionMgr::GetVersion())
   {
      // We are assuming that the bridge is open to traffic so all ducts are grouted

      // Find largest duct in section to use for the deduction factor
      Float64 duct_diameter = 0;

      GET_IFACE(IPointOfInterest, pPoi);
      // Check segment ducts
      if (pPoi->IsOnSegment(poi))
      {
         const CSegmentKey& segmentKey(poi.GetSegmentKey());
         // need to make sure poi is in segment (could be in closure)
         GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
         DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);
         if (pSegmentTendonGeometry->IsOnDuct(poi))
         {
            for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
            {
               Float64 d = pSegmentTendonGeometry->GetNominalDiameter(segmentKey, ductIdx);
               duct_diameter = Max(duct_diameter, d);
            }
         }
      }

      if (pPoi->IsOnGirder(poi))
      {
         const CGirderKey& girderKey(poi.GetSegmentKey());
         GET_IFACE(IGirderTendonGeometry, pGirderTendonGeometry);
         DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);
         for (DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++)
         {
            if (pGirderTendonGeometry->IsOnDuct(poi, ductIdx))
            {
               Float64 d = pGirderTendonGeometry->GetNominalDiameter(girderKey, ductIdx);
               duct_diameter = Max(duct_diameter, d);
            }
         }
      }


      // if there aren't any ducts, duct_diameter is 0.0 and lambda_duct
      // evalutes to 1.0, which is what we want
      Float64 delta = 2.0;
      Float64 bw = pscd->bw;
      Float64 lambda_duct = 1 - delta*pow(duct_diameter / bw, 2.0);

      pscd->duct_diameter = duct_diameter;
      pscd->delta = delta;
      pscd->lambda_duct = lambda_duct;

      Vs *= lambda_duct;
   }

   pscd->Vs = Vs;

   return true;
}

void pgsShearCapacityEngineer::ComputeVsReqd(const pgsPointOfInterest& poi, SHEARCAPACITYDETAILS* pscd) const
{
   Float64 AvOverS = 0;

   GET_IFACE(ISpecification, pSpec);
   GET_IFACE(ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   pgsTypes::ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   Float64 Vu = pscd->Vu;
   Float64 Vc = pscd->Vc;
   Float64 Vp = (shear_capacity_method == pgsTypes::scmVciVcw ? 0 : pscd->Vp);
   Float64 Phi = pscd->Phi;

   Float64 VsReqd = Max(Vu / Phi - Vc - Vp, 0.0); // based on strength, compute force required to be carried by stirrups
   if ( pscd->bStirrupsReqd || 0 < VsReqd)
   {
      // stirrups are required by either spec requirement (5.7.2.3) or strength requirement
      // compute the required AvOverS for this POI

      Float64 Theta = pscd->Theta;
      Float64 fy = pscd->fy;
      Float64 dv = pscd->dv;

      Float64 cot = 1/tan(Theta);
      AvOverS = VsReqd/(fy*dv*cot);
   }

   pscd->VsReqd       = VsReqd;
   pscd->AvOverS_Reqd = AvOverS;
}
