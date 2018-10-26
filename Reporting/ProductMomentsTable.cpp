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
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>

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
   CProductMomentsTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProductMomentsTable::CProductMomentsTable()
{
}

CProductMomentsTable::CProductMomentsTable(const CProductMomentsTable& rOther)
{
   MakeCopy(rOther);
}

CProductMomentsTable::~CProductMomentsTable()
{
}

//======================== OPERATORS  =======================================
CProductMomentsTable& CProductMomentsTable::operator= (const CProductMomentsTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}


//======================== OPERATIONS =======================================
rptRcTable* CProductMomentsTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsTypes::AnalysisType analysisType,
                                      bool bIndicateControllingLoad,IDisplayUnits* pDispUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDispUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment, pDispUnits->GetMomentUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bDeckPanels = (pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP ? true : false);
   pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType nSpans    = (span == ALL_SPANS ? pBridge->GetSpanCount() : startSpan+1 );

   GET_IFACE2(pBroker,IProductForces2,pForces2);
   GET_IFACE2(pBroker,IProductForces,pForces);
   bool bPedLoading = pForces->HasPedestrianLoad(startSpan,gdr);
   bool bSidewalk = pForces->HasSidewalkLoad(startSpan,gdr);

   pgsTypes::Stage continuity_stage = pgsTypes::BridgeSite2;
   SpanIndexType spanIdx;
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

   ColumnIndexType nCols = 8; // location, girder, diaphragm, slab, overlay, traffic barrier, LL min, LL max

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

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


   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Moments");
   RowIndexType row = ConfigureProductLoadTableHeading<rptMomentUnitTag,unitmgtMomentData>(p_table,false,bDeckPanels,bSidewalk,bPedLoading,bPermit,analysisType,continuity_stage,pDispUnits,pDispUnits->GetMomentUnit());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   for ( spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      // Get all the tabular poi's for flexure and shear
      // Merge the two vectors to form one vector to report on.
      std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(pgsTypes::BridgeSite1,spanIdx,gdr, POI_ALLACTIONS | POI_TABULAR | POI_CRITSECTSHEAR1 | POI_CRITSECTSHEAR2, POIFIND_OR);

      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);

      pgsTypes::Stage girderLoadStage = pForces->GetGirderDeadLoadStage(spanIdx,gdrIdx);

      // Get the results for this span (it is faster to get them as a vector rather than individually)
      std::vector<Float64> girder = pForces2->GetMoment(girderLoadStage,pftGirder,vPoi,SimpleSpan);
      std::vector<Float64> diaphragm = pForces2->GetMoment(pgsTypes::BridgeSite1,pftDiaphragm,vPoi,SimpleSpan);

      std::vector<Float64> minSlab, maxSlab;
      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      {
         maxSlab = pForces2->GetMoment( pgsTypes::BridgeSite1, pftSlab, vPoi, MaxSimpleContinuousEnvelope );
         minSlab = pForces2->GetMoment( pgsTypes::BridgeSite1, pftSlab, vPoi, MinSimpleContinuousEnvelope );
      }
      else
      {
         maxSlab = pForces2->GetMoment( pgsTypes::BridgeSite1, pftSlab, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
      }

      std::vector<Float64> minDeckPanel, maxDeckPanel;
      if ( bDeckPanels )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            maxDeckPanel = pForces2->GetMoment( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, MaxSimpleContinuousEnvelope );
            minDeckPanel = pForces2->GetMoment( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, MinSimpleContinuousEnvelope );
         }
         else
         {
            maxDeckPanel = pForces2->GetMoment( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         }
      }

      std::vector<Float64> dummy;
      std::vector<Float64> minOverlay, maxOverlay;
      std::vector<Float64> minTrafficBarrier, maxTrafficBarrier;
      std::vector<Float64> minSidewalk, maxSidewalk;
      std::vector<Float64> minPedestrian, maxPedestrian;
      std::vector<Float64> minDesignLL, maxDesignLL;
      std::vector<Float64> minFatigueLL, maxFatigueLL;
      std::vector<Float64> minPermitLL, maxPermitLL;

      std::vector<long> dummyTruck, minDesignLLtruck, maxDesignLLtruck, minFatigueLLtruck, maxFatigueLLtruck, minPermitLLtruck, maxPermitLLtruck;

      if (analysisType == pgsTypes::Envelope)
      {
         if ( bSidewalk )
         {
            maxSidewalk = pForces2->GetMoment( pgsTypes::BridgeSite2, pftSidewalk, vPoi, MaxSimpleContinuousEnvelope );
            minSidewalk = pForces2->GetMoment( pgsTypes::BridgeSite2, pftSidewalk, vPoi, MinSimpleContinuousEnvelope );
         }

         maxTrafficBarrier = pForces2->GetMoment( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, MaxSimpleContinuousEnvelope );
         minTrafficBarrier = pForces2->GetMoment( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, MinSimpleContinuousEnvelope );
         maxOverlay = pForces2->GetMoment( overlay_stage, pftOverlay, vPoi, MaxSimpleContinuousEnvelope );
         minOverlay = pForces2->GetMoment( overlay_stage, pftOverlay, vPoi, MinSimpleContinuousEnvelope );

         if ( bPedLoading )
         {
            pForces2->GetLiveLoadMoment( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxPedestrian );
            pForces2->GetLiveLoadMoment( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minPedestrian, &dummy );
         }

         pForces2->GetLiveLoadMoment( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
         pForces2->GetLiveLoadMoment( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetLiveLoadMoment( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxFatigueLL, &dummyTruck, &maxFatigueLLtruck );
            pForces2->GetLiveLoadMoment( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minFatigueLL, &dummy, &minFatigueLLtruck, &dummyTruck );
         }

         if ( bPermit )
         {
            pForces2->GetLiveLoadMoment( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxPermitLL, &dummyTruck, &maxPermitLLtruck );
            pForces2->GetLiveLoadMoment( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minPermitLL, &dummy, &minPermitLLtruck, &dummyTruck );
         }
      }
      else
      {
         if ( bSidewalk )
         {
            maxSidewalk = pForces2->GetMoment( pgsTypes::BridgeSite2, pftSidewalk, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         }

         maxTrafficBarrier = pForces2->GetMoment( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         maxOverlay = pForces2->GetMoment( overlay_stage, pftOverlay, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );

         if ( bPedLoading )
         {
            pForces2->GetLiveLoadMoment( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minPedestrian, &maxPedestrian );
         }

         pForces2->GetLiveLoadMoment( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minDesignLL, &maxDesignLL, &minDesignLLtruck, &maxDesignLLtruck );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetLiveLoadMoment( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minFatigueLL, &maxFatigueLL, &minFatigueLLtruck, &maxFatigueLLtruck );
         }

         if ( bPermit )
         {
            pForces2->GetLiveLoadMoment( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minPermitLL, &maxPermitLL, &minPermitLLtruck, &maxPermitLLtruck );
         }
      }


      // write out the results
      std::vector<pgsPointOfInterest>::const_iterator i;
      long index = 0;
      for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
      {
         const pgsPointOfInterest& poi = *i;

         int col = 0;

         Float64 end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

         (*p_table)(row,col++) << location.SetValue( poi, end_size );
         (*p_table)(row,col++) << moment.SetValue( girder[index] );
         (*p_table)(row,col++) << moment.SetValue( diaphragm[index] );

         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            (*p_table)(row,col++) << moment.SetValue( maxSlab[index] );
            (*p_table)(row,col++) << moment.SetValue( minSlab[index] );
         }
         else
         {
            (*p_table)(row,col++) << moment.SetValue( maxSlab[index] );
         }

         if ( bDeckPanels )
         {
            if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
            {
               (*p_table)(row,col++) << moment.SetValue( maxDeckPanel[index] );
               (*p_table)(row,col++) << moment.SetValue( minDeckPanel[index] );
            }
            else
            {
               (*p_table)(row,col++) << moment.SetValue( maxDeckPanel[index] );
            }
         }

         if ( analysisType == pgsTypes::Envelope )
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << moment.SetValue( maxSidewalk[index] );
               (*p_table)(row,col++) << moment.SetValue( minSidewalk[index] );
            }

            (*p_table)(row,col++) << moment.SetValue( maxTrafficBarrier[index] );
            (*p_table)(row,col++) << moment.SetValue( minTrafficBarrier[index] );
            (*p_table)(row,col++) << moment.SetValue( maxOverlay[index] );
            (*p_table)(row,col++) << moment.SetValue( minOverlay[index] );
         }
         else
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << moment.SetValue( maxSidewalk[index] );
            }

            (*p_table)(row,col++) << moment.SetValue( maxTrafficBarrier[index] );
            (*p_table)(row,col++) << moment.SetValue( maxOverlay[index] );
         }

         if ( bPedLoading )
         {
            (*p_table)(row,col++) << moment.SetValue( maxPedestrian[index] );
            (*p_table)(row,col++) << moment.SetValue( minPedestrian[index] );
         }

         (*p_table)(row,col) << moment.SetValue( maxDesignLL[index] );

         if ( bIndicateControllingLoad && 0 < maxDesignLLtruck.size() )
            (*p_table)(row,col) << rptNewLine << "(D" << maxDesignLLtruck[index] << ")";

         col++;

         (*p_table)(row,col) << moment.SetValue( minDesignLL[index] );
         
         if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size() )
            (*p_table)(row,col) << rptNewLine << "(D" << minDesignLLtruck[index] << ")";

         col++;

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*p_table)(row,col) << moment.SetValue( maxFatigueLL[index] );

            if ( bIndicateControllingLoad && 0 < maxFatigueLLtruck.size() )
               (*p_table)(row,col) << rptNewLine << "(F" << maxFatigueLLtruck[index] << ")";

            col++;

            (*p_table)(row,col) << moment.SetValue( minFatigueLL[index] );
            
            if ( bIndicateControllingLoad && 0 < minFatigueLLtruck.size() )
               (*p_table)(row,col) << rptNewLine << "(F" << minFatigueLLtruck[index] << ")";

            col++;
         }

         if ( bPermit )
         {
            (*p_table)(row,col) << moment.SetValue( maxPermitLL[index] );
            if ( bIndicateControllingLoad && 0 < maxPermitLLtruck.size() )
               (*p_table)(row,col) << rptNewLine << "(P" << maxPermitLLtruck[index] << ")";

            col++;

            (*p_table)(row,col) << moment.SetValue( minPermitLL[index] );
            if ( bIndicateControllingLoad && 0 < minPermitLLtruck.size() )
               (*p_table)(row,col) << rptNewLine << "(P" << minPermitLLtruck[index] << ")";

            col++;
         }

         row++;
      }
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CProductMomentsTable::MakeCopy(const CProductMomentsTable& rOther)
{
   // Add copy code here...
}

void CProductMomentsTable::MakeAssignment(const CProductMomentsTable& rOther)
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
bool CProductMomentsTable::AssertValid() const
{
   return true;
}

void CProductMomentsTable::Dump(dbgDumpContext& os) const
{
   os << "Dump for CProductMomentsTable" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CProductMomentsTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CProductMomentsTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CProductMomentsTable");

   TESTME_EPILOG("CProductMomentsTable");
}
#endif // _UNITTEST
