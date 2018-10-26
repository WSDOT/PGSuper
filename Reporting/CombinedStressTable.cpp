///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2006  Washington State Department of Transportation
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
#include <Reporting\CombinedStressTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>

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
   CCombinedStressTable
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCombinedStressTable::CCombinedStressTable()
{
}

CCombinedStressTable::CCombinedStressTable(const CCombinedStressTable& rOther)
{
   MakeCopy(rOther);
}

CCombinedStressTable::~CCombinedStressTable()
{
}

//======================== OPERATORS  =======================================
CCombinedStressTable& CCombinedStressTable::operator= (const CCombinedStressTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CCombinedStressTable::Build(IBroker* pBroker, rptChapter* pChapter,
                                         SpanIndexType span,GirderIndexType girder,
                                         IDisplayUnits* pDispUnits,
                                         pgsTypes::Stage stage,pgsTypes::AnalysisType analysisType) const
{
   // NOTE - Stregth II stresses not reported because they aren't used for anything

   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDispUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDispUnits->GetStressUnit(),     false );

   if ( stage == pgsTypes::CastingYard )
      location.MakeGirderPoi();
   else
      location.MakeSpanPoi();

   GET_IFACE2(pBroker,IBridge,pBridge);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table;

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

   ColumnIndexType nCols;
   ColumnIndexType col=0;

   bool bPedLoading = false;

   // Set up table headings
   if ( stage == pgsTypes::CastingYard || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
   {
      std::string strTitle(stage == pgsTypes::CastingYard     ? "Casting Yard" :
                           stage == pgsTypes::GirderPlacement ? "Girder Placement" : "Temporary Strand Removal");
      p_table = pgsReportStyleHolder::CreateDefaultTable(3,strTitle);
      (*p_table)(0,0) << COLHDR(RPT_GDR_END_LOCATION ,    rptLengthUnitTag, pDispUnits->GetSpanLengthUnit() );
      (*p_table)(0,1) << COLHDR("DC",          rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*p_table)(0,2) << COLHDR("Service I", rptStressUnitTag, pDispUnits->GetStressUnit() );
   }
   else if ( stage == pgsTypes::BridgeSite1)
   {
      nCols = 6;

      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         nCols += 5;

      col = 0;

      p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Deck and Diaphragm Placement (Bridge Site 1)");
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDispUnits->GetSpanLengthUnit() );
      (*p_table)(0,col++) << COLHDR("DC",          rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR("DW",          rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << "DC",          rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << "DW",          rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR("Service I", rptStressUnitTag, pDispUnits->GetStressUnit() );
   }
   else if ( stage == pgsTypes::BridgeSite2)
   {
      nCols = 6;

      col = 0;
      p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Superimposed Dead Loads (Bridge Site 2)");
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDispUnits->GetSpanLengthUnit() );
      (*p_table)(0,col++) << COLHDR("DC",          rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR("DW",          rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << "DC",          rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << "DW",          rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR("Service I", rptStressUnitTag, pDispUnits->GetStressUnit() );
   }
   else if ( stage == pgsTypes::BridgeSite3 )
   {
      nCols = 7;

      GET_IFACE2(pBroker,IProductForces,pProductForces);
      bPedLoading = pProductForces->HasPedestrianLoad(startSpan,girder);

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         nCols += 2;

      if ( bPedLoading )
         nCols += 2;

      col = 0;
      ColumnIndexType col2 = 0;

      p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Final with Live Load (Bridge Site 3)");

      p_table->SetNumberOfHeaderRows(2);

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,  rptLengthUnitTag, pDispUnits->GetSpanLengthUnit() );

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col++) << COLHDR("DC",          rptStressUnitTag, pDispUnits->GetStressUnit() );

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col++) << COLHDR("DW",          rptStressUnitTag, pDispUnits->GetStressUnit() );

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << "DC",          rptStressUnitTag, pDispUnits->GetStressUnit() );

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << "DW",          rptStressUnitTag, pDispUnits->GetStressUnit() );

      if ( bPedLoading )
      {
         p_table->SetColumnSpan(0,col,2);
         (*p_table)(0,col++) << "PL";
         (*p_table)(1,col2++) << COLHDR("Max",       rptStressUnitTag, pDispUnits->GetStressUnit() );
         (*p_table)(1,col2++) << COLHDR("Min",       rptStressUnitTag, pDispUnits->GetStressUnit() );
      }

      p_table->SetColumnSpan(0,col,2);
      (*p_table)(0,col++) << "*LL+IM" << rptNewLine << "Design";
      (*p_table)(1,col2++) << COLHDR("Max",       rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR("Min",       rptStressUnitTag, pDispUnits->GetStressUnit() );

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         p_table->SetColumnSpan(0,col,2);
         (*p_table)(0,col++) << "*LL+IM" << rptNewLine << "Fatigue";
         (*p_table)(1,col2++) << COLHDR("Max",       rptStressUnitTag, pDispUnits->GetStressUnit() );
         (*p_table)(1,col2++) << COLHDR("Min",       rptStressUnitTag, pDispUnits->GetStressUnit() );
      }


      for ( ColumnIndexType i = col; i < nCols; i++ )
         p_table->SetColumnSpan(0,i,-1);
   }
   else
      CHECK(0); // who added a new stage without telling me?

   *p << p_table << rptNewLine;

   if ( stage == pgsTypes::BridgeSite3 )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << p;
      *p << LIVELOAD_PER_GIRDER << rptNewLine;
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( stage, span, girder, POI_TABULAR );

   GET_IFACE2(pBroker,ICombinedForces,pForces);
   GET_IFACE2(pBroker,ICombinedForces2,pForces2);
   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);
   GET_IFACE2(pBroker,ILimitStateForces2,pLsForces2);

   BridgeAnalysisType bat;
   if (analysisType == pgsTypes::Simple)
      bat = SimpleSpan;
   else if ( analysisType == pgsTypes::Continuous )
      bat = ContinuousSpan;
   else
      bat = MaxSimpleContinuousEnvelope;

   std::vector<Float64> fTopDCinc, fBotDCinc;
   std::vector<Float64> fTopDWinc, fBotDWinc;
   std::vector<Float64> fTopDCcum, fBotDCcum;
   std::vector<Float64> fTopDWcum, fBotDWcum;
   std::vector<Float64> fTopMinServiceI, fBotMinServiceI;
   std::vector<Float64> fTopMaxServiceI, fBotMaxServiceI;
   std::vector<Float64> fTopMinPedestrianLL, fBotMinPedestrianLL;
   std::vector<Float64> fTopMaxPedestrianLL, fBotMaxPedestrianLL;
   std::vector<Float64> fTopMinDesignLL, fBotMinDesignLL;
   std::vector<Float64> fTopMaxDesignLL, fBotMaxDesignLL;
   std::vector<Float64> fTopMinFatigueLL, fBotMinFatigueLL;
   std::vector<Float64> fTopMaxFatigueLL, fBotMaxFatigueLL;
   if ( stage == pgsTypes::CastingYard || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
   {
      pForces2->GetStress(lcDC,stage,vPoi, ctIncremental, SimpleSpan, &fTopDCinc, &fBotDCinc);
      pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::TopGirder,    false, SimpleSpan, &fTopMinServiceI, &fTopMaxServiceI);
      pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::BottomGirder, false, SimpleSpan, &fBotMinServiceI, &fBotMaxServiceI);
   }
   else if ( stage == pgsTypes::BridgeSite1 )
   {
      pForces2->GetStress( lcDC, stage, vPoi, ctIncremental, bat, &fTopDCinc, &fBotDCinc);
      pForces2->GetStress( lcDW, stage, vPoi, ctIncremental, bat, &fTopDWinc, &fBotDWinc);
      pForces2->GetStress( lcDC, stage, vPoi, ctCummulative, bat, &fTopDCcum, &fBotDCcum);
      pForces2->GetStress( lcDW, stage, vPoi, ctCummulative, bat, &fTopDWcum, &fBotDWcum);

      pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceI,&fTopMaxServiceI);
      pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceI,&fBotMaxServiceI);
   }
   else if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3)
   {
      pForces2->GetStress( lcDC, stage, vPoi, ctIncremental, bat, &fTopDCinc, &fBotDCinc);
      pForces2->GetStress( lcDW, stage, vPoi, ctIncremental, bat, &fTopDWinc, &fBotDWinc);
      pForces2->GetStress( lcDC, stage, vPoi, ctCummulative, bat, &fTopDCcum, &fBotDCcum);
      pForces2->GetStress( lcDW, stage, vPoi, ctCummulative, bat, &fTopDWcum, &fBotDWcum);

      if ( stage == pgsTypes::BridgeSite2 )
      {
         pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceI,&fTopMaxServiceI);
         pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceI,&fBotMaxServiceI);
      }
      else
      {
         if ( bPedLoading )
         {
            pForces2->GetCombinedLiveLoadStress( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, bat, &fTopMinPedestrianLL, &fTopMaxPedestrianLL, &fBotMinPedestrianLL, &fBotMaxPedestrianLL );
         }

         pForces2->GetCombinedLiveLoadStress( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, bat, &fTopMinDesignLL, &fTopMaxDesignLL, &fBotMinDesignLL, &fBotMaxDesignLL );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            pForces2->GetCombinedLiveLoadStress( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, bat, &fTopMinFatigueLL, &fTopMaxFatigueLL, &fBotMinFatigueLL, &fBotMaxFatigueLL );
      }
   }

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   long index = 0;
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
   {
      const pgsPointOfInterest& poi = *i;

      col = 0;

      Float64 end_size = 0 ;
      if ( stage != pgsTypes::CastingYard )
         end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

      (*p_table)(row,col++) << location.SetValue( poi, end_size );
      if ( stage == pgsTypes::CastingYard || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
      {
         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDCinc[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDCinc[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinServiceI[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxServiceI[index]);
      }
      else if ( stage == pgsTypes::BridgeSite1 )
      {
         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDCinc[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDCinc[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDWinc[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDWinc[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDCcum[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDCcum[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDWcum[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDWcum[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinServiceI[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxServiceI[index]);
      }
      else if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3)
      {
         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDCinc[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDCinc[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDWinc[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDWinc[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDCcum[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDCcum[index]);

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopDWcum[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotDWcum[index]);


         if ( stage == pgsTypes::BridgeSite2 )
         {
            (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinServiceI[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxServiceI[index]);
         }
         else
         {
            if ( bPedLoading )
            {
               (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMaxPedestrianLL[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxPedestrianLL[index]);

               (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinPedestrianLL[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMinPedestrianLL[index]);
            }

            (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMaxDesignLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxDesignLL[index]);

            (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinDesignLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMinDesignLL[index]);

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMaxFatigueLL[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxFatigueLL[index]);

               (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinFatigueLL[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMinFatigueLL[index]);
            }
         }
      }

      row++;
   }

   // create second table for BSS3 Limit states
   if ( stage == pgsTypes::BridgeSite3 )
   {
      p = new rptParagraph;
      *pChapter << p;

      p = new rptParagraph;
      *pChapter << p;
      p_table = pgsReportStyleHolder::CreateDefaultTable(4,"");

      *p << p_table;

      ColumnIndexType col = 0;
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDispUnits->GetSpanLengthUnit() );
      (*p_table)(0,col++) << COLHDR("Service I",   rptStressUnitTag, pDispUnits->GetStressUnit() );
   
      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         (*p_table)(0,col++) << COLHDR("Service IA",  rptStressUnitTag, pDispUnits->GetStressUnit() );
      
      (*p_table)(0,col++) << COLHDR("Service III", rptStressUnitTag, pDispUnits->GetStressUnit() );
   
      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         (*p_table)(0,col++) << COLHDR("Fatigue I",  rptStressUnitTag, pDispUnits->GetStressUnit() );


      std::vector<Float64> fTopMinServiceI, fBotMinServiceI;
      std::vector<Float64> fTopMaxServiceI, fBotMaxServiceI;
      std::vector<Float64> fTopMinServiceIA, fBotMinServiceIA;
      std::vector<Float64> fTopMaxServiceIA, fBotMaxServiceIA;
      std::vector<Float64> fTopMinServiceIII, fBotMinServiceIII;
      std::vector<Float64> fTopMaxServiceIII, fBotMaxServiceIII;
      std::vector<Float64> fTopMinFatigueI, fBotMinFatigueI;
      std::vector<Float64> fTopMaxFatigueI, fBotMaxFatigueI;
      pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceI, &fTopMaxServiceI);
      pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceI, &fBotMaxServiceI);

      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         pLsForces2->GetStress( pgsTypes::ServiceIA, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceIA, &fTopMaxServiceIA);
         pLsForces2->GetStress( pgsTypes::ServiceIA, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceIA, &fBotMaxServiceIA);
      }
      else
      {
         pLsForces2->GetStress( pgsTypes::FatigueI, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinFatigueI, &fTopMaxFatigueI);
         pLsForces2->GetStress( pgsTypes::FatigueI, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinFatigueI, &fBotMaxFatigueI);
      }

      pLsForces2->GetStress( pgsTypes::ServiceIII, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceIII, &fTopMaxServiceIII);
      pLsForces2->GetStress( pgsTypes::ServiceIII, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceIII, &fBotMaxServiceIII);

      RowIndexType row = p_table->GetNumberOfHeaderRows();
   
      std::vector<pgsPointOfInterest>::const_iterator i;
      long index = 0;
      for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
      {
         col = 0;
         const pgsPointOfInterest& poi = *i;
         Float64 end_size = 0 ;
         if ( stage != pgsTypes::CastingYard )
            end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

         (*p_table)(row,col++) << location.SetValue( poi, end_size );

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinServiceI[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxServiceI[index]);

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinServiceIA[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxServiceIA[index]);
         }

         (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinServiceIII[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxServiceIII[index]);

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*p_table)(row,col  ) << RPT_FTOP << " = " << stress.SetValue(fTopMinFatigueI[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << " = " << stress.SetValue(fBotMaxFatigueI[index]);
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
void CCombinedStressTable::MakeCopy(const CCombinedStressTable& rOther)
{
   // Add copy code here...
}

void CCombinedStressTable::MakeAssignment(const CCombinedStressTable& rOther)
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
