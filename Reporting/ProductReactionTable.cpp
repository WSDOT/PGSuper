///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
                                         bool bIncludeImpact, bool bIncludeLLDF,bool bIndicateControllingLoad,IDisplayUnits* pDispUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDispUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDispUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bDeckPanels = (pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP ? true : false);
   pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

   PierIndexType nPiers = pBridge->GetPierCount();

   PierIndexType startPier = (span == ALL_SPANS ? 0 : span);
   PierIndexType endPier   = (span == ALL_SPANS ? nPiers : startPier+2 );

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE2(pBroker,IProductForces,pForces);
   bool bPedLoading = pForces->HasPedestrianLoad(startPier,gdr);
   bool bSidewalk = pForces->HasSidewalkLoad(startPier,gdr);

   pgsTypes::Stage continuity_stage = pgsTypes::BridgeSite2;
   PierIndexType pier;
   for ( pier = startPier; pier < endPier; pier++ )
   {
      pgsTypes::Stage left_stage, right_stage;
      pBridge->GetContinuityStage(pier,&left_stage,&right_stage);
      continuity_stage = _cpp_min(continuity_stage,left_stage);
      continuity_stage = _cpp_min(continuity_stage,right_stage);
   }

   ColumnIndexType nCols = 8;

   if ( bDeckPanels )
   {
      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1)
         nCols += 2;
      else
         nCols++;
   }

   if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      nCols++; // add on more column for min/max slab

   if ( analysisType == pgsTypes::Envelope )
      nCols += 2; // add one more each for min/max overlay and min/max traffic barrier

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      nCols += 2; // fatigue live load

   if ( bPermit )
      nCols += 2;

   if ( bPedLoading )
      nCols += 2;

   if ( bSidewalk )
   {
      if (analysisType == pgsTypes::Envelope )
      {
         nCols += 2;
      }
      else
      {
         nCols++;
      }
   }


   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Reactions");
   RowIndexType row = ConfigureProductLoadTableHeading<rptForceUnitTag,unitmgtForceData>(p_table,true,bDeckPanels,bSidewalk,bPedLoading,bPermit,analysisType,continuity_stage,pDispUnits,pDispUnits->GetShearUnit());

   // get the stage the girder dead load is applied in
   pgsTypes::Stage girderLoadStage = pForces->GetGirderDeadLoadStage(gdr);

    // Fill up the table
   for ( pier = startPier; pier < endPier; pier++ )
   {
      int col = 0;

      if ( pier == 0 || pier == nPiers-1 )
         (*p_table)(row,col++) << "Abutment " << (Int32)(pier+1);
      else
         (*p_table)(row,col++) << "Pier " << (Int32)(pier+1);
   
      (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( girderLoadStage, pftGirder,         pier, gdr, SimpleSpan ) );
      (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftDiaphragm,      pier, gdr, SimpleSpan ) );

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
            (*p_table)(row,col) << rptNewLine << "(D" << maxConfig << ")";
         }

         col++;

         pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, gdr, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig  );
         (*p_table)(row,col) << reaction.SetValue( min );

         if ( bIndicateControllingLoad )
         {
            (*p_table)(row,col) << rptNewLine << "(D" << minConfig << ")";
         }

         col++;

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces->GetLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, pier, gdr, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << reaction.SetValue( max );

            if ( bIndicateControllingLoad )
            {
               (*p_table)(row,col) << rptNewLine << "(F" << maxConfig << ")";
            }

            col++;

            pForces->GetLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, pier, gdr, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig  );
            (*p_table)(row,col) << reaction.SetValue( min );

            if ( bIndicateControllingLoad )
            {
               (*p_table)(row,col) << rptNewLine << "(F" << minConfig << ")";
            }

            col++;
         }


         if ( bPermit )
         {
            pForces->GetLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, pier, gdr, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << reaction.SetValue( max );

            if ( bIndicateControllingLoad )
            {
               (*p_table)(row,col) << rptNewLine << "(P" << maxConfig << ")";
            }

            col++;

            pForces->GetLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, pier, gdr, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << reaction.SetValue( min );

            if ( bIndicateControllingLoad )
            {
               (*p_table)(row,col) << rptNewLine << "(P" << minConfig << ")";
            }

            col++;
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
            (*p_table)(row,col) << rptNewLine << "(D" << maxConfig << ")";
         }

         col++;

         (*p_table)(row,col) << reaction.SetValue( min );
         if ( bIndicateControllingLoad )
         {
            (*p_table)(row,col) << rptNewLine << "(D" << minConfig << ")";
         }

         col++;

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces->GetLiveLoadReaction( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << reaction.SetValue( max );
            if ( bIndicateControllingLoad )
            {
               (*p_table)(row,col) << rptNewLine << "(F" << maxConfig << ")";
            }

            col++;

            (*p_table)(row,col) << reaction.SetValue( min );
            if ( bIndicateControllingLoad )
            {
               (*p_table)(row,col) << rptNewLine << "(F" << minConfig << ")";
            }

            col++;
         }

         if ( bPermit )
         {
            pForces->GetLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, pier, gdr, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << reaction.SetValue( max );
            if ( bIndicateControllingLoad )
            {
               (*p_table)(row,col) << rptNewLine << "(P" << maxConfig << ")";
            }
            col++;

            (*p_table)(row,col) << reaction.SetValue( min );
            if ( bIndicateControllingLoad )
            {
               (*p_table)(row,col) << rptNewLine << "(P" << minConfig << ")";
            }
            col++;
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
