///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <IFace\Artifact.h>
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\PointOfInterest.h>
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
                              IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                              pgsTypes::Stage stage,pgsTypes::LimitState ls,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   USES_CONVERSION;

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear,  pDisplayUnits->GetShearUnit(), false );

   rptCapacityToDemand cap_demand;

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   *pTitle << _T("Longitudinal Reinforcement for Shear Check - ") << OLE2T(pStageMap->GetLimitStateName(ls)) << _T(" [5.8.3.5]");

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      *pBody <<rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LongitudinalReinforcementForShear2005.png"))<<rptNewLine;
   else
      *pBody <<rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LongitudinalReinforcementForShear.png"))<<rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(5,_T(""));
   *pBody << table;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR(_T("Capacity"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,2)  << COLHDR(_T("Demand"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,3)  << _T("Equation");
   (*table)(0,4)  << _T("Status") << rptNewLine << _T("(C/D)");

   // Fill up the table
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsStirrupCheckArtifact* pstirrup_artifact= gdrArtifact->GetStirrupCheckArtifact();
   CHECK(pstirrup_artifact);

   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( span, girder, stage, POI_TABULAR|POI_SHEAR );

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   bool bAddFootnote = false;

   RowIndexType row = table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      const pgsLongReinfShearArtifact* pArtifact = psArtifact->GetLongReinfShearArtifact();

      if ( pArtifact->IsApplicable() )
      {
         (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );

         Float64 C = pArtifact->GetCapacityForce();
         Float64 D = pArtifact->GetDemandForce();
         (*table)(row,1) << shear.SetValue( C );
         (*table)(row,2) << shear.SetValue( D );

         (*table)(row,3) << _T("5.8.3.5-") << pArtifact->GetEquation();

         bool bPassed = pArtifact->Passed();
         if ( bPassed )
            (*table)(row,4) << RPT_PASS;
         else
            (*table)(row,4) << RPT_FAIL;

         Float64 ratio = IsZero(D) ? DBL_MAX : C/D;
         if ( bPassed && fabs(pArtifact->GetMu()) <= fabs(pArtifact->GetMr()) && ratio < 1.0 )
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

void CLongReinfShearCheck::Build(rptChapter* pChapter,
                              IBroker* pBroker,GirderIndexType gdrLineIdx,
                              pgsTypes::LimitState ls,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   USES_CONVERSION;

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   location.IncludeSpanAndGirder(true);

   INIT_UV_PROTOTYPE( rptForceSectionValue, shear,  pDisplayUnits->GetShearUnit(), false );

   rptCapacityToDemand cap_demand;

   pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(ls);

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   *pTitle << _T("Longitudinal Reinforcement for Shear Check - ") << OLE2T(pStageMap->GetLimitStateName(ls)) << _T(" [5.8.3.5]");

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      *pBody <<rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LongitudinalReinforcementForShear2005.png"))<<rptNewLine;
   else
      *pBody <<rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LongitudinalReinforcementForShear.png"))<<rptNewLine;

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
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   const pgsRatingArtifact* pRatingArtifact = pIArtifact->GetRatingArtifact(gdrLineIdx,ratingType,INVALID_INDEX);
   pgsRatingArtifact::ShearRatings shearRatings = pRatingArtifact->GetShearRatings();

   bool bAddFootnote = false;

   RowIndexType row = table->GetNumberOfHeaderRows();

   pgsRatingArtifact::ShearRatings::iterator i;
   for ( i = shearRatings.begin(); i != shearRatings.end(); i++ )
   {
      pgsPointOfInterest& poi = i->first;
      pgsShearRatingArtifact& shearRatingArtifact = i->second;
      const pgsLongReinfShearArtifact& artifact = shearRatingArtifact.GetLongReinfShearArtifact();

      Float64 end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

      if ( artifact.IsApplicable() )
      {
         (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );

         Float64 C = artifact.GetCapacityForce();
         Float64 D = artifact.GetDemandForce();
         (*table)(row,1) << shear.SetValue( C );
         (*table)(row,2) << shear.SetValue( D );

         (*table)(row,3) << _T("5.8.3.5-") << artifact.GetEquation();

         bool bPassed = artifact.Passed();
         if ( bPassed )
            (*table)(row,4) << RPT_PASS;
         else
            (*table)(row,4) << RPT_FAIL;

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
