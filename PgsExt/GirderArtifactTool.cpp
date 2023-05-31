///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\GirderArtifactTool.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\SplittingCheckEngineer.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Allowables.h>
#include <IFace\AnalysisResults.h>
#include <IFace\DocumentType.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool FlexureStressFailures(IBroker* pBroker,const CSegmentKey& segmentKey,const StressCheckTask& task,const pgsSegmentArtifact* pArtifact,bool bBeamStresses)
{
   CollectionIndexType nArtifacts = pArtifact->GetFlexuralStressArtifactCount(task);
   for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
   {
      const pgsFlexuralStressArtifact* pFlexure = pArtifact->GetFlexuralStressArtifact( task,idx );
      if ( bBeamStresses )
      {
         if( !pFlexure->BeamPassed() )
         {
            return true;
         }
      }
      else
      {
         if( !pFlexure->DeckPassed() )
         {
            return true;
         }
      }
   }

   return false;
}


void ListStressFailures(IBroker* pBroker, FailureList& rFailures, const pgsGirderArtifact* pGirderArtifact,bool referToDetailsReport)
{
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   bool bPrestressedGirder = pDocType->IsPGSuperDocument();

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   GET_IFACE2_NOCHECK(pBroker,IProductLoads,pProductLoads); // only used if there are failues
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker, IStressCheck, pStressCheck);

   IntervalIndexType noncompositeIntervalIdx = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();
   IntervalIndexType lastIntervalIdx      = pIntervals->GetIntervalCount()-1;

   GET_IFACE2(pBroker,IGirderTendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      const pgsTendonStressArtifact* pTendonStress = pGirderArtifact->GetTendonStressArtifact(ductIdx);
      if ( !pTendonStress->Passed() )
      {
         std::_tostringstream os;
         os << _T("Tendon ") << LABEL_DUCT(ductIdx) << _T(" is overstressed");
         rFailures.emplace_back(os.str());
      }
   }

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);

      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);

      StrandIndexType Nt    = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);
      StrandIndexType NtMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary);

      IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType tsRemovalIntervalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);

      const pgsStrandStressArtifact* pStrandStress = pArtifact->GetStrandStressArtifact();
      if ( !pStrandStress->Passed() )
      {
         if (referToDetailsReport)
         {
            std::_tstring msg(_T("Strand Stresses [") + std::_tstring(LrfdCw8th(_T("5.9.3"), _T("5.9.2.2"))) + _T("] have been exceeded.  See the Details Report for more information"));
            rFailures.emplace_back(msg);
         }
         else
         {
            rFailures.emplace_back(_T("Stresses in the prestressing strands are too high."));
         }
      }

      const pgsStrandSlopeArtifact* pStrandSlope = pArtifact->GetStrandSlopeArtifact();
      if ( !pStrandSlope->Passed() )
      {
         std::_tostringstream os;
         if ( 1 < nSegments )
         {
            os << _T("Strand slope exceeds maximum for Segment ") << LABEL_SEGMENT(segIdx);
         }
         else
         {
            os << _T("Strand slope exceeds maximum");
         }
         rFailures.emplace_back(os.str());
      }

      const pgsHoldDownForceArtifact* pHoldDownForce = pArtifact->GetHoldDownForceArtifact();
      if ( !pHoldDownForce->Passed() )
      {
         std::_tostringstream os;
         if ( 1 < nSegments )
         {
            os << _T("Hold down force exceeds maximum for Segment ") << LABEL_SEGMENT(segIdx);
         }
         else
         {
            os << _T("Hold down force exceeds maximum");
         }
         rFailures.emplace_back(os.str());
      }

      const auto* pPlantHandling = pArtifact->GetPlantHandlingWeightArtifact();
      if (!pPlantHandling->Passed())
      {
         std::_tostringstream os;
         if (1 < nSegments)
         {
            os << _T("Plant handling weight exceeds maximum for Segment ") <<  LABEL_SEGMENT(segIdx);
         }
         else
         {
            os << _T("Plant handling weight exceeds maximum");
         }
         rFailures.emplace_back(os.str());
      }


      std::vector<StressCheckTask> vStressCheckTasks = pStressCheck->GetStressCheckTasks(segmentKey);
      for (int i = 0; i < 2; i++) // loop for beam and deck
      {
         bool bBeamStresses = (i == 0 ? true : false);
         for (const auto& task : vStressCheckTasks)
         {
            if (FlexureStressFailures(pBroker, segmentKey, task, pArtifact, bBeamStresses))
            {
               std::_tostringstream os;
               os << GetStressTypeString(task.stressType) << _T(" stress check failed for ") << GetLimitStateString(task.limitState) << _T(" Limit State in Interval ") << LABEL_INTERVAL(task.intervalIdx) << _T(" ") << pIntervals->GetDescription(task.intervalIdx);
               if (bBeamStresses && 1 < nSegments)
               {
                  os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
               }
               else if (!bBeamStresses)
               {
                  os << _T(" for Deck");
               }
               rFailures.emplace_back(os.str());
            }
         } // next task
      } // beam/deck loop

      if (!pArtifact->GetPrincipalTensionStressArtifact()->Passed())
      {
         std::_tostringstream os;
         if (1 < nSegments)
         {
            os << _T("Principal tension stress in webs stress check failed for Segment ") << LABEL_SEGMENT(segIdx);
            if (referToDetailsReport)
            {
               os << _T(" See the Details Report for more information");
            }
         }
         else
         {
            os << _T("Principal tension stress in webs stress check failed.");
            if (referToDetailsReport)
            {
               os << _T(" See the Details Report for more information");
            }
         }
         rFailures.emplace_back(os.str());
      }

      const auto* pFatigueArtifact = pArtifact->GetReinforcementFatigueArtifact();
      if (!pArtifact->GetReinforcementFatigueArtifact()->Passed())
      {
         std::_tostringstream os;
         os << _T("Reinforcement fatigue check failed.");
         rFailures.emplace_back(os.str());
      }
   } // next segment
}

bool MomentCapacityFailures(IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bPositiveMoment)
{
   CollectionIndexType nArtifacts = (bPositiveMoment ? pGirderArtifact->GetPositiveMomentFlexuralCapacityArtifactCount(intervalIdx,ls) : pGirderArtifact->GetNegativeMomentFlexuralCapacityArtifactCount(intervalIdx, ls));
   for ( CollectionIndexType artifactIdx = 0; artifactIdx < nArtifacts; artifactIdx++ )
   {
      const pgsFlexuralCapacityArtifact* pFlexure = (bPositiveMoment ? 
         pGirderArtifact->GetPositiveMomentFlexuralCapacityArtifact( intervalIdx, ls, artifactIdx ) :
         pGirderArtifact->GetNegativeMomentFlexuralCapacityArtifact( intervalIdx, ls, artifactIdx ) );

      if ( !pFlexure->Passed() )
      {
         return true;
      }
   } // next artifact

   return false;
}

void ListMomentCapacityFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact,pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2_NOCHECK(pBroker,IProductLoads,pProductLoads); // only used if there is a failure

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;
   std::_tstring strLimitState(GetLimitStateString(ls));

   if ( MomentCapacityFailures(pBroker,pGirderArtifact,intervalIdx,ls,true) )
   {
      std::_tostringstream os;
      os << _T("Ultimate moment capacity (positive moment) check failed for ") << strLimitState << _T(" Limit State") << std::ends;
      rFailures.emplace_back(os.str());
   }

   if ( MomentCapacityFailures(pBroker,pGirderArtifact,intervalIdx,ls,false) )
   {
      std::_tostringstream os;
      os << _T("Ultimate moment capacity (negative moment) check failed for ") << strLimitState << _T(" Limit State") << std::ends;
      rFailures.emplace_back(os.str());
   }
}

void ListVerticalShearFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact,pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;
   std::_tstring strDescription(pIntervals->GetDescription(intervalIdx));
   std::_tstring strLimitState(GetLimitStateString(ls));

   bool bContinue1 = true; // prevents duplicate failure messages
   bool bContinue2 = true;

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact *pStirrups = pArtifact->GetStirrupCheckArtifact();

      CollectionIndexType nArtifacts = pStirrups->GetStirrupCheckAtPoisArtifactCount( intervalIdx,ls );
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* pPoiArtifacts = pStirrups->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );

         const pgsPointOfInterest& poi = pPoiArtifacts->GetPointOfInterest();

         const pgsVerticalShearArtifact* pShear = pPoiArtifacts->GetVerticalShearArtifact();
         const pgsLongReinfShearArtifact* pLongReinf = pPoiArtifacts->GetLongReinfShearArtifact();

         if ( bContinue1 && !pShear->Passed() )
         {
            std::_tostringstream os;
            os << _T("Ultimate vertical shear capacity check failed for ") << strLimitState << _T(" Limit State") << std::ends;
            rFailures.emplace_back(os.str());

            bContinue1 = false;
         }

         if ( bContinue2 && !pLongReinf->Passed() )
         {
            std::_tostringstream os;
            os << _T("Longitudinal Reinforcement for Shear check failed for ") << strLimitState << _T(" Limit State") << std::ends;
            rFailures.emplace_back(os.str());

            bContinue2 = false;
         }

         if ( !bContinue1 && !bContinue2 )
         {
            return;
         }
      }
   } // next segment
}

void ListHorizontalShearFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact,pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;
   std::_tstring strDescription(pIntervals->GetDescription(intervalIdx));
   std::_tstring strLimitState(GetLimitStateString(ls));

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact *pStirrups = pArtifact->GetStirrupCheckArtifact();

      CollectionIndexType nArtifacts = pStirrups->GetStirrupCheckAtPoisArtifactCount( intervalIdx,ls );
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* pPoiArtifacts = pStirrups->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );

         const pgsHorizontalShearArtifact* pShear = pPoiArtifacts->GetHorizontalShearArtifact();

         if ( !pShear->Passed() )
         {
            std::_tostringstream os;
            os << _T("Horizontal Interface Shear check failed for ") << strLimitState << _T(" Limit State.") << std::ends;
            rFailures.emplace_back(os.str());

            return;
         }
      }
   } // next segment
}

void ListStirrupDetailingFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact,pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;
   std::_tstring strLimitState(GetLimitStateString(ls));

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact *pStirrups = pArtifact->GetStirrupCheckArtifact();

      CollectionIndexType nArtifacts = pStirrups->GetStirrupCheckAtPoisArtifactCount( intervalIdx,ls );
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* pPoiArtifacts = pStirrups->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );

         const pgsStirrupDetailArtifact* pShear = pPoiArtifacts->GetStirrupDetailArtifact();
    
         if ( !pShear->Passed() )
         {
            std::_tostringstream os;
            os << _T("Stirrup detailing checks failed for the ") << strLimitState << _T(" Limit State.") << std::ends;
            rFailures.emplace_back(os.str());
            return;
         }
      }
   } // next segment
}

void ListDebondingFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsDebondArtifact* pDebond = pArtifact->GetDebondArtifact();

      if ( !pDebond->Passed() )
      {
         std::_tostringstream os;
         if ( 1 < nSegments )
         {
            os << _T("Debond arrangement checks failed for Segment ") << LABEL_SEGMENT(segIdx) << _T(".");
         }
         else
         {
            os << _T("Debond arrangement checks failed.");
         }

         rFailures.emplace_back(os.str());
      }
   } // next segment
}

void ListSplittingZoneFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const std::shared_ptr<pgsSplittingCheckArtifact> pSplittingCheckArtifact = pArtifact->GetStirrupCheckArtifact()->GetSplittingCheckArtifact();
      if (pSplittingCheckArtifact && !pSplittingCheckArtifact->Passed() )
      {
         std::_tstring strZone(pgsSplittingCheckEngineer::GetCheckName());
         std::_tostringstream os;
         if ( 1 < nSegments )
         {
            os << strZone << _T(" check failed for Segment ") << LABEL_SEGMENT(segIdx) << _T(".");
         }
         else
         {
            os << strZone << _T(" check failed.");
         }

         rFailures.emplace_back(os.str());
      }
   } // next segment
}

void ListConfinementZoneFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact *pStirrups = pArtifact->GetStirrupCheckArtifact();
      const pgsConfinementCheckArtifact& rShear = pStirrups->GetConfinementArtifact();
      if ( !rShear.Passed() )
      {
         std::_tostringstream os;
         if ( 1 < nSegments )
         {
            os << _T("Confinement zone check failed for Segment ") << LABEL_SEGMENT(segIdx) << _T(".");
         }
         else
         {
            os << _T("Confinement zone check failed.");
         }

         rFailures.emplace_back(os.str());
      }
   } // next segment
}

void ListVariousFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact,bool referToDetails)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);

      // Girder Detailing
      const pgsPrecastIGirderDetailingArtifact* pBeamDetails = pArtifact->GetPrecastIGirderDetailingArtifact();
      if ( !pBeamDetails->Passed() )
      {
         std::_tostringstream os;
         if ( 1 < nSegments )
         {
            os << _T("Girder Dimension Detailing check failed for Segment ") << LABEL_SEGMENT(segIdx) << _T(".");
         }
         else
         {
            os << _T("Girder Dimension Detailing check failed");
         }
         rFailures.emplace_back(os.str());
      }

      const pgsSegmentStabilityArtifact* pSegmentStability = pArtifact->GetSegmentStabilityArtifact();
      if ( !pSegmentStability->Passed() )
      {
         std::_tostringstream os;
         if ( 1 < nSegments )
         {
            os << _T("Global Girder Stability check failed for Segment ") << LABEL_SEGMENT(segIdx) << _T(".");
         }
         else
         {
            os << _T("Global Girder Stability check failed");
         }
         rFailures.push_back(os.str());
      }

      // Lifting
      const WBFL::Stability::LiftingCheckArtifact* pLifting = pArtifact->GetLiftingCheckArtifact();
      if (pLifting!=nullptr && !pLifting->Passed() )
      {
         std::_tostringstream os;
         if ( 1 < nSegments )
         {
            os << _T("Lifting checks failed for Segment ") << LABEL_SEGMENT(segIdx) << _T(".");
         }
         else
         {
            os << _T("Lifting checks failed");
         }
         rFailures.emplace_back(os.str());
      }

      // Hauling
      // not doing a full check... see pgsSegmentArtifact::Passed()
      // don't want to hard ding users for not passing overhangs or haul weight
      // fails will show up in the details.
      const pgsHaulingAnalysisArtifact* pHauling = pArtifact->GetHaulingAnalysisArtifact();
      if (pHauling != nullptr)
      {
         if (!pHauling->Passed(pgsTypes::CrownSlope) || !pHauling->Passed(pgsTypes::Superelevation))
         {
            std::_tostringstream os;
            if (1 < nSegments)
            {
               os << _T("Hauling checks failed for Segment ") << LABEL_SEGMENT(segIdx) << _T(".");
            }
            else
            {
               os << _T("Hauling checks failed");
            }
            rFailures.emplace_back(os.str());
         }
      }
   } // next segment


   // Constructability
   const pgsConstructabilityArtifact* pConstruct = pGirderArtifact->GetConstructabilityArtifact();
   if ( !pConstruct->SlabOffsetPassed() )
   {
      rFailures.emplace_back(_T("Slab Offset (\"A\" Dimension) check failed"));
   }

   if (!pConstruct->FinishedElevationPassed())
   {
      rFailures.emplace_back(_T("Finished elevation check failed"));
   }

   if (!pConstruct->MinimumHaunchDepthPassed())
   {
      rFailures.emplace_back(_T("Minimum haunch depth vs fillet check failed"));
   }

   if ( pConstruct->IsHaunchAtBearingCLsApplicable() && !pConstruct->HaunchAtBearingCLsPassed() )
   {
      rFailures.emplace_back(_T("Minimum haunch depth at bearing centerlines check failed"));
   }

   if ( !pConstruct->MinimumFilletPassed() )
   {
      rFailures.emplace_back(_T("Minimum fillet value exceeded for bridge check failed"));
   }

   if (!pConstruct->PrecamberPassed())
   {
      rFailures.emplace_back(_T("Precamber check failed"));
   }

   if ( !pConstruct->BottomFlangeClearancePassed() )
   {
      rFailures.emplace_back(_T("Bottom flange clearance check failed"));
   }

   if ( !pConstruct->HaunchGeometryPassed() )
   {
      rFailures.emplace_back(_T("Excess camber geometry check failed"));
   }

   // Live Load Deflection
   SpanIndexType startSpanIdx, endSpanIdx;
   pBridge->GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      IndexType index = spanIdx - startSpanIdx;
      const pgsDeflectionCheckArtifact* pDef = pGirderArtifact->GetDeflectionCheckArtifact(index);
      if (pDef!=nullptr && !pDef->Passed())
      {
         if (referToDetails)
         {
            rFailures.emplace_back(_T("Live Load Deflection check failed. Refer to the Details or Specification Check Report for more information"));
         }
         else
         {
            rFailures.emplace_back(_T("Live Load Deflection check failed"));
         }
      }
   }

   GET_IFACE2(pBroker,IGirderTendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      const pgsDuctSizeArtifact* pDuctSizeArtifact = pGirderArtifact->GetDuctSizeArtifact(ductIdx);
      if ( !pDuctSizeArtifact->PassedRadiusOfCurvature() )
      {
         std::_tostringstream os;
         os << _T("Radius of Curvature of duct failed for Duct ") << LABEL_DUCT(ductIdx) << _T(".");
         rFailures.emplace_back(os.str());
      }

      if ( !pDuctSizeArtifact->PassedDuctArea() || !pDuctSizeArtifact->PassedDuctSize() )
      {
         std::_tostringstream os;
         os << _T("Duct Size check failed for Duct ") << LABEL_DUCT(ductIdx) << _T(".");
         rFailures.emplace_back(os.str());
      }
   }
}

