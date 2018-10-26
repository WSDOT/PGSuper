///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
#include <Reporting\CombinedShearTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\MVRChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>

#include <IFace\Project.h>
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
   CCombinedShearTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCombinedShearTable::CCombinedShearTable()
{
}

CCombinedShearTable::CCombinedShearTable(const CCombinedShearTable& rOther)
{
   MakeCopy(rOther);
}

CCombinedShearTable::~CCombinedShearTable()
{
}

//======================== OPERATORS  =======================================
CCombinedShearTable& CCombinedShearTable::operator= (const CCombinedShearTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CCombinedShearTable::Build(IBroker* pBroker,rptChapter* pChapter,
                                       SpanIndexType span,GirderIndexType girder,
                                       IEAFDisplayUnits* pDisplayUnits,
                                       pgsTypes::Stage stage,pgsTypes::AnalysisType analysisType,
                                       bool bDesign,bool bRating) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear, pDisplayUnits->GetShearUnit(), false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = 0;

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

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedLoading = pProductLoads->HasPedestrianLoad(startSpan,girder);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   ColumnIndexType col = 0;
   RowIndexType row = CreateCombinedLoadingTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table,_T("Shear"),false,bDesign,bPermit,bPedLoading,bRating,stage,continuity_stage,analysisType,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());

   if ( span == ALL_SPANS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   *p << p_table;

   RowIndexType row2 = 1;
   rptRcTable* p_table2 = 0;
   if ( stage == pgsTypes::BridgeSite3 )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << p;
      *p << LIVELOAD_PER_GIRDER << rptNewLine;

      p = new rptParagraph;
      *pChapter << p;

      p = new rptParagraph;
      *pChapter << p;

      ColumnIndexType nCols = 1;

      if ( bDesign )
      {
         nCols += 5; // Service I, Service IA or Fatigue I, Strength I min/max

         if ( analysisType == pgsTypes::Envelope )
            nCols += 3; // min/max for Service I, Service III, ServiceIA/FatigueI

         if ( bPermit )
            nCols += 2; // Strength II min/max
      }

      if ( bRating )
      {
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            nCols += 2;

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
            nCols += 2;
      
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            nCols += 2;

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            nCols += 2;

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            nCols += 4;

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            nCols += 4;
      }

      GET_IFACE2(pBroker,IStageMap,pStageMap);
      p_table2 = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Shear"));
      row2 = ConfigureLimitStateTableHeading<rptForceUnitTag,unitmgtForceData>(p_table2,false,bDesign,bPermit,bRating,false,analysisType,pStageMap,pRatingSpec,pDisplayUnits,pDisplayUnits->GetGeneralForceUnit());
      *p << p_table2;
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   GET_IFACE2(pBroker,ICombinedForces,pForces);
   GET_IFACE2(pBroker,ICombinedForces2,pForces2);
   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);
   GET_IFACE2(pBroker,ILimitStateForces2,pLsForces2);

   // Fill up the table
   for ( spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( spanIdx, girder, stage, POI_ALL, POIFIND_OR);

      std::vector<sysSectionValue> dummy;
      std::vector<sysSectionValue> minServiceI, maxServiceI;
      std::vector<sysSectionValue> minDCinc, maxDCinc;
      std::vector<sysSectionValue> minDCcum, maxDCcum;
      std::vector<sysSectionValue> minDWinc, maxDWinc;
      std::vector<sysSectionValue> minDWcum, maxDWcum;
      std::vector<sysSectionValue> minPedestrianLL, maxPedestrianLL;
      std::vector<sysSectionValue> minDesignLL, maxDesignLL;
      std::vector<sysSectionValue> minFatigueLL, maxFatigueLL;
      std::vector<sysSectionValue> minPermitLL, maxPermitLL;
      std::vector<sysSectionValue> minLegalRoutineLL, maxLegalRoutineLL;
      std::vector<sysSectionValue> minLegalSpecialLL, maxLegalSpecialLL;
      std::vector<sysSectionValue> minPermitRoutineLL, maxPermitRoutineLL;
      std::vector<sysSectionValue> minPermitSpecialLL, maxPermitSpecialLL;

      if ( stage == pgsTypes::CastingYard || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
      {
         maxDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, SimpleSpan );
         pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, SimpleSpan, &dummy, &maxServiceI );
      }
      else if ( stage == pgsTypes::BridgeSite1 )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            maxDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, MaxSimpleContinuousEnvelope );
            minDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, MinSimpleContinuousEnvelope );
            maxDWinc = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctIncremental, MaxSimpleContinuousEnvelope );
            minDWinc = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctIncremental, MinSimpleContinuousEnvelope );
            maxDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, MaxSimpleContinuousEnvelope );
            minDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, MinSimpleContinuousEnvelope );
            maxDWcum = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctCummulative, MaxSimpleContinuousEnvelope );
            minDWcum = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctCummulative, MinSimpleContinuousEnvelope );

            pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxServiceI );
            pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, MinSimpleContinuousEnvelope, &minServiceI, &dummy );
         }
         else
         {
            maxDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, SimpleSpan );
            maxDWinc = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctIncremental, SimpleSpan );
            maxDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, SimpleSpan );
            maxDWcum = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctCummulative, SimpleSpan );
            pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, SimpleSpan, &minServiceI, &maxServiceI );
         }
      }
      else if ( stage == pgsTypes::BridgeSite2 )
      {
         if ( analysisType == pgsTypes::Envelope )
         {
            maxDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, MaxSimpleContinuousEnvelope );
            minDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, MinSimpleContinuousEnvelope );
            maxDWinc = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctIncremental, MaxSimpleContinuousEnvelope );
            minDWinc = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctIncremental, MinSimpleContinuousEnvelope );
            maxDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, MaxSimpleContinuousEnvelope );
            minDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, MinSimpleContinuousEnvelope );
            maxDWcum = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctCummulative, MaxSimpleContinuousEnvelope );
            minDWcum = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctCummulative, MinSimpleContinuousEnvelope );

            pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxServiceI );
            pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, MinSimpleContinuousEnvelope, &minServiceI, &dummy );
         }
         else
         {
            BridgeAnalysisType bat = ( analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
            maxDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, bat );
            maxDWinc = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctIncremental, bat );
            maxDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, bat );
            maxDWcum = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctCummulative, bat );

            pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, bat, &minServiceI, &maxServiceI );
         }
      }
      else
      {
         // stage == pgsTypes::BridgeSite3
         if ( analysisType == pgsTypes::Envelope )
         {
            maxDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, MaxSimpleContinuousEnvelope );
            minDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, MinSimpleContinuousEnvelope );
            maxDWinc = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctIncremental, MaxSimpleContinuousEnvelope );
            minDWinc = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctIncremental, MinSimpleContinuousEnvelope );
            maxDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, MaxSimpleContinuousEnvelope );
            minDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, MinSimpleContinuousEnvelope );
            maxDWcum = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctCummulative, MaxSimpleContinuousEnvelope );
            minDWcum = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctCummulative, MinSimpleContinuousEnvelope );

            if ( bDesign )
            {
               if ( bPedLoading )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxPedestrianLL );
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, &minPedestrianLL, &dummy );
               }

               pForces2->GetCombinedLiveLoadShear( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxDesignLL );
               pForces2->GetCombinedLiveLoadShear( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, &minDesignLL, &dummy );

               if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxFatigueLL );
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, &minFatigueLL, &dummy );
               }

               if ( bPermit )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxPermitLL );
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, &minPermitLL, &dummy );
               }
            }

            if ( bRating )
            {
               if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory)) )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxDesignLL );
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, &minDesignLL, &dummy );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxLegalRoutineLL );
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, &minLegalRoutineLL, &dummy );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxLegalSpecialLL );
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, &minLegalSpecialLL, &dummy );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxPermitRoutineLL );
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, &minPermitRoutineLL, &dummy );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxPermitSpecialLL );
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, &minPermitSpecialLL, &dummy );
               }
            }
         }
         else
         {
            BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
            maxDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, bat );
            maxDWinc = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctIncremental, bat );
            maxDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, bat );
            maxDWcum = pForces2->GetShear( bRating ? lcDWRating : lcDW, stage, vPoi, ctCummulative, bat );

            if ( bDesign )
            {
               if ( bPedLoading )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, bat, &minPedestrianLL, &maxPedestrianLL );
               }

               pForces2->GetCombinedLiveLoadShear( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, bat, &minDesignLL, &maxDesignLL );

               if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, bat, &minFatigueLL, &maxFatigueLL );
               }

               if ( bPermit )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, bat, &minPermitLL, &maxPermitLL );
               }
            }

            if ( bRating )
            {
               if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, bat, &minDesignLL, &maxDesignLL );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, vPoi, bat, &minLegalRoutineLL, &maxLegalRoutineLL );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, vPoi, bat, &minLegalSpecialLL, &maxLegalSpecialLL );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, vPoi, bat, &minPermitRoutineLL, &maxPermitRoutineLL );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
               {
                  pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, vPoi, bat, &minPermitSpecialLL, &maxPermitSpecialLL );
               }
            }
         }
      }



      std::vector<pgsPointOfInterest>::const_iterator i;
      long index = 0;
      for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
      {
         const pgsPointOfInterest& poi = *i;

         col = 0;

         Float64 end_size = 0 ;
         if ( stage != pgsTypes::CastingYard )
            end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

         (*p_table)(row,col++) << location.SetValue( stage, poi, end_size );

         if ( stage == pgsTypes::CastingYard  || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
         {
            (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
            (*p_table)(row,col++) << shear.SetValue( maxServiceI[index] );
         }
         else if ( stage == pgsTypes::BridgeSite1 )
         {
            if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
            {
               (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minDWcum[index] );

               (*p_table)(row,col++) << shear.SetValue( maxServiceI[index] );
               (*p_table)(row,col++) << shear.SetValue( minServiceI[index] );
            }
            else
            {
               (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWcum[index] );

               (*p_table)(row,col++) << shear.SetValue( maxServiceI[index] );
            }
         }
         else if ( stage == pgsTypes::BridgeSite2 )
         {
            if ( analysisType == pgsTypes::Envelope )
            {
               (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minDWcum[index] );

               (*p_table)(row,col++) << shear.SetValue( maxServiceI[index] );
               (*p_table)(row,col++) << shear.SetValue( minServiceI[index] );
            }
            else
            {
               (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWcum[index] );

               (*p_table)(row,col++) << shear.SetValue( maxServiceI[index] );
            }

         }
         else
         {
            // stage == pgsTypes::BridgeSite3
            if ( analysisType == pgsTypes::Envelope )
            {
               (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minDWcum[index] );
            }
            else
            {
               (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWcum[index] );
            }

            if ( bDesign )
            {
               if ( bPedLoading )
               {
                  (*p_table)(row,col++) << shear.SetValue( maxPedestrianLL[index] );
                  (*p_table)(row,col++) << shear.SetValue( minPedestrianLL[index] );
               }

               (*p_table)(row,col++) << shear.SetValue( maxDesignLL[index] );
               (*p_table)(row,col++) << shear.SetValue( minDesignLL[index] );
            
               if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
               {
                  (*p_table)(row,col++) << shear.SetValue( maxFatigueLL[index] );
                  (*p_table)(row,col++) << shear.SetValue( minFatigueLL[index] );
               }

               if ( bPermit )
               {
                  (*p_table)(row,col++) << shear.SetValue( maxPermitLL[index] );
                  (*p_table)(row,col++) << shear.SetValue( minPermitLL[index] );
               }
            }

            if ( bRating )
            {
               if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
               {
                  (*p_table)(row,col++) << shear.SetValue( maxDesignLL[index] );
                  (*p_table)(row,col++) << shear.SetValue( minDesignLL[index] );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               {
                  (*p_table)(row,col++) << shear.SetValue( maxLegalRoutineLL[index] );
                  (*p_table)(row,col++) << shear.SetValue( minLegalRoutineLL[index] );
               }


               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
               {
                  (*p_table)(row,col++) << shear.SetValue( maxLegalSpecialLL[index] );
                  (*p_table)(row,col++) << shear.SetValue( minLegalSpecialLL[index] );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
               {
                  (*p_table)(row,col++) << shear.SetValue( maxPermitRoutineLL[index] );
                  (*p_table)(row,col++) << shear.SetValue( minPermitRoutineLL[index] );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
               {
                  (*p_table)(row,col++) << shear.SetValue( maxPermitSpecialLL[index] );
                  (*p_table)(row,col++) << shear.SetValue( minPermitSpecialLL[index] );
               }
            }
         }

         row++;
      }

      // create second table for BSS3 Limit states
      if ( stage == pgsTypes::BridgeSite3 )
      {
         std::vector<sysSectionValue> dummy;
         std::vector<sysSectionValue> minServiceI,   maxServiceI;
         std::vector<sysSectionValue> minServiceIA,  maxServiceIA;
         std::vector<sysSectionValue> minServiceIII, maxServiceIII;
         std::vector<sysSectionValue> minFatigueI,   maxFatigueI;
         std::vector<sysSectionValue> minStrengthI,  maxStrengthI;
         std::vector<sysSectionValue> minStrengthII, maxStrengthII;
         std::vector<sysSectionValue> minStrengthI_Inventory, maxStrengthI_Inventory;
         std::vector<sysSectionValue> minStrengthI_Operating, maxStrengthI_Operating;
         std::vector<sysSectionValue> minStrengthI_Legal_Routine, maxStrengthI_Legal_Routine;
         std::vector<sysSectionValue> minStrengthI_Legal_Special, maxStrengthI_Legal_Special;
         std::vector<sysSectionValue> minStrengthII_Permit_Routine, maxStrengthII_Permit_Routine;
         std::vector<sysSectionValue> minServiceI_Permit_Routine, maxServiceI_Permit_Routine;
         std::vector<sysSectionValue> minStrengthII_Permit_Special, maxStrengthII_Permit_Special;
         std::vector<sysSectionValue> minServiceI_Permit_Special, maxServiceI_Permit_Special;

         if ( analysisType == pgsTypes::Envelope )
         {
            if ( bDesign )
            {
               pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxServiceI );
               pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, MinSimpleContinuousEnvelope, &minServiceI, &dummy );

               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
               {
                  pLsForces2->GetShear( pgsTypes::ServiceIA, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxServiceIA );
                  pLsForces2->GetShear( pgsTypes::ServiceIA, stage, vPoi, MinSimpleContinuousEnvelope, &minServiceIA, &dummy );
               }
               else
               {
                  pLsForces2->GetShear( pgsTypes::FatigueI, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxFatigueI );
                  pLsForces2->GetShear( pgsTypes::FatigueI, stage, vPoi, MinSimpleContinuousEnvelope, &minFatigueI, &dummy );
               }

               pLsForces2->GetShear( pgsTypes::ServiceIII, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxServiceIII );
               pLsForces2->GetShear( pgsTypes::ServiceIII, stage, vPoi, MinSimpleContinuousEnvelope, &minServiceIII, &dummy );

               pLsForces2->GetShear( pgsTypes::StrengthI, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxStrengthI );
               pLsForces2->GetShear( pgsTypes::StrengthI, stage, vPoi, MinSimpleContinuousEnvelope, &minStrengthI, &dummy );

               if ( bPermit )
               {
                  pLsForces2->GetShear( pgsTypes::StrengthII, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxStrengthII );
                  pLsForces2->GetShear( pgsTypes::StrengthII, stage, vPoi, MinSimpleContinuousEnvelope, &minStrengthII, &dummy );
               }
            }

            if ( bRating )
            {
               // Design - Inventory
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
               {
                  pLsForces2->GetShear( pgsTypes::StrengthI_Inventory, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxStrengthI_Inventory );
                  pLsForces2->GetShear( pgsTypes::StrengthI_Inventory, stage, vPoi, MinSimpleContinuousEnvelope, &minStrengthI_Inventory, &dummy );
               }

               // Design - Operating
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
               {
                  pLsForces2->GetShear( pgsTypes::StrengthI_Operating, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxStrengthI_Operating );
                  pLsForces2->GetShear( pgsTypes::StrengthI_Operating, stage, vPoi, MinSimpleContinuousEnvelope, &minStrengthI_Operating, &dummy );
               }

               // Legal - Routine
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               {
                  pLsForces2->GetShear( pgsTypes::StrengthI_LegalRoutine, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxStrengthI_Legal_Routine );
                  pLsForces2->GetShear( pgsTypes::StrengthI_LegalRoutine, stage, vPoi, MinSimpleContinuousEnvelope, &minStrengthI_Legal_Routine, &dummy );
               }

               // Legal - Special
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
               {
                  pLsForces2->GetShear( pgsTypes::StrengthI_LegalSpecial, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxStrengthI_Legal_Special );
                  pLsForces2->GetShear( pgsTypes::StrengthI_LegalSpecial, stage, vPoi, MinSimpleContinuousEnvelope, &minStrengthI_Legal_Special, &dummy );
               }

               // Permit Rating - Routine
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
               {
                  pLsForces2->GetShear( pgsTypes::StrengthII_PermitRoutine, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxStrengthII_Permit_Routine );
                  pLsForces2->GetShear( pgsTypes::StrengthII_PermitRoutine, stage, vPoi, MinSimpleContinuousEnvelope, &minStrengthII_Permit_Routine, &dummy );
                  pLsForces2->GetShear( pgsTypes::ServiceI_PermitRoutine,   stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxServiceI_Permit_Routine );
                  pLsForces2->GetShear( pgsTypes::ServiceI_PermitRoutine,   stage, vPoi, MinSimpleContinuousEnvelope, &minServiceI_Permit_Routine, &dummy );
               }

               // Permit Rating - Special
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
               {
                  pLsForces2->GetShear( pgsTypes::StrengthII_PermitSpecial, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxStrengthII_Permit_Special );
                  pLsForces2->GetShear( pgsTypes::StrengthII_PermitSpecial, stage, vPoi, MinSimpleContinuousEnvelope, &minStrengthII_Permit_Special, &dummy );
                  pLsForces2->GetShear( pgsTypes::ServiceI_PermitSpecial,   stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxServiceI_Permit_Special );
                  pLsForces2->GetShear( pgsTypes::ServiceI_PermitSpecial,   stage, vPoi, MinSimpleContinuousEnvelope, &minServiceI_Permit_Special, &dummy );
               }
            }
         }
         else
         {
            BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);

            if ( bDesign )
            {
               pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, bat, &minServiceI, &maxServiceI );

               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
                  pLsForces2->GetShear( pgsTypes::ServiceIA, stage, vPoi, bat, &minServiceIA, &maxServiceIA );
               else
                  pLsForces2->GetShear( pgsTypes::FatigueI, stage, vPoi, bat, &minFatigueI, &maxFatigueI );

               pLsForces2->GetShear( pgsTypes::ServiceIII, stage, vPoi, bat, &minServiceIII, &maxServiceIII );
               pLsForces2->GetShear( pgsTypes::StrengthI, stage, vPoi, bat, &minStrengthI, &maxStrengthI );

               if ( bPermit )
               {
                  pLsForces2->GetShear( pgsTypes::StrengthII, stage, vPoi, bat, &minStrengthII, &maxStrengthII );
               }
            }

            if ( bRating )
            {
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
               {
                  pLsForces2->GetShear( pgsTypes::StrengthI_Inventory,  stage, vPoi, bat, &minStrengthI_Inventory, &maxStrengthI_Inventory );
               }
 
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
               {
                  pLsForces2->GetShear( pgsTypes::StrengthI_Operating,  stage, vPoi, bat, &minStrengthI_Operating, &maxStrengthI_Operating );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               {
                  pLsForces2->GetShear( pgsTypes::StrengthI_LegalRoutine,  stage, vPoi, bat, &minStrengthI_Legal_Routine,  &maxStrengthI_Legal_Routine );
               }
 
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
               {
                  pLsForces2->GetShear( pgsTypes::StrengthI_LegalSpecial,  stage, vPoi, bat, &minStrengthI_Legal_Special,  &maxStrengthI_Legal_Special );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
               {
                  pLsForces2->GetShear( pgsTypes::ServiceI_PermitRoutine,   stage, vPoi, bat, &minServiceI_Permit_Routine,   &maxServiceI_Permit_Routine );
                  pLsForces2->GetShear( pgsTypes::StrengthII_PermitRoutine, stage, vPoi, bat, &minStrengthII_Permit_Routine, &maxStrengthII_Permit_Routine );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
               {
                  pLsForces2->GetShear( pgsTypes::ServiceI_PermitSpecial,   stage, vPoi, bat, &minServiceI_Permit_Special,   &maxServiceI_Permit_Special );
                  pLsForces2->GetShear( pgsTypes::StrengthII_PermitSpecial, stage, vPoi, bat, &minStrengthII_Permit_Special, &maxStrengthII_Permit_Special );
               }
            }
         }

         std::vector<pgsPointOfInterest>::const_iterator i;
         long index = 0;
         for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
         {
            col = 0;

            const pgsPointOfInterest& poi = *i;

            Float64 end_size = 0 ;
            if ( stage != pgsTypes::CastingYard )
               end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

            (*p_table2)(row2,col++) << location.SetValue( stage, poi, end_size );

            if ( analysisType == pgsTypes::Envelope )
            {
               if ( bDesign )
               {
                  (*p_table2)(row2,col++) << shear.SetValue( maxServiceI[index] );
                  (*p_table2)(row2,col++) << shear.SetValue( minServiceI[index] );

                  if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue( maxServiceIA[index] );
                     (*p_table2)(row2,col++) << shear.SetValue( minServiceIA[index] );
                  }

                  (*p_table2)(row2,col++) << shear.SetValue( maxServiceIII[index] );
                  (*p_table2)(row2,col++) << shear.SetValue( minServiceIII[index] );

                  if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue( maxFatigueI[index] );
                     (*p_table2)(row2,col++) << shear.SetValue( minFatigueI[index] );
                  }

                  (*p_table2)(row2,col++) << shear.SetValue( maxStrengthI[index] );
                  (*p_table2)(row2,col++) << shear.SetValue( minStrengthI[index] );

                  if ( bPermit )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue( maxStrengthII[index] );
                     (*p_table2)(row2,col++) << shear.SetValue( minStrengthII[index] );
                  }
               }

               if ( bRating )
               {
                  if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue(maxStrengthI_Inventory[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minStrengthI_Inventory[index]);
                  }

                  if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue(maxStrengthI_Operating[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minStrengthI_Operating[index]);
                  }

                  if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue(maxStrengthI_Legal_Routine[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minStrengthI_Legal_Routine[index]);
                  }

                  if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue(maxStrengthI_Legal_Special[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minStrengthI_Legal_Special[index]);
                  }

                  if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue(maxStrengthII_Permit_Routine[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minStrengthII_Permit_Routine[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(maxServiceI_Permit_Routine[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minServiceI_Permit_Routine[index]);
                  }

                  if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue(maxStrengthII_Permit_Special[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minStrengthII_Permit_Special[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(maxServiceI_Permit_Special[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minServiceI_Permit_Special[index]);
                  }
               }
            }
            else
            {
               if ( bDesign )
               {
                  (*p_table2)(row2,col++) << shear.SetValue( maxServiceI[index] );

                  if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
                     (*p_table2)(row2,col++) << shear.SetValue( maxServiceIA[index] );

                  (*p_table2)(row2,col++) << shear.SetValue( maxServiceIII[index] );

                  if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
                     (*p_table2)(row2,col++) << shear.SetValue( maxFatigueI[index] );

                  (*p_table2)(row2,col++) << shear.SetValue( maxStrengthI[index] );
                  (*p_table2)(row2,col++) << shear.SetValue( minStrengthI[index] );

                  if ( bPermit )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue( maxStrengthII[index] );
                     (*p_table2)(row2,col++) << shear.SetValue( minStrengthII[index] );
                  }
               }

               if ( bRating )
               {
                  if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue(maxStrengthI_Inventory[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minStrengthI_Inventory[index]);
                  }

                  if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue(maxStrengthI_Operating[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minStrengthI_Operating[index]);
                  }

                  if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue(maxStrengthI_Legal_Routine[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minStrengthI_Legal_Routine[index]);
                  }

                  if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue(maxStrengthI_Legal_Special[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minStrengthI_Legal_Special[index]);
                  }

                  if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue(maxStrengthII_Permit_Routine[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minStrengthII_Permit_Routine[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(maxServiceI_Permit_Routine[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minServiceI_Permit_Routine[index]);
                  }

                  if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
                  {
                     (*p_table2)(row2,col++) << shear.SetValue(maxStrengthII_Permit_Special[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minStrengthII_Permit_Special[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(maxServiceI_Permit_Special[index]);
                     (*p_table2)(row2,col++) << shear.SetValue(minServiceI_Permit_Special[index]);
                  }
               }
            }

            row2++;
         }
      }
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CCombinedShearTable::MakeCopy(const CCombinedShearTable& rOther)
{
   // Add copy code here...
}

void CCombinedShearTable::MakeAssignment(const CCombinedShearTable& rOther)
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
bool CCombinedShearTable::AssertValid() const
{
   return true;
}

void CCombinedShearTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CCombinedShearTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CCombinedShearTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CCombinedShearTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CCombinedShearTable");

   TESTME_EPILOG("CCombinedShearTable");
}
#endif // _UNITTEST
