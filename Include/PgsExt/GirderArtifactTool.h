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

#ifndef INCLUDED_PGSEXT_GIRDERARTIFACTTOOL_H_
#define INCLUDED_PGSEXT_GIRDERARTIFACTTOOL_H_

#include <PgsExt\GirderArtifact.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <LRFD\VersionMgr.h>

#include <vector>

#pragma Reminder("TODO: Re-implement so these huge functions are not inline")

/*****************************************************************************

   GirderArtifactTools

   Tools for Artifact for a prestressed girder.


DESCRIPTION
   functions to work on artifacts


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.13.2009 : Created file
*****************************************************************************/
inline const std::_tstring LimitStateName(pgsTypes::LimitState ls)
{
   switch(ls)
   {
      case pgsTypes::StrengthI:    return _T("Strength I");
      case pgsTypes::StrengthII:   return _T("Strength II");
      default: ATLASSERT(FALSE);   return _T("???");
   }
}

// At one time, these functions worked directly with the Reporting system, but the list
// of failures are needed by other subsystems and just strings work fine
typedef std::vector<std::_tstring> FailureList;
typedef FailureList::iterator    FailureListIterator;

inline bool flexure_stress_failures(IBroker* pBroker,const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stressType,const pgsSegmentArtifact* pArtifact)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   CollectionIndexType nArtifacts = pArtifact->GetFlexuralStressArtifactCount(intervalIdx,ls,stressType);
   for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
   {
      const pgsFlexuralStressArtifact* pFlexure = pArtifact->GetFlexuralStressArtifact( intervalIdx,ls,stressType,idx );
      if( !pFlexure->Passed() )
         return true;
   }

   return false;
}


inline void list_stress_failures(IBroker* pBroker, FailureList& rFailures,
                           const pgsGirderArtifact* pGirderArtifact,bool referToDetailsReport)
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      const pgsTendonStressArtifact* pTendonStress = pGirderArtifact->GetTendonStressArtifact(ductIdx);
      if ( !pTendonStress->Passed() )
      {
   #pragma Reminder("UPDATE: need to do a better job reporting tendon stress failures")
         // Which duct?
         // There are several reasons - what failed?
         rFailures.push_back(_T("Stresses in the tendons are too high."));
      }
   }

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);

      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);

      GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
      StrandIndexType Nt    = pStrandGeom->GetNumStrands(segmentKey,pgsTypes::Temporary);
      StrandIndexType NtMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary);

      GET_IFACE2(pBroker,IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType tsRemovalIntervalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
      IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
      IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
      IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

      const pgsStrandStressArtifact* pStrandStress = pArtifact->GetStrandStressArtifact();
      if ( !pStrandStress->Passed() )
      {
         if (referToDetailsReport)
            rFailures.push_back(_T("Strand Stresses [5.9.3] have been exceeded.  See the Details Report for more information"));
         else
            rFailures.push_back(_T("Stresses in the prestressing strands are too high."));
      }

      const pgsStrandSlopeArtifact* pStrandSlope = pArtifact->GetStrandSlopeArtifact();
      if ( !pStrandSlope->Passed() )
      {
         rFailures.push_back(_T("Strand slope is too high."));
      }

      const pgsHoldDownForceArtifact* pHoldDownForce = pArtifact->GetHoldDownForceArtifact();
      if ( !pHoldDownForce->Passed() )
      {
         rFailures.push_back(_T("Hold Down Force is excessive."));
      }

      if ( flexure_stress_failures(pBroker,segmentKey,releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,pArtifact) )
      {
         rFailures.push_back(_T("Compressive stress check failed for Service I for the Casting Yard Stage (At Release)."));
      }

      if ( flexure_stress_failures(pBroker,segmentKey,releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,pArtifact) )
      {
         rFailures.push_back(_T("Tensile stress check failed for Service I for the Casting Yard Stage (At Release)."));
      }

      if ( 0 < NtMax && 0 < Nt )
      {
         if ( flexure_stress_failures(pBroker,segmentKey,tsRemovalIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,pArtifact) )
         {
            rFailures.push_back(_T("Compressive stress check failed for Service I for the Temporary Strand Removal Stage."));
         }

         if ( flexure_stress_failures(pBroker,segmentKey,tsRemovalIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,pArtifact) )
         {
            rFailures.push_back(_T("Tensile stress check failed for Service I for the Temporary Strand Removal."));
         }
      }

      if ( flexure_stress_failures(pBroker,segmentKey,castDeckIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,pArtifact) )
      {
         rFailures.push_back(_T("Compressive stress check failed for Service I for the Deck and Diaphragm Placement Stage (Bridge Site 1)."));
      }

      if ( flexure_stress_failures(pBroker,segmentKey,castDeckIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,pArtifact) )
      {
         rFailures.push_back(_T("Tensile stress check failed for Service I for the Deck and Diaphragm Placement Stage (Bridge Site 1)."));
      }

      if ( flexure_stress_failures(pBroker,segmentKey,compositeDeckIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,pArtifact) )
      {
         rFailures.push_back(_T("Compressive stress check failed for Service I for the Superimposed Dead Load Stage (Bridge Site 2)."));
      }

      if ( flexure_stress_failures(pBroker,segmentKey,liveLoadIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,pArtifact) )
      {
         rFailures.push_back(_T("Compressive stress check failed for Service I for the Final with Live Load Stage (Bridge Site 3)."));
      }

      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         if ( flexure_stress_failures(pBroker,segmentKey,liveLoadIntervalIdx,pgsTypes::ServiceIA,pgsTypes::Compression,pArtifact) )
         {
            rFailures.push_back(_T("Compressive stress check failed for Service IA for the Final with Live Load Stage (Bridge Site 3)."));
         }
      }

      if ( flexure_stress_failures(pBroker,segmentKey,liveLoadIntervalIdx,pgsTypes::ServiceIII,pgsTypes::Tension,pArtifact) )
      {
         rFailures.push_back(_T("Tensile stress check failed for Service III for the Final with Live Load Stage (Bridge Site 3)."));
      }

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         if ( flexure_stress_failures(pBroker,segmentKey,liveLoadIntervalIdx,pgsTypes::FatigueI,pgsTypes::Compression,pArtifact) )
         {
            rFailures.push_back(_T("Compressive stress check failed for Fatigue I for the Final with Live Load Stage (Bridge Site 3)."));
         }
      }
   }
}

inline bool momcap_failures(IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bPositiveMoment)
{
   CollectionIndexType nArtifacts = pGirderArtifact->GetFlexuralCapacityArtifactCount(intervalIdx,ls);
   for ( CollectionIndexType artifactIdx = 0; artifactIdx < nArtifacts; artifactIdx++ )
   {
      const pgsFlexuralCapacityArtifact* pFlexure = (bPositiveMoment ? 
         pGirderArtifact->GetPositiveMomentFlexuralCapacityArtifact( intervalIdx, ls, artifactIdx ) :
         pGirderArtifact->GetNegativeMomentFlexuralCapacityArtifact( intervalIdx, ls, artifactIdx ) );

      if ( !pFlexure->Passed() )
         return true;
   } // next artifact

   return false;
}

inline void list_momcap_failures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact,pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;
   if ( momcap_failures(pBroker,pGirderArtifact,lastIntervalIdx,ls,true) )
   {
      rFailures.push_back(std::_tstring(_T("Ultimate moment capacity (positive moment) check failed for ")) + LimitStateName(ls) + std::_tstring(_T(" Limit State for the Bridge Site Stage 3.")));
   }

   if ( momcap_failures(pBroker,pGirderArtifact,lastIntervalIdx,ls,false) )
   {
      rFailures.push_back(std::_tstring(_T("Ultimate moment capacity (negative moment) check failed for ")) + LimitStateName(ls) + std::_tstring(_T(" Limit State for the Bridge Site Stage 3.")));
   }
}


inline void list_vertical_shear_failures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact,pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact *pStirrups = pArtifact->GetStirrupCheckArtifact();

      GET_IFACE2(pBroker,IIntervals,pIntervals);
      IntervalIndexType intervalIdx = pIntervals->GetLiveLoadInterval();

      bool bContinue1 = true;
      bool bContinue2 = true;

      CollectionIndexType nArtifacts = pStirrups->GetStirrupCheckAtPoisArtifactCount( intervalIdx,ls );
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* pPoiArtifacts = pStirrups->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );

         const pgsPointOfInterest& poi = pPoiArtifacts->GetPointOfInterest();

         const pgsVerticalShearArtifact* pShear = pPoiArtifacts->GetVerticalShearArtifact();
         const pgsLongReinfShearArtifact* pLongReinf = pPoiArtifacts->GetLongReinfShearArtifact();

         if ( bContinue1 && !pShear->Passed() )
         {
            rFailures.push_back(_T("Ultimate vertical shear capacity check failed for ") + LimitStateName(ls) + _T(" Limit State for the Bridge Site Stage 3."));
            bContinue1 = false;
         }

         if ( bContinue2 && /*pLongReinf->IsApplicable() &&*/ !pLongReinf->Passed() )
         {
            rFailures.push_back(_T("Longitudinal Reinforcement for Shear check failed for ") + LimitStateName(ls) + _T(" Limit State for the Bridge Site Stage 3."));
            bContinue2 = false;
         }

         if ( !bContinue1 && !bContinue2 )
            return;
      }
   } // next segment
}

inline void list_horizontal_shear_failures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact,pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact *pStirrups = pArtifact->GetStirrupCheckArtifact();

      GET_IFACE2(pBroker,IIntervals,pIntervals);
      IntervalIndexType intervalIdx = pIntervals->GetLiveLoadInterval();

      CollectionIndexType nArtifacts = pStirrups->GetStirrupCheckAtPoisArtifactCount( intervalIdx,ls );
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* pPoiArtifacts = pStirrups->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );

         const pgsHorizontalShearArtifact* pShear = pPoiArtifacts->GetHorizontalShearArtifact();

         if ( !pShear->Passed() )
         {
            rFailures.push_back(_T("Horizontal Interface Shears/Length check failed for ") + LimitStateName(ls) + _T(" Limit State [5.8.4]."));
            return;
         }
      }
   } // next segment
}

inline void list_stirrup_detailing_failures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact,pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact *pStirrups = pArtifact->GetStirrupCheckArtifact();

      GET_IFACE2(pBroker,IIntervals,pIntervals);
      IntervalIndexType intervalIdx = pIntervals->GetLiveLoadInterval();

      CollectionIndexType nArtifacts = pStirrups->GetStirrupCheckAtPoisArtifactCount( intervalIdx,ls );
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* pPoiArtifacts = pStirrups->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );

         const pgsStirrupDetailArtifact* pShear = pPoiArtifacts->GetStirrupDetailArtifact();
    
         if ( !pShear->Passed() )
         {
            rFailures.push_back(_T("Stirrup detailing checks failed for the ") + LimitStateName(ls) + _T(" Limit State."));
            return;
         }
      }
   } // next segment
}

inline void list_debonding_failures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsDebondArtifact* pDebond = pArtifact->GetDebondArtifact(pgsTypes::Straight);

      if ( !pDebond->Passed() )
      {
         CString strMsg;
         if ( 1 < nSegments )
            strMsg.Format(_T("Debond arrangement checks failed for Segment %d."),LABEL_SEGMENT(segIdx));
         else
            strMsg.Format(_T("Debond arrangement checks failed."));

         rFailures.push_back(strMsg.GetBuffer());
      }
   } // next segment
}

inline void list_splitting_zone_failures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsSplittingZoneArtifact* pBZArtifact = pArtifact->GetStirrupCheckArtifact()->GetSplittingZoneArtifact();
      if ( !pBZArtifact->Passed() )
      {
         CString strZone( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() ? _T("Splitting") : _T("Bursting") );
         CString strMsg;
         if ( 1 < nSegments )
            strMsg.Format(_T("%s zone check failed for Segment %d."),strZone,LABEL_SEGMENT(segIdx));
         else
            strMsg.Format(_T("%s zone check failed."),strZone);

         rFailures.push_back(strMsg.GetBuffer());
      }
   } // next segment
}

inline void list_confinement_zone_failures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact *pStirrups = pArtifact->GetStirrupCheckArtifact();
      const pgsConfinementArtifact& rShear = pStirrups->GetConfinementArtifact();
      if ( !rShear.Passed() )
      {
         CString strMsg;
         if ( 1 < nSegments )
         {
            strMsg.Format(_T("Confinement zone checks failed for Segment %d."),LABEL_SEGMENT(segIdx));
         }
         else
         {
            strMsg.Format(_T("Confinement zone checks failed."));
         }

         rFailures.push_back(strMsg.GetBuffer());
      }
   } // next segment
}


inline void list_various_failures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact,bool referToDetails)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);

#pragma Reminder("UPDATE: need to list segment number in error message if multi-segment girder")

      // Girder Detailing
      const pgsPrecastIGirderDetailingArtifact* pBeamDetails = pArtifact->GetPrecastIGirderDetailingArtifact();
      if ( !pBeamDetails->Passed() )
      {
         rFailures.push_back(_T("Girder Dimension Detailing check failed"));
      }

      // Constructability
      const pgsConstructabilityArtifact* pConstruct = pArtifact->GetConstructabilityArtifact();
      if ( !pConstruct->SlabOffsetPassed() )
      {
         rFailures.push_back(_T("Slab Offset (\"A\" Dimension) check failed"));
      }

      if ( !pConstruct->GlobalGirderStabilityPassed() )
      {
         rFailures.push_back(_T("Global Girder Stability check failed"));
      }
      // Lifting
      const pgsLiftingAnalysisArtifact* pLifting = pArtifact->GetLiftingAnalysisArtifact();
      if (pLifting!=NULL && !pLifting->Passed() )
      {
         rFailures.push_back(_T("Lifting checks failed"));
      }

      // Hauling
      const pgsHaulingAnalysisArtifact* pHauling = pArtifact->GetHaulingAnalysisArtifact();
      if (pHauling!=NULL && !pHauling->Passed() )
      {
         rFailures.push_back(_T("Hauling checks failed"));
      }

      // Live Load Deflection
   #pragma Reminder("UPDATE: Need to fix deflection check failure notice")
      // commented out because deflection checks are now done on the girder level, not the segment level
      //
      //const pgsDeflectionCheckArtifact* pDef = pArtifact->GetDeflectionCheckArtifact();
      //if (pDef!=NULL && !pDef->Passed())
      //{
      //   if (referToDetails)
      //      rFailures.push_back(_T("Live Load Deflection check failed. Refer to the Details or Specification Check Report for more information"));
      //   else
      //      rFailures.push_back(_T("Live Load Deflection check failed"));
      //}
   } // next segment
}


#endif // INCLUDED_PGSEXT_GIRDERARTIFACTTOOL_H_
