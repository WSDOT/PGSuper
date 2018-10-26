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
#include <Reporting\ProductShearTable.h>
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
   CProductShearTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProductShearTable::CProductShearTable()
{
}

CProductShearTable::CProductShearTable(const CProductShearTable& rOther)
{
   MakeCopy(rOther);
}

CProductShearTable::~CProductShearTable()
{
}

//======================== OPERATORS  =======================================
CProductShearTable& CProductShearTable::operator= (const CProductShearTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CProductShearTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsTypes::AnalysisType analysisType,
                                      bool bIndicateControllingLoad,IDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear, pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bDeckPanels = (pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP ? true : false);
   pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType nSpans    = (span == ALL_SPANS ? pBridge->GetSpanCount() : startSpan+1 );
 
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE2(pBroker,IProductForces2,pForces2);
   GET_IFACE2(pBroker,IProductLoads,pLoads);
   bool bPedLoading = pLoads->HasPedestrianLoad(startSpan,gdr);
   bool bSidewalk = pLoads->HasSidewalkLoad(startSpan,gdr);
   bool bShearKey = pLoads->HasShearKeyLoad(startSpan,gdr);

   pgsTypes::Stage continuity_stage = pgsTypes::BridgeSite2;
   SpanIndexType spanIdx;
   for ( spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      pgsTypes::Stage left_stage,right_stage;
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

   if ( bShearKey )
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

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Shears");
   RowIndexType row = ConfigureProductLoadTableHeading<rptForceUnitTag,unitmgtForceData>(p_table,false,bDeckPanels,bSidewalk,bShearKey,bPedLoading,bPermit,analysisType,continuity_stage,pDisplayUnits,pDisplayUnits->GetShearUnit());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   for ( spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( pgsTypes::BridgeSite1, spanIdx, gdr, POI_TABULAR );

      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);

      Float64 end_size = pBridge->GetGirderStartConnectionLength(spanIdx,gdrIdx);

      pgsTypes::Stage girderLoadStage = pLoads->GetGirderDeadLoadStage(spanIdx,gdrIdx);

      // Get the results for this span (it is faster to get them as a vector rather than individually)
      std::vector<sysSectionValue> girder = pForces2->GetShear(girderLoadStage,pftGirder,vPoi,SimpleSpan);
      std::vector<sysSectionValue> diaphragm = pForces2->GetShear(pgsTypes::BridgeSite1,pftDiaphragm,vPoi,SimpleSpan);

      std::vector<sysSectionValue> minSlab, maxSlab;
      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      {
         maxSlab = pForces2->GetShear( pgsTypes::BridgeSite1, pftSlab, vPoi, MaxSimpleContinuousEnvelope );
         minSlab = pForces2->GetShear( pgsTypes::BridgeSite1, pftSlab, vPoi, MinSimpleContinuousEnvelope );
      }
      else
      {
         maxSlab = pForces2->GetShear( pgsTypes::BridgeSite1, pftSlab, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
      }

      std::vector<sysSectionValue> minDeckPanel, maxDeckPanel;
      if ( bDeckPanels )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            maxDeckPanel = pForces2->GetShear( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, MaxSimpleContinuousEnvelope );
            minDeckPanel = pForces2->GetShear( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, MinSimpleContinuousEnvelope );
         }
         else
         {
            maxDeckPanel = pForces2->GetShear( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         }
      }

      std::vector<sysSectionValue> dummy;
      std::vector<sysSectionValue> minOverlay, maxOverlay;
      std::vector<sysSectionValue> minTrafficBarrier, maxTrafficBarrier;
      std::vector<sysSectionValue> minSidewalk, maxSidewalk;
      std::vector<sysSectionValue> minShearKey, maxShearKey;
      std::vector<sysSectionValue> minPedestrian, maxPedestrian;
      std::vector<sysSectionValue> minDesignLL, maxDesignLL;
      std::vector<sysSectionValue> minFatigueLL, maxFatigueLL;
      std::vector<sysSectionValue> minPermitLL, maxPermitLL;

      std::vector<long> dummyTruck, minDesignLLtruck, maxDesignLLtruck, minFatigueLLtruck, maxFatigueLLtruck, minPermitLLtruck, maxPermitLLtruck;
      
      if (analysisType == pgsTypes::Envelope)
      {
         if ( bSidewalk )
         {
            maxSidewalk = pForces2->GetShear( pgsTypes::BridgeSite2, pftSidewalk, vPoi, MaxSimpleContinuousEnvelope );
            minSidewalk = pForces2->GetShear( pgsTypes::BridgeSite2, pftSidewalk, vPoi, MinSimpleContinuousEnvelope );
         }

         if ( bShearKey )
         {
            maxShearKey = pForces2->GetShear( pgsTypes::BridgeSite1, pftShearKey, vPoi, MaxSimpleContinuousEnvelope );
            minShearKey = pForces2->GetShear( pgsTypes::BridgeSite1, pftShearKey, vPoi, MinSimpleContinuousEnvelope );
         }

         maxTrafficBarrier = pForces2->GetShear( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, MaxSimpleContinuousEnvelope );
         minTrafficBarrier = pForces2->GetShear( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, MinSimpleContinuousEnvelope );
         maxOverlay = pForces2->GetShear( overlay_stage, pftOverlay, vPoi, MaxSimpleContinuousEnvelope );
         minOverlay = pForces2->GetShear( overlay_stage, pftOverlay, vPoi, MinSimpleContinuousEnvelope );

         if ( bPedLoading )
         {
            pForces2->GetLiveLoadShear( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxPedestrian );
            pForces2->GetLiveLoadShear( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minPedestrian, &dummy );
         }

         pForces2->GetLiveLoadShear( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
         pForces2->GetLiveLoadShear( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetLiveLoadShear( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxFatigueLL, &dummyTruck, &maxFatigueLLtruck );
            pForces2->GetLiveLoadShear( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minFatigueLL, &dummy, &minFatigueLLtruck, &dummyTruck );
         }

         if ( bPermit )
         {
            pForces2->GetLiveLoadShear( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxPermitLL, &dummyTruck, &maxPermitLLtruck );
            pForces2->GetLiveLoadShear( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minPermitLL, &dummy, &minPermitLLtruck, &dummyTruck );
         }
      }
      else
      {
         if ( bSidewalk )
         {
            maxSidewalk = pForces2->GetShear( pgsTypes::BridgeSite2, pftSidewalk, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         }

         if ( bShearKey )
         {
            maxShearKey = pForces2->GetShear( pgsTypes::BridgeSite1, pftShearKey, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         }

         maxTrafficBarrier = pForces2->GetShear( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         maxOverlay = pForces2->GetShear( overlay_stage, pftOverlay, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );

         if ( bPedLoading )
         {
            pForces2->GetLiveLoadShear( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minPedestrian, &maxPedestrian );
         }

         pForces2->GetLiveLoadShear( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minDesignLL, &maxDesignLL, &minDesignLLtruck, &maxDesignLLtruck );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetLiveLoadShear( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minFatigueLL, &maxFatigueLL, &minFatigueLLtruck, &maxFatigueLLtruck );
         }

         if ( bPermit )
         {
            pForces2->GetLiveLoadShear( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minPermitLL, &maxPermitLL, &minPermitLLtruck, &maxPermitLLtruck );
         }
      }


      // write out the results
      std::vector<pgsPointOfInterest>::const_iterator i;
      long index = 0;
      for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
      {
         const pgsPointOfInterest& poi = *i;

         int col = 0;
         (*p_table)(row,col++) << location.SetValue( poi, end_size );
         (*p_table)(row,col++) << shear.SetValue( girder[index] );
         (*p_table)(row,col++) << shear.SetValue( diaphragm[index] );

         if ( bShearKey )
         {
            if ( analysisType == pgsTypes::Envelope )
            {
               (*p_table)(row,col++) << shear.SetValue( maxShearKey[index] );
               (*p_table)(row,col++) << shear.SetValue( minShearKey[index] );
            }
            else
            {
               (*p_table)(row,col++) << shear.SetValue( maxShearKey[index] );
            }
         }

         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            (*p_table)(row,col++) << shear.SetValue( maxSlab[index] );
            (*p_table)(row,col++) << shear.SetValue( minSlab[index] );
         }
         else
         {
            (*p_table)(row,col++) << shear.SetValue( maxSlab[index] );
         }

         if ( bDeckPanels )
         {
            if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
            {
               (*p_table)(row,col++) << shear.SetValue( maxDeckPanel[index] );
               (*p_table)(row,col++) << shear.SetValue( minDeckPanel[index] );
            }
            else
            {
               (*p_table)(row,col++) << shear.SetValue( maxDeckPanel[index] );
            }
         }

         if ( analysisType == pgsTypes::Envelope )
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << shear.SetValue( maxSidewalk[index] );
               (*p_table)(row,col++) << shear.SetValue( minSidewalk[index] );
            }

            (*p_table)(row,col++) << shear.SetValue( maxTrafficBarrier[index] );
            (*p_table)(row,col++) << shear.SetValue( minTrafficBarrier[index] );
            (*p_table)(row,col++) << shear.SetValue( maxOverlay[index] );
            (*p_table)(row,col++) << shear.SetValue( minOverlay[index] );
         }
         else
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << shear.SetValue( maxSidewalk[index] );
            }

            (*p_table)(row,col++) << shear.SetValue( maxTrafficBarrier[index] );
            (*p_table)(row,col++) << shear.SetValue( maxOverlay[index] );
         }

         if ( bPedLoading )
         {
            (*p_table)(row,col++) << shear.SetValue( maxPedestrian[index] );
            (*p_table)(row,col++) << shear.SetValue( minPedestrian[index] );
         }

         (*p_table)(row,col) << shear.SetValue( maxDesignLL[index] );
         if ( bIndicateControllingLoad && 0 < maxDesignLLtruck.size() )
            (*p_table)(row,col) << rptNewLine << "(D" << maxDesignLLtruck[index] << ")";

         col++;

         (*p_table)(row,col) << shear.SetValue( minDesignLL[index] );
         if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size())
            (*p_table)(row,col) << rptNewLine << "(D" << minDesignLLtruck[index] << ")";

         col++;

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*p_table)(row,col) << shear.SetValue( maxFatigueLL[index] );
            if ( bIndicateControllingLoad && 0 < maxFatigueLLtruck.size() )
               (*p_table)(row,col) << rptNewLine << "(F" << maxFatigueLLtruck[index] << ")";

            col++;

            (*p_table)(row,col) << shear.SetValue( minFatigueLL[index] );
            if ( bIndicateControllingLoad && 0 < minFatigueLLtruck.size())
               (*p_table)(row,col) << rptNewLine << "(F" << minFatigueLLtruck[index] << ")";

            col++;
         }

         if ( bPermit )
         {
            (*p_table)(row,col) << shear.SetValue( maxPermitLL[index] );
            if ( bIndicateControllingLoad && 0 < maxPermitLLtruck.size())
               (*p_table)(row,col) << rptNewLine << "(P" << maxPermitLLtruck[index] << ")";

            col++;

            (*p_table)(row,col) << shear.SetValue( minPermitLL[index] );
            if ( bIndicateControllingLoad && 0 < minPermitLLtruck.size())
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
void CProductShearTable::MakeCopy(const CProductShearTable& rOther)
{
   // Add copy code here...
}

void CProductShearTable::MakeAssignment(const CProductShearTable& rOther)
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
bool CProductShearTable::AssertValid() const
{
   return true;
}

void CProductShearTable::Dump(dbgDumpContext& os) const
{
   os << "Dump for CProductShearTable" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CProductShearTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CProductShearTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CProductShearTable");

   TESTME_EPILOG("CProductShearTable");
}
#endif // _UNITTEST
