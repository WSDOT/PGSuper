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

#include "stdafx.h"
#include "resource.h"
#include "TimeStepLossEngineer.h"
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>
#include <PgsExt\LoadFactors.h>

#include <PgsExt\ReportStyleHolder.h>
#include <Reporting\ReportNotes.h>
#include <PgsExt\GirderLabel.h>

#include <Details.h>

#include <numeric>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTimeStepLossEngineer
HRESULT CTimeStepLossEngineer::FinalConstruct()
{
   m_Bat = pgsTypes::ContinuousSpan;
   return S_OK;
}

void CTimeStepLossEngineer::FinalRelease()
{
}

void CTimeStepLossEngineer::SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;
}

const LOSSDETAILS* CTimeStepLossEngineer::GetLosses(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx)
{
   CGirderKey girderKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount(girderKey);
   if ( intervalIdx == INVALID_INDEX )
   {
      intervalIdx = nIntervals-1;
   }

   IntervalIndexType nIntervalsToBeAnalyzed = intervalIdx+1;

   // Have losses been computed for this girder?
   std::map<CGirderKey,LOSSES>::iterator found;
   found = m_Losses.find( girderKey );
   if ( found == m_Losses.end() )
   {
      // No, compute them now
      ComputeLosses(girderKey,intervalIdx);

      found = m_Losses.find( girderKey );
      ATLASSERT(found != m_Losses.end());
   }
   else
   {
      // Yes! Have losses been computed upto and including the requested interval?
      // Losses at all POI are computed to the same interval, so check the first loss record
      // to see how many intervals have been analyzed
      LOSSES& losses = found->second;
      SectionLossContainer::iterator iter = losses.SectionLosses.begin();
      LOSSDETAILS& details = iter->second;
      IntervalIndexType nIntervalsAnalyzed = details.TimeStepDetails.size();
      ATLASSERT(0 < nIntervalsAnalyzed);
      if ( nIntervalsAnalyzed < nIntervalsToBeAnalyzed )
      {
         ComputeLosses(girderKey,intervalIdx,&losses);
      }
   }

   SectionLossContainer& losses(found->second.SectionLosses);
   SectionLossContainer::iterator poiFound = losses.find(poi);

   if ( poiFound == losses.end() )
   {
      ATLASSERT( poi.GetID() == INVALID_ID || 
                 poi.IsHarpingPoint()      || 
                 poi.HasAttribute(POI_CRITSECTSHEAR1) || 
                 poi.HasAttribute(POI_CRITSECTSHEAR2) || 
                 poi.HasAttribute(POI_LIFT_SEGMENT) ||
                 poi.HasAttribute(POI_HAUL_SEGMENT)
                );
      // losses were not computed at the POI we are looking for. This happens for a couple of reasons.
      // 1) The POI was created on the fly (its ID is INVALID_ID)
      // 2) Harp Points -> we tweak the position of the harp point so we get the results on the side
      //    with the sloped strands
      // 3) Critical Section for Shear -> We don't know where these locations are prior to computing losses. Losses
      //    are needed to compute this locations
      // 4) Lifting and Hauling POI - these change with changing locations of the pick and bunk points

      // approximate the losses at the poi by using the losses at the nearest poi
      // (could use linear interpolation, however, take a look at the LOSSDETAILS struct...
      // it would be a royal pain to interpolate all the values)
      SectionLossContainer::iterator iter1(losses.begin());
      SectionLossContainer::iterator iter2(losses.begin());
      iter2++;
      SectionLossContainer::iterator end(losses.end());
      for ( ; iter2 != end; iter1++, iter2++ )
      {
         const pgsPointOfInterest& poi1(iter1->first);
         const pgsPointOfInterest& poi2(iter2->first);
         if ( poi1 < poi && poi < poi2 )
         {
            if ( poi.GetDistFromStart() - poi1.GetDistFromStart() < poi2.GetDistFromStart() - poi.GetDistFromStart() )
            {
               // poi is closer to poi1 than poi2
               const LOSSDETAILS& details = iter1->second;
               return &details;
            }
            else
            {
               // poi is closer to poi2 than poi1
               const LOSSDETAILS& details = iter2->second;
               return &details;
            }
         }
      }
   }

   ATLASSERT( poiFound != losses.end() );
   const LOSSDETAILS* pLossDetails = &(*poiFound).second;
   ATLASSERT(intervalIdx < pLossDetails->TimeStepDetails.size());
   return pLossDetails;
}

const LOSSDETAILS* CTimeStepLossEngineer::GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx)
{
   ATLASSERT(false); // not doing design with Time Step method... therefore this should never be called
   return NULL;
}

void CTimeStepLossEngineer::ClearDesignLosses()
{
   ATLASSERT(false); // should not get called... not doing design with time-step method
}

const ANCHORSETDETAILS* CTimeStepLossEngineer::GetAnchorSetDetails(const CGirderKey& girderKey,DuctIndexType ductIdx)
{
   ComputeLosses(girderKey,0);
   std::map<CGirderKey,LOSSES>::const_iterator found;
   found = m_Losses.find( girderKey );
   ATLASSERT(found != m_Losses.end());
   return &(*found).second.AnchorSet[ductIdx];
}

Float64 CTimeStepLossEngineer::GetElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType)
{
#pragma Reminder("IMPLEMENT CTimeStepLossEngineer::GetElongation()")
   // Getting elongation is an important calculation for post-tensioning operations
   ATLASSERT(false);
   return 0;
}

void CTimeStepLossEngineer::GetAverageFrictionAndAnchorSetLoss(const CGirderKey& girderKey,DuctIndexType ductIdx,Float64* pfpF,Float64* pfpA)
{
   CTendonKey tendonKey(girderKey,ductIdx);

   std::map<CTendonKey,std::pair<Float64,Float64>>::iterator found = m_AvgFrictionAndAnchorSetLoss.find(tendonKey);
   if ( found == m_AvgFrictionAndAnchorSetLoss.end() )
   {
      ComputeLosses(girderKey,0);
      found = m_AvgFrictionAndAnchorSetLoss.find(tendonKey);
      ATLASSERT(found != m_AvgFrictionAndAnchorSetLoss.end());
   }

   *pfpF = found->second.first;
   *pfpA = found->second.second;
}

void CTimeStepLossEngineer::ComputeLosses(const CGirderKey& girderKey,IntervalIndexType endAnalysisIntervalIdx)
{
   std::map<CGirderKey,LOSSES>::const_iterator found;
   found = m_Losses.find( girderKey );
   if ( found == m_Losses.end() )
   {
      LOSSES losses;
      ComputeLosses(girderKey,endAnalysisIntervalIdx,&losses);
      m_Losses.insert(std::make_pair(girderKey,losses)); 
   }
}

void CTimeStepLossEngineer::ComputeLosses(const CGirderKey& girderKey,IntervalIndexType endAnalysisIntervalIdx,LOSSES* pLosses)
{
   // Get frequently used interfaces
   // NOTE: We can't get this interfaces and hold them for the lifetime of this object
   // because it creates circular references. As a result there are massive memory leaks
   // Get them here, do the full timestep analysis, then release them.
   m_pBroker->GetInterface(IID_IProgress,          (IUnknown**)&m_pProgress);
   m_pBroker->GetInterface(IID_IBridgeDescription, (IUnknown**)&m_pBridgeDesc);
   m_pBroker->GetInterface(IID_IBridge,            (IUnknown**)&m_pBridge);
   m_pBroker->GetInterface(IID_IStrandGeometry,    (IUnknown**)&m_pStrandGeom);
   m_pBroker->GetInterface(IID_ITendonGeometry,    (IUnknown**)&m_pTendonGeom);
   m_pBroker->GetInterface(IID_IIntervals,         (IUnknown**)&m_pIntervals);
   m_pBroker->GetInterface(IID_ISectionProperties, (IUnknown**)&m_pSectProp);
   m_pBroker->GetInterface(IID_IGirder,            (IUnknown**)&m_pGirder);
   m_pBroker->GetInterface(IID_IMaterials,         (IUnknown**)&m_pMaterials);
   m_pBroker->GetInterface(IID_IPretensionForce,   (IUnknown**)&m_pPSForce);
   m_pBroker->GetInterface(IID_IPosttensionForce,  (IUnknown**)&m_pPTForce);
   m_pBroker->GetInterface(IID_ILossParameters,    (IUnknown**)&m_pLossParams);
   m_pBroker->GetInterface(IID_IPointOfInterest,   (IUnknown**)&m_pPoi);
   m_pBroker->GetInterface(IID_ILongRebarGeometry, (IUnknown**)&m_pRebarGeom);
   m_pBroker->GetInterface(IID_IProductLoads,      (IUnknown**)&m_pProductLoads);
   m_pBroker->GetInterface(IID_IProductForces,     (IUnknown**)&m_pProductForces);
   m_pBroker->GetInterface(IID_ICombinedForces,    (IUnknown**)&m_pCombinedForces);
   m_pBroker->GetInterface(IID_IExternalLoading,   (IUnknown**)&m_pExternalLoading);
   m_pBroker->GetInterface(IID_IVirtualWork,       (IUnknown**)&m_pVirtualWork);
   m_pBroker->GetInterface(IID_IEAFDisplayUnits,   (IUnknown**)&m_pDisplayUnits);

   CEAFAutoProgress ap(m_pProgress);
   m_pProgress->UpdateMessage(_T("Computing prestress losses"));

   m_StrandTypes.clear();

   if ( pLosses->SectionLosses.size() == 0 )
   {
      // if this is the first time analyzing this losses, do the friction and anchor set
      ComputeFrictionLosses(girderKey,pLosses);
      ComputeAnchorSetLosses(girderKey,pLosses);
   }

   ComputeSectionLosses(girderKey,endAnalysisIntervalIdx,pLosses);

   m_pProgress.Release();
   m_pBridgeDesc.Release();
   m_pBridge.Release();
   m_pStrandGeom.Release();
   m_pTendonGeom.Release();
   m_pIntervals.Release();
   m_pSectProp.Release();
   m_pGirder.Release();
   m_pMaterials.Release();
   m_pPSForce.Release();
   m_pPTForce.Release();
   m_pLossParams.Release();
   m_pPoi.Release();
   m_pRebarGeom.Release();
   m_pProductLoads.Release();
   m_pProductForces.Release();
   m_pCombinedForces.Release();
   m_pExternalLoading.Release();
   m_pVirtualWork.Release();
   m_pDisplayUnits.Release();
}

void CTimeStepLossEngineer::ComputeFrictionLosses(const CGirderKey& girderKey,LOSSES* pLosses)
{
   CEAFAutoProgress ap(m_pProgress);
   m_pProgress->UpdateMessage(_T("Computing friction losses"));

   Float64 Dset, wobble, friction;
   m_pLossParams->GetTendonPostTensionParameters(&Dset,&wobble,&friction);

   std::vector<pgsPointOfInterest> vPoi(m_pPoi->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)));

   WebIndexType nWebs = m_pGirder->GetWebCount(girderKey);

   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest& poi(*iter);

      LOSSDETAILS details;
#if defined _DEBUG
      details.POI = poi;
#endif

      details.LossMethod = pgsTypes::TIME_STEP;

      //////////////////////////////////////////////////////////////////////////
      // Friction Losses
      //////////////////////////////////////////////////////////////////////////
      const CSplicedGirderData* pGirder = m_pBridgeDesc->GetGirder(poi.GetSegmentKey());
      const CPTData* pPTData = pGirder->GetPostTensioning();

      DuctIndexType nDucts = m_pTendonGeom->GetDuctCount(poi.GetSegmentKey());
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         FRICTIONLOSSDETAILS frDetails;

         const CDuctData* pDuct = pPTData->GetDuct(ductIdx/nWebs);
         Float64 Pj;
         if ( pDuct->bPjCalc )
         {
            Pj = m_pPTForce->GetPjackMax(poi.GetSegmentKey(),pDuct->nStrands);
         }
         else
         {
            Pj = pDuct->Pj;
         }

         Float64 aps = pPTData->pStrand->GetNominalArea();
         StrandIndexType nStrands = pDuct->nStrands;
         Float64 Aps = aps*nStrands;
         Float64 fpj = (nStrands == 0 ? 0 : Pj/Aps);
         
         Float64 Xg = m_pPoi->ConvertPoiToGirderCoordinate(poi); // distance along girder
         Float64 Lg = m_pBridge->GetGirderLength(poi.GetSegmentKey());

         // determine from which end of the girder to measure the angular change of the tendon path
         pgsTypes::MemberEndType endType;
         if ( pDuct->JackingEnd == pgsTypes::jeLeft )
         {
            endType = pgsTypes::metStart;
         }
         else if ( pDuct->JackingEnd == pgsTypes::jeRight )
         {
            endType = pgsTypes::metEnd;
         }
         else
         {
            // jacked from both ends.... if Xg < Lg/2, measure from start, otherwise from the end
            endType = (Xg < Lg/2) ? pgsTypes::metStart : pgsTypes::metEnd;
         }

         Float64 X = Xg; // distance from stressing end
         if ( endType == pgsTypes::metEnd )
         {
            X = Lg - Xg;
         }

         Float64 alpha = m_pTendonGeom->GetAngularChange(poi,ductIdx,endType);

         frDetails.X = Xg;
         frDetails.alpha = alpha;
         frDetails.dfpF = fpj*(1 - exp(-(friction*alpha + X*wobble)));

         details.FrictionLossDetails.push_back(frDetails);
      }

      pLosses->SectionLosses.insert(std::make_pair(poi,details));
   }
}

void CTimeStepLossEngineer::ComputeAnchorSetLosses(const CGirderKey& girderKey,LOSSES* pLosses)
{
   CEAFAutoProgress ap(m_pProgress);
   m_pProgress->UpdateMessage(_T("Computing anchor set losses"));

   // First, compute the seating wedge, then compute anchor set loss at each POI
   Float64 girder_length = m_pBridge->GetGirderLength(girderKey);

   const CSplicedGirderData* pGirder = m_pBridgeDesc->GetGirder(girderKey);
   const CPTData* pPTData = pGirder->GetPostTensioning();

   Float64 Dset, wobble, friction;
   m_pLossParams->GetTendonPostTensionParameters(&Dset,&wobble,&friction);

   WebIndexType nWebs = m_pGirder->GetWebCount(girderKey);


#pragma Reminder("UPDATE: this assumes that the PT starts/ends at the face of girder")
   // the input allows the PT to start and end at any arbitrary point along the girder

   DuctIndexType nDucts = m_pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      const CDuctData* pDuct = pPTData->GetDuct(ductIdx/nWebs);

      // find location of minimum friction loss. this is the location of no movement in the strand
      SectionLossContainer::iterator frMinIter;
      if ( pDuct->JackingEnd == pgsTypes::jeLeft )
      {
         // jack at left end, no movement occurs at right end
         frMinIter = pLosses->SectionLosses.end();
         frMinIter--;
      }
      else if ( pDuct->JackingEnd == pgsTypes::jeRight )
      {
         // jack at right end, no movement occurs at left end
         frMinIter = pLosses->SectionLosses.begin();
      }
      else
      {
         // jack at both ends, no movement occurs somewhere in the middle... search for it
         SectionLossContainer::iterator iter(pLosses->SectionLosses.begin());
         SectionLossContainer::iterator end(pLosses->SectionLosses.end());
         Float64 dfpF_Prev = iter->second.FrictionLossDetails[ductIdx].dfpF;
         iter++;
         for ( ; iter != end; iter++ )
         {
            Float64 dfpF_Curr = iter->second.FrictionLossDetails[ductIdx].dfpF;
            if ( dfpF_Curr < dfpF_Prev )
            {
               // the current friction loss is less than the previous friction loss... 
               // that means we have gone past the POI where the min value occurs.
               frMinIter = --iter; // back up one
               break;
            }

            dfpF_Prev = dfpF_Curr;
         }
      }

      ANCHORSETDETAILS anchor_set;
      anchor_set.girderKey = girderKey;
      anchor_set.ductIdx   = ductIdx;


      if ( pDuct->JackingEnd == pgsTypes::jeLeft || pDuct->JackingEnd == pgsTypes::jeBoth )
      {
         Float64 dfpAT, dfpS, Xset;
         ComputeAnchorSetLosses(pPTData,pDuct,ductIdx,pgsTypes::metStart,pLosses,girder_length,frMinIter,&dfpAT,&dfpS,&Xset);
         anchor_set.Lset[pgsTypes::metStart]  = Min(girder_length,Xset);
         anchor_set.dfpAT[pgsTypes::metStart] = dfpAT;
         anchor_set.dfpS[pgsTypes::metStart]  = dfpS;
      }
      else
      {
         anchor_set.Lset[pgsTypes::metStart]  = 0;
         anchor_set.dfpAT[pgsTypes::metStart] = 0;
         anchor_set.dfpS[pgsTypes::metStart]  = 0;
      }

      if ( pDuct->JackingEnd == pgsTypes::jeRight || pDuct->JackingEnd == pgsTypes::jeBoth  )
      {
         Float64 dfpAT, dfpS, Xset;
         ComputeAnchorSetLosses(pPTData,pDuct,ductIdx,pgsTypes::metEnd,pLosses,girder_length,frMinIter,&dfpAT,&dfpS,&Xset);
         anchor_set.Lset[pgsTypes::metEnd]  = Min(girder_length,girder_length - Xset);
         anchor_set.dfpAT[pgsTypes::metEnd] = dfpAT;
         anchor_set.dfpS[pgsTypes::metEnd]  = dfpS;
      }
      else
      {
         anchor_set.Lset[pgsTypes::metEnd]  = 0;
         anchor_set.dfpAT[pgsTypes::metEnd] = 0;
         anchor_set.dfpS[pgsTypes::metEnd]  = 0;
      }

      pLosses->AnchorSet.push_back(anchor_set);
   }

   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      ANCHORSETDETAILS& anchorSetDetails( pLosses->AnchorSet[ductIdx] );

      // sum of friction and anchor set losses along the tendon (for computing average value)
      Float64 Sum_dfpF = 0;
      Float64 Sum_dfpA = 0;

      SectionLossContainer::iterator iter(pLosses->SectionLosses.begin());
      SectionLossContainer::iterator end(pLosses->SectionLosses.end());
      for ( ; iter != end; iter++ )
      {
         const pgsPointOfInterest& poi(iter->first);
         LOSSDETAILS& details(iter->second);

         FRICTIONLOSSDETAILS& frDetails(details.FrictionLossDetails[ductIdx]);

         Float64 dfpA[2] = {0,0};
         const CDuctData* pDuct = pPTData->GetDuct(ductIdx/nWebs);
         if ( frDetails.X <= anchorSetDetails.Lset[pgsTypes::metStart] )
         {
            // POI is in the left anchorage zone
            if ( IsZero(anchorSetDetails.Lset[pgsTypes::metStart]) )
            {
               dfpA[pgsTypes::metStart] = 0;
            }
            else
            {
               dfpA[pgsTypes::metStart] = ::LinInterp(frDetails.X,anchorSetDetails.dfpAT[pgsTypes::metStart],anchorSetDetails.dfpS[pgsTypes::metStart],anchorSetDetails.Lset[pgsTypes::metStart]);
            }
         }

         if ( girder_length-anchorSetDetails.Lset[pgsTypes::metEnd] <= frDetails.X )
         {
            // POI is in the right anchorage zone
            if ( IsZero(anchorSetDetails.Lset[pgsTypes::metEnd]) )
            {
               dfpA[pgsTypes::metEnd] = 0;
            }
            else
            {
               dfpA[pgsTypes::metEnd] = ::LinInterp(anchorSetDetails.Lset[pgsTypes::metEnd] - (girder_length-frDetails.X),anchorSetDetails.dfpS[pgsTypes::metEnd],anchorSetDetails.dfpAT[pgsTypes::metEnd],anchorSetDetails.Lset[pgsTypes::metEnd]);
            }
         }

         frDetails.dfpA = dfpA[pgsTypes::metStart] + dfpA[pgsTypes::metEnd];

         Sum_dfpF += frDetails.dfpF;
         Sum_dfpA += frDetails.dfpA;
      } // next poi
      
      IndexType nPoints = pLosses->SectionLosses.size();
      Float64 dfpF = Sum_dfpF/nPoints;
      Float64 dfpA = Sum_dfpA/nPoints;

      CTendonKey tendonKey(girderKey,ductIdx);
      m_AvgFrictionAndAnchorSetLoss.insert(std::make_pair(tendonKey,std::make_pair(dfpF,dfpA)));
   } // next duct
}

void CTimeStepLossEngineer::ComputeSectionLosses(const CGirderKey& girderKey,IntervalIndexType endAnalysisIntervalIdx,LOSSES* pLosses)
{
   CEAFAutoProgress ap(m_pProgress);

   bool bIgnoreTimeDependentEffects = m_pLossParams->IgnoreTimeDependentEffects();

   IntervalIndexType nIntervals = m_pIntervals->GetIntervalCount(girderKey);
   IntervalIndexType startAnalysisIntervalIdx = pLosses->SectionLosses.begin()->second.TimeStepDetails.size();
   for ( IntervalIndexType intervalIdx = startAnalysisIntervalIdx; intervalIdx <= endAnalysisIntervalIdx; intervalIdx++ )
   {
      CString strMsg;
      strMsg.Format(_T("Performing time-step analysis: Interval %d of %d"),LABEL_INTERVAL(intervalIdx),nIntervals);
      m_pProgress->UpdateMessage(strMsg);

      SectionLossContainer::iterator iter(pLosses->SectionLosses.begin());
      SectionLossContainer::iterator end(pLosses->SectionLosses.end());

      // Initialize Time Step Analysis
      // Basically computes the restraining forces at each POI
      for ( ; iter != end; iter++ )
      {
         LOSSDETAILS& details(iter->second);
         const pgsPointOfInterest& poi(iter->first);

         InitializeTimeStepAnalysis(intervalIdx,poi,details);
      }

      if ( !bIgnoreTimeDependentEffects && 0 < m_pIntervals->GetDuration(girderKey,intervalIdx) )
      {
         AnalyzeInitialStrains(intervalIdx,girderKey,pLosses);
      }

      // Finalize Time Step Analysis
      // Basically computes the resultant strains and rotations at each POI
      iter = pLosses->SectionLosses.begin();
      for ( ; iter != end; iter++ )
      {
         LOSSDETAILS& details(iter->second);
         const pgsPointOfInterest& poi(iter->first);

         FinalizeTimeStepAnalysis(intervalIdx,poi,details);
      }

      // Compute Deflections
      // NOTE: the deflections at a strong back closure joint begin at the start
      // of the interval when the closure becomes continuous with the rest of the girder.
      // Typically, strong backs are located at the end of cantilevered segments. The
      // tips of the cantilever have deflected prior to the closure joint being cast.
      // For this reason, the deflection of the closure joint does not start at zero,
      // but instead starts at the deflection of the cantilever. To account for
      // this, we assign the average deflection of the cantilever tips to the closure joint.
      // In order to do this, we need to compute the deflection at the ends of the segments
      // on either side ofthe closure. When a closure POI is encountered, the deflection
      // at the start of the next segment has not yet been computed. Make note that we
      // just skipped over a closure joint POI and that the closure adjustment needs to 
      // be made the next time through the loop

      // use pointers so we don't have to copy the actual POIs
      const pgsPointOfInterest* pLeftSegmentPoi; // segment POI to the left of a closure joint
      const pgsPointOfInterest* pClosurePoi; // poi at the closure joint

      bool bMakeClosureJointAdjustment = false; // is it time to make a closure joint adjustment?

      InitializeDeflectionCalculations();

      iter = pLosses->SectionLosses.begin();
      for ( ; iter != end; iter++ )
      {
         const pgsPointOfInterest& poi(iter->first);

         // compute the deflection at this poi
         ComputeDeflections(intervalIdx,poi,pLosses);

         if ( bMakeClosureJointAdjustment )
         {
            // it is time to make the closure joint adjustment
            MakeClosureJointAdjustment(intervalIdx,*pLeftSegmentPoi,*pClosurePoi,poi,pLosses);
         }

         // determine if we need to make a closure joint adjustment
         if ( poi.HasAttribute(POI_CLOSURE) )
         {
            // we are at a closure
            pClosurePoi = &poi;

            const CClosureKey& closureKey(poi.GetSegmentKey());
            IntervalIndexType compositeClosureIntervalIdx = m_pIntervals->GetCompositeClosureJointInterval(closureKey);

            const CClosureJointData* pClosure = m_pBridgeDesc->GetClosureJointData(closureKey);
            const CTemporarySupportData* pTS = pClosure->GetTemporarySupport();
            if ( pTS && pTS->GetSupportType() == pgsTypes::StrongBack && 
                 intervalIdx == compositeClosureIntervalIdx-1 )
            {
               // closure is at a strong back and the closure is just about to become composite
               bMakeClosureJointAdjustment = true;
            }
         }
         else
         {
            bMakeClosureJointAdjustment = false;
            pLeftSegmentPoi = &poi;
         }
      }
   }
}

void CTimeStepLossEngineer::InitializeTimeStepAnalysis(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LOSSDETAILS& details)
{
   // Initializes the time step analysis
   // Gets basic information about the cross section such as section properties.
   // Computes the force required to restrain the initial strains due to creep, shrinkage, and relaxation at each POI.

   bool bIgnoreTimeDependentEffects = m_pLossParams->IgnoreTimeDependentEffects();

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   CGirderKey girderKey(segmentKey);


   DuctIndexType nDucts = m_pTendonGeom->GetDuctCount(segmentKey);

   IntervalIndexType stressStrandsIntervalIdx = m_pIntervals->GetStressStrandInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx       = m_pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = m_pIntervals->GetCompositeDeckInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx      = m_pIntervals->GetLiveLoadInterval(segmentKey);

   bool bIsClosure = m_pPoi->IsInClosureJoint(poi);
   IntervalIndexType compositeClosureIntervalIdx = (bIsClosure ? m_pIntervals->GetCompositeClosureJointInterval(segmentKey) : INVALID_INDEX);


   // Material Properties
   Float64 EGirder = (bIsClosure ? m_pMaterials->GetClosureJointEc(segmentKey,intervalIdx) : m_pMaterials->GetSegmentEc(segmentKey,intervalIdx));
   Float64 EDeck = m_pMaterials->GetDeckEc(segmentKey,intervalIdx);
   Float64 EStrand[3] = { m_pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Straight)->GetE(),
                          m_pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Harped)->GetE(),
                          m_pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetE()};
   Float64 ETendon = m_pMaterials->GetTendonMaterial(girderKey)->GetE();
   
   Float64 gdrHeight = m_pGirder->GetHeight(poi);

   // Initialize time step details
   TIME_STEP_DETAILS tsDetails;

   // TIME PARAMETERS
   tsDetails.intervalIdx = intervalIdx;
   tsDetails.tStart      = m_pIntervals->GetStart(segmentKey,intervalIdx);
   tsDetails.tMiddle     = m_pIntervals->GetMiddle(segmentKey,intervalIdx);
   tsDetails.tEnd        = m_pIntervals->GetEnd(segmentKey,intervalIdx);

   // TRANSFORMED PROPERTIES OF COMPOSITE SECTION (all parts are transformed to equivalent girder concrete)
   tsDetails.Atr = m_pSectProp->GetAg(pgsTypes::sptTransformed,intervalIdx,poi);
   tsDetails.Itr = m_pSectProp->GetIx(pgsTypes::sptTransformed,intervalIdx,poi);
   tsDetails.Ytr = -m_pSectProp->GetY(pgsTypes::sptTransformed,intervalIdx,poi,pgsTypes::TopGirder); // Negative because this is measured down from Y=0 at the top of the girder

   // SEGMENT PARAMETERS

   // net section properties of segment
   tsDetails.Girder.An  = m_pSectProp->GetNetAg(intervalIdx,poi);
   tsDetails.Girder.In  = m_pSectProp->GetNetIg(intervalIdx,poi);
   tsDetails.Girder.Yn  = -m_pSectProp->GetNetYtg(intervalIdx,poi);
   tsDetails.Girder.H   = gdrHeight;

   // DECK PARAMETERS

   // net section properties of deck
   tsDetails.Deck.An  = m_pSectProp->GetNetAd(intervalIdx,poi);
   tsDetails.Deck.In  = m_pSectProp->GetNetId(intervalIdx,poi);
   tsDetails.Deck.Yn  = m_pSectProp->GetNetYbd(intervalIdx,poi); // distance from CG of net section to bottom of deck
   tsDetails.Deck.H   = tsDetails.Deck.Yn + m_pSectProp->GetNetYtd(intervalIdx,poi);

   // deck rebar
   if ( compositeDeckIntervalIdx <= intervalIdx  )
   {
      // deck is composite so the rebar is in play
      tsDetails.DeckRebar[pgsTypes::drmTop   ].As = m_pRebarGeom->GetAsTopMat(poi,ILongRebarGeometry::All);
      tsDetails.DeckRebar[pgsTypes::drmTop   ].Ys = m_pRebarGeom->GetTopMatLocation(poi,ILongRebarGeometry::All);

      tsDetails.DeckRebar[pgsTypes::drmBottom].As = m_pRebarGeom->GetAsBottomMat(poi,ILongRebarGeometry::All);
      tsDetails.DeckRebar[pgsTypes::drmBottom].Ys = m_pRebarGeom->GetBottomMatLocation(poi,ILongRebarGeometry::All);
   }

   // Girder/Closure Rebar
   CComPtr<IRebarSection> rebar_section;
   m_pRebarGeom->GetRebars(poi,&rebar_section);

   // POI is in a precast segment....
   CComPtr<IEnumRebarSectionItem> enum_items;
   rebar_section->get__EnumRebarSectionItem(&enum_items);
   CComPtr<IRebarSectionItem> item;
   while ( enum_items->Next(1,&item,NULL) != S_FALSE )
   {
      TIME_STEP_REBAR tsRebar;

      if ( (bIsClosure && (intervalIdx < compositeClosureIntervalIdx)) // POI is in a closure and the closure is not composite with the girder yet
            || // -OR-
           (intervalIdx < releaseIntervalIdx) // POI is in a segment and it is before the prestress is released
         )
      {
         // don't model the rebar
         tsRebar.As = 0;
         tsRebar.Ys = 0;
      }
      else
      {
         CComPtr<IRebar> rebar;
         item->get_Rebar(&rebar);

         Float64 Ab;
         rebar->get_NominalArea(&Ab);

         tsRebar.As = Ab;

         CComPtr<IPoint2d> p;
         item->get_Location(&p);

         Float64 y;
         p->get_Y(&y); // this is in Girder Section Coordinates.. that's exactly what we want

         tsRebar.Ys = y;
      }

      item.Release();

      tsDetails.GirderRebar.push_back(tsRebar);
   }

   // STRAND PARAMETERS
   if ( !bIsClosure && stressStrandsIntervalIdx <= intervalIdx )
   {
      // The strands are stressed
      const std::vector<pgsTypes::StrandType>& strandTypes = GetStrandTypes(segmentKey);
      std::vector<pgsTypes::StrandType>::const_iterator strandTypeIter(strandTypes.begin());
      std::vector<pgsTypes::StrandType>::const_iterator strandTypeIterEnd(strandTypes.end());
      for ( ; strandTypeIter != strandTypeIterEnd; strandTypeIter++ )
      {
         pgsTypes::StrandType strandType = *strandTypeIter;

         // time from strand stressing to end of this interval
         Float64 tStressing       = m_pIntervals->GetStart(segmentKey,stressStrandsIntervalIdx);
         Float64 tEndThisInterval = m_pIntervals->GetEnd(segmentKey,intervalIdx);

#if defined LUMP_STRANDS
         tsDetails.Strands[strandType].tEnd = Max(0.0,tEndThisInterval - tStressing);

         // Assumes transformed section properties are based on the strands being lumped in one location
         // Consider modeling each strand individually. This would be a more accurate analysis, however
         // it would take longer. Instead of looping over three strand types, we'll have to loop over
         // each strand for each strand type.

         // section properties
         tsDetails.Strands[strandType].As = m_pStrandGeom->GetStrandArea(segmentKey,intervalIdx,strandType);

         // location of strands from top of girder
         Float64 nEffectiveStrands = 0;
         tsDetails.Strands[strandType].Ys = (intervalIdx <= stressStrandsIntervalIdx ? 0 : m_pStrandGeom->GetStrandOffset(intervalIdx,poi,strandType,&nEffectiveStrands));

         if ( intervalIdx == stressStrandsIntervalIdx )
         {
            // Strands are stress in this interval.. get Pjack and fpj
            tsDetails.Strands[strandType].Pj  = (tsDetails.Strands[strandType].As == 0 ? 0 : m_pStrandGeom->GetPjack(segmentKey,strandType));
            tsDetails.Strands[strandType].fpj = (tsDetails.Strands[strandType].As == 0 ? 0 : tsDetails.Strands[strandType].Pj/tsDetails.Strands[strandType].As);

            // strands relax over the duration of the interval. compute the amount of relaxation
            if ( bIgnoreTimeDependentEffects )
            {
               tsDetails.Strands[strandType].fr = 0;
            }
            else
            {
               tsDetails.Strands[strandType].fr = m_pMaterials->GetStrandRelaxation(segmentKey,tsDetails.Strands[strandType].tEnd,0.0,tsDetails.Strands[strandType].fpj,strandType);
            }
         }
         else
         {
            TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);

            // strands were stressed in a previous interval
            if ( intervalIdx == releaseIntervalIdx )
            {
               // accounts for lack of development and location of debonding
               Float64 xfer_factor = m_pPSForce->GetXferLengthAdjustment(poi,strandType);

               // this the interval when the prestress force is release into the girders, apply the
               // prestress as an external force. The prestress force is the area of strand times
               // the effective stress in the strands at the end of the previous interval.
               // Negative sign because the force resisted by the girder section is equal and opposite
               // the force in the strand (stands put a compression force into the girder)
               Float64 P = -xfer_factor*tsDetails.Strands[strandType].As*prevTimeStepDetails.Strands[strandType].fpe;
               tsDetails.dP[pftPretension] += P;
               tsDetails.dM[pftPretension] += P*(tsDetails.Ytr - tsDetails.Strands[strandType].Ys);

               tsDetails.P[pftPretension] += prevTimeStepDetails.P[pftPretension] + tsDetails.dP[pftPretension];
               tsDetails.M[pftPretension] += prevTimeStepDetails.M[pftPretension] + tsDetails.dM[pftPretension];
            }

            // relaxation during this interval
            // by using the effective prestress at the end of the previous interval we get a very good approximation for 
            // the actual (reduced) relaxation. See "Time-Dependent Analysis of Composite Frames", Tadros, Ghali, Dilger, pg 876
            if ( bIgnoreTimeDependentEffects )
            {
               tsDetails.Strands[strandType].fr = 0;
            }
            else
            {
               tsDetails.Strands[strandType].fr = m_pMaterials->GetStrandRelaxation(segmentKey,tsDetails.Strands[strandType].tEnd,prevTimeStepDetails.Strands[strandType].tEnd,prevTimeStepDetails.Strands[strandType].fpe,strandType);
            }
         }

         // apparent strain due to relacation
         tsDetails.Strands[strandType].er = tsDetails.Strands[strandType].fr/EStrand[strandType]; // Tadros 1977, second term in Eqn 8

         // force required to resist apparent relaxation strain
         tsDetails.Strands[strandType].PrRelaxation = -tsDetails.Strands[strandType].fr*tsDetails.Strands[strandType].As; // Tadros 1977, Eqn 10
#else
         Float64 Pjack = m_pStrandGeom->GetPjack(segmentKey,strandType);
         StrandIndexType nStrands = m_pStrandGeom->GetStrandCount(segmentKey,strandType);
         if ( 0 < nStrands )
         {
            Pjack /= nStrands; // Pjack per strand
         }

         for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
         {
            TIME_STEP_STRAND strand;
            strand.tEnd = Max(0.0,tEndThisInterval - tStressing);

            if ( !bIsClosure )
            {
               // only fill up with data if NOT at closure joint...

               // section properties
               strand.As = m_pMaterials->GetStrandMaterial(segmentKey,strandType)->GetNominalArea();

               // location of strand from top of girder
               CComPtr<IPoint2d> point;
               m_pStrandGeom->GetStrandPosition(poi,strandIdx,strandType,&point);
               point->get_Y(&strand.Ys);

               if ( intervalIdx == stressStrandsIntervalIdx )
               {
                  // Strands are stress in this interval.. get Pjack and fpj
                  strand.Pj  = (IsZero(strand.As) ? 0 : Pjack);
                  strand.fpj = (IsZero(strand.As) ? 0 : strand.Pj/strand.As);

                  // strands relax over the duration of the interval. compute the amount of relaxation
                  if ( bIgnoreTimeDependentEffects )
                  {
                     strand.fr = 0;
                  }
                  else
                  {
                     strand.fr = m_pMaterials->GetStrandRelaxation(segmentKey,strand.tEnd,0.0,strand.fpj,strandType);
                  }
               }
               else
               {
                  // strands were stressed in a previous interval
                  if ( intervalIdx == releaseIntervalIdx )
                  {
                     // this is the interval when the prestress force is release into the girders, apply the
                     // prestress as an external force. The prestress force is the area of strand times
                     // the effective stress in the strand at the end of the previous interval.
                     // Negative sign because the force resisted by the girder section is equal and opposite
                     // the force in the strand (stand put a compression force into the girder)

                     // accounts for lack of development and location of debonding
                     Float64 xfer_factor = m_pPSForce->GetXferLengthAdjustment(poi,strandType);

                     Float64 P = -xfer_factor*strand.As*prevTimeStepDetails.Strands[strandType][strandIdx].fpe;
                     tsDetails.dP[pftPretension] += P;
                     tsDetails.dM[pftPretension] += P*(tsDetails.Ytr - strand.Ys);

                     tsDetails.P[pftPretension] += prevTimeStepDetails.P[pftPretension] + tsDetails.dP[pftPretension];
                     tsDetails.M[pftPretension] += prevTimeStepDetails.M[pftPretension] + tsDetails.dM[pftPretension];
                  }

                  // relaxation during this interval
                  // by using the effective prestress at the end of the previous interval we get a very good approximation for 
                  // the actual (reduced) relaxation. See "Time-Dependent Analysis of Composite Frames", Tadros, Ghali, Dilger, pg 876
                  if ( bIgnoreTimeDependentEffects )
                  {
                     strand.fr = 0;
                  }
                  else
                  {
                     strand.fr = m_pMaterials->GetStrandRelaxation(segmentKey,strand.tEnd,prevTimeStepDetails.Strands[strandType][strandIdx].tEnd,prevTimeStepDetails.Strands[strandType][strandIdx].fpe,strandType);
                  }
               }

               // apparent strain due to relacation
               strand.er = strand.fr/EStrand[strandType]; // Tadros 1977, second term in Eqn 8

               // force required to resist apparent relaxation strain
               strand.PrRelaxation = -strand.fr*strand.As; // Tadros 1977, Eqn 10
            } // if not closure

            tsDetails.Strands[strandType].push_back(strand);
         } // next strand
#endif // LUMP_STRANDS
      } // next strand type
   }

   // TENDON PARAMETERS
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      IntervalIndexType stressTendonIntervalIdx  = m_pIntervals->GetStressTendonInterval(girderKey,ductIdx);

      TIME_STEP_STRAND tsTendon;

      Float64 tStressing = m_pIntervals->GetStart(girderKey,stressTendonIntervalIdx);
      Float64 tEndThisInterval = m_pIntervals->GetEnd(girderKey,intervalIdx);
      tsTendon.tEnd = Max(0.0,tEndThisInterval - tStressing);
      tsTendon.As = m_pTendonGeom->GetTendonArea(girderKey,intervalIdx,ductIdx);
      tsTendon.Ys = (intervalIdx < stressTendonIntervalIdx ? 0 : m_pTendonGeom->GetDuctOffset(intervalIdx,poi,ductIdx));

      if ( intervalIdx == stressTendonIntervalIdx )
      {
         // The jacking force that is transfered to the girder is Pjack - Apt*(Friction Loss + Anchor Set Loss)
         //Float64 dfpF = details.FrictionLossDetails[ductIdx].dfpF; // friction loss at this POI
         //Float64 dfpA = details.FrictionLossDetails[ductIdx].dfpA; // anchor set loss at this POI
         Float64 dfpF, dfpA; // equiv. PT loads are based on average loss so use the same thing here

         // look up directly, don't call this method... calling this method here will
         // mess up the m_Losses data structure
         //GetAverageFrictionAndAnchorSetLoss(girderKey,ductIdx,&dfpF,&dfpA);

         CTendonKey tendonKey(girderKey,ductIdx);
         std::map<CTendonKey,std::pair<Float64,Float64>>::iterator found = m_AvgFrictionAndAnchorSetLoss.find(tendonKey);
         ATLASSERT( found != m_AvgFrictionAndAnchorSetLoss.end() ); // this should have already been done!

         dfpF = found->second.first;
         dfpA = found->second.second;

         tsTendon.Pj  = m_pTendonGeom->GetPjack(girderKey,ductIdx) - tsTendon.As*(dfpF + dfpA);
      }
      else
      {
         tsTendon.Pj = 0;
      }
      tsTendon.fpj = IsZero(tsTendon.As) ? 0 : tsTendon.Pj/tsTendon.As;
      tsTendon.P = tsTendon.Pj;

      // time from tendon stressing to end of this interval
      if ( intervalIdx < stressTendonIntervalIdx )
      {
         // tendons not installed yet, no relaxation
         tsTendon.fr = 0;
      }
      else if ( intervalIdx == stressTendonIntervalIdx )
      {
         TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);

         // this the interval when the pt force is release into the girders, apply the
         // prestress as an external force. The prestress force is the area of strand times
         // the effective stress at the end of the previous interval.
         // Negative sign because the force resisted by the girder section is equal and opposite
         // the force in the strand
         tsDetails.dP[pftPrimaryPostTensioning] -= tsTendon.Pj;
         tsDetails.dM[pftPrimaryPostTensioning] -= tsTendon.Pj*(tsDetails.Ytr - tsTendon.Ys);

         tsDetails.P[pftPrimaryPostTensioning] += prevTimeStepDetails.P[pftPrimaryPostTensioning] + tsDetails.dP[pftPrimaryPostTensioning];
         tsDetails.M[pftPrimaryPostTensioning] += prevTimeStepDetails.M[pftPrimaryPostTensioning] + tsDetails.dM[pftPrimaryPostTensioning];

         // NOTE: secondary effects are account for below in InitializeTimeStepAnalysis
         // when getting the contribution of all externally applied loads

         // relaxation during the stressing interval
         if ( bIgnoreTimeDependentEffects )
         {
            tsTendon.fr = 0;
         }
         else
         {
            tsTendon.fr = m_pMaterials->GetTendonRelaxation(girderKey,ductIdx,tsTendon.tEnd,0.0,tsTendon.fpj);
         }
      }
      else
      {
         TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);

         // by using the effective prestress at the end of the previous interval we get a very good approximation for 
         // the actual (reduced) relaxation. See "Time-Dependent Analysis of Composite Frames", Tadros, Ghali, Dilger, pg 876
         if ( bIgnoreTimeDependentEffects )
         {
            tsTendon.fr = 0;
         }
         else
         {
            tsTendon.fr = m_pMaterials->GetTendonRelaxation(girderKey,ductIdx,tsTendon.tEnd,prevTimeStepDetails.Tendons[ductIdx].tEnd,prevTimeStepDetails.Tendons[ductIdx].fpe);
         }
      }

      // apparent strain due to relaxation
      tsTendon.er = tsTendon.fr/ETendon;

      // force required to resist apparent relaxation strain
      tsTendon.PrRelaxation = -tsTendon.fr*tsTendon.As;

      tsDetails.Tendons.push_back(tsTendon);
   }

   // Compute unrestrained creep strain due to loads applied prior to this interval
   // Tadros 1977, Equations 3 and 4
   tsDetails.Girder.ec.reserve(intervalIdx);
   tsDetails.Girder.rc.reserve(intervalIdx);
   tsDetails.Deck.ec.reserve(intervalIdx);
   tsDetails.Deck.rc.reserve(intervalIdx);

   if ( 0 < intervalIdx )
   {
      for ( IntervalIndexType i = 0; i < intervalIdx-1; i++ )
      {
         TIME_STEP_CONCRETE::CREEP_STRAIN    girderCreepStrain,    deckCreepStrain;
         TIME_STEP_CONCRETE::CREEP_CURVATURE girderCreepCurvature, deckCreepCurvature;

         TIME_STEP_DETAILS& iTimeStepDetails(details.TimeStepDetails[i]); // time step details for interval i

         // Girder
         Float64 Cs, Ce;
         if ( bIgnoreTimeDependentEffects )
         {
            Cs = 0;
            Ce = 0;
         }
         else
         {
            Cs = (bIsClosure ? m_pMaterials->GetClosureJointCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::Start) : m_pMaterials->GetSegmentCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::Start));
            Ce = (bIsClosure ? m_pMaterials->GetClosureJointCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::End)   : m_pMaterials->GetSegmentCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::End));
         }

         Float64 EGirder = (bIsClosure ? m_pMaterials->GetClosureJointEc(segmentKey,i) : m_pMaterials->GetSegmentEc(segmentKey,i));
         Float64 EAGirder = EGirder*iTimeStepDetails.Girder.An;
         Float64 EIGirder = EGirder*iTimeStepDetails.Girder.In;

         Float64 dP_Girder = 0;
         Float64 dM_Girder = 0;
         Float64 dP_Deck = 0;
         Float64 dM_Deck = 0;
         std::vector<ProductForceType> vProductForces = GetApplicableProductLoads(i,poi);
         std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
         std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
         for ( ; pfIter != pfIterEnd; pfIter++ )
         {
            ProductForceType pfType = *pfIter;
            dP_Girder += iTimeStepDetails.Girder.dP[pfType];
            dM_Girder += iTimeStepDetails.Girder.dM[pfType];

            dP_Deck += iTimeStepDetails.Deck.dP[pfType];
            dM_Deck += iTimeStepDetails.Deck.dM[pfType];
         }

         Float64 e = IsZero(EAGirder) ? 0 : (Ce-Cs)*dP_Girder/EAGirder;
         Float64 r = IsZero(EIGirder) ? 0 : (Ce-Cs)*dM_Girder/EIGirder;

         tsDetails.Girder.e += e;
         tsDetails.Girder.r += r;

         girderCreepStrain.A = iTimeStepDetails.Girder.An;
         girderCreepStrain.E = EGirder;
         girderCreepStrain.P = dP_Girder;
         girderCreepStrain.Cs = Cs;
         girderCreepStrain.Ce = Ce;
         girderCreepStrain.e = e;

         girderCreepCurvature.I = iTimeStepDetails.Girder.In;
         girderCreepCurvature.E = EGirder;
         girderCreepCurvature.M = dM_Girder;
         girderCreepCurvature.Cs = Cs;
         girderCreepCurvature.Ce = Ce;
         girderCreepCurvature.r = r;

         tsDetails.Girder.ec.push_back(girderCreepStrain);
         tsDetails.Girder.rc.push_back(girderCreepCurvature);

         // Deck
         if ( bIgnoreTimeDependentEffects )
         {
            Cs = 0;
            Ce = 0;
         }
         else
         {
            Cs = m_pMaterials->GetDeckCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::Start);
            Ce = m_pMaterials->GetDeckCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::End);
         }

         Float64 Edeck = m_pMaterials->GetDeckEc(segmentKey,i);
         Float64 EAdeck = Edeck*iTimeStepDetails.Deck.An;
         Float64 EIdeck = Edeck*iTimeStepDetails.Deck.In;

         e = IsZero(EAdeck) ? 0 : (Ce-Cs)*dP_Deck/EAdeck;
         r = IsZero(EIdeck) ? 0 : (Ce-Cs)*dM_Deck/EIdeck;

         tsDetails.Deck.e += e;
         tsDetails.Deck.r += r;

         deckCreepStrain.A = iTimeStepDetails.Deck.An;
         deckCreepStrain.E = Edeck;
         deckCreepStrain.P = dP_Deck;
         deckCreepStrain.Cs = Cs;
         deckCreepStrain.Ce = Ce;
         deckCreepStrain.e = e;

         deckCreepCurvature.I = iTimeStepDetails.Deck.In;
         deckCreepCurvature.E = Edeck;
         deckCreepCurvature.M = dM_Deck;
         deckCreepCurvature.Cs = Cs;
         deckCreepCurvature.Ce = Ce;
         deckCreepCurvature.r = r;

         tsDetails.Deck.ec.push_back(deckCreepStrain);
         tsDetails.Deck.rc.push_back(deckCreepCurvature);
      }

      // Compute total unrestrained deformation due to creep and shrinkage during this interval
      // AKA Compute initial strain conditions due to creep and shrinkage
      Float64 esi;
      if ( bIgnoreTimeDependentEffects )
      {
         esi = 0;
      }
      else
      {
         esi = (bIsClosure ? m_pMaterials->GetClosureJointFreeShrinkageStrain(segmentKey,intervalIdx) : m_pMaterials->GetSegmentFreeShrinkageStrain(segmentKey,intervalIdx));
      }

      tsDetails.Girder.esi = esi;

      if ( bIgnoreTimeDependentEffects )
      {
         esi = 0;
      }
      else
      {
         esi = m_pMaterials->GetDeckFreeShrinkageStrain(segmentKey,intervalIdx);
      }

      tsDetails.Deck.esi = esi;

      // Compute forces to restrain creep and shrinkage deformations
      tsDetails.Girder.PrCreep     = -tsDetails.Girder.e   * tsDetails.Girder.An * EGirder; // Tadros 1977, Eqn 10
      tsDetails.Girder.MrCreep     = -tsDetails.Girder.r   * tsDetails.Girder.In * EGirder; // Tadros 1977, Eqn 11
      tsDetails.Girder.PrShrinkage = -tsDetails.Girder.esi * tsDetails.Girder.An * EGirder; // Tadros 1977, Eqn 10

      tsDetails.Deck.PrCreep     = -tsDetails.Deck.e   * tsDetails.Deck.An * EDeck; // Tadros 1977, Eqn 10
      tsDetails.Deck.MrCreep     = -tsDetails.Deck.r   * tsDetails.Deck.In * EDeck; // Tadros 1977, Eqn 11
      tsDetails.Deck.PrShrinkage = -tsDetails.Deck.esi * tsDetails.Deck.An * EDeck; // Tadros 1977, Eqn 10
   }

   // Total force/moment to restrain initial strains
   // Tadros 1977, Eqn 12 and 13
   if ( releaseIntervalIdx <= intervalIdx )
   {
      // Total Restraining Force
      tsDetails.Pr[TIMESTEP_CR] = tsDetails.Girder.PrCreep     + tsDetails.Deck.PrCreep;
      
      tsDetails.Pr[TIMESTEP_SH] = tsDetails.Girder.PrShrinkage + tsDetails.Deck.PrShrinkage;
      
#if defined LUMP_STRANDS
      tsDetails.Pr[TIMESTEP_RE] = tsDetails.Strands[pgsTypes::Straight ].PrRelaxation
                                + tsDetails.Strands[pgsTypes::Harped   ].PrRelaxation
                                + tsDetails.Strands[pgsTypes::Temporary].PrRelaxation;
#else
      tsDetails.Pr[TIMESTEP_RE] = 0;
#endif // LUMP_STRANDS

      tsDetails.Mr[TIMESTEP_CR] = tsDetails.Girder.MrCreep + tsDetails.Girder.PrCreep*(tsDetails.Ytr - tsDetails.Girder.Yn)
                                + tsDetails.Deck.MrCreep   + tsDetails.Deck.PrCreep  *(tsDetails.Ytr - tsDetails.Deck.Yn);
      
      tsDetails.Mr[TIMESTEP_SH] = tsDetails.Girder.PrShrinkage*(tsDetails.Ytr - tsDetails.Girder.Yn)
                                + tsDetails.Deck.PrShrinkage  *(tsDetails.Ytr - tsDetails.Deck.Yn);
      
#if defined LUMP_STRANDS
      tsDetails.Mr[TIMESTEP_RE] = tsDetails.Strands[pgsTypes::Straight ].PrRelaxation*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Straight ].Ys)
                                + tsDetails.Strands[pgsTypes::Harped   ].PrRelaxation*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Harped   ].Ys)
                                + tsDetails.Strands[pgsTypes::Temporary].PrRelaxation*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Temporary].Ys);
#else
      tsDetails.Mr[TIMESTEP_RE] = 0;
#endif // LUMP_STRANDS

#if !defined LUMP_STRANDS
      const std::vector<pgsTypes::StrandType>& strandTypes = GetStrandTypes(segmentKey);
      std::vector<pgsTypes::StrandType>::const_iterator strandTypeIter(strandTypes.begin());
      std::vector<pgsTypes::StrandType>::const_iterator strandTypeIterEnd(strandTypes.end());
      for ( ; strandTypeIter != strandTypeIterEnd; strandTypeIter++ )
      {
         pgsTypes::StrandType strandType = *strandTypeIter;
         StrandIndexType nStrands = m_pStrandGeom->GetStrandCount(segmentKey,strandType);
         for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
         {
            TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];
            tsDetails.Pr[TIMESTEP_RE] += strand.PrRelaxation;
            tsDetails.Mr[TIMESTEP_RE] += strand.PrRelaxation*(tsDetails.Ytr - strand.Ys);
         } // next strand
      } // next strand type
#endif // LUMP_STRANDS

      for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         TIME_STEP_STRAND& tendon = tsDetails.Tendons[ductIdx];

         tsDetails.Pr[TIMESTEP_RE] += tendon.PrRelaxation;
         tsDetails.Mr[TIMESTEP_RE] += tendon.PrRelaxation*(tsDetails.Ytr - tendon.Ys);
      }
   }

   details.TimeStepDetails.push_back(tsDetails);
}

void CTimeStepLossEngineer::AnalyzeInitialStrains(IntervalIndexType intervalIdx,const CGirderKey& girderKey,LOSSES* pLosses)
{
   // Compute the response to the inital strains (see Tadros 1977, section titled "Effects of Initial Strains")

   // Create a load group for the restraining forces in this interval
   std::_tstring strLoadGroupName[3] = {_T("Creep"),_T("Shrinkage"),_T("Relaxation")};

#pragma Reminder("UPDATE: CreateLoadGroup doesn't work") // using hard coded InitialStrains loads groups created in the analysis agent
//   // for the time being... get this fixed
//   //pExtLoading->CreateLoadGroup(strLoadGroupName[TIMESTEP_CR].c_str());
//   //pExtLoading->CreateLoadGroup(strLoadGroupName[TIMESTEP_SH].c_str());
//   //pExtLoading->CreateLoadGroup(strLoadGroupName[TIMESTEP_RE].c_str());
//   // Using pre-defined product load types pftCreep, pftShrinkage, pftRelaxation

   // At each POI, compute the equivalent restraining loads and apply to the load group
   SectionLossContainer::iterator iter1(pLosses->SectionLosses.begin());
   SectionLossContainer::iterator end(pLosses->SectionLosses.end());
   SectionLossContainer::iterator iter2(iter1);
   iter2++;

   for ( ; iter2 != end; iter1++, iter2++ )
   {
      const pgsPointOfInterest& poi1(iter1->first);
      const CSegmentKey& segmentKey1(poi1.GetSegmentKey());
      LOSSDETAILS& details1(iter1->second);
      bool bIsClosure1 = poi1.HasAttribute(POI_CLOSURE);
      bool bIsClosureEffective1 = (bIsClosure1 ? m_pIntervals->GetCompositeClosureJointInterval(segmentKey1) <= intervalIdx : false);
      
      const pgsPointOfInterest& poi2(iter2->first);
      const CSegmentKey& segmentKey2(poi2.GetSegmentKey());
      LOSSDETAILS& details2(iter2->second);
      bool bIsClosure2 = poi2.HasAttribute(POI_CLOSURE);
      bool bIsClosureEffective2 = (bIsClosure2 ? m_pIntervals->GetCompositeClosureJointInterval(segmentKey2) <= intervalIdx : false);

      TIME_STEP_DETAILS& tsDetails1(details1.TimeStepDetails[intervalIdx]);
      TIME_STEP_DETAILS& tsDetails2(details2.TimeStepDetails[intervalIdx]);

      // Compute initial strain at poi1
      Float64 Atr1 = tsDetails1.Atr;
      Float64 Itr1 = tsDetails1.Itr;
      Float64 E1 = (bIsClosure1 ? m_pMaterials->GetClosureJointEc(segmentKey1,intervalIdx) : m_pMaterials->GetSegmentEc(segmentKey1,intervalIdx));
      Float64 EA1 = Atr1*E1;
      Float64 EI1 = Itr1*E1;

      // Compute initial strain at poi2
      Float64 Atr2 = tsDetails2.Atr;
      Float64 Itr2 = tsDetails2.Itr;
      Float64 E2 = (bIsClosure2 ? m_pMaterials->GetClosureJointEc(segmentKey2,intervalIdx) : m_pMaterials->GetSegmentEc(segmentKey2,intervalIdx));
      Float64 EA2 = Atr2*E2;
      Float64 EI2 = Itr2*E2;

      for ( int i = 0; i < 3; i++ ) // i is one of the TIMESTEP_XXX constants
      {
         Float64 Pr1 = tsDetails1.Pr[i];
         Float64 Mr1 = tsDetails1.Mr[i];
         Float64 e1  = (EA1 == 0 ? 0 : Pr1/EA1);
         Float64 r1  = (EI1 == 0 ? 0 : Mr1/EI1);

         Float64 Pr2 = tsDetails2.Pr[i];
         Float64 Mr2 = tsDetails2.Mr[i];
         Float64 e2  = (EA2 == 0 ? 0 : Pr2/EA2);
         Float64 r2  = (EI2 == 0 ? 0 : Mr2/EI2);

         // The formulation in "Time-Dependent Analysis of Composite Frames (Tadros 1977)" assumes that the
         // initial strains vary lineraly along the bridge. The underlying LBAM and FEM models
         // only supports constant imposed deformations so we are going to use the average
         // values between POI. In general, the POI are tightly spaced so this is a good approximation
         Float64 e(0.5*(e1 + e2));
         Float64 r(0.5*(r1 + r2));

         // if POI is at a closure joint and the closure joint is not yet effective (cast and concrete has set)
         // there can't be any strains applied
         if ( (bIsClosure1 && !bIsClosureEffective1) || (bIsClosure2 && !bIsClosureEffective2) )
         {
            e = 0;
            r = 0;
         }

         // Negative signs because we want to impose deformations that are equal and opposite
         // to the deformations required to fully restrain the initial strains
         tsDetails1.e[i][pgsTypes::Ahead] = -e;
         tsDetails1.r[i][pgsTypes::Ahead] = -r;

         tsDetails2.e[i][pgsTypes::Back] = -e;
         tsDetails2.r[i][pgsTypes::Back] = -r;

         m_pExternalLoading->CreateInitialStrainLoad(intervalIdx,poi1,poi2,e,r,strLoadGroupName[i].c_str());
      }
   }

   // Get the interal forces caused by the initial strains
   SectionLossContainer::iterator iter(pLosses->SectionLosses.begin());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi(iter->first);
      bool bIsClosure = m_pPoi->IsInClosureJoint(poi);
      IntervalIndexType compositeClosureIntervalIdx = (bIsClosure ? m_pIntervals->GetCompositeClosureJointInterval(poi.GetSegmentKey()) : INVALID_INDEX);

      LOSSDETAILS& details(iter->second);
      TIME_STEP_DETAILS& tsDetails(details.TimeStepDetails[intervalIdx]);

      for ( int i = 0; i < 3; i++ ) // i is one of the TIMESTEP_XXX constants
      {
         Float64 P = m_pExternalLoading->GetAxial( intervalIdx,strLoadGroupName[i].c_str(),poi,m_Bat,rtIncremental);
         Float64 M = m_pExternalLoading->GetMoment(intervalIdx,strLoadGroupName[i].c_str(),poi,m_Bat,rtIncremental);

         // If the POI is at a closure joint, and it is before the closure is composite
         // with the adjacent girder segments, load doesn't get transfered to the concrete.
         // At strongbacks, a moment is computed, but this is the moment in the strongback hardware,
         // not the moment in the closure.
         if ( bIsClosure && intervalIdx < compositeClosureIntervalIdx )
         {
            P = 0;
            M = 0;
         }

         tsDetails.Pre[i] = P;
         tsDetails.Mre[i] = M;
      }
   }
}

void CTimeStepLossEngineer::FinalizeTimeStepAnalysis(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LOSSDETAILS& details)
{
   // Determine the effect of the initial strains on the various parts of the cross section
   // Determine the change in force in each part of the cross section
   // Determine the stresses on the girder

   TIME_STEP_DETAILS& tsDetails(details.TimeStepDetails[intervalIdx]);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   CGirderKey girderKey(segmentKey);

   bool bIsClosure = m_pPoi->IsInClosureJoint(poi);

   IntervalIndexType stressStrandsIntervalIdx = m_pIntervals->GetStressStrandInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx       = m_pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = m_pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx      = m_pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = m_pIntervals->GetCompositeDeckInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx      = m_pIntervals->GetLiveLoadInterval(segmentKey);

   if ( intervalIdx < releaseIntervalIdx )
   {
      // Prestress force not released onto segment yet...
      if ( !bIsClosure && stressStrandsIntervalIdx <= intervalIdx )
      {
         // strands are stressed, but not released
         const std::vector<pgsTypes::StrandType>& strandTypes = GetStrandTypes(segmentKey);
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIter(strandTypes.begin());
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIterEnd(strandTypes.end());
         for ( ; strandTypeIter != strandTypeIterEnd; strandTypeIter++ )
         {
            pgsTypes::StrandType strandType = *strandTypeIter;
#if defined LUMP_STRANDS
            // prestress loss is the intrinsic relaxation during this interval
            tsDetails.Strands[strandType].loss = tsDetails.Strands[strandType].fr;

            tsDetails.Strands[strandType].dP   = -tsDetails.Strands[strandType].loss * tsDetails.Strands[strandType].As;
            tsDetails.Strands[strandType].dFps = -tsDetails.Strands[strandType].loss;

            if ( intervalIdx == stressStrandsIntervalIdx )
            {
               // This is the interval when strands are stressed... 
               // Force in the strand at the end of the interval is equal to Pj - Change in Force during this interval
               // Effective prestress = fpj + dFps
               tsDetails.Strands[strandType].P   = tsDetails.Strands[strandType].Pj  + tsDetails.Strands[strandType].dP;
               tsDetails.Strands[strandType].fpe = tsDetails.Strands[strandType].fpj + tsDetails.Strands[strandType].dFps;
            }
            else
            {
               TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);

               // Force in strand is force at end of previous interval plus change in force during this interval
               tsDetails.Strands[strandType].P   = prevTimeStepDetails.Strands[strandType].P   + tsDetails.Strands[strandType].dP;

               // Effective prestress is effective prestress at end of previous interval plus change in stress during this interval
               tsDetails.Strands[strandType].fpe = prevTimeStepDetails.Strands[strandType].fpe + tsDetails.Strands[strandType].dFps;
            }
#else
            StrandIndexType nStrands = m_pStrandGeom->GetStrandCount(segmentKey,strandType);
            for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
            {
               TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];

               // prestress loss is the intrinsic relaxation during this interval
               strand.loss = strand.fr;

               strand.dP   = -strand.loss * strand.As;
               strand.dFps = -strand.loss;

               if ( intervalIdx == stressStrandsIntervalIdx )
               {
                  // This is the interval when strands are stressed... 
                  // Force in the strand at the end of the interval is equal to Pj - Change in Force during this interval
                  // Effective prestress = fpj + dFps
                  strand.P   = strand.Pj  + strand.dP;
                  strand.fpe = strand.fpj + strand.dFps;
               }
               else
               {
                  TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);

                  // Force in strand is force at end of previous interval plus change in force during this interval
                  strand.P   = prevTimeStepDetails.Strands[strandType][strandIdx].P   + strand.dP;

                  // Effective prestress is effective prestress at end of previous interval plus change in stress during this interval
                  strand.fpe = prevTimeStepDetails.Strands[strandType][strandIdx].fpe + strand.dFps;
               }
            } // next strand idx
#endif // LUMP_STRANDS
         } // next strand type
      }
   }
   else
   {
      TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);

      // get some material properties that we are going to need for the analysis
      Float64 EaGirder  = (bIsClosure ? 
                            m_pMaterials->GetClosureJointAgeAdjustedEc(segmentKey,intervalIdx) : 
                            m_pMaterials->GetSegmentAgeAdjustedEc(segmentKey,intervalIdx));
      Float64 EaDeck     = m_pMaterials->GetDeckAgeAdjustedEc(segmentKey,intervalIdx);
      Float64 EStrand[3] = { m_pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Straight)->GetE(),
                             m_pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Harped)->GetE(),
                             m_pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetE()};

      Float64 ETendon = m_pMaterials->GetTendonMaterial(girderKey)->GetE();

      Float64 EDeckRebar, EGirderRebar, Fy, Fu;
      m_pMaterials->GetDeckRebarProperties(&EDeckRebar,&Fy,&Fu);

      if ( bIsClosure )
      {
         m_pMaterials->GetClosureJointLongitudinalRebarProperties(segmentKey,&EGirderRebar,&Fy,&Fu);
      }
      else
      {
         m_pMaterials->GetSegmentLongitudinalRebarProperties(segmentKey,&EGirderRebar,&Fy,&Fu);
      }

      // EaGirder*tsDetails.Atr and EaGirder*tsDetails.Itr are used frequently... 
      // Use a variable here so we don't have to multiply it over and over
      Float64 EaGirder_Atr = EaGirder*tsDetails.Atr;
      Float64 EaGirder_Itr = EaGirder*tsDetails.Itr;

      Float64 EaGirder_An = EaGirder*tsDetails.Girder.An;
      Float64 EaGirder_In = EaGirder*tsDetails.Girder.In;

      Float64 EaDeck_An = EaDeck*tsDetails.Deck.An;
      Float64 EaDeck_In = EaDeck*tsDetails.Deck.In;

#if defined _BETA_VERSION
      // Verify section properties
      Float64 EA  = 0;
      Float64 EAy = 0;
      EA  = EaGirder_An + EaDeck_An;
      EAy = EaGirder_An*tsDetails.Girder.Yn + EaDeck_An*tsDetails.Deck.Yn;

      EA  += EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmTop].As + 
             EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmBottom].As;
      EAy += EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmTop].As*tsDetails.DeckRebar[pgsTypes::drmTop].Ys +
             EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmBottom].As*tsDetails.DeckRebar[pgsTypes::drmBottom].Ys;

      std::vector<TIME_STEP_REBAR>::iterator rIter(tsDetails.GirderRebar.begin());
      std::vector<TIME_STEP_REBAR>::iterator rIterEnd(tsDetails.GirderRebar.end());
      for ( ; rIter != rIterEnd; rIter++ )
      {
         TIME_STEP_REBAR& tsRebar(*rIter);
         EA  += EGirderRebar*tsRebar.As;
         EAy += EGirderRebar*tsRebar.As*tsRebar.Ys;
      }

      if ( !bIsClosure )
      {
         const std::vector<pgsTypes::StrandType>& strandTypes = GetStrandTypes(segmentKey);
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIter(strandTypes.begin());
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIterEnd(strandTypes.end());
         for ( ; strandTypeIter != strandTypeIterEnd; strandTypeIter++ )
         {
            pgsTypes::StrandType strandType = *strandTypeIter;
#if defined LUMP_STRANDS
            EA  += EStrand[strandType]*tsDetails.Strands[strandType].As;
            EAy += EStrand[strandType]*tsDetails.Strands[strandType].As*tsDetails.Strands[strandType].Ys;
#else
            StrandIndexType nStrands = m_pStrandGeom->GetStrandCount(segmentKey,strandType);
            for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
            {
               TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];
               EA  += EStrand[strandType]*strand.As;
               EAy += EStrand[strandType]*strand.As*strand.Ys;
            }
#endif // LUMP_STRANDS
         }
      }

      DuctIndexType nDucts = m_pTendonGeom->GetDuctCount(girderKey);
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         IntervalIndexType stressTendonIntervalIdx  = m_pIntervals->GetStressTendonInterval(girderKey,ductIdx);

         TIME_STEP_STRAND& tendon = tsDetails.Tendons[ductIdx];

         if ( stressTendonIntervalIdx < intervalIdx )
         {
            EA  += ETendon*tendon.As;
            EAy += ETendon*tendon.As*tendon.Ys;
         }
      }

      Float64 Ytr = IsZero(EA) ? 0 : EAy/EA;
      Float64 Atr = IsZero(EaGirder) ? 0 : EA/EaGirder;

      Float64 EI  = 0;

      EI  = EaGirder*(tsDetails.Girder.In + tsDetails.Girder.An*pow((Ytr - tsDetails.Girder.Yn),2));
      EI += EaDeck*(tsDetails.Deck.In + tsDetails.Deck.An*pow((Ytr - tsDetails.Deck.Yn),2));

      EI += EDeckRebar*(tsDetails.DeckRebar[pgsTypes::drmTop   ].As*pow((Ytr - tsDetails.DeckRebar[pgsTypes::drmTop   ].Ys),2));
      EI += EDeckRebar*(tsDetails.DeckRebar[pgsTypes::drmBottom].As*pow((Ytr - tsDetails.DeckRebar[pgsTypes::drmBottom].Ys),2));

      rIter = tsDetails.GirderRebar.begin();
      for ( ; rIter != rIterEnd; rIter++ )
      {
         TIME_STEP_REBAR& tsRebar(*rIter);
         EI += EGirderRebar*(tsRebar.As*pow((Ytr - tsRebar.Ys),2));
      }

      if ( !bIsClosure )
      {
         const std::vector<pgsTypes::StrandType>& strandTypes = GetStrandTypes(segmentKey);
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIter(strandTypes.begin());
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIterEnd(strandTypes.end());
         for ( ; strandTypeIter != strandTypeIterEnd; strandTypeIter++ )
         {
            pgsTypes::StrandType strandType = *strandTypeIter;
#if defined LUMP_STRANDS
            TIME_STEP_STRAND& strand = tsDetails.Strands[strandType];
            EI += EStrand[strandType]*strand.As*pow((Ytr - strand.Ys),2);
#else
            StrandIndexType nStrands = m_pStrandGeom->GetStrandCount(segmentKey,strandType);
            for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
            {
               TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];
               EI += EStrand[strandType]*strand.As*pow((Ytr - strand.Ys),2);
            }
#endif // LUMP_STRANDS
         }
      }

      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         IntervalIndexType stressTendonIntervalIdx  = m_pIntervals->GetStressTendonInterval(girderKey,ductIdx);

         TIME_STEP_STRAND& tendon = tsDetails.Tendons[ductIdx];

         if ( stressTendonIntervalIdx < intervalIdx )
         {
            EI += ETendon*(tendon.As*pow((Ytr - tendon.Ys),2));
         }
      }

      Float64 Itr = IsZero(EaGirder) ? 0 : EI/EaGirder;

      ATLASSERT(IsEqual(tsDetails.Ytr,Ytr));
      ATLASSERT(IsEqual(tsDetails.Atr,Atr));
      ATLASSERT(IsEqual(tsDetails.Itr,Itr));
#if !defined _DEBUG
      if ( !IsEqual(tsDetails.Ytr,Ytr) || !IsEqual(tsDetails.Atr,Atr) || !IsEqual(tsDetails.Itr,Itr) )
      {
         CString strMsg;
         strMsg.Format(_T("Ytr (%s,%s)\nAtr (%s,%s)\nItr (%s,%s)\n at POI %d"),
            ::FormatDimension(tsDetails.Ytr,m_pDisplayUnits->GetComponentDimUnit()),
            ::FormatDimension(Ytr,m_pDisplayUnits->GetComponentDimUnit()),
            ::FormatDimension(tsDetails.Atr,m_pDisplayUnits->GetAreaUnit()),
            ::FormatDimension(Atr,m_pDisplayUnits->GetAreaUnit()),
            ::FormatDimension(tsDetails.Itr,m_pDisplayUnits->GetMomentOfInertiaUnit()),
            ::FormatDimension(Itr,m_pDisplayUnits->GetMomentOfInertiaUnit()),
            poi.GetID());

         AfxMessageBox(strMsg);
      }
#endif // !_DEBUG
#endif // _BETA_VERSION

      IntervalIndexType compositeClosureIntervalIdx = (bIsClosure ? m_pIntervals->GetCompositeClosureJointInterval(segmentKey) : INVALID_INDEX);

      // Get Live Load Moment
      Float64 MllMin, MllMax;
      m_pCombinedForces->GetCombinedLiveLoadMoment(intervalIdx,pgsTypes::lltDesign,poi,m_Bat,&MllMin,&MllMax);

      //
      // Compute total change of force on the section due to externally applied loads during this interval
      //

      std::vector<ProductForceType> vProductForces = GetApplicableProductLoads(intervalIdx,poi,true/*externally applied loads only*/);
      std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
      std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
      for ( ; pfIter != pfIterEnd; pfIter++ )
      {
         // Change in forces due to externally applied loads only occur in a small subset of the analysis intervals
         // It is better to get this interface for each product force loading change than it is to get it for
         // every interval analyzed.
         ProductForceType pfType = *pfIter;

         // requesting the secondary effects causes recursion
         // the Analysis Agent uses the time-step loss analysis to get secondary effects
         // and since we are doing the time-step analysis right now, we get recursion
         // The analysis agent computes total post tensioning effects from a direct
         // stiffness analysis.
         if ( pfType == pftSecondaryEffects )
         {
            pfType = pftTotalPostTensioning;
         }

         // Get change in moment in this interval
         Float64 dM = m_pProductForces->GetMoment(intervalIdx,pfType,poi,m_Bat, rtIncremental);

         // dM is the total post tensioning moment... we only want the secondary effects.
         // subtract out the primary post tensioning and what remains is the secondary effects
         if ( pfType == pftTotalPostTensioning )
         {
            dM -= tsDetails.dM[pftPrimaryPostTensioning];

            // put the type back to the way it was so we don't overwrite the array bounds
            pfType = pftSecondaryEffects;
         }

         // If the POI is at a closure joint, and it is before the closure is composite
         // with the adjacent girder segments, the moment is zero. At strongbacks,
         // a moment is computed, but this is the moment in the strongback hardware,
         // not the moment in the closure.
         if ( bIsClosure && intervalIdx < compositeClosureIntervalIdx )
         {
            dM = 0;
            MllMin = 0;
            MllMax = 0;
         }

         ATLASSERT(pfType != pftTotalPostTensioning);

         tsDetails.dM[pfType] += dM;
         //tsDetails.dP[pfType] += 0; // no axial loads are modeled so sum is always zero

         tsDetails.P[pfType] += prevTimeStepDetails.P[pfType] + tsDetails.dP[pfType];
         tsDetails.M[pfType] += prevTimeStepDetails.M[pfType] + tsDetails.dM[pfType];
      }

      // Curvature due to live load moment (we'll need this to compute stresses due to live load later)
      // Compute it once and use it may times later
      Float64 rLLMin = IsZero(EaGirder_Itr) ? 0 : MllMin/EaGirder_Itr;
      Float64 rLLMax = IsZero(EaGirder_Itr) ? 0 : MllMax/EaGirder_Itr;

      // Add in the product forces for creep, shrinkage, and relaxation
      if ( !IsZero(EaGirder_Atr) )
      {
         tsDetails.dP[pftCreep]      += -(tsDetails.Pre[TIMESTEP_CR] + tsDetails.Pr[TIMESTEP_CR]);
         tsDetails.dP[pftShrinkage]  += -(tsDetails.Pre[TIMESTEP_SH] + tsDetails.Pr[TIMESTEP_SH]);
         tsDetails.dP[pftRelaxation] += -(tsDetails.Pre[TIMESTEP_RE] + tsDetails.Pr[TIMESTEP_RE]);

         tsDetails.P[pftCreep]      += prevTimeStepDetails.P[pftCreep]      + tsDetails.dP[pftCreep];
         tsDetails.P[pftShrinkage]  += prevTimeStepDetails.P[pftShrinkage]  + tsDetails.dP[pftShrinkage];
         tsDetails.P[pftRelaxation] += prevTimeStepDetails.P[pftRelaxation] + tsDetails.dP[pftRelaxation];
      }

      if ( !IsZero(EaGirder_Itr) )
      {
         tsDetails.dM[pftCreep]      += -(tsDetails.Mre[TIMESTEP_CR] + tsDetails.Mr[TIMESTEP_CR]);
         tsDetails.dM[pftShrinkage]  += -(tsDetails.Mre[TIMESTEP_SH] + tsDetails.Mr[TIMESTEP_SH]);
         tsDetails.dM[pftRelaxation] += -(tsDetails.Mre[TIMESTEP_RE] + tsDetails.Mr[TIMESTEP_RE]);

         tsDetails.M[pftCreep]      += prevTimeStepDetails.M[pftCreep]      + tsDetails.dM[pftCreep];
         tsDetails.M[pftShrinkage]  += prevTimeStepDetails.M[pftShrinkage]  + tsDetails.dM[pftShrinkage];
         tsDetails.M[pftRelaxation] += prevTimeStepDetails.M[pftRelaxation] + tsDetails.dM[pftRelaxation];
      }


      //
      // total change in axial and moment in this interval
      //

      Float64 dP = 0;
      Float64 dM = 0;
      int n = GetProductForceCount();
      for ( int i = 0; i < n; i++ ) 
      {
         // Must do all product forces for externally applyed loads here because we
         // are computing the forces due to each product type at the end of the current interval.
         // If we skip a product force type, the summation will be wrong (current = prev + change)
         ProductForceType pfType = (ProductForceType)(i);

         dP += tsDetails.dP[pfType];
         dM += tsDetails.dM[pfType];

         // Deformation of the composite section this interval
         tsDetails.der[pfType] = (IsZero(EaGirder_Atr) ? 0 : tsDetails.dP[pfType]/EaGirder_Atr);
         tsDetails.drr[pfType] = (IsZero(EaGirder_Itr) ? 0 : tsDetails.dM[pfType]/EaGirder_Itr);

         // Total deformation of the composite section
         tsDetails.er[pfType] = prevTimeStepDetails.er[pfType] + tsDetails.der[pfType];
         tsDetails.rr[pfType] = prevTimeStepDetails.rr[pfType] + tsDetails.drr[pfType];

         // Compute change in force in girder for this interval
         tsDetails.Girder.dP[pfType] += IsZero(EaGirder_Atr) ? 0 : tsDetails.dP[pfType]*EaGirder_An/EaGirder_Atr;
         tsDetails.Girder.dP[pfType] += IsZero(EaGirder_Itr) ? 0 : tsDetails.dM[pfType]*EaGirder_An*(tsDetails.Ytr-tsDetails.Girder.Yn)/EaGirder_Itr;
         tsDetails.Girder.dM[pfType] += IsZero(EaGirder_Itr) ? 0 : tsDetails.dM[pfType]*EaGirder_In/EaGirder_Itr;

         // From Tadros 1977.. "Effects of Initial Strains"
         // The stress resultants, in the k-th part, corresponding to these internal forces
         // are added to the initial stress resultants given by Eqs. 10 and 11 to give the final 
         // stress resultants.
         if ( pfType == pftCreep )
         {
            tsDetails.Girder.dP[pfType] += tsDetails.Girder.PrCreep;
            tsDetails.Girder.dM[pfType] += tsDetails.Girder.MrCreep;
         }
         else if ( pfType == pftShrinkage )
         {
            tsDetails.Girder.dP[pfType] += tsDetails.Girder.PrShrinkage;
         }

         tsDetails.Girder.P[pfType] += prevTimeStepDetails.Girder.P[pfType] + tsDetails.Girder.dP[pfType];
         tsDetails.Girder.M[pfType] += prevTimeStepDetails.Girder.M[pfType] + tsDetails.Girder.dM[pfType];

         // Compute girder stresses at end of interval
         // f = f end of previous interval + change in stress this interval
         if ( !IsZero(tsDetails.Girder.An) && !IsZero(tsDetails.Girder.In) )
         {
            tsDetails.Girder.f[pgsTypes::TopFace][pfType][rtIncremental] = tsDetails.Girder.dP[pfType]/tsDetails.Girder.An + tsDetails.Girder.dM[pfType]*tsDetails.Girder.Yn/tsDetails.Girder.In;
            tsDetails.Girder.f[pgsTypes::TopFace][pfType][rtCumulative] = prevTimeStepDetails.Girder.f[pgsTypes::TopFace][pfType][rtCumulative] + tsDetails.Girder.f[pgsTypes::TopFace][pfType][rtIncremental];
            
            tsDetails.Girder.f[pgsTypes::BottomFace][pfType][rtIncremental] = tsDetails.Girder.dP[pfType]/tsDetails.Girder.An + tsDetails.Girder.dM[pfType]*(tsDetails.Girder.H + tsDetails.Girder.Yn)/tsDetails.Girder.In;
            tsDetails.Girder.f[pgsTypes::BottomFace][pfType][rtCumulative] = prevTimeStepDetails.Girder.f[pgsTypes::BottomFace][pfType][rtCumulative] + tsDetails.Girder.f[pgsTypes::BottomFace][pfType][rtIncremental];
         }

         // Compute change in force in deck for this interval
         tsDetails.Deck.dP[pfType] += IsZero(EaGirder_Atr) ? 0 : tsDetails.dP[pfType]*EaDeck_An/EaGirder_Atr;
         tsDetails.Deck.dP[pfType] += IsZero(EaGirder_Itr) ? 0 : tsDetails.dM[pfType]*EaDeck_An*(tsDetails.Ytr-tsDetails.Deck.Yn)/EaGirder_Itr;
         tsDetails.Deck.dM[pfType] += IsZero(EaGirder_Itr) ? 0 : tsDetails.dM[pfType]*EaDeck_In/EaGirder_Itr;

         if ( pfType == pftCreep )
         {
            tsDetails.Deck.dP[pfType] += tsDetails.Deck.PrCreep;
            tsDetails.Deck.dM[pfType] += tsDetails.Deck.MrCreep;
         }
         else if ( pfType == pftShrinkage )
         {
            tsDetails.Deck.dP[pfType] += tsDetails.Deck.PrShrinkage;
         }

         tsDetails.Deck.P[pfType] += prevTimeStepDetails.Deck.P[pfType] + tsDetails.Deck.dP[pfType];
         tsDetails.Deck.M[pfType] += prevTimeStepDetails.Deck.M[pfType] + tsDetails.Deck.dM[pfType];

         // Compute deck stresses at end of interval
         // f = f end of previous interval + change in stress this interval
         if ( compositeDeckIntervalIdx <= intervalIdx && 
             !IsZero(tsDetails.Deck.An) && !IsZero(tsDetails.Deck.In) )
         {
            tsDetails.Deck.f[pgsTypes::TopFace][pfType][rtIncremental] = tsDetails.Deck.dP[pfType]/tsDetails.Deck.An + tsDetails.Deck.dM[pfType]*(tsDetails.Deck.Yn - tsDetails.Deck.H)/tsDetails.Deck.In;
            tsDetails.Deck.f[pgsTypes::TopFace][pfType][rtCumulative] = prevTimeStepDetails.Deck.f[pgsTypes::TopFace][pfType][rtCumulative] + tsDetails.Deck.f[pgsTypes::TopFace][pfType][rtIncremental];

            tsDetails.Deck.f[pgsTypes::BottomFace][pfType][rtIncremental] = tsDetails.Deck.dP[pfType]/tsDetails.Deck.An + tsDetails.Deck.dM[pfType]*tsDetails.Deck.Yn/tsDetails.Deck.In;
            tsDetails.Deck.f[pgsTypes::BottomFace][pfType][rtCumulative] = prevTimeStepDetails.Deck.f[pgsTypes::BottomFace][pfType][rtCumulative] + tsDetails.Deck.f[pgsTypes::BottomFace][pfType][rtIncremental];
         }
      } // next product force

      // Compute stresses in girder due to live load
      // Axial force in girder
      Float64 Pmin = EaGirder_An*rLLMin*(tsDetails.Ytr - tsDetails.Girder.Yn);
      Float64 Pmax = EaGirder_An*rLLMax*(tsDetails.Ytr - tsDetails.Girder.Yn);

      // Moment in girder
      Float64 Mmin = EaGirder_In*rLLMin;
      Float64 Mmax = EaGirder_In*rLLMax;

      if ( !IsZero(tsDetails.Girder.An) && !IsZero(tsDetails.Girder.In) )
      {
         tsDetails.Girder.fLLMin[pgsTypes::TopFace] = Pmin/tsDetails.Girder.An + Mmin*tsDetails.Girder.Yn/tsDetails.Girder.In;
         tsDetails.Girder.fLLMax[pgsTypes::TopFace] = Pmax/tsDetails.Girder.An + Mmax*tsDetails.Girder.Yn/tsDetails.Girder.In;
         if (tsDetails.Girder.fLLMax[pgsTypes::TopFace] < tsDetails.Girder.fLLMin[pgsTypes::TopFace])
         {
            std::swap(tsDetails.Girder.fLLMin[pgsTypes::TopFace],tsDetails.Girder.fLLMax[pgsTypes::TopFace]);
         }

         tsDetails.Girder.fLLMin[pgsTypes::BottomFace] = Pmin/tsDetails.Girder.An + Mmin*(tsDetails.Girder.H + tsDetails.Girder.Yn)/tsDetails.Girder.In;
         tsDetails.Girder.fLLMax[pgsTypes::BottomFace] = Pmax/tsDetails.Girder.An + Mmax*(tsDetails.Girder.H + tsDetails.Girder.Yn)/tsDetails.Girder.In;
         if (tsDetails.Girder.fLLMax[pgsTypes::BottomFace] < tsDetails.Girder.fLLMin[pgsTypes::BottomFace])
         {
            std::swap(tsDetails.Girder.fLLMin[pgsTypes::BottomFace],tsDetails.Girder.fLLMax[pgsTypes::BottomFace]);
         }
      }

      // Compute stresses in deck due to live load
      // Axial force in deck
      Pmin = EaDeck_An*rLLMin*(tsDetails.Ytr - tsDetails.Deck.Yn);
      Pmax = EaDeck_An*rLLMax*(tsDetails.Ytr - tsDetails.Deck.Yn);

      // Moment in deck
      Mmin = EaDeck_In*rLLMin;
      Mmax = EaDeck_In*rLLMax;

      if ( compositeDeckIntervalIdx <= intervalIdx && !IsZero(tsDetails.Deck.An) && !IsZero(tsDetails.Deck.In) )
      {
         tsDetails.Deck.fLLMin[pgsTypes::TopFace] = Pmin/tsDetails.Deck.An + Mmin*(tsDetails.Deck.Yn - tsDetails.Deck.H)/tsDetails.Deck.In;
         tsDetails.Deck.fLLMax[pgsTypes::TopFace] = Pmax/tsDetails.Deck.An + Mmax*(tsDetails.Deck.Yn - tsDetails.Deck.H)/tsDetails.Deck.In;
         if (tsDetails.Deck.fLLMax[pgsTypes::TopFace] < tsDetails.Deck.fLLMin[pgsTypes::TopFace])
         {
            std::swap(tsDetails.Deck.fLLMin[pgsTypes::TopFace],tsDetails.Deck.fLLMax[pgsTypes::TopFace]);
         }

         tsDetails.Deck.fLLMin[pgsTypes::BottomFace] = Pmin/tsDetails.Deck.An + Mmin*tsDetails.Deck.Yn/tsDetails.Deck.In;
         tsDetails.Deck.fLLMax[pgsTypes::BottomFace] = Pmax/tsDetails.Deck.An + Mmax*tsDetails.Deck.Yn/tsDetails.Deck.In;
         if (tsDetails.Deck.fLLMax[pgsTypes::BottomFace] < tsDetails.Deck.fLLMin[pgsTypes::BottomFace])
         {
            std::swap(tsDetails.Deck.fLLMin[pgsTypes::BottomFace],tsDetails.Deck.fLLMax[pgsTypes::BottomFace]);
         }
      }

      // Compute change in force in deck rebar
      tsDetails.DeckRebar[pgsTypes::drmTop   ].dP  = IsZero(EaGirder_Atr) ? 0 : dP*EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmTop   ].As/EaGirder_Atr;
      tsDetails.DeckRebar[pgsTypes::drmTop   ].dP += IsZero(EaGirder_Itr) ? 0 : dM*EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmTop   ].As*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmTop   ].Ys)/EaGirder_Itr;
      tsDetails.DeckRebar[pgsTypes::drmBottom].dP  = IsZero(EaGirder_Atr) ? 0 : dP*EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmBottom].As/EaGirder_Atr;
      tsDetails.DeckRebar[pgsTypes::drmBottom].dP += IsZero(EaGirder_Itr) ? 0 : dM*EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmBottom].As*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmBottom].Ys)/EaGirder_Itr;

      tsDetails.DeckRebar[pgsTypes::drmTop   ].P = prevTimeStepDetails.DeckRebar[pgsTypes::drmTop   ].P + tsDetails.DeckRebar[pgsTypes::drmTop   ].dP;
      tsDetails.DeckRebar[pgsTypes::drmBottom].P = prevTimeStepDetails.DeckRebar[pgsTypes::drmBottom].P + tsDetails.DeckRebar[pgsTypes::drmBottom].dP;

      // Compute change in force in girder Rebar
      IndexType rebarIdx = 0;
      std::vector<TIME_STEP_REBAR>::iterator rebarIter(tsDetails.GirderRebar.begin());
      std::vector<TIME_STEP_REBAR>::iterator rebarIterEnd(tsDetails.GirderRebar.end());
      for ( ; rebarIter != rebarIterEnd; rebarIter++, rebarIdx++ )
      {
         TIME_STEP_REBAR& tsRebar(*rebarIter);
         tsRebar.dP  = IsZero(EaGirder_Atr) ? 0 : dP*EGirderRebar*tsRebar.As/EaGirder_Atr;
         tsRebar.dP += IsZero(EaGirder_Itr) ? 0 : dM*EGirderRebar*tsRebar.As*(tsDetails.Ytr - tsRebar.Ys)/EaGirder_Itr;

         tsRebar.P = prevTimeStepDetails.GirderRebar[rebarIdx].P + tsRebar.dP;
      }

      // Compute change in force in strands
      if ( !m_pPoi->IsInClosureJoint(poi) )
      {
         const std::vector<pgsTypes::StrandType>& strandTypes = GetStrandTypes(segmentKey);
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIter(strandTypes.begin());
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIterEnd(strandTypes.end());
         for ( ; strandTypeIter != strandTypeIterEnd; strandTypeIter++ )
         {
            pgsTypes::StrandType strandType = *strandTypeIter;
#if defined LUMP_STRANDS
            // change in strand force
            tsDetails.Strands[strandType].dP += IsZero(EaGirder_Atr) ? 0 : dP*EStrand[strandType]*tsDetails.Strands[strandType].As/EaGirder_Atr;
            tsDetails.Strands[strandType].dP += IsZero(EaGirder_Itr) ? 0 : dM*EStrand[strandType]*tsDetails.Strands[strandType].As*(tsDetails.Ytr - tsDetails.Strands[strandType].Ys)/EaGirder_Itr;
            tsDetails.Strands[strandType].dP += tsDetails.Strands[strandType].PrRelaxation;
            tsDetails.Strands[strandType].P  = prevTimeStepDetails.Strands[strandType].P + tsDetails.Strands[strandType].dP;

            // Losses and effective prestress
            tsDetails.Strands[strandType].dFps = IsZero(tsDetails.Strands[strandType].As) ? 0 : tsDetails.Strands[strandType].dP/tsDetails.Strands[strandType].As;
            tsDetails.Strands[strandType].fpe  = prevTimeStepDetails.Strands[strandType].fpe  + tsDetails.Strands[strandType].dFps;
            tsDetails.Strands[strandType].loss = prevTimeStepDetails.Strands[strandType].loss - tsDetails.Strands[strandType].dFps;

            // check total loss = fpj - fpe
            ATLASSERT(IsEqual(tsDetails.Strands[strandType].loss,details.TimeStepDetails[stressStrandsIntervalIdx].Strands[strandType].fpj - tsDetails.Strands[strandType].fpe));

            // change in forces on composite section due to change in pretension forces this interval
            Float64 dPps = -tsDetails.Strands[strandType].dP;
            Float64 dMps = -tsDetails.Strands[strandType].dP*(tsDetails.Ytr - tsDetails.Strands[strandType].Ys);
            tsDetails.dP[pftPretension] += dPps; // += because we are accumulating over all the strand types
            tsDetails.dM[pftPretension] += dMps;

            // deformation due the prestress loss this interval
            Float64 eps = IsZero(EaGirder_Atr) ? 0 : dPps/EaGirder_Atr;
            Float64 rps = IsZero(EaGirder_Itr) ? 0 : dMps/EaGirder_Itr;
            tsDetails.der[pftPretension] += eps;
            tsDetails.drr[pftPretension] += rps;
            tsDetails.er[pftPretension] += tsDetails.der[pftPretension];
            tsDetails.rr[pftPretension] += tsDetails.drr[pftPretension];

            // change in force on girder concrete section due to loss in prestress this interal
            Float64 dPpsg = eps*EaGirder_An;
            Float64 dMpsg = rps*EaGirder_In;

            // change in stress in girder due to prestress loss this interval
            if ( !IsZero(tsDetails.Girder.An) && !IsZero(tsDetails.Girder.In) )
            {
               tsDetails.Girder.f[pgsTypes::TopFace   ][pftPretension][rtIncremental] += dPpsg/tsDetails.Girder.An + dMpsg*tsDetails.Girder.Yn/tsDetails.Girder.In;
               tsDetails.Girder.f[pgsTypes::BottomFace][pftPretension][rtIncremental] += dPpsg/tsDetails.Girder.An + dMpsg*(tsDetails.Girder.H + tsDetails.Girder.Yn)/tsDetails.Girder.In;
            }

            // change in force on deck concrete section due to loss in prestress this interval
            if ( compositeDeckIntervalIdx < intervalIdx )
            {
               Float64 dPpsd = eps*EaDeck_An;
               Float64 dMpsd = rps*EaDeck_In;

               // change in stress in deck due to prestress loss this interval
               if ( !IsZero(tsDetails.Deck.An) && !IsZero(tsDetails.Deck.In) )
               {
                  tsDetails.Deck.f[pgsTypes::TopFace   ][pftPretension][rtIncremental] += dPpsd/tsDetails.Deck.An + dMpsd*(tsDetails.Deck.Yn - tsDetails.Deck.H)/tsDetails.Deck.In;
                  tsDetails.Deck.f[pgsTypes::BottomFace][pftPretension][rtIncremental] += dPpsd/tsDetails.Deck.An + dMpsd*tsDetails.Deck.Yn/tsDetails.Deck.In;
               }
            }

            // Add elastic effect due to live load
            if ( intervalIdx < liveLoadIntervalIdx // before live load is applied
               || // OR
               IsZero(tsDetails.Strands[strandType].As) // there aren't any strands of this type
               )
            {
               tsDetails.Strands[strandType].dFllMin = 0;
               tsDetails.Strands[strandType].dFllMax = 0;
            }
            else
            {
               // live load is on the structure... add elastic effect
               tsDetails.Strands[strandType].dFllMin = rLLMin*EStrand[strandType]*(tsDetails.Ytr - tsDetails.Strands[strandType].Ys);
               tsDetails.Strands[strandType].dFllMax = rLLMax*EStrand[strandType]*(tsDetails.Ytr - tsDetails.Strands[strandType].Ys);
            }
            tsDetails.Strands[strandType].fpeLLMin  = tsDetails.Strands[strandType].fpe  + tsDetails.Strands[strandType].dFllMin;
            tsDetails.Strands[strandType].lossLLMin = tsDetails.Strands[strandType].loss - tsDetails.Strands[strandType].dFllMin;

            tsDetails.Strands[strandType].fpeLLMax  = tsDetails.Strands[strandType].fpe  + tsDetails.Strands[strandType].dFllMax;
            tsDetails.Strands[strandType].lossLLMax = tsDetails.Strands[strandType].loss - tsDetails.Strands[strandType].dFllMax;
#else
            StrandIndexType nStrands = m_pStrandGeom->GetStrandCount(segmentKey,strandType);
            for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
            {
               TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];

               // change in strand force
               strand.dP += IsZero(EaGirder_Atr) ? 0 : dP*EStrand[strandType]*strand.As/EaGirder_Atr;
               strand.dP += IsZero(EaGirder_Itr) ? 0 : dM*EStrand[strandType]*strand.As*(tsDetails.Ytr - strand.Ys)/EaGirder_Itr;
               strand.dP += strand.PrRelaxation;
               strand.P  = prevTimeStepDetails.Strands[strandType][strandIdx].P + strand.dP;

               // Losses and effective prestress
               strand.dFps = IsZero(strand.As) ? 0 : strand.dP/strand.As;
               strand.fpe  = prevTimeStepDetails.Strands[strandType][strandIdx].fpe  + strand.dFps;
               strand.loss = prevTimeStepDetails.Strands[strandType][strandIdx].loss - strand.dFps;

               // check total loss = fpj - fpe
               ATLASSERT(IsEqual(strand.loss,details.TimeStepDetails[stressStrandsIntervalIdx].Strands[strandType][strandIdx].fpj - strand.fpe));

               // change in forces on composite section due to change in pretension forces
               Float64 dPps = -tsDetails.Strands[strandType][strandIdx].dP;
               Float64 dMps = -tsDetails.Strands[strandType][strandIdx].dP*(tsDetails.Ytr - tsDetails.Strands[strandType][strandIdx].Ys);
               tsDetails.dP[pftPretension] += dPps;
               tsDetails.dM[pftPretension] += dMps;

               // deformation due to prestress loss
               Float64 eps = IsZero(EaGirder_Atr) ? 0 : dPps/EaGirder_Atr;
               Float64 rps = IsZero(EaGirder_Itr) ? 0 : dMps/EaGirder_Itr;
               tsDetails.der[pftPretension] += eps;
               tsDetials.drr[pftPretension] += rps;
               tsDetails.er[pftPretension] += tsDetails.der[pftPretension];
               tsDetails.rr[pftPretension] += tsDetails.drr[pftPretension];

               // change in force on girder concrete section due to loss in prestress
               Float64 dPpsg = eps*EaGirder_An;
               Float64 dMpsg = rps*EaGirder_In;

               // change in stress in girder due to prestress loss
               if ( !IsZero(tsDetails.Girder.An) && !IsZero(tsDetails.Girder.In) )
               {
                  tsDetails.Girder.f[pgsTypes::TopFace   ][pftPretension][rtIncremental] += dPpsg/tsDetails.Girder.An + dMpsg*tsDetails.Girder.Yn/tsDetails.Girder.In;
                  tsDetails.Girder.f[pgsTypes::BottomFace][pftPretension][rtIncremental] += dPpsg/tsDetails.Girder.An + dMpsg*(tsDetails.Girder.H + tsDetails.Girder.Yn)/tsDetails.Girder.In;
               }

               // change in force on deck concrete section due to loss in prestress
               if ( compositeDeckIntervalIdx < intervalIdx )
               {
                  Float64 dPpsd = eps*EaDeck_An;
                  Float64 dMpsd = rps*EaDeck_In;

                  // change in stress in deck due to prestress loss
                  if ( !IsZero(tsDetails.Deck.An) && !IsZero(tsDetails.Deck.In) )
                  {
                     tsDetails.Deck.f[pgsTypes::TopFace   ][pftPretension][rtIncremental] += dPpsd/tsDetails.Deck.An + dMpsd*(tsDetails.Deck.Yn - tsDetails.Deck.H)/tsDetails.Deck.In;
                     tsDetails.Deck.f[pgsTypes::BottomFace][pftPretension][rtIncremental] += dPpsd/tsDetails.Deck.An + dMpsd*tsDetails.Deck.Yn/tsDetails.Deck.In;
                  }
               }

               // Add elastic effect due to live load
               if ( intervalIdx < liveLoadIntervalIdx // before live load is applied
                  || // OR
                  IsZero(strand.As) // there aren't any strands of this type
                  )
               {
                  strand.dFllMin = 0;
                  strand.dFllMax = 0;
               }
               else
               {
                  // live load is on the structure... add elastic effect
                  strand.dFllMin = rLLMin*EStrand[strandType]*(tsDetails.Ytr - strand.Ys);
                  strand.dFllMax = rLLMax*EStrand[strandType]*(tsDetails.Ytr - strand.Ys);
               }
               strand.fpeLLMin  = strand.fpe  + strand.dFllMin;
               strand.lossLLMin = strand.loss - strand.dFllMin;

               strand.fpeLLMax  = strand.fpe  + strand.dFllMax;
               strand.lossLLMax = strand.loss - strand.dFllMax;
            } // next strand
#endif // LUMP_STRANDS
         } // next strand type

         // force due to pretensioning at the end of this interval
         tsDetails.P[pftPretension] = prevTimeStepDetails.P[pftPretension] + tsDetails.dP[pftPretension];
         tsDetails.M[pftPretension] = prevTimeStepDetails.M[pftPretension] + tsDetails.dM[pftPretension];

         // stress at end of this interval in girder/deck = stress at end of previous interval + change during this interval
         tsDetails.Girder.f[pgsTypes::TopFace   ][pftPretension][rtCumulative] = prevTimeStepDetails.Girder.f[pgsTypes::TopFace   ][pftPretension][rtCumulative] + tsDetails.Girder.f[pgsTypes::TopFace   ][pftPretension][rtIncremental];
         tsDetails.Girder.f[pgsTypes::BottomFace][pftPretension][rtCumulative] = prevTimeStepDetails.Girder.f[pgsTypes::BottomFace][pftPretension][rtCumulative] + tsDetails.Girder.f[pgsTypes::BottomFace][pftPretension][rtIncremental];

         tsDetails.Deck.f[pgsTypes::TopFace   ][pftPretension][rtCumulative] = prevTimeStepDetails.Deck.f[pgsTypes::TopFace   ][pftPretension][rtCumulative] + tsDetails.Deck.f[pgsTypes::TopFace   ][pftPretension][rtIncremental];
         tsDetails.Deck.f[pgsTypes::BottomFace][pftPretension][rtCumulative] = prevTimeStepDetails.Deck.f[pgsTypes::BottomFace][pftPretension][rtCumulative] + tsDetails.Deck.f[pgsTypes::BottomFace][pftPretension][rtIncremental];
      } // if not closure joint

      // Compute change in force in tendons
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         TIME_STEP_STRAND& tendon = tsDetails.Tendons[ductIdx];

         IntervalIndexType stressTendonIntervalIdx  = m_pIntervals->GetStressTendonInterval(girderKey,ductIdx);

         // Change in tendon force due to deformations during this interval
         // tendon forces are already set in the Initialize method at and before the stressing interval
         if ( stressTendonIntervalIdx < intervalIdx )
         {
            tendon.dP += IsZero(EaGirder_Atr) ? 0 : dP*ETendon*tendon.As/EaGirder_Atr;
            tendon.dP += IsZero(EaGirder_Itr) ? 0 : dM*ETendon*tendon.As*(tsDetails.Ytr - tendon.Ys)/EaGirder_Itr;
            tendon.dP += tendon.PrRelaxation;
            tendon.P  = prevTimeStepDetails.Tendons[ductIdx].P + tendon.dP;
         }

         // Losses and effective prestress
         tendon.dFps = (tendon.As == 0 ? 0 : tendon.dP/tendon.As);
         if ( intervalIdx < stressTendonIntervalIdx )
         {
            // interval is before tendons is stressed
            tendon.fpe  = 0;
            tendon.loss = 0;
         }
         else if ( intervalIdx == stressTendonIntervalIdx )
         {
            // tendons are stressed, but not released
            // effective stress at end of interval is fpj - relaxation in this interval
            tendon.fpe  = tendon.fpj + tendon.dFps;
            tendon.loss = tendon.dFps;
         }
         else
         {
            // effective stress at end of this interval = effective stress at end of previous interval + change in stress this interval
            tendon.fpe  = prevTimeStepDetails.Tendons[ductIdx].fpe  + tendon.dFps;
            tendon.loss = prevTimeStepDetails.Tendons[ductIdx].loss - tendon.dFps;

            ATLASSERT(IsEqual(tendon.loss,details.TimeStepDetails[stressTendonIntervalIdx].Tendons[ductIdx].fpj - tendon.fpe));
         }

         // change in forces on composite section due to change in primary post-tension forces
         Float64 dPppt = -tendon.dP;
         Float64 dMppt = -tendon.dP*(tsDetails.Ytr - tendon.Ys);

         // sum into running total for all tendons
         tsDetails.dP[pftPrimaryPostTensioning] += dPppt;
         tsDetails.dM[pftPrimaryPostTensioning] += dMppt;

         // deformations due to change in tendon force
         Float64 ept = IsZero(EaGirder_Atr) ? 0 : dPppt/EaGirder_Atr;
         Float64 rpt = IsZero(EaGirder_Itr) ? 0 : dMppt/EaGirder_Itr;
         tsDetails.der[pftPrimaryPostTensioning] += ept;
         tsDetails.drr[pftPrimaryPostTensioning] += rpt;
         tsDetails.er[pftPrimaryPostTensioning] += tsDetails.der[pftPrimaryPostTensioning];
         tsDetails.rr[pftPrimaryPostTensioning] += tsDetails.drr[pftPrimaryPostTensioning];

         // change in force on girder concrete section due to loss in prestress
         Float64 dPptg = ept*EaGirder_An;
         Float64 dMptg = rpt*EaGirder_In;

         // change in stress in girder due to prestress loss
         if ( !IsZero(tsDetails.Girder.An) && !IsZero(tsDetails.Girder.In) )
         {
            tsDetails.Girder.f[pgsTypes::TopFace   ][pftPrimaryPostTensioning][rtIncremental] += dPptg/tsDetails.Girder.An + dMptg*tsDetails.Girder.Yn/tsDetails.Girder.In;
            tsDetails.Girder.f[pgsTypes::BottomFace][pftPrimaryPostTensioning][rtIncremental] += dPptg/tsDetails.Girder.An + dMptg*(tsDetails.Girder.H + tsDetails.Girder.Yn)/tsDetails.Girder.In;
         }

         // change in force on deck concrete section due to loss in prestress
         if ( compositeDeckIntervalIdx < intervalIdx )
         {
            Float64 dPptd = ept*EaDeck_An;
            Float64 dMptd = rpt*EaDeck_In;

            // change in stress in deck due to prestress loss
            if ( !IsZero(tsDetails.Deck.An) && !IsZero(tsDetails.Deck.In) )
            {
               tsDetails.Deck.f[pgsTypes::TopFace   ][pftPrimaryPostTensioning][rtIncremental] += dPptd/tsDetails.Deck.An + dMptd*(tsDetails.Deck.Yn - tsDetails.Deck.H)/tsDetails.Deck.In;
               tsDetails.Deck.f[pgsTypes::BottomFace][pftPrimaryPostTensioning][rtIncremental] += dPptd/tsDetails.Deck.An + dMptd*tsDetails.Deck.Yn/tsDetails.Deck.In;
            }
         }

         // change in secondary moment due to force loss in tendon
         // This is a linear-elastic system. The change in PT force can be related to a change in
         // the equivalent PT loading. This change results in a change in the secondary effects.
         // The analysis model doesn't model the change in PT, however we can compute the secondary
         // effect as the secondary effect at the time the tendon was stresses times the ratio
         // of the loss to the tendon force.
         // Secondary effect this interval = (Secondary effect at jacking)(loss/Pj)
         Float64 Ps = 0; // axial due to secondary during the interval when the tendon is stressed
         Float64 Ms = 0; // moment due to secondary during the interval when the tendon is stressed
         Float64 Pj = 0; // Pjack causing the secondary effects
         if ( stressTendonIntervalIdx <= intervalIdx )
         {
            TIME_STEP_DETAILS& timeStepDetailsAtStressing(details.TimeStepDetails[stressTendonIntervalIdx]);
            Ps = timeStepDetailsAtStressing.P[pftSecondaryEffects];
            Ms = timeStepDetailsAtStressing.M[pftSecondaryEffects];
            Pj = timeStepDetailsAtStressing.Tendons[ductIdx].Pj;
         }
         Float64 dPps = IsZero(Pj) ? 0 : Ps*(tendon.dP)/Pj;
         Float64 dMps = IsZero(Pj) ? 0 : Ms*(tendon.dP)/Pj;

         // sum into running total for all tendons
         tsDetails.dP[pftSecondaryEffects] += dPps;
         tsDetails.dM[pftSecondaryEffects] += dMps;

         Float64 eps = IsZero(EaGirder_Atr) ? 0 : dPps/EaGirder_Atr;
         Float64 rps = IsZero(EaGirder_Itr) ? 0 : dMps/EaGirder_Itr;
         tsDetails.der[pftSecondaryEffects] += eps;
         tsDetails.drr[pftSecondaryEffects] += rps;
         tsDetails.er[pftSecondaryEffects] += tsDetails.der[pftSecondaryEffects];
         tsDetails.rr[pftSecondaryEffects] += tsDetails.drr[pftSecondaryEffects];

         Float64 dPpsg = eps*EaGirder_An;
         Float64 dMpsg = rps*EaGirder_In;
         
         tsDetails.Girder.dP[pftSecondaryEffects] += dPpsg;
         tsDetails.Girder.dM[pftSecondaryEffects] += dMpsg;
         tsDetails.Girder.P[pftSecondaryEffects]  += dPpsg;
         tsDetails.Girder.M[pftSecondaryEffects]  += dMpsg;

         // change in stress in girder due to change in secondary effects
         if ( !IsZero(tsDetails.Girder.An) && !IsZero(tsDetails.Girder.In) )
         {
            tsDetails.Girder.f[pgsTypes::TopFace   ][pftSecondaryEffects][rtIncremental] += dPpsg/tsDetails.Girder.An + dMpsg*tsDetails.Girder.Yn/tsDetails.Girder.In;
            tsDetails.Girder.f[pgsTypes::BottomFace][pftSecondaryEffects][rtIncremental] += dPpsg/tsDetails.Girder.An + dMpsg*(tsDetails.Girder.H + tsDetails.Girder.Yn)/tsDetails.Girder.In;
         }

         if ( compositeDeckIntervalIdx < intervalIdx )
         {
            Float64 dPpsd = eps*EaDeck_An;
            Float64 dMpsd = rps*EaDeck_In;
         
            tsDetails.Deck.dP[pftSecondaryEffects] += dPpsd;
            tsDetails.Deck.dM[pftSecondaryEffects] += dMpsd;
            tsDetails.Deck.P[pftSecondaryEffects]  += dPpsd;
            tsDetails.Deck.M[pftSecondaryEffects]  += dMpsd;

            if ( !IsZero(tsDetails.Deck.An) && !IsZero(tsDetails.Deck.In) )
            {
               tsDetails.Deck.f[pgsTypes::TopFace   ][pftSecondaryEffects][rtIncremental] += dPpsd/tsDetails.Deck.An + dMpsd*(tsDetails.Deck.Yn - tsDetails.Deck.H)/tsDetails.Deck.In;
               tsDetails.Deck.f[pgsTypes::BottomFace][pftSecondaryEffects][rtIncremental] += dPpsd/tsDetails.Deck.An + dMpsd*tsDetails.Deck.Yn/tsDetails.Deck.In;
            }
         }

         // Add elastic effect due to live load
         if ( intervalIdx < liveLoadIntervalIdx // before live load
              || // OR
              IsZero(tendon.As) // aren't any strands in the duct
            )
         {
            tendon.dFllMin = 0;
            tendon.dFllMax = 0;
         }
         else
         {
            // live load is on the structure... add elastic effect
            tendon.dFllMin = rLLMin*ETendon*(tsDetails.Ytr - tendon.Ys);
            tendon.dFllMax = rLLMax*ETendon*(tsDetails.Ytr - tendon.Ys);
         }
         tendon.fpeLLMin  = tendon.fpe  + tendon.dFllMin;
         tendon.lossLLMin = tendon.loss - tendon.dFllMin;

         tendon.fpeLLMax  = tendon.fpe  + tendon.dFllMax;
         tendon.lossLLMax = tendon.loss - tendon.dFllMax;
      } // next tendon

      // total due to primary pt at the end of this interval
      tsDetails.P[pftPrimaryPostTensioning] = prevTimeStepDetails.P[pftPrimaryPostTensioning] + tsDetails.dP[pftPrimaryPostTensioning];
      tsDetails.M[pftPrimaryPostTensioning] = prevTimeStepDetails.M[pftPrimaryPostTensioning] + tsDetails.dM[pftPrimaryPostTensioning];

      // total due to secondary pt at the end of this interval
      tsDetails.P[pftSecondaryEffects] = prevTimeStepDetails.P[pftSecondaryEffects] + tsDetails.dP[pftSecondaryEffects];
      tsDetails.M[pftSecondaryEffects] = prevTimeStepDetails.M[pftSecondaryEffects] + tsDetails.dM[pftSecondaryEffects];

      // change in stresses during this interval due to primary PT
      tsDetails.Girder.f[pgsTypes::TopFace   ][pftPrimaryPostTensioning][rtCumulative] = prevTimeStepDetails.Girder.f[pgsTypes::TopFace   ][pftPrimaryPostTensioning][rtCumulative] + tsDetails.Girder.f[pgsTypes::TopFace   ][pftPrimaryPostTensioning][rtIncremental];
      tsDetails.Girder.f[pgsTypes::BottomFace][pftPrimaryPostTensioning][rtCumulative] = prevTimeStepDetails.Girder.f[pgsTypes::BottomFace][pftPrimaryPostTensioning][rtCumulative] + tsDetails.Girder.f[pgsTypes::BottomFace][pftPrimaryPostTensioning][rtIncremental];

      tsDetails.Deck.f[pgsTypes::TopFace   ][pftPrimaryPostTensioning][rtCumulative] = prevTimeStepDetails.Deck.f[pgsTypes::TopFace   ][pftPrimaryPostTensioning][rtCumulative] + tsDetails.Deck.f[pgsTypes::TopFace   ][pftPrimaryPostTensioning][rtIncremental];
      tsDetails.Deck.f[pgsTypes::BottomFace][pftPrimaryPostTensioning][rtCumulative] = prevTimeStepDetails.Deck.f[pgsTypes::BottomFace][pftPrimaryPostTensioning][rtCumulative] + tsDetails.Deck.f[pgsTypes::BottomFace][pftPrimaryPostTensioning][rtIncremental];

      // change in stresses during this interval due to secondary effects
      tsDetails.Girder.f[pgsTypes::TopFace   ][pftSecondaryEffects][rtCumulative] = prevTimeStepDetails.Girder.f[pgsTypes::TopFace   ][pftSecondaryEffects][rtCumulative] + tsDetails.Girder.f[pgsTypes::TopFace   ][pftSecondaryEffects][rtIncremental];
      tsDetails.Girder.f[pgsTypes::BottomFace][pftSecondaryEffects][rtCumulative] = prevTimeStepDetails.Girder.f[pgsTypes::BottomFace][pftSecondaryEffects][rtCumulative] + tsDetails.Girder.f[pgsTypes::BottomFace][pftSecondaryEffects][rtIncremental];

      tsDetails.Deck.f[pgsTypes::TopFace   ][pftSecondaryEffects][rtCumulative] = prevTimeStepDetails.Deck.f[pgsTypes::TopFace   ][pftSecondaryEffects][rtCumulative] + tsDetails.Deck.f[pgsTypes::TopFace   ][pftSecondaryEffects][rtIncremental];
      tsDetails.Deck.f[pgsTypes::BottomFace][pftSecondaryEffects][rtCumulative] = prevTimeStepDetails.Deck.f[pgsTypes::BottomFace][pftSecondaryEffects][rtCumulative] + tsDetails.Deck.f[pgsTypes::BottomFace][pftSecondaryEffects][rtIncremental];
   } // end if interval < releaseIntervalidx

   //
   // Equilibrium Checks
   //

#pragma Reminder("UPDATE: make the equilibrium checks for debug builds only")
   // this will require updating the Time Step Parameters Chapter Builder

   // Check : Change in External Forces = Change in Internal Forces
   tsDetails.dPext = 0;
   tsDetails.dMext = 0;
   std::vector<ProductForceType> vProductForces = GetApplicableProductLoads(intervalIdx,poi,true/*externally applied loads only*/);
   vProductForces.push_back(pftPrimaryPostTensioning); // this is an external load in this case
   vProductForces.push_back(pftPretension); // this is an external load in this case
   std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
   std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
   for ( ; pfIter != pfIterEnd; pfIter++ )
   {
      ProductForceType pfType = *pfIter;
      tsDetails.dPext += tsDetails.dP[pfType];
      tsDetails.dMext += tsDetails.dM[pfType];
   }

   tsDetails.dPint = 0;
   tsDetails.dMint = 0;
   vProductForces = GetApplicableProductLoads(intervalIdx,poi);
   pfIter = vProductForces.begin();
   pfIterEnd = vProductForces.end();
   for ( ; pfIter != pfIterEnd; pfIter++ )
   {
      ProductForceType pfType = *pfIter;
      tsDetails.dPint += tsDetails.Girder.dP[pfType] + tsDetails.Deck.dP[pfType];
      tsDetails.dMint += tsDetails.Girder.dM[pfType] + tsDetails.Girder.dP[pfType]*(tsDetails.Ytr - tsDetails.Girder.Yn)
                       + tsDetails.Deck.dM[pfType]   + tsDetails.Deck.dP[pfType]  *(tsDetails.Ytr - tsDetails.Deck.Yn);
   }

   tsDetails.dPint += tsDetails.DeckRebar[pgsTypes::drmTop   ].dP 
                    + tsDetails.DeckRebar[pgsTypes::drmBottom].dP
                    + tsDetails.Pre[TIMESTEP_CR]
                    + tsDetails.Pre[TIMESTEP_SH]
                    + tsDetails.Pre[TIMESTEP_RE];

   tsDetails.dMint += tsDetails.DeckRebar[pgsTypes::drmTop   ].dP*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmTop   ].Ys)
                    + tsDetails.DeckRebar[pgsTypes::drmBottom].dP*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmBottom].Ys)
                    + tsDetails.Mre[TIMESTEP_CR]
                    + tsDetails.Mre[TIMESTEP_SH]
                    + tsDetails.Mre[TIMESTEP_RE];


   std::vector<TIME_STEP_REBAR>::iterator iter(tsDetails.GirderRebar.begin());
   std::vector<TIME_STEP_REBAR>::iterator end(tsDetails.GirderRebar.end());
   for ( ; iter != end; iter++ )
   {
      TIME_STEP_REBAR& tsRebar(*iter);
      tsDetails.dPint += tsRebar.dP;
      tsDetails.dMint += tsRebar.dP*(tsDetails.Ytr - tsRebar.Ys);
   }

#if defined _BETA_VERSION
   // change in internal force and moment must be the same as the change external force and moment
   // duing this interval
   if ( stressStrandsIntervalIdx < intervalIdx )
   {
      ATLASSERT(IsEqual(tsDetails.dPext,tsDetails.dPint,500.0));
      ATLASSERT(IsEqual(tsDetails.dMext,tsDetails.dMint,500.0));

#if !defined _DEBUG
      if ( !IsEqual(tsDetails.dPext,tsDetails.dPint,500.0) )
      {
         CString strMsg;
         strMsg.Format(_T("dPext != dPint (%s != %s) at POI %d"),
            ::FormatDimension(tsDetails.dPext,m_pDisplayUnits->GetGeneralForceUnit()),
            ::FormatDimension(tsDetails.dPint,m_pDisplayUnits->GetGeneralForceUnit()),
            poi.GetID());

         AfxMessageBox(strMsg);
      }

      if ( !IsEqual(tsDetails.dMext,tsDetails.dMint,500.0) )
      {
         CString strMsg;
         strMsg.Format(_T("dMext != dMint (%s != %s) at POI %d"),
            ::FormatDimension(tsDetails.dMext,m_pDisplayUnits->GetMomentUnit()),
            ::FormatDimension(tsDetails.dMint,m_pDisplayUnits->GetMomentUnit()),
            poi.GetID());

         AfxMessageBox(strMsg);
      }
#endif
   }
#endif // _BETA_VERSION

   // Check: Final external forces = final interal forces
   tsDetails.Pext = 0;
   tsDetails.Mext = 0;
   
   tsDetails.Pint = 0;
   tsDetails.Mint = 0;

   if ( intervalIdx < stressStrandsIntervalIdx )
   {
      // strands aren't stressed yet so there isn't any force
      // in the section
      tsDetails.Pext = 0;
      tsDetails.Mext = 0;
      tsDetails.Pint = 0;
      tsDetails.Mint = 0;
   }
   else if ( intervalIdx == stressStrandsIntervalIdx )
   {
      // strands are stressed this interval so the force at the end of the interval
      // is the change this interval
      tsDetails.Pext = tsDetails.dPext;
      tsDetails.Mext = tsDetails.dMext;
      tsDetails.Pint = tsDetails.dPint;
      tsDetails.Mint = tsDetails.dMint;
   }
   else if ( intervalIdx == releaseIntervalIdx )
   {
      // prestressed is transfered to the concrete section in this interval
      // internal forces need to be adjusted for stress in strand. generally internal
      // forces due to the strands are incremental changes but this interval, we need to
      // include the actual force transfered
      TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);
      tsDetails.Pext = prevTimeStepDetails.Pext + tsDetails.dPext;
      tsDetails.Mext = prevTimeStepDetails.Mext + tsDetails.dMext;
      tsDetails.Pint = prevTimeStepDetails.Pint + tsDetails.dPint - details.TimeStepDetails[stressStrandsIntervalIdx].dPint;
      tsDetails.Mint = prevTimeStepDetails.Mint + tsDetails.dMint - details.TimeStepDetails[stressStrandsIntervalIdx].dMint;
   }
   else
   {
      // force at end of this intervale = force at end of previous interval plus change in force during this interval
      TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);
      tsDetails.Pext = prevTimeStepDetails.Pext + tsDetails.dPext;
      tsDetails.Mext = prevTimeStepDetails.Mext + tsDetails.dMext;
      tsDetails.Pint = prevTimeStepDetails.Pint + tsDetails.dPint;
      tsDetails.Mint = prevTimeStepDetails.Mint + tsDetails.dMint;
   }

   // Total internal force and moment at the end of this interval must equal
   // the total external force and moment
#if defined _BETA_VERSION
   if ( stressStrandsIntervalIdx < intervalIdx )
   {
      ATLASSERT(IsEqual(tsDetails.Pext,tsDetails.Pint,500.0));
      ATLASSERT(IsEqual(tsDetails.Mext,tsDetails.Mint,500.0));

#if !defined _DEBUG
      if ( !IsEqual(tsDetails.Pext,tsDetails.Pint,500.0) )
      {
         CString strMsg;
         strMsg.Format(_T("Pext != Pint (%s != %s) at POI %d"),
            ::FormatDimension(tsDetails.Pext,m_pDisplayUnits->GetGeneralForceUnit()),
            ::FormatDimension(tsDetails.Pint,m_pDisplayUnits->GetGeneralForceUnit()),
            poi.GetID());

         AfxMessageBox(strMsg);
      }

      if ( !IsEqual(tsDetails.Mext,tsDetails.Mint,500.0) )
      {
         CString strMsg;
         strMsg.Format(_T("Mext != Mint (%s != %s) at POI %d"),
            ::FormatDimension(tsDetails.Mext,m_pDisplayUnits->GetMomentUnit()),
            ::FormatDimension(tsDetails.Mint,m_pDisplayUnits->GetMomentUnit()),
            poi.GetID());

         AfxMessageBox(strMsg);
      }
#endif // !_DEBUG
   }
#endif // _BETA_VERSION
}

void CTimeStepLossEngineer::ComputeDeflections(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LOSSES* pLosses)
{
   // Deflections are computing using the method of virtual work.
   // Reference:
   // Structural Analysis, 4th Edition, Jack C. McCormac
   // pg 365, Section 18.5, Application of Virtual Work to Beams and Frames
   // pg 371, Section 18.6, Rotations or Slopes by Virtual Work
   // Harper & Row, Publishers, New York, 1984
   // ISBN 0-06-044342-1

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   IntervalIndexType releaseIntervalIdx = m_pIntervals->GetPrestressReleaseInterval(segmentKey);

   if ( intervalIdx < releaseIntervalIdx )
   {
      return;
   }

   IntervalIndexType erectionIntervalIdx = m_pIntervals->GetErectSegmentInterval(segmentKey);

   LOSSDETAILS* pDetails = GetLossDetails(pLosses,poi);
   TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);


   CGirderKey girderKey(segmentKey);
   Float64 duration = m_pIntervals->GetDuration(girderKey,intervalIdx);

   std::vector<pgsPointOfInterest> vAllPoi(m_pPoi->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)));

   // Only do the virtual work deflection calculations for product forces that cause deflections
   // in this interval. (if the load doesn't cause deflection it would be a lot of work to
   // get an answer of zero)
   bool bIsErectionInterval = m_pIntervals->IsSegmentErectionInterval(segmentKey,intervalIdx);
   bool bIsStorageInterval  = m_pIntervals->GetStorageInterval(segmentKey) == intervalIdx ? true : false;
   std::vector<ProductForceType> vProductForces(GetApplicableProductLoads(intervalIdx,poi,true));
   
   // if strands have been stressed, they cause displacements in all non-zero duration intervals
   if ( m_pIntervals->GetPrestressReleaseInterval(segmentKey) == intervalIdx ||
       (m_pIntervals->GetFirstStressStrandInterval(girderKey) <= intervalIdx && !IsZero(duration)) )
   {
      vProductForces.push_back(pftPretension);
   }

   // if tendons have been stressed, they cause displacements in all non-zero duration intervals
   if ( m_pIntervals->IsTendonStressingInterval(girderKey,intervalIdx) ||
       (m_pIntervals->GetFirstTendonStressingInterval(girderKey) <= intervalIdx && !IsZero(duration)) )
   {
      vProductForces.push_back(pftPrimaryPostTensioning);
      vProductForces.push_back(pftSecondaryEffects);
   }

   // time-dependent effects cause displacements in non-zero duration intervals
   if ( !IsZero(duration) )
   {
      vProductForces.push_back(pftCreep);
      vProductForces.push_back(pftShrinkage);
      vProductForces.push_back(pftRelaxation);
   }

   // remove dupilcate product force types
   std::sort(vProductForces.begin(),vProductForces.end());
   vProductForces.erase(std::unique(vProductForces.begin(),vProductForces.end()),vProductForces.end());

   if ( 0 < vProductForces.size() )
   {
      // Since we are going to compute some deflections, get the moment diagram for a unit load at the poi 
      // that is being analyzed.
      if ( bIsErectionInterval || bIsStorageInterval )
      {
         // Support locations of the erected segment are in a different location then when the segment was in storage.
         // The incremental deflection calculations need to take this into account.
         //
         // This version of ComputeIncrementalDeflections accounts for the change in support locations however
         // it is slower than the other version because more calculations are required. This is why we only
         // want to use it for intervals when a segment is erected.
         std::vector<Float64> unitLoadMomentsPreviousInterval = m_pVirtualWork->GetUnitLoadMoment(vAllPoi,poi,m_Bat,intervalIdx-1);
         std::vector<Float64> unitLoadMomentsThisInterval     = m_pVirtualWork->GetUnitLoadMoment(vAllPoi,poi,m_Bat,intervalIdx);
         ATLASSERT(unitLoadMomentsPreviousInterval.size() == vAllPoi.size());
         ATLASSERT(unitLoadMomentsThisInterval.size() == vAllPoi.size());

         std::vector<sysSectionValue> unitCoupleMomentsPreviousInterval = m_pVirtualWork->GetUnitCoupleMoment(vAllPoi,poi,m_Bat,intervalIdx-1);
         std::vector<sysSectionValue> unitCoupleMomentsThisInterval     = m_pVirtualWork->GetUnitCoupleMoment(vAllPoi,poi,m_Bat,intervalIdx);
         ATLASSERT(unitCoupleMomentsPreviousInterval.size() == vAllPoi.size());
         ATLASSERT(unitCoupleMomentsThisInterval.size() == vAllPoi.size());

         std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
         std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
         for ( ; pfIter != pfIterEnd; pfIter++ )
         {
            ProductForceType pfType = *pfIter;
            ComputeIncrementalDeflections(intervalIdx,poi,pfType,vAllPoi,unitLoadMomentsPreviousInterval,unitLoadMomentsThisInterval,unitCoupleMomentsPreviousInterval,unitCoupleMomentsThisInterval,pLosses);
         }
      }
      else
      {
         // this version of ComputeIncrementalDeflections doesn't account for changing support locations
         std::vector<Float64> unitLoadMoments = m_pVirtualWork->GetUnitLoadMoment(vAllPoi,poi,m_Bat,intervalIdx);
         ATLASSERT(unitLoadMoments.size() == vAllPoi.size());

         std::vector<sysSectionValue> unitCoupleMoments = m_pVirtualWork->GetUnitCoupleMoment(vAllPoi,poi,m_Bat,intervalIdx);
         ATLASSERT(unitCoupleMoments.size() == vAllPoi.size());

         std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
         std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
         for ( ; pfIter != pfIterEnd; pfIter++ )
         {
            ProductForceType pfType = *pfIter;
            ComputeIncrementalDeflections(intervalIdx,poi,pfType,vAllPoi,unitLoadMoments,unitCoupleMoments,pLosses);
         }
      }
   }

   InitializeErectionAdjustments(intervalIdx,segmentKey,pLosses);

   int n = GetProductForceCount();
   for ( int i = 0; i < n; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;

      Float64 dAdj, rAdj;
      GetErectionAdjustment(intervalIdx,poi,pfType,&dAdj,&rAdj);
      tsDetails.dD[pfType] += dAdj;
      tsDetails.dR[pfType] += rAdj;

      // compute total deflection in this interval
      // total deflection this interval is deflection at end of last interval plus the change in this interval
      tsDetails.D[pfType] += pDetails->TimeStepDetails[intervalIdx-1].D[pfType] + tsDetails.dD[pfType];
      tsDetails.R[pfType] += pDetails->TimeStepDetails[intervalIdx-1].R[pfType] + tsDetails.dR[pfType];

      tsDetails.dY += tsDetails.dD[pfType];
      tsDetails.Y  += tsDetails.D[pfType];
   }
}

void CTimeStepLossEngineer::ComputeIncrementalDeflections(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vAllPoi,const std::vector<Float64>& unitLoadMomentsPreviousInterval,const std::vector<Float64>& unitLoadMomentsThisInterval,const std::vector<sysSectionValue>& unitCoupleMomentsPreviousInterval,const std::vector<sysSectionValue>& unitCoupleMomentsThisInterval,LOSSES* pLosses)
{
   // This version accounts for changes in support locations between the previous interval and this interval
   LOSSDETAILS* pDetails = GetLossDetails(pLosses,poi);
   TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

   ATLASSERT(unitLoadMomentsPreviousInterval.size()   == vAllPoi.size());
   ATLASSERT(unitLoadMomentsThisInterval.size()       == vAllPoi.size());
   ATLASSERT(unitCoupleMomentsPreviousInterval.size() == vAllPoi.size());
   ATLASSERT(unitCoupleMomentsThisInterval.size()     == vAllPoi.size());

   std::vector<Float64>::const_iterator prevIntervalMomentIter(unitLoadMomentsPreviousInterval.begin());
   std::vector<Float64>::const_iterator prevIntervalMomentIterEnd(unitLoadMomentsPreviousInterval.end());
   std::vector<Float64>::const_iterator thisIntervalMomentIter(unitLoadMomentsThisInterval.begin());
   std::vector<Float64>::const_iterator thisIntervalMomentIterEnd(unitLoadMomentsThisInterval.end());
   std::vector<sysSectionValue>::const_iterator prevIntervalCoupleIter(unitCoupleMomentsPreviousInterval.begin());
   std::vector<sysSectionValue>::const_iterator prevIntervalCoupleIterEnd(unitCoupleMomentsPreviousInterval.end());
   std::vector<sysSectionValue>::const_iterator thisIntervalCoupleIter(unitCoupleMomentsThisInterval.begin());
   std::vector<sysSectionValue>::const_iterator thisIntervalCoupleIterEnd(unitCoupleMomentsThisInterval.end());
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vAllPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vAllPoi.end());

   // "Subscripts"
   // i = previous interval
   // j = this interval
   // 1 = section 1
   // 2 = section 2
   // l = unit load
   // c = unit couple

   // Initialize values for first time through the loop
   Float64 delta = 0; // change in deflection during this interval
   Float64 rotation = 0; // change in rotation during this interval
   Float64 mli1 = *prevIntervalMomentIter; // unit load moment in the previous interval
   Float64 mlj1 = *thisIntervalMomentIter; // unit load moment in this interval
   sysSectionValue mci1 = *prevIntervalCoupleIter; // unit couple moment in the previous interval
   sysSectionValue mcj1 = *thisIntervalCoupleIter; // unit couple moment in this interval

   pgsPointOfInterest poi1(*poiIter); // can't be a reference because of the assignment in the loop below
   Float64 x1 = m_pPoi->ConvertPoiToGirderCoordinate(poi1);
   const LOSSDETAILS* pDetails1 = GetLossDetails(pLosses,poi1);
   ATLASSERT(pDetails1->POI == poi1);

   // advance the iterators so when we start the loop
   // the iterators are at the second location
   prevIntervalMomentIter++;
   thisIntervalMomentIter++;
   prevIntervalCoupleIter++;
   thisIntervalCoupleIter++;
   poiIter++;

   for ( ; poiIter != poiIterEnd; poiIter++, prevIntervalMomentIter++, thisIntervalMomentIter++, prevIntervalCoupleIter++, thisIntervalCoupleIter++ )
   {
      Float64 mli2 = *prevIntervalMomentIter;
      Float64 mlj2 = *thisIntervalMomentIter;

      sysSectionValue mci2 = *prevIntervalCoupleIter;
      sysSectionValue mcj2 = *thisIntervalCoupleIter;

      const pgsPointOfInterest& poi2(*poiIter);

      Float64 x2 = m_pPoi->ConvertPoiToGirderCoordinate(poi2);

      const LOSSDETAILS* pDetails2 = GetLossDetails(pLosses,poi2);

      const TIME_STEP_DETAILS& tsDetailsPrevInterval1 = pDetails1->TimeStepDetails[intervalIdx-1];
      const TIME_STEP_DETAILS& tsDetailsPrevInterval2 = pDetails2->TimeStepDetails[intervalIdx-1];

      const TIME_STEP_DETAILS& tsDetailsThisInterval1 = pDetails1->TimeStepDetails[intervalIdx];
      const TIME_STEP_DETAILS& tsDetailsThisInterval2 = pDetails2->TimeStepDetails[intervalIdx];

      ATLASSERT(pDetails2->POI == poi2);

      // curvature at section 1 and 2 at the end of the previous interval
      Float64 ci1 = tsDetailsPrevInterval1.rr[pfType];
      Float64 ci2 = tsDetailsPrevInterval2.rr[pfType];

      // change in curvature at sections 1 and 2 during this interval
      Float64 dc1 = tsDetailsThisInterval1.drr[pfType];
      Float64 dc2 = tsDetailsThisInterval2.drr[pfType];

      // should be dividing (x2-x1)*(...) by 2 however it is faster to just
      // divide by 2 at the end... here is an example of the algebra
      // K = A/2 + B/2 + C/2 = (A+B+C)/2
      // we only have to divide by 2 once instead of every time through the loop
      delta    += (x2-x1)*((dc1*mlj1         + dc2*mlj2)        + (ci1*(mlj1-mli1)                 + ci2*(mlj2-mli2)));
      rotation += (x2-x1)*((dc1*mcj1.Right() + dc2*mcj2.Left()) + (ci1*(mcj1.Right()-mci1.Right()) + ci2*(mcj2.Left()-mci2.Left())));

      // get ready for next time through the loop
      mli1      = mli2;
      mlj1      = mlj2;
      mci1      = mci2;
      mcj1      = mcj2;
      poi1      = poi2;
      x1        = x2;
      pDetails1 = pDetails2;
   }

   delta /= 2.0;
   rotation /= 2.0;

   tsDetails.dD[pfType] = delta;
   tsDetails.dR[pfType] = rotation;
}

void CTimeStepLossEngineer::ComputeIncrementalDeflections(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vAllPoi,const std::vector<Float64>& unitLoadMoments,const std::vector<sysSectionValue>& unitCoupleMoments,LOSSES* pLosses)
{
   // this version assumes the support locations don't change from the previous interval to this interval
   LOSSDETAILS* pDetails = GetLossDetails(pLosses,poi);
   TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

   ATLASSERT(unitLoadMoments.size() == vAllPoi.size());
   ATLASSERT(unitCoupleMoments.size() == vAllPoi.size());

   std::vector<Float64>::const_iterator momentIter(unitLoadMoments.begin());
   std::vector<Float64>::const_iterator momentIterEnd(unitLoadMoments.end());
   std::vector<sysSectionValue>::const_iterator coupleIter(unitCoupleMoments.begin());
   std::vector<sysSectionValue>::const_iterator coupleIterEnd(unitCoupleMoments.end());
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vAllPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vAllPoi.end());

   // "Subscripts"
   // 1 = section 1
   // 2 = section 2

   // Initialize values for first time through the loop
   Float64 delta = 0; // change in deflection during this interval
   Float64 rotation = 0; // change in rotation during this interval
   Float64 ml1 = *momentIter; // unit load moment
   sysSectionValue mc1 = *coupleIter; // unit couple moment

   pgsPointOfInterest poi1(*poiIter); // can't be a reference because of the assignment in the loop below
   Float64 x1 = m_pPoi->ConvertPoiToGirderCoordinate(poi1);
   const LOSSDETAILS* pDetails1 = GetLossDetails(pLosses,poi1);
   ATLASSERT(pDetails1->POI == poi1);

   // advance the iterators so when we start the loop
   // the iterators are at the second location
   momentIter++;
   coupleIter++;
   poiIter++;

   for ( ; poiIter != poiIterEnd; poiIter++, momentIter++, coupleIter++ )
   {
      Float64 ml2 = *momentIter;
      sysSectionValue mc2 = *coupleIter;

      const pgsPointOfInterest& poi2(*poiIter);

      Float64 x2 = m_pPoi->ConvertPoiToGirderCoordinate(poi2);

      const LOSSDETAILS* pDetails2 = GetLossDetails(pLosses,poi2);

      const TIME_STEP_DETAILS& tsDetails1 = pDetails1->TimeStepDetails[intervalIdx];
      const TIME_STEP_DETAILS& tsDetails2 = pDetails2->TimeStepDetails[intervalIdx];

      ATLASSERT(pDetails2->POI == poi2);

      // change in curvature at sections 1 and 2 during this interval
      Float64 dc1 = tsDetails1.drr[pfType];
      Float64 dc2 = tsDetails2.drr[pfType];

      // should be dividing (x2-x1)*(...) by 2 however it is faster to just
      // divide by 2 at the end... here is an example of the algebra
      // K = A/2 + B/2 + C/2 = (A+B+C)/2
      // we only have to divde by 2 once instead of every time through the loop
      delta    += (x2-x1)*(dc1*ml1 + dc2*ml2);
      rotation += (x2-x1)*(dc1*mc1.Right() + dc2*mc2.Left());

      // get ready for next time through the loop
      ml1       = ml2;
      mc1       = mc2;
      poi1      = poi2;
      x1        = x2;
      pDetails1 = pDetails2;
   }

   delta /= 2.0;
   rotation /= 2.0;

   tsDetails.dD[pfType] = delta;
   tsDetails.dR[pfType] = rotation;
}

void CTimeStepLossEngineer::BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
#pragma Reminder("UPDATE: this is a dummy implementation")
   // Need to do a better job of report time-step loss details for the details report

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),     false );

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount(girderKey);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE(IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      std::_tostringstream os;
      if ( 1 < nSegments )
      {
         os << _T("Segment ") << LABEL_SEGMENT(segIdx) << _T(" - Losses in Pretensioned Strands");
      }
      else
      {
         os << _T("Losses in Pretensioned Strands");
      }

      rptRcTable* pPSLossTable = pgsReportStyleHolder::CreateDefaultTable(2+nIntervals,os.str().c_str());
      *pPara << pPSLossTable << rptNewLine;

      RowIndexType row = pPSLossTable->GetNumberOfHeaderRows();


      ColumnIndexType col = 0;
      (*pPSLossTable)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
      {
         (*pPSLossTable)(0,col) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("ps")) << rptNewLine << _T("Interval ") << LABEL_INTERVAL(intervalIdx), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*pPSLossTable)(0,col) << rptNewLine << _T("Straight") << rptNewLine << _T("Harped");
         col++;
      }
      (*pPSLossTable)(0,col) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*pPSLossTable)(0,col) << rptNewLine << _T("Straight") << rptNewLine << _T("Harped");

      Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,segIdx));

      // Get the losses for this segment
      GET_IFACE(IPointOfInterest,pPoi);
      std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(CSegmentKey(girderKey,segIdx),POI_ERECTED_SEGMENT | POI_TENTH_POINTS));
      std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
      std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
      for ( ; iter != end; iter++, row++ )
      {
         col = 0;
         const pgsPointOfInterest& poi(*iter);
         const LOSSDETAILS* pLossDetails = GetLosses(poi,INVALID_INDEX);

         (*pPSLossTable)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

         for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
         {
#if defined LUMP_STRANDS
            (*pPSLossTable)(row,col) << stress.SetValue( pLossDetails->TimeStepDetails[intervalIdx].Strands[pgsTypes::Straight].dFps );
            (*pPSLossTable)(row,col) << rptNewLine;
            (*pPSLossTable)(row,col) << stress.SetValue( pLossDetails->TimeStepDetails[intervalIdx].Strands[pgsTypes::Harped].dFps );
#else
#pragma Reminder("UPDATE: reporting of loss in strands")
            //(*pPSLossTable)(row,col) << stress.SetValue( pLossDetails->TimeStepDetails[intervalIdx].Strands[pgsTypes::Straight].dFps );
            //(*pPSLossTable)(row,col) << rptNewLine;
            //(*pPSLossTable)(row,col) << stress.SetValue( pLossDetails->TimeStepDetails[intervalIdx].Strands[pgsTypes::Harped].dFps );
#endif // LUMP_STRANDS
            col++;
         }

#if defined LUMP_STRANDS
         (*pPSLossTable)(row,col) << stress.SetValue( pLossDetails->TimeStepDetails.back().Strands[pgsTypes::Straight].loss );
         (*pPSLossTable)(row,col) << rptNewLine;
         (*pPSLossTable)(row,col) << stress.SetValue( pLossDetails->TimeStepDetails.back().Strands[pgsTypes::Harped].loss );
#else
#pragma Reminder("UPDATE: reporting of loss in strands")
         (*pPSLossTable)(row,col) << rptNewLine; //stress.SetValue( pLossDetails->TimeStepDetails.back().Strands[pgsTypes::Straight].loss );
         (*pPSLossTable)(row,col) << rptNewLine;
         (*pPSLossTable)(row,col) << rptNewLine; //stress.SetValue( pLossDetails->TimeStepDetails.back().Strands[pgsTypes::Harped].loss );
#endif // LLUMP_STRANDS
      } // next POI
   } // next segment

  
   GET_IFACE(ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);

   if ( 0 < nDucts )
   {
      rptRcTable** pPTLossTable = new rptRcTable*[nDucts];
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         CString strTitle;

         IntervalIndexType ptIntervalIdx = pIntervals->GetStressTendonInterval(girderKey,ductIdx);

         strTitle.Format(_T("Duct %d - Losses in Post-Tension Tendon"),LABEL_DUCT(ductIdx));
         pPTLossTable[ductIdx] = pgsReportStyleHolder::CreateDefaultTable(4+(nIntervals-ptIntervalIdx),strTitle);
         *pPara << pPTLossTable[ductIdx] << rptNewLine;


         ColumnIndexType ductCol = 0;
         (*pPTLossTable[ductIdx])(0,ductCol++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*pPTLossTable[ductIdx])(0,ductCol++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pF")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*pPTLossTable[ductIdx])(0,ductCol++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pA")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         for ( IntervalIndexType intervalIdx = ptIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
         {
            (*pPTLossTable[ductIdx])(0,ductCol++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pt")) << rptNewLine << _T("Interval ") << LABEL_INTERVAL(intervalIdx), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }
         (*pPTLossTable[ductIdx])(0,ductCol++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      } // next duct

      Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));
      RowIndexType row = pPTLossTable[0]->GetNumberOfHeaderRows();

      GET_IFACE(IPointOfInterest,pPoi);
      std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)));
      std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
      std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
      for ( ; iter != end; iter++, row++ )
      {
         const pgsPointOfInterest& poi(*iter);
         const LOSSDETAILS* pLossDetails = GetLosses(poi,INVALID_INDEX);

         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            ColumnIndexType ductCol = 0;

            IntervalIndexType ptIntervalIdx = pIntervals->GetStressTendonInterval(girderKey,ductIdx);

            (*pPTLossTable[ductIdx])(row,ductCol++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

            (*pPTLossTable[ductIdx])(row,ductCol++) << stress.SetValue( pLossDetails->FrictionLossDetails[ductIdx].dfpF );
            (*pPTLossTable[ductIdx])(row,ductCol++) << stress.SetValue( pLossDetails->FrictionLossDetails[ductIdx].dfpA );

            for ( IntervalIndexType intervalIdx = ptIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
            {
               (*pPTLossTable[ductIdx])(row,ductCol++) << stress.SetValue( pLossDetails->TimeStepDetails[intervalIdx].Tendons[ductIdx].dFps );
            }

            (*pPTLossTable[ductIdx])(row,ductCol++) << stress.SetValue( pLossDetails->TimeStepDetails.back().Tendons[ductIdx].loss );
         }
      }

      // deletes the array of points, not the pointers themselves
      delete[] pPTLossTable;
   }
}

void CTimeStepLossEngineer::ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
#pragma Reminder("UPDATE: implement - time step loss reporting")
   ATLASSERT(false);
   //m_Engineer.ReportFinalLosses(m_BeamType,segmentKey,pChapter,pDisplayUnits);
}


void CTimeStepLossEngineer::ComputeAnchorSetLosses(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,LOSSES* pLosses,Float64 Lg,SectionLossContainer::iterator& frMinIter,Float64* pdfpAT,Float64* pdfpS,Float64* pXset)
{
   if ( pDuctData->nStrands == 0 )
   {
      // If there aren't any strands in the tendon, there aren't any losses.
      *pXset  = 0;
      *pdfpAT = 0;
      *pdfpS  = 0;
      return;
   }

   // solve with method of false position (aka regula falsi method)
   // http://en.wikipedia.org/wiki/False_position_method
   // http://mathworld.wolfram.com/MethodofFalsePosition.html

   Float64 Dset, wobble, friction;
   m_pLossParams->GetTendonPostTensionParameters(&Dset,&wobble,&friction);

   const CGirderKey& girderKey(pPTData->GetGirder()->GetGirderKey());

   IntervalIndexType stressTendonIntervalIdx = m_pIntervals->GetStressTendonInterval(girderKey,ductIdx);

   Float64 Apt = m_pTendonGeom->GetTendonArea(girderKey,stressTendonIntervalIdx,ductIdx);
   Float64 Pj  = (Apt == 0 ? 0 : m_pTendonGeom->GetPjack(girderKey,ductIdx));
   Float64 fpj = (Apt == 0 ? 0 : Pj/Apt);

   Float64 XsetMin, XsetMax; // position of end of anchor set zone measured from left end of girder
   Float64 DsetMin, DsetMax;
   Float64 dfpATMin, dfpATMax;
   Float64 dfpSMin, dfpSMax;
   BoundAnchorSet(pPTData,pDuctData,ductIdx,endType,Dset,pLosses,fpj,Lg,frMinIter,&XsetMin,&DsetMin,&dfpATMin,&dfpSMin,&XsetMax,&DsetMax,&dfpATMax,&dfpSMax);

   // If the solution is nailed, get the heck outta here
   if ( IsEqual(DsetMin,Dset) )
   {
      *pXset  = XsetMin;
      *pdfpAT = dfpATMin;
      *pdfpS  = dfpSMin;
      return;
   }

   if ( IsEqual(DsetMax,Dset) )
   {
      *pXset  = XsetMax;
      *pdfpAT = dfpATMax;
      *pdfpS  = dfpSMax;
      return;
   }

   // ok, we've got some work to do...
   int side = 0;
   long maxIter = 100;
   Float64 Xset;
   Float64 dfpAT;
   Float64 dfpS;
   long iter = 0;
   for ( iter = 0; iter < maxIter; iter++ )
   {
      Xset = ((DsetMin-Dset)*XsetMax - (DsetMax-Dset)*XsetMin)/((DsetMin-Dset) - (DsetMax-Dset));
      Float64 Dset1 = EvaluateAnchorSet(pPTData,pDuctData,ductIdx,endType,pLosses,fpj,Lg,frMinIter,Xset,&dfpAT,&dfpS);

      if ( IsEqual(Dset,Dset1) )
      {
         break;
      }

      if ( 0 < (Dset1-Dset)*(DsetMax-Dset) )
      {
         XsetMax = Xset;
         DsetMax = Dset1;
         if ( side == -1 )
         {
            DsetMin = (DsetMin+Dset)/2;
         }

         side = -1;
      }
      else if ( 0 < (Dset1-Dset)*(DsetMin-Dset) )
      {
         XsetMin = Xset;
         DsetMin = Dset1;
         if ( side == 1 )
         {
            DsetMax = (DsetMax+Dset)/2;
         }

         side = 1;
      }
      else
      {
         break;
      }
   }

   if ( maxIter <= iter )
   {
      ATLASSERT(false); // did not converge
   }

   *pXset  = Xset;
   *pdfpAT = dfpAT;
   *pdfpS  = dfpS;
}

void CTimeStepLossEngineer::BoundAnchorSet(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,Float64 Dset,LOSSES* pLosses,Float64 fpj,Float64 Lg,SectionLossContainer::iterator& frMinIter,Float64* pXsetMin,Float64* pDsetMin,Float64* pdfpATMin,Float64* pdfpSMin,Float64* pXsetMax,Float64* pDsetMax,Float64* pdfpATMax,Float64* pdfpSMax)
{
   const CSplicedGirderData* pGirder = pPTData->GetGirder();
   const CGirderGroupData*   pGroup  = pGirder->GetGirderGroup();
   const CPierData2*         pPier   = pGroup->GetPier(endType);

   SpanIndexType   spanIdx = (endType == pgsTypes::metStart ? pPier->GetNextSpan()->GetIndex() : pPier->GetPrevSpan()->GetIndex());
   GirderIndexType gdrIdx  = pGirder->GetIndex();

   Float64 span_length = m_pBridge->GetSpanLength(spanIdx,gdrIdx);

   Float64 XsetMin, XsetMax;

   Float64 deltaX = ::ConvertToSysUnits(1.0,unitMeasure::Meter);
   Float64 K = 1.5; // increase deltaX by 50% each time it is used... 
   // exponentially grow the values we are trying to bound the solution.

   XsetMin = 0.5*span_length; // Take a guess at the location of the end of the anchor set zone
   if ( endType == pgsTypes::metEnd )
   {
      // need XsetMin measured from left end of girder
      XsetMin = Lg - XsetMin;
      deltaX *= -1;
   }

   Float64 dfpAT, dfpS;
   Float64 Dset1 = EvaluateAnchorSet(pPTData,pDuctData,ductIdx,endType,pLosses,fpj,Lg,frMinIter,XsetMin,&dfpAT,&dfpS);
   if ( IsEqual(Dset1,Dset) )
   {
      // Nailed it first guess!!!
      *pXsetMin  = XsetMin;
      *pDsetMin  = Dset;
      *pdfpATMin = dfpAT;
      *pdfpSMin  = dfpS;

      *pXsetMax  = *pXsetMin;
      *pDsetMax  = *pDsetMin;
      *pdfpATMax = *pdfpATMin;
      *pdfpSMax  = *pdfpSMin;

      return;
   }

   if ( Dset1 < Dset )
   {
      // XsetMin is too small... we have a lower bound
      *pXsetMin  = XsetMin;
      *pDsetMin  = Dset1;
      *pdfpATMin = dfpAT;
      *pdfpSMin  = dfpS;

      // find the upper bound
      XsetMax = XsetMin;
      while ( true )
      {
         XsetMax += deltaX;
         deltaX *= K;

         Dset1 = EvaluateAnchorSet(pPTData,pDuctData,ductIdx,endType,pLosses,fpj,Lg,frMinIter,XsetMax,&dfpAT,&dfpS);
         if ( Dset < Dset1 )
         {
            // LsetMax is too big... we have an upper bound
            *pXsetMax  = XsetMax;
            *pDsetMax  = Dset1;
            *pdfpATMax = dfpAT;
            *pdfpSMax  = dfpS;

            return;
         }
      }
   }
   else if ( Dset < Dset1 )
   {
      // LsetMin is too big... we have the upper bound
      *pXsetMax  = XsetMin;
      *pDsetMax  = Dset1;
      *pdfpATMax = dfpAT;
      *pdfpSMax  = dfpS;

      // find the lower bound
      while ( true )
      {
         XsetMin -= deltaX;
         deltaX *= K;

         Dset1 = EvaluateAnchorSet(pPTData,pDuctData,ductIdx,endType,pLosses,fpj,Lg,frMinIter,XsetMin,&dfpAT,&dfpS);
         if ( Dset1 < Dset )
         {
            // XsetMin is too small.. we have a lower bound
            *pXsetMin  = XsetMin;
            *pDsetMin  = Dset1;
            *pdfpATMin = dfpAT;
            *pdfpSMin  = dfpS;

            return;
         }
      }
   }
}

Float64 CTimeStepLossEngineer::EvaluateAnchorSet(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,LOSSES* pLosses,Float64 fpj,Float64 Lg,SectionLossContainer::iterator& frMinIter,Float64 Xset,Float64* pdfpAT,Float64* pdfpS)
{
   //
   // Computes Dset given an assumed length of the anchor set zone, Lset
   //
   FRICTIONLOSSDETAILS& minFrDetails = frMinIter->second.FrictionLossDetails[ductIdx];

   // Find friction loss at Xset
   Float64 dfpF_Xset;
   Float64 dfpF;
   Float64 Dset = 0;
   if ( endType == pgsTypes::metStart )
   {
      SectionLossContainer::iterator iter(pLosses->SectionLosses.begin());
      SectionLossContainer::iterator end(pLosses->SectionLosses.end());
      if ( minFrDetails.X < Xset )
      {
         // anchor set exceeds the length of the tendon
         Float64 X1 = minFrDetails.X;
         Float64 f1 = minFrDetails.dfpF;

         SectionLossContainer::iterator frIter = frMinIter;
         Float64 X2,f2;
         do
         {
            frIter--; // back up one location
            X2 = frIter->second.FrictionLossDetails[ductIdx].X;
            f2 = frIter->second.FrictionLossDetails[ductIdx].dfpF;
         } while (IsEqual(X1,X2));

         dfpF_Xset = ::LinInterp(Xset-X2,f2,f1,X1-X2);
         dfpF = f1;

         end = frMinIter;
      }
      else
      {
         // do a linear search for the loss details at locations that bound Lset
         Float64 X1  = iter->second.FrictionLossDetails[ductIdx].X;
         Float64 fr1 = iter->second.FrictionLossDetails[ductIdx].dfpF;
         iter++;
         for ( ; iter != end; iter++ )
         {
            Float64 X2  = iter->second.FrictionLossDetails[ductIdx].X;
            Float64 fr2 = iter->second.FrictionLossDetails[ductIdx].dfpF;

            if ( InRange(X1,Xset,X2) )
            {
               // use linear interpolation to get friction loss at Lset
               dfpF_Xset = ::LinInterp(Xset-X1,fr1,fr2,X2-X1);
               dfpF = 0;
               end = iter;
               break;
            }

            X1  = X2;
            fr1 = fr2;
         }
      }

      // Calculate incremental contribution to seating loss along the strand
      // This is numerical integration using the trapezoidal rule
      iter = pLosses->SectionLosses.begin();
      Float64 X1  = iter->second.FrictionLossDetails[ductIdx].X;
      Float64 fr1 = iter->second.FrictionLossDetails[ductIdx].dfpF;
      iter++;
      bool bDone = false;
      for ( ; !bDone; iter++ )
      {
         Float64 X2, fr2;

         if ( iter == end )
         {
            iter--;
            fr2 = iter->second.FrictionLossDetails[ductIdx].dfpF;

            X2  = Min(Lg,Xset);
            fr2 = (Xset < Lg ? dfpF_Xset : fr2);
            bDone = true;
         }
         else
         {
            X2  = iter->second.FrictionLossDetails[ductIdx].X;
            fr2 = iter->second.FrictionLossDetails[ductIdx].dfpF;
         }

         Float64 fpt1 = fpj - fr1;
         Float64 fpt2 = fpj - fr2;
         Float64 fps  = fpj - dfpF_Xset;
         Float64 dDset = (0.5*(fpt1+fpt2) - fps)*(X2-X1);
         Dset += dDset;

         X1  = X2;
         fr1 = fr2;
      }
   }
   else
   {
      SectionLossContainer::reverse_iterator rIter(pLosses->SectionLosses.rbegin());
      SectionLossContainer::reverse_iterator rIterEnd(pLosses->SectionLosses.rend());
      if ( Xset < minFrDetails.X )
      {
         // anchor set exceeds the length of the tendon
         Float64 X1 = minFrDetails.X;
         Float64 f1 = minFrDetails.dfpF;


         SectionLossContainer::iterator frIter = frMinIter;
         Float64 X2,f2;
         do
         {
            frIter++; // advance one location
            X2 = frIter->second.FrictionLossDetails[ductIdx].X;
            f2 = frIter->second.FrictionLossDetails[ductIdx].dfpF;
         } while (IsEqual(X1,X2));

         dfpF_Xset = ::LinInterp(Xset-X1,f1,f2,X2-X1);
         dfpF = f1;

         rIterEnd = SectionLossContainer::reverse_iterator(frMinIter);
      }
      else
      {
         Float64 X1  = rIter->second.FrictionLossDetails[ductIdx].X;
         Float64 fr1 = rIter->second.FrictionLossDetails[ductIdx].dfpF;
         rIter++;
         for ( ; rIter != rIterEnd; rIter++ )
         {
            Float64 X2  = rIter->second.FrictionLossDetails[ductIdx].X;
            Float64 fr2 = rIter->second.FrictionLossDetails[ductIdx].dfpF;

            if ( InRange(X2,Xset,X1) )
            {
               dfpF_Xset = ::LinInterp(Xset-X2,fr2,fr1,X1-X2);
               dfpF = 0;
               rIterEnd = rIter;
               break;
            }

            X1  = X2;
            fr1 = fr2;
         }
      }

      // Calculate incremental contribution to seating loss along the strand
      // This is numerical integration using the trapezoidal rule
      rIter = pLosses->SectionLosses.rbegin();
      Float64 X1  = rIter->second.FrictionLossDetails[ductIdx].X;
      Float64 fr1 = rIter->second.FrictionLossDetails[ductIdx].dfpF;
      rIter++;
      bool bDone = false;
      for ( ; !bDone; rIter++ )
      {
         Float64 X2, fr2;
         if ( rIter == rIterEnd )
         {
            rIter--;
            fr2 = rIter->second.FrictionLossDetails[ductIdx].dfpF;

            X2  = Max(Xset,0.0);
            fr2 = (0 < Xset ? dfpF_Xset : fr2);
            bDone = true;
         }
         else
         {
            X2  = rIter->second.FrictionLossDetails[ductIdx].X;
            fr2 = rIter->second.FrictionLossDetails[ductIdx].dfpF;
         }

         Float64 fpt1 = fpj - fr1;
         Float64 fpt2 = fpj - fr2;
         Float64 fps  = fpj - dfpF_Xset;
         Float64 dDset = (0.5*(fpt1+fpt2) - fps)*(X1-X2);
         Dset += dDset;

         X1  = X2;
         fr1 = fr2;
      }
   }

   *pdfpAT = 2*dfpF_Xset;

   if ( InRange(0.0,Xset,Lg) )
   {
      *pdfpS = 0;
   }
   else
   {
      *pdfpS  = 2*(dfpF_Xset - dfpF);
   }

   Float64 Ept = pPTData->pStrand->GetE();
   Dset /= Ept;
   return Dset;
}

LOSSDETAILS* CTimeStepLossEngineer::GetLossDetails(LOSSES* pLosses,const pgsPointOfInterest& poi)
{
   SectionLossContainer::iterator found(pLosses->SectionLosses.find(poi));
   ATLASSERT( found != pLosses->SectionLosses.end() );
   ATLASSERT(poi.GetID() == found->first.GetID());
   ATLASSERT(poi.GetID() == found->second.POI.GetID());
   return &(found->second);
}

std::vector<ProductForceType> CTimeStepLossEngineer::GetApplicableProductLoads(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bExternalForcesOnly)
{
   // creates a vector of the product force type for only those loads that are applied during this interval
   // or cause a force effect during this interval. An example of a load causing a force effect during
   // and interval after the load is applied is girder self-weight causes a secondary force effect when
   // temporary supports are removed
   //
   // if bExternalLoadsOnly is true, product force types are only for externally applied loads
   // Prestress, Post-tensioning, creep, shrinkage, and relaxation are excluded

   std::vector<ProductForceType> vProductForces;
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   std::vector<IntervalIndexType> vTSRemovalIntervals = m_pIntervals->GetTemporarySupportRemovalIntervals(segmentKey);
   bool bIsTempSupportRemovalInterval = false;
   if ( 0 < vTSRemovalIntervals.size() )
   {
      std::vector<IntervalIndexType>::iterator found = std::find(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),intervalIdx);
      bIsTempSupportRemovalInterval = (found == vTSRemovalIntervals.end() ? false : true);
   }

   bool bIsClosure = m_pPoi->IsInClosureJoint(poi);
   IntervalIndexType compositeClosureIntervalIdx = m_pIntervals->GetCompositeClosureJointInterval(segmentKey);

   // Force effects due to girder self weight occur at prestress release, storage, and erection of this interval,
   // erection of any other segment that is erected after this interval, and
   // any interval when a temporary support is removed after the segment is erected

   bool bGirderLoad = false;
   if ( bIsClosure )
   {
      // poi is at a closure
      if ( compositeClosureIntervalIdx <= intervalIdx )
      {
         // this is after the closure is composite so it is considered to be part of the girder

         // if segments are erected after this poi is composite, the dead load of those segments effect this poi
         IntervalIndexType lastSegmentErectionIntervalIdx = m_pIntervals->GetLastSegmentErectionInterval(segmentKey);
         bool bDoesDeadLoadOfSegmentEffectThisClosure = (compositeClosureIntervalIdx < lastSegmentErectionIntervalIdx && intervalIdx <= lastSegmentErectionIntervalIdx ? true : false);

         // if temporary supports are removed after this closure becomes composite then deflections happen at this poi
         // count the number of intervals in the vTSRemovalIntervals vector that are greater than compositeClosureIntervalIdx... if there is 1 or more
         // temporary support removal effects this poi
         bool bTSRemovedAfterCompositeClosure = (0 < std::count_if(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),std::bind2nd(std::greater_equal<IntervalIndexType>(),compositeClosureIntervalIdx)));
         
         if ( bDoesDeadLoadOfSegmentEffectThisClosure || (bIsTempSupportRemovalInterval && bTSRemovedAfterCompositeClosure) )
         {
            bGirderLoad = true;
         }
      }
   }
   else
   {
      // poi is in a girder segment

      // girder load comes into play at release (obviously) and when support locations change
      // for storage and erection
      IntervalIndexType releasePrestressIntervalIdx = m_pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType storageIntervalIdx = m_pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType erectSegmentIntervalIdx = m_pIntervals->GetErectSegmentInterval(segmentKey);

      // girder load comes into play if there are segments erected after this segment is erected
      // consider the case of this segment being a cantilevered pier segment and during this interval
      // a drop in segment is erected and that new segment is supported by the cantilever end of this
      // segment. The dead load reaction of the drop-in is a point load on the cantilver.
      IntervalIndexType lastSegmentErectionIntervalIdx = m_pIntervals->GetLastSegmentErectionInterval(segmentKey);
      bool bDoesDeadLoadOfAnotherSegmentEffectThisSegment = (erectSegmentIntervalIdx < lastSegmentErectionIntervalIdx && intervalIdx <= lastSegmentErectionIntervalIdx ? true : false);

      // if temporary supports are removed after this segment is erected then deflections happen at this poi
      // count the number of intervals in the vTSRemovalIntervals vector that are greater or equal than erectSegmentIntervalIdx... if there is 1 or more
      // temporary support removal effects this poi
      bool bTSRemovedAfterSegmentErection = (0 < std::count_if(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),std::bind2nd(std::greater_equal<IntervalIndexType>(),erectSegmentIntervalIdx)));

      if ( intervalIdx == releasePrestressIntervalIdx || // load first introduced
           intervalIdx == storageIntervalIdx || // supports move
           intervalIdx == erectSegmentIntervalIdx || // supports move
           bDoesDeadLoadOfAnotherSegmentEffectThisSegment || // this segment gets loaded from a construction event
           (bIsTempSupportRemovalInterval && bTSRemovedAfterSegmentErection) ) // this segments gets loadede from a construction event
      {
         bGirderLoad = true;
      }
   }

   if ( bGirderLoad )
   {
      vProductForces.push_back(pftGirder);

      if ( !bExternalForcesOnly )
      {
         vProductForces.push_back(pftPretension);
      }
   }

   // Force effects due to dead loads that are applied along with the slab self-weight occur
   // at the deck casting interval and any interval when a temporary support is removed after
   // the slab (and related) dead load is applied
   IntervalIndexType castDeckIntervalIdx = m_pIntervals->GetCastDeckInterval(segmentKey);
   bool bTSRemovedAfterDeckCasting = (0 < std::count_if(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),std::bind2nd(std::greater<IntervalIndexType>(),castDeckIntervalIdx)));
   if ( intervalIdx == castDeckIntervalIdx || (bIsTempSupportRemovalInterval && bTSRemovedAfterDeckCasting) )
   {
      vProductForces.push_back(pftDiaphragm); // verify this... are there cases when we don't apply a diaphragm load?

      pgsTypes::SupportedDeckType deckType = m_pBridge->GetDeckType();
      if ( deckType != pgsTypes::sdtNone )
      {
         vProductForces.push_back(pftSlab);
         vProductForces.push_back(pftSlabPad);
      }

      if ( deckType == pgsTypes::sdtCompositeSIP )
      {
         vProductForces.push_back(pftSlabPanel);
      }

      // only model construction loads if the magnitude is non-zero
      std::vector<ConstructionLoad> vConstructionLoads;
      m_pProductLoads->GetConstructionLoad(segmentKey,&vConstructionLoads);
      std::vector<ConstructionLoad>::iterator constrLoadIter(vConstructionLoads.begin());
      std::vector<ConstructionLoad>::iterator constrLoadIterEnd(vConstructionLoads.end());
      for ( ; constrLoadIter != constrLoadIterEnd; constrLoadIter++ )
      {
         ConstructionLoad& load = *constrLoadIter;
         if ( !IsZero(load.wStart) || !IsZero(load.wEnd) )
         {
            vProductForces.push_back(pftConstruction);
            break;
         }
      }

      if ( m_pProductLoads->HasShearKeyLoad(segmentKey) )
      {
         vProductForces.push_back(pftShearKey);
      }
   }

   IntervalIndexType installOverlayIntervalIdx = m_pIntervals->GetOverlayInterval(segmentKey);
   bool bTSRemovedAfterOverlayInstallation = (0 < std::count_if(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),std::bind2nd(std::greater<IntervalIndexType>(),installOverlayIntervalIdx)));
   if ( intervalIdx == installOverlayIntervalIdx || 
       (bIsTempSupportRemovalInterval && bTSRemovedAfterOverlayInstallation)
      )
   {
      vProductForces.push_back(pftOverlay);
   }

   IntervalIndexType installRailingSystemIntervalIdx = m_pIntervals->GetInstallRailingSystemInterval(segmentKey);
   bool bTSRemovedAfterRailingSystemInstalled = (0 < std::count_if(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),std::bind2nd(std::greater<IntervalIndexType>(),installRailingSystemIntervalIdx)));
   if ( intervalIdx == installRailingSystemIntervalIdx || 
        (bIsTempSupportRemovalInterval && bTSRemovedAfterRailingSystemInstalled)
      )
   {
      Float64 wTB = m_pProductLoads->GetTrafficBarrierLoad(segmentKey);
      Float64 wSW = m_pProductLoads->GetSidewalkLoad(segmentKey);

      if ( !IsZero(wTB) )
      {
         vProductForces.push_back(pftTrafficBarrier);
      }

      if ( !IsZero(wSW) )
      {
         vProductForces.push_back(pftSidewalk);
      }
   }


   // User defined loads

   // if user defined DC or DW is applied in this interval, or if they were applied
   // in a previous interval (before temporary support removal) and 
   // temporary supports are removed in this interval, include the product load type
   for ( int i = 0; i < 2; i++ )
   {
      ProductForceType pfType = (i == 0 ? pftUserDC : pftUserDW);

      // get all intervals when the user load is applied
      std::vector<IntervalIndexType> vUserLoadIntervals = m_pIntervals->GetUserDefinedLoadIntervals(segmentKey,pfType);
      // remove all intervals that occur after this interval
      vUserLoadIntervals.erase(std::remove_if(vUserLoadIntervals.begin(),vUserLoadIntervals.end(),std::bind2nd(std::greater<IntervalIndexType>(),intervalIdx)),vUserLoadIntervals.end());

      bool bIsUserDefinedLoadInterval = false;
      bool bIsTSRemovedAfterUserDefinedLoad = false;
      std::vector<IntervalIndexType>::iterator intervalIter(vUserLoadIntervals.begin());
      std::vector<IntervalIndexType>::iterator intervalIterEnd(vUserLoadIntervals.end());
      for ( ; intervalIter != intervalIterEnd; intervalIter++ )
      {
         IntervalIndexType loadingIntervalIdx = *intervalIter;
         if ( loadingIntervalIdx == intervalIdx )
         {
            // the user defined load applied is applied in this interval
            bIsUserDefinedLoadInterval = true;
         }

         // count the number of temporary support removal intervals that occur after the loading interval
         // if it is greater than zero this loading is applied when a temporary support is removed
         bool bTSRemovedAfterLoadingApplied = (0 < std::count_if(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),std::bind2nd(std::greater<IntervalIndexType>(),loadingIntervalIdx)));
         if ( bTSRemovedAfterLoadingApplied )
         {
            bIsTSRemovedAfterUserDefinedLoad = true;
         }
      }

      if ( bIsUserDefinedLoadInterval || (bIsTempSupportRemovalInterval && bIsTSRemovedAfterUserDefinedLoad) )
      {
         vProductForces.push_back(pfType);
      }
   }

   if ( !bIsClosure || (bIsClosure && compositeClosureIntervalIdx <= intervalIdx) )
   {
      if ( m_pIntervals->IsTendonStressingInterval(segmentKey,intervalIdx) )
      {
         if ( !bExternalForcesOnly )
         {
            vProductForces.push_back(pftPrimaryPostTensioning);
            vProductForces.push_back(pftSecondaryEffects);
         }
      }

      bool bIgnoreTimeDependentEffects = m_pLossParams->IgnoreTimeDependentEffects();
      if ( !bExternalForcesOnly && !bIgnoreTimeDependentEffects && !IsZero(m_pIntervals->GetDuration(segmentKey,intervalIdx)) )
      {
         // creep, shrinkage, and relaxation only occur in intervals with non-zero duration
         vProductForces.push_back(pftCreep);
         vProductForces.push_back(pftShrinkage);
         vProductForces.push_back(pftRelaxation);
      }
   }

   ATLASSERT(vProductForces.size() <= (bExternalForcesOnly ? 14 : GetProductForceCount()));

   return vProductForces;
}

void CTimeStepLossEngineer::MakeClosureJointAdjustment(IntervalIndexType intervalIdx,const pgsPointOfInterest& leftPoi,const pgsPointOfInterest& closurePoi,const pgsPointOfInterest& rightPoi,LOSSES* pLosses)
{
   // Take rigid body deflection at closure joint to be the deflection of the adjacent poi
   // at the end of the previous interval

   LOSSDETAILS* pLeftDetails    = GetLossDetails(pLosses,leftPoi);
   LOSSDETAILS* pClosureDetails = GetLossDetails(pLosses,closurePoi);
   LOSSDETAILS* pRightDetails   = GetLossDetails(pLosses,rightPoi);

   TIME_STEP_DETAILS& leftTsDetails(pLeftDetails->TimeStepDetails[intervalIdx-1]);
   TIME_STEP_DETAILS& rightTsDetails(pRightDetails->TimeStepDetails[intervalIdx-1]);
   TIME_STEP_DETAILS& closureTsDetails(pClosureDetails->TimeStepDetails[intervalIdx]);

   int n = GetProductForceCount();
   for ( int i = 0; i < n; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;

      // incremental deflection this interval is the deflection at the end of the previous interval
      // (rigid body movement)
      Float64 dD = (leftTsDetails.D[pfType] + rightTsDetails.D[pfType])/2.0;

      closureTsDetails.dD[pfType] += dD; // incremental deflection this interval
      closureTsDetails.D[pfType]  += dD; // deflection at the end of this interval

      // update the total deflection too
      closureTsDetails.dY += dD;
      closureTsDetails.Y  += dD;
   }
}

void CTimeStepLossEngineer::InitializeDeflectionCalculations()
{
   // clears the segment key so that it is basically unassigned
   // when InitializeErectionAdjustments is called for the first time for
   // a given interval, the segment key is guarenteed to not be equal to 
   // this segment key.
   m_SegmentKey = CSegmentKey();
}

void CTimeStepLossEngineer::InitializeErectionAdjustments(IntervalIndexType intervalIdx,const CSegmentKey& segmentKey,LOSSES* pLosses)
{
   ASSERT_SEGMENT_KEY(segmentKey); // must be a full segment key

   if ( m_SegmentKey.IsEqual(segmentKey) )
   {
      return; // adjustments have already been initialized, return now and don't duplicate work
   }

   // hang onto the current segment key
   m_SegmentKey = segmentKey;

   // reset the functions
   int n = sizeof(m_ErectionAdjustment)/sizeof(m_ErectionAdjustment[0]);
   for ( int i = 0; i < n; i++ )
   {
      m_ErectionAdjustment[i] = mathLinFunc2d();
   }

   IntervalIndexType erectionIntervalIdx = m_pIntervals->GetErectSegmentInterval(m_SegmentKey);

   if ( intervalIdx == erectionIntervalIdx )
   {
      // When a segment is erected, in general, the location of the supports are different than
      // where the segment was supported during storage. Consider a segment that is supported
      // very near its ends during storage, then, when erected, it is supported near one end and
      // near its center. This would be the case for a cantilevered segment that will receive a 
      // drop in segment later in the construction sequence.
      //
      //     ---------------------------------    Storage
      //     ^                               ^
      //
      //     ---------------------------------    Erection
      //     ^                     ^
      //
      // The deflections due to externally applied loads are self-adjusting. The change in moment and hence
      // the change in curvature account for the change is support locations. 
      //
      // The internal load deformations due to creep, shrinkage, and relaxation don't behave the same. 
      // For the segment described above, there will be inelastic deflections at mid-segment during
      // storage. The net deflection at the supports in the erected configuration must be zero. The
      // only way to accomplish this is to recogize that there is rigid body motion of the
      // inelastic deformations. That's the purpose of this code.
      //
      // Find the "hard" supports at the time of erection. These are the supports where the
      // deflections are zero relative to the placement of the erected segment. Get
      // the deflections at these locations in the previous interval. Compute the equation
      // of the straight line connecting these points. The negative of the y-value of this
      // line is the rigid body motion.
      Float64 signLeft(1.0);
      Float64 signRight(1.0);

      std::vector<ProductForceType> vProductForces;
      vProductForces.push_back(pftPretension);
      vProductForces.push_back(pftCreep);
      vProductForces.push_back(pftShrinkage);
      vProductForces.push_back(pftRelaxation);

      std::vector<pgsPointOfInterest> vPoi(m_pPoi->GetPointsOfInterest(m_SegmentKey,POI_INTERMEDIATE_TEMPSUPPORT | POI_INTERMEDIATE_PIER | POI_BOUNDARY_PIER | POI_ABUTMENT));
      if ( vPoi.size() < 2 )
      {
         const CPrecastSegmentData* pSegment = m_pBridgeDesc->GetPrecastSegmentData(m_SegmentKey);
         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSegment->GetSupport(pgsTypes::metStart,&pPier,&pTS);
         if ( pPier || (pTS && pTS->GetSupportType() == pgsTypes::ErectionTower) )
         {
            std::vector<pgsPointOfInterest> vPoi2(m_pPoi->GetPointsOfInterest(m_SegmentKey,POI_ERECTED_SEGMENT | POI_0L));
            ATLASSERT(vPoi2.size() == 1);
            vPoi.insert(vPoi.begin(),vPoi2.begin(),vPoi2.end());
         }

         pSegment->GetSupport(pgsTypes::metEnd,&pPier,&pTS);
         if ( pPier || (pTS && pTS->GetSupportType() == pgsTypes::ErectionTower) )
         {
            std::vector<pgsPointOfInterest> vPoi2(m_pPoi->GetPointsOfInterest(m_SegmentKey,POI_ERECTED_SEGMENT | POI_10L));
            ATLASSERT(vPoi2.size() == 1);
            vPoi.insert(vPoi.end(),vPoi2.begin(),vPoi2.end());
         }

         // eliminate duplicate POI
         vPoi.erase(std::unique(vPoi.begin(),vPoi.end()),vPoi.end());

         // vPoi will have two entries if the segment is supported at one end by a pier/abutment and the
         // other end by an erection tower, or if it is supported at both ends by an erection tower.

         // if the segment is supported on one or both ends by strong backs vPoi.size() will be less than 2

         if ( vPoi.size() < 2 )
         {
            // if there is less than 2 hard supports we have to use the adjacent segments
            // to figure out the erection adjustment equation
            SegmentIndexType nSegments = m_pBridge->GetSegmentCount(segmentKey);
            
            if ( m_SegmentKey.segmentIndex == 0 )
            {
               // the first segment has one hard support... use the start poi for the next segment
               ATLASSERT(vPoi.size() == 1);
               CSegmentKey nextSegmentKey(m_SegmentKey);
               nextSegmentKey.segmentIndex++;
               std::vector<pgsPointOfInterest> vPoiR(m_pPoi->GetPointsOfInterest(nextSegmentKey,POI_RELEASED_SEGMENT | POI_0L));
               ATLASSERT(vPoiR.size() == 1);
               vPoi.push_back(vPoiR.front());
               signRight = -1;
            }
            else if ( m_SegmentKey.segmentIndex == nSegments-1 )
            {
               // the last segment has one hard support... use the end poi for the previous segment
               ATLASSERT(vPoi.size() == 1);
               CSegmentKey prevSegmentKey(m_SegmentKey);
               prevSegmentKey.segmentIndex--;
               std::vector<pgsPointOfInterest> vPoiL(m_pPoi->GetPointsOfInterest(prevSegmentKey,POI_RELEASED_SEGMENT | POI_10L));
               ATLASSERT(vPoiL.size() == 1);
               vPoi.insert(vPoi.begin(),vPoiL.front());
               signLeft = -1;
            }
            else
            {
               // this is a drop-in...
               ATLASSERT(pSegment->IsDropIn() == true);

               vProductForces.push_back(pftGirder);

               vPoi.clear();

               CSegmentKey prevSegmentKey(m_SegmentKey);
               prevSegmentKey.segmentIndex--;

               
               CSegmentKey nextSegmentKey(m_SegmentKey);
               nextSegmentKey.segmentIndex++;

               std::vector<pgsPointOfInterest> vPoiL(m_pPoi->GetPointsOfInterest(prevSegmentKey,POI_RELEASED_SEGMENT | POI_10L));
               std::vector<pgsPointOfInterest> vPoiR(m_pPoi->GetPointsOfInterest(nextSegmentKey,POI_RELEASED_SEGMENT | POI_0L));
               ATLASSERT(vPoiL.size() == 1 && vPoiR.size() == 1);
               vPoi.push_back(vPoiL.front());
               vPoi.push_back(vPoiR.front());

               signLeft  = -1;
               signRight = -1;
            }
         }
      }

      ATLASSERT( 2 <= vPoi.size() );
      pgsPointOfInterest leftPoi(vPoi.front());
      pgsPointOfInterest rightPoi(vPoi.back());

      LOSSDETAILS* pLeftDetails    = GetLossDetails(pLosses,leftPoi);
      LOSSDETAILS* pRightDetails   = GetLossDetails(pLosses,rightPoi);
      ATLASSERT(pLeftDetails != NULL && pRightDetails != NULL);

      Float64 Xs = m_pPoi->ConvertPoiToGirderPathCoordinate(leftPoi);
      Float64 Xe = m_pPoi->ConvertPoiToGirderPathCoordinate(rightPoi);
      Float64 Dx = Xe-Xs;
      ATLASSERT(Xs < Xe && !IsZero(Dx));

      std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
      std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
      for ( ; pfIter != pfIterEnd; pfIter++ )
      {
         ProductForceType pfType = *pfIter;

         Float64 Ys = -signLeft*pLeftDetails->TimeStepDetails[intervalIdx-1].D[pfType];
         Float64 Ye = -signRight*pRightDetails->TimeStepDetails[intervalIdx-1].D[pfType];

         Float64 Dy = Ye-Ys;
         Float64 slope = Dy/Dx;

         // y = mx+b
         // b = y-mx
         Float64 b = Ys - slope*Xs;

         m_ErectionAdjustment[pfType].SetSlope(slope);
         m_ErectionAdjustment[pfType].SetYIntercept(b);
      }
   }
}

void CTimeStepLossEngineer::GetErectionAdjustment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,ProductForceType pfType,Float64* pD,Float64* pR)
{
   ATLASSERT(m_SegmentKey == poi.GetSegmentKey());

   *pD = 0;
   *pR = 0;

   if ( poi.HasAttribute(POI_CLOSURE) )
   {
      // if POI is at a closure joint there isn't an erection adjustment.
      // closures are cast after erection
      return;
   }

   Float64 Xgp = m_pPoi->ConvertPoiToGirderPathCoordinate(poi);
   *pD = m_ErectionAdjustment[pfType].Evaluate(Xgp);
   *pR = m_ErectionAdjustment[pfType].GetSlope();
}

void CTimeStepLossEngineer::InitializeStrandTypes(const CSegmentKey& segmentKey)
{
   std::vector<pgsTypes::StrandType> strandTypes;
   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
      if ( 0 < m_pStrandGeom->GetStrandCount(segmentKey,strandType) )
      {
         strandTypes.push_back(strandType);
      }
   }

   m_StrandTypes.insert(std::make_pair(segmentKey,strandTypes));
}

const std::vector<pgsTypes::StrandType>& CTimeStepLossEngineer::GetStrandTypes(const CSegmentKey& segmentKey)
{
   std::map<CSegmentKey,std::vector<pgsTypes::StrandType>>::iterator found(m_StrandTypes.find(segmentKey));
   if ( found == m_StrandTypes.end() )
   {
      InitializeStrandTypes(segmentKey);
      found = m_StrandTypes.find(segmentKey);
   }

   return found->second;
}

int CTimeStepLossEngineer::GetProductForceCount()
{
   // returns the number of product forces that we need to consider from the ProductForceType enum.
   // all off the product forces count except for the two special cases at the end of the enum.
   int n = (int)pftProductForceTypeCount;
   n -= 2; // remove pftTotalPostTensioning and pftOverlayRating from the count as they are special cases and don't apply here
   return n;
}
