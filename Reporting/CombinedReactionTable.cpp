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
#include <Reporting\CombinedReactionTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
#include <IFace\AnalysisResults.h>

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
                                          IDisplayUnits* pDisplayUnits,
                                          pgsTypes::Stage stage, pgsTypes::AnalysisType analysisType) const
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

   RowIndexType row = CreateCombinedLoadingTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table,"Reaction",true,bPermit,bPedLoading,stage,continuity_stage,analysisType,pDisplayUnits,pDisplayUnits->GetShearUnit());
   *p << p_table;

   // Get the interface pointers we need
   GET_IFACE2(pBroker,ICombinedForces,pForces);
   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);

   // Fill up the table
   Float64 min, max;
   for ( pier = startPier; pier < endPier; pier++ )
   {
      if (pier == 0 || pier == nPiers-1 )
         (*p_table)(row,0) << "Abutment " << (Int32)(pier+1);
      else
         (*p_table)(row,0) << "Pier " << (Int32)(pier+1);

      if ( stage == pgsTypes::CastingYard || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
      {
         (*p_table)(row,1) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, SimpleSpan ) );
         pLsForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, SimpleSpan, true, &min, &max );
         (*p_table)(row,2) << reaction.SetValue( max );
      }
      else if ( stage == pgsTypes::BridgeSite1 )
      {
         int col = 1;
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctIncremental, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctCummulative, MinSimpleContinuousEnvelope ) );

            pLsForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pLsForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, MinSimpleContinuousEnvelope,true,  &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );
         }
         else
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, SimpleSpan ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctIncremental, SimpleSpan ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, SimpleSpan ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctCummulative, SimpleSpan ) );

            pLsForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, SimpleSpan, true, &min, &max );
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
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctIncremental, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctCummulative, MinSimpleContinuousEnvelope ) );

            pLsForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pLsForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );
         }
         else
         {
            BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctIncremental, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctCummulative, bat ) );

            pLsForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, bat, true, &min, &max );
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
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctIncremental, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctIncremental, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctCummulative, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctCummulative, MinSimpleContinuousEnvelope ) );

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
         else
         {
            BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctIncremental, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctIncremental, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, bat ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctCummulative, bat ) );

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

      ColumnIndexType nCols = 6;

      if ( bPermit )
         nCols += 2;

      if ( analysisType == pgsTypes::Envelope )
         nCols += 3;

      p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"");
      row = ConfigureLimitStateTableHeading<rptForceUnitTag,unitmgtForceData>(p_table,true,bPermit,false,analysisType,pDisplayUnits,pDisplayUnits->GetShearUnit());
      *p << p_table;

      ColumnIndexType col = 0;
      (*p_table)(0,col++) << "";


      for ( PierIndexType pier = startPier; pier < endPier; pier++ )
      {
         col = 0;
         if (pier == 0 || pier == nPiers-1 )
            (*p_table)(row,col++) << "Abutment " << (Int32)(pier+1);
         else
            (*p_table)(row,col++) << "Pier " << (Int32)(pier+1);

         if ( analysisType == pgsTypes::Envelope )
         {
            pLsForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pLsForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
            {
               pLsForces->GetReaction( pgsTypes::ServiceIA, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pLsForces->GetReaction( pgsTypes::ServiceIA, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pLsForces->GetReaction( pgsTypes::ServiceIII, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pLsForces->GetReaction( pgsTypes::ServiceIII, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims  <= lrfdVersionMgr::GetVersion() )
            {
               pLsForces->GetReaction( pgsTypes::FatigueI, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pLsForces->GetReaction( pgsTypes::FatigueI, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pLsForces->GetReaction( pgsTypes::StrengthI, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pLsForces->GetReaction( pgsTypes::StrengthI, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( bPermit )
            {
               pLsForces->GetReaction( pgsTypes::StrengthII, stage, pier, girder, MaxSimpleContinuousEnvelope, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pLsForces->GetReaction( pgsTypes::StrengthII, stage, pier, girder, MinSimpleContinuousEnvelope, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }
         else
         {
            BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
            pLsForces->GetReaction( pgsTypes::ServiceI, stage, pier, girder, bat, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
            {
               pLsForces->GetReaction( pgsTypes::ServiceIA, stage, pier, girder, bat, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
            }

            pLsForces->GetReaction( pgsTypes::ServiceIII, stage, pier, girder, bat, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pLsForces->GetReaction( pgsTypes::FatigueI, stage, pier, girder, bat, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
            }

            pLsForces->GetReaction( pgsTypes::StrengthI, stage, pier, girder, bat, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( bPermit )
            {
               pLsForces->GetReaction( pgsTypes::StrengthII, stage, pier, girder, bat, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
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
   os << "Dump for CCombinedReactionTable" << endl;
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
