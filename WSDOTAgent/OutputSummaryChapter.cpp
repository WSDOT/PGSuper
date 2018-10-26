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
#include "OutputSummaryChapter.h"
#include <Reporting\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\SectPropTable.h>
#include <Reporting\SectPropTable2.h>
#include <Reporting\MomentCapacityParagraphBuilder.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\MomentCapacity.h>

#include <psgLib\SpecLibraryEntry.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\LiftingCheckArtifact.h>
#include <PgsExt\GirderData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   COutputSummaryChapter
****************************************************************************/

void castingyard_stresses(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits);
void bridgesite1_stresses(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits);
void bridgesite2_stresses(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits);
void bridgesite3_stresses(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits);
void shear_capacity(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits);
void section_properties(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits);
void creep_and_losses(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits);
void deflection_and_camber(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits);
void lifting(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits);
void hauling(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits);

COutputSummaryChapter::COutputSummaryChapter()
{
}

LPCTSTR COutputSummaryChapter::GetName() const
{
   return TEXT("Output Summary");
}

rptChapter* COutputSummaryChapter::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType spanIdx = pSGRptSpec->GetSpan();
   GirderIndexType gdrIdx = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,ISpecification,pSpec);
   *pPara << "Specification = " << pSpec->GetSpecification() << rptNewLine;

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pArtifact = pIArtifact->GetArtifact(spanIdx,gdrIdx);
   
   rptParagraph* p = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << p;
   *p << "Status" << rptNewLine;

   p = new rptParagraph;
   *pChapter << p;

   if ( pArtifact->Passed() )
   {
      *p << color(Green) << "This girder has passed a comprehensive check" << color(Black) << rptNewLine;
   }
   else
   {
      *p << color(Red) << "This girder has failed a comprehensive check" << color(Black) << rptNewLine;
      *p << "NOTE: This report does not contain a detailed listing of all analysis checks" << rptNewLine;
   }


   castingyard_stresses( pChapter, pBroker, spanIdx, gdrIdx, pDisplayUnits );
   bridgesite1_stresses( pChapter, pBroker, spanIdx, gdrIdx, pDisplayUnits );
   bridgesite2_stresses( pChapter, pBroker, spanIdx, gdrIdx, pDisplayUnits );
   bridgesite3_stresses( pChapter, pBroker, spanIdx, gdrIdx, pDisplayUnits );

   CMomentCapacityParagraphBuilder mcbuilder;
   p = mcbuilder.Build(pRptSpec, level);
   *pChapter << p;

   shear_capacity( pChapter, pBroker, spanIdx, gdrIdx, pDisplayUnits );
   section_properties( pChapter, pBroker, spanIdx, gdrIdx, pDisplayUnits );
   creep_and_losses( pChapter, pBroker, spanIdx, gdrIdx, pDisplayUnits );
   deflection_and_camber( pChapter, pBroker, spanIdx, gdrIdx, pDisplayUnits );
   lifting( pChapter, pBroker, spanIdx, gdrIdx, pDisplayUnits );
   hauling( pChapter, pBroker, spanIdx, gdrIdx, pDisplayUnits );

   return pChapter;
}

CChapterBuilder* COutputSummaryChapter::Clone() const
{
   return new COutputSummaryChapter;
}

void section_properties(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,IGirder,pGirder);
   bool bPrismaticNonComp = pGirder->IsPrismatic(pgsTypes::CastingYard,span,girder);
   bool bPrismaticComp    = pGirder->IsPrismatic(pgsTypes::BridgeSite3,span,girder);

   if ( bPrismaticNonComp )
   {
      // non-composite section is prismatic
      if ( bPrismaticComp )
      {
         // prismatc componsite and non-composite properties
         *pPara << CSectionPropertiesTable().Build(pBroker,span,girder,true,pDisplayUnits);
      }
      else
      {
         // prismatic non-composite properties
         *pPara << CSectionPropertiesTable().Build(pBroker,span,girder,false,pDisplayUnits);

         // non-prismatic, composite properties
         *pPara << CSectionPropertiesTable2().Build(pBroker,span,girder,pgsTypes::BridgeSite3,pDisplayUnits);
      }
   }
   else
   {
      // non-prismatic, non-composite properties
      *pPara << CSectionPropertiesTable2().Build(pBroker,span,girder,pgsTypes::CastingYard,pDisplayUnits);

      // non-prismatic, composite properties
      *pPara << CSectionPropertiesTable2().Build(pBroker,span,girder,pgsTypes::BridgeSite3,pDisplayUnits);
   }
}

void creep_and_losses(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
#if defined IGNORE_2007_CHANGES
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
   {

      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << color(Red) << bold(ON) << "Changes to LRFD 4th Edition, 2007, Article 5.4.2.3.2 have been ignored." << bold(OFF) << color(Black) << rptNewLine;
   }
#endif

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(2,"Creep Coefficients");
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetLongTimeUnit(), false );

   // Get the interfaces we need
   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,girder);
   bool bTempStrands = (0 < girderData.Nstrands[pgsTypes::Temporary] && girderData.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   for ( Uint16 col = CREEP_MINTIME; col <= CREEP_MAXTIME; col++ )
   {
      (*pTable)(0,col) << (col == CREEP_MINTIME ? "Minimum Time" : "Maximum Time");

      CREEPCOEFFICIENTDETAILS details;

      switch( deckType )
      {
         case pgsTypes::sdtCompositeCIP:
         case pgsTypes::sdtCompositeOverlay:
         case pgsTypes::sdtCompositeSIP:
            if ( bTempStrands )
            {
               details = pCamber->GetCreepCoefficientDetails(span,girder,ICamber::cpReleaseToDiaphragm,col);
               (*pTable)(row,col) << symbol(psi) << "(" << time2.SetValue(details.t);
               (*pTable)(row,col) << "," << time2.SetValue(details.ti) << ") = " << details.Ct;

               (*pTable)(row,col) << rptNewLine;

               details = pCamber->GetCreepCoefficientDetails(span,girder,ICamber::cpReleaseToDeck,col);
               (*pTable)(row,col) << symbol(psi) << "(" << time2.SetValue(details.t);
               (*pTable)(row,col) << "," << time2.SetValue(details.ti) << ") = " << details.Ct;

               (*pTable)(row,col) << rptNewLine;

               details = pCamber->GetCreepCoefficientDetails(span,girder,ICamber::cpDiaphragmToDeck,col);
               (*pTable)(row,col) << symbol(psi) << "(" << time2.SetValue(details.t);
               (*pTable)(row,col) << "," << time2.SetValue(details.ti) << ") = " << details.Ct;
            }
            else
            {
               details = pCamber->GetCreepCoefficientDetails(span,girder,ICamber::cpReleaseToDeck,col);
               (*pTable)(row,col) << symbol(psi) << "(" << time2.SetValue(details.t);
               (*pTable)(row,col) << "," << time2.SetValue(details.ti) << ") = " << details.Ct;
            }
            break;

         case pgsTypes::sdtNone:
               details = pCamber->GetCreepCoefficientDetails(span,girder,ICamber::cpReleaseToDiaphragm,col);
               (*pTable)(row,col) << symbol(psi) << "(" << time2.SetValue(details.t);
               (*pTable)(row,col) << "," << time2.SetValue(details.ti) << ") = " << details.Ct;

               (*pTable)(row,col) << rptNewLine;

               details = pCamber->GetCreepCoefficientDetails(span,girder,ICamber::cpReleaseToDeck,col);
               (*pTable)(row,col) << symbol(psi) << "(" << time2.SetValue(details.t);
               (*pTable)(row,col) << "," << time2.SetValue(details.ti) << ") = " << details.Ct;

               (*pTable)(row,col) << rptNewLine;

               details = pCamber->GetCreepCoefficientDetails(span,girder,ICamber::cpDiaphragmToDeck,col);
               (*pTable)(row,col) << symbol(psi) << "(" << time2.SetValue(details.t);
               (*pTable)(row,col) << "," << time2.SetValue(details.ti) << ") = " << details.Ct;

               (*pTable)(row,col) << rptNewLine;

               details = pCamber->GetCreepCoefficientDetails(span,girder,ICamber::cpDiaphragmToFinal,col);
               (*pTable)(row,col) << symbol(psi) << "(" << time2.SetValue(details.t);
               (*pTable)(row,col) << "," << time2.SetValue(details.ti) << ") = " << details.Ct;

               (*pTable)(row,col) << rptNewLine;

               details = pCamber->GetCreepCoefficientDetails(span,girder,ICamber::cpReleaseToFinal,col);
               (*pTable)(row,col) << symbol(psi) << "(" << time2.SetValue(details.t);
               (*pTable)(row,col) << "," << time2.SetValue(details.ti) << ") = " << details.Ct;

               (*pTable)(row,col) << rptNewLine;

               details = pCamber->GetCreepCoefficientDetails(span,girder,ICamber::cpDeckToFinal,col);
               (*pTable)(row,col) << symbol(psi) << "(" << time2.SetValue(details.t);
               (*pTable)(row,col) << "," << time2.SetValue(details.ti) << ") = " << details.Ct;
            break;
      }
   }

   GET_IFACE2(pBroker,ILosses,pLosses);

   pTable = pgsReportStyleHolder::CreateTableNoHeading(2,"Prestress Losses at Mid Span");
   *p << pTable << rptNewLine;

   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(span,girder,pgsTypes::BridgeSite3,POI_MIDSPAN);
   CHECK(vPoi.size()==1);
   pgsPointOfInterest poi = *vPoi.begin();

   (*pTable)(0,0) << "Prestress Loss at Lifting";
   (*pTable)(0,1) << stress.SetValue( pLosses->GetLiftingLosses(poi,pgsTypes::Permanent) );

   (*pTable)(1,0) << "Prestress Loss at Shipping";
   (*pTable)(1,1) << stress.SetValue( pLosses->GetShippingLosses(poi,pgsTypes::Permanent) );

   (*pTable)(2,0) << "Prestress Loss at Final";
   (*pTable)(2,1) << stress.SetValue( pLosses->GetFinal(poi,pgsTypes::Permanent) );
}

void deflection_and_camber(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,"Camber and Deflection");
   *p << pTable << rptNewLine;

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue, disp, pDisplayUnits->GetDisplacementUnit(), true );
   INIT_FRACTIONAL_LENGTH_PROTOTYPE( camber, IS_US_UNITS(pDisplayUnits), 8, pDisplayUnits->GetDisplacementUnit(), true, false );

   // Get the interfaces we need
   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IProductLoads, pProductLoads);
   GET_IFACE2(pBroker,IProductForces, pProductForces);
   GET_IFACE2( pBroker, ILibrary, pLib );
   GET_IFACE2( pBroker, ISpecification, pSpec );

   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,girder);

   std::string spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   bool do_defl = pSpecEntry->GetDoEvaluateLLDeflection();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   bool bSidewalk = pProductLoads->HasSidewalkLoad(span,girder);

   // Get Midspan std::vector<pgsPointOfInterest>
   std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(span,girder,pgsTypes::BridgeSite3,POI_MIDSPAN);
   CHECK(vPoi.size()==1);
   pgsPointOfInterest poi = *vPoi.begin();

   // Compute mid span deflections
   Float64 delta_gdr;  // due to girder self weight
   Float64 delta_dl;   // due to dead loads on girder
   Float64 delta_sidl; // due to traffic barrier
   Float64 delta_sidewalk; // due to sidewalk
   Float64 delta_overlay; // due to overlay
   Float64 delta_ll;   // due to live load
   Float64 delta_oll;  // due to optoinal live load
   Float64 temp;

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

   delta_gdr = pProductForces->GetGirderDeflectionForCamber( poi );

   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : analysisType == pgsTypes::Continuous ? ContinuousSpan : MinSimpleContinuousEnvelope);

   delta_dl = pProductForces->GetDisplacement(pgsTypes::BridgeSite1, pftSlab, poi, bat )
            + pProductForces->GetDisplacement(pgsTypes::BridgeSite1, pftDiaphragm, poi, bat )
            + pProductForces->GetDisplacement(pgsTypes::BridgeSite1, pftShearKey, poi, bat );

   delta_overlay = pProductForces->GetDisplacement(overlay_stage, pftOverlay, poi, bat );

   delta_sidl = pProductForces->GetDisplacement(pgsTypes::BridgeSite2, pftTrafficBarrier, poi, bat );
   delta_sidewalk = pProductForces->GetDisplacement(pgsTypes::BridgeSite2, pftSidewalk, poi, bat );

   pProductForces->GetLiveLoadDisplacement(pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, bat, true, false, &delta_ll, &temp );

   pProductForces->GetDeflLiveLoadDisplacement(IProductForces::DeflectionLiveLoadEnvelope, poi, &delta_oll, &temp );

   // get # of days for creep

   Float64 min_days = ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Min(), unitMeasure::Day);
   Float64 max_days = ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day);

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   // Populate the table
   Uint16 row = 0;
   if ( deckType == pgsTypes::sdtNone )
      (*pTable)(row,0) << "Estimated camber immediately before superimposed dead loads at "<< min_days<<" days, D";
   else
      (*pTable)(row,0) << "Estimated camber immediately before slab casting at "<< min_days<<" days, D";

   double D = pCamber->GetDCamberForGirderSchedule( poi,CREEP_MINTIME);
   if ( D < 0 )
      (*pTable)(row,1) << color(Red) << camber.SetValue( D ) << color(Black);
   else
      (*pTable)(row,1) << camber.SetValue( D );
   row++;

   if ( deckType == pgsTypes::sdtNone )
      (*pTable)(row,0) << "Estimated camber immediately before superimposed dead loads at "<< max_days<<" days, D";
   else
      (*pTable)(row,0) << "Estimated camber immediately before slab casting  at "<< max_days<<" days, D";

   D = pCamber->GetDCamberForGirderSchedule( poi,CREEP_MAXTIME);
   if ( D < 0 )
      (*pTable)(row,1) << color(Red) << camber.SetValue( D ) << color(Black);
   else
      (*pTable)(row,1) << camber.SetValue( D );
   row++;

   if ( 0 < girderData.Nstrands[pgsTypes::Temporary] && girderData.TempStrandUsage != pgsTypes::ttsPTBeforeShipping )
   {
      (*pTable)(row,0) << "Deflection (Prestressing including temp strands)";
      (*pTable)(row,1) << disp.SetValue( pCamber->GetPrestressDeflection(poi,true) );
   }
   else
   {
      (*pTable)(row,0) << "Deflection (Prestressing)";
      (*pTable)(row,1) << disp.SetValue( pCamber->GetPrestressDeflection(poi,false) );
   }
   row++;

   if ( 0 < girderData.Nstrands[pgsTypes::Temporary] && girderData.TempStrandUsage != pgsTypes::ttsPTBeforeShipping )
   {
      (*pTable)(row,0) << "Deflection (Temporary Strand Removal)";
      (*pTable)(row,1) << disp.SetValue( pCamber->GetReleaseTempPrestressDeflection(poi) );
      row++;
   }

   (*pTable)(row,0) << "Deflection (Girder)";
   (*pTable)(row,1) << disp.SetValue( delta_gdr );
   row++;

   (*pTable)(row,0) << "Deflection (Slab and Diaphragms)";
   (*pTable)(row,1) << disp.SetValue( delta_dl );
   row++;

   if ( bSidewalk )
   {
      (*pTable)(row,0) << "Deflection (Sidewalk)";
      (*pTable)(row,1) << disp.SetValue( delta_sidewalk );
      row++;
   }

   (*pTable)(row,0) << "Deflection (Traffic Barrier)";
   (*pTable)(row,1) << disp.SetValue( delta_sidl );
   row++;

   (*pTable)(row,0) << "Deflection (Overlay)";
   (*pTable)(row,1) << disp.SetValue( delta_overlay );
   row++;

   if ( deckType == pgsTypes::sdtNone )
   {
      (*pTable)(row,0) << "Initial Camber, " << Sub2("C","i");
      (*pTable)(row,1) << camber.SetValue( pCamber->GetCreepDeflection(poi,ICamber::cpReleaseToDiaphragm,CREEP_MAXTIME) );
      row++;

      (*pTable)(row,0) << "Concrete Topping, Barrier, and Overlay " << Sub2("C","topping");
      (*pTable)(row,1) << camber.SetValue( pCamber->GetSlabBarrierOverlayDeflection(poi) );
      row++;
   }
   else
   {
      (*pTable)(row,0) << "Screed Camber, C";
      (*pTable)(row,1) << camber.SetValue( pCamber->GetScreedCamber(poi) );
      row++;
   }

   (*pTable)(row,0) << "Excess Camber" << rptNewLine << "(based on D at " << max_days << " days)";
   double excess_camber = pCamber->GetExcessCamber(poi,CREEP_MAXTIME);
   if ( excess_camber < 0 )
      (*pTable)(row,1) << color(Red) << camber.SetValue( excess_camber ) << color(Black);
   else
      (*pTable)(row,1) << camber.SetValue( excess_camber );
   row++;

   (*pTable)(row,0) << "Live Load Deflection (HL93 - Per Lane)";
   (*pTable)(row,1) << camber.SetValue( delta_ll );
   row++;

   if (do_defl)
   {
      (*pTable)(row,0) << "Optional Live Load Deflection (LRFD 3.6.1.3.2)";
      (*pTable)(row,1) << camber.SetValue( delta_oll );
      row++;
   }
}


void castingyard_stresses(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,"Casting Yard Stresses (At Release)");
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(4, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );

   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(4, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << "Location";
   (*pTable)(0,1) << "Limit State";
   (*pTable)(0,2) << COLHDR("Demand", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,3) << COLHDR("Allowable", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,4) << "Status";

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,   pDisplayUnits->GetAreaUnit(),   true  );

   // Interfaces
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   long NhMax = pStrandGeom->GetMaxStrands(span,girder,pgsTypes::Harped);

   // Get std::vector<pgsPointOfInterest>
   std::vector<pgsPointOfInterest> vPoi;
   std::vector<pgsPointOfInterest>::iterator iter;

   //    PS Xfer from end of girder
   vPoi = pIPOI->GetPointsOfInterest(span,girder,pgsTypes::CastingYard,POI_PSXFER);
   pgsPointOfInterest psxfer_left  = vPoi.front();
   pgsPointOfInterest psxfer_right = vPoi.back();

   //    H from end of girder
   vPoi = pIPOI->GetPointsOfInterest(span,girder,pgsTypes::CastingYard,POI_H);
   CHECK( vPoi.size() == 2 );
   iter = vPoi.begin();
   pgsPointOfInterest h_left  = *iter++;
   pgsPointOfInterest h_right = *iter++;

   //   Harping points
   pgsPointOfInterest hp_left;
   pgsPointOfInterest hp_right;
   Uint16 hp_count = 0;
   if ( 0 < NhMax )
   {
      vPoi = pIPOI->GetPointsOfInterest(span,girder,pgsTypes::CastingYard,POI_HARPINGPOINT);
      hp_count = vPoi.size();
      iter = vPoi.begin();
      hp_left = *iter;
      hp_right = hp_left;
      if ( hp_count > 1 )
      {
         iter++;
         hp_right = *iter;
      }
   }

   // Get artifacts
   const pgsGirderArtifact* pArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsFlexuralStressArtifact* pStresses;

   Float64 fTop,fBot;
   Float64 fAllow;
   
   Float64 AsMin = pArtifact->GetCastingYardMildRebarRequirement();

   // Populate the table
   RowIndexType row = pTable->GetNumberOfHeaderRows();


   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Tension,psxfer_left.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();

   (*pTable)(row,0) << "Top of girder at prestress transfer length from left end (w/o mild rebar)";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->TopPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   fAllow = pArtifact->GetCastingYardCapacityWithMildRebar();
   if ( !IsZero(AsMin) )
   {
      (*pTable)(row,0) << "Top of girder at prestress transfer length from left end (if at least " << area.SetValue(AsMin) << " of mild reinforcement is provided)";
   }
   else
   {
      (*pTable)(row,0) << "Top of girder at prestress transfer length from left end (if bonded reinforcement sufficient to resist the tensile force in the concrete is provided)";
   }

   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( fTop <= fAllow )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Compression,psxfer_left.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();
   (*pTable)(row,0) << "Bottom of girder at prestress transfer length from left end";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Tension,h_left.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();

   (*pTable)(row,0) << "Top of girder at h from left end (w/o mild rebar)";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->TopPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   fAllow = pArtifact->GetCastingYardCapacityWithMildRebar();
   if ( !IsZero(AsMin) )
   {
      (*pTable)(row,0) << "Top of girder at h from left end (if at least " << area.SetValue(AsMin) << " of mild reinforcement is provided)";
   }
   else
   {
      (*pTable)(row,0) << "Top of girder at h from left end (if bonded reinforcement sufficient to resist the tensile force in the concrete is provided)";
   }
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( fTop <= fAllow )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Compression,h_left.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();
   (*pTable)(row,0) << "Bottom of girder at h from left end";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   if ( 0 < NhMax )
   {
      if ( hp_count == 1 )
      {
         pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Tension,hp_left.GetDistFromStart()));
         pStresses->GetDemand( &fTop, &fBot );
         fAllow = pStresses->GetCapacity();
         (*pTable)(row,0) << "Top of girder at harping point (w/o mild rebar)";
         (*pTable)(row,1) << "Service I";
         (*pTable)(row,2) << stress.SetValue( fTop );
         (*pTable)(row,3) << stress.SetValue( fAllow );
         if ( pStresses->TopPassed(pgsFlexuralStressArtifact::WithoutRebar) )
            (*pTable)(row,4) << RPT_PASS;
         else
            (*pTable)(row,4) << RPT_FAIL;
         row++;

         fAllow = pArtifact->GetCastingYardCapacityWithMildRebar();
         if ( !IsZero(AsMin) )
         {
            (*pTable)(row,0) << "Top of girder at harping point (if at least " << area.SetValue(AsMin) << " of mild reinforcement is provided)";
         }
         else
         {
            (*pTable)(row,0) << "Top of girder at harping point (if bonded reinforcement sufficient to resist the tensile force in the concrete is provided)";
         }
         (*pTable)(row,1) << "Service I";
         (*pTable)(row,2) << stress.SetValue( fTop );
         (*pTable)(row,3) << stress.SetValue( fAllow );
         if ( fTop <= fAllow )
            (*pTable)(row,4) << RPT_PASS;
         else
            (*pTable)(row,4) << RPT_FAIL;
         row++;

         pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Compression,hp_left.GetDistFromStart()));
         pStresses->GetDemand( &fTop, &fBot );
         fAllow = pStresses->GetCapacity();
         (*pTable)(row,0) << "Bottom of girder at harping point";
         (*pTable)(row,1) << "Service I";
         (*pTable)(row,2) << stress.SetValue( fBot );
         (*pTable)(row,3) << stress.SetValue( fAllow );
         if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithRebar) )
            (*pTable)(row,4) << RPT_PASS;
         else
            (*pTable)(row,4) << RPT_FAIL;
         row++;
      }
      else
      {
         pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Tension,hp_left.GetDistFromStart()));
         pStresses->GetDemand( &fTop, &fBot );
         fAllow = pStresses->GetCapacity();
         (*pTable)(row,0) << "Top of girder at left harping point (w/o mild rebar)";
         (*pTable)(row,1) << "Service I";
         (*pTable)(row,2) << stress.SetValue( fTop );
         (*pTable)(row,3) << stress.SetValue( fAllow );
         if ( pStresses->TopPassed(pgsFlexuralStressArtifact::WithoutRebar) )
            (*pTable)(row,4) << RPT_PASS;
         else
            (*pTable)(row,4) << RPT_FAIL;
         row++;

         fAllow = pArtifact->GetCastingYardCapacityWithMildRebar();
         if ( !IsZero(AsMin) )
         {
            (*pTable)(row,0) << "Top of girder at left harping point (if at least " << area.SetValue(AsMin) << " of mild reinforcement is provided)";
         }
         else
         {
            (*pTable)(row,0) << "Top of girder at left harping point (if bonded reinforcement sufficient to resist the tensile force in the concrete is provided)";
         }
         (*pTable)(row,1) << "Service I";
         (*pTable)(row,2) << stress.SetValue( fTop );
         (*pTable)(row,3) << stress.SetValue( fAllow );
         if ( fTop <= fAllow )
            (*pTable)(row,4) << RPT_PASS;
         else
            (*pTable)(row,4) << RPT_FAIL;
         row++;

         pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Compression,hp_left.GetDistFromStart()));
         pStresses->GetDemand( &fTop, &fBot );
         fAllow = pStresses->GetCapacity();
         (*pTable)(row,0) << "Bottom of girder at left harping point";
         (*pTable)(row,1) << "Service I";
         (*pTable)(row,2) << stress.SetValue( fBot );
         (*pTable)(row,3) << stress.SetValue( fAllow );
         if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithRebar) )
            (*pTable)(row,4) << RPT_PASS;
         else
            (*pTable)(row,4) << RPT_FAIL;
         row++;

         pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Tension,hp_right.GetDistFromStart()));
         pStresses->GetDemand( &fTop, &fBot );
         fAllow = pStresses->GetCapacity();
         (*pTable)(row,0) << "Top of girder at right harping point (w/o mild rebar)";
         (*pTable)(row,1) << "Service I";
         (*pTable)(row,2) << stress.SetValue( fTop );
         (*pTable)(row,3) << stress.SetValue( fAllow );
         if ( pStresses->TopPassed(pgsFlexuralStressArtifact::WithoutRebar) )
            (*pTable)(row,4) << RPT_PASS;
         else
            (*pTable)(row,4) << RPT_FAIL;
         row++;

         fAllow = pArtifact->GetCastingYardCapacityWithMildRebar();
         if ( !IsZero(AsMin) )
         {
            (*pTable)(row,0) << "Top of girder at right harping point (if at least " << area.SetValue(AsMin) << " of mild reinforcement is provided)";
         }
         else
         {
            (*pTable)(row,0) << "Top of girder at right harping point (if bonded reinforcement sufficient to resist the tensile force in the concrete is provided0";
         }
         (*pTable)(row,1) << "Service I";
         (*pTable)(row,2) << stress.SetValue( fTop );
         (*pTable)(row,3) << stress.SetValue( fAllow );
         if ( fTop <= fAllow )
            (*pTable)(row,4) << RPT_PASS;
         else
            (*pTable)(row,4) << RPT_FAIL;
         row++;

         pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Compression,hp_right.GetDistFromStart()));
         pStresses->GetDemand( &fTop, &fBot );
         fAllow = pStresses->GetCapacity();
         (*pTable)(row,0) << "Bottom of girder at right harping point";
         (*pTable)(row,1) << "Service I";
         (*pTable)(row,2) << stress.SetValue( fBot );
         (*pTable)(row,3) << stress.SetValue( fAllow );
         if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithRebar) )
            (*pTable)(row,4) << RPT_PASS;
         else
            (*pTable)(row,4) << RPT_FAIL;
         row++;
      }
   }

   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Tension,h_right.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();

   (*pTable)(row,0) << "Top of girder at h from right end (w/o mild rebar)";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->TopPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   fAllow = pArtifact->GetCastingYardCapacityWithMildRebar();
   if ( !IsZero(AsMin) )
   {
      (*pTable)(row,0) << "Top of girder at h from right end (if at least " << area.SetValue(AsMin) << " of mild reinforcement is provided)";
   }
   else
   {
      (*pTable)(row,0) << "Top of girder at h from right end (if bonded reinforcement sufficient to resist the tensile force in the concrete is provided)";
   }
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( fTop <= fAllow )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Compression,h_right.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();
   (*pTable)(row,0) << "Bottom of girder at h from right end";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;
}


void bridgesite1_stresses(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   pgsTypes::Stage stage = pgsTypes::BridgeSite1;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,"Deck and Diaphragm Placement Stage Stresses (Bridge Site 1)");
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(4, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );

   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(4, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << "Location";
   (*pTable)(0,1) << "Limit State";
   (*pTable)(0,2) << COLHDR("Demand", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,3) << COLHDR("Allowable", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,4) << "Status";

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   // Interfaces
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   // Get std::vector<pgsPointOfInterest>
   std::vector<pgsPointOfInterest> vPoi;
   std::vector<pgsPointOfInterest>::iterator iter;

   //    H from end of girder
   vPoi = pIPOI->GetPointsOfInterest(span,girder,stage,POI_H);
   CHECK( vPoi.size() == 2 );
   iter = vPoi.begin();
   pgsPointOfInterest h_left  = *iter++;
   pgsPointOfInterest h_right = *iter++;

   //   Midspan
   vPoi = pIPOI->GetPointsOfInterest(span,girder,stage,POI_MIDSPAN);
   pgsPointOfInterest cl = *vPoi.begin();

   // Get artifacts
   const pgsGirderArtifact* pArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsFlexuralStressArtifact* pStresses;

   Float64 fTop,fBot;
   Float64 fAllow;

   // Populate the table
   RowIndexType row = pTable->GetNumberOfHeaderRows();


   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceI,pgsTypes::Tension,h_left.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();

   (*pTable)(row,0) << "Top of girder at h from left end";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->TopPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceI,pgsTypes::Compression,h_left.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();
   (*pTable)(row,0) << "Bottom of girder at h from left end";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceI,pgsTypes::Tension,cl.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();
   (*pTable)(row,0) << "Top of girder at mid-span";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->TopPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceI,pgsTypes::Compression,cl.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();
   (*pTable)(row,0) << "Bottom of girder at mid-span";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceI,pgsTypes::Tension,h_right.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();

   (*pTable)(row,0) << "Top of girder at h from right end";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->TopPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceI,pgsTypes::Compression,h_right.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();
   (*pTable)(row,0) << "Bottom of girder at h from right end";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;
}

void bridgesite2_stresses(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   pgsTypes::Stage stage = pgsTypes::BridgeSite2;
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,"Superimposed Dead Load Stage Stresses (Bridge Site 2)");
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(4, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );

   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(4, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << "Location";
   (*pTable)(0,1) << "Limit State";
   (*pTable)(0,2) << COLHDR("Demand", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,3) << COLHDR("Allowable", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,4) << "Status";

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   // Interfaces
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   // Get std::vector<pgsPointOfInterest>
   std::vector<pgsPointOfInterest> vPoi;
   std::vector<pgsPointOfInterest>::iterator iter;

   //    H from end of girder
   vPoi = pIPOI->GetPointsOfInterest(span,girder,stage,POI_H);
   CHECK( vPoi.size() == 2 );
   iter = vPoi.begin();
   pgsPointOfInterest h_left  = *iter++;
   pgsPointOfInterest h_right = *iter++;

   //   Midspan
   vPoi = pIPOI->GetPointsOfInterest(span,girder,stage,POI_MIDSPAN);
   pgsPointOfInterest cl = *vPoi.begin();

   // Get artifacts
   const pgsGirderArtifact* pArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsFlexuralStressArtifact* pStresses;

   Float64 fTop,fBot;
   Float64 fAllow;

   // Populate the table
   RowIndexType row = pTable->GetNumberOfHeaderRows();


   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceI,pgsTypes::Compression,h_left.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();
   (*pTable)(row,0) << "Bottom of girder at h from left end";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceI,pgsTypes::Compression,cl.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();
   (*pTable)(row,0) << "Top of girder at mid-span";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->TopPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceI,pgsTypes::Compression,h_right.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();
   (*pTable)(row,0) << "Bottom of girder at h from right end";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;
}

void bridgesite3_stresses(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   pgsTypes::Stage stage = pgsTypes::BridgeSite3;
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,"Final with Live Load Stage Stresses (Bridge Site 3)");
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(4, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );

   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(4, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << "Location";
   (*pTable)(0,1) << "Limit State";
   (*pTable)(0,2) << COLHDR("Demand", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,3) << COLHDR("Allowable", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,4) << "Status";

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   // Interfaces
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   // Get std::vector<pgsPointOfInterest>
   std::vector<pgsPointOfInterest> vPoi;
   std::vector<pgsPointOfInterest>::iterator iter;

   //    H from end of girder
   vPoi = pIPOI->GetPointsOfInterest(span,girder,stage,POI_H);
   CHECK( vPoi.size() == 2 );
   iter = vPoi.begin();
   pgsPointOfInterest h_left  = *iter++;
   pgsPointOfInterest h_right = *iter++;

   //   Midspan
   vPoi = pIPOI->GetPointsOfInterest(span,girder,stage,POI_MIDSPAN);
   iter = vPoi.begin();
   pgsPointOfInterest cl = *iter;

   // Get artifacts
   const pgsGirderArtifact* pArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsFlexuralStressArtifact* pStresses;

   Float64 fTop,fBot;
   Float64 fAllow;

   // Populate the table
   RowIndexType row = pTable->GetNumberOfHeaderRows();


   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceI,pgsTypes::Compression,h_left.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();
   (*pTable)(row,0) << "Bottom of girder at h from left end";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceIA,pgsTypes::Compression,h_left.GetDistFromStart()));
      pStresses->GetDemand( &fTop, &fBot );
      fAllow = pStresses->GetCapacity();
      (*pTable)(row,0) << "Bottom of girder at h from left end";
      (*pTable)(row,1) << "Service IA";
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllow );
      if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;
   }
   else
   {
      pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::FatigueI,pgsTypes::Compression,h_left.GetDistFromStart()));
      pStresses->GetDemand( &fTop, &fBot );
      fAllow = pStresses->GetCapacity();
      (*pTable)(row,0) << "Bottom of girder at h from left end";
      (*pTable)(row,1) << "Fatigue I";
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllow );
      if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;
   }

   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceI,pgsTypes::Compression,cl.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();
   (*pTable)(row,0) << "Top of girder at mid-span";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->TopPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceIA,pgsTypes::Compression,cl.GetDistFromStart()));
      pStresses->GetDemand( &fTop, &fBot );
      fAllow = pStresses->GetCapacity();
      (*pTable)(row,0) << "Top of girder at mid-span";
      (*pTable)(row,1) << "Service IA";
      (*pTable)(row,2) << stress.SetValue( fTop );
      (*pTable)(row,3) << stress.SetValue( fAllow );
      if ( pStresses->TopPassed(pgsFlexuralStressArtifact::WithoutRebar) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;
   }
   else
   {
      pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::FatigueI,pgsTypes::Compression,cl.GetDistFromStart()));
      pStresses->GetDemand( &fTop, &fBot );
      fAllow = pStresses->GetCapacity();
      (*pTable)(row,0) << "Top of girder at mid-span";
      (*pTable)(row,1) << "Fatigue I";
      (*pTable)(row,2) << stress.SetValue( fTop );
      (*pTable)(row,3) << stress.SetValue( fAllow );
      if ( pStresses->TopPassed(pgsFlexuralStressArtifact::WithoutRebar) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;
   }

   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceIII,pgsTypes::Tension,cl.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();
   (*pTable)(row,0) << "Bottom of girder at mid-span";
   (*pTable)(row,1) << "Service III";
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceI,pgsTypes::Compression,h_right.GetDistFromStart()));
   pStresses->GetDemand( &fTop, &fBot );
   fAllow = pStresses->GetCapacity();

   (*pTable)(row,0) << "Bottom of girder at h from right end";
   (*pTable)(row,1) << "Service I";
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllow );
   if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::ServiceIA,pgsTypes::Compression,h_right.GetDistFromStart()));
      pStresses->GetDemand( &fTop, &fBot );
      fAllow = pStresses->GetCapacity();

      (*pTable)(row,0) << "Bottom of girder at h from right end";
      (*pTable)(row,1) << "Service IA";
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllow );
      if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;
   }
   else
   {
      pStresses = pArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(stage,pgsTypes::FatigueI,pgsTypes::Compression,h_right.GetDistFromStart()));
      pStresses->GetDemand( &fTop, &fBot );
      fAllow = pStresses->GetCapacity();

      (*pTable)(row,0) << "Bottom of girder at h from right end";
      (*pTable)(row,1) << "Fatigue I";
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllow );
      if ( pStresses->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;
   }
}

void write_shear_capacity(rptRcTable* pTable,int row,const std::string& lbl, const pgsVerticalShearArtifact* pArtifact,IEAFDisplayUnits* pDisplayUnits)
{
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear, pDisplayUnits->GetGeneralForceUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );

   (*pTable)(row,0) << lbl;
   (*pTable)(row,1) << shear.SetValue( pArtifact->GetDemand() );
   (*pTable)(row,2) << shear.SetValue( pArtifact->GetCapacity() );
   if ( pArtifact->Passed() )
      (*pTable)(row,3) << RPT_PASS;
   else
      (*pTable)(row,3) << RPT_FAIL;
}

void shear_capacity(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(4,"Shear Capacity");
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   (*pTable)(0,0) << "Location";
   (*pTable)(0,1) << COLHDR(Sub2("V","u"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*pTable)(0,2) << COLHDR(symbol(phi) << Sub2("V","n"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*pTable)(0,3) << "Status";

   // Setup up some unit value prototypes

   // Interfaces
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   long NhMax = pStrandGeom->GetMaxStrands(span,girder,pgsTypes::Harped);

   // Get Points of Interest
   std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(span,girder,pgsTypes::BridgeSite3,POI_H | POI_15H | POI_HARPINGPOINT,POIFIND_OR);
   CHECK( vPoi.size() == 4 || vPoi.size() == 5 || vPoi.size() == 6 );
   std::vector<pgsPointOfInterest>::iterator iter;
   Uint16 hp_count = vPoi.size() - 4; // subtract 4 because of h and 1.5h left and right
   iter = vPoi.begin();
   pgsPointOfInterest left_h   = *iter++;
   pgsPointOfInterest left_15h = *iter++;

   pgsPointOfInterest hp_left;
   pgsPointOfInterest hp_right;
   if ( 0 < NhMax )
   {
      hp_left = *iter++;
      hp_right = hp_left;
      if ( hp_count > 1 )
         hp_right = *iter++;
   }

   pgsPointOfInterest right_15h = *iter++;
   pgsPointOfInterest right_h   = *iter++;

   pgsPointOfInterest left_cs;
   pgsPointOfInterest right_cs;
   pIPOI->GetCriticalSection(pgsTypes::StrengthI,span,girder,&left_cs,&right_cs);

   const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsStirrupCheckArtifact* pstirrup_artifact= pGdrArtifact->GetStirrupCheckArtifact();
   CHECK(pstirrup_artifact);
   const pgsStirrupCheckAtPoisArtifact* pPoiArtifact;
   const pgsVerticalShearArtifact* pArtifact;

   Int16 row = 1;

   pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact(pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,left_cs.GetDistFromStart()));
   pArtifact = pPoiArtifact->GetVerticalShearArtifact();
   CHECK(pArtifact!=0);
   write_shear_capacity(pTable,row++,"Left Critical Section", pArtifact, pDisplayUnits );

   pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact(pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,left_h.GetDistFromStart()));
   pArtifact = pPoiArtifact->GetVerticalShearArtifact();
   CHECK(pArtifact!=0);
   write_shear_capacity(pTable,row++,"H from left end", pArtifact, pDisplayUnits );
   
   pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact(pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,left_15h.GetDistFromStart()));
   pArtifact = pPoiArtifact->GetVerticalShearArtifact();
   CHECK(pArtifact!=0);
   write_shear_capacity(pTable,row++,"1.5H from left end", pArtifact, pDisplayUnits );

   if ( 0 < NhMax )
   {
      if ( hp_count > 1 )
      {
         pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact(pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,hp_left.GetDistFromStart()));
         pArtifact = pPoiArtifact->GetVerticalShearArtifact();
         CHECK(pArtifact!=0);
         write_shear_capacity(pTable,row++,"Left Harping Point", pArtifact, pDisplayUnits );

         pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact(pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,hp_right.GetDistFromStart()));
         pArtifact = pPoiArtifact->GetVerticalShearArtifact();
         CHECK(pArtifact!=0);
         write_shear_capacity(pTable,row++,"Right Harping Point", pArtifact, pDisplayUnits );
      }
      else
      {
         pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact(pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,hp_left.GetDistFromStart()));
         pArtifact = pPoiArtifact->GetVerticalShearArtifact();
         CHECK(pArtifact!=0);
         write_shear_capacity(pTable,row++,"Harping Point", pArtifact, pDisplayUnits );
      }
   }

   pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact(pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,right_15h.GetDistFromStart()));
   pArtifact = pPoiArtifact->GetVerticalShearArtifact();
   CHECK(pArtifact!=0);
   write_shear_capacity(pTable,row++,"1.5H from right end", pArtifact, pDisplayUnits );

   pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact(pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,right_h.GetDistFromStart()));
   pArtifact = pPoiArtifact->GetVerticalShearArtifact();
   CHECK(pArtifact!=0);
   write_shear_capacity(pTable,row++,"H from right end", pArtifact, pDisplayUnits );

   pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact(pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,right_cs.GetDistFromStart()));
   pArtifact = pPoiArtifact->GetVerticalShearArtifact();
   CHECK(pArtifact!=0);
   write_shear_capacity(pTable,row++,"Right Critical Section", pArtifact, pDisplayUnits );
}

void lifting(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,girder);
   const pgsLiftingCheckArtifact* pLiftArtifact = pArtifact->GetLiftingCheckArtifact();

   if (pLiftArtifact==NULL)
   {
      *p<<"Lifting check not performed because it is not enabled in the library"<<rptNewLine;
      return;
   }

   // unstable girders are a problem
   if (!pLiftArtifact->IsGirderStable())
   {
      rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pTitle;
      *pTitle<<color(Red)<<"Lifting Check Failed - Girder is unstable - CG is higher than pick points"<< color(Black)<<rptNewLine;
   }
   else
   {
      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(4,"Lifting in the Casting Yard");
      *p << pTable << rptNewLine;

      // Setup the table
      pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
      pTable->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT));
      pTable->SetColumnStyle(2, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT));
      pTable->SetColumnStyle(3, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );

      pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
      pTable->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
      pTable->SetStripeRowColumnStyle(2, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
      pTable->SetStripeRowColumnStyle(3, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

      // Setup up some unit value prototypes
      rptRcScalar scalar;
      scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
      scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
      scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );
      INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true );
      INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,   pDisplayUnits->GetAreaUnit(),   true );

      (*pTable)(0,0) << "";
      (*pTable)(0,1) << "Demand";
      (*pTable)(0,2) << "Allowable";
      (*pTable)(0,3) << "Status";

      Float64 min_stress, max_stress;
      Float64 minDistFromStart, maxDistFromStart;
      pLiftArtifact->GetMinMaxStresses(&min_stress, &max_stress, &minDistFromStart, &maxDistFromStart);
      Float64 max_all_stress = pLiftArtifact->GetAllowableTensileStress();
      Float64 min_all_stress = pLiftArtifact->GetAllowableCompressionStress();
      Float64 allow_with_rebar = pLiftArtifact->GetAlternativeTensionAllowableStress();
      Float64 AsMin = pLiftArtifact->GetAlterantiveTensileStressAsMax();

      RowIndexType row = pTable->GetNumberOfHeaderRows();

      (*pTable)(row,0) << "Tensile Stress (w/o mild rebar)";
      (*pTable)(row,1) <<  stress.SetValue(max_stress);
      (*pTable)(row,2) <<  stress.SetValue(max_all_stress);
      if (max_all_stress>=max_stress)
         (*pTable)(row,3) << RPT_PASS;
      else
         (*pTable)(row,3) << RPT_FAIL;
      row++;

      if ( !IsZero(AsMin) )
      {
         (*pTable)(row,0) << "Tensile Stress (if at least " << area.SetValue(AsMin) << " of mild reinforcement is provided)";
      }
      else
      {
         (*pTable)(row,0) << "Tensile Stress (if bonded reinforcement sufficient to resist the tensile force in the concrete is provided)";
      }
      (*pTable)(row,1) <<  stress.SetValue(max_stress);
      (*pTable)(row,2) <<  stress.SetValue(allow_with_rebar);
      if (allow_with_rebar>=max_stress)
         (*pTable)(row,3) << RPT_PASS;
      else
         (*pTable)(row,3) << RPT_FAIL;
      row++;

      (*pTable)(row,0) << "Compressive Stress";
      (*pTable)(row,1) <<  stress.SetValue(min_stress);
      (*pTable)(row,2) <<  stress.SetValue(min_all_stress);
      if (min_all_stress<=min_stress)
         (*pTable)(row,3) << RPT_PASS;
      else
         (*pTable)(row,3) << RPT_FAIL;
      row++;


      Float64 fs_crack = pLiftArtifact->GetMinFsForCracking();
      Float64 all_fs_crack = pLiftArtifact->GetAllowableFsForCracking();
      (*pTable)(row,0) << "F.S. - Cracking";
      (*pTable)(row,1) <<  scalar.SetValue(fs_crack);
      (*pTable)(row,2) <<  scalar.SetValue(all_fs_crack);
      if (all_fs_crack<=fs_crack)
         (*pTable)(row,3) << RPT_PASS;
      else
         (*pTable)(row,3) << RPT_FAIL;
      row++;

      Float64 fs_fail = pLiftArtifact->GetFsFailure();
      Float64 all_fs_fail = pLiftArtifact->GetAllowableFsForFailure();
      (*pTable)(row,0) << "F.S. - Failure";
      (*pTable)(row,1) <<  scalar.SetValue(fs_fail);
      (*pTable)(row,2) <<  scalar.SetValue(all_fs_fail);
      if (pLiftArtifact->PassedFailureCheck())
         (*pTable)(row,3) << RPT_PASS;
      else
         (*pTable)(row,3) << RPT_FAIL;
      row++;
   }
}

void hauling(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,girder);
   const pgsHaulingCheckArtifact* pHaulArtifact = pArtifact->GetHaulingCheckArtifact();

   if (pHaulArtifact==NULL)
   {
      *p<<"Hauling check not performed because it is not enabled in the library"<<rptNewLine;
      return;
   }

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(4,"Hauling to the Bridge Site");
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT) );
   pTable->SetColumnStyle(2, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT) );
   pTable->SetColumnStyle(3, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );

   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT) );
   pTable->SetStripeRowColumnStyle(2, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT) );
   pTable->SetStripeRowColumnStyle(3, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   // Setup up some unit value prototypes
   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetShearUnit(),  true );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,   pDisplayUnits->GetAreaUnit(),   true );

   (*pTable)(0,0) << "";
   (*pTable)(0,1) << "Demand";
   (*pTable)(0,2) << "Allowable";
   (*pTable)(0,3) << "Status";

   Float64 min_stress, max_stress;
   pHaulArtifact->GetMinMaxStresses(&min_stress, &max_stress);
   Float64 max_all_stress = pHaulArtifact->GetAllowableTensileStress();
   Float64 min_all_stress = pHaulArtifact->GetAllowableCompressionStress();
   Float64 allow_with_rebar = pHaulArtifact->GetAlternativeTensionAllowableStress();
   Float64 AsMin = pHaulArtifact->GetAlterantiveTensileStressAsMax();

   Uint16 row = 1;
   (*pTable)(row,0) << "Tensile Stress (w/o mild rebar)";
   (*pTable)(row,1) <<  stress.SetValue(max_stress);
   (*pTable)(row,2) <<  stress.SetValue(max_all_stress);
   if (max_all_stress>=max_stress)
      (*pTable)(row,3) << RPT_PASS;
   else
      (*pTable)(row,3) << RPT_FAIL;
   row++;

   if ( !IsZero(AsMin) )
   {
      (*pTable)(row,0) << "Tensile Stress (if at least " << area.SetValue(AsMin) << " of mild reinforcement is provided)";
   }
   else
   {
      (*pTable)(row,0) << "Tensile Stress (if bonded reinforcement sufficient to resist the tensile force in the concrete is provided)";
   }

   (*pTable)(row,1) <<  stress.SetValue(max_stress);
   (*pTable)(row,2) <<  stress.SetValue(allow_with_rebar);
   if (allow_with_rebar>=max_stress)
      (*pTable)(row,3) << RPT_PASS;
   else
      (*pTable)(row,3) << RPT_FAIL;
   row++;

   (*pTable)(row,0) << "Compressive Stress";
   (*pTable)(row,1) <<  stress.SetValue(min_stress);
   (*pTable)(row,2) <<  stress.SetValue(min_all_stress);
   if (min_all_stress<=min_stress)
      (*pTable)(row,3) << RPT_PASS;
   else
      (*pTable)(row,3) << RPT_FAIL;
   row++;

   Float64 fs_crack = pHaulArtifact->GetMinFsForCracking();
   Float64 all_fs_crack = pHaulArtifact->GetAllowableFsForCracking();
   (*pTable)(row,0) << "F.S. - Cracking";
   (*pTable)(row,1) <<  scalar.SetValue(fs_crack);
   (*pTable)(row,2) <<  scalar.SetValue(all_fs_crack);
   if (all_fs_crack<=fs_crack)
      (*pTable)(row,3) << RPT_PASS;
   else
      (*pTable)(row,3) << RPT_FAIL;
   row++;

   Float64 fs_fail = pHaulArtifact->GetFsRollover();
   Float64 all_fs_fail = pHaulArtifact->GetAllowableFsForRollover();
   (*pTable)(row,0) << "F.S. - Rollover";
   (*pTable)(row,1) <<  scalar.SetValue(fs_fail);
   (*pTable)(row,2) <<  scalar.SetValue(all_fs_fail);
   if (all_fs_fail<=fs_fail)
      (*pTable)(row,3) << RPT_PASS;
   else
      (*pTable)(row,3) << RPT_FAIL;
   row++;

   Float64 wgt = pHaulArtifact->GetGirderWeight();
   Float64 maxwgt = pHaulArtifact->GetMaxGirderWgt();
   (*pTable)(row,0) << "Girder Weight";
   (*pTable)(row,1) <<  force.SetValue(wgt);
   (*pTable)(row,2) <<  force.SetValue(maxwgt);
   if (wgt<=maxwgt)
      (*pTable)(row,3) << RPT_PASS;
   else
      (*pTable)(row,3) << RPT_FAIL;
   row++;
}
