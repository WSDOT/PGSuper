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

#ifndef INCLUDED_PGSEXT_GIRDERARTIFACTTOOL_H_
#define INCLUDED_PGSEXT_GIRDERARTIFACTTOOL_H_

#include <PgsExt\GirderArtifact.h>
#include <IFace\Bridge.h>
#include <LRFD\VersionMgr.h>

#include <vector>

// TODO: Re-implement so these huge functions are not inline

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

inline bool flexure_stress_failures(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType stressType,const pgsGirderArtifact* pArtifact)
{
   std::vector<pgsFlexuralStressArtifactKey> keys;
   keys = pArtifact->GetFlexuralStressArtifactKeys();
   std::vector<pgsFlexuralStressArtifactKey>::iterator iter;
   for ( iter = keys.begin(); iter != keys.end(); iter++ )
   {
      pgsFlexuralStressArtifactKey key = *iter;

      // is this they type of key we want?
      if ( key.GetStage() != stage || key.GetLimitState() != ls || key.GetStressType() != stressType )
         continue;

      const pgsFlexuralStressArtifact* pFlexure = pArtifact->GetFlexuralStressArtifact( key );

      ATLASSERT( pFlexure != NULL );
      if ( pFlexure == NULL )
         continue;


      if ( stage == pgsTypes::BridgeSite3 && ls == pgsTypes::ServiceIII )
      {
	      if ( !pFlexure->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar) || !pFlexure->BottomPassed(pgsFlexuralStressArtifact::WithRebar))
            return true;
      }
      else if ( stage == pgsTypes::BridgeSite3 && (ls == pgsTypes::ServiceIA || ls == pgsTypes::ServiceI || ls == pgsTypes::FatigueI )  )
      {
	      if ( !pFlexure->TopPassed(pgsFlexuralStressArtifact::WithoutRebar) || !pFlexure->TopPassed(pgsFlexuralStressArtifact::WithRebar) )
            return true;
      }
      else
      {
	      if ( !pFlexure->Passed(pgsFlexuralStressArtifact::WithoutRebar) || !pFlexure->Passed(pgsFlexuralStressArtifact::WithRebar))
            return true;
      }
   }

   return false;
}


inline void list_stress_failures(IBroker* pBroker, FailureList& rFailures,SpanIndexType span,GirderIndexType girder,
                           const pgsGirderArtifact* pArtifact,bool referToDetailsReport)
{
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType Nt    = pStrandGeom->GetNumStrands(span,girder,pgsTypes::Temporary);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(span,girder,pgsTypes::Temporary);

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

   if ( flexure_stress_failures(pBroker,span,girder,pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Compression,pArtifact) )
   {
      rFailures.push_back(_T("Compressive stress check failed for Service I for the Casting Yard Stage (At Release)."));
   }

   if ( flexure_stress_failures(pBroker,span,girder,pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Tension,pArtifact) )
   {
      rFailures.push_back(_T("Tensile stress check failed for Service I for the Casting Yard Stage (At Release)."));
   }

   if ( 0 < NtMax && 0 < Nt )
   {
      if ( flexure_stress_failures(pBroker,span,girder,pgsTypes::TemporaryStrandRemoval,pgsTypes::ServiceI,pgsTypes::Compression,pArtifact) )
      {
         rFailures.push_back(_T("Compressive stress check failed for Service I for the Temporary Strand Removal Stage."));
      }

      if ( flexure_stress_failures(pBroker,span,girder,pgsTypes::TemporaryStrandRemoval,pgsTypes::ServiceI,pgsTypes::Tension,pArtifact) )
      {
         rFailures.push_back(_T("Tensile stress check failed for Service I for the Temporary Strand Removal."));
      }
   }

   if ( flexure_stress_failures(pBroker,span,girder,pgsTypes::BridgeSite1,pgsTypes::ServiceI,pgsTypes::Compression,pArtifact) )
   {
      rFailures.push_back(_T("Compressive stress check failed for Service I for the Deck and Diaphragm Placement Stage (Bridge Site 1)."));
   }

   if ( flexure_stress_failures(pBroker,span,girder,pgsTypes::BridgeSite1,pgsTypes::ServiceI,pgsTypes::Tension,pArtifact) )
   {
      rFailures.push_back(_T("Tensile stress check failed for Service I for the Deck and Diaphragm Placement Stage (Bridge Site 1)."));
   }

   if ( flexure_stress_failures(pBroker,span,girder,pgsTypes::BridgeSite2,pgsTypes::ServiceI,pgsTypes::Compression,pArtifact) )
   {
      rFailures.push_back(_T("Compressive stress check failed for Service I for the Superimposed Dead Load Stage (Bridge Site 2)."));
   }

   if ( flexure_stress_failures(pBroker,span,girder,pgsTypes::BridgeSite3,pgsTypes::ServiceI,pgsTypes::Compression,pArtifact) )
   {
      rFailures.push_back(_T("Compressive stress check failed for Service I for the Final with Live Load Stage (Bridge Site 3)."));
   }

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      if ( flexure_stress_failures(pBroker,span,girder,pgsTypes::BridgeSite3,pgsTypes::ServiceIA,pgsTypes::Compression,pArtifact) )
      {
         rFailures.push_back(_T("Compressive stress check failed for Service IA for the Final with Live Load Stage (Bridge Site 3)."));
      }
   }

   if ( flexure_stress_failures(pBroker,span,girder,pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,pArtifact) )
   {
      rFailures.push_back(_T("Tensile stress check failed for Service III for the Final with Live Load Stage (Bridge Site 3)."));
   }

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      if ( flexure_stress_failures(pBroker,span,girder,pgsTypes::BridgeSite3,pgsTypes::FatigueI,pgsTypes::Compression,pArtifact) )
      {
         rFailures.push_back(_T("Compressive stress check failed for Fatigue I for the Final with Live Load Stage (Bridge Site 3)."));
      }
   }
}

inline bool momcap_failures(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,pgsTypes::Stage stage,pgsTypes::LimitState ls,const pgsGirderArtifact* pArtifact,bool bPositiveMoment)
{
   std::vector<pgsFlexuralCapacityArtifactKey> keys;
   keys = pArtifact->GetFlexuralCapacityArtifactKeys();
   
   std::vector<pgsFlexuralCapacityArtifactKey>::iterator iter;
   for ( iter = keys.begin(); iter != keys.end(); iter++ )
   {
      pgsFlexuralCapacityArtifactKey key = *iter;

      // is this the type of key we are after?
      if ( key.GetStage() != stage || key.GetLimitState() != ls )
         continue;

      const pgsFlexuralCapacityArtifact* pFlexure = (bPositiveMoment ? 
         pArtifact->GetPositiveMomentFlexuralCapacityArtifact( key ) :
         pArtifact->GetNegativeMomentFlexuralCapacityArtifact( key ) );

      if ( !pFlexure->Passed() )
         return true;
   }

   return false;
}

inline void list_momcap_failures(IBroker* pBroker,FailureList& rFailures,SpanIndexType span,GirderIndexType girder,pgsTypes::LimitState ls,const pgsGirderArtifact* pArtifact)
{
   if ( momcap_failures(pBroker,span,girder,pgsTypes::BridgeSite3,ls,pArtifact,true) )
   {
      rFailures.push_back(std::_tstring(_T("Ultimate moment capacity (positive moment) check failed for ")) + LimitStateName(ls) + std::_tstring(_T(" Limit State for the Bridge Site Stage 3.")));
   }

   if ( momcap_failures(pBroker,span,girder,pgsTypes::BridgeSite3,ls,pArtifact,false) )
   {
      rFailures.push_back(std::_tstring(_T("Ultimate moment capacity (negative moment) check failed for ")) + LimitStateName(ls) + std::_tstring(_T(" Limit State for the Bridge Site Stage 3.")));
   }
}


inline void list_vertical_shear_failures(IBroker* pBroker,FailureList& rFailures,SpanIndexType span,GirderIndexType girder,pgsTypes::LimitState ls,const pgsGirderArtifact* pArtifact)
{
   const pgsStirrupCheckArtifact *pStirrups = pArtifact->GetStirrupCheckArtifact();

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( span, girder, pgsTypes::BridgeSite3, POI_SHEAR|POI_TABULAR) );

   bool bContinue1 = true;
   bool bContinue2 = true;

   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi = *poiIter;
      const pgsStirrupCheckAtPoisArtifact* pPoiArtifacts = pStirrups->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3,ls,poi.GetDistFromStart()) );
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
}

inline void list_horizontal_shear_failures(IBroker* pBroker,FailureList& rFailures,SpanIndexType span,GirderIndexType girder,pgsTypes::LimitState ls,const pgsGirderArtifact* pArtifact)
{
   const pgsStirrupCheckArtifact *pStirrups = pArtifact->GetStirrupCheckArtifact();
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( span, girder, pgsTypes::BridgeSite3, POI_SHEAR|POI_TABULAR) );

   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi = *poiIter;
      const pgsStirrupCheckAtPoisArtifact* pPoiArtifacts = pStirrups->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3,ls,poi.GetDistFromStart()) );
      const pgsHorizontalShearArtifact* pShear = pPoiArtifacts->GetHorizontalShearArtifact();

      if ( !pShear->Passed() )
      {
         rFailures.push_back(_T("Horizontal Interface Shears/Length check failed for ") + LimitStateName(ls) + _T(" Limit State [5.8.4]."));
         return;
      }
   }
}

inline void list_stirrup_detailing_failures(IBroker* pBroker,FailureList& rFailures,SpanIndexType span,GirderIndexType girder,pgsTypes::LimitState ls,const pgsGirderArtifact* pArtifact)
{
   const pgsStirrupCheckArtifact *pStirrups = pArtifact->GetStirrupCheckArtifact();
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( span, girder, pgsTypes::BridgeSite3, POI_SHEAR|POI_TABULAR) );

   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi = *poiIter;
      const pgsStirrupCheckAtPoisArtifact* pPoiArtifacts = pStirrups->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3,ls,poi.GetDistFromStart()) );
      const pgsStirrupDetailArtifact* pShear = pPoiArtifacts->GetStirrupDetailArtifact();
 
      if ( !pShear->Passed() )
      {
         rFailures.push_back(_T("Stirrup detailing checks failed for the ") + LimitStateName(ls) + _T(" Limit State."));
         return;
      }
   }
}

inline void list_debonding_failures(IBroker* pBroker,FailureList& rFailures,SpanIndexType span,GirderIndexType girder,const pgsGirderArtifact* pArtifact)
{
   const pgsDebondArtifact* pDebond = pArtifact->GetDebondArtifact(pgsTypes::Straight);

   if ( !pDebond->Passed() )
   {
         rFailures.push_back(_T("Debond arrangement checks failed."));
   }
}

inline void list_splitting_zone_failures(IBroker* pBroker,FailureList& rFailures,SpanIndexType span,GirderIndexType girder,const pgsGirderArtifact* pArtifact)
{
   const pgsSplittingZoneArtifact* pBZArtifact = pArtifact->GetStirrupCheckArtifact()->GetSplittingZoneArtifact();
   if ( !pBZArtifact->Passed() )
   {
      if ( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() )
         rFailures.push_back(_T("Splitting zone checks failed."));
      else
         rFailures.push_back(_T("Bursting zone checks failed."));
   }
}

inline void list_confinement_zone_failures(IBroker* pBroker,FailureList& rFailures,SpanIndexType span,GirderIndexType girder,const pgsGirderArtifact* pArtifact)
{
   const pgsStirrupCheckArtifact *pStirrups = pArtifact->GetStirrupCheckArtifact();

   const pgsConfinementArtifact& rShear = pStirrups->GetConfinementArtifact();

   if ( !rShear.Passed() )
   {
      rFailures.push_back(_T("Confinement zone checks failed."));
      return;
   }
}


inline void list_various_failures(IBroker* pBroker,FailureList& rFailures,SpanIndexType span,GirderIndexType girder,const pgsGirderArtifact* pArtifact,bool referToDetails)
{
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
   const pgsLiftingCheckArtifact* pLifting = pArtifact->GetLiftingCheckArtifact();
   if (pLifting!=NULL && !pLifting->Passed() )
   {
      rFailures.push_back(_T("Lifting checks failed"));
   }

   // Hauling
   const pgsHaulingCheckArtifact* pHauling = pArtifact->GetHaulingCheckArtifact();
   if (pHauling!=NULL && !pHauling->Passed() )
   {
      rFailures.push_back(_T("Hauling checks failed"));
   }

   // Live Load Deflection
   const pgsDeflectionCheckArtifact* pDef = pArtifact->GetDeflectionCheckArtifact();
   if (pDef!=NULL && !pDef->Passed())
   {
      if (referToDetails)
         rFailures.push_back(_T("Live Load Deflection check failed. Refer to the Details or Specification Check Report for more information"));
      else
         rFailures.push_back(_T("Live Load Deflection check failed"));
   }
}


#endif // INCLUDED_PGSEXT_GIRDERARTIFACTTOOL_H_
