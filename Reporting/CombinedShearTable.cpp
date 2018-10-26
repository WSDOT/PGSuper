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
#include <Reporting\CombinedShearTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\MVRChapterBuilder.h>
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
   CCombinedShearTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCombinedShearTable::CCombinedShearTable()
{
}

CCombinedShearTable::CCombinedShearTable(const CCombinedShearTable& rOther)
{
   MakeCopy(rOther);
}

CCombinedShearTable::~CCombinedShearTable()
{
}

//======================== OPERATORS  =======================================
CCombinedShearTable& CCombinedShearTable::operator= (const CCombinedShearTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CCombinedShearTable::Build(IBroker* pBroker,rptChapter* pChapter,
                                       SpanIndexType span,GirderIndexType girder,
                                       IDisplayUnits* pDispUnits,
                                       pgsTypes::Stage stage,pgsTypes::AnalysisType analysisType) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDispUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear, pDispUnits->GetShearUnit(), false );

   if ( stage == pgsTypes::CastingYard )
      location.MakeGirderPoi();
   else
      location.MakeSpanPoi();

   GET_IFACE2(pBroker,IBridge,pBridge);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = 0;

   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType nSpans    = (span == ALL_SPANS ? pBridge->GetSpanCount() : startSpan+1 );
 
   pgsTypes::Stage continuity_stage = pgsTypes::BridgeSite2;
   SpanIndexType spanIdx = 0;
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

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   bool bPedLoading = pProductForces->HasPedestrianLoad(startSpan,girder);

   ColumnIndexType col = 0;
   RowIndexType row = CreateCombinedLoadingTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table,"Shear",false,bPermit,bPedLoading,stage,continuity_stage,analysisType,pDispUnits,pDispUnits->GetShearUnit());

   *p << p_table;

   RowIndexType row2 = 1;
   rptRcTable* p_table2 = 0;
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

      p_table2 = pgsReportStyleHolder::CreateDefaultTable(nCols,"");
      row2 = ConfigureLimitStateTableHeading<rptForceUnitTag,unitmgtForceData>(p_table2,false,bPermit,analysisType,pDispUnits,pDispUnits->GetGeneralForceUnit());
      *p << p_table2;
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   GET_IFACE2(pBroker,ICombinedForces,pForces);
   GET_IFACE2(pBroker,ICombinedForces2,pForces2);
   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);
   GET_IFACE2(pBroker,ILimitStateForces2,pLsForces2);

   // Fill up the table
   for ( spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( stage, spanIdx, girder, POI_ALLACTIONS | POI_TABULAR | POI_CRITSECTSHEAR1 | POI_CRITSECTSHEAR2, POIFIND_OR);

      std::vector<sysSectionValue> dummy;
      std::vector<sysSectionValue> minServiceI, maxServiceI;
      std::vector<sysSectionValue> minDCinc, maxDCinc;
      std::vector<sysSectionValue> minDCcum, maxDCcum;
      std::vector<sysSectionValue> minDWinc, maxDWinc;
      std::vector<sysSectionValue> minDWcum, maxDWcum;
      std::vector<sysSectionValue> minPedestrianLL, maxPedestrianLL;
      std::vector<sysSectionValue> minDesignLL, maxDesignLL;
      std::vector<sysSectionValue> minFatigueLL, maxFatigueLL;
      std::vector<sysSectionValue> minPermitLL, maxPermitLL;

      if ( stage == pgsTypes::CastingYard || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
      {
         maxDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, SimpleSpan );
         pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, SimpleSpan, &dummy, &maxServiceI );
      }
      else if ( stage == pgsTypes::BridgeSite1 )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            maxDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, MaxSimpleContinuousEnvelope );
            minDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, MinSimpleContinuousEnvelope );
            maxDWinc = pForces2->GetShear( lcDW, stage, vPoi, ctIncremental, MaxSimpleContinuousEnvelope );
            minDWinc = pForces2->GetShear( lcDW, stage, vPoi, ctIncremental, MinSimpleContinuousEnvelope );
            maxDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, MaxSimpleContinuousEnvelope );
            minDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, MinSimpleContinuousEnvelope );
            maxDWcum = pForces2->GetShear( lcDW, stage, vPoi, ctCummulative, MaxSimpleContinuousEnvelope );
            minDWcum = pForces2->GetShear( lcDW, stage, vPoi, ctCummulative, MinSimpleContinuousEnvelope );

            pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxServiceI );
            pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, MinSimpleContinuousEnvelope, &minServiceI, &dummy );
         }
         else
         {
            maxDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, SimpleSpan );
            maxDWinc = pForces2->GetShear( lcDW, stage, vPoi, ctIncremental, SimpleSpan );
            maxDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, SimpleSpan );
            maxDWcum = pForces2->GetShear( lcDW, stage, vPoi, ctCummulative, SimpleSpan );
            pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, SimpleSpan, &minServiceI, &maxServiceI );
         }
      }
      else if ( stage == pgsTypes::BridgeSite2 )
      {
         if ( analysisType == pgsTypes::Envelope )
         {
            maxDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, MaxSimpleContinuousEnvelope );
            minDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, MinSimpleContinuousEnvelope );
            maxDWinc = pForces2->GetShear( lcDW, stage, vPoi, ctIncremental, MaxSimpleContinuousEnvelope );
            minDWinc = pForces2->GetShear( lcDW, stage, vPoi, ctIncremental, MinSimpleContinuousEnvelope );
            maxDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, MaxSimpleContinuousEnvelope );
            minDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, MinSimpleContinuousEnvelope );
            maxDWcum = pForces2->GetShear( lcDW, stage, vPoi, ctCummulative, MaxSimpleContinuousEnvelope );
            minDWcum = pForces2->GetShear( lcDW, stage, vPoi, ctCummulative, MinSimpleContinuousEnvelope );

            pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxServiceI );
            pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, MinSimpleContinuousEnvelope, &minServiceI, &dummy );
         }
         else
         {
            BridgeAnalysisType bat = ( analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
            maxDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, bat );
            maxDWinc = pForces2->GetShear( lcDW, stage, vPoi, ctIncremental, bat );
            maxDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, bat );
            maxDWcum = pForces2->GetShear( lcDW, stage, vPoi, ctCummulative, bat );

            pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, bat, &minServiceI, &maxServiceI );
         }
      }
      else
      {
         // stage == pgsTypes::BridgeSite3
         if ( analysisType == pgsTypes::Envelope )
         {
            maxDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, MaxSimpleContinuousEnvelope );
            minDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, MinSimpleContinuousEnvelope );
            maxDWinc = pForces2->GetShear( lcDW, stage, vPoi, ctIncremental, MaxSimpleContinuousEnvelope );
            minDWinc = pForces2->GetShear( lcDW, stage, vPoi, ctIncremental, MinSimpleContinuousEnvelope );
            maxDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, MaxSimpleContinuousEnvelope );
            minDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, MinSimpleContinuousEnvelope );
            maxDWcum = pForces2->GetShear( lcDW, stage, vPoi, ctCummulative, MaxSimpleContinuousEnvelope );
            minDWcum = pForces2->GetShear( lcDW, stage, vPoi, ctCummulative, MinSimpleContinuousEnvelope );

            if ( bPedLoading )
            {
               pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxPedestrianLL );
               pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, &minPedestrianLL, &dummy );
            }

            pForces2->GetCombinedLiveLoadShear( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxDesignLL );
            pForces2->GetCombinedLiveLoadShear( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, &minDesignLL, &dummy );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces2->GetCombinedLiveLoadShear( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxFatigueLL );
               pForces2->GetCombinedLiveLoadShear( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, &minFatigueLL, &dummy );
            }

            if ( bPermit )
            {
               pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxPermitLL );
               pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, &minPermitLL, &dummy );
            }
         }
         else
         {
            BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
            maxDCinc = pForces2->GetShear( lcDC, stage, vPoi, ctIncremental, bat );
            maxDWinc = pForces2->GetShear( lcDW, stage, vPoi, ctIncremental, bat );
            maxDCcum = pForces2->GetShear( lcDC, stage, vPoi, ctCummulative, bat );
            maxDWcum = pForces2->GetShear( lcDW, stage, vPoi, ctCummulative, bat );

            if ( bPedLoading )
            {
               pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, bat, &minPedestrianLL, &maxPedestrianLL );
            }

            pForces2->GetCombinedLiveLoadShear( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, bat, &minDesignLL, &maxDesignLL );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces2->GetCombinedLiveLoadShear( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, bat, &minFatigueLL, &maxFatigueLL );
            }

            if ( bPermit )
            {
               pForces2->GetCombinedLiveLoadShear( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, bat, &minPermitLL, &maxPermitLL );
            }
         }
      }



      std::vector<pgsPointOfInterest>::const_iterator i;
      long index = 0;
      for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
      {
         const pgsPointOfInterest& poi = *i;

         col = 0;

         Float64 end_size = 0 ;
         if ( stage != pgsTypes::CastingYard )
            end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

         (*p_table)(row,col++) << location.SetValue( poi, end_size );

         if ( stage == pgsTypes::CastingYard  || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
         {
            (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
            (*p_table)(row,col++) << shear.SetValue( maxServiceI[index] );
         }
         else if ( stage == pgsTypes::BridgeSite1 )
         {
            if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
            {
               (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minDWcum[index] );

               (*p_table)(row,col++) << shear.SetValue( maxServiceI[index] );
               (*p_table)(row,col++) << shear.SetValue( minServiceI[index] );
            }
            else
            {
               (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWcum[index] );

               (*p_table)(row,col++) << shear.SetValue( maxServiceI[index] );
            }
         }
         else if ( stage == pgsTypes::BridgeSite2 )
         {
            if ( analysisType == pgsTypes::Envelope )
            {
               (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minDWcum[index] );

               (*p_table)(row,col++) << shear.SetValue( maxServiceI[index] );
               (*p_table)(row,col++) << shear.SetValue( minServiceI[index] );
            }
            else
            {
               (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWcum[index] );

               (*p_table)(row,col++) << shear.SetValue( maxServiceI[index] );
            }

         }
         else
         {
            // stage == pgsTypes::BridgeSite3
            if ( analysisType == pgsTypes::Envelope )
            {
               (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minDWcum[index] );
            }
            else
            {
               (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDCcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxDWcum[index] );
            }

            if ( bPedLoading )
            {
               (*p_table)(row,col++) << shear.SetValue( maxPedestrianLL[index] );
               (*p_table)(row,col++) << shear.SetValue( minPedestrianLL[index] );
            }

            (*p_table)(row,col++) << shear.SetValue( maxDesignLL[index] );
            (*p_table)(row,col++) << shear.SetValue( minDesignLL[index] );
         
            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col++) << shear.SetValue( maxFatigueLL[index] );
               (*p_table)(row,col++) << shear.SetValue( minFatigueLL[index] );
            }

            if ( bPermit )
            {
               (*p_table)(row,col++) << shear.SetValue( maxPermitLL[index] );
               (*p_table)(row,col++) << shear.SetValue( minPermitLL[index] );
            }
         }

         row++;
      }

      // create second table for BSS3 Limit states
      if ( stage == pgsTypes::BridgeSite3 )
      {
         std::vector<sysSectionValue> dummy;
         std::vector<sysSectionValue> minServiceI,   maxServiceI;
         std::vector<sysSectionValue> minServiceIA,  maxServiceIA;
         std::vector<sysSectionValue> minServiceIII, maxServiceIII;
         std::vector<sysSectionValue> minFatigueI,   maxFatigueI;
         std::vector<sysSectionValue> minStrengthI,  maxStrengthI;
         std::vector<sysSectionValue> minStrengthII, maxStrengthII;
         if ( analysisType == pgsTypes::Envelope )
         {
            pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxServiceI );
            pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, MinSimpleContinuousEnvelope, &minServiceI, &dummy );

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
            {
               pLsForces2->GetShear( pgsTypes::ServiceIA, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxServiceIA );
               pLsForces2->GetShear( pgsTypes::ServiceIA, stage, vPoi, MinSimpleContinuousEnvelope, &minServiceIA, &dummy );
            }
            else
            {
               pLsForces2->GetShear( pgsTypes::FatigueI, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxFatigueI );
               pLsForces2->GetShear( pgsTypes::FatigueI, stage, vPoi, MinSimpleContinuousEnvelope, &minFatigueI, &dummy );
            }

            pLsForces2->GetShear( pgsTypes::ServiceIII, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxServiceIII );
            pLsForces2->GetShear( pgsTypes::ServiceIII, stage, vPoi, MinSimpleContinuousEnvelope, &minServiceIII, &dummy );

            pLsForces2->GetShear( pgsTypes::StrengthI, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxStrengthI );
            pLsForces2->GetShear( pgsTypes::StrengthI, stage, vPoi, MinSimpleContinuousEnvelope, &minStrengthI, &dummy );

            if ( bPermit )
            {
               pLsForces2->GetShear( pgsTypes::StrengthII, stage, vPoi, MaxSimpleContinuousEnvelope, &dummy, &maxStrengthII );
               pLsForces2->GetShear( pgsTypes::StrengthII, stage, vPoi, MinSimpleContinuousEnvelope, &minStrengthII, &dummy );
            }
         }
         else
         {
            BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
            pLsForces2->GetShear( pgsTypes::ServiceI, stage, vPoi, bat, &minServiceI, &maxServiceI );

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
               pLsForces2->GetShear( pgsTypes::ServiceIA, stage, vPoi, bat, &minServiceIA, &maxServiceIA );
            else
               pLsForces2->GetShear( pgsTypes::FatigueI, stage, vPoi, bat, &minFatigueI, &maxFatigueI );

            pLsForces2->GetShear( pgsTypes::ServiceIII, stage, vPoi, bat, &minServiceIII, &maxServiceIII );
            pLsForces2->GetShear( pgsTypes::StrengthI, stage, vPoi, bat, &minStrengthI, &maxStrengthI );

            if ( bPermit )
            {
               pLsForces2->GetShear( pgsTypes::StrengthII, stage, vPoi, bat, &minStrengthII, &maxStrengthII );
            }
         }

         std::vector<pgsPointOfInterest>::const_iterator i;
         long index = 0;
         for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
         {
            col = 0;

            const pgsPointOfInterest& poi = *i;

            Float64 end_size = 0 ;
            if ( stage != pgsTypes::CastingYard )
               end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

            (*p_table2)(row2,col++) << location.SetValue( poi, end_size );

            if ( analysisType == pgsTypes::Envelope )
            {
               (*p_table2)(row2,col++) << shear.SetValue( maxServiceI[index] );
               (*p_table2)(row2,col++) << shear.SetValue( minServiceI[index] );

               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
               {
                  (*p_table2)(row2,col++) << shear.SetValue( maxServiceIA[index] );
                  (*p_table2)(row2,col++) << shear.SetValue( minServiceIA[index] );
               }

               (*p_table2)(row2,col++) << shear.SetValue( maxServiceIII[index] );
               (*p_table2)(row2,col++) << shear.SetValue( minServiceIII[index] );

               if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
               {
                  (*p_table2)(row2,col++) << shear.SetValue( maxFatigueI[index] );
                  (*p_table2)(row2,col++) << shear.SetValue( minFatigueI[index] );
               }

               (*p_table2)(row2,col++) << shear.SetValue( maxStrengthI[index] );
               (*p_table2)(row2,col++) << shear.SetValue( minStrengthI[index] );

               if ( bPermit )
               {
                  (*p_table2)(row2,col++) << shear.SetValue( maxStrengthII[index] );
                  (*p_table2)(row2,col++) << shear.SetValue( minStrengthII[index] );
               }
            }
            else
            {
               (*p_table2)(row2,col++) << shear.SetValue( maxServiceI[index] );

               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
                  (*p_table2)(row2,col++) << shear.SetValue( maxServiceIA[index] );

               (*p_table2)(row2,col++) << shear.SetValue( maxServiceIII[index] );

               if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
                  (*p_table2)(row2,col++) << shear.SetValue( maxFatigueI[index] );

               (*p_table2)(row2,col++) << shear.SetValue( maxStrengthI[index] );
               (*p_table2)(row2,col++) << shear.SetValue( minStrengthI[index] );

               if ( bPermit )
               {
                  (*p_table2)(row2,col++) << shear.SetValue( maxStrengthII[index] );
                  (*p_table2)(row2,col++) << shear.SetValue( minStrengthII[index] );
               }
            }

            row2++;
         }
      }
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CCombinedShearTable::MakeCopy(const CCombinedShearTable& rOther)
{
   // Add copy code here...
}

void CCombinedShearTable::MakeAssignment(const CCombinedShearTable& rOther)
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
bool CCombinedShearTable::AssertValid() const
{
   return true;
}

void CCombinedShearTable::Dump(dbgDumpContext& os) const
{
   os << "Dump for CCombinedShearTable" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CCombinedShearTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CCombinedShearTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CCombinedShearTable");

   TESTME_EPILOG("CCombinedShearTable");
}
#endif // _UNITTEST
