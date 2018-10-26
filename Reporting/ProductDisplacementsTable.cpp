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
#include <Reporting\ProductDisplacementsTable.h>
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
   CProductDisplacementsTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProductDisplacementsTable::CProductDisplacementsTable()
{
}

CProductDisplacementsTable::CProductDisplacementsTable(const CProductDisplacementsTable& rOther)
{
   MakeCopy(rOther);
}

CProductDisplacementsTable::~CProductDisplacementsTable()
{
}

//======================== OPERATORS  =======================================
CProductDisplacementsTable& CProductDisplacementsTable::operator= (const CProductDisplacementsTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CProductDisplacementsTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsTypes::AnalysisType analysisType,
                                              bool bIndicateControllingLoad,IDisplayUnits* pDispUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDispUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, displacement, pDispUnits->GetDisplacementUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bDeckPanels = (pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP ? true : false);
   pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType nSpans    = (span == ALL_SPANS ? pBridge->GetSpanCount() : startSpan+1 );
 
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

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

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Displacements");
   RowIndexType row = ConfigureProductLoadTableHeading<rptLengthUnitTag,unitmgtLengthData>(p_table,false,bDeckPanels,bSidewalk,bPedLoading,bPermit,analysisType,continuity_stage,pDispUnits,pDispUnits->GetDisplacementUnit());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   for ( spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( pgsTypes::BridgeSite1, spanIdx, gdr, POI_TABULAR );

      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);

      pgsTypes::Stage girderLoadStage = pForces->GetGirderDeadLoadStage(spanIdx,gdrIdx);

      // Get the results for this span (it is faster to get them as a vector rather than individually)
      std::vector<Float64> girder = pForces2->GetDisplacement(girderLoadStage,pftGirder,vPoi,SimpleSpan);
      std::vector<Float64> diaphragm = pForces2->GetDisplacement(pgsTypes::BridgeSite1,pftDiaphragm,vPoi,SimpleSpan);

      std::vector<Float64> minSlab, maxSlab;
      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      {
         maxSlab = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftSlab, vPoi, MaxSimpleContinuousEnvelope );
         minSlab = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftSlab, vPoi, MinSimpleContinuousEnvelope );
      }
      else
      {
         maxSlab = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftSlab, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
      }

      std::vector<Float64> minDeckPanel, maxDeckPanel;
      if ( bDeckPanels )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            maxDeckPanel = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, MaxSimpleContinuousEnvelope );
            minDeckPanel = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, MinSimpleContinuousEnvelope );
         }
         else
         {
            maxDeckPanel = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
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
            maxSidewalk = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftSidewalk, vPoi, MaxSimpleContinuousEnvelope );
            minSidewalk = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftSidewalk, vPoi, MinSimpleContinuousEnvelope );
         }

         maxTrafficBarrier = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, MaxSimpleContinuousEnvelope );
         minTrafficBarrier = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, MinSimpleContinuousEnvelope );
         maxOverlay = pForces2->GetDisplacement( overlay_stage, pftOverlay, vPoi, MaxSimpleContinuousEnvelope );
         minOverlay = pForces2->GetDisplacement( overlay_stage, pftOverlay, vPoi, MinSimpleContinuousEnvelope );

         if ( bPedLoading )
         {
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxPedestrian );
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minPedestrian, &dummy );
         }

         pForces2->GetLiveLoadDisplacement( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
         pForces2->GetLiveLoadDisplacement( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxFatigueLL, &dummyTruck, &maxFatigueLLtruck );
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minFatigueLL, &dummy, &minFatigueLLtruck, &dummyTruck );
         }

         if ( bPermit )
         {
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxPermitLL, &dummyTruck, &maxPermitLLtruck );
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minPermitLL, &dummy, &minPermitLLtruck, &dummyTruck );
         }
      }
      else
      {
         if ( bSidewalk )
         {
            maxSidewalk = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftSidewalk, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         }

         maxTrafficBarrier = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         maxOverlay = pForces2->GetDisplacement( overlay_stage, pftOverlay, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );

         if ( bPedLoading )
         {
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minPedestrian, &maxPedestrian );
         }

         pForces2->GetLiveLoadDisplacement( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minDesignLL, &maxDesignLL, &minDesignLLtruck, &maxDesignLLtruck );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minFatigueLL, &maxFatigueLL, &minFatigueLLtruck, &maxFatigueLLtruck );
         }

         if ( bPermit )
         {
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minPermitLL, &maxPermitLL, &minPermitLLtruck, &maxPermitLLtruck );
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
         (*p_table)(row,col++) << displacement.SetValue( girder[index] );
         (*p_table)(row,col++) << displacement.SetValue( diaphragm[index] );

         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            (*p_table)(row,col++) << displacement.SetValue( maxSlab[index] );
            (*p_table)(row,col++) << displacement.SetValue( minSlab[index] );
         }
         else
         {
            (*p_table)(row,col++) << displacement.SetValue( maxSlab[index] );
         }

         if ( bDeckPanels )
         {
            if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
            {
               (*p_table)(row,col++) << displacement.SetValue( maxDeckPanel[index] );
               (*p_table)(row,col++) << displacement.SetValue( minDeckPanel[index] );
            }
            else
            {
               (*p_table)(row,col++) << displacement.SetValue( maxDeckPanel[index] );
            }
         }

         if ( analysisType == pgsTypes::Envelope )
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << displacement.SetValue( maxSidewalk[index] );
               (*p_table)(row,col++) << displacement.SetValue( minSidewalk[index] );
            }

            (*p_table)(row,col++) << displacement.SetValue( maxTrafficBarrier[index] );
            (*p_table)(row,col++) << displacement.SetValue( minTrafficBarrier[index] );
            (*p_table)(row,col++) << displacement.SetValue( maxOverlay[index] );
            (*p_table)(row,col++) << displacement.SetValue( minOverlay[index] );
         }
         else
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << displacement.SetValue( maxSidewalk[index] );
            }

            (*p_table)(row,col++) << displacement.SetValue( maxTrafficBarrier[index] );
            (*p_table)(row,col++) << displacement.SetValue( maxOverlay[index] );
         }

         if ( bPedLoading )
         {
            (*p_table)(row,col++) << displacement.SetValue( maxPedestrian[index] );
            (*p_table)(row,col++) << displacement.SetValue( minPedestrian[index] );
         }

         (*p_table)(row,col) << displacement.SetValue( maxDesignLL[index] );

         if ( bIndicateControllingLoad && 0 < maxDesignLLtruck.size() )
            (*p_table)(row,col) << rptNewLine << "(D" << maxDesignLLtruck[index] << ")";

         col++;

         (*p_table)(row,col) << displacement.SetValue( minDesignLL[index] );
         
         if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size() )
            (*p_table)(row,col) << rptNewLine << "(D" << minDesignLLtruck[index] << ")";

         col++;

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*p_table)(row,col) << displacement.SetValue( maxFatigueLL[index] );

            if ( bIndicateControllingLoad && 0 < maxFatigueLLtruck.size() )
               (*p_table)(row,col) << rptNewLine << "(F" << maxFatigueLLtruck[index] << ")";

            col++;

            (*p_table)(row,col) << displacement.SetValue( minFatigueLL[index] );
            
            if ( bIndicateControllingLoad && 0 < minFatigueLLtruck.size() )
               (*p_table)(row,col) << rptNewLine << "(F" << minFatigueLLtruck[index] << ")";

            col++;
         }


         if ( bPermit )
         {
            (*p_table)(row,col) << displacement.SetValue( maxPermitLL[index] );

            if ( bIndicateControllingLoad && 0 < maxPermitLLtruck.size())
               (*p_table)(row,col) << rptNewLine << "(P" << maxPermitLLtruck[index] << ")";

            col++;

            (*p_table)(row,col) << displacement.SetValue( minPermitLL[index] );

            if ( bIndicateControllingLoad && 0 < minPermitLLtruck.size())
               (*p_table)(row,col) << rptNewLine << "(P" << minPermitLLtruck[index] << ")";

            col++;
         }

         row++;
      }
   }

   return p_table;
}

rptRcTable* CProductDisplacementsTable::BuildLiveLoadTable(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                                           IDisplayUnits* pDispUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDispUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, displacement, pDispUnits->GetDisplacementUnit(), false );

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(4,"Displacements For The LRFD Optional Deflection Live Load (LRFD 3.6.1.3.2)");

   // Set up table headings
   (*p_table)(0,0) << COLHDR(RPT_LFT_SUPPORT_LOCATION,        rptLengthUnitTag, pDispUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR("D1",          rptLengthUnitTag, pDispUnits->GetDisplacementUnit() );
   (*p_table)(0,2) << COLHDR("D2",       rptLengthUnitTag, pDispUnits->GetDisplacementUnit() );
   (*p_table)(0,3) << COLHDR("D" << rptNewLine << "Controlling",            rptLengthUnitTag, pDispUnits->GetDisplacementUnit() );

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( pgsTypes::BridgeSite3, span, girder, POI_TABULAR );

   GET_IFACE2(pBroker,IProductForces,pForces);
   GET_IFACE2(pBroker,IBridge,pBridge);

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      (*p_table)(row,0) << location.SetValue( poi, end_size );

      Float64 min, max;
      pForces->GetDeflLiveLoadDisplacement( IProductForces::DesignTruckAlone, poi, &min, &max );
      (*p_table)(row,1) << displacement.SetValue( min );
      pForces->GetDeflLiveLoadDisplacement( IProductForces::Design25PlusLane, poi, &min, &max );
      (*p_table)(row,2) << displacement.SetValue( min );
      pForces->GetDeflLiveLoadDisplacement( IProductForces::DeflectionLiveLoadEnvelope, poi, &min, &max );
      (*p_table)(row,3) << displacement.SetValue( min );

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
void CProductDisplacementsTable::MakeCopy(const CProductDisplacementsTable& rOther)
{
   // Add copy code here...
}

void CProductDisplacementsTable::MakeAssignment(const CProductDisplacementsTable& rOther)
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
bool CProductDisplacementsTable::AssertValid() const
{
   return true;
}

void CProductDisplacementsTable::Dump(dbgDumpContext& os) const
{
   os << "Dump for CProductDisplacementsTable" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CProductDisplacementsTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CProductDisplacementsTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CProductDisplacementsTable");

   TESTME_EPILOG("CProductDisplacementsTable");
}
#endif // _UNITTEST
