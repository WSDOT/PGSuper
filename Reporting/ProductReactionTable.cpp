///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <Reporting\ReactionInterfaceAdapters.h>

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
rptRcTable* CProductReactionTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                                         TableType tableType, bool bIncludeImpact, bool bIncludeLLDF,bool bDesign,bool bRating,bool bIndicateControllingLoad,
                                         IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   PierIndexType nPiers = pBridge->GetPierCount();

   bool bConstruction, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   GroupIndexType startGroup, nGroups;
   IntervalIndexType continuityIntervalIdx;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,bDesign,bRating,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&continuityIntervalIdx,&startGroup,&nGroups);

   PierIndexType startPier = pBridge->GetGirderGroupStartPier(startGroup);
   PierIndexType endPier   = pBridge->GetGirderGroupEndPier(startGroup+nGroups-1);
   
   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,
                         tableType==PierReactionsTable ?_T("Total Girderline Reactions at Abutments and Piers"): _T("Girder Bearing Reactions") );
   RowIndexType row = ConfigureProductLoadTableHeading<rptForceUnitTag,unitmgtForceData>(pBroker,p_table,true,false,bConstruction,bDeckPanels,bSidewalk,bShearKey,bDesign,bPedLoading,bPermit,bRating,analysisType,continuityIntervalIdx,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);

   // TRICKY: use adapter class to get correct reaction interfaces
   std::auto_ptr<IProductReactionAdapter> pForces;
   if( tableType == PierReactionsTable )
   {
      pForces =  std::auto_ptr<ProductForcesReactionAdapter>(new ProductForcesReactionAdapter(pProductForces));
   }
   else
   {
      pForces =  std::auto_ptr<BearingDesignProductReactionAdapter>(new BearingDesignProductReactionAdapter(pBearingDesign, startPier, endPier) );
   }

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetFirstErectedSegmentInterval();

   for ( PierIndexType pier = startPier; pier <= endPier; pier++ )
   {
      if (!pForces->DoReportAtPier(pier, girderKey))
      {
         // Don't report pier if information is not available
         continue;
      }

      ColumnIndexType col = 0;

      if ( pier == 0 || pier == nPiers-1 )
         (*p_table)(row,col++) << _T("Abutment ") << LABEL_PIER(pier);
      else
         (*p_table)(row,col++) << _T("Pier ") << LABEL_PIER(pier);
   
      (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( erectSegmentIntervalIdx, pftGirder,    pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
      (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx,     pftDiaphragm, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );

      if ( bShearKey )
      {
         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftShearKey, pier, girderKey, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftShearKey, pier, girderKey, minBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftShearKey, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
         }
      }

      if ( bConstruction )
      {
         if ( analysisType == pgsTypes::Envelope && continuityIntervalIdx == castDeckIntervalIdx )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftConstruction,   pier, girderKey, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftConstruction,   pier, girderKey, minBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftConstruction,   pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
         }
      }

      if ( analysisType == pgsTypes::Envelope && continuityIntervalIdx == castDeckIntervalIdx )
      {
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftSlab, pier, girderKey, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftSlab, pier, girderKey, minBAT ) );

         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftSlabPad, pier, girderKey, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftSlabPad, pier, girderKey, minBAT ) );
      }
      else
      {
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftSlab, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan  ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftSlabPad, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan  ) );
      }

      if ( bDeckPanels )
      {
         if ( analysisType == pgsTypes::Envelope && continuityIntervalIdx == castDeckIntervalIdx )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftSlabPanel,   pier, girderKey, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftSlabPanel,   pier, girderKey, minBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftSlabPanel,   pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
         }
      }

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bSidewalk )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( railingSystemIntervalIdx, pftSidewalk, pier, girderKey, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( railingSystemIntervalIdx, pftSidewalk, pier, girderKey, minBAT ) );
         }

         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( railingSystemIntervalIdx, pftTrafficBarrier, pier, girderKey, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( railingSystemIntervalIdx, pftTrafficBarrier, pier, girderKey, minBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( overlayIntervalIdx, bRating ? pftOverlayRating : pftOverlay, pier, girderKey, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( overlayIntervalIdx, bRating ? pftOverlayRating : pftOverlay, pier, girderKey, minBAT ) );

         Float64 min, max;
         VehicleIndexType minConfig, maxConfig;

         if ( bPedLoading )
         {
            pForces->GetLiveLoadReaction( pgsTypes::lltPedestrian, liveLoadIntervalIdx, pier, girderKey, maxBAT, bIncludeImpact, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetLiveLoadReaction( pgsTypes::lltPedestrian, liveLoadIntervalIdx, pier, girderKey, minBAT, bIncludeImpact, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );
         }

         if ( bDesign )
         {
            pForces->GetLiveLoadReaction( pgsTypes::lltDesign, liveLoadIntervalIdx, pier, girderKey, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << reaction.SetValue( max );

            if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
            {
               (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
            }

            col++;

            pForces->GetLiveLoadReaction( pgsTypes::lltDesign, liveLoadIntervalIdx, pier, girderKey, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig  );
            (*p_table)(row,col) << reaction.SetValue( min );

            if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
            {
               (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltDesign)<< minConfig << _T(")");
            }

            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltFatigue, liveLoadIntervalIdx, pier, girderKey, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxConfig << _T(")");
               }

               col++;

               pForces->GetLiveLoadReaction( pgsTypes::lltFatigue, liveLoadIntervalIdx, pier, girderKey, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig  );
               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minConfig << _T(")");
               }

               col++;
            }

            if ( bPermit )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPermit, liveLoadIntervalIdx, pier, girderKey, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxConfig << _T(")");
               }

               col++;

               pForces->GetLiveLoadReaction( pgsTypes::lltPermit, liveLoadIntervalIdx, pier, girderKey, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minConfig << _T(")");
               }

               col++;
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltDesign, liveLoadIntervalIdx, pier, girderKey, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
               }

               col++;

               pForces->GetLiveLoadReaction( pgsTypes::lltDesign, liveLoadIntervalIdx, pier, girderKey, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig  );
               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltDesign)<< minConfig << _T(")");
               }

               col++;
            }

            // Legal - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Routine, liveLoadIntervalIdx, pier, girderKey, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxConfig << _T(")");
               }

               col++;

               pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Routine, liveLoadIntervalIdx, pier, girderKey, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minConfig << _T(")");
               }

               col++;
            }

            // Legal - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Special, liveLoadIntervalIdx, pier, girderKey, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxConfig << _T(")");
               }

               col++;

               pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Special, liveLoadIntervalIdx, pier, girderKey, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << minConfig << _T(")");
               }

               col++;
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Routine, liveLoadIntervalIdx, pier, girderKey, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxConfig << _T(")");
               }

               col++;

               pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Routine, liveLoadIntervalIdx, pier, girderKey, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minConfig << _T(")");
               }

               col++;
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Special, liveLoadIntervalIdx, pier, girderKey, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad && maxConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxConfig << _T(")");
               }

               col++;

               pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Special, liveLoadIntervalIdx, pier, girderKey, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << minConfig << _T(")");
               }

               col++;
            }
         }
      }
      else
      {
         if ( bSidewalk )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( railingSystemIntervalIdx, pftSidewalk, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
         }

         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( railingSystemIntervalIdx, pftTrafficBarrier, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( overlayIntervalIdx, bRating ? pftOverlayRating : pftOverlay,        pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );

         Float64 min, max;
         VehicleIndexType minConfig, maxConfig;
         if ( bPedLoading )
         {
            pForces->GetLiveLoadReaction( pgsTypes::lltPedestrian, liveLoadIntervalIdx, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
            (*p_table)(row,col++) << reaction.SetValue( min );
         }

         if ( bDesign )
         {
            pForces->GetLiveLoadReaction( pgsTypes::lltDesign, liveLoadIntervalIdx, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << reaction.SetValue( max );
            if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX)
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
            }

            col++;

            (*p_table)(row,col) << reaction.SetValue( min );
            if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << _T(")");
            }

            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltFatigue, liveLoadIntervalIdx, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );
               if ( bIndicateControllingLoad && maxConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxConfig << _T(")");
               }

               col++;

               (*p_table)(row,col) << reaction.SetValue( min );
               if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minConfig << _T(")");
               }

               col++;
            }

            if ( bPermit )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPermit, liveLoadIntervalIdx, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating))  )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltDesign, liveLoadIntervalIdx, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );
               if ( bIndicateControllingLoad && maxConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
               }

               col++;

               (*p_table)(row,col) << reaction.SetValue( min );
               if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << _T(")");
               }

               col++;
            }

            // Legal - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Routine, liveLoadIntervalIdx, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );
               if ( bIndicateControllingLoad && maxConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxConfig << _T(")");
               }
               col++;

               (*p_table)(row,col) << reaction.SetValue( min );
               if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine)<< minConfig << _T(")");
               }
               col++;
            }

            // Legal - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltLegalRating_Special, liveLoadIntervalIdx, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );
               if ( bIndicateControllingLoad && maxConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxConfig << _T(")");
               }
               col++;

               (*p_table)(row,col) << reaction.SetValue( min );
               if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special)<< minConfig << _T(")");
               }
               col++;
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Routine, liveLoadIntervalIdx, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );
               if ( bIndicateControllingLoad && maxConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxConfig << _T(")");
               }
               col++;

               (*p_table)(row,col) << reaction.SetValue( min );
               if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine)<< minConfig << _T(")");
               }
               col++;
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetLiveLoadReaction( pgsTypes::lltPermitRating_Special, liveLoadIntervalIdx, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );
               if ( bIndicateControllingLoad && maxConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxConfig << _T(")");
               }
               col++;

               (*p_table)(row,col) << reaction.SetValue( min );
               if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special)<< minConfig << _T(")");
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
