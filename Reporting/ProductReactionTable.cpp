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
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductMomentsTable.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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
                                         ReactionTableType tableType, bool bIncludeImpact, bool bIncludeLLDF,bool bDesign,bool bRating,bool bIndicateControllingLoad,
                                         IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bFutureOverlay = pBridge->IsFutureOverlay();
   pgsTypes::Stage overlay_stage = pgsTypes::BridgeSite2;

   bool bConstruction, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   SpanIndexType startSpan, nSpans;
   pgsTypes::Stage continuity_stage;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,span,gdr,analysisType,bDesign,bRating,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&continuity_stage,&startSpan,&nSpans);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,
                         tableType==PierReactionsTable ?_T("Total Girderline Reactions at Abutments and Piers"): _T("Girder Bearing Reactions") );
   RowIndexType row = ConfigureProductLoadTableHeading<rptForceUnitTag,unitmgtForceData>(p_table,true,false,bConstruction,bDeckPanels,bSidewalk,bShearKey,bFutureOverlay,bDesign,bPedLoading,bPermit,bRating,analysisType,continuity_stage,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());

   // get the stage the girder dead load is applied in
   GET_IFACE2(pBroker,IProductLoads,pLoads);
   pgsTypes::Stage girderLoadStage = pLoads->GetGirderDeadLoadStage(gdr);

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);

   // TRICKY: use adapter class to get correct reaction interfaces
   std::auto_ptr<IProductReactionAdapter> pForces;
   if( tableType==PierReactionsTable )
   {
      pForces =  std::auto_ptr<ProductForcesReactionAdapter>(new ProductForcesReactionAdapter(pProductForces, span, gdr));
   }
   else
   {
      pForces =  std::auto_ptr<BearingDesignProductReactionAdapter>(new BearingDesignProductReactionAdapter(pBearingDesign, pgsTypes::GirderPlacement, span, gdr) );
   }

   // Use iterator to walk locations
   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      ColumnIndexType col = 0;

      const ReactionLocation& rct_locn = iter.CurrentItem();

     (*p_table)(row,col++) << rct_locn.PierLabel;
   
      (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( girderLoadStage, rct_locn, pftGirder, SimpleSpan ) );

      // Use reaction decider tool to determine when to report stages
      ReactionDecider rctdr(tableType, rct_locn, pBridge);

      if ( rctdr.DoReport(pgsTypes::BridgeSite1) )
      {
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftDiaphragm, SimpleSpan ) );
      }
      else
      {
         (*p_table)(row,col++) << RPT_NA;
      }

      if ( bShearKey )
      {
         if ( analysisType == pgsTypes::Envelope )
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftShearKey, MaxSimpleContinuousEnvelope ) );
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftShearKey, MinSimpleContinuousEnvelope ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }
         else
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftShearKey, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }
      }

      if ( bConstruction )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftConstruction, MaxSimpleContinuousEnvelope ) );
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftConstruction, MinSimpleContinuousEnvelope ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }
         else
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftConstruction, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }
      }

      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      {
         if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftSlab, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftSlab, MinSimpleContinuousEnvelope ) );

            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftSlabPad, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftSlabPad, MinSimpleContinuousEnvelope ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;

            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }
      }
      else
      {
         if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftSlab,    analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan  ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftSlabPad, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan  ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }
      }

      if ( bDeckPanels )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftSlabPanel, MaxSimpleContinuousEnvelope ) );
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftSlabPanel, MinSimpleContinuousEnvelope ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }
         else
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftSlabPanel, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }
      }

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bSidewalk )
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite2 ))
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, rct_locn, pftSidewalk, MaxSimpleContinuousEnvelope ) );
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, rct_locn, pftSidewalk, MinSimpleContinuousEnvelope ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         if (rctdr.DoReport(pgsTypes::BridgeSite2 ))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, rct_locn, pftTrafficBarrier, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, rct_locn, pftTrafficBarrier, MinSimpleContinuousEnvelope ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         if (rctdr.DoReport(overlay_stage ))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( overlay_stage, rct_locn, !bDesign ? pftOverlayRating : pftOverlay, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( overlay_stage, rct_locn, !bDesign ? pftOverlayRating : pftOverlay, MinSimpleContinuousEnvelope ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         Float64 min, max;
         VehicleIndexType minConfig, maxConfig;

         if ( bPedLoading )
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, bIncludeImpact, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }

            if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, bIncludeImpact, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         if ( bDesign )
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
               }
            }
            else
            {
               (*p_table)(row,col) << RPT_NA;
            }

            col++;

            if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig  );
               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltDesign)<< minConfig << _T(")");
               }
            }
            else
            {
               (*p_table)(row,col) << RPT_NA;
            }

            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );

                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;

               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig  );
                  (*p_table)(row,col) << reaction.SetValue( min );

                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;
            }

            if ( bPermit )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );

                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;

               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( min );

                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );

                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;

               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig  );
                  (*p_table)(row,col) << reaction.SetValue( min );

                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltDesign)<< minConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;
            }

            // Legal - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );

                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;

               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( min );

                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;
            }

            // Legal - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );

                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;

               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( min );

                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << minConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );

                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;

               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( min );

                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );

                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;

               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( min );

                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << minConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;
            }
         }
      }
      else
      {
         if ( bSidewalk )
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite2 ))
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, rct_locn, pftSidewalk, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         if (rctdr.DoReport(pgsTypes::BridgeSite2 ))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, rct_locn, pftTrafficBarrier, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
         }

         if (rctdr.DoReport(overlay_stage ))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( overlay_stage, rct_locn, !bDesign ? pftOverlayRating : pftOverlay, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
         }

         Float64 min, max;
         VehicleIndexType minConfig, maxConfig;

         if ( bPedLoading )
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, rct_locn, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         if ( bDesign )
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, rct_locn, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );
               if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX)
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
               }

               col++;

               (*p_table)(row,col) << reaction.SetValue( min );
               if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << _T(")");
               }

               col++;
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }


            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, rct_locn, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );
                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxConfig << _T(")");
                  }

                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );
                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minConfig << _T(")");
                  }

                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            if ( bPermit )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, rct_locn, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );
                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );
                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit)<< minConfig << _T(")");
                  }
                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating))  )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, rct_locn, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );
                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
                  }

                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );
                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << _T(")");
                  }

                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            // Legal - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, rct_locn, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );
                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );
                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine)<< minConfig << _T(")");
                  }
                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            // Legal - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, rct_locn, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );
                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );
                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special)<< minConfig << _T(")");
                  }
                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, rct_locn, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );
                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );
                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine)<< minConfig << _T(")");
                  }
                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, rct_locn, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );
                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );
                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special)<< minConfig << _T(")");
                  }
                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
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
   os << _T("Dump for CProductReactionTable") << endl;
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
