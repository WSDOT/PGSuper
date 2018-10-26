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
                                          pgsTypes::Stage stage, pgsTypes::AnalysisType analysisType,ReactionTableType tableType,
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
   ReactionTableType tableType = BearingReactionsTable;

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
                                         pgsTypes::Stage stage,pgsTypes::AnalysisType analysisType, ReactionTableType tableType,
                                         bool bDesign,bool bRating) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(pBroker,ICombinedForces,pCmbForces);
   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);

   // TRICKY:
   // Use the adapter class to get the reaction response functions we need and to iterate piers
  std::auto_ptr<ICmbLsReactionAdapter> pForces;
   if(  tableType==PierReactionsTable )
   {
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CombinedLsForcesReactionAdapter(pCmbForces,pLsForces,span,girder));
   }

   else
   {
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CmbLsBearingDesignReactionAdapter(pBearingDesign, stage, span, girder) );
   }

   // Use iterator to walk locations
   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

   pgsTypes::Stage continuity_stage = pgsTypes::BridgeSite2;
   PierIndexType pier = 0;
   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      const ReactionLocation& rct_locn = iter.CurrentItem();

      pgsTypes::Stage left_stage, right_stage;
      pBridge->GetContinuityStage(rct_locn.Pier,&left_stage,&right_stage);
      continuity_stage = _cpp_min(continuity_stage,left_stage);
      continuity_stage = _cpp_min(continuity_stage,right_stage);
   }

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table=0;

   RowIndexType row = CreateCombinedDeadLoadingTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table,(tableType==PierReactionsTable ? _T("Total Girderline Reactions at Abutments and Piers"): _T("Girder Bearing Reactions")),
                                 true ,bRating,stage,continuity_stage,
                                 analysisType,pDisplayUnits,pDisplayUnits->GetShearUnit());
   *p << p_table;

   // Fill up the table
   Float64 min, max;
   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      const ReactionLocation& rct_locn = iter.CurrentItem();

     (*p_table)(row,0) << rct_locn.PierLabel;

      if ( stage == pgsTypes::CastingYard || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
      {
         (*p_table)(row,1) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctIncremental, SimpleSpan ) );
         pForces->GetReaction( pgsTypes::ServiceI, stage, rct_locn, SimpleSpan, true, &min, &max );
         (*p_table)(row,2) << reaction.SetValue( max );
      }
      else if ( stage == pgsTypes::BridgeSite1 )
      {
         int col = 1;
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctIncremental, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctIncremental, MinSimpleContinuousEnvelope ) );
            if(bRating)
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctIncremental, MaxSimpleContinuousEnvelope ) );
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctIncremental, MinSimpleContinuousEnvelope ) );
            }
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctCummulative, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctCummulative, MinSimpleContinuousEnvelope ) );
            if(bRating)
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctCummulative, MaxSimpleContinuousEnvelope ) );
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctCummulative, MinSimpleContinuousEnvelope ) );
            }

            pForces->GetReaction( pgsTypes::ServiceI, stage, rct_locn, MaxSimpleContinuousEnvelope, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( pgsTypes::ServiceI, stage, rct_locn, MinSimpleContinuousEnvelope,true,  &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );
         }
         else
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctIncremental, SimpleSpan ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctIncremental, SimpleSpan ) );
            if(bRating)
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctIncremental, SimpleSpan ) );
            }
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctCummulative, SimpleSpan ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctCummulative, SimpleSpan ) );
            if(bRating)
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctCummulative, SimpleSpan ) );
            }

            pForces->GetReaction( pgsTypes::ServiceI, stage, rct_locn, SimpleSpan, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
         }
      }
      else if ( stage == pgsTypes::BridgeSite2 )
      {
         int col = 1;

         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctIncremental, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctIncremental, MinSimpleContinuousEnvelope ) );
            if(bRating)
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctIncremental, MaxSimpleContinuousEnvelope ) );
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctIncremental, MinSimpleContinuousEnvelope ) );
            }
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctCummulative, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctCummulative, MinSimpleContinuousEnvelope ) );
            if(bRating)
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctCummulative, MaxSimpleContinuousEnvelope ) );
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctCummulative, MinSimpleContinuousEnvelope ) );
            }

            pForces->GetReaction( pgsTypes::ServiceI, stage, rct_locn, MaxSimpleContinuousEnvelope, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( pgsTypes::ServiceI, stage, rct_locn, MinSimpleContinuousEnvelope, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );
         }
         else
         {
            BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctIncremental, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctIncremental, bat ) );
            if(bRating)
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctIncremental, bat ) );
            }
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctCummulative, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctCummulative, bat ) );
            if(bRating)
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction(lcDWRating, stage, rct_locn, ctCummulative, bat ) );
            }

            pForces->GetReaction( pgsTypes::ServiceI, stage, rct_locn, bat, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
         }

      }
      else if ( stage == pgsTypes::BridgeSite3 )
      {
         int col = 1;

         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctIncremental, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctIncremental, MinSimpleContinuousEnvelope ) );
            if(bRating)
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctIncremental, MaxSimpleContinuousEnvelope ) );
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctIncremental, MinSimpleContinuousEnvelope ) );
            }
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctCummulative, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctCummulative, MinSimpleContinuousEnvelope ) );
            if(bRating)
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctCummulative, MaxSimpleContinuousEnvelope ) );
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctCummulative, MinSimpleContinuousEnvelope ) );
            }
         }
         else
         {
            BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctIncremental, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctIncremental, bat ) );
            if(bRating)
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctIncremental, bat ) );
            }
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, rct_locn, ctCummulative, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, rct_locn, ctCummulative, bat ) );
            if(bRating)
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, stage, rct_locn, ctCummulative, bat ) );
            }
         }
      }

      row++;
   }
}

void CCombinedReactionTable::BuildLiveLoad(IBroker* pBroker, rptChapter* pChapter,
                                         SpanIndexType span,GirderIndexType girder,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         pgsTypes::AnalysisType analysisType, ReactionTableType tableType,
                                         bool includeImpact, bool bDesign,bool bRating) const
{
   ATLASSERT(!(bDesign&&bRating)); // these are separate tables, can't do both

   pgsTypes::Stage stage = pgsTypes::BridgeSite3; // always

   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );


   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,ICombinedForces,pCmbForces);
   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);

   // TRICKY:
   // Use the adapter class to get the reaction response functions we need and to iterate piers
  std::auto_ptr<ICmbLsReactionAdapter> pForces;
   if(  tableType==PierReactionsTable )
   {
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CombinedLsForcesReactionAdapter(pCmbForces,pLsForces,span,girder));
   }

   else
   {
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CmbLsBearingDesignReactionAdapter(pBearingDesign, pgsTypes::BridgeSite3, span ,girder) );
   }

   // Use iterator to walk locations
   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

   // Use first location to determine if ped load is applied
   iter.First();
   const ReactionLocation& first_locn = iter.CurrentItem();

   bool bPermit = pLsForces->IsStrengthIIApplicable(span, girder);
   bool bPedLoading = pProductLoads->HasPedestrianLoad(first_locn.Pier, first_locn.Girder);

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


   // Fill up the table
   RowIndexType    row = Nhrows;
   Float64 min, max;
   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      const ReactionLocation& rct_locn = iter.CurrentItem();

     (*p_table)(row,0) << rct_locn.PierLabel;

      ColumnIndexType col = 1;
      ColumnIndexType pedCol = startPedCol;

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bDesign )
         {
            Float64 pedMin(0), pedMax(0);
            if ( bPedLoading )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &pedMin, &max );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            // Design
            pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );


            if (bPedLoading)
            {
               CombineReportPedResult(DesignPedLoad, p_table, reaction,  
                                      row, pedCol, min, max, pedMin, pedMax);
            }

            // Fatigue
            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if (bPedLoading)
               {
                  CombineReportPedResult(FatiguePedLoad, p_table, reaction,  
                                         row, pedCol, min, max, pedMin, pedMax);
               }
            }

            if ( bPermit )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
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
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &pedMin, &max );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
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
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, rct_locn, bat, includeImpact, &pedMin, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, rct_locn, bat, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if (bPedLoading)
            {
               CombineReportPedResult(DesignPedLoad, p_table, reaction,  
                                      row, pedCol, min, max, pedMin, pedMax);
            }

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, rct_locn, bat, includeImpact, &min, &max );
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
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, rct_locn, bat, includeImpact, &min, &max );
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
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, rct_locn, bat, includeImpact, &pedMin, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, rct_locn, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, rct_locn, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, rct_locn, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, rct_locn, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, rct_locn, bat, includeImpact, &min, &max );
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
                                         pgsTypes::AnalysisType analysisType, ReactionTableType tableType,
                                         bool bDesign,bool bRating) const
{
   ATLASSERT(!(bDesign&&bRating)); // these are separate tables, can't do both

   pgsTypes::Stage stage = pgsTypes::BridgeSite3; // always

   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(pBroker,ICombinedForces,pCmbForces);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   GET_IFACE2(pBroker,IStageMap,pStageMap);

   // TRICKY:
   // Use the adapter class to get the reaction response functions we need and to iterate piers
  std::auto_ptr<ICmbLsReactionAdapter> pForces;
   if(  tableType==PierReactionsTable )
   {
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CombinedLsForcesReactionAdapter(pCmbForces,pLsForces,span,girder));
   }

   else
   {
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CmbLsBearingDesignReactionAdapter(pBearingDesign, pgsTypes::BridgeSite3, span ,girder) );
   }

   // Use iterator to walk locations
   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

   // Use first location to determine if ped load is applied
   iter.First();
   const ReactionLocation& first_locn = iter.CurrentItem();

   bool bPermit = pLsForces->IsStrengthIIApplicable(span, girder);
   bool bPedLoading = pProductLoads->HasPedestrianLoad(first_locn.Pier, first_locn.Girder);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   std::_tstring strLabel (tableType==PierReactionsTable ? _T("Total Girderline Reactions at Abutments and Piers"): _T("Girder Bearing Reactions"));
   strLabel += std::_tstring(includeImpact ? _T(" (Including Impact)") : _T(" (Without Impact)"));

   rptRcTable * p_table;
   RowIndexType row = CreateLimitStateTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table, strLabel.c_str(),
                             true,bDesign,bPermit,bRating,false,analysisType,pStageMap,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());
   *p << p_table;

   Float64 min, max;
   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      ColumnIndexType col = 0;
      const ReactionLocation& rct_locn = iter.CurrentItem();

     (*p_table)(row,col++) << rct_locn.PierLabel;

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bDesign )
         {
            pForces->GetReaction( pgsTypes::ServiceI, stage, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( pgsTypes::ServiceI, stage, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
            {
               pForces->GetReaction( pgsTypes::ServiceIA, stage, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::ServiceIA, stage, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pForces->GetReaction( pgsTypes::ServiceIII, stage, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( pgsTypes::ServiceIII, stage, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims  <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetReaction( pgsTypes::FatigueI, stage, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::FatigueI, stage, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pForces->GetReaction( pgsTypes::StrengthI, stage, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( pgsTypes::StrengthI, stage, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( bPermit )
            {
               pForces->GetReaction( pgsTypes::StrengthII, stage, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthII, stage, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }

         if ( bRating )
         {
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_Inventory, stage, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthI_Inventory, stage, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_Operating, stage, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthI_Operating, stage, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_LegalRoutine, stage, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthI_LegalRoutine, stage, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_LegalSpecial, stage, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthI_LegalSpecial, stage, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetReaction( pgsTypes::ServiceI_PermitRoutine, stage, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::ServiceI_PermitRoutine, stage, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( pgsTypes::StrengthII_PermitRoutine, stage, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthII_PermitRoutine, stage, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetReaction( pgsTypes::ServiceI_PermitSpecial, stage, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::ServiceI_PermitSpecial, stage, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( pgsTypes::StrengthII_PermitSpecial, stage, rct_locn, MaxSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthII_PermitSpecial, stage, rct_locn, MinSimpleContinuousEnvelope, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }
      }
      else
      {
         BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);

         if ( bDesign )
         {
            pForces->GetReaction( pgsTypes::ServiceI, stage, rct_locn, bat, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
            {
               pForces->GetReaction( pgsTypes::ServiceIA, stage, rct_locn, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pForces->GetReaction( pgsTypes::ServiceIII, stage, rct_locn, bat, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetReaction( pgsTypes::FatigueI, stage, rct_locn, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pForces->GetReaction( pgsTypes::StrengthI, stage, rct_locn, bat, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( bPermit )
            {
               pForces->GetReaction( pgsTypes::StrengthII, stage, rct_locn, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }

         if ( bRating )
         {
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_Inventory, stage, rct_locn, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_Operating, stage, rct_locn, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_LegalRoutine, stage, rct_locn, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_LegalSpecial, stage, rct_locn, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetReaction( pgsTypes::ServiceI_PermitRoutine, stage, rct_locn, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( pgsTypes::StrengthII_PermitRoutine, stage, rct_locn, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetReaction( pgsTypes::ServiceI_PermitSpecial, stage, rct_locn, bat, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( pgsTypes::StrengthII_PermitSpecial, stage, rct_locn, bat, includeImpact, &min, &max );
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
