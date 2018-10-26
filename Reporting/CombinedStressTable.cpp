///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <Reporting\CombinedStressTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\RatingSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CCombinedStressTable
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCombinedStressTable::CCombinedStressTable()
{
}

CCombinedStressTable::CCombinedStressTable(const CCombinedStressTable& rOther)
{
   MakeCopy(rOther);
}

CCombinedStressTable::~CCombinedStressTable()
{
}

//======================== OPERATORS  =======================================
CCombinedStressTable& CCombinedStressTable::operator= (const CCombinedStressTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CCombinedStressTable::Build(IBroker* pBroker, rptChapter* pChapter,
                                         SpanIndexType span,GirderIndexType girder,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         pgsTypes::Stage stage,pgsTypes::AnalysisType analysisType,
                                         bool bDesign,bool bRating) const
{
   // NOTE - Stregth II stresses not reported because they aren't used for anything

   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),     false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);
   if ( stage == pgsTypes::CastingYard )
      location.MakeGirderPoi();
   else
      location.MakeSpanPoi();

   GET_IFACE2(pBroker,IBridge,pBridge);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table;

   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType nSpans    = (span == ALL_SPANS ? pBridge->GetSpanCount() : startSpan+1 );
 
   pgsTypes::Stage continuity_stage = pgsTypes::BridgeSite2;
   SpanIndexType spanIdx = 0;
   for ( spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      pgsTypes::Stage left_stage, right_stage;
      pBridge->GetContinuityStage(spanIdx,&left_stage,&right_stage);
      continuity_stage = _cpp_min(continuity_stage,left_stage);
      continuity_stage = _cpp_min(continuity_stage,right_stage);
   }
   // last pier
   pgsTypes::Stage left_stage, right_stage;
   pBridge->GetContinuityStage(spanIdx,&left_stage,&right_stage);
   continuity_stage = _cpp_min(continuity_stage,left_stage);
   continuity_stage = _cpp_min(continuity_stage,right_stage);

   ColumnIndexType nCols;
   ColumnIndexType col=0;

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedLoading = pProductLoads->HasPedestrianLoad(startSpan,girder);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   // Set up table headings
   if ( stage == pgsTypes::CastingYard || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
   {
      std::string strTitle(stage == pgsTypes::CastingYard     ? "Casting Yard" :
                           stage == pgsTypes::GirderPlacement ? "Girder Placement" : "Temporary Strand Removal");
      p_table = pgsReportStyleHolder::CreateDefaultTable(3,strTitle);
      (*p_table)(0,0) << COLHDR(RPT_GDR_END_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,1) << COLHDR("DC",          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,2) << COLHDR("Service I", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else if ( stage == pgsTypes::BridgeSite1)
   {
      nCols = 6;

      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         nCols += 5;

      col = 0;

      p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Deck and Diaphragm Placement (Bridge Site 1)");
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,col++) << COLHDR("DC",          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR("DW",          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << "DC",          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << "DW",          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR("Service I", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else if ( stage == pgsTypes::BridgeSite2)
   {
      nCols = 6;

      col = 0;
      p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Superimposed Dead Loads (Bridge Site 2)");
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,col++) << COLHDR("DC",          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR("DW",          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << "DC",          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << "DW",          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR("Service I", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else if ( stage == pgsTypes::BridgeSite3 )
   {
      nCols = 5;

      if ( bDesign )
      {
         nCols += 2;

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            nCols += 2;

         if ( bPedLoading )
            nCols += 2;
      }

      if ( bRating )
      {
         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            nCols += 2;
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            nCols += 2;

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            nCols += 2;
      }


      col = 0;
      ColumnIndexType col2 = 0;

      p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Final with Live Load (Bridge Site 3)");


      p_table->SetNumberOfHeaderRows(2);

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,  rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col++) << COLHDR("DC",          rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col++) << COLHDR("DW",          rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << "DC",          rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << "DW",          rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      if ( bDesign )
      {
         if ( bPedLoading )
         {
            p_table->SetColumnSpan(0,col,2);
            (*p_table)(0,col++) << "PL";
            (*p_table)(1,col2++) << COLHDR("Max",       rptStressUnitTag, pDisplayUnits->GetStressUnit() );
            (*p_table)(1,col2++) << COLHDR("Min",       rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }

         p_table->SetColumnSpan(0,col,2);
         (*p_table)(0,col++) << "*LL+IM" << rptNewLine << "Design";
         (*p_table)(1,col2++) << COLHDR("Max",       rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*p_table)(1,col2++) << COLHDR("Min",       rptStressUnitTag, pDisplayUnits->GetStressUnit() );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            p_table->SetColumnSpan(0,col,2);
            (*p_table)(0,col++) << "*LL+IM" << rptNewLine << "Fatigue";
            (*p_table)(1,col2++) << COLHDR("Max",       rptStressUnitTag, pDisplayUnits->GetStressUnit() );
            (*p_table)(1,col2++) << COLHDR("Min",       rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }
      }

      if ( bRating )
      {
         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            p_table->SetColumnSpan(0,col,2);
            (*p_table)(0,col++) << "*LL+IM" << rptNewLine << "Design";
            (*p_table)(1,col2++) << COLHDR("Max",       rptStressUnitTag, pDisplayUnits->GetStressUnit() );
            (*p_table)(1,col2++) << COLHDR("Min",       rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            p_table->SetColumnSpan(0,col,2);
            (*p_table)(0,col++) << "*LL+IM" << rptNewLine << "Legal Rating" << rptNewLine << "Routine";
            (*p_table)(1,col2++) << COLHDR("Max",       rptStressUnitTag, pDisplayUnits->GetStressUnit() );
            (*p_table)(1,col2++) << COLHDR("Min",       rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            p_table->SetColumnSpan(0,col,2);
            (*p_table)(0,col++) << "*LL+IM" << rptNewLine << "Legal Rating" << rptNewLine << "Special";
            (*p_table)(1,col2++) << COLHDR("Max",       rptStressUnitTag, pDisplayUnits->GetStressUnit() );
            (*p_table)(1,col2++) << COLHDR("Min",       rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }
      }


      for ( ColumnIndexType i = col; i < nCols; i++ )
         p_table->SetColumnSpan(0,i,-1);
   }
   else
   {
      ATLASSERT(false); // who added a new stage without telling me?
   }


   if ( span == ALL_SPANS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   *p << p_table << rptNewLine;

   if ( stage == pgsTypes::BridgeSite3 )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << p;
      *p << LIVELOAD_PER_GIRDER << rptNewLine;
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( stage, span, girder, POI_ALL, POIFIND_OR );

   GET_IFACE2(pBroker,ICombinedForces,pForces);
   GET_IFACE2(pBroker,ICombinedForces2,pForces2);
   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);
   GET_IFACE2(pBroker,ILimitStateForces2,pLsForces2);

   BridgeAnalysisType bat;
   if (analysisType == pgsTypes::Simple)
      bat = SimpleSpan;
   else if ( analysisType == pgsTypes::Continuous )
      bat = ContinuousSpan;
   else
      bat = MaxSimpleContinuousEnvelope;

   std::vector<Float64> fTopDCinc, fBotDCinc;
   std::vector<Float64> fTopDWinc, fBotDWinc;
   std::vector<Float64> fTopDCcum, fBotDCcum;
   std::vector<Float64> fTopDWcum, fBotDWcum;
   std::vector<Float64> fTopMinServiceI, fBotMinServiceI;
   std::vector<Float64> fTopMaxServiceI, fBotMaxServiceI;
   std::vector<Float64> fTopMinPedestrianLL, fBotMinPedestrianLL;
   std::vector<Float64> fTopMaxPedestrianLL, fBotMaxPedestrianLL;
   std::vector<Float64> fTopMinDesignLL, fBotMinDesignLL;
   std::vector<Float64> fTopMaxDesignLL, fBotMaxDesignLL;
   std::vector<Float64> fTopMinFatigueLL, fBotMinFatigueLL;
   std::vector<Float64> fTopMaxFatigueLL, fBotMaxFatigueLL;
   std::vector<Float64> fTopMinLegalRoutineLL, fBotMinLegalRoutineLL;
   std::vector<Float64> fTopMaxLegalRoutineLL, fBotMaxLegalRoutineLL;
   std::vector<Float64> fTopMinLegalSpecialLL, fBotMinLegalSpecialLL;
   std::vector<Float64> fTopMaxLegalSpecialLL, fBotMaxLegalSpecialLL;

   if ( stage == pgsTypes::CastingYard || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
   {
      pForces2->GetStress(lcDC,stage,vPoi, ctIncremental, SimpleSpan, &fTopDCinc, &fBotDCinc);
      pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::TopGirder,    false, SimpleSpan, &fTopMinServiceI, &fTopMaxServiceI);
      pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::BottomGirder, false, SimpleSpan, &fBotMinServiceI, &fBotMaxServiceI);
   }
   else if ( stage == pgsTypes::BridgeSite1 )
   {
      pForces2->GetStress( lcDC, stage, vPoi, ctIncremental, bat, &fTopDCinc, &fBotDCinc);
      pForces2->GetStress( bRating ? lcDWRating : lcDW, stage, vPoi, ctIncremental, bat, &fTopDWinc, &fBotDWinc);
      pForces2->GetStress( lcDC, stage, vPoi, ctCummulative, bat, &fTopDCcum, &fBotDCcum);
      pForces2->GetStress( bRating ? lcDWRating : lcDW, stage, vPoi, ctCummulative, bat, &fTopDWcum, &fBotDWcum);

      pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceI,&fTopMaxServiceI);
      pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceI,&fBotMaxServiceI);
   }
   else if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3)
   {
      pForces2->GetStress( lcDC, stage, vPoi, ctIncremental, bat, &fTopDCinc, &fBotDCinc);
      pForces2->GetStress( bRating ? lcDWRating : lcDW, stage, vPoi, ctIncremental, bat, &fTopDWinc, &fBotDWinc);
      pForces2->GetStress( lcDC, stage, vPoi, ctCummulative, bat, &fTopDCcum, &fBotDCcum);
      pForces2->GetStress( bRating ? lcDWRating : lcDW, stage, vPoi, ctCummulative, bat, &fTopDWcum, &fBotDWcum);

      if ( stage == pgsTypes::BridgeSite2 )
      {
         pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceI,&fTopMaxServiceI);
         pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceI,&fBotMaxServiceI);
      }
      else
      {
         // Bridge site 3
         if ( bDesign )
         {
            if ( bPedLoading )
            {
               pForces2->GetCombinedLiveLoadStress( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, bat, &fTopMinPedestrianLL, &fTopMaxPedestrianLL, &fBotMinPedestrianLL, &fBotMaxPedestrianLL );
            }

            pForces2->GetCombinedLiveLoadStress( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, bat, &fTopMinDesignLL, &fTopMaxDesignLL, &fBotMinDesignLL, &fBotMaxDesignLL );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces2->GetCombinedLiveLoadStress( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, bat, &fTopMinFatigueLL, &fTopMaxFatigueLL, &fBotMinFatigueLL, &fBotMaxFatigueLL );
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces2->GetCombinedLiveLoadStress( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, bat, &fTopMinDesignLL, &fTopMaxDesignLL, &fBotMinDesignLL, &fBotMaxDesignLL );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces2->GetCombinedLiveLoadStress( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, vPoi, bat, &fTopMinLegalRoutineLL, &fTopMaxLegalRoutineLL, &fBotMinLegalRoutineLL, &fBotMaxLegalRoutineLL );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces2->GetCombinedLiveLoadStress( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, vPoi, bat, &fTopMinLegalSpecialLL, &fTopMaxLegalSpecialLL, &fBotMinLegalSpecialLL, &fBotMaxLegalSpecialLL );
            }
         }
      }
   }

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   long index = 0;
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
   {
      const pgsPointOfInterest& poi = *i;

      col = 0;

      Float64 end_size = 0 ;
      if ( stage != pgsTypes::CastingYard )
         end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

      (*p_table)(row,col++) << location.SetValue( poi, end_size );
      if ( stage == pgsTypes::CastingYard || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
      {
         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDCinc[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDCinc[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinServiceI[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxServiceI[index]);
      }
      else if ( stage == pgsTypes::BridgeSite1 )
      {
         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDCinc[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDCinc[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDWinc[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDWinc[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDCcum[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDCcum[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDWcum[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDWcum[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinServiceI[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxServiceI[index]);
      }
      else if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3)
      {
         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDCinc[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDCinc[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDWinc[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDWinc[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDCcum[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDCcum[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDWcum[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDWcum[index]);


         if ( stage == pgsTypes::BridgeSite2 )
         {
            (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinServiceI[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxServiceI[index]);
         }
         else
         {
            if ( bDesign )
            {
               if ( bPedLoading )
               {
                  (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMaxPedestrianLL[index]) << rptNewLine;
                  (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxPedestrianLL[index]);

                  (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinPedestrianLL[index]) << rptNewLine;
                  (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMinPedestrianLL[index]);
               }

               (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMaxDesignLL[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxDesignLL[index]);

               (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinDesignLL[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMinDesignLL[index]);

               if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
               {
                  (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMaxFatigueLL[index]) << rptNewLine;
                  (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxFatigueLL[index]);

                  (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinFatigueLL[index]) << rptNewLine;
                  (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMinFatigueLL[index]);
               }
            }

            if ( bRating )
            {
               if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
               {
                  (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMaxDesignLL[index]) << rptNewLine;
                  (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxDesignLL[index]);

                  (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinDesignLL[index]) << rptNewLine;
                  (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMinDesignLL[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               {
                  (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMaxLegalRoutineLL[index]) << rptNewLine;
                  (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxLegalRoutineLL[index]);

                  (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinLegalRoutineLL[index]) << rptNewLine;
                  (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMinLegalRoutineLL[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               {
                  (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMaxLegalSpecialLL[index]) << rptNewLine;
                  (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxLegalSpecialLL[index]);

                  (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinLegalSpecialLL[index]) << rptNewLine;
                  (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMinLegalSpecialLL[index]);
               }
             }
         }
      }

      row++;
   }

   // create second table for BSS3 Limit states
   if ( stage == pgsTypes::BridgeSite3 )
   {
      p = new rptParagraph;
      *pChapter << p;

      p = new rptParagraph;
      *pChapter << p;

      ColumnIndexType nCols = 1; 
      if ( bDesign )
      {
         nCols += 3;
      }

      if ( bRating )
      {
         if ( !bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            nCols++;

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            nCols++;

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            nCols++;
      }

      p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"");

      *p << p_table;

      p_table->SetNumberOfHeaderRows(2);

      ColumnIndexType col1 = 0;
      ColumnIndexType col2 = 0;
      p_table->SetRowSpan(0,col1,2);
      (*p_table)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      p_table->SetRowSpan(1,col2++,-1);

      if ( bDesign )
      {
         p_table->SetColumnSpan(0,col1,3);
         (*p_table)(0,col1++) << "Design";
         p_table->SetColumnSpan(0,col1++,-1);
         p_table->SetColumnSpan(0,col1++,-1);

         (*p_table)(1,col2++) << COLHDR("Service I",   rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      
         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
            (*p_table)(1,col2++) << COLHDR("Service IA",  rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         
         (*p_table)(1,col2++) << COLHDR("Service III", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      
         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            (*p_table)(1,col2++) << COLHDR("Fatigue I",  rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }

      if ( bRating )
      {
         p_table->SetColumnSpan(0,col1,(!bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) ? 3 : 2));
         (*p_table)(0,col1++) << "Rating";
         p_table->SetColumnSpan(0,col1++,-1);
   
         if ( !bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            p_table->SetColumnSpan(0,col1++,-1);

         if ( !bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
         {
            (*p_table)(1,col2++) << COLHDR("Service III" << rptNewLine << "Inventory", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            (*p_table)(1,col2++) << COLHDR("Service III" << rptNewLine << "Legal Routine", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            (*p_table)(1,col2++) << COLHDR("Service III" << rptNewLine << "Legal Special", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }
      }

      std::vector<Float64> fTopMinServiceI,   fBotMinServiceI;
      std::vector<Float64> fTopMaxServiceI,   fBotMaxServiceI;
      std::vector<Float64> fTopMinServiceIA,  fBotMinServiceIA;
      std::vector<Float64> fTopMaxServiceIA,  fBotMaxServiceIA;
      std::vector<Float64> fTopMinServiceIII, fBotMinServiceIII;
      std::vector<Float64> fTopMaxServiceIII, fBotMaxServiceIII;
      std::vector<Float64> fTopMinFatigueI,   fBotMinFatigueI;
      std::vector<Float64> fTopMaxFatigueI,   fBotMaxFatigueI;
      std::vector<Float64> fTopMinServiceIII_Inventory, fBotMinServiceIII_Inventory;
      std::vector<Float64> fTopMaxServiceIII_Inventory, fBotMaxServiceIII_Inventory;
      std::vector<Float64> fTopMinServiceIII_Routine,   fBotMinServiceIII_Routine;
      std::vector<Float64> fTopMaxServiceIII_Routine,   fBotMaxServiceIII_Routine;
      std::vector<Float64> fTopMinServiceIII_Special,   fBotMinServiceIII_Special;
      std::vector<Float64> fTopMaxServiceIII_Special,   fBotMaxServiceIII_Special;

      if ( bDesign )
      {
         pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceI, &fTopMaxServiceI);
         pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceI, &fBotMaxServiceI);

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            pLsForces2->GetStress( pgsTypes::ServiceIA, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceIA, &fTopMaxServiceIA);
            pLsForces2->GetStress( pgsTypes::ServiceIA, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceIA, &fBotMaxServiceIA);
         }
         else
         {
            pLsForces2->GetStress( pgsTypes::FatigueI, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinFatigueI, &fTopMaxFatigueI);
            pLsForces2->GetStress( pgsTypes::FatigueI, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinFatigueI, &fBotMaxFatigueI);
         }

         pLsForces2->GetStress( pgsTypes::ServiceIII, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceIII, &fTopMaxServiceIII);
         pLsForces2->GetStress( pgsTypes::ServiceIII, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceIII, &fBotMaxServiceIII);
      }

      if ( bRating )
      {
         if ( !bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
         {
            pLsForces2->GetStress( pgsTypes::ServiceIII_Inventory, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceIII_Inventory, &fTopMaxServiceIII_Inventory);
            pLsForces2->GetStress( pgsTypes::ServiceIII_Inventory, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceIII_Inventory, &fBotMaxServiceIII_Inventory);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pLsForces2->GetStress( pgsTypes::ServiceIII_LegalRoutine, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceIII_Routine, &fTopMaxServiceIII_Routine);
            pLsForces2->GetStress( pgsTypes::ServiceIII_LegalRoutine, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceIII_Routine, &fBotMaxServiceIII_Routine);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            pLsForces2->GetStress( pgsTypes::ServiceIII_LegalSpecial, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceIII_Special, &fTopMaxServiceIII_Special);
            pLsForces2->GetStress( pgsTypes::ServiceIII_LegalSpecial, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceIII_Special, &fBotMaxServiceIII_Special);
         }
      }

      RowIndexType row = p_table->GetNumberOfHeaderRows();
   
      std::vector<pgsPointOfInterest>::const_iterator i;
      long index = 0;
      for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
      {
         col = 0;
         const pgsPointOfInterest& poi = *i;
         Float64 end_size = 0 ;
         if ( stage != pgsTypes::CastingYard )
            end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

         (*p_table)(row,col++) << location.SetValue( poi, end_size );

         if ( bDesign )
         {
            (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinServiceI[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxServiceI[index]);

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
            {
               (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinServiceIA[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxServiceIA[index]);
            }

            (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinServiceIII[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxServiceIII[index]);

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinFatigueI[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxFatigueI[index]);
            }
         }

         if ( bRating )
         {
            if ( !bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            {
               (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinServiceIII_Inventory[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxServiceIII_Inventory[index]);
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinServiceIII_Routine[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxServiceIII_Routine[index]);
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinServiceIII_Special[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxServiceIII_Special[index]);
            }
         }

         row++;
      }
   }
}

void CCombinedStressTable::MakeCopy(const CCombinedStressTable& rOther)
{
   // Add copy code here...
}

void CCombinedStressTable::MakeAssignment(const CCombinedStressTable& rOther)
{
   MakeCopy( rOther );
}
