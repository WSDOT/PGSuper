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
#include <Reporting\CombinedReactionTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\ReactionInterfaceAdapters.h>

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
   CCombinedReactionTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCombinedReactionTable::CCombinedReactionTable()
{
}

CCombinedReactionTable::CCombinedReactionTable(const CCombinedReactionTable& rOther)
{
   MakeCopy(rOther);
}

CCombinedReactionTable::~CCombinedReactionTable()
{
}

//======================== OPERATORS  =======================================
CCombinedReactionTable& CCombinedReactionTable::operator= (const CCombinedReactionTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CCombinedReactionTable::Build(IBroker* pBroker, rptChapter* pChapter,
                                          SpanIndexType span,GirderIndexType girder, 
                                          IEAFDisplayUnits* pDisplayUnits,
                                          pgsTypes::Stage stage, pgsTypes::AnalysisType analysisType,TableType tableType,
                                          bool bDesign,bool bRating) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);


   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table=0;

   PierIndexType nPiers = pBridge->GetPierCount();

   PierIndexType startPier = (span == ALL_SPANS ? 0 : span);
   PierIndexType endPier   = (span == ALL_SPANS ? nPiers : startPier+2 );

   pgsTypes::Stage continuity_stage = pgsTypes::BridgeSite2;
   PierIndexType pier = 0;
   for ( pier = startPier; pier < endPier; pier++ )
   {
      pgsTypes::Stage left_stage, right_stage;
      pBridge->GetContinuityStage(pier,&left_stage,&right_stage);
      continuity_stage = _cpp_min(continuity_stage,left_stage);
      continuity_stage = _cpp_min(continuity_stage,right_stage);
   }

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedLoading = pProductLoads->HasPedestrianLoad(startPier,girder);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   RowIndexType row = CreateCombinedLoadingTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table,
                               (tableType==PierReactionsTable ? _T("Total Girderline Reactions at Abutments and Piers"): _T("Girder Bearing Reactions")),
                               true,bDesign,bPermit,bPedLoading,bRating,stage,continuity_stage,analysisType,pRatingSpec,pDisplayUnits,
                               pDisplayUnits->GetShearUnit());
   *p << p_table;

   // TRICKY:
   // Use the adapter class to get the reaction response functions we need
   GET_IFACE2(pBroker,ICombinedForces,pCmbForces);
   GET_IFACE2(pBroker,ILimitStateForces,pILsForces);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);

   std::auto_ptr<ICmbLsReactionAdapter> pForces;
   if(  tableType==PierReactionsTable )
   {
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CombinedLsForcesReactionAdapter(pCmbForces,pILsForces));
   }

   else
   {
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CmbLsBearingDesignReactionAdapter(pBearingDesign, span) );
   }

   // Fill up the table
   Float64 min, max;
   for ( pier = startPier; pier < endPier; pier++ )
   {
      if (! pForces->DoReportAtPier(pier, girder) )
      {
         continue; // don't report piers that have no bearing information
      }

      if (pier == 0 || pier == pBridge->GetPierCount()-1 )
         (*p_table)(row,0) << _T("Abutment ") << LABEL_PIER(pier);
      else
         (*p_table)(row,0) << _T("Pier ") << LABEL_PIER(pier);

      if ( stage == pgsTypes::CastingYard || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
      {
         (*p_table)(row,1) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, SimpleSpan ) );
         pForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, SimpleSpan, true, &min, &max );
         (*p_table)(row,2) << reaction.SetValue( max );
      }
      else if ( stage == pgsTypes::BridgeSite1 )
      {
         int col = 1;
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctIncremental, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctCummulative, MinSimpleContinuousEnvelope ) );

            pForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, MinSimpleContinuousEnvelope,true,  &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );
         }
         else
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, SimpleSpan ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctIncremental, SimpleSpan ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, SimpleSpan ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctCummulative, SimpleSpan ) );

            pForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, SimpleSpan, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
         }
      }
      else if ( stage == pgsTypes::BridgeSite2 )
      {
         int col = 1;

         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctIncremental, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctCummulative, MinSimpleContinuousEnvelope ) );

            pForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );
         }
         else
         {
            BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctIncremental, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctCummulative, bat ) );

            pForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, bat, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
         }

      }
      else if ( stage == pgsTypes::BridgeSite3 )
      {
         int col = 1;

         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctIncremental, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctCummulative, MinSimpleContinuousEnvelope ) );

            if ( bDesign )
            {
               if ( bPedLoading )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( bPermit )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }
            }

            if ( bRating )
            {
               if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }
 
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }
 
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }
 
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }
            }
         }
         else
         {
            BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctIncremental, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctCummulative, bat ) );

            if ( bDesign )
            {
               if ( bPedLoading )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, bat, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( bPermit )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }
            }

            if ( bRating )
            {
               if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
               {
                  pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }
            }
         }
      }

      row++;
   }

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
      p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,(tableType==PierReactionsTable ? _T("Total Girderline Reactions at Abutments and Piers"): _T("Girder Bearing Reactions")));
      row = ConfigureLimitStateTableHeading<rptForceUnitTag,unitmgtForceData>(p_table,true,bDesign,bPermit,bRating,false,analysisType,pStageMap,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());
      *p << p_table;

      ColumnIndexType col = 0;
      (*p_table)(0,col++) << _T("");


      for ( PierIndexType pier = startPier; pier < endPier; pier++ )
      {
         if (! pForces->DoReportAtPier(pier, girder) )
         {
            continue; // don't report piers that have no bearing information
         }

         col = 0;
         if (pier == 0 || pier == nPiers-1 )
            (*p_table)(row,col++) << _T("Abutment ") << LABEL_PIER(pier);
         else
            (*p_table)(row,col++) << _T("Pier ") << LABEL_PIER(pier);

         if ( analysisType == pgsTypes::Envelope )
         {
            if ( bDesign )
            {
               pForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
               {
                  pForces->GetReaction( pgsTypes::ServiceIA, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetReaction( pgsTypes::ServiceIA, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               pForces->GetReaction( pgsTypes::ServiceIII, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::ServiceIII, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if ( lrfdVersionMgr::FourthEditionWith2009Interims  <= lrfdVersionMgr::GetVersion() )
               {
                  pForces->GetReaction( pgsTypes::FatigueI, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetReaction( pgsTypes::FatigueI, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               pForces->GetReaction( pgsTypes::StrengthI, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthI, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if ( bPermit )
               {
                  pForces->GetReaction( pgsTypes::StrengthII, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetReaction( pgsTypes::StrengthII, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }
            }

            if ( bRating )
            {
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
               {
                  pForces->GetReaction( pgsTypes::StrengthI_Inventory, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetReaction( pgsTypes::StrengthI_Inventory, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
               {
                  pForces->GetReaction( pgsTypes::StrengthI_Operating, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetReaction( pgsTypes::StrengthI_Operating, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               {
                  pForces->GetReaction( pgsTypes::StrengthI_LegalRoutine, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetReaction( pgsTypes::StrengthI_LegalRoutine, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
               {
                  pForces->GetReaction( pgsTypes::StrengthI_LegalSpecial, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetReaction( pgsTypes::StrengthI_LegalSpecial, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
               {
                  pForces->GetReaction( pgsTypes::ServiceI_PermitRoutine, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetReaction( pgsTypes::ServiceI_PermitRoutine, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );

                  pForces->GetReaction( pgsTypes::StrengthII_PermitRoutine, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetReaction( pgsTypes::StrengthII_PermitRoutine, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
               {
                  pForces->GetReaction( pgsTypes::ServiceI_PermitSpecial, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetReaction( pgsTypes::ServiceI_PermitSpecial, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );

                  pForces->GetReaction( pgsTypes::StrengthII_PermitSpecial, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );

                  pForces->GetReaction( pgsTypes::StrengthII_PermitSpecial, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }
            }
         }
         else
         {
            BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);

            if ( bDesign )
            {
               pForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, bat, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
               {
                  pForces->GetReaction( pgsTypes::ServiceIA, stage, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
               }

               pForces->GetReaction( pgsTypes::ServiceIII, stage, pier, girder, bat, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
               {
                  pForces->GetReaction( pgsTypes::FatigueI, stage, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
               }

               pForces->GetReaction( pgsTypes::StrengthI, stage, pier, girder, bat, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if ( bPermit )
               {
                  pForces->GetReaction( pgsTypes::StrengthII, stage, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }
            }

            if ( bRating )
            {
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
               {
                  pForces->GetReaction( pgsTypes::StrengthI_Inventory, stage, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
               {
                  pForces->GetReaction( pgsTypes::StrengthI_Operating, stage, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               {
                  pForces->GetReaction( pgsTypes::StrengthI_LegalRoutine, stage, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
               {
                  pForces->GetReaction( pgsTypes::StrengthI_LegalSpecial, stage, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
               {
                  pForces->GetReaction( pgsTypes::ServiceI_PermitRoutine, stage, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );

                  pForces->GetReaction( pgsTypes::StrengthII_PermitRoutine, stage, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
               {
                  pForces->GetReaction( pgsTypes::ServiceI_PermitSpecial, stage, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );

                  pForces->GetReaction( pgsTypes::StrengthII_PermitSpecial, stage, pier, girder, bat, true, &min, &max );
                  (*p_table)(row,col++) << reaction.SetValue( max );
                  (*p_table)(row,col++) << reaction.SetValue( min );
               }
            }
         }

         row++;
      }
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CCombinedReactionTable::MakeCopy(const CCombinedReactionTable& rOther)
{
   // Add copy code here...
}

void CCombinedReactionTable::MakeAssignment(const CCombinedReactionTable& rOther)
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
bool CCombinedReactionTable::AssertValid() const
{
   return true;
}

void CCombinedReactionTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CCombinedReactionTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CCombinedReactionTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CCombinedReactionTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CCombinedReactionTable");

   TESTME_EPILOG("CCombinedReactionTable");
}
#endif // _UNITTEST
