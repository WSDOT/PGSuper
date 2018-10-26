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
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductMomentsTable.h>

#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CProductReactionTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProductReactionTable::CProductReactionTable()
{
}

CProductReactionTable::CProductReactionTable(const CProductReactionTable& rOther)
{
   MakeCopy(rOther);
}

CProductReactionTable::~CProductReactionTable()
{
}

//======================== OPERATORS  =======================================
CProductReactionTable& CProductReactionTable::operator= (const CProductReactionTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CProductReactionTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsTypes::AnalysisType analysisType,
                                         bool bIncludeImpact, bool bIncludeLLDF,bool bDesign,bool bRating,bool bIndicateControllingLoad,IDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

   bool bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   SpanIndexType startSpan, nSpans;
   pgsTypes::Stage continuity_stage;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,span,gdr,analysisType,bDesign,bRating,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&continuity_stage,&startSpan,&nSpans);

   PierIndexType nPiers = nSpans+1;
   PierIndexType startPier = startSpan;
   PierIndexType endPier   = (span == ALL_SPANS ? nPiers : startPier+2 );
   
   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Reactions");
   RowIndexType row = ConfigureProductLoadTableHeading<rptForceUnitTag,unitmgtForceData>(p_table,true,bDeckPanels,bSidewalk,bShearKey,bDesign,bPedLoading,bPermit,bRating,analysisType,continuity_stage,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());

   // get the stage the girder dead load is applied in
   GET_IFACE2(pBroker,IProductLoads,pLoads);
   pgsTypes::Stage girderLoadStage = pLoads->GetGirderDeadLoadStage(gdr);

   GET_IFACE2(pBroker,IProductForces,pForces);
   for ( PierIndexType pier = startPier; pier < endPier; pier++ )
   {
      ColumnIndexType col = 0;

      if ( pier == 0 || pier == pBridge->GetPierCount()-1 )
         (*p_table)(row,col++) << "Abutment " << LABEL_PIER(pier);
      else
         (*p_table)(row,col++) << "Pier " << LABEL_PIER(pier);
   
      (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( girderLoadStage, pftGirder,         pier, gdr, SimpleSpan ) );
      (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftDiaphragm,      pier, gdr, SimpleSpan ) );

      if ( bShearKey )
      {
         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftShearKey, pier, gdr, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftShearKey, pier, gdr, MinSimpleContinuousEnvelope ) );
         }
         else
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftShearKey, pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         }
      }

      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      {
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftSlab,           pier, gdr, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftSlab,           pier, gdr, MinSimpleContinuousEnvelope ) );
      }
      else
      {
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftSlab,           pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan  ) );
      }

      if ( bDeckPanels )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftSlabPanel,   pier, gdr, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftSlabPanel,   pier, gdr, MinSimpleContinuousEnvelope ) );
         }
         else
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftSlabPanel,   pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         }
      }

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bSidewalk )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, pftSidewalk, pier, gdr, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, pftSidewalk, pier, gdr, MinSimpleContinuousEnvelope ) );
         }

         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, pftTrafficBarrier, pier, gdr, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, pftTrafficBarrier, pier, gdr, MinSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( overlay_stage, pftOverlay,        pier, gdr, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( overlay_stage, pftOverlay,        pier, gdr, MinSimpleContinuousEnvelope ) );

         Float64 min, max;
         long minConfig, maxConfig;

         if ( bDesign )
         {
            if ( bPedLoading )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, pier, gdr, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, pier, gdr, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, gdr, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << reaction.SetValue( max );

            if ( bIndicateControllingLoad )
            {
               (*p_table)(row,col) << rptNewLine <<  "(" << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << ")";
            }

            col++;

            pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, gdr, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig  );
            (*p_table)(row,col) << reaction.SetValue( min );

            if ( bIndicateControllingLoad )
            {
               (*p_table)(row,col) << rptNewLine <<  "(" << LiveLoadPrefix(pgsTypes::lltDesign)<< minConfig << ")";
            }

            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, pier, gdr, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine <<  "(" << LiveLoadPrefix(pgsTypes::lltFatigue) << maxConfig << ")";
               }

               col++;

               pForces->GetLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, pier, gdr, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig  );
               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine <<  "(" << LiveLoadPrefix(pgsTypes::lltFatigue) << minConfig << ")";
               }

               col++;
            }

            if ( bPermit )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, pier, gdr, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine <<  "(" << LiveLoadPrefix(pgsTypes::lltPermit) << maxConfig << ")";
               }

               col++;

               pForces->GetLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, pier, gdr, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine <<  "(" << LiveLoadPrefix(pgsTypes::lltPermit) << minConfig << ")";
               }

               col++;
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, gdr, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine <<  "(" << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << ")";
               }

               col++;

               pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, gdr, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig  );
               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine <<  "(" << LiveLoadPrefix(pgsTypes::lltDesign)<< minConfig << ")";
               }

               col++;
            }

            // Legal - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, pier, gdr, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxConfig << ")";
               }

               col++;

               pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, pier, gdr, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minConfig << ")";
               }

               col++;
            }

            // Legal - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, pier, gdr, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxConfig << ")";
               }

               col++;

               pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, pier, gdr, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << minConfig << ")";
               }

               col++;
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, pier, gdr, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxConfig << ")";
               }

               col++;

               pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, pier, gdr, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minConfig << ")";
               }

               col++;
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, pier, gdr, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxConfig << ")";
               }

               col++;

               pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, pier, gdr, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << minConfig << ")";
               }

               col++;
            }
         }
      }
      else
      {
         if ( bSidewalk )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, pftSidewalk, pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         }

         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, pftTrafficBarrier, pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( overlay_stage, pftOverlay,        pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );

         Float64 min, max;
         long minConfig, maxConfig;
         if ( bDesign )
         {
            if ( bPedLoading )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << reaction.SetValue( max );
            if ( bIndicateControllingLoad )
            {
               (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << ")";
            }

            col++;

            (*p_table)(row,col) << reaction.SetValue( min );
            if ( bIndicateControllingLoad )
            {
               (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << ")";
            }

            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );
               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltFatigue) << maxConfig << ")";
               }

               col++;

               (*p_table)(row,col) << reaction.SetValue( min );
               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltFatigue) << minConfig << ")";
               }

               col++;
            }

            if ( bPermit )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );
               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermit) << maxConfig << ")";
               }
               col++;

               (*p_table)(row,col) << reaction.SetValue( min );
               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermit)<< minConfig << ")";
               }
               col++;
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating))  )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );
               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << ")";
               }

               col++;

               (*p_table)(row,col) << reaction.SetValue( min );
               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << ")";
               }

               col++;
            }

            // Legal - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );
               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxConfig << ")";
               }
               col++;

               (*p_table)(row,col) << reaction.SetValue( min );
               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine)<< minConfig << ")";
               }
               col++;
            }

            // Legal - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );
               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxConfig << ")";
               }
               col++;

               (*p_table)(row,col) << reaction.SetValue( min );
               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltLegalRating_Special)<< minConfig << ")";
               }
               col++;
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );
               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxConfig << ")";
               }
               col++;

               (*p_table)(row,col) << reaction.SetValue( min );
               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine)<< minConfig << ")";
               }
               col++;
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );
               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxConfig << ")";
               }
               col++;

               (*p_table)(row,col) << reaction.SetValue( min );
               if ( bIndicateControllingLoad )
               {
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermitRating_Special)<< minConfig << ")";
               }
               col++;
            }
         }
      }

      row++;
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CProductReactionTable::MakeCopy(const CProductReactionTable& rOther)
{
   // Add copy code here...
}

void CProductReactionTable::MakeAssignment(const CProductReactionTable& rOther)
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
bool CProductReactionTable::AssertValid() const
{
   return true;
}

void CProductReactionTable::Dump(dbgDumpContext& os) const
{
   os << "Dump for CProductReactionTable" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CProductReactionTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CProductReactionTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CProductReactionTable");

   TESTME_EPILOG("CProductReactionTable");
}
#endif // _UNITTEST
