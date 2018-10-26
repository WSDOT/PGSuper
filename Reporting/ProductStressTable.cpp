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
#include <Reporting\ProductStressTable.h>
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
   CProductStressTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProductStressTable::CProductStressTable()
{
}

CProductStressTable::CProductStressTable(const CProductStressTable& rOther)
{
   MakeCopy(rOther);
}

CProductStressTable::~CProductStressTable()
{
}

//======================== OPERATORS  =======================================
CProductStressTable& CProductStressTable::operator= (const CProductStressTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CProductStressTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsTypes::AnalysisType analysisType,
                                      IDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bDeckPanels = (pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP ? true : false);
   pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType nSpans    = (span == ALL_SPANS ? pBridge->GetSpanCount() : startSpan+1 );

   GET_IFACE2(pBroker,IProductLoads,pLoads);
   GET_IFACE2(pBroker,IProductForces2,pForces2);
   pgsTypes::Stage girderLoadStage = pLoads->GetGirderDeadLoadStage(span,gdr);
   bool bPedLoading = pLoads->HasPedestrianLoad(startSpan,gdr);
   bool bSidewalk = pLoads->HasSidewalkLoad(startSpan,gdr);
   bool bShearKey = pLoads->HasShearKeyLoad(startSpan,gdr);

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

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
      nCols += 2;

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

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Bridge Site Stress");
   RowIndexType row = ConfigureProductLoadTableHeading<rptStressUnitTag,unitmgtStressData>(p_table,false,bDeckPanels,bSidewalk,bShearKey,bPedLoading,bPermit,analysisType,continuity_stage,pDisplayUnits,pDisplayUnits->GetStressUnit());


   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   // Get all the tabular poi's for flexure and shear
   // Merge the two vectors to form one vector to report on.
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(pgsTypes::BridgeSite1,span,gdr, POI_TABULAR);

   std::vector<Float64> fTopGirder, fBotGirder;
   std::vector<Float64> fTopDiaphragm, fBotDiaphragm;
   std::vector<Float64> fTopMaxSlab, fTopMinSlab, fBotMaxSlab, fBotMinSlab;
   std::vector<Float64> fTopMaxSlabPanel, fTopMinSlabPanel, fBotMaxSlabPanel, fBotMinSlabPanel;
   std::vector<Float64> fTopMaxOverlay, fTopMinOverlay, fBotMaxOverlay, fBotMinOverlay;
   std::vector<Float64> fTopMaxSidewalk, fTopMinSidewalk, fBotMaxSidewalk, fBotMinSidewalk;
   std::vector<Float64> fTopMaxShearKey, fTopMinShearKey, fBotMaxShearKey, fBotMinShearKey;
   std::vector<Float64> fTopMaxTrafficBarrier, fTopMinTrafficBarrier, fBotMaxTrafficBarrier, fBotMinTrafficBarrier;
   std::vector<Float64> fTopMaxPedestrianLL, fBotMaxPedestrianLL;
   std::vector<Float64> fTopMinPedestrianLL, fBotMinPedestrianLL;
   std::vector<Float64> fTopMaxDesignLL, fBotMaxDesignLL;
   std::vector<Float64> fTopMinDesignLL, fBotMinDesignLL;
   std::vector<Float64> fTopMaxFatigueLL, fBotMaxFatigueLL;
   std::vector<Float64> fTopMinFatigueLL, fBotMinFatigueLL;
   std::vector<Float64> fTopMaxPermitLL, fBotMaxPermitLL;
   std::vector<Float64> fTopMinPermitLL, fBotMinPermitLL;
   std::vector<Float64> dummy1, dummy2;

   pForces2->GetStress( girderLoadStage, pftGirder, vPoi, SimpleSpan, &fTopGirder, &fBotGirder);
   pForces2->GetStress( pgsTypes::BridgeSite1, pftDiaphragm, vPoi, SimpleSpan, &fTopDiaphragm, &fBotDiaphragm);

   if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
   {
      pForces2->GetStress( pgsTypes::BridgeSite1, pftSlab, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxSlab, &fBotMaxSlab );
      pForces2->GetStress( pgsTypes::BridgeSite1, pftSlab, vPoi, MinSimpleContinuousEnvelope, &fTopMinSlab, &fBotMinSlab );
   }
   else
   {
      pForces2->GetStress( pgsTypes::BridgeSite1, pftSlab, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxSlab, &fBotMaxSlab );
   }

   if ( bDeckPanels )
   {
      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      {
         pForces2->GetStress( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxSlabPanel, &fBotMaxSlabPanel );
         pForces2->GetStress( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, MinSimpleContinuousEnvelope, &fTopMinSlabPanel, &fBotMinSlabPanel );
      }
      else
      {
         pForces2->GetStress( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxSlabPanel, &fBotMaxSlabPanel );
      }
   }

   if ( analysisType == pgsTypes::Envelope )
   {
      if ( bSidewalk )
      {
         pForces2->GetStress( pgsTypes::BridgeSite2, pftSidewalk, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxSidewalk, &fBotMaxSidewalk);
         pForces2->GetStress( pgsTypes::BridgeSite2, pftSidewalk, vPoi, MinSimpleContinuousEnvelope, &fTopMinSidewalk, &fBotMinSidewalk);
      }

      if ( bShearKey )
      {
         pForces2->GetStress( pgsTypes::BridgeSite1, pftShearKey, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxShearKey, &fBotMaxShearKey);
         pForces2->GetStress( pgsTypes::BridgeSite1, pftShearKey, vPoi, MinSimpleContinuousEnvelope, &fTopMinShearKey, &fBotMinShearKey);
      }

      pForces2->GetStress( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxTrafficBarrier, &fBotMaxTrafficBarrier);
      pForces2->GetStress( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, MinSimpleContinuousEnvelope, &fTopMinTrafficBarrier, &fBotMinTrafficBarrier);

      pForces2->GetStress( overlay_stage, pftOverlay, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxOverlay, &fBotMaxOverlay);
      pForces2->GetStress( overlay_stage, pftOverlay, vPoi, MinSimpleContinuousEnvelope, &fTopMinOverlay, &fBotMinOverlay);

      if ( bPedLoading )
      {
         pForces2->GetLiveLoadStress(pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy1, &fTopMaxPedestrianLL, &dummy2, &fBotMaxPedestrianLL);
         pForces2->GetLiveLoadStress(pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &fTopMinPedestrianLL, &dummy1, &fBotMinPedestrianLL, &dummy2);
      }

      pForces2->GetLiveLoadStress(pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy1, &fTopMaxDesignLL, &dummy2, &fBotMaxDesignLL);
      pForces2->GetLiveLoadStress(pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &fTopMinDesignLL, &dummy1, &fBotMinDesignLL, &dummy2);

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         pForces2->GetLiveLoadStress(pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy1, &fTopMaxFatigueLL, &dummy2, &fBotMaxFatigueLL);
         pForces2->GetLiveLoadStress(pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &fTopMinFatigueLL, &dummy1, &fBotMinFatigueLL, &dummy2);
      }

      if ( bPermit )
      {
         pForces2->GetLiveLoadStress(pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy1, &fTopMaxPermitLL, &dummy2, &fBotMaxPermitLL);
         pForces2->GetLiveLoadStress(pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &fTopMinPermitLL, &dummy1, &fBotMinPermitLL, &dummy2);
      }
   }
   else
   {
      if ( bSidewalk )
         pForces2->GetStress( pgsTypes::BridgeSite2, pftSidewalk, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxSidewalk, &fBotMaxSidewalk);

      if ( bShearKey )
         pForces2->GetStress( pgsTypes::BridgeSite1, pftShearKey, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxShearKey, &fBotMaxShearKey);

      pForces2->GetStress( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxTrafficBarrier, &fBotMaxTrafficBarrier);
      pForces2->GetStress( overlay_stage, pftOverlay, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxOverlay, &fBotMaxOverlay);

      if ( bPedLoading )
      {
         pForces2->GetLiveLoadStress(pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &fTopMinPedestrianLL, &fTopMaxPedestrianLL, &fBotMinPedestrianLL, &fBotMaxPedestrianLL);
      }

      pForces2->GetLiveLoadStress(pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &fTopMinDesignLL, &fTopMaxDesignLL, &fBotMinDesignLL, &fBotMaxDesignLL);

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         pForces2->GetLiveLoadStress(pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &fTopMinFatigueLL, &fTopMaxFatigueLL, &fBotMinFatigueLL, &fBotMaxFatigueLL);
      }

      if ( bPermit )
      {
         pForces2->GetLiveLoadStress(pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &fTopMinPermitLL, &fTopMaxPermitLL, &fBotMinPermitLL, &fBotMaxPermitLL);
      }
   }



   // Fill up the table
   Uint32 index = 0;
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
   {
      const pgsPointOfInterest& poi = *i;
      SpanIndexType spanIdx = poi.GetSpan();
      GirderIndexType gdrIdx  = poi.GetGirder();
   
      Float64 end_size = pBridge->GetGirderStartConnectionLength(spanIdx,gdrIdx);

      pgsTypes::Stage girderLoadStage = pLoads->GetGirderDeadLoadStage(spanIdx,gdrIdx);

      ColumnIndexType col = 0;

      (*p_table)(row,col++) << location.SetValue( poi, end_size );

      (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopGirder[index]) << rptNewLine;
      (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotGirder[index]);
      col++;

      (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopDiaphragm[index]) << rptNewLine;
      (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotDiaphragm[index]);
      col++;

      if ( bShearKey )
      {
         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxShearKey[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxShearKey[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMinShearKey[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMinShearKey[index]);
            col++;
         }
         else
         {
            (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxShearKey[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxShearKey[index]);
            col++;
         }
      }

      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      {
         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxSlab[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxSlab[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMinSlab[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMinSlab[index]);
         col++;
      }
      else
      {
         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxSlab[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxSlab[index]);
         col++;
      }

      if ( bDeckPanels )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxSlabPanel[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxSlabPanel[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMinSlabPanel[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMinSlabPanel[index]);
            col++;
         }
         else
         {
            (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxSlabPanel[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxSlabPanel[index]);
            col++;
         }
      }

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bSidewalk )
         {
            (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxSidewalk[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxSidewalk[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMinSidewalk[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMinSidewalk[index]);
            col++;
         }

         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxTrafficBarrier[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxTrafficBarrier[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMinTrafficBarrier[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMinTrafficBarrier[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxOverlay[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxOverlay[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMinOverlay[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMinOverlay[index]);
         col++;
      }
      else
      {
         if ( bSidewalk )
         {
            (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxSidewalk[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxSidewalk[index]);
            col++;
         }

         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxTrafficBarrier[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxTrafficBarrier[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxOverlay[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxOverlay[index]);
         col++;
      }


      if ( bPedLoading )
      {
         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxPedestrianLL[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxPedestrianLL[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMinPedestrianLL[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMinPedestrianLL[index]);
         col++;
      }

      (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxDesignLL[index]) << rptNewLine;
      (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxDesignLL[index]);
      col++;

      (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMinDesignLL[index]) << rptNewLine;
      (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMinDesignLL[index]);
      col++;

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxFatigueLL[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxFatigueLL[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMinFatigueLL[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMinFatigueLL[index]);
         col++;
      }

      if ( bPermit )
      {
         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMaxPermitLL[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMaxPermitLL[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue(fTopMinPermitLL[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue(fBotMinPermitLL[index]);
         col++;
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
void CProductStressTable::MakeCopy(const CProductStressTable& rOther)
{
   // Add copy code here...
}

void CProductStressTable::MakeAssignment(const CProductStressTable& rOther)
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
