///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <Reporting\CombinedStressTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>

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
                                         IEAFDisplayUnits* pDisplayUnits,
                                         pgsTypes::Stage stage,pgsTypes::AnalysisType analysisType,
                                         bool bDesign,bool bRating) const
{
   BuildCombinedDeadTable(pBroker, pChapter, span, girder, pDisplayUnits, stage, analysisType, bDesign, bRating);

   if (stage==pgsTypes::BridgeSite3)
   {
      if (bDesign)
         BuildCombinedLiveTable(pBroker, pChapter, span, girder, pDisplayUnits, analysisType, true, false);
      if (bRating)
         BuildCombinedLiveTable(pBroker, pChapter, span, girder, pDisplayUnits, analysisType, false, true);

      if (bDesign)
         BuildLimitStateTable(pBroker, pChapter, span, girder, pDisplayUnits, analysisType, true, false);
      if (bRating)
         BuildLimitStateTable(pBroker, pChapter, span, girder, pDisplayUnits, analysisType, false, true);
   }
}

void CCombinedStressTable::BuildCombinedDeadTable(IBroker* pBroker, rptChapter* pChapter,
                                         SpanIndexType span,GirderIndexType girder,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         pgsTypes::Stage stage,pgsTypes::AnalysisType analysisType,
                                         bool bDesign,bool bRating) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),     false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILimitStateForces2,pLsForces2);

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

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   // Set up table headings
   if ( stage == pgsTypes::CastingYard || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
   {
      std::_tstring strTitle(stage == pgsTypes::CastingYard     ? _T("Casting Yard") :
                           stage == pgsTypes::GirderPlacement ? _T("Girder Placement") : _T("Temporary Strand Removal"));
      p_table = pgsReportStyleHolder::CreateDefaultTable(3,strTitle);
      (*p_table)(0,0) << COLHDR(RPT_GDR_END_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,1) << COLHDR(_T("DC"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,2) << COLHDR(_T("Service I"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else if ( stage == pgsTypes::BridgeSite1)
   {
      nCols = 6;

      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         nCols += 5;

      if(bRating)
         nCols +=2;

      col = 0;

      p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Deck and Diaphragm Placement (Bridge Site 1)"));
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,col++) << COLHDR(_T("DC"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR(_T("DW"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      if(bRating)
      {
         (*p_table)(0,col++) << COLHDR(_T("DW") << rptNewLine << _T("Rating"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << _T("DC"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << _T("DW"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      if(bRating)
      {
         (*p_table)(0,col++) << COLHDR(symbol(SUM) << _T("DW")<< rptNewLine << _T("Rating"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      (*p_table)(0,col++) << COLHDR(_T("Service I"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else if ( stage == pgsTypes::BridgeSite2)
   {
      nCols = 6;

      if(bRating)
         nCols +=2;

      col = 0;
      p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Final without Live Load (Bridge Site 2)"));
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,col++) << COLHDR(_T("DC"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR(_T("DW"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      if(bRating)
      {
         (*p_table)(0,col++) << COLHDR(_T("DW") << rptNewLine << _T("Rating"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << _T("DC"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << _T("DW"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      if(bRating)
      {
         (*p_table)(0,col++) << COLHDR(symbol(SUM) << _T("DW")<< rptNewLine << _T("Rating"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      (*p_table)(0,col++) << COLHDR(_T("Service I"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else if ( stage == pgsTypes::BridgeSite3 )
   {
      nCols = 5;

      if(bRating)
         nCols +=4;

      col = 0;
      ColumnIndexType col2 = 0;

      p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Final with Live Load (Bridge Site 3)"));


      p_table->SetNumberOfHeaderRows(2);

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,  rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col++) << COLHDR(_T("DC"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col++) << COLHDR(_T("DW"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      if(bRating)
      {
         p_table->SetRowSpan(0,col,2);
         p_table->SetRowSpan(1,col2++,SKIP_CELL);
         (*p_table)(0,col++) << COLHDR(_T("DW")<< rptNewLine << _T("Rating"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << _T("DC"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col++) << COLHDR(symbol(SUM) << _T("DW"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      if(bRating)
      {
         p_table->SetRowSpan(0,col,2);
         p_table->SetRowSpan(1,col2++,SKIP_CELL);
         (*p_table)(0,col++) << COLHDR(symbol(SUM) << _T("DW")<< rptNewLine << _T("Rating"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }

      for ( ColumnIndexType i = col; i < nCols; i++ )
         p_table->SetColumnSpan(0,i,SKIP_CELL);
   }
   else
   {
      ATLASSERT(false); // who added a new stage without telling me?
   }


   if ( span == ALL_SPANS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   *p << p_table << rptNewLine;

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( span, girder, stage, POI_ALL, POIFIND_OR );

   GET_IFACE2(pBroker,ICombinedForces,pForces);
   GET_IFACE2(pBroker,ICombinedForces2,pForces2);

   BridgeAnalysisType bat;
   if (analysisType == pgsTypes::Simple)
      bat = SimpleSpan;
   else if ( analysisType == pgsTypes::Continuous )
      bat = ContinuousSpan;
   else
      bat = MaxSimpleContinuousEnvelope;

   std::vector<Float64> fTopDCinc, fBotDCinc;
   std::vector<Float64> fTopDWinc, fBotDWinc;
   std::vector<Float64> fTopDWRatinginc, fBotDWRatinginc;
   std::vector<Float64> fTopDCcum, fBotDCcum;
   std::vector<Float64> fTopDWcum, fBotDWcum;
   std::vector<Float64> fTopDWRatingcum, fBotDWRatingcum;
   std::vector<Float64> fTopMinServiceI, fBotMinServiceI;
   std::vector<Float64> fTopMaxServiceI, fBotMaxServiceI;

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
      if(bRating)
      {
         pForces2->GetStress( lcDWRating, stage, vPoi, ctIncremental, bat, &fTopDWRatinginc, &fBotDWRatinginc);
      }
      pForces2->GetStress( lcDC, stage, vPoi, ctCummulative, bat, &fTopDCcum, &fBotDCcum);
      pForces2->GetStress( lcDW, stage, vPoi, ctCummulative, bat, &fTopDWcum, &fBotDWcum);
      if(bRating)
      {
         pForces2->GetStress( lcDWRating, stage, vPoi, ctCummulative, bat, &fTopDWRatingcum, &fBotDWRatingcum);
      }

      pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceI,&fTopMaxServiceI);
      pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceI,&fBotMaxServiceI);
   }
   else if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3)
   {
      pForces2->GetStress( lcDC, stage, vPoi, ctIncremental, bat, &fTopDCinc, &fBotDCinc);
      pForces2->GetStress( lcDW, stage, vPoi, ctIncremental, bat, &fTopDWinc, &fBotDWinc);
      if(bRating)
      {
         pForces2->GetStress( lcDWRating, stage, vPoi, ctIncremental, bat, &fTopDWRatinginc, &fBotDWRatinginc);
      }
      pForces2->GetStress( lcDC, stage, vPoi, ctCummulative, bat, &fTopDCcum, &fBotDCcum);
      pForces2->GetStress( lcDW, stage, vPoi, ctCummulative, bat, &fTopDWcum, &fBotDWcum);
      if(bRating)
      {
         pForces2->GetStress( lcDWRating, stage, vPoi, ctCummulative, bat, &fTopDWRatingcum, &fBotDWRatingcum);
      }

      if ( stage == pgsTypes::BridgeSite2 )
      {
         pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceI,&fTopMaxServiceI);
         pLsForces2->GetStress( pgsTypes::ServiceI, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceI,&fBotMaxServiceI);
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

      (*p_table)(row,col++) << location.SetValue( stage, poi, end_size );
      if ( stage == pgsTypes::CastingYard || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
      {
         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDCinc[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDCinc[index]);

         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceI[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceI[index]);
      }
      else if ( stage == pgsTypes::BridgeSite1 )
      {
         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDCinc[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDCinc[index]);

         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDWinc[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDWinc[index]);

         if(bRating)
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDWRatinginc[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDWRatinginc[index]);
         }

         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDCcum[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDCcum[index]);

         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDWcum[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDWcum[index]);

         if(bRating)
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDWRatingcum[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDWRatingcum[index]);
         }

         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceI[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceI[index]);
      }
      else if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3)
      {
         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDCinc[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDCinc[index]);

         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDWinc[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDWinc[index]);

         if(bRating)
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDWRatinginc[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDWRatinginc[index]);
         }

         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDCcum[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDCcum[index]);

         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDWcum[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDWcum[index]);

         if(bRating)
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDWRatingcum[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDWRatingcum[index]);
         }

         if ( stage == pgsTypes::BridgeSite2 )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceI[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceI[index]);
         }
      }

      row++;
   }
}

void CCombinedStressTable::BuildCombinedLiveTable(IBroker* pBroker, rptChapter* pChapter,
                                         SpanIndexType span,GirderIndexType girder,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         pgsTypes::AnalysisType analysisType,
                                         bool bDesign,bool bRating) const
{
   pgsTypes::Stage stage = pgsTypes::BridgeSite3; // always

   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),     false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType nSpans    = (span == ALL_SPANS ? pBridge->GetSpanCount() : startSpan+1 );
 
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedLoading = bDesign && pProductLoads->HasPedestrianLoad(startSpan,girder) || 
                      bRating && pRatingSpec->IncludePedestrianLiveLoad();

   bool bPermit = false;// never for stress

   LPCTSTR strLabel = bDesign ? _T("Stress - Design Vehicles") : _T("Stress - Rating Vehicles");

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table;
   RowIndexType Nhrows = CreateCombinedLiveLoadingTableHeading<rptStressUnitTag,unitmgtStressData>(&p_table,strLabel,false,bDesign,bPermit,bPedLoading,bRating,true,true,
                           stage,analysisType,pRatingSpec,pDisplayUnits,pDisplayUnits->GetStressUnit());
   *p << p_table;

   if ( span == ALL_SPANS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   rptParagraph* pNote = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pNote;
   *pNote << LIVELOAD_PER_GIRDER << rptNewLine;

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( span, girder, stage, POI_ALL, POIFIND_OR );

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

   std::vector<Float64> fTopMinPedestrianLL, fBotMinPedestrianLL;
   std::vector<Float64> fTopMaxPedestrianLL, fBotMaxPedestrianLL;
   std::vector<Float64> fTopMinDesignLL, fBotMinDesignLL;
   std::vector<Float64> fTopMaxDesignLL, fBotMaxDesignLL;
   std::vector<Float64> fTopMinFatigueLL, fBotMinFatigueLL;
   std::vector<Float64> fTopMaxFatigueLL, fBotMaxFatigueLL;
   std::vector<Float64> fTopMinLegalRoutineLL, fBotMinLegalRoutineLL;
   std::vector<Float64> fTopMaxLegalRoutineLL, fBotMaxLegalRoutineLL;
   std::vector<Float64> fTopMinLegalSpecialLL, fBotMinLegalSpecialLL;
   std::vector<Float64> fTopMaxLegalSpecialLL, fBotMaxLegalSpecialLL;

   // Bridge site 3
   if ( bPedLoading )
   {
      pForces2->GetCombinedLiveLoadStress( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, bat, &fTopMinPedestrianLL, &fTopMaxPedestrianLL, &fBotMinPedestrianLL, &fBotMaxPedestrianLL );
   }

   if ( bDesign )
   {
      pForces2->GetCombinedLiveLoadStress( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, bat, &fTopMinDesignLL, &fTopMaxDesignLL, &fBotMinDesignLL, &fBotMaxDesignLL );

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         pForces2->GetCombinedLiveLoadStress( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, bat, &fTopMinFatigueLL, &fTopMaxFatigueLL, &fBotMinFatigueLL, &fBotMaxFatigueLL );
      }
   }

   if ( bRating )
   {
      if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
      {
         pForces2->GetCombinedLiveLoadStress( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, bat, &fTopMinDesignLL, &fTopMaxDesignLL, &fBotMinDesignLL, &fBotMaxDesignLL );
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         pForces2->GetCombinedLiveLoadStress( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, vPoi, bat, &fTopMinLegalRoutineLL, &fTopMaxLegalRoutineLL, &fBotMinLegalRoutineLL, &fBotMaxLegalRoutineLL );
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         pForces2->GetCombinedLiveLoadStress( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, vPoi, bat, &fTopMinLegalSpecialLL, &fTopMaxLegalSpecialLL, &fBotMinLegalSpecialLL, &fBotMaxLegalSpecialLL );
      }
   }

   // Fill up the table
   RowIndexType row = Nhrows;
   ColumnIndexType col = 0;
   long index = 0;
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
   {
      const pgsPointOfInterest& poi = *i;

      col = 0;

      Float64 end_size = 0 ;
      if ( stage != pgsTypes::CastingYard )
         end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

      (*p_table)(row,col++) << location.SetValue( stage, poi, end_size );

      if ( bPedLoading )
      {
         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxPedestrianLL[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxPedestrianLL[index]);

         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinPedestrianLL[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinPedestrianLL[index]);
      }

      if ( bDesign )
      {
         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxDesignLL[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxDesignLL[index]);

         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinDesignLL[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinDesignLL[index]);

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxFatigueLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxFatigueLL[index]);

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinFatigueLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinFatigueLL[index]);
         }
      }

      if ( bRating )
      {
         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxDesignLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxDesignLL[index]);

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinDesignLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinDesignLL[index]);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxLegalRoutineLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxLegalRoutineLL[index]);

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinLegalRoutineLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinLegalRoutineLL[index]);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxLegalSpecialLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxLegalSpecialLL[index]);

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinLegalSpecialLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinLegalSpecialLL[index]);
         }
       }

      row++;
   }


   // fill second half of table if design & ped load
   if ( bDesign && bPedLoading )
   {
      // Sum or envelope pedestrian values with live loads to give final LL

      GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
      ILiveLoads::PedestrianLoadApplicationType DesignPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltDesign);
      ILiveLoads::PedestrianLoadApplicationType FatiguePedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltFatigue);
      ILiveLoads::PedestrianLoadApplicationType PermitPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltPermit);

      SumPedAndLiveLoad(DesignPedLoad, fTopMinDesignLL, fTopMaxDesignLL, fTopMinPedestrianLL, fTopMaxPedestrianLL);
      SumPedAndLiveLoad(DesignPedLoad, fBotMinDesignLL, fBotMaxDesignLL, fBotMinPedestrianLL, fBotMaxPedestrianLL);

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         SumPedAndLiveLoad(FatiguePedLoad, fTopMinFatigueLL, fTopMaxFatigueLL, fTopMinPedestrianLL, fTopMaxPedestrianLL);
         SumPedAndLiveLoad(FatiguePedLoad, fBotMinFatigueLL, fBotMaxFatigueLL, fBotMinPedestrianLL, fBotMaxPedestrianLL);

      }

      // Now we can fill table
      RowIndexType    row = Nhrows;
      ColumnIndexType recCol = col;
      int psiz = (int)vPoi.size();
      for ( index=0; index<psiz; index++ )
      {
         col = recCol;

         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxDesignLL[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxDesignLL[index]);

         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinDesignLL[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinDesignLL[index]);

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxFatigueLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxFatigueLL[index]);

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinFatigueLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinFatigueLL[index]);
         }

         row++;
      }

      // footnotes for pedestrian loads
      int lnum=1;
      *pNote<< lnum++ << PedestrianFootnote(DesignPedLoad) << rptNewLine;

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         *pNote << lnum++ << PedestrianFootnote(FatiguePedLoad) << rptNewLine;
      }
   }

   if ( bRating && pRatingSpec->IncludePedestrianLiveLoad())
   {
      // Note for rating and pedestrian load
      *pNote << _T("$ Pedestrian load results will be summed with vehicular load results at time of load combination.");
   }
}

void CCombinedStressTable::BuildLimitStateTable(IBroker* pBroker, rptChapter* pChapter,
                                         SpanIndexType span,GirderIndexType girder,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         pgsTypes::AnalysisType analysisType,
                                         bool bDesign,bool bRating) const
{

   // NOTE - Stregth II stresses not reported because they aren't used for anything
   pgsTypes::Stage stage = pgsTypes::BridgeSite3; // always

   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),     false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(pBroker,ILimitStateForces2,pLsForces2);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table;

   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType nSpans    = (span == ALL_SPANS ? pBridge->GetSpanCount() : startSpan+1 );
 
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedLoading = bDesign && pProductLoads->HasPedestrianLoad(startSpan,girder) || 
                      bRating && pRatingSpec->IncludePedestrianLiveLoad();

   BridgeAnalysisType bat;
   if (analysisType == pgsTypes::Simple)
      bat = SimpleSpan;
   else if ( analysisType == pgsTypes::Continuous )
      bat = ContinuousSpan;
   else
      bat = MaxSimpleContinuousEnvelope;

   // create second table for BSS3 Limit states
   p = new rptParagraph;
   *pChapter << p;

   ColumnIndexType nCols = 1; 
   if ( bDesign )
   {
      nCols += 6;
   }

   if ( bRating )
   {
      if ( !bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
         nCols += 2;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         nCols += 2;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         nCols += 2;
   }

   p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,_T(""));

   *p << p_table;

   p_table->SetNumberOfHeaderRows(3);

   ColumnIndexType col1 = 0;
   ColumnIndexType col2 = 0;
   ColumnIndexType col3 = 0;
   p_table->SetRowSpan(0,col1,3);
   (*p_table)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   p_table->SetRowSpan(1,col2++,SKIP_CELL);
   p_table->SetRowSpan(2,col3++,SKIP_CELL);

   if ( bDesign )
   {
      p_table->SetColumnSpan(0,col1,6);
      (*p_table)(0,col1++) << _T("Design");
      p_table->SetColumnSpan(0,col1++,SKIP_CELL);
      p_table->SetColumnSpan(0,col1++,SKIP_CELL);
      p_table->SetColumnSpan(0,col1++,SKIP_CELL);
      p_table->SetColumnSpan(0,col1++,SKIP_CELL);
      p_table->SetColumnSpan(0,col1++,SKIP_CELL);

      p_table->SetColumnSpan(1,col2,2);
      (*p_table)(1,col2++) << _T("Service I");
      p_table->SetColumnSpan(1,col2++,SKIP_CELL);

      (*p_table)(2,col3++) << COLHDR(_T("Max"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(2,col3++) << COLHDR(_T("Min"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      
      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         p_table->SetColumnSpan(1,col2,2);
         (*p_table)(1,col2++) << _T("Service IA");
         p_table->SetColumnSpan(1,col2++,SKIP_CELL);

         (*p_table)(2,col3++) << COLHDR(_T("Max"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*p_table)(2,col3++) << COLHDR(_T("Min"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      
      p_table->SetColumnSpan(1,col2,2);
      (*p_table)(1,col2++) << _T("Service III");
      p_table->SetColumnSpan(1,col2++,SKIP_CELL);

      (*p_table)(2,col3++) << COLHDR(_T("Max"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(2,col3++) << COLHDR(_T("Min"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   
      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         p_table->SetColumnSpan(1,col2,2);
         (*p_table)(1,col2++) << _T("Fatigue I");
         p_table->SetColumnSpan(1,col2++,SKIP_CELL);

         (*p_table)(2,col3++) << COLHDR(_T("Max"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*p_table)(2,col3++) << COLHDR(_T("Min"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
   }

   if ( bRating )
   {
      ColumnIndexType colSpan = 0;

      if ( !bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory)  )
         colSpan += 2;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         colSpan += 2;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         colSpan += 2;

      if ( 0 < colSpan )
      {
         p_table->SetColumnSpan(0,col1,colSpan);
         (*p_table)(0,col1++) << _T("Rating");

         for ( ColumnIndexType i = 0; i < colSpan-1; i++ )
            p_table->SetColumnSpan(0,col1++,SKIP_CELL);

         if ( !bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
         {
            p_table->SetColumnSpan(1,col2,2);
            (*p_table)(1,col2++) << _T("Service III") << rptNewLine << _T("Inventory");
            p_table->SetColumnSpan(1,col2++,SKIP_CELL);

            (*p_table)(2,col3++) << COLHDR(_T("Max"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
            (*p_table)(2,col3++) << COLHDR(_T("Min"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            p_table->SetColumnSpan(1,col2,2);
            (*p_table)(1,col2++) << _T("Service III") << rptNewLine << _T("Legal Routine");
            p_table->SetColumnSpan(1,col2++,SKIP_CELL);

            (*p_table)(2,col3++) << COLHDR(_T("Max"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
            (*p_table)(2,col3++) << COLHDR(_T("Min"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            p_table->SetColumnSpan(1,col2,2);
            (*p_table)(1,col2++) << _T("Service III") << rptNewLine << _T("Legal Special");
            p_table->SetColumnSpan(1,col2++,SKIP_CELL);

            (*p_table)(2,col3++) << COLHDR(_T("Max"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
            (*p_table)(2,col3++) << COLHDR(_T("Min"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }
      }
   }

   std::vector<Float64> fTopMinServiceI,   fBotMinServiceI;
   std::vector<Float64> fTopMaxServiceI,   fBotMaxServiceI;
   std::vector<Float64> fTopMinServiceIA,  fBotMinServiceIA;
   std::vector<Float64> fTopMaxServiceIA,  fBotMaxServiceIA;
   std::vector<Float64> fTopMinServiceIII, fBotMinServiceIII;
   std::vector<Float64> fTopMaxServiceIII, fBotMaxServiceIII;
   std::vector<Float64> fTopMinFatigueI,   fBotMinFatigueI;
   std::vector<Float64> fTopMaxFatigueI,   fBotMaxFatigueI;
   std::vector<Float64> fTopMinServiceIII_Inventory, fBotMinServiceIII_Inventory;
   std::vector<Float64> fTopMaxServiceIII_Inventory, fBotMaxServiceIII_Inventory;
   std::vector<Float64> fTopMinServiceIII_Routine,   fBotMinServiceIII_Routine;
   std::vector<Float64> fTopMaxServiceIII_Routine,   fBotMaxServiceIII_Routine;
   std::vector<Float64> fTopMinServiceIII_Special,   fBotMinServiceIII_Special;
   std::vector<Float64> fTopMaxServiceIII_Special,   fBotMaxServiceIII_Special;

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( span, girder, stage, POI_ALL, POIFIND_OR );

   if ( bDesign )
   {
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
   }

   if ( bRating )
   {
      if ( !bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
      {
         pLsForces2->GetStress( pgsTypes::ServiceIII_Inventory, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceIII_Inventory, &fTopMaxServiceIII_Inventory);
         pLsForces2->GetStress( pgsTypes::ServiceIII_Inventory, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceIII_Inventory, &fBotMaxServiceIII_Inventory);
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         pLsForces2->GetStress( pgsTypes::ServiceIII_LegalRoutine, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceIII_Routine, &fTopMaxServiceIII_Routine);
         pLsForces2->GetStress( pgsTypes::ServiceIII_LegalRoutine, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceIII_Routine, &fBotMaxServiceIII_Routine);
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         pLsForces2->GetStress( pgsTypes::ServiceIII_LegalSpecial, stage, vPoi, pgsTypes::TopGirder,    false, bat, &fTopMinServiceIII_Special, &fTopMaxServiceIII_Special);
         pLsForces2->GetStress( pgsTypes::ServiceIII_LegalSpecial, stage, vPoi, pgsTypes::BottomGirder, false, bat, &fBotMinServiceIII_Special, &fBotMaxServiceIII_Special);
      }
   }

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i;
   long index = 0;
   for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
   {
      ColumnIndexType col = 0;
      const pgsPointOfInterest& poi = *i;
      Float64 end_size = 0 ;
      if ( stage != pgsTypes::CastingYard )
         end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

      (*p_table)(row,col++) << location.SetValue( stage, poi, end_size );

      if ( bDesign )
      {
         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxServiceI[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceI[index]);

         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceI[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinServiceI[index]);

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxServiceIA[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceIA[index]);

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceIA[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinServiceIA[index]);
         }

         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxServiceIII[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceIII[index]);

         (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceIII[index]) << rptNewLine;
         (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinServiceIII[index]);

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxFatigueI[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxFatigueI[index]);

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinFatigueI[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinFatigueI[index]);
         }
      }

      if ( bRating )
      {
         if ( !bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxServiceIII_Inventory[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceIII_Inventory[index]);

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceIII_Inventory[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinServiceIII_Inventory[index]);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxServiceIII_Routine[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceIII_Routine[index]);

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceIII_Routine[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinServiceIII_Routine[index]);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxServiceIII_Special[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceIII_Special[index]);

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceIII_Special[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinServiceIII_Special[index]);
         }
      }

      row++;
   }
}

void CCombinedStressTable::MakeCopy(const CCombinedStressTable& rOther)
{
   // Add copy code here...
}

void CCombinedStressTable::MakeAssignment(const CCombinedStressTable& rOther)
{
   MakeCopy( rOther );
}
