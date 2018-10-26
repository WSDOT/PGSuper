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
#include "TimeStepLossEngineer.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\Intervals.h>
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

//#define IGNORE_CREEP_EFFECTS
//#define IGNORE_SHRINKAGE_EFFECTS
//#define IGNORE_RELAXATION_EFFECTS



/////////////////////////////////////////////////////////////////////////////
// CTimeStepLossEngineer
HRESULT CTimeStepLossEngineer::FinalConstruct()
{
   return S_OK;
}

void CTimeStepLossEngineer::SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;
}

const LOSSDETAILS* CTimeStepLossEngineer::GetLosses(const pgsPointOfInterest& poi)
{
#pragma Reminder("UPDATE: Time Step Model")
   // THINK ABOUT HOW TIME STEP CAN BE RUN FROM INTERVAL 0 to INTERVAL i 
   // INSTEAD OF RUNNING FOR ALL INTERVALS ALL AT ONCE
   std::map<CGirderKey,LOSSES>::iterator found;
   CGirderKey girderKey(poi.GetSegmentKey());
   found = m_Losses.find( girderKey );
   if ( found == m_Losses.end() )
   {
      // losses haven't been computed for this girder yet.... compute them
      ComputeLosses(girderKey);

      found = m_Losses.find( girderKey );
      ATLASSERT(found != m_Losses.end());
   }

   std::map<pgsPointOfInterest,LOSSDETAILS>& losses(found->second.SectionLosses);
   std::map<pgsPointOfInterest,LOSSDETAILS>::iterator poiFound = losses.find(poi);

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
      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter1(losses.begin());
      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter2(losses.begin());
      iter2++;
      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator end(losses.end());
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
   return &(*poiFound).second;
}

const LOSSDETAILS* CTimeStepLossEngineer::GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config)
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
   std::map<CGirderKey,LOSSES>::const_iterator found;
   found = m_Losses.find( girderKey );
   if ( found == m_Losses.end() )
   {
      // losses not found for this girder
      ComputeLosses(girderKey);

      found = m_Losses.find( girderKey );
      ATLASSERT(found != m_Losses.end());
   }

   return &(*found).second.AnchorSet[ductIdx];
}

Float64 CTimeStepLossEngineer::GetElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType)
{
#pragma Reminder("IMPLEMENT CTimeStepLossEngineer::GetElongation()")
   // Getting elongation is an important calculation for post-tensioning operations
   ATLASSERT(false);
   return 0;
}

void CTimeStepLossEngineer::ComputeLosses(const CGirderKey& girderKey)
{
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Computing prestress losses"));


   LOSSES losses;

   ComputeFrictionLosses(girderKey,&losses);
   ComputeAnchorSetLosses(girderKey,&losses);
   ComputeSectionLosses(girderKey,&losses);

   m_Losses.insert(std::make_pair(girderKey,losses));
}

void CTimeStepLossEngineer::ComputeFrictionLosses(const CGirderKey& girderKey,LOSSES* pLosses)
{
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Computing friction losses"));

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(IPosttensionForce,pPTForce);
   GET_IFACE(ITendonGeometry,pTendonGeom);
   GET_IFACE(IBridge,pBridge);

   GET_IFACE(ILossParameters,pLossParams);
   Float64 Dset, wobble, friction;
   pLossParams->GetTendonPostTensionParameters(&Dset,&wobble,&friction);

   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPoi(pIPOI->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)));

   GET_IFACE(IGirder,pGdr);
   WebIndexType nWebs = pGdr->GetWebCount(girderKey);

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
      const CSplicedGirderData* pGirder = pIBridgeDesc->GetGirder(poi.GetSegmentKey());
      const CPTData* pPTData = pGirder->GetPostTensioning();

      DuctIndexType nDucts = pTendonGeom->GetDuctCount(poi.GetSegmentKey());
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         FRICTIONLOSSDETAILS frDetails;

         const CDuctData* pDuct = pPTData->GetDuct(ductIdx/nWebs);
         Float64 Pj;
         if ( pDuct->bPjCalc )
         {
            Pj = pPTForce->GetPjackMax(poi.GetSegmentKey(),pDuct->nStrands);
         }
         else
         {
            Pj = pDuct->Pj;
         }

         Float64 aps = pPTData->pStrand->GetNominalArea();
         StrandIndexType nStrands = pDuct->nStrands;
         Float64 Aps = aps*nStrands;
         Float64 fpj = (nStrands == 0 ? 0 : Pj/Aps);
         
         Float64 Xg = pIPOI->ConvertPoiToGirderCoordinate(poi); // distance along girder
         Float64 Lg = pBridge->GetGirderLength(poi.GetSegmentKey());

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

         Float64 alpha = pTendonGeom->GetAngularChange(poi,ductIdx,endType);

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
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Computing anchor set losses"));

   // First, compute the seating wedge, then compute anchor set loss at each POI
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder,pIGirder);

   Float64 girder_length = pBridge->GetGirderLength(girderKey);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CSplicedGirderData* pGirder = pIBridgeDesc->GetGirder(girderKey);
   const CPTData* pPTData = pGirder->GetPostTensioning();

   GET_IFACE(ILossParameters,pLossParams);
   Float64 Dset, wobble, friction;
   pLossParams->GetTendonPostTensionParameters(&Dset,&wobble,&friction);

   WebIndexType nWebs = pIGirder->GetWebCount(girderKey);


#pragma Reminder("UPDATE: this assumes that the PT starts/ends at the face of girder")
   // the input allows the PT to start and end at any arbitrary point along the girder

   GET_IFACE(ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      const CDuctData* pDuct = pPTData->GetDuct(ductIdx/nWebs);

      // find location of minimum friction loss. this is the location of no movement in the strand
      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator frMinIter;
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
         std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter(pLosses->SectionLosses.begin());
         std::map<pgsPointOfInterest,LOSSDETAILS>::iterator end(pLosses->SectionLosses.end());
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

   std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter(pLosses->SectionLosses.begin());
   std::map<pgsPointOfInterest,LOSSDETAILS>::iterator end(pLosses->SectionLosses.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi(iter->first);
      LOSSDETAILS& details(iter->second);

      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         ANCHORSETDETAILS& anchorSetDetails( pLosses->AnchorSet[ductIdx] );
         FRICTIONLOSSDETAILS& frDetails(details.FrictionLossDetails[ductIdx]);

         Float64 dfpA[2] = {0,0};
         const CDuctData* pDuct = pPTData->GetDuct(ductIdx/nWebs);
         if ( frDetails.X <= anchorSetDetails.Lset[pgsTypes::metStart] )
         {
            // POI is in the left anchorage zone
            if ( IsZero(anchorSetDetails.Lset[pgsTypes::metStart]) )
               dfpA[pgsTypes::metStart] = 0;
            else
               dfpA[pgsTypes::metStart] = ::LinInterp(frDetails.X,anchorSetDetails.dfpAT[pgsTypes::metStart],anchorSetDetails.dfpS[pgsTypes::metStart],anchorSetDetails.Lset[pgsTypes::metStart]);
         }

         if ( girder_length-anchorSetDetails.Lset[pgsTypes::metEnd] <= frDetails.X )
         {
            // POI is in the right anchorage zone
            if ( IsZero(anchorSetDetails.Lset[pgsTypes::metEnd]) )
               dfpA[pgsTypes::metEnd] = 0;
            else
               dfpA[pgsTypes::metEnd] = ::LinInterp(anchorSetDetails.Lset[pgsTypes::metEnd] - (girder_length-frDetails.X),anchorSetDetails.dfpS[pgsTypes::metEnd],anchorSetDetails.dfpAT[pgsTypes::metEnd],anchorSetDetails.Lset[pgsTypes::metEnd]);
         }

         frDetails.dfpA = dfpA[pgsTypes::metStart] + dfpA[pgsTypes::metEnd];
      }

   }
}

void CTimeStepLossEngineer::ComputeSectionLosses(const CGirderKey& girderKey,LOSSES* pLosses)
{
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount(girderKey);
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      CString strMsg;
      strMsg.Format(_T("Performing time-step analysis: Interval %d of %d"),LABEL_INTERVAL(intervalIdx),nIntervals);
      pProgress->UpdateMessage(strMsg);

      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter(pLosses->SectionLosses.begin());
      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator end(pLosses->SectionLosses.end());

      // Initialize Time Step Analysis
      // Basically computes the restraining forces at each POI
      for ( ; iter != end; iter++ )
      {
         LOSSDETAILS& details(iter->second);
         const pgsPointOfInterest& poi(iter->first);

         InitializeTimeStepAnalysis(intervalIdx,poi,details);
      }

      if ( 0 < pIntervals->GetDuration(girderKey,intervalIdx) )
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

            IntervalIndexType compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(closureKey);

            const CClosureJointData* pClosure = pIBridgeDesc->GetClosureJointData(closureKey);
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
   // Computes the force required to restrain the initial strains due to creep, shrinkage, and relaxation at each POI

   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IGirder,pGirder);
   GET_IFACE(IMaterials,pMaterials);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(ITendonGeometry,pTendonGeom);
   GET_IFACE(ILongRebarGeometry,pRebarGeom);
   GET_IFACE(ICombinedForces,pForces);
   GET_IFACE(IPretensionForce,pPretensionForce);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   CGirderKey girderKey(segmentKey);


   DuctIndexType nDucts = pTendonGeom->GetDuctCount(segmentKey);

   IntervalIndexType stressStrandsIntervalIdx = pIntervals->GetStressStrandInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval(segmentKey);

   bool bIsClosure = poi.HasAttribute(POI_CLOSURE);
   IntervalIndexType compositeClosureIntervalIdx = (bIsClosure ? pIntervals->GetCompositeClosureJointInterval(segmentKey) : INVALID_INDEX);


   // Material Properties
   Float64 EGirder = (bIsClosure ? pMaterials->GetClosureJointEc(segmentKey,intervalIdx) : pMaterials->GetSegmentEc(segmentKey,intervalIdx));
   Float64 EDeck = pMaterials->GetDeckEc(segmentKey,intervalIdx);
   Float64 EStrand[3] = { pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Straight)->GetE(),
                          pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Harped)->GetE(),
                          pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetE()};
   Float64 ETendon = pMaterials->GetTendonMaterial(girderKey)->GetE();
   
   Float64 gdrHeight = pGirder->GetHeight(poi);

   // Initialize time step details
   TIME_STEP_DETAILS tsDetails;

   // TIME PARAMETERS
   tsDetails.intervalIdx = intervalIdx;
   tsDetails.tStart      = pIntervals->GetStart(segmentKey,intervalIdx);
   tsDetails.tMiddle     = pIntervals->GetMiddle(segmentKey,intervalIdx);
   tsDetails.tEnd        = pIntervals->GetEnd(segmentKey,intervalIdx);

   // TRANSFORMED PROPERTIES OF COMPOSITE SECTION (all parts are transformed to equivalent girder concrete)
   tsDetails.Atr = pSectProp->GetAg(pgsTypes::sptTransformed,intervalIdx,poi);
   tsDetails.Itr = pSectProp->GetIx(pgsTypes::sptTransformed,intervalIdx,poi);
   tsDetails.Ytr = -pSectProp->GetY(pgsTypes::sptTransformed,intervalIdx,poi,pgsTypes::TopGirder); // Negative because this is measured down from Y=0 at the top of the girder

   // SEGMENT PARAMETERS

   // net section properties of segment
   tsDetails.Girder.An  = pSectProp->GetNetAg(intervalIdx,poi);
   tsDetails.Girder.In  = pSectProp->GetNetIg(intervalIdx,poi);
   tsDetails.Girder.Yn  = -pSectProp->GetNetYtg(intervalIdx,poi);
   tsDetails.Girder.H   = gdrHeight;

   // DECK PARAMETERS

   // net section properties of deck
   tsDetails.Deck.An  = pSectProp->GetNetAd(intervalIdx,poi);
   tsDetails.Deck.In  = pSectProp->GetNetId(intervalIdx,poi);
   tsDetails.Deck.Yn  = pSectProp->GetNetYbd(intervalIdx,poi); // distance from CG of net section to bottom of deck
   tsDetails.Deck.H   = tsDetails.Deck.Yn + pSectProp->GetNetYtd(intervalIdx,poi);

   // deck rebar
   if ( compositeDeckIntervalIdx <= intervalIdx  )
   {
      // deck is composite so the rebar is in play
      tsDetails.DeckRebar[pgsTypes::drmTop   ].As = pRebarGeom->GetAsTopMat(poi,ILongRebarGeometry::All);
      tsDetails.DeckRebar[pgsTypes::drmTop   ].Ys = pRebarGeom->GetTopMatLocation(poi,ILongRebarGeometry::All);

      tsDetails.DeckRebar[pgsTypes::drmBottom].As = pRebarGeom->GetAsBottomMat(poi,ILongRebarGeometry::All);
      tsDetails.DeckRebar[pgsTypes::drmBottom].Ys = pRebarGeom->GetBottomMatLocation(poi,ILongRebarGeometry::All);
   }

   // Girder/Closure Rebar
   CComPtr<IRebarSection> rebar_section;
   pRebarGeom->GetRebars(poi,&rebar_section);

   // POI is in a precast segment....
   CComPtr<IEnumRebarSectionItem> enum_items;
   rebar_section->get__EnumRebarSectionItem(&enum_items);
   CComPtr<IRebarSectionItem> item;
   while ( enum_items->Next(1,&item,NULL) != S_FALSE )
   {
      TIME_STEP_REBAR tsRebar;

      if ( (bIsClosure && intervalIdx < compositeClosureIntervalIdx) // POI is in a closure and the closure is not composite with the girder yet
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
      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::StrandType strandType = pgsTypes::StrandType(i);

         // time from strand stressing to end of this interval
         Float64 tStressing       = pIntervals->GetStart(segmentKey,stressStrandsIntervalIdx);
         Float64 tEndThisInterval = pIntervals->GetEnd(segmentKey,intervalIdx);

#if defined LUMP_STRANDS
         tsDetails.Strands[strandType].tEnd = Max(0.0,tEndThisInterval - tStressing);

         // Assumes transformed section properties are based on the strands being lumped in one location
         // Consider modeling each strand individually. This would be a more accurate analysis, however
         // it would take longer. Instead of looping over three strand types, we'll have to loop over
         // each strand for each strand type.

         // section properties
         tsDetails.Strands[strandType].As = pStrandGeom->GetStrandArea(segmentKey,intervalIdx,strandType);

         // location of strands from top of girder.... need to use eccentricity based on transformed section properties
         // Want Ys to be < 0 if below top of girder... that's where there is a negative sign
         Float64 nEffectiveStrands = 0;
         tsDetails.Strands[strandType].Ys = (intervalIdx < stressStrandsIntervalIdx ? 0 : pStrandGeom->GetStrandOffset(intervalIdx,poi,strandType,&nEffectiveStrands));
         if ( nEffectiveStrands == 0 )
         {
            tsDetails.Strands[strandType].Ys = 0;
         }

         if ( intervalIdx == stressStrandsIntervalIdx )
         {
            // Strands are stress in this interval.. get Pjack and fpj
            tsDetails.Strands[strandType].Pj  = (tsDetails.Strands[strandType].As == 0 ? 0 : pStrandGeom->GetPjack(segmentKey,strandType));
            tsDetails.Strands[strandType].fpj = (tsDetails.Strands[strandType].As == 0 ? 0 : tsDetails.Strands[strandType].Pj/tsDetails.Strands[strandType].As);

            // strands relax over the duration of the interval. compute the amount of relaxation
   #if defined IGNORE_RELAXATION_EFFECTS
            tsDetails.Strands[strandType].fr = 0;
   #else
            tsDetails.Strands[strandType].fr = pMaterials->GetStrandRelaxation(segmentKey,tsDetails.Strands[strandType].tEnd,0.0,tsDetails.Strands[strandType].fpj,strandType);
   #endif
         }
         else
         {
            // strands were stressed in a previous interval
            if ( intervalIdx == releaseIntervalIdx )
            {
               // accounts for lack of development and location of debonding
               Float64 xfer_factor = pPretensionForce->GetXferLengthAdjustment(poi,strandType);

               // this the interval when the prestress force is release into the girders, apply the
               // prestress as an external force. The prestress force is the area of strand times
               // the effective stress in the strands at the end of the previous interval.
               // Negative sign because the force resisted by the girder section is equal and opposite
               // the force in the strand (stands put a compression force into the girder)
               Float64 P = -xfer_factor*tsDetails.Strands[strandType].As*details.TimeStepDetails[intervalIdx-1].Strands[strandType].fpe;
               tsDetails.dP[pftPrestress] += P;
               tsDetails.dM[pftPrestress] += P*(tsDetails.Ytr - tsDetails.Strands[strandType].Ys);
            }

            // relaxation during this interval
            // by using the effective prestress at the end of the previous interval we get a very good approximation for 
            // the actual (reduced) relaxation. See "Time-Dependent Analysis of Composite Frames", Tadros, Ghali, Dilger, pg 876
   #if defined IGNORE_RELAXATION_EFFECTS
            tsDetails.Strands[strandType].fr = 0;
   #else
            tsDetails.Strands[strandType].fr = pMaterials->GetStrandRelaxation(segmentKey,tsDetails.Strands[strandType].tEnd,details.TimeStepDetails[intervalIdx-1].Strands[strandType].tEnd,details.TimeStepDetails[intervalIdx-1].Strands[strandType].fpe,strandType);
   #endif
         }

         // apparent strain due to relacation
         tsDetails.Strands[strandType].er = tsDetails.Strands[strandType].fr/EStrand[strandType]; // Tadros 1977, second term in Eqn 8

         // force required to resist apparent relaxation strain
         tsDetails.Strands[strandType].PrRelaxation = -tsDetails.Strands[strandType].fr*tsDetails.Strands[strandType].As; // Tadros 1977, Eqn 10
#else
         Float64 Pjack = pStrandGeom->GetPjack(segmentKey,strandType);
         StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey,strandType);
         if ( 0 < nStrands )
         {
            Pjack /= nStrands; // Pjack per strand
         }

         // accounts for lack of development and location of debonding
         Float64 xfer_factor = pPretensionForce->GetXferLengthAdjustment(poi,strandType);

         for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
         {
            TIME_STEP_STRAND strand;
            strand.tEnd = Max(0.0,tEndThisInterval - tStressing);

            if ( !bIsClosure )
            {
               // only fill up with data if NOT at closure joint...

               // section properties
               strand.As = pMaterials->GetStrandMaterial(segmentKey,strandType)->GetNominalArea();

               CComPtr<IPoint2d> point;
               pStrandGeom->GetStrandPosition(poi,strandIdx,strandType,&point);

               point->get_Y(&strand.Ys);

               if ( intervalIdx == stressStrandsIntervalIdx )
               {
                  // Strands are stress in this interval.. get Pjack and fpj
                  strand.Pj  = (IsZero(strand.As) ? 0 : Pjack);
                  strand.fpj = (IsZero(strand.As) ? 0 : strand.Pj/strand.As);

                  // strands relax over the duration of the interval. compute the amount of relaxation
   #if defined IGNORE_RELAXATION_EFFECTS
                  strand.fr = 0;
   #else
                  strand.fr = pMaterials->GetStrandRelaxation(segmentKey,strand.tEnd,0.0,strand.fpj,strandType);
   #endif
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
                     Float64 P = -xfer_factor*strand.As*details.TimeStepDetails[intervalIdx-1].Strands[strandType][strandIdx].fpe;
                     tsDetails.dP[pftPrestress] += P;
                     tsDetails.dM[pftPrestress] += P*(tsDetails.Ytr - strand.Ys);
                  }

                  // relaxation during this interval
                  // by using the effective prestress at the end of the previous interval we get a very good approximation for 
                  // the actual (reduced) relaxation. See "Time-Dependent Analysis of Composite Frames", Tadros, Ghali, Dilger, pg 876
   #if defined IGNORE_RELAXATION_EFFECTS
                  strand.fr = 0;
   #else
                  strand.fr = pMaterials->GetStrandRelaxation(segmentKey,strand.tEnd,details.TimeStepDetails[intervalIdx-1].Strands[strandType][strandIdx].tEnd,details.TimeStepDetails[intervalIdx-1].Strands[strandType][strandIdx].fpe,strandType);
   #endif
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
      IntervalIndexType stressTendonIntervalIdx  = pIntervals->GetStressTendonInterval(girderKey,ductIdx);

      TIME_STEP_STRAND tsTendon;

      Float64 tStressing = pIntervals->GetStart(girderKey,stressTendonIntervalIdx);
      Float64 tEndThisInterval = pIntervals->GetEnd(girderKey,intervalIdx);
      tsTendon.tEnd = Max(0.0,tEndThisInterval - tStressing);
      tsTendon.As = pTendonGeom->GetTendonArea(girderKey,intervalIdx,ductIdx);
      tsTendon.Ys = (intervalIdx < stressTendonIntervalIdx ? 0 : pTendonGeom->GetDuctOffset(intervalIdx,poi,ductIdx));

      Float64 dfpF = details.FrictionLossDetails[ductIdx].dfpF; // friction loss at this POI
      Float64 dfpA = details.FrictionLossDetails[ductIdx].dfpA; // anchor set loss at this POI
      // The jacking force that is transfered to the girder is Pjack - (Friction Loss + Anchor Set Loss)Apt
      tsTendon.Pj  = (tsTendon.As == 0 ? 0 : pTendonGeom->GetPjack(girderKey,ductIdx) - tsTendon.As*(dfpF + dfpA));
      tsTendon.fpj = (tsTendon.As == 0 ? 0 : tsTendon.Pj/tsTendon.As);

      // time from tendon stressing to end of this interval
      if ( intervalIdx < stressTendonIntervalIdx )
      {
         // tendons not installed yet, no relaxation
         tsTendon.fr = 0;
      }
      else if ( intervalIdx == stressTendonIntervalIdx )
      {
         // this the interval when the pt force is release into the girders, apply the
         // prestress as an external force. The prestress force is the area of strand times
         // the effective stress at the end of the previous interval.
         // Negative sign because the force resisted by the girder section is equal and opposite
         // the force in the strand
         Float64 P = -tsTendon.As*tsTendon.fpj;
         tsDetails.dP[pftPrimaryPostTensioning] += P;
         tsDetails.dM[pftPrimaryPostTensioning] += P*(tsDetails.Ytr - tsTendon.Ys);

         // relaxation during the stressing interval
#if defined IGNORE_RELAXATION_EFFECTS
         tsTendon.fr = 0;
#else
         tsTendon.fr = pMaterials->GetTendonRelaxation(girderKey,ductIdx,tsTendon.tEnd,0.0,tsTendon.fpj);
#endif
      }
      else
      {
         // by using the effective prestress at the end of the previous interval we get a very good approximation for 
         // the actual (reduced) relaxation. See "Time-Dependent Analysis of Composite Frames", Tadros, Ghali, Dilger, pg 876
#if defined IGNORE_RELAXATION_EFFECTS
         tsTendon.fr = 0;
#else
         tsTendon.fr = pMaterials->GetTendonRelaxation(girderKey,ductIdx,tsTendon.tEnd,details.TimeStepDetails[intervalIdx-1].Tendons[ductIdx].tEnd,details.TimeStepDetails[intervalIdx-1].Tendons[ductIdx].fpe);
#endif
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
         // Girder
#if defined IGNORE_CREEP_EFFECTS
         Float64 Cs = 0;
         Float64 Ce = 0;
#else
         Float64 Cs = (bIsClosure ? pMaterials->GetClosureJointCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::Start) : pMaterials->GetSegmentCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::Start));
         Float64 Ce = (bIsClosure ? pMaterials->GetClosureJointCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::End)   : pMaterials->GetSegmentCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::End));
#endif

         Float64 EGirder = (bIsClosure ? pMaterials->GetClosureJointEc(segmentKey,i) : pMaterials->GetSegmentEc(segmentKey,i));
         Float64 EAGirder = EGirder*details.TimeStepDetails[i].Girder.An;
         Float64 EIGirder = EGirder*details.TimeStepDetails[i].Girder.In;

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
            dP_Girder += details.TimeStepDetails[i].Girder.dP[pfType];
            dM_Girder += details.TimeStepDetails[i].Girder.dM[pfType];

            dP_Deck += details.TimeStepDetails[i].Deck.dP[pfType];
            dM_Deck += details.TimeStepDetails[i].Deck.dM[pfType];
         }

         Float64 e = IsZero(EAGirder) ? 0 : (Ce-Cs)*dP_Girder/EAGirder;
         Float64 r = IsZero(EIGirder) ? 0 : (Ce-Cs)*dM_Girder/EIGirder;

         tsDetails.Girder.e += e;
         tsDetails.Girder.r += r;

         TIME_STEP_CONCRETE::CREEP_STRAIN creepStrain;
         TIME_STEP_CONCRETE::CREEP_CURVATURE creepCurvature;

         creepStrain.A = details.TimeStepDetails[i].Girder.An;
         creepStrain.E = EGirder;
         creepStrain.P = dP_Girder;
         creepStrain.Cs = Cs;
         creepStrain.Ce = Ce;
         creepStrain.e = e;

         creepCurvature.I = details.TimeStepDetails[i].Girder.In;
         creepCurvature.E = EGirder;
         creepCurvature.M = dM_Girder;
         creepCurvature.Cs = Cs;
         creepCurvature.Ce = Ce;
         creepCurvature.r = r;

         tsDetails.Girder.ec.push_back(creepStrain);
         tsDetails.Girder.rc.push_back(creepCurvature);

         // Deck
#if defined IGNORE_CREEP_EFFECTS
         Cs = 0;
         Ce = 0;
#else
         Cs = pMaterials->GetDeckCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::Start);
         Ce = pMaterials->GetDeckCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::End);
#endif

         Float64 Edeck = pMaterials->GetDeckEc(segmentKey,i);
         Float64 EAdeck = Edeck*details.TimeStepDetails[i].Deck.An;
         Float64 EIdeck = Edeck*details.TimeStepDetails[i].Deck.In;

         e = IsZero(EAdeck) ? 0 : (Ce-Cs)*dP_Deck/EAdeck;
         r = IsZero(EIdeck) ? 0 : (Ce-Cs)*dM_Deck/EIdeck;

         tsDetails.Deck.e += e;
         tsDetails.Deck.r += r;

         creepStrain.A = details.TimeStepDetails[i].Deck.An;
         creepStrain.E = Edeck;
         creepStrain.P = dP_Deck;
         creepStrain.Cs = Cs;
         creepStrain.Ce = Ce;
         creepStrain.e = e;

         creepCurvature.I = details.TimeStepDetails[i].Deck.In;
         creepCurvature.E = Edeck;
         creepCurvature.M = dM_Deck;
         creepCurvature.Cs = Cs;
         creepCurvature.Ce = Ce;
         creepCurvature.r = r;

         tsDetails.Deck.ec.push_back(creepStrain);
         tsDetails.Deck.rc.push_back(creepCurvature);
      }

      // Compute total unrestrained deformation due to creep and shrinkage during this interval
      // AKA Compute initial strain conditions due to creep and shrinkage
#if defined IGNORE_SHRINKAGE_EFFECTS
      Float64 esi = 0;
#else
      Float64 esi = (bIsClosure ? pMaterials->GetClosureJointFreeShrinkageStrain(segmentKey,intervalIdx) : pMaterials->GetSegmentFreeShrinkageStrain(segmentKey,intervalIdx));
#endif 
      tsDetails.Girder.esi = esi;

#if defined IGNORE_SHRINKAGE_EFFECTS
      esi = 0;
#else
      esi = pMaterials->GetDeckFreeShrinkageStrain(segmentKey,intervalIdx);
#endif
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
      tsDetails.Pr[TIMESTEP_PS] = tsDetails.Strands[pgsTypes::Straight ].PrRelaxation
                                + tsDetails.Strands[pgsTypes::Harped   ].PrRelaxation
                                + tsDetails.Strands[pgsTypes::Temporary].PrRelaxation;
#else
      tsDetails.Pr[TIMESTEP_PS] = 0;
#endif // LUMP_STRANDS

      tsDetails.Mr[TIMESTEP_CR] = tsDetails.Girder.MrCreep + tsDetails.Girder.PrCreep*(tsDetails.Ytr - tsDetails.Girder.Yn)
                                + tsDetails.Deck.MrCreep   + tsDetails.Deck.PrCreep  *(tsDetails.Ytr - tsDetails.Deck.Yn);
      
      tsDetails.Mr[TIMESTEP_SH] = tsDetails.Girder.PrShrinkage*(tsDetails.Ytr - tsDetails.Girder.Yn)
                                + tsDetails.Deck.PrShrinkage  *(tsDetails.Ytr - tsDetails.Deck.Yn);
      
#if defined LUMP_STRANDS
      tsDetails.Mr[TIMESTEP_PS] = tsDetails.Strands[pgsTypes::Straight ].PrRelaxation*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Straight ].Ys)
                                + tsDetails.Strands[pgsTypes::Harped   ].PrRelaxation*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Harped   ].Ys)
                                + tsDetails.Strands[pgsTypes::Temporary].PrRelaxation*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Temporary].Ys);
#else
      tsDetails.Mr[TIMESTEP_PS] = 0;
#endif // LUMP_STRANDS

#if !defined LUMP_STRANDS
      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
         StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey,strandType);
         for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
         {
            TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];
            tsDetails.Pr[TIMESTEP_PS] += strand.PrRelaxation;
            tsDetails.Mr[TIMESTEP_PS] += strand.PrRelaxation*(tsDetails.Ytr - strand.Ys);
         } // next strand
      } // next strand type
#endif // LUMP_STRANDS

      for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         TIME_STEP_STRAND& tendon = tsDetails.Tendons[ductIdx];

         tsDetails.Pr[TIMESTEP_PS] += tendon.PrRelaxation;
         tsDetails.Mr[TIMESTEP_PS] += tendon.PrRelaxation*(tsDetails.Ytr - tendon.Ys);
      }
   }

   details.TimeStepDetails.push_back(tsDetails);
}

void CTimeStepLossEngineer::AnalyzeInitialStrains(IntervalIndexType intervalIdx,const CGirderKey& girderKey,LOSSES* pLosses)
{
   // Compute the response to the inital strains (see Tadros 1977, section titled "Effects of Initial Strains")
   GET_IFACE(IMaterials,pMaterials);
   GET_IFACE(IIntervals,pIntervals);

   // Create a load group for the restraining forces in this interval
   GET_IFACE(IExternalLoading,pExtLoading);
   std::_tstring strLoadGroupName[3] = {_T("Creep"),_T("Shrinkage"),_T("Relaxation")};

#pragma Reminder("UPDATE: CreateLoadGroup doesn't work") // using hard coded InitialStrains loads groups created in the analysis agent
//   // for the time being... get this fixed
//   //pExtLoading->CreateLoadGroup(strLoadGroupName[TIMESTEP_CR].c_str());
//   //pExtLoading->CreateLoadGroup(strLoadGroupName[TIMESTEP_SH].c_str());
//   //pExtLoading->CreateLoadGroup(strLoadGroupName[TIMESTEP_PS].c_str());
//   // Using pre-defined product load types pftCreep, pftShrinkage, pftRelaxation

   // At each POI, compute the equivalent restraining loads and apply to the load group
   std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter1(pLosses->SectionLosses.begin());
   std::map<pgsPointOfInterest,LOSSDETAILS>::iterator end(pLosses->SectionLosses.end());
   std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter2(iter1);
   iter2++;

   for ( ; iter2 != end; iter1++, iter2++ )
   {
      const pgsPointOfInterest& poi1(iter1->first);
      const CSegmentKey& segmentKey1(poi1.GetSegmentKey());
      LOSSDETAILS& details1(iter1->second);
      bool bIsClosure1 = poi1.HasAttribute(POI_CLOSURE);
      bool bIsClosureEffective1 = (bIsClosure1 ? pIntervals->GetCompositeClosureJointInterval(segmentKey1) <= intervalIdx : false);
      
      const pgsPointOfInterest& poi2(iter2->first);
      const CSegmentKey& segmentKey2(poi2.GetSegmentKey());
      LOSSDETAILS& details2(iter2->second);
      bool bIsClosure2 = poi2.HasAttribute(POI_CLOSURE);
      bool bIsClosureEffective2 = (bIsClosure2 ? pIntervals->GetCompositeClosureJointInterval(segmentKey2) <= intervalIdx : false);

      TIME_STEP_DETAILS& tsDetails1(details1.TimeStepDetails[intervalIdx]);
      TIME_STEP_DETAILS& tsDetails2(details2.TimeStepDetails[intervalIdx]);

      // Compute initial strain at poi1
      Float64 Atr1 = tsDetails1.Atr;
      Float64 Itr1 = tsDetails1.Itr;
      Float64 E1 = (bIsClosure1 ? pMaterials->GetClosureJointEc(segmentKey1,intervalIdx) : pMaterials->GetSegmentEc(segmentKey1,intervalIdx));
      Float64 EA1 = Atr1*E1;
      Float64 EI1 = Itr1*E1;

      // Compute initial strain at poi2
      Float64 Atr2 = tsDetails2.Atr;
      Float64 Itr2 = tsDetails2.Itr;
      Float64 E2 = (bIsClosure2 ? pMaterials->GetClosureJointEc(segmentKey2,intervalIdx) : pMaterials->GetSegmentEc(segmentKey2,intervalIdx));
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

         pExtLoading->CreateInitialStrainLoad(intervalIdx,poi1,poi2,e,r,strLoadGroupName[i].c_str());
      }
   }

   // Get the interal forces caused by the initial strains
   std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter(pLosses->SectionLosses.begin());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi(iter->first);
      bool bIsClosure = poi.HasAttribute(POI_CLOSURE);
      IntervalIndexType compositeClosureIntervalIdx = (bIsClosure ? pIntervals->GetCompositeClosureJointInterval(poi.GetSegmentKey()) : INVALID_INDEX);

      LOSSDETAILS& details(iter->second);
      TIME_STEP_DETAILS& tsDetails(details.TimeStepDetails[intervalIdx]);

      for ( int i = 0; i < 3; i++ ) // i is one of the TIMESTEP_XXX constants
      {
         Float64 P = pExtLoading->GetAxial( intervalIdx,strLoadGroupName[i].c_str(),poi,pgsTypes::ContinuousSpan);
         Float64 M = pExtLoading->GetMoment(intervalIdx,strLoadGroupName[i].c_str(),poi,pgsTypes::ContinuousSpan);

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

   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IMaterials,pMaterials);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(ITendonGeometry,pTendonGeom);
   GET_IFACE(ICombinedForces,pForces);
   GET_IFACE(IProductForces,pProductForces);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   CGirderKey girderKey(segmentKey);

   bool bIsClosure = poi.HasAttribute(POI_CLOSURE);

   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);

   IntervalIndexType stressStrandsIntervalIdx = pIntervals->GetStressStrandInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx      = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval(segmentKey);

   if ( intervalIdx < releaseIntervalIdx )
   {
      // Prestress force not released onto segment yet...
      if ( !bIsClosure && stressStrandsIntervalIdx <= intervalIdx )
      {
         // strands are stressed, but not released
         for ( int i = 0; i < 3; i++ )
         {
            pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
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
               // Force in strand is force at end of previous interval plus change in force during this interval
               tsDetails.Strands[strandType].P   = details.TimeStepDetails[intervalIdx-1].Strands[strandType].P   + tsDetails.Strands[strandType].dP;

               // Effective prestress is effective prestress at end of previous interval plus change in stress during this interval
               tsDetails.Strands[strandType].fpe = details.TimeStepDetails[intervalIdx-1].Strands[strandType].fpe + tsDetails.Strands[strandType].dFps;
            }
#else
            StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey,strandType);
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
                  // Force in strand is force at end of previous interval plus change in force during this interval
                  strand.P   = details.TimeStepDetails[intervalIdx-1].Strands[strandType][strandIdx].P   + strand.dP;

                  // Effective prestress is effective prestress at end of previous interval plus change in stress during this interval
                  strand.fpe = details.TimeStepDetails[intervalIdx-1].Strands[strandType][strandIdx].fpe + strand.dFps;
               }
            } // next strand idx
#endif // LUMP_STRANDS
         } // next strand type
      }
   }
   else
   {
      // get some material properties that we are going to need for the analysis
      Float64 EaGirder  = (bIsClosure ? 
                            pMaterials->GetClosureJointAgeAdjustedEc(segmentKey,intervalIdx) : 
                            pMaterials->GetSegmentAgeAdjustedEc(segmentKey,intervalIdx));
      Float64 EaDeck     = pMaterials->GetDeckAgeAdjustedEc(segmentKey,intervalIdx);
      Float64 EStrand[3] = { pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Straight)->GetE(),
                             pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Harped)->GetE(),
                             pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetE()};

      Float64 ETendon = pMaterials->GetTendonMaterial(girderKey)->GetE();

      Float64 EDeckRebar, EGirderRebar, Fy, Fu;
      pMaterials->GetDeckRebarProperties(&EDeckRebar,&Fy,&Fu);

      if ( bIsClosure )
      {
         pMaterials->GetClosureJointLongitudinalRebarProperties(segmentKey,&EGirderRebar,&Fy,&Fu);
      }
      else
      {
         pMaterials->GetSegmentLongitudinalRebarProperties(segmentKey,&EGirderRebar,&Fy,&Fu);
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
         for ( int i = 0; i < 3; i++ )
         {
            pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
#if defined LUMP_STRANDS
            EA  += EStrand[strandType]*tsDetails.Strands[strandType].As;
            EAy += EStrand[strandType]*tsDetails.Strands[strandType].As*tsDetails.Strands[strandType].Ys;
#else
            StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey,strandType);
            for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
            {
               TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];
               EA  += EStrand[strandType]*strand.As;
               EAy += EStrand[strandType]*strand.As*strand.Ys;
            }
#endif // LUMP_STRANDS
         }
      }

      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         IntervalIndexType stressTendonIntervalIdx  = pIntervals->GetStressTendonInterval(girderKey,ductIdx);

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
         for ( int i = 0; i < 3; i++ )
         {
            pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
#if defined LUMP_STRANDS
            TIME_STEP_STRAND& strand = tsDetails.Strands[strandType];
            EI += EStrand[strandType]*strand.As*pow((Ytr - strand.Ys),2);
#else
            StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey,strandType);
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
         IntervalIndexType stressTendonIntervalIdx  = pIntervals->GetStressTendonInterval(girderKey,ductIdx);

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
         GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
         CString strMsg;
         strMsg.Format(_T("Ytr (%s,%s)\nAtr (%s,%s)\nItr (%s,%s)\n at POI %d"),
            ::FormatDimension(tsDetails.Ytr,pDisplayUnits->GetComponentDimUnit()),
            ::FormatDimension(Ytr,pDisplayUnits->GetComponentDimUnit()),
            ::FormatDimension(tsDetails.Atr,pDisplayUnits->GetAreaUnit()),
            ::FormatDimension(Atr,pDisplayUnits->GetAreaUnit()),
            ::FormatDimension(tsDetails.Itr,pDisplayUnits->GetMomentOfInertiaUnit()),
            ::FormatDimension(Itr,pDisplayUnits->GetMomentOfInertiaUnit()),
            poi.GetID());

         AfxMessageBox(strMsg);
      }
#endif // !_DEBUG
#endif // _BETA_VERSION

      IntervalIndexType compositeClosureIntervalIdx = (bIsClosure ? pIntervals->GetCompositeClosureJointInterval(segmentKey) : INVALID_INDEX);

      // Get Live Load Moment
      Float64 MllMin, MllMax;
      pForces->GetCombinedLiveLoadMoment(pgsTypes::lltDesign,intervalIdx,poi,pgsTypes::ContinuousSpan,&MllMin,&MllMax);

      //
      // Compute total change of force on the section due to externally applied loads during this interval
      //
      std::vector<ProductForceType> vProductForces = GetApplicableProductLoads(intervalIdx,poi,true/*externally applied loads only*/);
      std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
      std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
      for ( ; pfIter != pfIterEnd; pfIter++ )
      {
         ProductForceType pfType = *pfIter;

         // Get change in moment in this interval
         Float64 dM = pProductForces->GetMoment(intervalIdx,pfType,poi,pgsTypes::ContinuousSpan, ctIncremental);

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

         if ( intervalIdx == storageIntervalIdx )
         {
            // We don't want the moment while the girder segment is in storage. We want the change in moment
            // between release and storage. No new load was added, the boundary conditions changed causing a
            // change in moment.
            Float64 Mrelease = pProductForces->GetMoment(releaseIntervalIdx,pfType,poi,pgsTypes::ContinuousSpan, ctIncremental);
            dM -= Mrelease;
         }
         else if ( intervalIdx == erectionIntervalIdx )
         {
            // When the segment is erected we want the difference between the moment during storage and erection.
            // Again, the change in moment is due to a change in boundary conditions
            Float64 Mstorage = pProductForces->GetMoment(storageIntervalIdx,pfType,poi,pgsTypes::ContinuousSpan, ctIncremental);
            dM -= Mstorage;
         }

         tsDetails.dM[pfType] += dM;
         //tsDetails.dP[pfType] += 0; // no axial loads are modeled so sum is always zero
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
         tsDetails.dP[pftRelaxation] += -(tsDetails.Pre[TIMESTEP_PS] + tsDetails.Pr[TIMESTEP_PS]);
      }

      if ( !IsZero(EaGirder_Itr) )
      {
         tsDetails.dM[pftCreep]      += -(tsDetails.Mre[TIMESTEP_CR] + tsDetails.Mr[TIMESTEP_CR]);
         tsDetails.dM[pftShrinkage]  += -(tsDetails.Mre[TIMESTEP_SH] + tsDetails.Mr[TIMESTEP_SH]);
         tsDetails.dM[pftRelaxation] += -(tsDetails.Mre[TIMESTEP_PS] + tsDetails.Mr[TIMESTEP_PS]);
      }


      //
      // total change in axial and moment in this interval
      //

      // also going to compute the total force on each piece of the section for all intervals that have occured thus far
      // force at the end of this interval = force at end of previous interval + change in force during this interval
      // initialize the force at the end of this interval with the force at the end of the last interval
      // then will add in the change in force during this interval in the loop below
      tsDetails.Girder.P = details.TimeStepDetails[intervalIdx-1].Girder.P;
      tsDetails.Girder.M = details.TimeStepDetails[intervalIdx-1].Girder.M;
      tsDetails.Deck.P   = details.TimeStepDetails[intervalIdx-1].Deck.P;
      tsDetails.Deck.M   = details.TimeStepDetails[intervalIdx-1].Deck.M;

      Float64 dP = 0;
      Float64 dM = 0;
      for ( int i = 0; i < 19; i++ )
      {
         ProductForceType pfType = (ProductForceType)(i);

         dP += tsDetails.dP[pfType];
         dM += tsDetails.dM[pfType];

         // Deformation of the composite section this interval
         tsDetails.der[pfType] = (IsZero(EaGirder_Atr) ? 0 : tsDetails.dP[pfType]/EaGirder_Atr);
         tsDetails.drr[pfType] = (IsZero(EaGirder_Itr) ? 0 : tsDetails.dM[pfType]/EaGirder_Itr);

         // Total deformation of the composite section
         tsDetails.er[pfType] = details.TimeStepDetails[intervalIdx-1].er[pfType] + tsDetails.der[pfType];
         tsDetails.rr[pfType] = details.TimeStepDetails[intervalIdx-1].rr[pfType] + tsDetails.drr[pfType];

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

         tsDetails.Girder.P += tsDetails.Girder.dP[pfType];
         tsDetails.Girder.M += tsDetails.Girder.dM[pfType];

         // Compute girder stresses at end of interval
         // f = f end of previous interval + change in stress this interval
         if ( !IsZero(tsDetails.Girder.An) && !IsZero(tsDetails.Girder.In) )
         {
            tsDetails.Girder.f[pgsTypes::TopFace][pfType][ctIncremental] = tsDetails.Girder.dP[pfType]/tsDetails.Girder.An + tsDetails.Girder.dM[pfType]*tsDetails.Girder.Yn/tsDetails.Girder.In;
            tsDetails.Girder.f[pgsTypes::TopFace][pfType][ctCumulative] = details.TimeStepDetails[intervalIdx-1].Girder.f[pgsTypes::TopFace][pfType][ctCumulative] + tsDetails.Girder.f[pgsTypes::TopFace][pfType][ctIncremental];
            
            tsDetails.Girder.f[pgsTypes::BottomFace][pfType][ctIncremental] = tsDetails.Girder.dP[pfType]/tsDetails.Girder.An + tsDetails.Girder.dM[pfType]*(tsDetails.Girder.H + tsDetails.Girder.Yn)/tsDetails.Girder.In;
            tsDetails.Girder.f[pgsTypes::BottomFace][pfType][ctCumulative] = details.TimeStepDetails[intervalIdx-1].Girder.f[pgsTypes::BottomFace][pfType][ctCumulative] + tsDetails.Girder.f[pgsTypes::BottomFace][pfType][ctIncremental];
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

         tsDetails.Deck.P += tsDetails.Deck.dP[pfType];
         tsDetails.Deck.M += tsDetails.Deck.dM[pfType];

         // Compute deck stresses at end of interval
         // f = f end of previous interval + change in stress this interval
         if ( compositeDeckIntervalIdx <= intervalIdx && 
             !IsZero(tsDetails.Deck.An) && !IsZero(tsDetails.Deck.In) )
         {
            tsDetails.Deck.f[pgsTypes::TopFace][pfType][ctIncremental] = tsDetails.Deck.dP[pfType]/tsDetails.Deck.An + tsDetails.Deck.dM[pfType]*(tsDetails.Deck.Yn - tsDetails.Deck.H)/tsDetails.Deck.In;
            tsDetails.Deck.f[pgsTypes::TopFace][pfType][ctCumulative] = details.TimeStepDetails[intervalIdx-1].Deck.f[pgsTypes::TopFace][pfType][ctCumulative] + tsDetails.Deck.f[pgsTypes::TopFace][pfType][ctIncremental];

            tsDetails.Deck.f[pgsTypes::BottomFace][pfType][ctIncremental] = tsDetails.Deck.dP[pfType]/tsDetails.Deck.An + tsDetails.Deck.dM[pfType]*tsDetails.Deck.Yn/tsDetails.Deck.In;
            tsDetails.Deck.f[pgsTypes::BottomFace][pfType][ctCumulative] = details.TimeStepDetails[intervalIdx-1].Deck.f[pgsTypes::BottomFace][pfType][ctCumulative] + tsDetails.Deck.f[pgsTypes::BottomFace][pfType][ctIncremental];
         }
      } // next product force

      // Compute stresses in girder due to live load
      // Axial force in girder
      Float64 Pmin = EaGirder_An*rLLMin*(tsDetails.Ytr - tsDetails.Girder.Yn);
      Float64 Pmax = EaGirder_An*rLLMax*(tsDetails.Ytr - tsDetails.Girder.Yn);

      // Moment in girder
      Float64 Mmin = EaGirder_In*rLLMin;
      Float64 Mmax = EaGirder_In*rLLMax;

      tsDetails.Girder.fLLMin[pgsTypes::TopFace] = Pmin/tsDetails.Girder.An + Mmin*tsDetails.Girder.Yn/tsDetails.Girder.In;
      tsDetails.Girder.fLLMax[pgsTypes::TopFace] = Pmax/tsDetails.Girder.An + Mmax*tsDetails.Girder.Yn/tsDetails.Girder.In;

      tsDetails.Girder.fLLMin[pgsTypes::BottomFace] = Pmin/tsDetails.Girder.An + Mmin*(tsDetails.Girder.H + tsDetails.Girder.Yn)/tsDetails.Girder.In;
      tsDetails.Girder.fLLMax[pgsTypes::BottomFace] = Pmax/tsDetails.Girder.An + Mmax*(tsDetails.Girder.H + tsDetails.Girder.Yn)/tsDetails.Girder.In;

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

         tsDetails.Deck.fLLMin[pgsTypes::BottomFace] = Pmin/tsDetails.Deck.An + Mmin*tsDetails.Deck.Yn/tsDetails.Deck.In;
         tsDetails.Deck.fLLMax[pgsTypes::BottomFace] = Pmax/tsDetails.Deck.An + Mmax*tsDetails.Deck.Yn/tsDetails.Deck.In;
      }

      // Compute change in force in deck rebar
      tsDetails.DeckRebar[pgsTypes::drmTop   ].dP  = IsZero(EaGirder_Atr) ? 0 : dP*EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmTop   ].As/EaGirder_Atr;
      tsDetails.DeckRebar[pgsTypes::drmTop   ].dP += IsZero(EaGirder_Itr) ? 0 : dM*EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmTop   ].As*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmTop   ].Ys)/EaGirder_Itr;
      tsDetails.DeckRebar[pgsTypes::drmBottom].dP  = IsZero(EaGirder_Atr) ? 0 : dP*EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmBottom].As/EaGirder_Atr;
      tsDetails.DeckRebar[pgsTypes::drmBottom].dP += IsZero(EaGirder_Itr) ? 0 : dM*EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmBottom].As*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmBottom].Ys)/EaGirder_Itr;

      tsDetails.DeckRebar[pgsTypes::drmTop   ].P = details.TimeStepDetails[intervalIdx-1].DeckRebar[pgsTypes::drmTop   ].P + tsDetails.DeckRebar[pgsTypes::drmTop   ].dP;
      tsDetails.DeckRebar[pgsTypes::drmBottom].P = details.TimeStepDetails[intervalIdx-1].DeckRebar[pgsTypes::drmBottom].P + tsDetails.DeckRebar[pgsTypes::drmBottom].dP;

      // Compute change in force in girder Rebar
      IndexType rebarIdx = 0;
      std::vector<TIME_STEP_REBAR>::iterator rebarIter(tsDetails.GirderRebar.begin());
      std::vector<TIME_STEP_REBAR>::iterator rebarIterEnd(tsDetails.GirderRebar.end());
      for ( ; rebarIter != rebarIterEnd; rebarIter++, rebarIdx++ )
      {
         TIME_STEP_REBAR& tsRebar(*rebarIter);
         tsRebar.dP  = IsZero(EaGirder_Atr) ? 0 : dP*EGirderRebar*tsRebar.As/EaGirder_Atr;
         tsRebar.dP += IsZero(EaGirder_Itr) ? 0 : dM*EGirderRebar*tsRebar.As*(tsDetails.Ytr - tsRebar.Ys)/EaGirder_Itr;

         tsRebar.P = details.TimeStepDetails[intervalIdx-1].GirderRebar[rebarIdx].P + tsRebar.dP;
      }

      // Compute change in force in strands
      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
#if defined LUMP_STRANDS
         // change in strand force
         tsDetails.Strands[strandType].dP += IsZero(EaGirder_Atr) ? 0 : dP*EStrand[strandType]*tsDetails.Strands[strandType].As/EaGirder_Atr;
         tsDetails.Strands[strandType].dP += IsZero(EaGirder_Itr) ? 0 : dM*EStrand[strandType]*tsDetails.Strands[strandType].As*(tsDetails.Ytr - tsDetails.Strands[strandType].Ys)/EaGirder_Itr;
         tsDetails.Strands[strandType].dP += tsDetails.Strands[strandType].PrRelaxation;
         tsDetails.Strands[strandType].P  = details.TimeStepDetails[intervalIdx-1].Strands[strandType].P + tsDetails.Strands[strandType].dP;

         // Losses and effective prestress
         tsDetails.Strands[strandType].dFps = IsZero(tsDetails.Strands[strandType].As) ? 0 : tsDetails.Strands[strandType].dP/tsDetails.Strands[strandType].As;
         tsDetails.Strands[strandType].fpe  = details.TimeStepDetails[intervalIdx-1].Strands[strandType].fpe  + tsDetails.Strands[strandType].dFps;
         tsDetails.Strands[strandType].loss = details.TimeStepDetails[intervalIdx-1].Strands[strandType].loss - tsDetails.Strands[strandType].dFps;

         // check total loss = fpj - fpe
         ATLASSERT(IsEqual(tsDetails.Strands[strandType].loss,details.TimeStepDetails[stressStrandsIntervalIdx].Strands[strandType].fpj - tsDetails.Strands[strandType].fpe));

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
         StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey,strandType);
         for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
         {
            TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];

            // change in strand force
            strand.dP += IsZero(EaGirder_Atr) ? 0 : dP*EStrand[strandType]*strand.As/EaGirder_Atr;
            strand.dP += IsZero(EaGirder_Itr) ? 0 : dM*EStrand[strandType]*strand.As*(tsDetails.Ytr - strand.Ys)/EaGirder_Itr;
            strand.dP += strand.PrRelaxation;
            strand.P  = details.TimeStepDetails[intervalIdx-1].Strands[strandType][strandIdx].P + strand.dP;

            // Losses and effective prestress
            strand.dFps = IsZero(strand.As) ? 0 : strand.dP/strand.As;
            strand.fpe  = details.TimeStepDetails[intervalIdx-1].Strands[strandType][strandIdx].fpe  + strand.dFps;
            strand.loss = details.TimeStepDetails[intervalIdx-1].Strands[strandType][strandIdx].loss - strand.dFps;

            // check total loss = fpj - fpe
            ATLASSERT(IsEqual(strand.loss,details.TimeStepDetails[stressStrandsIntervalIdx].Strands[strandType][strandIdx].fpj - strand.fpe));

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

      // Compute change in force in tendons
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         TIME_STEP_STRAND& tendon = tsDetails.Tendons[ductIdx];

         IntervalIndexType stressTendonIntervalIdx  = pIntervals->GetStressTendonInterval(girderKey,ductIdx);

         // Change in tendon force due to deformations during this interval
         if ( intervalIdx <= stressTendonIntervalIdx )
         {
            tendon.dP = 0; // tendon not installed or grouted yet
            tendon.P  = 0;
         }
         else
         {
            tendon.dP += IsZero(EaGirder_Atr) ? 0 : dP*ETendon*tendon.As/EaGirder_Atr;
            tendon.dP += IsZero(EaGirder_Itr) ? 0 : dM*ETendon*tendon.As*(tsDetails.Ytr - tendon.Ys)/EaGirder_Itr;
            tendon.dP += tendon.PrRelaxation;
            tendon.P  = details.TimeStepDetails[intervalIdx-1].Tendons[ductIdx].P + tendon.dP;
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
            tendon.fpe  = details.TimeStepDetails[intervalIdx-1].Tendons[ductIdx].fpe  + tendon.dFps;
            tendon.loss = details.TimeStepDetails[intervalIdx-1].Tendons[ductIdx].loss - tendon.dFps;
         }
         ATLASSERT(IsEqual(tendon.loss,tendon.fpj - tendon.fpe));

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
      }
   }

   //
   // Equilibrium Checks
   //

#pragma Reminder("UPDATE: make the equilibrium checks for debug builds only")
   // this will require updating the Time Step Parameters Chapter Builder

   // Check : Change in External Forces = Change in Internal Forces
   tsDetails.dPext = 0;
   tsDetails.dMext = 0;
   std::vector<ProductForceType> vProductForces = GetApplicableProductLoads(intervalIdx,poi,true/*externally applied loads only*/);
   vProductForces.push_back(pftPrimaryPostTensioning);
   vProductForces.push_back(pftPrestress);
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
                    + tsDetails.Pre[TIMESTEP_PS];

   tsDetails.dMint += tsDetails.DeckRebar[pgsTypes::drmTop   ].dP*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmTop   ].Ys)
                    + tsDetails.DeckRebar[pgsTypes::drmBottom].dP*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmBottom].Ys)
                    + tsDetails.Mre[TIMESTEP_CR]
                    + tsDetails.Mre[TIMESTEP_SH]
                    + tsDetails.Mre[TIMESTEP_PS];


   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
#if defined LUMP_STRANDS
      TIME_STEP_STRAND& strand = tsDetails.Strands[strandType];
      tsDetails.dPint += strand.dP;
      tsDetails.dMint += strand.dP*(tsDetails.Ytr - strand.Ys);
#else
      StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey,strandType);
      for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
      {
         TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];

         tsDetails.dPint += strand.dP + strand.PrRelaxation;
         tsDetails.dMint += strand.dP*(tsDetails.Ytr - strand.Ys);
      } // next strand
#endif // LUMP_STRANDS
   } // next strand type

   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      TIME_STEP_STRAND& tendon = tsDetails.Tendons[ductIdx];
      tsDetails.dPint += tendon.dP;
      tsDetails.dMint += tendon.dP*(tsDetails.Ytr - tendon.Ys);
   }

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
         GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
         CString strMsg;
         strMsg.Format(_T("dPext != dPint (%s != %s) at POI %d"),
            ::FormatDimension(tsDetails.dPext,pDisplayUnits->GetGeneralForceUnit()),
            ::FormatDimension(tsDetails.dPint,pDisplayUnits->GetGeneralForceUnit()),
            poi.GetID());

         AfxMessageBox(strMsg);
      }

      if ( !IsEqual(tsDetails.dMext,tsDetails.dMint,500.0) )
      {
         GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
         CString strMsg;
         strMsg.Format(_T("dMext != dMint (%s != %s) at POI %d"),
            ::FormatDimension(tsDetails.dMext,pDisplayUnits->GetMomentUnit()),
            ::FormatDimension(tsDetails.dMint,pDisplayUnits->GetMomentUnit()),
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
      tsDetails.Pext = details.TimeStepDetails[intervalIdx-1].Pext + tsDetails.dPext;
      tsDetails.Mext = details.TimeStepDetails[intervalIdx-1].Mext + tsDetails.dMext;
      tsDetails.Pint = details.TimeStepDetails[intervalIdx-1].Pint + tsDetails.dPint - details.TimeStepDetails[stressStrandsIntervalIdx].dPint;
      tsDetails.Mint = details.TimeStepDetails[intervalIdx-1].Mint + tsDetails.dMint - details.TimeStepDetails[stressStrandsIntervalIdx].dMint;
   }
   else
   {
      // force at end of this intervale = force at end of previous interval plus change in force during this interval
      tsDetails.Pext = details.TimeStepDetails[intervalIdx-1].Pext + tsDetails.dPext;
      tsDetails.Mext = details.TimeStepDetails[intervalIdx-1].Mext + tsDetails.dMext;
      tsDetails.Pint = details.TimeStepDetails[intervalIdx-1].Pint + tsDetails.dPint;
      tsDetails.Mint = details.TimeStepDetails[intervalIdx-1].Mint + tsDetails.dMint;
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
         GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
         CString strMsg;
         strMsg.Format(_T("Pext != Pint (%s != %s) at POI %d"),
            ::FormatDimension(tsDetails.Pext,pDisplayUnits->GetGeneralForceUnit()),
            ::FormatDimension(tsDetails.Pint,pDisplayUnits->GetGeneralForceUnit()),
            poi.GetID());

         AfxMessageBox(strMsg);
      }

      if ( !IsEqual(tsDetails.Mext,tsDetails.Mint,500.0) )
      {
         GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
         CString strMsg;
         strMsg.Format(_T("Mext != Mint (%s != %s) at POI %d"),
            ::FormatDimension(tsDetails.Mext,pDisplayUnits->GetMomentUnit()),
            ::FormatDimension(tsDetails.Mint,pDisplayUnits->GetMomentUnit()),
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
   // See Structural Analysis, 4th Edition, Jack C. McCormac, pg365. Section 18.5, Application of Virtual Work to Beams and Frames

   if ( intervalIdx == 0 )
      return;

   LOSSDETAILS* pDetails = GetLossDetails(pLosses,poi);
   TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   CGirderKey girderKey(segmentKey);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<pgsPointOfInterest> vAllPoi(pPoi->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)));

   // Only do the virtual work deflection calculations for product forces that cause deflections
   // in this interval. (if the load doesn't cause deflection it would be a lot of work to
   // get an answer of zero)
   bool bIsErectionInterval = pIntervals->IsSegmentErectionInterval(segmentKey,intervalIdx);
   bool bIsStorageInterval  = pIntervals->GetStorageInterval(segmentKey) == intervalIdx ? true : false;
   std::vector<ProductForceType> vProductForces(GetApplicableProductLoads(intervalIdx,poi));
   if ( 0 < vProductForces.size() )
   {
      // Since we are going to compute some deflections, get the moment diagram for a unit load at the poi 
      // that is being analyzed.
      GET_IFACE(IInfluenceResults,pInfluenceResults);
      if ( bIsErectionInterval || bIsStorageInterval )
      {
         // Support locations of the erected segment are in a different location then when the segment was in storage.
         // The incremental deflection calculations need to take this into account.
         //
         // This version of ComputeIncrementalDeflections accounts for the change in support locations however
         // it is slower than the other version because more calculations are required. This is why we only
         // want to use it for intervals when a segment is erected.
         std::vector<Float64> unitLoadMomentsPreviousInterval = pInfluenceResults->GetUnitLoadMoment(vAllPoi,poi,pgsTypes::ContinuousSpan,intervalIdx-1);
         std::vector<Float64> unitLoadMomentsThisInterval     = pInfluenceResults->GetUnitLoadMoment(vAllPoi,poi,pgsTypes::ContinuousSpan,intervalIdx);
         ATLASSERT(unitLoadMomentsPreviousInterval.size() == vAllPoi.size());
         ATLASSERT(unitLoadMomentsThisInterval.size() == vAllPoi.size());

         std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
         std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
         for ( ; pfIter != pfIterEnd; pfIter++ )
         {
            ProductForceType pfType = *pfIter;
            ComputeIncrementalDeflections(intervalIdx,poi,pfType,vAllPoi,unitLoadMomentsPreviousInterval,unitLoadMomentsThisInterval,pLosses);
         }
      }
      else
      {
         // this version of ComputeIncrementalDeflections doesn't account for changing support locations
         std::vector<Float64> unitLoadMoments = pInfluenceResults->GetUnitLoadMoment(vAllPoi,poi,pgsTypes::ContinuousSpan,intervalIdx);
         ATLASSERT(unitLoadMoments.size() == vAllPoi.size());

         std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
         std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
         for ( ; pfIter != pfIterEnd; pfIter++ )
         {
            ProductForceType pfType = *pfIter;
            ComputeIncrementalDeflections(intervalIdx,poi,pfType,vAllPoi,unitLoadMoments,pLosses);
         }
      }
   }

   InitializeErectionAdjustments(intervalIdx,segmentKey,pLosses);

   for ( int i = 0; i < 19; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;

      Float64 adjustment = GetErectionAdjustment(intervalIdx,poi,pfType);
      tsDetails.dD[pfType] += adjustment;

      // compute total deflection in this interval
      // total deflection this interval is deflection at end of last interval plus the change in this interval
      tsDetails.D[pfType] += pDetails->TimeStepDetails[intervalIdx-1].D[pfType] + tsDetails.dD[pfType];

      tsDetails.dY += tsDetails.dD[pfType];
      tsDetails.Y  += tsDetails.D[pfType];
   }
}

void CTimeStepLossEngineer::ComputeIncrementalDeflections(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vAllPoi,const std::vector<Float64>& unitLoadMomentsPreviousInterval,const std::vector<Float64>& unitLoadMomentsThisInterval,LOSSES* pLosses)
{
   // This version accounts for changes in support locations between the previous interval and this interval
   GET_IFACE(IPointOfInterest,pPoi);

   LOSSDETAILS* pDetails = GetLossDetails(pLosses,poi);
   TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

   ATLASSERT(unitLoadMomentsPreviousInterval.size() == vAllPoi.size());
   ATLASSERT(unitLoadMomentsThisInterval.size() == vAllPoi.size());

   std::vector<Float64>::const_iterator prevIntervalMomentIter(unitLoadMomentsPreviousInterval.begin());
   std::vector<Float64>::const_iterator prevIntervalMomentIterEnd(unitLoadMomentsPreviousInterval.end());
   std::vector<Float64>::const_iterator thisIntervalMomentIter(unitLoadMomentsThisInterval.begin());
   std::vector<Float64>::const_iterator thisIntervalMomentIterEnd(unitLoadMomentsThisInterval.end());
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vAllPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vAllPoi.end());

   // "Subscripts"
   // i = previous interval
   // j = this interval
   // 1 = section 1
   // 2 = section 2

   // Initialize values for first time through the loop
   Float64 delta = 0; // change in deflection during this interval
   Float64 mi1 = *prevIntervalMomentIter; // unit load moment in the previous interval
   Float64 mj1 = *thisIntervalMomentIter; // unit load moment in this interval

   pgsPointOfInterest poi1(*poiIter); // can't be a reference because of the assignment in the loop below
   Float64 x1 = pPoi->ConvertPoiToGirderCoordinate(poi1);
   const LOSSDETAILS* pDetails1 = GetLossDetails(pLosses,poi1);
   ATLASSERT(pDetails1->POI == poi1);

   // advance the iterators so when we start the loop
   // the iterators are at the second location
   prevIntervalMomentIter++;
   thisIntervalMomentIter++;
   poiIter++;

   for ( ; poiIter != poiIterEnd; poiIter++, prevIntervalMomentIter++, thisIntervalMomentIter++ )
   {
      Float64 mi2 = *prevIntervalMomentIter;
      Float64 mj2 = *thisIntervalMomentIter;

      const pgsPointOfInterest& poi2(*poiIter);

      Float64 x2 = pPoi->ConvertPoiToGirderCoordinate(poi2);

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
      // we only have to divde by 2 once instead of every time through the loop
      delta += (x2-x1)*((dc1*mj1 + dc2*mj2) + (ci1*(mj1-mi1) + ci2*(mj2-mi2)));

      // get ready for next time through the loop
      mi1       = mi2;
      mj1       = mj2;
      poi1      = poi2;
      x1        = x2;
      pDetails1 = pDetails2;
   }

   delta /= 2.0;

   tsDetails.dD[pfType] = delta;
}

void CTimeStepLossEngineer::ComputeIncrementalDeflections(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vAllPoi,const std::vector<Float64>& unitLoadMoments,LOSSES* pLosses)
{
   // this version assumes the support locations don't change from the previous interval to this interval
   GET_IFACE(IPointOfInterest,pPoi);

   LOSSDETAILS* pDetails = GetLossDetails(pLosses,poi);
   TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

   ATLASSERT(unitLoadMoments.size() == vAllPoi.size());

   std::vector<Float64>::const_iterator momentIter(unitLoadMoments.begin());
   std::vector<Float64>::const_iterator momentIterEnd(unitLoadMoments.end());
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vAllPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vAllPoi.end());

   // "Subscripts"
   // 1 = section 1
   // 2 = section 2

   // Initialize values for first time through the loop
   Float64 delta = 0; // change in deflection during this interval
   Float64 m1 = *momentIter; // unit load moment

   pgsPointOfInterest poi1(*poiIter); // can't be a reference because of the assignment in the loop below
   Float64 x1 = pPoi->ConvertPoiToGirderCoordinate(poi1);
   const LOSSDETAILS* pDetails1 = GetLossDetails(pLosses,poi1);
   ATLASSERT(pDetails1->POI == poi1);

   // advance the iterators so when we start the loop
   // the iterators are at the second location
   momentIter++;
   poiIter++;

   for ( ; poiIter != poiIterEnd; poiIter++, momentIter++ )
   {
      Float64 m2 = *momentIter;

      const pgsPointOfInterest& poi2(*poiIter);

      Float64 x2 = pPoi->ConvertPoiToGirderCoordinate(poi2);

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
      delta += (x2-x1)*(dc1*m1 + dc2*m2);

      // get ready for next time through the loop
      m1        = m2;
      poi1      = poi2;
      x1        = x2;
      pDetails1 = pDetails2;
   }

   delta /= 2.0;

   tsDetails.dD[pfType] = delta;
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

   GET_IFACE(IPointOfInterest,pPoi);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      std::_tostringstream os;
      if ( 1 < nSegments )
         os << _T("Segment ") << LABEL_SEGMENT(segIdx) << _T(" - Losses in Pretensioned Strands");
      else
         os << _T("Losses in Pretensioned Strands");

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
      std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(CSegmentKey(girderKey,segIdx)));
      std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
      std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
      for ( ; iter != end; iter++, row++ )
      {
         col = 0;
         const pgsPointOfInterest& poi(*iter);
         const LOSSDETAILS* pLossDetails = GetLosses(poi);

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

   std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)));
   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++, row++ )
   {
      const pgsPointOfInterest& poi(*iter);
      const LOSSDETAILS* pLossDetails = GetLosses(poi);

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

void CTimeStepLossEngineer::ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
#pragma Reminder("UPDATE: implement - time step loss reporting")
   ATLASSERT(false);
   //m_Engineer.ReportFinalLosses(m_BeamType,segmentKey,pChapter,pDisplayUnits);
}


void CTimeStepLossEngineer::ComputeAnchorSetLosses(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,LOSSES* pLosses,Float64 Lg,std::map<pgsPointOfInterest,LOSSDETAILS>::iterator& frMinIter,Float64* pdfpAT,Float64* pdfpS,Float64* pXset)
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

   GET_IFACE(ILossParameters,pLossParams);
   Float64 Dset, wobble, friction;
   pLossParams->GetTendonPostTensionParameters(&Dset,&wobble,&friction);

   const CGirderKey& girderKey(pPTData->GetGirder()->GetGirderKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressTendonInterval(girderKey,ductIdx);

   GET_IFACE(ITendonGeometry,pTendonGeom);
   Float64 Apt = pTendonGeom->GetTendonArea(girderKey,stressTendonIntervalIdx,ductIdx);
   Float64 Pj  = (Apt == 0 ? 0 : pTendonGeom->GetPjack(girderKey,ductIdx));
   Float64 fpj = (Apt == 0 ? 0 : Pj/Apt);

   Float64 XsetMin, XsetMax; // position of end of anchor set zone measured from left end of girder
   Float64 DsetMin, DsetMax;
   Float64 dfpATMin, dfpATMax;
   Float64 dfpSMin, dfpSMax;
   BoundAnchorSet(pPTData,pDuctData,ductIdx,endType,Dset,pLosses,fpj,Lg,frMinIter,&XsetMin,&DsetMin,&dfpATMin,&dfpSMin,&XsetMax,&DsetMax,&dfpATMax,&dfpSMax);

   // If the solution nailed, get the heck outta here
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
         break;

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

void CTimeStepLossEngineer::BoundAnchorSet(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,Float64 Dset,LOSSES* pLosses,Float64 fpj,Float64 Lg,std::map<pgsPointOfInterest,LOSSDETAILS>::iterator& frMinIter,Float64* pXsetMin,Float64* pDsetMin,Float64* pdfpATMin,Float64* pdfpSMin,Float64* pXsetMax,Float64* pDsetMax,Float64* pdfpATMax,Float64* pdfpSMax)
{
   const CSplicedGirderData* pGirder = pPTData->GetGirder();
   const CGirderGroupData*   pGroup  = pGirder->GetGirderGroup();
   const CPierData2*         pPier   = pGroup->GetPier(endType);

   SpanIndexType   spanIdx = (endType == pgsTypes::metStart ? pPier->GetNextSpan()->GetIndex() : pPier->GetPrevSpan()->GetIndex());
   GirderIndexType gdrIdx  = pGirder->GetIndex();

   GET_IFACE(IBridge,pBridge);
   Float64 span_length = pBridge->GetSpanLength(spanIdx,gdrIdx);

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

Float64 CTimeStepLossEngineer::EvaluateAnchorSet(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,LOSSES* pLosses,Float64 fpj,Float64 Lg,std::map<pgsPointOfInterest,LOSSDETAILS>::iterator& frMinIter,Float64 Xset,Float64* pdfpAT,Float64* pdfpS)
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
      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter(pLosses->SectionLosses.begin());
      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator end(pLosses->SectionLosses.end());
      if ( minFrDetails.X < Xset )
      {
         // anchor set exceeds the length of the tendon
         Float64 X1 = minFrDetails.X;
         Float64 f1 = minFrDetails.dfpF;
         frMinIter--; // back up one location
         Float64 X2 = frMinIter->second.FrictionLossDetails[ductIdx].X;
         Float64 f2 = frMinIter->second.FrictionLossDetails[ductIdx].dfpF;
         frMinIter++; // go back to where it was

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
      std::map<pgsPointOfInterest,LOSSDETAILS>::reverse_iterator rIter(pLosses->SectionLosses.rbegin());
      std::map<pgsPointOfInterest,LOSSDETAILS>::reverse_iterator rIterEnd(pLosses->SectionLosses.rend());
      if ( Xset < minFrDetails.X )
      {
         // anchor set exceeds the length of the tendon
         Float64 X1 = minFrDetails.X;
         Float64 f1 = minFrDetails.dfpF;
         frMinIter++;
         Float64 X2 = frMinIter->second.FrictionLossDetails[ductIdx].X;
         Float64 f2 = frMinIter->second.FrictionLossDetails[ductIdx].dfpF;
         frMinIter--;

         dfpF_Xset = ::LinInterp(Xset-X1,f1,f2,X2-X1);
         dfpF = f1;

         rIterEnd = std::map<pgsPointOfInterest,LOSSDETAILS>::reverse_iterator(frMinIter);
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
      *pdfpS = 0;
   else
      *pdfpS  = 2*(dfpF_Xset - dfpF);

   Float64 Ept = pPTData->pStrand->GetE();
   Dset /= Ept;
   return Dset;
}

LOSSDETAILS* CTimeStepLossEngineer::GetLossDetails(LOSSES* pLosses,const pgsPointOfInterest& poi)
{
   std::map<pgsPointOfInterest,LOSSDETAILS>::iterator found = pLosses->SectionLosses.find(poi);
   ATLASSERT( found != pLosses->SectionLosses.end() );
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

   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IProductLoads,pProductLoads);

   std::vector<IntervalIndexType> vTSRemovalIntervals = pIntervals->GetTemporarySupportRemovalIntervals(segmentKey);
   bool bIsTempSupportRemovalInterval = false;
   if ( 0 < vTSRemovalIntervals.size() )
   {
      std::vector<IntervalIndexType>::iterator found = std::find(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),intervalIdx);
      bIsTempSupportRemovalInterval = (found == vTSRemovalIntervals.end() ? false : true);
   }

   bool bIsClosure = poi.HasAttribute(POI_CLOSURE) ? true : false;
   IntervalIndexType compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(segmentKey);

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
         IntervalIndexType lastSegmentErectionIntervalIdx = pIntervals->GetLastSegmentErectionInterval(segmentKey);
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
      IntervalIndexType releasePrestressIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

      // girder load comes into play if there are segments erected after this segment is erected
      // consider the case of this segment being a cantilevered pier segment and during this interval
      // a drop in segment is erected and that new segment is supported by the cantilever end of this
      // segment. The dead load reaction of the drop-in is a point load on the cantilver.
      IntervalIndexType lastSegmentErectionIntervalIdx = pIntervals->GetLastSegmentErectionInterval(segmentKey);
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
         vProductForces.push_back(pftPrestress);
      }
   }

   // Force effects due to dead loads that are applied along with the slab self-weight occur
   // at the deck casting interval and any interval when a temporary support is removed after
   // the slab (and related) dead load is applied
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(segmentKey);
   bool bTSRemovedAfterDeckCasting = (0 < std::count_if(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),std::bind2nd(std::greater<IntervalIndexType>(),castDeckIntervalIdx)));
   if ( intervalIdx == castDeckIntervalIdx || (bIsTempSupportRemovalInterval && bTSRemovedAfterDeckCasting) )
   {
      vProductForces.push_back(pftDiaphragm); // verify this... are there cases when we don't apply a diaphragm load?

      GET_IFACE(IBridge,pBridge);
      pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
      if ( deckType != pgsTypes::sdtNone )
      {
         vProductForces.push_back(pftSlab);
         vProductForces.push_back(pftSlabPad);
      }

      if ( deckType == pgsTypes::sdtCompositeSIP )
      {
         vProductForces.push_back(pftSlabPanel);
      }

      std::vector<ConstructionLoad> vConstructionLoads;
      pProductLoads->GetConstructionLoad(segmentKey,&vConstructionLoads);
      if ( 0 < vConstructionLoads.size() )
      {
         vProductForces.push_back(pftConstruction);
      }

      if ( pProductLoads->HasShearKeyLoad(segmentKey) )
      {
         vProductForces.push_back(pftShearKey);
      }
   }

   IntervalIndexType installOverlayIntervalIdx = pIntervals->GetOverlayInterval(segmentKey);
   bool bTSRemovedAfterOverlayInstallation = (0 < std::count_if(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),std::bind2nd(std::greater<IntervalIndexType>(),installOverlayIntervalIdx)));
   if ( intervalIdx == installOverlayIntervalIdx || 
       (bIsTempSupportRemovalInterval && bTSRemovedAfterOverlayInstallation)
      )
   {
      vProductForces.push_back(pftOverlay);
   }

   IntervalIndexType installRailingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval(segmentKey);
   bool bTSRemovedAfterRailingSystemInstalled = (0 < std::count_if(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),std::bind2nd(std::greater<IntervalIndexType>(),installRailingSystemIntervalIdx)));
   if ( intervalIdx == installRailingSystemIntervalIdx || 
        (bIsTempSupportRemovalInterval && bTSRemovedAfterRailingSystemInstalled)
      )
   {
      Float64 wTB = pProductLoads->GetTrafficBarrierLoad(segmentKey);
      Float64 wSW = pProductLoads->GetSidewalkLoad(segmentKey);

      if ( !IsZero(wTB) )
      {
         vProductForces.push_back(pftTrafficBarrier);
      }

      if ( !IsZero(wSW) )
      {
         vProductForces.push_back(pftSidewalk);
      }
   }

#pragma Reminder("UPDATE: need to deal with user defined loads")
   // if user defined DC,DW,LLIM is applied in this interval, or if they were applied
   // in a previous interval and temporary supports are removed in this interval,
   // include the product load type
   //std::vector<IntervalIndexType> vUserDefinedLoads = GetUserDefinedLoadIntervals(segmentKey,pftUserDC);
   //bool bIsUserDefinedLoadInterval = false;
   //bool bIsTSRemovedAfterUserDefinedLoad = false;
   //if ( bIsUserDefinedLoadInterval || bIsTSRemovedAfterUserDefinedLoad )
   //{
   //   vProductForces.push_back(pftUserDC);
   //}

   //vUserDefinedLoads = GetUserDefinedLoadIntervals(segmentKey,pftUserDW);
   //bIsUserDefinedLoadInterval = false;
   //bIsTSRemovedAfterUserDefinedLoad = false;
   //if ( bIsUserDefinedLoadInterval || bIsTSRemovedAfterUserDefinedLoad )
   //{
   //   vProductForces.push_back(pftUserDW);
   //}

   if ( !bIsClosure || (bIsClosure && compositeClosureIntervalIdx <= intervalIdx) )
   {
      if ( pIntervals->IsTendonStressingInterval(segmentKey,intervalIdx) )
      {
         if ( !bExternalForcesOnly )
         {
            vProductForces.push_back(pftPrimaryPostTensioning);
         }

         vProductForces.push_back(pftSecondaryEffects);
      }

      if ( !bExternalForcesOnly && !IsZero(pIntervals->GetDuration(segmentKey,intervalIdx)) )
      {
         vProductForces.push_back(pftCreep);
         vProductForces.push_back(pftShrinkage);
         vProductForces.push_back(pftRelaxation);
      }
   }

   ATLASSERT(vProductForces.size() <= (bExternalForcesOnly ? 14 : 19));

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

   for ( int i = 0; i < 19; i++ )
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
      return; // adjustments have already been initialized, return now and don't duplicate work

   // hang onto the current segment key
   m_SegmentKey = segmentKey;

   // reset the functions
   m_ErectionAdjustment[0] = mathLinFunc2d();
   m_ErectionAdjustment[1] = mathLinFunc2d();
   m_ErectionAdjustment[2] = mathLinFunc2d();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(m_SegmentKey);

   if ( intervalIdx == erectionIntervalIdx )
   {
      // When a segment is erected, in general, the location of the supports are different than
      // where the segment was supported during storage. Consider a segment that is supported
      // very near its ends during storage, then, when erected, it is supported near one end and
      // near its center. This would be the case for a cantilevered segment that will receive a 
      // drop in segment later in the construction sequence.
      //
      // The deflections due to elastic loads are self-adjusting. The change in moment and hence
      // the change in curvature account for the change is support locations. 
      //
      // The inelastic deformations due to creep, shrinkage, and relaxation don't behave the same. 
      // The the segment described above, there will be inelastic deflections at mid-segment during
      // storage. The net deflection at the supports in the erected configuration must be zero. The
      // only way to accomplish this is to recogize that there is a sort of rigid body motion of the
      // inelastic deformations. That's the purpose of this code.
      //
      // Find the "hard" supports at the time of erection. These are the supports where the
      // deflections are zero relative to the placement of the erected segment. Get
      // the deflections at these locations in the previous interval. Compute the equation
      // of the straight line connecting these points. The negative of the y-value of this
      // line is the rigid body motion.
      GET_IFACE(IPointOfInterest,pIPoi);

      Float64 signLeft(1.0);
      Float64 signRight(1.0);

      std::vector<pgsPointOfInterest> vPoi(pIPoi->GetPointsOfInterest(m_SegmentKey,POI_INTERMEDIATE_TEMPSUPPORT | POI_INTERMEDIATE_PIER | POI_BOUNDARY_PIER | POI_ABUTMENT,POIFIND_OR));
      if ( vPoi.size() < 2 )
      {
         GET_IFACE(IBridgeDescription,pIBridgeDesc);
         const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(m_SegmentKey);
         const CPierData2* pPier;
         const CTemporarySupportData* pTS;
         pSegment->GetStartSupport(&pPier,&pTS);
         if ( pPier || (pTS && pTS->GetSupportType() == pgsTypes::ErectionTower) )
         {
            std::vector<pgsPointOfInterest> vPoi2(pIPoi->GetPointsOfInterest(m_SegmentKey,POI_ERECTED_SEGMENT | POI_0L));
            ATLASSERT(vPoi2.size() == 1);
            vPoi.insert(vPoi.begin(),vPoi2.begin(),vPoi2.end());
         }

         pSegment->GetEndSupport(&pPier,&pTS);
         if ( pPier || (pTS && pTS->GetSupportType() == pgsTypes::ErectionTower) )
         {
            std::vector<pgsPointOfInterest> vPoi2(pIPoi->GetPointsOfInterest(m_SegmentKey,POI_ERECTED_SEGMENT | POI_10L));
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
            GET_IFACE(IBridge,pBridge);
            SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
            
            if ( m_SegmentKey.segmentIndex == 0 )
            {
               // the first segment has one hard support... use the start poi for the next segment
               ATLASSERT(vPoi.size() == 1);
               CSegmentKey nextSegmentKey(m_SegmentKey);
               nextSegmentKey.segmentIndex++;
               std::vector<pgsPointOfInterest> vPoiR(pIPoi->GetPointsOfInterest(nextSegmentKey,POI_RELEASED_SEGMENT | POI_0L));
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
               std::vector<pgsPointOfInterest> vPoiL(pIPoi->GetPointsOfInterest(prevSegmentKey,POI_RELEASED_SEGMENT | POI_10L));
               ATLASSERT(vPoiL.size() == 1);
               vPoi.insert(vPoi.begin(),vPoiL.front());
               signLeft = -1;
            }
            else
            {
               // this is an interior segment...
               vPoi.clear();

               CSegmentKey prevSegmentKey(m_SegmentKey);
               prevSegmentKey.segmentIndex--;

               
               CSegmentKey nextSegmentKey(m_SegmentKey);
               nextSegmentKey.segmentIndex++;

               std::vector<pgsPointOfInterest> vPoiL(pIPoi->GetPointsOfInterest(prevSegmentKey,POI_RELEASED_SEGMENT | POI_10L));
               std::vector<pgsPointOfInterest> vPoiR(pIPoi->GetPointsOfInterest(nextSegmentKey,POI_RELEASED_SEGMENT | POI_0L));
               ATLASSERT(vPoiL.size() == 1 && vPoiR.size() == 1);
               vPoi.push_back(vPoiL.front());
               vPoi.push_back(vPoiR.front());

               signLeft = -1;
               signRight = -1;
            }
         }

         ATLASSERT(vPoi.size() == 2);
      }

      ATLASSERT( 2 <= vPoi.size() );
      pgsPointOfInterest leftPoi(vPoi.front());
      pgsPointOfInterest rightPoi(vPoi.back());

      LOSSDETAILS* pLeftDetails    = GetLossDetails(pLosses,leftPoi);
      LOSSDETAILS* pRightDetails   = GetLossDetails(pLosses,rightPoi);
      ATLASSERT(pLeftDetails != NULL && pRightDetails != NULL);

      Float64 Xs = pIPoi->ConvertPoiToGirderPathCoordinate(leftPoi);
      Float64 Xe = pIPoi->ConvertPoiToGirderPathCoordinate(rightPoi);
      Float64 Dx = Xe-Xs;
      ATLASSERT(Xs < Xe && !IsZero(Dx));

      for ( int i = 0; i < 3; i++ )
      {
         ProductForceType pfType = (ProductForceType)(pftCreep + i);

         Float64 Ys = -signLeft*pLeftDetails->TimeStepDetails[intervalIdx-1].D[pfType];
         Float64 Ye = -signRight*pRightDetails->TimeStepDetails[intervalIdx-1].D[pfType];

         Float64 Dy = Ye-Ys;
         Float64 slope = Dy/Dx;

         // y = mx+b
         // b = y-mx
         Float64 b = Ys-slope*Xs;

         m_ErectionAdjustment[i].SetSlope(slope);
         m_ErectionAdjustment[i].SetYIntercept(b);
      }
   }
}

Float64 CTimeStepLossEngineer::GetErectionAdjustment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,ProductForceType pfType)
{
   ATLASSERT(m_SegmentKey == poi.GetSegmentKey());

   if ( poi.HasAttribute(POI_CLOSURE) )
   {
      // if POI is at a closure joint there isn't an erection adjustment.
      // closures are cast after erection
      return 0.0;
   }

   GET_IFACE(IPointOfInterest,pIPoi);
   Float64 Xgp = pIPoi->ConvertPoiToGirderPathCoordinate(poi);

   Float64 adjustment = 0;
   switch( pfType )
   {
   case pftCreep:
   case pftShrinkage:
   case pftRelaxation:
      adjustment = m_ErectionAdjustment[pfType-pftCreep].Evaluate(Xgp);
      break;
   default:
      adjustment = 0;
   }
   return adjustment;
}
