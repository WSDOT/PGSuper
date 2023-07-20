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

#include "StdAfx.h"

#include <Reporting\LongReinfShearCheck.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Artifact.h>
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>


#include <PgsExt\GirderArtifact.h>
#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\CapacityToDemand.h>
#include <PgsExt\RatingArtifact.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <Reporter\ReportingUtils.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void CLongReinfShearCheck::Build(rptChapter* pChapter,
                              IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                              IntervalIndexType intervalIdx,pgsTypes::LimitState ls,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear,  pDisplayUnits->GetShearUnit(), false );

   rptCapacityToDemand cap_demand;

   GET_IFACE2(pBroker, IBridge, pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   bool bUHPC = false;
   GET_IFACE2(pBroker, IMaterials, pMaterials);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      if (pMaterials->GetSegmentConcreteType(CSegmentKey(girderKey, segIdx)) == pgsTypes::UHPC)
      {
         bUHPC = true;
         break;
      }
   }


   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Longitudinal Reinforcement for Shear Check - ") << GetLimitStateString(ls) << _T(" [");
   if (bUHPC)
   {
      *pTitle << _T("GS 1.7.3.5");
   }
   else
   {
      *pTitle << WBFL::LRFD::LrfdCw8th(_T("5.8.3.5"), _T("5.7.3.5"));
   }
   *pTitle << _T("]");

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   GET_IFACE2(pBroker,IGirderTendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);

   WBFL::LRFD::LRFDVersionMgr::Version vers = WBFL::LRFD::LRFDVersionMgr::GetVersion();

   if (bUHPC)
   {
      *pBody << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("UHPC_LongitudinalReinforcementForShear.png")) << rptNewLine;
   }
   else
   {
      if (0 < nDucts)
      {
         if (WBFL::LRFD::LRFDVersionMgr::Version::EighthEdition2017 <= vers)
         {
            *pBody << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear2017_with_PT.png")) << rptNewLine;
         }
         else if (WBFL::LRFD::LRFDVersionMgr::Version::ThirdEditionWith2005Interims <= vers)
         {
            *pBody << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear2005_with_PT.png")) << rptNewLine;
         }
         else
         {
            *pBody << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear_with_PT.png")) << rptNewLine;
         }
      }
      else
      {
         if (WBFL::LRFD::LRFDVersionMgr::Version::EighthEdition2017 <= vers)
         {
            *pBody << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear2017.png")) << rptNewLine;
         }
         else if (WBFL::LRFD::LRFDVersionMgr::Version::ThirdEditionWith2005Interims <= vers)
         {
            *pBody << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear2005.png")) << rptNewLine;
         }
         else
         {
            *pBody << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear.png")) << rptNewLine;
         }
      }

      if (WBFL::LRFD::LRFDVersionMgr::Version::NinthEdition2020 <= vers)
      {
         *pBody << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LRS_ReinforcementLimit.png")) << rptNewLine;
      }
   }

   rptRcTable* table = rptStyleManager::CreateDefaultTable(vers < WBFL::LRFD::LRFDVersionMgr::Version::NinthEdition2020 ? 5 : 8,_T(""));
   *pBody << table;

   ColumnIndexType col = 0;
   (*table)(0, col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0, col++)  << COLHDR(_T("Capacity"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0, col++)  << COLHDR(_T("Demand"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0, col++)  << _T("Equation");
   (*table)(0, col++)  << _T("Status") << rptNewLine << _T("(C/D)");
   
   if (WBFL::LRFD::LRFDVersionMgr::Version::NinthEdition2020 <= vers)
   {
      (*table)(0, col++) << COLHDR(RPT_APS << RPT_FPS, rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      if (bUHPC)
      {
         (*table)(0, col++) << COLHDR(RPT_AS << RPT_ES << Sub2(symbol(gamma),_T("u")) << Sub2(symbol(epsilon), _T("t,loc")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      }
      else
      {
         (*table)(0, col++) << COLHDR(RPT_AS << RPT_FY, rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      }
      (*table)(0, col++) << _T("Status");
   }

   // Fill up the table
   bool bAddFootnote = false;

   RowIndexType row = table->GetNumberOfHeaderRows();

   location.IncludeSpanAndGirder(1 < nSegments);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact* pStirrupArtifact= pSegmentArtifact->GetStirrupCheckArtifact();
      ATLASSERT(pStirrupArtifact);

      IndexType nArtifacts = pStirrupArtifact->GetStirrupCheckAtPoisArtifactCount(intervalIdx,ls);
      for ( IndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pStirrupArtifact->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );
         if ( psArtifact == nullptr )
         {
            continue;
         }

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();

         const pgsLongReinfShearArtifact* pArtifact = psArtifact->GetLongReinfShearArtifact();

         if ( pArtifact->IsApplicable() )
         {
            col = 0;
            (*table)(row, col++) << location.SetValue( POI_SPAN, poi );

            Float64 C = pArtifact->GetCapacityForce();
            Float64 D = pArtifact->GetDemandForce();
            (*table)(row, col++) << shear.SetValue( C );
            (*table)(row, col++) << shear.SetValue( D );

            if (pArtifact->IsUHPC())
            {
               (*table)(row, col++) << _T("1.7.3.5-") << pArtifact->GetEquation();
            }
            else
            {
               (*table)(row, col++) << WBFL::LRFD::LrfdCw8th(_T("5.8.3.5-"), _T("5.7.3.5-")) << pArtifact->GetEquation();
            }

            bool bPassed = pArtifact->PassedCapacity();
            if ( bPassed )
            {
               (*table)(row, col) << RPT_PASS;
            }
            else
            {
               (*table)(row, col) << RPT_FAIL;
            }

            Float64 ratio = IsZero(D) ? DBL_MAX : C/D;
            if ( bPassed && fabs(pArtifact->GetMu()) <= fabs(pArtifact->GetMr()) && ratio < 1.0 )
            {
               bAddFootnote = true;
               (*table)(row, col) << _T("*");
            }

            (*table)(row, col) << rptNewLine << _T("(") << cap_demand.SetValue(C,D,bPassed) << _T(")");
            col++;

            if (WBFL::LRFD::LRFDVersionMgr::Version::NinthEdition2020 <= vers)
            {
               (*table)(row, col++) << shear.SetValue(pArtifact->GetPretensionForce());
               (*table)(row, col++) << shear.SetValue(pArtifact->GetRebarForce());
               if (pArtifact->PretensionForceMustExceedBarForce())
               {
                  if (pArtifact->PassedPretensionForce())
                  {
                     (*table)(row, col++) << RPT_PASS;
                  }
                  else
                  {
                     (*table)(row, col++) << RPT_FAIL;
                  }
               }
               else
               {
                  (*table)(row, col++) << RPT_NA;
               }
            }

            row++;
         }
      }  // next artifact
   } // next segment

   if ( bAddFootnote )
   {
      rptParagraph* pFootnote = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pFootnote;

      *pFootnote << _T("* The area of longitudinal reinforcement on the flexural tension side of the member need not exceed the area required to resist the maximum moment acting alone") << rptNewLine;
   }
}

void CLongReinfShearCheck::Build(rptChapter* pChapter,
                              IBroker* pBroker,const CGirderKey& girderKey,
                              pgsTypes::LimitState ls,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   location.IncludeSpanAndGirder(true);

   INIT_UV_PROTOTYPE( rptForceSectionValue, shear,  pDisplayUnits->GetShearUnit(), false );

   rptCapacityToDemand cap_demand;

   pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(ls);

   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;

   *pTitle << _T("Longitudinal Reinforcement for Shear Check - ") << GetLimitStateString(ls) << _T(" [") << WBFL::LRFD::LrfdCw8th(_T("5.8.3.5"),_T("5.7.3.5")) << _T("]");

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   if ( WBFL::LRFD::LRFDVersionMgr::Version::ThirdEditionWith2005Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion() )
   {
      *pBody <<rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear2005.png"))<<rptNewLine;
   }
   else
   {
      *pBody <<rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear.png"))<<rptNewLine;
   }

   rptRcTable* table = rptStyleManager::CreateDefaultTable(5,_T(""));
   *pBody << table;

   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR(_T("Capacity"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,2)  << COLHDR(_T("Demand"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,3)  << _T("Equation");
   (*table)(0,4)  << _T("Status") << rptNewLine << _T("(C/D)");

   // Fill up the table
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   const pgsRatingArtifact* pRatingArtifact = pIArtifact->GetRatingArtifact(girderKey,ratingType,INVALID_INDEX);
   const pgsRatingArtifact::LongitudinalReinforcementForShear& longReinfShear = pRatingArtifact->GetLongitudinalReinforcementForShear();

   bool bAddFootnote = false;

   RowIndexType row = table->GetNumberOfHeaderRows();

   for( const auto& item : longReinfShear)
   {
      const pgsPointOfInterest& poi = item.first;
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      const auto& artifact = item.second;

      Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

      if ( artifact.IsApplicable() )
      {
         (*table)(row,0) << location.SetValue( POI_SPAN, poi );

         Float64 C = artifact.GetCapacityForce();
         Float64 D = artifact.GetDemandForce();
         (*table)(row,1) << shear.SetValue( C );
         (*table)(row,2) << shear.SetValue( D );

         (*table)(row,3) << WBFL::LRFD::LrfdCw8th(_T("5.8.3.5-"),_T("5.7.3.5-")) << artifact.GetEquation();

         bool bPassed = artifact.Passed();
         if ( bPassed )
         {
            (*table)(row,4) << RPT_PASS;
         }
         else
         {
            (*table)(row,4) << RPT_FAIL;
         }

         Float64 ratio = IsZero(D) ? DBL_MAX : C/D;
         if ( artifact.Passed() && fabs(artifact.GetMu()) <= fabs(artifact.GetMr()) && ratio < 1.0 )
         {
            bAddFootnote = true;
            (*table)(row,4) << _T("*");
         }

         (*table)(row,4) << rptNewLine << _T("(") << cap_demand.SetValue(C,D,bPassed) << _T(")");

         row++;
      }
   }

   if ( bAddFootnote )
   {
      rptParagraph* pFootnote = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pFootnote;

      *pFootnote << _T("* The area of longitudinal reinforcement on the flexural tension side of the member need not exceed the area required to resist the maximum moment acting alone") << rptNewLine;
   }
}
