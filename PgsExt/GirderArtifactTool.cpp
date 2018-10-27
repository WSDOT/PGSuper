///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

bool FlexureStressFailures(IBroker* pBroker,const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stressType,const pgsSegmentArtifact* pArtifact,bool bBeamStresses)
{
   CollectionIndexType nArtifacts = pArtifact->GetFlexuralStressArtifactCount(intervalIdx,ls,stressType);
   for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
   {
      const pgsFlexuralStressArtifact* pFlexure = pArtifact->GetFlexuralStressArtifact( intervalIdx,ls,stressType,idx );
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


void ListStressFailures(IBroker* pBroker, FailureList& rFailures,
                        const pgsGirderArtifact* pGirderArtifact,bool referToDetailsReport)
{
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   bool bPrestressedGirder = pDocType->IsPGSuperDocument();

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   GET_IFACE2_NOCHECK(pBroker,IProductLoads,pProductLoads); // only used if there are failues
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   IntervalIndexType noncompositeIntervalIdx = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();
   IntervalIndexType lastIntervalIdx      = pIntervals->GetIntervalCount()-1;

   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      const pgsTendonStressArtifact* pTendonStress = pGirderArtifact->GetTendonStressArtifact(ductIdx);
      if ( !pTendonStress->Passed() )
      {
         CString strMsg;
         strMsg.Format(_T("Tendon %d is overstressed"),LABEL_DUCT(ductIdx));
         rFailures.push_back(std::_tstring(strMsg.GetBuffer()));
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
            rFailures.push_back(msg);
         }
         else
         {
            rFailures.push_back(_T("Stresses in the prestressing strands are too high."));
         }
      }

      const pgsStrandSlopeArtifact* pStrandSlope = pArtifact->GetStrandSlopeArtifact();
      if ( !pStrandSlope->Passed() )
      {
         if ( 1 < nSegments )
         {
            CString strMsg;
            strMsg.Format(_T("Strand slope exceeds maximum for Segment %d"),LABEL_SEGMENT(segIdx));
            rFailures.push_back(std::_tstring(strMsg.GetBuffer()));
         }
         else
         {
            rFailures.push_back(_T("Strand slope exceeds maximum"));
         }
      }

      const pgsHoldDownForceArtifact* pHoldDownForce = pArtifact->GetHoldDownForceArtifact();
      if ( !pHoldDownForce->Passed() )
      {
         if ( 1 < nSegments )
         {
            CString strMsg;
            strMsg.Format(_T("Hold down force exceeds maximum for Segment %d"),LABEL_SEGMENT(segIdx));
            rFailures.push_back(std::_tstring(strMsg.GetBuffer()));
         }
         else
         {
            rFailures.push_back(_T("Hold down force exceeds maximum"));
         }
      }

      bool bFutureOverlay = pBridge->IsFutureOverlay();
      IntervalIndexType overlayIntervalIdx = !bFutureOverlay ? INVALID_INDEX : pIntervals->GetOverlayInterval();

      // loop for beam and deck stresses
      for ( int i = 0; i < 2; i++ )
      {
         bool bBeamStresses = (i == 0 ? true : false);

         if ( bPrestressedGirder )
         {
            if ( bBeamStresses )
            {
               if ( FlexureStressFailures(pBroker,segmentKey,releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,pArtifact,bBeamStresses) )
               {
                  std::_tostringstream os;
                  os << _T("Compression stress check failed for ") << GetLimitStateString(pgsTypes::ServiceI) << _T(" Limit State in Interval ") << LABEL_INTERVAL(releaseIntervalIdx) << _T(" ") << pIntervals->GetDescription(releaseIntervalIdx);
                  if ( 1 < nSegments )
                  {
                     os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
                  }
                  rFailures.push_back(os.str());
               }

               if ( FlexureStressFailures(pBroker,segmentKey,releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,pArtifact,bBeamStresses) )
               {
                  std::_tostringstream os;
                  os << _T("Tensile stress check failed for ") << GetLimitStateString(pgsTypes::ServiceI) << _T(" Limit State in Interval ") << LABEL_INTERVAL(releaseIntervalIdx) << _T(" ") << pIntervals->GetDescription(releaseIntervalIdx);
                  if ( 1 < nSegments )
                  {
                     os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
                  }
                  rFailures.push_back(os.str());
               }

               GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowable);
               if ( pAllowable->CheckTemporaryStresses() )
               {
                  if ( 0 < NtMax && 0 < Nt )
                  {
                     if ( FlexureStressFailures(pBroker,segmentKey,tsRemovalIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,pArtifact,bBeamStresses) )
                     {
                        std::_tostringstream os;
                        os << _T("Compression stress check failed for ") << GetLimitStateString(pgsTypes::ServiceI) << _T(" Limit State in Interval ") << LABEL_INTERVAL(tsRemovalIntervalIdx) << _T(" ") << pIntervals->GetDescription(tsRemovalIntervalIdx);
                        if ( 1 < nSegments )
                        {
                           os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
                        }
                        rFailures.push_back(os.str());
                     }

                     if ( FlexureStressFailures(pBroker,segmentKey,tsRemovalIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,pArtifact,bBeamStresses) )
                     {
                        std::_tostringstream os;
                        os << _T("Tensile stress check failed for ") << GetLimitStateString(pgsTypes::ServiceI) << _T(" Limit State in Interval ") << LABEL_INTERVAL(tsRemovalIntervalIdx) << _T(" ") << pIntervals->GetDescription(tsRemovalIntervalIdx);
                        if ( 1 < nSegments )
                        {
                           os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
                        }
                        rFailures.push_back(os.str());
                     }
                  }

                  if ( FlexureStressFailures(pBroker,segmentKey,noncompositeIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,pArtifact,bBeamStresses) )
                  {
                     std::_tostringstream os;
                     os << _T("Compression stress check failed for ") << GetLimitStateString(pgsTypes::ServiceI) << _T(" Limit State in Interval ") << LABEL_INTERVAL(noncompositeIntervalIdx) << _T(" ") << pIntervals->GetDescription(noncompositeIntervalIdx);
                     if ( 1 < nSegments )
                     {
                        os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
                     }
                     rFailures.push_back(os.str());
                  }

                  if ( FlexureStressFailures(pBroker,segmentKey, noncompositeIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,pArtifact,bBeamStresses) )
                  {
                     std::_tostringstream os;
                     os << _T("Tensile stress check failed for ") << GetLimitStateString(pgsTypes::ServiceI) << _T(" Limit State in Interval ") << LABEL_INTERVAL(noncompositeIntervalIdx) << _T(" ") << pIntervals->GetDescription(noncompositeIntervalIdx);
                     if ( 1 < nSegments )
                     {
                        os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
                     }
                     rFailures.push_back(os.str());
                  }
               }
            } // end if bBeamStresses

            if (noncompositeIntervalIdx != compositeIntervalIdx)
            {
               if (FlexureStressFailures(pBroker, segmentKey, compositeIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression, pArtifact, bBeamStresses))
               {
                  std::_tostringstream os;
                  os << _T("Compression stress check failed for ") << GetLimitStateString(pgsTypes::ServiceI) << _T(" Limit State in Interval ") << LABEL_INTERVAL(compositeIntervalIdx) << _T(" ") << pIntervals->GetDescription(compositeIntervalIdx);
                  if (bBeamStresses && 1 < nSegments)
                  {
                     os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
                  }
                  else if (!bBeamStresses)
                  {
                     os << _T(" for Deck");
                  }
                  rFailures.push_back(os.str());
               }

               if (FlexureStressFailures(pBroker, segmentKey, compositeIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension, pArtifact, bBeamStresses))
               {
                  std::_tostringstream os;
                  os << _T("Tensile stress check failed for ") << GetLimitStateString(pgsTypes::ServiceI) << _T(" Limit State in Interval ") << LABEL_INTERVAL(compositeIntervalIdx) << _T(" ") << pIntervals->GetDescription(compositeIntervalIdx);
                  if (bBeamStresses && 1 < nSegments)
                  {
                     os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
                  }
                  else if (!bBeamStresses)
                  {
                     os << _T(" for Deck");
                  }
                  rFailures.push_back(os.str());
               }
            }

            if (bFutureOverlay)
            {
               if (FlexureStressFailures(pBroker, segmentKey, overlayIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression, pArtifact, bBeamStresses))
               {
                  std::_tostringstream os;
                  os << _T("Compression stress check failed for ") << GetLimitStateString(pgsTypes::ServiceI) << _T(" Limit State in Interval ") << LABEL_INTERVAL(overlayIntervalIdx) << _T(" ") << pIntervals->GetDescription(overlayIntervalIdx);
                  if (bBeamStresses && 1 < nSegments)
                  {
                     os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
                  }
                  else if (!bBeamStresses)
                  {
                     os << _T(" for Deck");
                  }
                  rFailures.push_back(os.str());
               }

               if (FlexureStressFailures(pBroker, segmentKey, overlayIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension, pArtifact, bBeamStresses))
               {
                  std::_tostringstream os;
                  os << _T("Tensile stress check failed for ") << GetLimitStateString(pgsTypes::ServiceI) << _T(" Limit State in Interval ") << LABEL_INTERVAL(overlayIntervalIdx) << _T(" ") << pIntervals->GetDescription(overlayIntervalIdx);
                  if (bBeamStresses && 1 < nSegments)
                  {
                     os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
                  }
                  else if (!bBeamStresses)
                  {
                     os << _T(" for Deck");
                  }
                  rFailures.push_back(os.str());
               }
            }

            GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowable);
            if ( pAllowable->CheckFinalDeadLoadTensionStress() )
            {
               if ( FlexureStressFailures(pBroker,segmentKey,compositeIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,pArtifact,bBeamStresses) )
               {
                  std::_tostringstream os;
                  os << _T("Tension stress check failed for ") << GetLimitStateString(pgsTypes::ServiceI) << _T(" Limit State in Interval ") << LABEL_INTERVAL(compositeIntervalIdx) << _T(" ") << pIntervals->GetDescription(compositeIntervalIdx);
                  if ( bBeamStresses && 1 < nSegments )
                  {
                     os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
                  }
                  else if ( !bBeamStresses )
                  {
                     os << _T(" for Deck");
                  }
                  rFailures.push_back(os.str());
               }
            }

            if ( FlexureStressFailures(pBroker,segmentKey,lastIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,pArtifact,bBeamStresses) )
            {
               std::_tostringstream os;
               os << _T("Compression stress check failed for ") << GetLimitStateString(pgsTypes::ServiceI) << _T(" Limit State in Interval ") << LABEL_INTERVAL(lastIntervalIdx) << _T(" ") << pIntervals->GetDescription(lastIntervalIdx);
               if ( bBeamStresses && 1 < nSegments )
               {
                  os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
               }
               else if ( !bBeamStresses )
               {
                  os << _T(" for Deck");
               }
               rFailures.push_back(os.str());
            }

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
            {
               if ( FlexureStressFailures(pBroker,segmentKey,lastIntervalIdx,pgsTypes::ServiceIA,pgsTypes::Compression,pArtifact,bBeamStresses) )
               {
                  std::_tostringstream os;
                  os << _T("Compression stress check failed for ") << GetLimitStateString(pgsTypes::ServiceIA) << _T(" Limit State in Interval ") << LABEL_INTERVAL(lastIntervalIdx) << _T(" ") << pIntervals->GetDescription(lastIntervalIdx);
                  if ( bBeamStresses && 1 < nSegments )
                  {
                     os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
                  }
                  else if ( !bBeamStresses )
                  {
                     os << _T(" for Deck");
                  }
                  rFailures.push_back(os.str());
               }
            }

            if ( FlexureStressFailures(pBroker,segmentKey,lastIntervalIdx,pgsTypes::ServiceIII,pgsTypes::Tension,pArtifact,bBeamStresses) )
            {
               std::_tostringstream os;
               os << _T("Tensile stress check failed for ") << GetLimitStateString(pgsTypes::ServiceIII) << _T(" Limit State in Interval ") << LABEL_INTERVAL(lastIntervalIdx) << _T(" ") << pIntervals->GetDescription(lastIntervalIdx);
               if ( bBeamStresses && 1 < nSegments )
               {
                  os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
               }
               else if ( !bBeamStresses )
               {
                  os << _T(" for Deck");
               }
               rFailures.push_back(os.str());
            }

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               if ( FlexureStressFailures(pBroker,segmentKey,lastIntervalIdx,pgsTypes::FatigueI,pgsTypes::Compression,pArtifact,bBeamStresses) )
               {
                  std::_tostringstream os;
                  os << _T("Compression stress check failed for ") << GetLimitStateString(pgsTypes::FatigueI) << _T(" Limit State in Interval ") << LABEL_INTERVAL(lastIntervalIdx) << _T(" ") << pIntervals->GetDescription(lastIntervalIdx);
                  if ( bBeamStresses && 1 < nSegments )
                  {
                     os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
                  }
                  else if ( !bBeamStresses )
                  {
                     os << _T(" for Deck");
                  }
                  rFailures.push_back(os.str());
               }
            }
         }
         else
         {
            // spliced girder
            GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowableConcreteStress);
            std::vector<IntervalIndexType> vIntervals( pIntervals->GetSpecCheckIntervals(segmentKey) );
            for (const auto& intervalIdx : vIntervals)
            {
               std::vector<pgsTypes::LimitState> vLimitStates(pAllowableConcreteStress->GetStressCheckLimitStates(intervalIdx));
               for ( int st = 0; st < 2; st++ ) // loop over Compression/Tension
               {
                  pgsTypes::StressType stressType = (pgsTypes::StressType)st;
                  for (const auto& limitState : vLimitStates)
                  {
                     if ( pAllowableConcreteStress->IsStressCheckApplicable(girderKey,intervalIdx,limitState,stressType) )
                     {
                        if ( FlexureStressFailures(pBroker,segmentKey,intervalIdx,limitState,stressType,pArtifact,bBeamStresses) )
                        {
                           std::_tostringstream os;
                           os << GetStressTypeString(stressType) << _T(" stress check failed for ") << GetLimitStateString(limitState) << _T(" Limit State in Interval ") << LABEL_INTERVAL(intervalIdx) << _T(", ") << pIntervals->GetDescription(intervalIdx);
                           if ( 1 < nSegments )
                           {
                              os << _T(" for Segment ") << LABEL_SEGMENT(segIdx);
                           }
                           rFailures.push_back(os.str());
                        }
                     }
                  } // next limit state
               } // next stress type
            } // next interval
         } // end if 
      } // next i (beam,deck)
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
      rFailures.push_back(os.str());
   }

   if ( MomentCapacityFailures(pBroker,pGirderArtifact,intervalIdx,ls,false) )
   {
      std::_tostringstream os;
      os << _T("Ultimate moment capacity (negative moment) check failed for ") << strLimitState << _T(" Limit State") << std::ends;
      rFailures.push_back(os.str());
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
            rFailures.push_back(os.str());

            bContinue1 = false;
         }

         if ( bContinue2 && /*pLongReinf->IsApplicable() &&*/ !pLongReinf->Passed() )
         {
            std::_tostringstream os;
            os << _T("Longitudinal Reinforcement for Shear check failed for ") << strLimitState << _T(" Limit State") << std::ends;
            rFailures.push_back(os.str());

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
            os << _T("Horizontal Interface Shears/Length check failed for ") << strLimitState << _T(" Limit State [") << LrfdCw8th(_T("5.8.4"),_T("5.7.4")) << _T("].") << std::ends;
            rFailures.push_back(os.str());

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
            rFailures.push_back(os.str());
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
         CString strMsg;
         if ( 1 < nSegments )
         {
            strMsg.Format(_T("Debond arrangement checks failed for Segment %d."),LABEL_SEGMENT(segIdx));
         }
         else
         {
            strMsg.Format(_T("Debond arrangement checks failed."));
         }

         rFailures.push_back(strMsg.GetBuffer());
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
      const pgsSplittingZoneArtifact* pBZArtifact = pArtifact->GetStirrupCheckArtifact()->GetSplittingZoneArtifact();
      if ( !pBZArtifact->Passed() )
      {
         CString strZone( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() ? _T("Splitting") : _T("Bursting") );
         CString strMsg;
         if ( 1 < nSegments )
         {
            strMsg.Format(_T("%s zone check failed for Segment %d."),strZone,LABEL_SEGMENT(segIdx));
         }
         else
         {
            strMsg.Format(_T("%s zone check failed."),strZone);
         }

         rFailures.push_back(strMsg.GetBuffer());
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
      const pgsConfinementArtifact& rShear = pStirrups->GetConfinementArtifact();
      if ( !rShear.Passed() )
      {
         CString strMsg;
         if ( 1 < nSegments )
         {
            strMsg.Format(_T("Confinement zone check failed for Segment %d."),LABEL_SEGMENT(segIdx));
         }
         else
         {
            strMsg.Format(_T("Confinement zone check failed."));
         }

         rFailures.push_back(strMsg.GetBuffer());
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
         CString strMsg;
         if ( 1 < nSegments )
         {
            strMsg.Format(_T("Girder Dimension Detailing check failed for Segment %d."),LABEL_SEGMENT(segIdx));
         }
         else
         {
            strMsg.Format(_T("Girder Dimension Detailing check failed"));
         }
         rFailures.push_back(strMsg.GetBuffer());
      }

      const pgsSegmentStabilityArtifact* pSegmentStability = pArtifact->GetSegmentStabilityArtifact();
      if ( !pSegmentStability->Passed() )
      {
         CString strMsg;
         if ( 1 < nSegments )
         {
            strMsg.Format(_T("Global Girder Stability check failed for Segment %d."),LABEL_SEGMENT(segIdx));
         }
         else
         {
            strMsg.Format(_T("Global Girder Stability check failed"));
         }
         rFailures.push_back(strMsg.GetBuffer());
      }

      // Lifting
      const stbLiftingCheckArtifact* pLifting = pArtifact->GetLiftingCheckArtifact();
      if (pLifting!=nullptr && !pLifting->Passed() )
      {
         CString strMsg;
         if ( 1 < nSegments )
         {
            strMsg.Format(_T("Lifting checks failed for Segment %d."),LABEL_SEGMENT(segIdx));
         }
         else
         {
            strMsg.Format(_T("Lifting checks failed"));
         }
         rFailures.push_back(strMsg.GetBuffer());
      }

      // Hauling
      const pgsHaulingAnalysisArtifact* pHauling = pArtifact->GetHaulingAnalysisArtifact();
      if ( pHauling != nullptr && !pHauling->Passed() )
      {
         CString strMsg;
         if ( 1 < nSegments )
         {
            strMsg.Format(_T("Hauling checks failed for Segment %d."),LABEL_SEGMENT(segIdx));
         }
         else
         {
            strMsg.Format(_T("Hauling checks failed"));
         }
         rFailures.push_back(strMsg.GetBuffer());
      }
   } // next segment


   // Constructability
   const pgsConstructabilityArtifact* pConstruct = pGirderArtifact->GetConstructabilityArtifact();
   if ( !pConstruct->SlabOffsetPassed() )
   {
      CString strMsg(_T("Slab Offset (\"A\" Dimension) check failed"));
      rFailures.push_back(strMsg.GetBuffer());
   }

   if (!pConstruct->FinishedElevationPassed())
   {
      CString strMsg(_T("Finished elevation check failed"));
      rFailures.push_back(strMsg.GetBuffer());
   }

   if ( pConstruct->IsHaunchAtBearingCLsApplicable() && !pConstruct->HaunchAtBearingCLsPassed() )
   {
      CString strMsg(_T("Minimum haunch depth at bearing centerlines check failed"));
      rFailures.push_back(strMsg.GetBuffer());
   }

   if ( !pConstruct->MinimumFilletPassed() )
   {
      CString strMsg(_T("Minimum fillet value exceeded for bridge check failed"));
      rFailures.push_back(strMsg.GetBuffer());
   }

   if (!pConstruct->PrecamberPassed())
   {
      rFailures.push_back(_T("Precamber check failed"));
   }

   if ( !pConstruct->BottomFlangeClearancePassed() )
   {
      rFailures.push_back(_T("Bottom flange clearance check failed"));
   }

   if ( !pConstruct->HaunchGeometryPassed() )
   {
      rFailures.push_back(_T("Excess camber geometry check failed"));
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
            rFailures.push_back(_T("Live Load Deflection check failed. Refer to the Details or Specification Check Report for more information"));
         }
         else
         {
            rFailures.push_back(_T("Live Load Deflection check failed"));
         }
      }
   }

   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      const pgsDuctSizeArtifact* pDuctSizeArtifact = pGirderArtifact->GetDuctSizeArtifact(ductIdx);
      if ( !pDuctSizeArtifact->PassedRadiusOfCurvature() )
      {
         CString strMsg;
         strMsg.Format(_T("Radius of Curvature of duct failed for Duct %d."),LABEL_DUCT(ductIdx));
         rFailures.push_back(strMsg.GetBuffer());
      }

      if ( !pDuctSizeArtifact->PassedDuctArea() || !pDuctSizeArtifact->PassedDuctSize() )
      {
         CString strMsg;
         strMsg.Format(_T("Duct Size check failed for Duct %d."),LABEL_DUCT(ductIdx));
         rFailures.push_back(strMsg.GetBuffer());
      }
   }
}

