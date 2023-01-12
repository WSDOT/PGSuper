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
#include "PsForceEng.h"
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
#include <IFace\Intervals.h>
#include <IFace\RatingSpecification.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\LoadFactors.h>

#include <numeric>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   pgsPsForceEng
****************************************************************************/


pgsPsForceEng::pgsPsForceEng() :
m_StatusGroupID(INVALID_ID)
{
}

pgsPsForceEng::~pgsPsForceEng()
{
}

void pgsPsForceEng::SetBroker(IBroker* pBroker)
{
   m_pBroker = pBroker;
}

void pgsPsForceEng::SetStatusGroupID(StatusGroupIDType statusGroupID)
{
   m_StatusGroupID = statusGroupID;
}

void pgsPsForceEng::CreateLossEngineer(const CGirderKey& girderKey) const
{
   if (m_LossEngineer )
   {
      return;
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
   const GirderLibraryEntry* pGdr = pGirder->GetGirderLibraryEntry();

   CComPtr<IBeamFactory> beamFactory;
   pGdr->GetBeamFactory(&beamFactory);

   beamFactory->CreatePsLossEngineer(m_pBroker,m_StatusGroupID,girderKey,&m_LossEngineer);
}

void pgsPsForceEng::Invalidate()
{
   m_LossEngineer.Release();
}     

void pgsPsForceEng::ClearDesignLosses()
{
   if (m_LossEngineer)
   {
      m_LossEngineer->ClearDesignLosses();
   }
}

void pgsPsForceEng::ReportLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const
{
   CreateLossEngineer(girderKey);
   m_LossEngineer->BuildReport(girderKey,pChapter,pDisplayUnits);
}

void pgsPsForceEng::ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const
{
   CreateLossEngineer(girderKey);
   m_LossEngineer->ReportFinalLosses(girderKey,pChapter,pDisplayUnits);
}

const ANCHORSETDETAILS* pgsPsForceEng::GetGirderTendonAnchorSetDetails(const CGirderKey& girderKey,DuctIndexType ductIdx) const
{
   CreateLossEngineer(girderKey);
   return m_LossEngineer->GetGirderTendonAnchorSetDetails(girderKey,ductIdx);
}

const ANCHORSETDETAILS* pgsPsForceEng::GetSegmentTendonAnchorSetDetails(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const
{
   CreateLossEngineer(segmentKey);
   return m_LossEngineer->GetSegmentTendonAnchorSetDetails(segmentKey, ductIdx);
}

Float64 pgsPsForceEng::GetGirderTendonElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType) const
{
   CreateLossEngineer(girderKey);
   return m_LossEngineer->GetGirderTendonElongation(girderKey,ductIdx,endType);
}

Float64 pgsPsForceEng::GetSegmentTendonElongation(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const
{
   CreateLossEngineer(segmentKey);
   return m_LossEngineer->GetSegmentTendonElongation(segmentKey, ductIdx, endType);
}

Float64 pgsPsForceEng::GetGirderTendonAverageFrictionLoss(const CGirderKey& girderKey,DuctIndexType ductIdx) const
{
   CreateLossEngineer(girderKey);
   Float64 dfpF, dfpA;
   m_LossEngineer->GetGirderTendonAverageFrictionAndAnchorSetLoss(girderKey,ductIdx,&dfpF,&dfpA);
   return dfpF;
}

Float64 pgsPsForceEng::GetSegmentTendonAverageFrictionLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const
{
   CreateLossEngineer(segmentKey);
   Float64 dfpF, dfpA;
   m_LossEngineer->GetSegmentTendonAverageFrictionAndAnchorSetLoss(segmentKey, ductIdx, &dfpF, &dfpA);
   return dfpF;
}

Float64 pgsPsForceEng::GetGirderTendonAverageAnchorSetLoss(const CGirderKey& girderKey,DuctIndexType ductIdx) const
{
   CreateLossEngineer(girderKey);
   Float64 dfpF, dfpA;
   m_LossEngineer->GetGirderTendonAverageFrictionAndAnchorSetLoss(girderKey,ductIdx,&dfpF,&dfpA);
   return dfpA;
}

Float64 pgsPsForceEng::GetSegmentTendonAverageAnchorSetLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const
{
   CreateLossEngineer(segmentKey);
   Float64 dfpF, dfpA;
   m_LossEngineer->GetSegmentTendonAverageFrictionAndAnchorSetLoss(segmentKey, ductIdx, &dfpF, &dfpA);
   return dfpA;
}

const LOSSDETAILS* pgsPsForceEng::GetLosses(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx) const
{
   CreateLossEngineer(poi.GetSegmentKey());
   return m_LossEngineer->GetLosses(poi,intervalIdx);
}

const LOSSDETAILS* pgsPsForceEng::GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx) const
{
   CreateLossEngineer(poi.GetSegmentKey());
   return m_LossEngineer->GetLosses(poi,config,intervalIdx);
}

Float64 pgsPsForceEng::GetPjackMax(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,StrandIndexType nStrands) const
{
   GET_IFACE(ISegmentData,pSegmentData);
   const auto* pStrand = pSegmentData->GetStrandMaterial(segmentKey,strandType);
   ATLASSERT(pStrand != 0);

   return GetPjackMax(segmentKey,*pStrand,nStrands);
}

Float64 pgsPsForceEng::GetPjackMax(const CSegmentKey& segmentKey,const WBFL::Materials::PsStrand& strand,StrandIndexType nStrands) const
{
   GET_IFACE( ISpecification, pSpec );
   std::_tstring spec_name = pSpec->GetSpecification();

   GET_IFACE( ILibrary, pLib );
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   Float64 Pjack = 0.0;
   if ( pSpecEntry->CheckStrandStress(CSS_AT_JACKING) )
   {
      Float64 coeff;
      if ( strand.GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation )
      {
         coeff = pSpecEntry->GetStrandStressCoefficient(CSS_AT_JACKING,LOW_RELAX);
      }
      else
      {
         coeff = pSpecEntry->GetStrandStressCoefficient(CSS_AT_JACKING,STRESS_REL);
      }

      Float64 fpu;
      Float64 aps;

      fpu = strand.GetUltimateStrength();
      aps = strand.GetNominalArea();

      Float64 Fu = fpu * aps * nStrands;

      Pjack = coeff*Fu;
   }
   else
   {
      Float64 coeff;
      if ( strand.GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation )
      {
         coeff = pSpecEntry->GetStrandStressCoefficient(CSS_BEFORE_TRANSFER,LOW_RELAX);
      }
      else
      {
         coeff = pSpecEntry->GetStrandStressCoefficient(CSS_BEFORE_TRANSFER,STRESS_REL);
      }

      // fake up some data so losses are computed before transfer
      GET_IFACE(IPointOfInterest,pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vPoi);
      const pgsPointOfInterest& poi( vPoi.front() );

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType stressStrandsIntervalIdx = pIntervals->GetStressStrandInterval(segmentKey);
      Float64 loss = GetEffectivePrestressLoss(poi,pgsTypes::Permanent,stressStrandsIntervalIdx,pgsTypes::End,false/*don't apply elastic gain reduction*/,nullptr);

      Float64 fpu = strand.GetUltimateStrength();
      Float64 aps = strand.GetNominalArea();

      Pjack = (coeff*fpu + loss) * aps * nStrands;
   }

   return Pjack;
}

Float64 pgsPsForceEng::GetHoldDownForce(const CSegmentKey& segmentKey, bool bTotal, Float64* pSlope, pgsPointOfInterest* pPoi, const GDRCONFIG* pConfig) const
{
   GET_IFACE(IStrandGeometry, pStrandGeom);
   StrandIndexType Nh = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped, pConfig);
   if (0 < Nh)
   {
      GET_IFACE(IPointOfInterest,pIPoi);
      PoiList vPoi;
      pIPoi->GetPointsOfInterest(segmentKey, POI_HARPINGPOINT, &vPoi);
   
      // no hold down force if there aren't any harped strands
      if ( vPoi.size() == 0 )
      {
         if (pPoi)
         {
            *pPoi = pgsPointOfInterest(segmentKey, 0.0);
         }

         if (pSlope)
         {
            *pSlope = 0;
         }
         return 0;
      }

      const pgsPointOfInterest& poi(vPoi.front());
   
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType intervalIdx = pIntervals->GetStressStrandInterval(segmentKey);
      // The slope of the strands may be different at each harp point... need to compute the
      // hold down force for each harp point and return the maximum value

      Float64 s = Float64_Max; // slope associated with governing hold down force
      Float64 F = 0; // governing hold down force
      if (pPoi)
      {
         *pPoi = vPoi.front();
      }
      for (const pgsPointOfInterest& poi : vPoi)
      {
         Float64 harped = GetPrestressForce(poi,pgsTypes::Harped,intervalIdx,pgsTypes::Start,pgsTypes::tltMinimum,pConfig);

         // Adjust for slope
         Float64 slope;
         if (bTotal)
         {
            slope = pStrandGeom->GetAvgStrandSlope(poi, pConfig);
         }
         else
         {
            harped /= Nh; // per strand force

            // maximum hold down force is associated with the maximum strand slope
            slope = pStrandGeom->GetMaxStrandSlope(poi, pConfig);
         }


         Float64 Fhd;
         Fhd = harped / sqrt(1 + slope*slope);

         if (F < Fhd)
         {
            F = Fhd;
            s = slope;
            if (pPoi)
            {
               *pPoi = poi;
            }
         }
      }

      // NOTE: increase the force by some percentage to account for friction
      // in the hold down device. harped *= 1.05 (for a 5% increase)
      // See PCI BDM Example 9.1a,pg 9.1a-28
      // Also see LRFD 5.9.3.2.2a (pre2017: 5.9.5.2.2a)
      //
      // Do this computation outside of the loop for efficiency
      GET_IFACE(ISpecification, pSpec);
      GET_IFACE(ILibrary, pLib);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

      bool bCheck, bDesign;
      int holdDownForceType;
      Float64 maxHoldDownForce, friction;
      pSpecEntry->GetHoldDownForce(&bCheck, &bDesign, &holdDownForceType, &maxHoldDownForce, &friction);
      ATLASSERT(bTotal == (holdDownForceType == HOLD_DOWN_TOTAL));

      F *= (1 + friction);

      if (pSlope)
      {
         *pSlope = s;
      }
      return F;
   }
   else
   {
      if (pPoi)
      {
         *pPoi = pgsPointOfInterest(segmentKey, 0);
      }

      if (pSlope)
      {
         *pSlope = 0;
      }

      return 0;
   }
}

Float64 pgsPsForceEng::GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,pgsTypes::TransferLengthType xferLengthType,const GDRCONFIG* pConfig) const
{
   GET_IFACE(ISectionProperties, pSectProps);
   bool bIncludeElasticEffects = (pSectProps->GetSectionPropertiesMode() == pgsTypes::spmGross || intervalTime == pgsTypes::End ? true : false);
   return GetPrestressForce(poi, strandType, intervalIdx, intervalTime, bIncludeElasticEffects, xferLengthType, pConfig);
}

Float64 pgsPsForceEng::GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,bool bIncludeElasticEffects,pgsTypes::TransferLengthType xferLengthType,const GDRCONFIG* pConfig) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IStrandGeometry,pStrandGeom);
   Float64 Aps = pStrandGeom->GetStrandArea(poi,intervalIdx,strandType,pConfig);
   if (IsZero(Aps))
   {
      // no strand, no force
      return 0.0;
   }

   Float64 P = 0;
   if (strandType == pgsTypes::Permanent)
   {
      Float64 Ps = GetPrestressForce(poi, pgsTypes::Straight, intervalIdx, intervalTime, bIncludeElasticEffects, xferLengthType, pConfig);
      Float64 Ph = GetPrestressForce(poi, pgsTypes::Harped, intervalIdx, intervalTime, bIncludeElasticEffects, xferLengthType, pConfig);
      P = Ps + Ph;
   }
   else
   {
      GET_IFACE(ISegmentData, pSegmentData);
      const auto* pStrand = pSegmentData->GetStrandMaterial(segmentKey, strandType);

      Float64 fpe = GetEffectivePrestress(poi, strandType, intervalIdx, intervalTime, bIncludeElasticEffects, true/*apply elastic gain reduction*/, pConfig); // this fpj - loss + gain, without adjustment

      // Reduce for transfer effect (no transfer effect if the strands aren't released)
      GET_IFACE(IIntervals, pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      Float64 adjust = 1.0;
      if (releaseIntervalIdx <= intervalIdx)
      {
         GET_IFACE(IPretensionForce, pPSForce);
         adjust = pPSForce->GetTransferLengthAdjustment(poi, strandType, xferLengthType, pConfig);
      }

      P = adjust*Aps*fpe;
   }

   return P;
}

Float64 pgsPsForceEng::GetEffectivePrestress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig) const
{
   return GetEffectivePrestress(poi,strandType,intervalIdx,intervalTime,true/*include elastic effects*/,true/*apply elastic gain reduction*/, pConfig);
}

Float64 pgsPsForceEng::GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex, bool bIncludeElasticEffects, const GDRCONFIG* pConfig) const
{
   GET_IFACE(IPointOfInterest, pPoi);
   if (pPoi->IsOffSegment(poi))
   {
      return 0;
   }

   if (strandType == pgsTypes::Permanent)
   {
      // must handle straight and harped separately because they could have different strand sizes
      Float64 Ps = GetPrestressForceWithLiveLoad(poi, pgsTypes::Straight, limitState, vehicleIndex, bIncludeElasticEffects, pConfig);
      Float64 Ph = GetPrestressForceWithLiveLoad(poi, pgsTypes::Harped, limitState, vehicleIndex, bIncludeElasticEffects, pConfig);
      return Ps + Ph;
   }

   ATLASSERT(strandType != pgsTypes::Permanent); // can't use permanent after this point

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType liveLoadIntervalIdx;
   if (IsRatingLimitState(limitState))
   {
      liveLoadIntervalIdx = pIntervals->GetLoadRatingInterval();
   }
   else
   {
      liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   }

   GET_IFACE(IStrandGeometry, pStrandGeom);
   Float64 Aps = pStrandGeom->GetStrandArea(poi, liveLoadIntervalIdx,strandType,pConfig);
   if (IsZero(Aps))
   {
      // no strand, no force
      return 0;
   }

   GET_IFACE(ISegmentData,pSegmentData );
   const auto* pStrand = pSegmentData->GetStrandMaterial(segmentKey,strandType);

   GET_IFACE(IPretensionForce, pPSForce);
   Float64 fpe = GetEffectivePrestressWithLiveLoad(poi,strandType,limitState,vehicleIndex,bIncludeElasticEffects,true/*apply elastic gain reduction*/, pConfig); // this fpj - loss + gain, without adjustment
   // The use of this Prestress force (with live load) is for a stress analysis in a service limit state, for this reason, use the minimum transfer length.
   Float64 adjust = pPSForce->GetTransferLengthAdjustment(poi, strandType, pgsTypes::tltMinimum, pConfig);

   Float64 P = adjust*Aps*fpe;

   return P;
}

Float64 pgsPsForceEng::GetEffectivePrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime, bool bApplyElasticGainReduction,const GDRCONFIG* pConfig) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      return 0;
   }

   Float64 time_dependent_loss = GetTimeDependentLosses(poi,strandType,intervalIdx,intervalTime,pConfig);
   Float64 instantaneous_effects = GetInstantaneousEffects(poi,strandType,intervalIdx,intervalTime, bApplyElasticGainReduction,pConfig);
   Float64 effective_loss = time_dependent_loss - instantaneous_effects;
   return effective_loss;
}

Float64 pgsPsForceEng::GetEffectivePrestressLossWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx,bool bIncludeElasticEffects, bool bApplyElasticGainReduction, const GDRCONFIG* pConfig) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      return 0;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx;
   if ( IsRatingLimitState(limitState) )
   {
      liveLoadIntervalIdx = pIntervals->GetLoadRatingInterval();
   }
   else
   {
      liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   }
   pgsTypes::IntervalTimeType intervalTime = pgsTypes::End;
   Float64 time_dependent_loss = GetTimeDependentLosses(poi,strandType,liveLoadIntervalIdx,intervalTime,pConfig);
   Float64 instantaneous_effects = 0;
   if (bIncludeElasticEffects)
   {
      instantaneous_effects = GetInstantaneousEffectsWithLiveLoad(poi, strandType, limitState, bApplyElasticGainReduction, vehicleIdx, pConfig);
   }
   Float64 effective_loss = time_dependent_loss - instantaneous_effects;
   return effective_loss;
}

Float64 pgsPsForceEng::GetTimeDependentLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      return 0;
   }

   GET_IFACE(ILosses,pLosses);
   const LOSSDETAILS* pDetails;
#pragma Reminder("REVIEW = why does one version of GetLosses use the intervalIdx and the other doesnt?")
   if ( pConfig )
   {
      pDetails = pLosses->GetLossDetails(poi,*pConfig);
   }
   else
   {
      pDetails = pLosses->GetLossDetails(poi,intervalIdx);
   }

   return GetTimeDependentLosses(poi,strandType,intervalIdx,intervalTime,pConfig,pDetails);
}

Float64 pgsPsForceEng::GetTimeDependentLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig,const LOSSDETAILS* pDetails) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      return 0;
   }

   // NOTE: Losses are just time-dependent change in prestress force.
   // Losses do not include elastic effects include elastic shortening or elastic gains due to external loads
   //
   // fpe = fpj - loss + gains

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // if losses were computed with the time-step method
   // look up change in stress in strands due to creep, shrinkage, and relaxation
   if ( pDetails->LossMethod == pgsTypes::TIME_STEP )
   {
      if ( intervalIdx == 0 && intervalTime == pgsTypes::Start )
      {
         return 0; // wanting losses at the start of the first interval.. nothing has happened yet
      }

      // time step losses are computed for the end of an interval
      IntervalIndexType theIntervalIdx = intervalIdx;
      switch(intervalTime)
      {
      case pgsTypes::Start:
         theIntervalIdx--; // losses at start of this interval are equal to losses at end of previous interval
         break;

      case pgsTypes::Middle:
         // drop through so we just use the end
      case pgsTypes::End:
         break; // do nothing... theIntervalIdx is correct
      }

#if !defined LUMP_STRANDS
      GET_IFACE(IStrandGeometry,pStrandGeom);
#endif

      if ( strandType == pgsTypes::Permanent) 
      {
#if defined LUMP_STRANDS
         GET_IFACE(IStrandGeometry, pStrandGeom);
         std::array<Float64, 2> Aps{ pStrandGeom->GetStrandArea(poi,theIntervalIdx,pgsTypes::Straight,pConfig),pStrandGeom->GetStrandArea(poi,theIntervalIdx,pgsTypes::Harped,pConfig) };
         Float64 A = std::accumulate(Aps.begin(), Aps.end(), 0.0);
         if (IsZero(A))
         {
            return 0;
         }

         Float64 dfpe_creep_straight      = 0;
         Float64 dfpe_shrinkage_straight  = 0;
         Float64 dfpe_relaxation_straight = 0;
         Float64 dfpe_creep_harped      = 0;
         Float64 dfpe_shrinkage_harped  = 0;
         Float64 dfpe_relaxation_harped = 0;
         for ( IntervalIndexType i = 0; i <= theIntervalIdx; i++ )
         {
            dfpe_creep_straight += pDetails->TimeStepDetails[i].Strands[pgsTypes::Straight].dfpei[pgsTypes::pftCreep];
            dfpe_creep_harped   += pDetails->TimeStepDetails[i].Strands[pgsTypes::Harped  ].dfpei[pgsTypes::pftCreep];

            dfpe_shrinkage_straight += pDetails->TimeStepDetails[i].Strands[pgsTypes::Straight].dfpei[pgsTypes::pftShrinkage];
            dfpe_shrinkage_harped   += pDetails->TimeStepDetails[i].Strands[pgsTypes::Harped  ].dfpei[pgsTypes::pftShrinkage];

            dfpe_relaxation_straight += pDetails->TimeStepDetails[i].Strands[pgsTypes::Straight].dfpei[pgsTypes::pftRelaxation];
            dfpe_relaxation_harped   += pDetails->TimeStepDetails[i].Strands[pgsTypes::Harped  ].dfpei[pgsTypes::pftRelaxation];
         }

         return -(Aps[pgsTypes::Straight]*(dfpe_creep_straight + dfpe_shrinkage_straight + dfpe_relaxation_straight) + Aps[pgsTypes::Harped] *(dfpe_creep_harped + dfpe_shrinkage_harped + dfpe_relaxation_harped))/(A);
#else
#pragma Reminder("IMPLEMENT")
#endif
      }
      else
      {
#if defined LUMP_STRANDS
         Float64 dfpe_creep      = 0;
         Float64 dfpe_shrinkage  = 0;
         Float64 dfpe_relaxation = 0;
         for ( IntervalIndexType i = 0; i <= theIntervalIdx; i++ )
         {
            dfpe_creep += pDetails->TimeStepDetails[i].Strands[strandType].dfpei[pgsTypes::pftCreep];

            dfpe_shrinkage += pDetails->TimeStepDetails[i].Strands[strandType].dfpei[pgsTypes::pftShrinkage];

            dfpe_relaxation += pDetails->TimeStepDetails[i].Strands[strandType].dfpei[pgsTypes::pftRelaxation];
         }

         return -(dfpe_creep + dfpe_shrinkage + dfpe_relaxation);
#else
#pragma Reminder("IMPLEMENT")
#endif
      }
   }
   else
   {
      // some method other than Time Step
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType stressStrandIntervalIdx  = pIntervals->GetStressStrandInterval(segmentKey);
      IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType liftSegmentIntervalIdx   = pIntervals->GetLiftSegmentInterval(segmentKey);
      IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType haulSegmentIntervalIdx   = pIntervals->GetHaulSegmentInterval(segmentKey);
      IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetErectSegmentInterval(segmentKey);
      IntervalIndexType tsInstallationIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
      IntervalIndexType tsRemovalIntervalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
      IntervalIndexType noncompositeCastingIntervalIdx = pIntervals->GetLastNoncompositeInterval();
      IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();
      IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
      IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
      IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

      GET_IFACE(IBridge,pBridge);
      bool bIsFutureOverlay = pBridge->IsFutureOverlay();

      Float64 loss = 0;
      if ( intervalIdx == stressStrandIntervalIdx )
      {
         if ( intervalTime == pgsTypes::Start )
         {
            loss = 0.0;
         }
         else if ( intervalTime == pgsTypes::Middle )
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_BeforeTransfer();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_BeforeTransfer();
            }

            loss /= 2.0;
         }
         else if ( intervalTime == pgsTypes::End )
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_BeforeTransfer();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_BeforeTransfer();
            }
         }
      }
      else if ( intervalIdx == releaseIntervalIdx || intervalIdx == storageIntervalIdx )
      {
         if ( intervalTime == pgsTypes::Start )
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_BeforeTransfer();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_BeforeTransfer();
            }
         }
         else
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_AfterTransfer();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_AfterTransfer();
            }
         }
      }
      else if ( intervalIdx == liftSegmentIntervalIdx )
      {
         if ( strandType == pgsTypes::Temporary )
         {
            loss = pDetails->pLosses->TemporaryStrand_AtLifting();
         }
         else
         {
            loss = pDetails->pLosses->PermanentStrand_AtLifting();
         }
      }
      else if ( intervalIdx == haulSegmentIntervalIdx )
      {
         if ( strandType == pgsTypes::Temporary )
         {
            loss = pDetails->pLosses->TemporaryStrand_AtShipping();
         }
         else
         {
            loss = pDetails->pLosses->PermanentStrand_AtShipping();
         }
      }
      else if ( intervalIdx == tsInstallationIntervalIdx )
      {
         if ( intervalTime == pgsTypes::Start )
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_AtShipping();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_AtShipping();
            }
         }
         else
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_AfterTemporaryStrandInstallation();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_AfterTemporaryStrandInstallation();
            }
         }
      }
      else if ( intervalIdx == erectSegmentIntervalIdx )
      {
         if ( strandType == pgsTypes::Temporary )
         {
            loss = pDetails->pLosses->TemporaryStrand_BeforeTemporaryStrandRemoval();
         }
         else
         {
            loss = pDetails->pLosses->PermanentStrand_BeforeTemporaryStrandRemoval();
         }
      }
      else if ( intervalIdx == tsRemovalIntervalIdx )
      {
         if ( intervalTime == pgsTypes::Start )
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_BeforeTemporaryStrandRemoval();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_BeforeTemporaryStrandRemoval();
            }
         }
         else
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_AfterTemporaryStrandRemoval();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_AfterTemporaryStrandRemoval();
            }
         }
      }
      else if ( intervalIdx == noncompositeCastingIntervalIdx || (intervalIdx == compositeIntervalIdx && compositeIntervalIdx != railingSystemIntervalIdx) )
      {
         if ( intervalTime == pgsTypes::Start )
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_AfterTemporaryStrandRemoval();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_AfterTemporaryStrandRemoval();
            }
         }
         else
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_AfterDeckPlacement();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_AfterDeckPlacement();
            }
         }
      }
      else
      {
         if ( strandType == pgsTypes::Temporary )
         {
            loss = pDetails->pLosses->TemporaryStrand_Final();
         }
         else
         {
            loss = pDetails->pLosses->PermanentStrand_Final();
         }
      }

      return loss;
   }
}

Float64 pgsPsForceEng::GetInstantaneousEffects(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime, bool bApplyElasticGainReduction,const GDRCONFIG* pConfig) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      return 0;
   }

   GET_IFACE(ILosses,pLosses);
   const LOSSDETAILS* pDetails;
#pragma Reminder("REVIEW = why does one version of GetLossDetails use the intervalIdx and the other doesnt?")
   if ( pConfig )
   {
      pDetails = pLosses->GetLossDetails(poi,*pConfig);
   }
   else
   {
      pDetails = pLosses->GetLossDetails(poi,intervalIdx);
   }

   return GetInstantaneousEffects(poi,strandType,intervalIdx,intervalTime, bApplyElasticGainReduction,pConfig,pDetails);
}

Float64 pgsPsForceEng::GetInstantaneousEffects(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime, bool bApplyElasticGainReduction,const GDRCONFIG* pConfig,const LOSSDETAILS* pDetails) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 gain = 0;

   if ( pDetails->LossMethod == pgsTypes::GENERAL_LUMPSUM )
   {
      // all changes to effective prestress are included in general lump sum losses
      // since the TimeDependentEffects returns the user input value for loss,
      // we return 0 here.
      return 0;
   }
   else if ( pDetails->LossMethod == pgsTypes::TIME_STEP )
   {
      // effective loss = time-dependent loss - elastic effects
      // effective loss is on the strand objects, just look it up
      // elastic effects = time-dependent loss - effective loss
      if ( intervalIdx == 0 && intervalTime == pgsTypes::Start )
      {
         return 0; // wanting losses at the start of the first interval.. nothing has happened yet
      }

      Float64 time_dependent_loss = GetTimeDependentLosses(poi,strandType,intervalIdx,intervalTime,pConfig,pDetails);

      // time step losses are computed for the end of an interval
      IntervalIndexType theIntervalIdx = intervalIdx;
      switch(intervalTime)
      {
      case pgsTypes::Start:
         theIntervalIdx--; // losses at start of this interval are equal to losses at end of previous interval
         break;

      case pgsTypes::Middle:
         // drop through so we just use the end
      case pgsTypes::End:
         break; // do nothing... theIntervalIdx is correct
      }

      if ( strandType == pgsTypes::Permanent )
      {
#if defined LUMP_STRANDS
         GET_IFACE(IStrandGeometry, pStrandGeom);
         std::array<Float64, 2> Aps{ pStrandGeom->GetStrandArea(poi,theIntervalIdx,pgsTypes::Straight,pConfig),pStrandGeom->GetStrandArea(poi,theIntervalIdx,pgsTypes::Harped,pConfig) };
         Float64 A = std::accumulate(Aps.begin(), Aps.end(), 0.0);
         if (IsZero(A))
         {
            return 0;
         }

         Float64 effective_loss = std::inner_product(Aps.begin(), Aps.end(), pDetails->TimeStepDetails[theIntervalIdx].Strands.begin(), 0.0, std::plus<Float64>(), [](const auto& aps, const auto& strand) {return aps*strand.loss; });
         effective_loss /= A;

#if defined _DEBUG
         Float64 _effective_loss = (Aps[pgsTypes::Straight]*pDetails->TimeStepDetails[theIntervalIdx].Strands[pgsTypes::Straight].loss
                                  + Aps[pgsTypes::Harped]  *pDetails->TimeStepDetails[theIntervalIdx].Strands[pgsTypes::Harped  ].loss)/(A);

         ATLASSERT(IsEqual(effective_loss, _effective_loss));
#endif

         return time_dependent_loss - effective_loss;
#else
#pragma Reminder("IMPLEMENT")
#endif
      }
      else
      {
#if defined LUMP_STRANDS
         Float64 effective_loss = pDetails->TimeStepDetails[theIntervalIdx].Strands[strandType].loss;
         return time_dependent_loss - effective_loss;
#else
#pragma Reminder("IMPLEMENT")
#endif
      }
   }
   else
   {
      // some method other than Time Step
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType stressStrandIntervalIdx  = pIntervals->GetStressStrandInterval(segmentKey);
      IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType liftSegmentIntervalIdx   = pIntervals->GetLiftSegmentInterval(segmentKey);
      IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType haulSegmentIntervalIdx   = pIntervals->GetHaulSegmentInterval(segmentKey);
      IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetErectSegmentInterval(segmentKey);
      IntervalIndexType tsInstallationIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
      IntervalIndexType tsRemovalIntervalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
      IntervalIndexType noncompositeCastingIntervalIdx      = pIntervals->GetLastNoncompositeInterval();
      IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
      IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
      IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

      Float64 gain = 0;

      if ( releaseIntervalIdx <= intervalIdx )
      {
         if ( intervalIdx == releaseIntervalIdx && intervalTime == pgsTypes::Start )
         {
            gain += 0;
         }
         else
         {
            if ( strandType == pgsTypes::Temporary )
            {
               gain += -pDetails->pLosses->TemporaryStrand_ElasticShorteningLosses();
            }
            else
            {
               gain += -pDetails->pLosses->PermanentStrand_ElasticShorteningLosses();
            }
         }
      }

      if ( tsInstallationIntervalIdx <= intervalIdx )
      {
         if ( strandType == pgsTypes::Temporary )
         {
            GET_IFACE(ISegmentData,pSegmentData);
            const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

            if ( pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned )
            {
               gain += -(pDetails->pLosses->FrictionLoss() + pDetails->pLosses->AnchorSetLoss() + pDetails->pLosses->GetDeltaFptAvg());
            }
         }
         else
         {
            gain += -pDetails->pLosses->GetDeltaFpp();
         }
      }

      if ( tsRemovalIntervalIdx <= intervalIdx )
      {
         if ( strandType != pgsTypes::Temporary && intervalTime != pgsTypes::Start )
         {
            gain += -pDetails->pLosses->GetDeltaFptr();
         }
      }

      if (noncompositeCastingIntervalIdx <= intervalIdx )
      {
         if ( intervalIdx == noncompositeCastingIntervalIdx && intervalTime == pgsTypes::Start )
         {
            gain += 0;
         }
         else
         {
            if ( strandType == pgsTypes::Temporary )
            {
               gain += 0;
            }
            else
            {
               gain += pDetails->pLosses->ElasticGainDueToDeckPlacement(bApplyElasticGainReduction);
            }
         }
      }

      if ( railingSystemIntervalIdx <= intervalIdx )
      {
         if ( railingSystemIntervalIdx == intervalIdx && intervalTime == pgsTypes::Start )
         {
            gain += 0;
         }
         else
         {
            if ( strandType == pgsTypes::Temporary )
            {
               gain += 0;
            }
            else
            {
               gain += pDetails->pLosses->ElasticGainDueToSIDL(bApplyElasticGainReduction);
            }
         }
      }

      return gain;
   }
}

Float64 pgsPsForceEng::GetInstantaneousEffectsWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, bool bApplyElasticGainReduction, VehicleIndexType vehicleIdx, const GDRCONFIG* pConfig) const
{
   GET_IFACE(ILosses,pLosses);
   const LOSSDETAILS* pDetails;
   if ( pConfig )
   {
      pDetails = pLosses->GetLossDetails(poi,*pConfig);
   }
   else
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx;
      if ( IsRatingLimitState(limitState) )
      {
         liveLoadIntervalIdx = pIntervals->GetLoadRatingInterval();
      }
      else
      {
         liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      }
      pDetails = pLosses->GetLossDetails(poi,liveLoadIntervalIdx);
   }

   return GetInstantaneousEffectsWithLiveLoad(poi,strandType,limitState, bApplyElasticGainReduction,vehicleIdx,pConfig,pDetails);
}

Float64 pgsPsForceEng::GetInstantaneousEffectsWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, bool bApplyElasticGainReduction,VehicleIndexType vehicleIdx,const GDRCONFIG* pConfig,const LOSSDETAILS* pDetails) const
{
   ATLASSERT(!IsStrengthLimitState(limitState)); // limit state must be servie or fatigue

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx;
   if ( IsRatingLimitState(limitState) )
   {
      liveLoadIntervalIdx = pIntervals->GetLoadRatingInterval();
   }
   else
   {
      liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   }

   pgsTypes::IntervalTimeType intervalTime = pgsTypes::End;

   Float64 gain = GetInstantaneousEffects(poi,strandType,liveLoadIntervalIdx,intervalTime, bApplyElasticGainReduction,pConfig,pDetails);
   gain += GetElasticGainDueToLiveLoad(poi, strandType, limitState, vehicleIdx, true/*include live load factor*/, bApplyElasticGainReduction,pConfig, pDetails);
   return gain;
}

Float64 pgsPsForceEng::GetEffectivePrestress(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, bool bIncludeElasticEffects, bool bApplyElasticGainReduction, const GDRCONFIG* pConfig) const
{
   GET_IFACE(IPointOfInterest, pPoi);
   if (pPoi->IsOffSegment(poi))
   {
      return 0;
   }

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx        = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType tsStressingIntervalIdx = pIntervals->GetTemporaryStrandStressingInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx      = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);

   GET_IFACE(IStrandGeometry, pStrandGeom);
   StrandIndexType N = pStrandGeom->GetStrandCount(segmentKey, strandType, pConfig);

   if (strandType == pgsTypes::Temporary &&
      (N == 0 ||
            intervalIdx < tsStressingIntervalIdx ||
            tsRemovalIntervalIdx < intervalIdx      || 
           (tsRemovalIntervalIdx == intervalIdx && intervalTime != pgsTypes::Start) 
         )
      )
   {
      // if we are after temporary strand stress and Nt is zero... the result is zero
      return 0;
   }

   Float64 fpj = pStrandGeom->GetJackingStress(segmentKey, strandType, pConfig);

   if (IsZero(fpj))
   {
      // no strand, no jack force... the strand stress is 0
      return 0;
   }

   // Compute the requested strand stress
   Float64 loss;
   if (bIncludeElasticEffects)
   {
      loss = GetEffectivePrestressLoss(poi, strandType, intervalIdx, intervalTime, bApplyElasticGainReduction, pConfig);
   }
   else
   {
      loss = GetTimeDependentLosses(poi, strandType, intervalIdx, intervalTime, pConfig);
   }
   Float64 fpe = fpj - loss;

   ATLASSERT(0 <= fpe); // strand stress must be greater than or equal to zero.

   return fpe;
}

Float64 pgsPsForceEng::GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx, bool bIncludeElasticEffects, bool bApplyElasticGainReduction, const GDRCONFIG* pConfig) const
{
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType liveLoadIntervalIdx;
   if (IsRatingLimitState(limitState))
   {
      liveLoadIntervalIdx = pIntervals->GetLoadRatingInterval();
   }
   else
   {
      liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   }

   Float64 fpe = GetEffectivePrestress(poi, strandType, liveLoadIntervalIdx, pgsTypes::End, bIncludeElasticEffects, bApplyElasticGainReduction, pConfig);

   if (IsZero(fpe)) // this happens where there are no strands, or the strands aren't jacked
      return 0;

   GET_IFACE(ILosses, pLosses);
   const LOSSDETAILS* pDetails;
   if (pConfig)
   {
      pDetails = pLosses->GetLossDetails(poi, *pConfig);
   }
   else
   {
      pDetails = pLosses->GetLossDetails(poi, liveLoadIntervalIdx);
   }

   if (bIncludeElasticEffects)
   {
      Float64 dfpLL = GetElasticGainDueToLiveLoad(poi, strandType, limitState, vehicleIdx, true/*include live load factor*/, bApplyElasticGainReduction, pConfig, pDetails);
      fpe += dfpLL;
   }

   return fpe;
}

Float64 pgsPsForceEng::GetElasticGainDueToLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex, bool bIncludeLiveLoadFactor, bool bApplyElasticGainReduction, const GDRCONFIG* pConfig, const LOSSDETAILS* pDetails) const
{
   ATLASSERT(!IsStrengthLimitState(limitState)); // limit state must be service or fatigue

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType liveLoadIntervalIdx;
   Float64 gLL = 1.0;
   if (IsRatingLimitState(limitState))
   {
      liveLoadIntervalIdx = pIntervals->GetLoadRatingInterval();

      if (bIncludeLiveLoadFactor)
      {
         GET_IFACE(IRatingSpecification, pRatingSpec);
         gLL = pRatingSpec->GetLiveLoadFactor(limitState, true);
      }
   }
   else
   {
      liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

      if (bIncludeLiveLoadFactor)
      {
         GET_IFACE(ILoadFactors, pLoadFactors);
         const CLoadFactors* pLF = pLoadFactors->GetLoadFactors();
         gLL = pLF->GetLLIMMax(limitState);
      }
   }
   pgsTypes::IntervalTimeType intervalTime = pgsTypes::End;

   GET_IFACE(IProductForces, pProductForces);
   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);
   pgsTypes::LiveLoadType llType = LiveLoadTypeFromLimitState(limitState);
   Float64 Mmin, Mmax;

   if (vehicleIndex == INVALID_INDEX)
   {
      pProductForces->GetLiveLoadMoment(liveLoadIntervalIdx, llType, poi, bat, true/*include impact*/, true/*include LLDF*/, &Mmin, &Mmax);
   }
   else
   {
      pProductForces->GetVehicularLiveLoadMoment(liveLoadIntervalIdx, llType, vehicleIndex, poi, bat, true/*include impact*/, true/*include LLDF*/, &Mmin, &Mmax);
   }


   Float64 gain = 0;
   if (pDetails->LossMethod == pgsTypes::TIME_STEP)
   {
#if defined LUMP_STRANDS
      GET_IFACE(IStrandGeometry, pStrandGeom);
      GET_IFACE(ISectionProperties, pSectProps);
      Float64 e = pStrandGeom->GetEccentricity(liveLoadIntervalIdx, poi, strandType).Y();
      Float64 Ixx = pSectProps->GetIxx(liveLoadIntervalIdx, poi);
      gain = gLL * Mmax * e / Ixx;
#else
      GET_IFACE(IStrandGeometry, pStrandGeom);
      for (int i = 0; i < 2; i++) // straight and harped only, temp strands have been removed
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
         StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey, strandType);
         for (StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
         {
            THIS IS NOT CORRECT FOR INDIVIDUAL STRANDS
            const TIME_STEP_STRAND& strand(pDetails->TimeStepDetails[liveLoadIntervalIdx].Strands[strandType][strandIdx]);
            gain += gLL*strand.dFllMax;
         }
      }
#endif // LUMP_STRANDS
   }
   else
   {
      Float64 llGain;
      if (pDetails->LossMethod == pgsTypes::GENERAL_LUMPSUM)
      {
         llGain = 0.0;
      }
      else
      {
         GET_IFACE(ISpecification, pSpec);
         GET_IFACE(ILibrary, pLibrary);
         const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());
         Float64 K_liveload = pSpecEntry->GetLiveLoadElasticGain();

         if ((pDetails->LossMethod == pgsTypes::AASHTO_LUMPSUM_2005 ||
             pDetails->LossMethod == pgsTypes::AASHTO_REFINED_2005 ||
             pDetails->LossMethod == pgsTypes::WSDOT_LUMPSUM_2005 ||
             pDetails->LossMethod == pgsTypes::WSDOT_REFINED_2005)
             &&
             !bApplyElasticGainReduction)
         {
           K_liveload = 1.0;
         }

         llGain = pDetails->pLosses->ElasticGainDueToLiveLoad(Mmax);
         llGain *= K_liveload;
      }
      gain += gLL*llGain;
   }
   return gain;
}
