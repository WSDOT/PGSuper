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

// Function to report rows for combined pedestrian result
inline void CombineReportPedResult(ILiveLoads::PedestrianLoadApplicationType appType,
                              rptRcTable* pTable, rptForceSectionValue& reaction,
                              RowIndexType row, ColumnIndexType& pedCol,
                              Float64 llMin, Float64 llMax, Float64 pedMin, Float64 pedMax)
{
   if (appType==ILiveLoads::PedDontApply)
   {
      // no combo
     (*pTable)(row,pedCol++) << reaction.SetValue( llMax );
     (*pTable)(row,pedCol++) << reaction.SetValue( llMin );
   }
   else if (appType==ILiveLoads::PedConcurrentWithVehicular)
   {
      // sum
     (*pTable)(row,pedCol++) << reaction.SetValue( llMax + pedMax );
     (*pTable)(row,pedCol++) << reaction.SetValue( llMin + pedMin );
   }
   else if (appType==ILiveLoads::PedEnvelopeWithVehicular)
   {
     // envelope
     (*pTable)(row,pedCol++) << reaction.SetValue( max(llMax, pedMax) );
     (*pTable)(row,pedCol++) << reaction.SetValue( min(llMin, pedMin) );
   }
   else
   {
      ATLASSERT(0);
   }
}

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

   BuildCombinedDeadTable(pBroker, pChapter, span, girder, pDisplayUnits, stage, analysisType, tableType, bDesign, bRating);

   if (stage==pgsTypes::BridgeSite3)
   {
      if (bDesign)
         BuildLiveLoad(pBroker, pChapter, span, girder, pDisplayUnits, analysisType, tableType, true, true, false);
      if (bRating)
         BuildLiveLoad(pBroker, pChapter, span, girder, pDisplayUnits, analysisType, tableType, true, false, true);

      if (bDesign)
         BuildLimitStateTable(pBroker, pChapter, span, girder, true, pDisplayUnits, analysisType, tableType, true, false);
      if (bRating)
         BuildLimitStateTable(pBroker, pChapter, span, girder, true, pDisplayUnits, analysisType, tableType, false, true);
   }
}

void CCombinedReactionTable::BuildForBearingDesign(IBroker* pBroker, rptChapter* pChapter,
                                          SpanIndexType span,GirderIndexType girder, 
                                          IEAFDisplayUnits* pDisplayUnits,
                                          pgsTypes::Stage stage, pgsTypes::AnalysisType analysisType) const
{
   TableType tableType = CCombinedReactionTable::BearingReactionsTable;

   BuildCombinedDeadTable(pBroker, pChapter, span, girder, pDisplayUnits, stage, analysisType, tableType, true, false);

   if (stage==pgsTypes::BridgeSite3)
   {
      // first no impact
      BuildLiveLoad(pBroker, pChapter, span, girder, pDisplayUnits, analysisType, tableType, false, true, false);
      BuildLimitStateTable(pBroker, pChapter, span, girder, false, pDisplayUnits, analysisType, tableType, true, false);

      // with impact
      BuildLiveLoad(pBroker, pChapter, span, girder, pDisplayUnits, analysisType, tableType, true, true, false);
      BuildLimitStateTable(pBroker, pChapter, span, girder, true, pDisplayUnits, analysisType, tableType, true, false);
   }
}


void CCombinedReactionTable::BuildCombinedDeadTable(IBroker* pBroker, rptChapter* pChapter,
                                         SpanIndexType span,GirderIndexType girder,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         pgsTypes::Stage stage,pgsTypes::AnalysisType analysisType, TableType tableType,
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

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   RowIndexType row = CreateCombinedDeadLoadingTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table,(tableType==PierReactionsTable ? _T("Total Girderline Reactions at Abutments and Piers"): _T("Girder Bearing Reactions")),
                                 true ,bRating,stage,continuity_stage,
                                 analysisType,pDisplayUnits,pDisplayUnits->GetShearUnit());
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
         }
         else
         {
            BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctIncremental, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( bRating ? lcDWRating : lcDW, stage, pier, girder, ctCummulative, bat ) );
         }
      }

      row++;
   }
}

void CCombinedReactionTable::BuildLiveLoad(IBroker* pBroker, rptChapter* pChapter,
                                         SpanIndexType span,GirderIndexType girder,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         pgsTypes::AnalysisType analysisType, TableType tableType,
                                         bool includeImpact, bool bDesign,bool bRating) const
{
   ATLASSERT(!(bDesign&&bRating)); // these are separate tables, can't do both

   pgsTypes::Stage stage = pgsTypes::BridgeSite3; // always

   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   PierIndexType nPiers = pBridge->GetPierCount();

   PierIndexType startPier = (span == ALL_SPANS ? 0 : span);
   PierIndexType endPier   = (span == ALL_SPANS ? nPiers : startPier+2 );

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);

   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(span, girder);
   bool bPedLoading = pProductLoads->HasPedestrianLoad(startPier,girder);

   // pedestrian live load combination types, if applicable
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   ILiveLoads::PedestrianLoadApplicationType DesignPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltDesign);
   ILiveLoads::PedestrianLoadApplicationType FatiguePedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltFatigue);
   ILiveLoads::PedestrianLoadApplicationType PermitPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltPermit);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   std::_tstring strLabel (tableType==PierReactionsTable ? _T("Total Girderline Reactions at Abutments and Piers"): _T("Girder Bearing Reactions"));
   strLabel += std::_tstring(bDesign ? _T(" - Design Vehicles") : _T(" - Rating Vehicles"));
   strLabel += std::_tstring(includeImpact ? _T(" (Including Impact)") : _T(" (Without Impact)"));

   rptRcTable* p_table=0;
   RowIndexType Nhrows = CreateCombinedLiveLoadingTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table, strLabel.c_str(),
                                 true,bDesign,bPermit,bPedLoading,bRating,false,includeImpact,stage,analysisType,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());
   *p << p_table;

   // Compute start column for pedestrian-combined columns if needed
   ColumnIndexType startPedCol(INVALID_INDEX);
   if (bPedLoading)
   {
      ColumnIndexType nc = p_table->GetNumberOfColumns();
      startPedCol = 3 + (nc-3)/2; // first three rows loc, pedmn, pedmx
   }

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
   RowIndexType    row = Nhrows;
   Float64 min, max;
   for ( PierIndexType pier = startPier; pier < endPier; pier++ )
   {
      if (! pForces->DoReportAtPier(pier, girder) )
      {
         continue; // don't report piers that have no bearing information
      }

      if (pier == 0 || pier == pBridge->GetPierCount()-1 )
         (*p_table)(row,0) << _T("Abutment ") << LABEL_PIER(pier);
      else
         (*p_table)(row,0) << _T("Pier ") << LABEL_PIER(pier);

      ColumnIndexType col = 1;
      ColumnIndexType pedCol = startPedCol;

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bDesign )
         {
            Float64 pedMin(0), pedMax(0);
            if ( bPedLoading )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &pedMin, &max );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            // Design
            pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );


            if (bPedLoading)
            {
               CombineReportPedResult(DesignPedLoad, p_table, reaction,  
                                      row, pedCol, min, max, pedMin, pedMax);
            }

            // Fatigue
            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if (bPedLoading)
               {
                  CombineReportPedResult(FatiguePedLoad, p_table, reaction,  
                                         row, pedCol, min, max, pedMin, pedMax);
               }
            }

            if ( bPermit )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if (bPedLoading)
               {
                  CombineReportPedResult(PermitPedLoad, p_table, reaction,  
                                         row, pedCol, min, max, pedMin, pedMax);
               }
            }
         }

         if ( bRating )
         {
            Float64 pedMin(0), pedMax(0);
            if ( bPedLoading && pRatingSpec->IncludePedestrianLiveLoad() )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &pedMin, &max );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }
      }
      else
      {
         BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);

         if ( bDesign )
         {
            Float64 pedMin(0), pedMax(0);
            if ( bPedLoading )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, pier, girder, bat, includeImpact, &pedMin, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, bat, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if (bPedLoading)
            {
               CombineReportPedResult(DesignPedLoad, p_table, reaction,  
                                      row, pedCol, min, max, pedMin, pedMax);
            }

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if (bPedLoading)
               {
                  CombineReportPedResult(FatiguePedLoad, p_table, reaction,  
                                         row, pedCol, min, max, pedMin, pedMax);
               }
            }

            if ( bPermit )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if (bPedLoading)
               {
                  CombineReportPedResult(PermitPedLoad, p_table, reaction,  
                                         row, pedCol, min, max, pedMin, pedMax);
               }
            }
         }

         if ( bRating )
         {
            Float64 pedMin(0), pedMax(0);
            if ( bPedLoading && pRatingSpec->IncludePedestrianLiveLoad() )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, pier, girder, bat, includeImpact, &pedMin, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }
      }

      row++;
   }

   p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << p;
   *p << (includeImpact ? LIVELOAD_PER_GIRDER : LIVELOAD_PER_GIRDER_NO_IMPACT) << rptNewLine;

   if (bDesign && bPedLoading)
   {
      // footnotes for pedestrian loads
      int lnum=1;
      *p<< lnum++ << PedestrianFootnote(DesignPedLoad) << rptNewLine;

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         *p << lnum++ << PedestrianFootnote(FatiguePedLoad) << rptNewLine;
      }

      if ( bPermit )
      {
         *p << lnum++ << PedestrianFootnote(PermitPedLoad) << rptNewLine;
      }
   }

   if ( bRating && pRatingSpec->IncludePedestrianLiveLoad())
   {
      // Note for rating and pedestrian load
      *p << _T("$ Pedestrian load results will be summed with vehicular load results at time of load combination.");
   }
}


void CCombinedReactionTable::BuildLimitStateTable(IBroker* pBroker, rptChapter* pChapter,
                                         SpanIndexType span,GirderIndexType girder, bool includeImpact,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         pgsTypes::AnalysisType analysisType, TableType tableType,
                                         bool bDesign,bool bRating) const
{
   ATLASSERT(!(bDesign&&bRating)); // these are separate tables, can't do both

   pgsTypes::Stage stage = pgsTypes::BridgeSite3; // always

   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);

   PierIndexType nPiers = pBridge->GetPierCount();

   PierIndexType startPier = (span == ALL_SPANS ? 0 : span);
   PierIndexType endPier   = (span == ALL_SPANS ? nPiers : startPier+2 );

   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(span, girder);
   bool bPedLoading = pProductLoads->HasPedestrianLoad(startPier,girder);

   // TRICKY:
   // Use the adapter class to get the reaction response functions we need
   GET_IFACE2(pBroker,ICombinedForces,pCmbForces);
   GET_IFACE2(pBroker,ILimitStateForces,pILsForces);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   GET_IFACE2(pBroker,IStageMap,pStageMap);

   std::auto_ptr<ICmbLsReactionAdapter> pForces;
   if(  tableType==PierReactionsTable )
   {
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CombinedLsForcesReactionAdapter(pCmbForces,pILsForces));
   }

   else
   {
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CmbLsBearingDesignReactionAdapter(pBearingDesign, span) );
   }

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   std::_tstring strLabel (tableType==PierReactionsTable ? _T("Total Girderline Reactions at Abutments and Piers"): _T("Girder Bearing Reactions"));
   strLabel += std::_tstring(includeImpact ? _T(" (Including Impact)") : _T(" (Without Impact)"));

   rptRcTable * p_table;
   RowIndexType row = CreateLimitStateTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table, strLabel.c_str(),
                             true,bDesign,bPermit,bRating,false,analysisType,pStageMap,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());
   *p << p_table;

   ColumnIndexType col = 0;

   Float64 min, max;
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
            pForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
            {
               pForces->GetReaction( pgsTypes::ServiceIA, stage, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::ServiceIA, stage, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pForces->GetReaction( pgsTypes::ServiceIII, stage, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( pgsTypes::ServiceIII, stage, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims  <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetReaction( pgsTypes::FatigueI, stage, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::FatigueI, stage, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pForces->GetReaction( pgsTypes::StrengthI, stage, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( pgsTypes::StrengthI, stage, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( bPermit )
            {
               pForces->GetReaction( pgsTypes::StrengthII, stage, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthII, stage, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }

         if ( bRating )
         {
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_Inventory, stage, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthI_Inventory, stage, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_Operating, stage, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthI_Operating, stage, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_LegalRoutine, stage, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthI_LegalRoutine, stage, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_LegalSpecial, stage, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthI_LegalSpecial, stage, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetReaction( pgsTypes::ServiceI_PermitRoutine, stage, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::ServiceI_PermitRoutine, stage, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( pgsTypes::StrengthII_PermitRoutine, stage, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthII_PermitRoutine, stage, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetReaction( pgsTypes::ServiceI_PermitSpecial, stage, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::ServiceI_PermitSpecial, stage, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( pgsTypes::StrengthII_PermitSpecial, stage, pier, girder, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthII_PermitSpecial, stage, pier, girder, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }
      }
      else
      {
         BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);

         if ( bDesign )
         {
            pForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, bat, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
            {
               pForces->GetReaction( pgsTypes::ServiceIA, stage, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
            }

            pForces->GetReaction( pgsTypes::ServiceIII, stage, pier, girder, bat, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetReaction( pgsTypes::FatigueI, stage, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
            }

            pForces->GetReaction( pgsTypes::StrengthI, stage, pier, girder, bat, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( bPermit )
            {
               pForces->GetReaction( pgsTypes::StrengthII, stage, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }

         if ( bRating )
         {
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_Inventory, stage, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_Operating, stage, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_LegalRoutine, stage, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_LegalSpecial, stage, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetReaction( pgsTypes::ServiceI_PermitRoutine, stage, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( pgsTypes::StrengthII_PermitRoutine, stage, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetReaction( pgsTypes::ServiceI_PermitSpecial, stage, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( pgsTypes::StrengthII_PermitSpecial, stage, pier, girder, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }
      }

      row++;
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
