///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

/****************************************************************************
CLASS
   CLongReinfShearCheck
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CLongReinfShearCheck::CLongReinfShearCheck()
{
}

CLongReinfShearCheck::CLongReinfShearCheck(const CLongReinfShearCheck& rOther)
{
   MakeCopy(rOther);
}

CLongReinfShearCheck::~CLongReinfShearCheck()
{
}

//======================== OPERATORS  =======================================
CLongReinfShearCheck& CLongReinfShearCheck::operator= (const CLongReinfShearCheck& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CLongReinfShearCheck::Build(rptChapter* pChapter,
                              IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                              IntervalIndexType intervalIdx,pgsTypes::LimitState ls,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear,  pDisplayUnits->GetShearUnit(), false );

   rptCapacityToDemand cap_demand;

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   *pTitle << _T("Longitudinal Reinforcement for Shear Check - ") << pProductLoads->GetLimitStateName(ls) << _T(" [5.8.3.5]");

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);

   if ( 0 < nDucts )
   {
      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         *pBody <<rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LongitudinalReinforcementForShear2005_with_PT.png"))<<rptNewLine;
      }
      else
      {
         *pBody <<rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LongitudinalReinforcementForShear_with_PT.png"))<<rptNewLine;
      }
   }
   else
   {
      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         *pBody <<rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LongitudinalReinforcementForShear2005.png"))<<rptNewLine;
      }
      else
      {
         *pBody <<rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LongitudinalReinforcementForShear.png"))<<rptNewLine;
      }
   }

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(5,_T(""));
   *pBody << table;

   (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,1)  << COLHDR(_T("Capacity"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,2)  << COLHDR(_T("Demand"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,3)  << _T("Equation");
   (*table)(0,4)  << _T("Status") << rptNewLine << _T("(C/D)");

   // Fill up the table
   bool bAddFootnote = false;

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact* pStirrupArtifact= pSegmentArtifact->GetStirrupCheckArtifact();
      ATLASSERT(pStirrupArtifact);

      CollectionIndexType nArtifacts = pStirrupArtifact->GetStirrupCheckAtPoisArtifactCount(intervalIdx,ls);
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pStirrupArtifact->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );
         if ( psArtifact == NULL )
         {
            continue;
         }

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();

         const pgsLongReinfShearArtifact* pArtifact = psArtifact->GetLongReinfShearArtifact();

         if ( pArtifact->IsApplicable() )
         {
            (*table)(row,0) << location.SetValue( POI_SPAN, poi );

            Float64 C = pArtifact->GetCapacityForce();
            Float64 D = pArtifact->GetDemandForce();
            (*table)(row,1) << shear.SetValue( C );
            (*table)(row,2) << shear.SetValue( D );

            (*table)(row,3) << _T("5.8.3.5-") << pArtifact->GetEquation();

            bool bPassed = pArtifact->Passed();
            if ( bPassed )
            {
               (*table)(row,4) << RPT_PASS;
            }
            else
            {
               (*table)(row,4) << RPT_FAIL;
            }

            Float64 ratio = IsZero(D) ? DBL_MAX : C/D;
            if ( bPassed && fabs(pArtifact->GetMu()) <= fabs(pArtifact->GetMr()) && ratio < 1.0 )
            {
               bAddFootnote = true;
               (*table)(row,4) << _T("*");
            }

            (*table)(row,4) << rptNewLine << _T("(") << cap_demand.SetValue(C,D,bPassed) << _T(")");

            row++;
         }
      }  // next artifact
   } // next segment

   if ( bAddFootnote )
   {
      rptParagraph* pFootnote = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
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

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   *pTitle << _T("Longitudinal Reinforcement for Shear Check - ") << pProductLoads->GetLimitStateName(ls) << _T(" [5.8.3.5]");

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      *pBody <<rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LongitudinalReinforcementForShear2005.png"))<<rptNewLine;
   }
   else
   {
      *pBody <<rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LongitudinalReinforcementForShear.png"))<<rptNewLine;
   }

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(5,_T(""));
   *pBody << table;

   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR(_T("Capacity"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,2)  << COLHDR(_T("Demand"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,3)  << _T("Equation");
   (*table)(0,4)  << _T("Status") << rptNewLine << _T("(C/D)");

   // Fill up the table
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   const pgsRatingArtifact* pRatingArtifact = pIArtifact->GetRatingArtifact(girderKey,ratingType,INVALID_INDEX);
   pgsRatingArtifact::ShearRatings shearRatings = pRatingArtifact->GetShearRatings();

   bool bAddFootnote = false;

   RowIndexType row = table->GetNumberOfHeaderRows();

   pgsRatingArtifact::ShearRatings::iterator i(shearRatings.begin());
   pgsRatingArtifact::ShearRatings::iterator end(shearRatings.end());
   for ( ; i != end; i++ )
   {
      pgsPointOfInterest& poi = i->first;
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      pgsShearRatingArtifact& shearRatingArtifact = i->second;
      const pgsLongReinfShearArtifact& artifact = shearRatingArtifact.GetLongReinfShearArtifact();

      Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

      if ( artifact.IsApplicable() )
      {
         (*table)(row,0) << location.SetValue( POI_SPAN, poi );

         Float64 C = artifact.GetCapacityForce();
         Float64 D = artifact.GetDemandForce();
         (*table)(row,1) << shear.SetValue( C );
         (*table)(row,2) << shear.SetValue( D );

         (*table)(row,3) << _T("5.8.3.5-") << artifact.GetEquation();

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
      rptParagraph* pFootnote = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pFootnote;

      *pFootnote << _T("* The area of longitudinal reinforcement on the flexural tension side of the member need not exceed the area required to resist the maximum moment acting alone") << rptNewLine;
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CLongReinfShearCheck::MakeCopy(const CLongReinfShearCheck& rOther)
{
   // Add copy code here...
}

void CLongReinfShearCheck::MakeAssignment(const CLongReinfShearCheck& rOther)
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
bool CLongReinfShearCheck::AssertValid() const
{
   return true;
}

void CLongReinfShearCheck::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CLongReinfShearCheck") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CLongReinfShearCheck::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CLongReinfShearCheck");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CLongReinfShearCheck");

   TESTME_EPILOG("LiveLoadDistributionFactorTable");
}
#endif // _UNITTEST
