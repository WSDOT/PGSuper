///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <PgsExt\ReportStyleHolder.h>
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
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Intervals.h>

#include <psgLib\SpecLibraryEntry.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\LiftingAnalysisArtifact.h>
#include <PgsExt\StrandData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   COutputSummaryChapter
****************************************************************************/

void castingyard_stresses(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits);
void bridgesite1_stresses(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits);
void bridgesite2_stresses(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits);
void bridgesite3_stresses(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits);
void shear_capacity(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits);
void section_properties(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits);
void creep_and_losses(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits);
void deflection_and_camber(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits);
void lifting(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits);
void hauling(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits);

COutputSummaryChapter::COutputSummaryChapter(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR COutputSummaryChapter::GetName() const
{
   return TEXT("Output Summary");
}

rptChapter* COutputSummaryChapter::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey( pGirderRptSpec->GetGirderKey() );

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridge,pBridge);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,ISpecification,pSpec);
   *pPara << _T("Specification = ") << pSpec->GetSpecification() << rptNewLine;

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pGirderArtifact = pIArtifact->GetGirderArtifact(girderKey);
   
   rptParagraph* p = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << p;
   *p << _T("Status") << rptNewLine;

   p = new rptParagraph;
   *pChapter << p;

   if ( pGirderArtifact->Passed() )
   {
      *p << color(Green) << _T("This girder has passed a comprehensive check") << color(Black) << rptNewLine;
   }
   else
   {
      *p << color(Red) << _T("This girder has failed a comprehensive check") << color(Black) << rptNewLine;
      *p << _T("NOTE: This report does not contain a detailed listing of all analysis checks") << rptNewLine;
   }

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      if ( 1 < nSegments )
      {
         rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }

      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);

      castingyard_stresses( pChapter, pBroker, segmentKey, pDisplayUnits );
      bridgesite1_stresses( pChapter, pBroker, segmentKey, pDisplayUnits );
      bridgesite2_stresses( pChapter, pBroker, segmentKey, pDisplayUnits );
      bridgesite3_stresses( pChapter, pBroker, segmentKey, pDisplayUnits );

      CMomentCapacityParagraphBuilder mcbuilder;
      p = mcbuilder.Build(pRptSpec, level);
      *pChapter << p;

      shear_capacity(        pChapter, pBroker, segmentKey, pDisplayUnits );
      section_properties(    pChapter, pBroker, segmentKey, pDisplayUnits );
      creep_and_losses(      pChapter, pBroker, segmentKey, pDisplayUnits );
      deflection_and_camber( pChapter, pBroker, segmentKey, pDisplayUnits );
      lifting(               pChapter, pBroker, segmentKey, pDisplayUnits );
      hauling(               pChapter, pBroker, segmentKey, pDisplayUnits );
   } // next segment

   return pChapter;
}

CChapterBuilder* COutputSummaryChapter::Clone() const
{
   return new COutputSummaryChapter;
}

void section_properties(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE2(pBroker,IGirder,pGirder);
   bool bPrismaticNonComp = pGirder->IsPrismatic(releaseIntervalIdx,segmentKey);
   bool bPrismaticComp    = pGirder->IsPrismatic(liveLoadIntervalIdx,segmentKey);

   if ( bPrismaticNonComp )
   {
      // non-composite section is prismatic
      if ( bPrismaticComp )
      {
         // prismatc componsite and non-composite properties
         *pPara << CSectionPropertiesTable().Build(pBroker,segmentKey,true,pDisplayUnits);
      }
      else
      {
         // prismatic non-composite properties
         *pPara << CSectionPropertiesTable().Build(pBroker,segmentKey,false,pDisplayUnits);

         // non-prismatic, composite properties
         *pPara << CSectionPropertiesTable2().Build(pBroker,segmentKey,liveLoadIntervalIdx,pDisplayUnits);
      }
   }
   else
   {
      // non-prismatic, non-composite properties
      *pPara << CSectionPropertiesTable2().Build(pBroker,segmentKey,releaseIntervalIdx,pDisplayUnits);

      // non-prismatic, composite properties
      *pPara << CSectionPropertiesTable2().Build(pBroker,segmentKey,liveLoadIntervalIdx,pDisplayUnits);
   }
}

void creep_and_losses(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(2,_T("Creep Coefficients"));
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
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
   bool bTempStrands = (0 < pStrands->Nstrands[pgsTypes::Temporary] && pStrands->TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   for ( Uint16 col = CREEP_MINTIME; col <= CREEP_MAXTIME; col++ )
   {
      (*pTable)(0,col) << (col == CREEP_MINTIME ? _T("Minimum Time") : _T("Maximum Time"));

      CREEPCOEFFICIENTDETAILS details;

      switch( deckType )
      {
         case pgsTypes::sdtCompositeCIP:
         case pgsTypes::sdtCompositeOverlay:
         case pgsTypes::sdtCompositeSIP:
            if ( bTempStrands )
            {
               details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDiaphragm,col);
               (*pTable)(row,col) << symbol(psi) << _T("(") << time2.SetValue(details.t);
               (*pTable)(row,col) << _T(",") << time2.SetValue(details.ti) << _T(") = ") << details.Ct;

               (*pTable)(row,col) << rptNewLine;

               details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,col);
               (*pTable)(row,col) << symbol(psi) << _T("(") << time2.SetValue(details.t);
               (*pTable)(row,col) << _T(",") << time2.SetValue(details.ti) << _T(") = ") << details.Ct;

               (*pTable)(row,col) << rptNewLine;

               details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToDeck,col);
               (*pTable)(row,col) << symbol(psi) << _T("(") << time2.SetValue(details.t);
               (*pTable)(row,col) << _T(",") << time2.SetValue(details.ti) << _T(") = ") << details.Ct;
            }
            else
            {
               details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,col);
               (*pTable)(row,col) << symbol(psi) << _T("(") << time2.SetValue(details.t);
               (*pTable)(row,col) << _T(",") << time2.SetValue(details.ti) << _T(") = ") << details.Ct;
            }
            break;

         case pgsTypes::sdtNone:
               details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDiaphragm,col);
               (*pTable)(row,col) << symbol(psi) << _T("(") << time2.SetValue(details.t);
               (*pTable)(row,col) << _T(",") << time2.SetValue(details.ti) << _T(") = ") << details.Ct;

               (*pTable)(row,col) << rptNewLine;

               details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,col);
               (*pTable)(row,col) << symbol(psi) << _T("(") << time2.SetValue(details.t);
               (*pTable)(row,col) << _T(",") << time2.SetValue(details.ti) << _T(") = ") << details.Ct;

               (*pTable)(row,col) << rptNewLine;

               details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToDeck,col);
               (*pTable)(row,col) << symbol(psi) << _T("(") << time2.SetValue(details.t);
               (*pTable)(row,col) << _T(",") << time2.SetValue(details.ti) << _T(") = ") << details.Ct;

               (*pTable)(row,col) << rptNewLine;

               details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToFinal,col);
               (*pTable)(row,col) << symbol(psi) << _T("(") << time2.SetValue(details.t);
               (*pTable)(row,col) << _T(",") << time2.SetValue(details.ti) << _T(") = ") << details.Ct;

               (*pTable)(row,col) << rptNewLine;

               details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToFinal,col);
               (*pTable)(row,col) << symbol(psi) << _T("(") << time2.SetValue(details.t);
               (*pTable)(row,col) << _T(",") << time2.SetValue(details.ti) << _T(") = ") << details.Ct;

               (*pTable)(row,col) << rptNewLine;

               details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDeckToFinal,col);
               (*pTable)(row,col) << symbol(psi) << _T("(") << time2.SetValue(details.t);
               (*pTable)(row,col) << _T(",") << time2.SetValue(details.ti) << _T(") = ") << details.Ct;
            break;
      }
   }

   GET_IFACE2(pBroker,ILosses,pLosses);

   pTable = pgsReportStyleHolder::CreateTableNoHeading(2,_T("Prestress Losses at Mid Span"));
   *p << pTable << rptNewLine;

   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPoi( pIPOI->GetPointsOfInterest(segmentKey,POI_MIDSPAN) );
   ATLASSERT(vPoi.size()==1);
   pgsPointOfInterest poi( *vPoi.begin() );

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx    = pIntervals->GetLiveLoadInterval();

   (*pTable)(0,0) << _T("Prestress Loss at Lifting");
   (*pTable)(0,1) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Permanent,liftSegmentIntervalIdx,pgsTypes::Middle) );

   (*pTable)(1,0) << _T("Prestress Loss at Shipping");
   (*pTable)(1,1) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Permanent,haulSegmentIntervalIdx,pgsTypes::Middle) );

   (*pTable)(2,0) << _T("Prestress Loss at Final (without live load)");
   (*pTable)(2,1) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Permanent,liveLoadIntervalIdx,pgsTypes::Middle) );
}

void deflection_and_camber(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,_T("Camber and Deflection"));
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

   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   bool do_defl = pSpecEntry->GetDoEvaluateLLDeflection();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   bool bSidewalk = pProductLoads->HasSidewalkLoad(segmentKey);

   // Get Midspan std::vector<pgsPointOfInterest>
   std::vector<pgsPointOfInterest> vPoi( pIPOI->GetPointsOfInterest(segmentKey,POI_MIDSPAN) );
   ATLASSERT(vPoi.size()==1);
   pgsPointOfInterest poi( *vPoi.begin() );

   // Compute mid span deflections
   Float64 delta_gdr;  // due to girder self weight
   Float64 delta_dl;   // due to dead loads on girder
   Float64 delta_sidl; // due to traffic barrier
   Float64 delta_sidewalk; // due to sidewalk
   Float64 delta_overlay; // due to overlay
   Float64 delta_ll;   // due to live load
   Float64 delta_oll;  // due to optoinal live load
   Float64 temp;

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   delta_gdr = pProductForces->GetGirderDeflectionForCamber( poi );

   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   delta_dl = pProductForces->GetDisplacement(castDeckIntervalIdx, pftSlab, poi, bat )
            + pProductForces->GetDisplacement(castDeckIntervalIdx, pftDiaphragm, poi, bat )
            + pProductForces->GetDisplacement(castDeckIntervalIdx, pftShearKey, poi, bat );

   delta_overlay = pProductForces->GetDisplacement(overlayIntervalIdx, pftOverlay, poi, bat );

   delta_sidl = pProductForces->GetDisplacement(railingSystemIntervalIdx, pftTrafficBarrier, poi, bat );
   delta_sidewalk = pProductForces->GetDisplacement(railingSystemIntervalIdx, pftSidewalk, poi, bat );

   pProductForces->GetLiveLoadDisplacement(pgsTypes::lltDesign, liveLoadIntervalIdx, poi, bat, true, false, &delta_ll, &temp );

   pProductForces->GetDeflLiveLoadDisplacement(IProductForces::DeflectionLiveLoadEnvelope, poi, bat, &delta_oll, &temp );

   // get # of days for creep

   Float64 min_days = ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Min(), unitMeasure::Day);
   Float64 max_days = ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   // Populate the table
   RowIndexType row = 0;
   if ( deckType == pgsTypes::sdtNone )
      (*pTable)(row,0) << _T("Estimated camber immediately before superimposed dead loads at ")<< min_days<<_T(" days, D");
   else
      (*pTable)(row,0) << _T("Estimated camber immediately before slab casting at ")<< min_days<<_T(" days, D");

   Float64 D = pCamber->GetDCamberForGirderSchedule( poi,CREEP_MINTIME);
   if ( D < 0 )
      (*pTable)(row,1) << color(Red) << camber.SetValue( D ) << color(Black);
   else
      (*pTable)(row,1) << camber.SetValue( D );
   row++;

   if ( deckType == pgsTypes::sdtNone )
      (*pTable)(row,0) << _T("Estimated camber immediately before superimposed dead loads at ")<< max_days<<_T(" days, D");
   else
      (*pTable)(row,0) << _T("Estimated camber immediately before slab casting  at ")<< max_days<<_T(" days, D");

   D = pCamber->GetDCamberForGirderSchedule( poi,CREEP_MAXTIME);
   if ( D < 0 )
      (*pTable)(row,1) << color(Red) << camber.SetValue( D ) << color(Black);
   else
      (*pTable)(row,1) << camber.SetValue( D );
   row++;

   if ( 0 < pStrands->Nstrands[pgsTypes::Temporary] && pStrands->TempStrandUsage != pgsTypes::ttsPTBeforeShipping )
   {
      (*pTable)(row,0) << _T("Deflection (Prestressing including temp strands)");
      (*pTable)(row,1) << disp.SetValue( pCamber->GetPrestressDeflection(poi,true) );
   }
   else
   {
      (*pTable)(row,0) << _T("Deflection (Prestressing)");
      (*pTable)(row,1) << disp.SetValue( pCamber->GetPrestressDeflection(poi,false) );
   }
   row++;

   if ( 0 < pStrands->Nstrands[pgsTypes::Temporary] && pStrands->TempStrandUsage != pgsTypes::ttsPTBeforeShipping )
   {
      (*pTable)(row,0) << _T("Deflection (Temporary Strand Removal)");
      (*pTable)(row,1) << disp.SetValue( pCamber->GetReleaseTempPrestressDeflection(poi) );
      row++;
   }

   (*pTable)(row,0) << _T("Deflection (Girder)");
   (*pTable)(row,1) << disp.SetValue( delta_gdr );
   row++;

   (*pTable)(row,0) << _T("Deflection (Slab and Diaphragms)");
   (*pTable)(row,1) << disp.SetValue( delta_dl );
   row++;

   if ( bSidewalk )
   {
      (*pTable)(row,0) << _T("Deflection (Sidewalk)");
      (*pTable)(row,1) << disp.SetValue( delta_sidewalk );
      row++;
   }

   (*pTable)(row,0) << _T("Deflection (Traffic Barrier)");
   (*pTable)(row,1) << disp.SetValue( delta_sidl );
   row++;

   (*pTable)(row,0) << _T("Deflection (Overlay)");
   (*pTable)(row,1) << disp.SetValue( delta_overlay );
   row++;

   if ( deckType == pgsTypes::sdtNone )
   {
      (*pTable)(row,0) << _T("Initial Camber, ") << Sub2(_T("C"),_T("i"));
      (*pTable)(row,1) << camber.SetValue( pCamber->GetCreepDeflection(poi,ICamber::cpReleaseToDiaphragm,CREEP_MAXTIME) );
      row++;

      (*pTable)(row,0) << _T("Concrete Topping, Barrier, and Overlay ") << Sub2(_T("C"),_T("topping"));
      (*pTable)(row,1) << camber.SetValue( pCamber->GetSlabBarrierOverlayDeflection(poi) );
      row++;
   }
   else
   {
      (*pTable)(row,0) << _T("Screed Camber, C");
      (*pTable)(row,1) << camber.SetValue( pCamber->GetScreedCamber(poi) );
      row++;
   }

   (*pTable)(row,0) << _T("Excess Camber") << rptNewLine << _T("(based on D at ") << max_days << _T(" days)");
   Float64 excess_camber = pCamber->GetExcessCamber(poi,CREEP_MAXTIME);
   if ( excess_camber < 0 )
      (*pTable)(row,1) << color(Red) << camber.SetValue( excess_camber ) << color(Black);
   else
      (*pTable)(row,1) << camber.SetValue( excess_camber );
   row++;

   (*pTable)(row,0) << _T("Live Load Deflection (HL93 - Per Lane)");
   (*pTable)(row,1) << camber.SetValue( delta_ll );
   row++;

   if (do_defl)
   {
      (*pTable)(row,0) << _T("Optional Live Load Deflection (LRFD 3.6.1.3.2)");
      (*pTable)(row,1) << camber.SetValue( delta_oll );
      row++;
   }
}


void castingyard_stresses(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,_T("Casting Yard Stresses (At Release)"));
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(4, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );

   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(4, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << _T("Location");
   (*pTable)(0,1) << _T("Limit State");
   (*pTable)(0,2) << COLHDR(_T("Demand"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,3) << COLHDR(_T("Allowable"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,4) << _T("Status");

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,   pDisplayUnits->GetAreaUnit(),   true  );

   // Interfaces
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   StrandIndexType NhMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Harped);

   // Get std::vector<pgsPointOfInterest>
   std::vector<pgsPointOfInterest> vPoi;
   std::vector<pgsPointOfInterest>::iterator iter;

   //    PS Xfer from end of segment
   vPoi = pIPOI->GetPointsOfInterest(segmentKey,POI_PSXFER);
   pgsPointOfInterest psxfer_left( vPoi.front() );
   pgsPointOfInterest psxfer_right( vPoi.back() );

   //    H from end of segment
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   pgsPointOfInterest h_left;
   pgsPointOfInterest h_right;
   vPoi = pIPOI->GetPointsOfInterest(segmentKey,POI_H);
   if ( 0 < vPoi.size() )
   {
      iter = vPoi.begin();
      if ( vPoi.size() == 2 )
      {
         h_left  = *iter++;
         h_right = *iter++;
      }
      else
      {
         if ( iter->GetDistFromStart() < segment_length/2 )
            h_left = *iter;
         else
            h_right = *iter;
      }
   }

   //   Harping points
   pgsPointOfInterest hp_left;
   pgsPointOfInterest hp_right;
   std::vector<pgsPointOfInterest>::size_type hp_count = 0;
   if ( 0 < NhMax )
   {
      vPoi = pIPOI->GetPointsOfInterest(segmentKey,POI_HARPINGPOINT);
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
   const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
   const pgsFlexuralStressArtifact* pArtifact;

   Float64 fTop,fBot;
   Float64 fAllowTop, fAllowBot;

   // Populate the table
   RowIndexType row = pTable->GetNumberOfHeaderRows();


   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,psxfer_left.GetID());
   fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
   fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);

   if (pArtifact->WasWithRebarAllowableStressUsed(pgsTypes::BottomGirder))
   {
      (*pTable)(row,0) << _T("Top of girder tension at prestress transfer length from left end (w/ mild rebar) ");
   }
   else
   {
      (*pTable)(row,0) << _T("Top of girder tension at prestress transfer length from left end (w/o mild rebar) ");
   }

   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllowTop );
   if ( pArtifact->Passed(pgsTypes::TopGirder) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,psxfer_left.GetID());

   fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
   fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);

   (*pTable)(row,0) << _T("Bottom of girder compression at prestress transfer length from left end");
   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllowBot );
   if ( pArtifact->Passed(pgsTypes::BottomGirder) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   if ( h_left.GetID() != INVALID_ID )
   {
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,h_left.GetID());

      fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
      fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);

      if (pArtifact->WasWithRebarAllowableStressUsed(pgsTypes::BottomGirder))
      {
         (*pTable)(row,0) << _T("Top of girder tension at h from left end (w/ mild rebar) ");
      }
      else
      {
         (*pTable)(row,0) << _T("Top of girder tension at h from left end (w/o mild rebar) ");
      }

      (*pTable)(row,1) << _T("Service I");
      (*pTable)(row,2) << stress.SetValue( fTop );
      (*pTable)(row,3) << stress.SetValue( fAllowTop );
      if ( pArtifact->Passed(pgsTypes::TopGirder) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;

      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,h_left.GetID());

      fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
      fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);

      (*pTable)(row,0) << _T("Bottom of girder compression at h from left end");
      (*pTable)(row,1) << _T("Service I");
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllowBot );
      if ( pArtifact->Passed(pgsTypes::BottomGirder) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;
   }

   if ( 0 < NhMax )
   {
      if ( hp_count == 1 )
      {
         pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,hp_left.GetID());
         fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
         fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);

         if (pArtifact->WasWithRebarAllowableStressUsed(pgsTypes::BottomGirder))
         {
            (*pTable)(row,0) << _T("Top of girder tension at harping point (w/ mild rebar)");
         }
         else
         {
            (*pTable)(row,0) << _T("Top of girder tension at harping point (w/o mild rebar)");
         }

         (*pTable)(row,1) << _T("Service I");
         (*pTable)(row,2) << stress.SetValue( fTop );
         (*pTable)(row,3) << stress.SetValue( fAllowTop );
         if ( pArtifact->Passed(pgsTypes::TopGirder) )
            (*pTable)(row,4) << RPT_PASS;
         else
            (*pTable)(row,4) << RPT_FAIL;
         row++;

         pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,hp_left.GetID());
         fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
         fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
         (*pTable)(row,0) << _T("Bottom of girder compression at harping point");
         (*pTable)(row,1) << _T("Service I");
         (*pTable)(row,2) << stress.SetValue( fBot );
         (*pTable)(row,3) << stress.SetValue( fAllowBot );
         if ( pArtifact->Passed(pgsTypes::BottomGirder) )
            (*pTable)(row,4) << RPT_PASS;
         else
            (*pTable)(row,4) << RPT_FAIL;
         row++;
      }
      else
      {
         pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,hp_left.GetID());
         fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
         fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);

         if (pArtifact->WasWithRebarAllowableStressUsed(pgsTypes::BottomGirder))
         {
            (*pTable)(row,0) << _T("Top of girder tension at left harping point (w/ mild rebar)");
         }
         else
         {
            (*pTable)(row,0) << _T("Top of girder tension at left harping point (w/o mild rebar)");
         }

         (*pTable)(row,1) << _T("Service I");
         (*pTable)(row,2) << stress.SetValue( fTop );
         (*pTable)(row,3) << stress.SetValue( fAllowBot );
         if ( pArtifact->Passed(pgsTypes::TopGirder) )
            (*pTable)(row,4) << RPT_PASS;
         else
            (*pTable)(row,4) << RPT_FAIL;
         row++;

         pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,hp_left.GetID());
         fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
         fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
         (*pTable)(row,0) << _T("Bottom of girder compression at left harping point");
         (*pTable)(row,1) << _T("Service I");
         (*pTable)(row,2) << stress.SetValue( fBot );
         (*pTable)(row,3) << stress.SetValue( fAllowBot );
         if ( pArtifact->Passed(pgsTypes::BottomGirder) )
            (*pTable)(row,4) << RPT_PASS;
         else
            (*pTable)(row,4) << RPT_FAIL;
         row++;

         pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,hp_right.GetID());
         fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
         fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);

         if (pArtifact->WasWithRebarAllowableStressUsed(pgsTypes::BottomGirder))
         {
            (*pTable)(row,0) << _T("Top of girder tension at right harping point (w/ mild rebar)");
         }
         else
         {
            (*pTable)(row,0) << _T("Top of girder tension at right harping point (w/o mild rebar)");
         }

         (*pTable)(row,1) << _T("Service I");
         (*pTable)(row,2) << stress.SetValue( fTop );
         (*pTable)(row,3) << stress.SetValue( fAllowTop );
         if ( pArtifact->Passed(pgsTypes::TopGirder) )
            (*pTable)(row,4) << RPT_PASS;
         else
            (*pTable)(row,4) << RPT_FAIL;
         row++;

         pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,hp_right.GetID());
         fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
         fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
         (*pTable)(row,0) << _T("Bottom of girder compression at right harping point");
         (*pTable)(row,1) << _T("Service I");
         (*pTable)(row,2) << stress.SetValue( fBot );
         (*pTable)(row,3) << stress.SetValue( fAllowBot );
         if ( pArtifact->Passed(pgsTypes::BottomGirder) )
            (*pTable)(row,4) << RPT_PASS;
         else
            (*pTable)(row,4) << RPT_FAIL;
         row++;
      }
   }

   if ( h_right.GetID() != INVALID_ID )
   {
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,h_right.GetID());
      fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
      fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);

      if (pArtifact->WasWithRebarAllowableStressUsed(pgsTypes::BottomGirder))
      {
         (*pTable)(row,0) << _T("Top of girder tension at h from right end (w/ mild rebar)");
      }
      else
      {
         (*pTable)(row,0) << _T("Top of girder tension at h from right end (w/o mild rebar)");
      }

      (*pTable)(row,1) << _T("Service I");
      (*pTable)(row,2) << stress.SetValue( fTop );
      (*pTable)(row,3) << stress.SetValue( fAllowTop );
      if ( pArtifact->Passed(pgsTypes::TopGirder) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;

      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,h_right.GetID());
      fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
      fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
      (*pTable)(row,0) << _T("Bottom of girder compression at h from right end");
      (*pTable)(row,1) << _T("Service I");
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllowBot );
      if ( pArtifact->Passed(pgsTypes::BottomGirder) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;
   }
}


void bridgesite1_stresses(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetCastDeckInterval();

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,_T("Deck and Diaphragm Placement Stage Stresses (Bridge Site 1)"));
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(4, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );

   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(4, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << _T("Location");
   (*pTable)(0,1) << _T("Limit State");
   (*pTable)(0,2) << COLHDR(_T("Demand"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,3) << COLHDR(_T("Allowable"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,4) << _T("Status");

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   // Interfaces
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IBridge,pBridge);

   // Get std::vector<pgsPointOfInterest>
   std::vector<pgsPointOfInterest> vPoi;
   std::vector<pgsPointOfInterest>::iterator iter;

   //    H from end of segment
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   pgsPointOfInterest h_left;
   pgsPointOfInterest h_right;
   vPoi = pIPOI->GetPointsOfInterest(segmentKey,POI_H);
   if ( 0 < vPoi.size() )
   {
      iter = vPoi.begin();
      if ( vPoi.size() == 2 )
      {
         h_left  = *iter++;
         h_right = *iter++;
      }
      else
      {
         if ( iter->GetDistFromStart() < segment_length/2 )
            h_left = *iter;
         else
            h_right = *iter;
      }
   }

   //   Midspan
   vPoi = pIPOI->GetPointsOfInterest(segmentKey,POI_MIDSPAN);
   pgsPointOfInterest cl( *vPoi.begin() );

   // Get artifacts
   const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
   const pgsFlexuralStressArtifact* pArtifact;

   Float64 fTop,fBot;
   Float64 fAllowTop, fAllowBot;

   // Populate the table
   RowIndexType row = pTable->GetNumberOfHeaderRows();


   if ( h_left.GetID() != INVALID_ID )
   {
   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,h_left.GetID());
   fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
   fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);

   (*pTable)(row,0) << _T("Top of girder at h from left end");
   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllowTop );
   if ( pArtifact->Passed(pgsTypes::TopGirder) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,h_left.GetID());
   fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
   fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
   (*pTable)(row,0) << _T("Bottom of girder at h from left end");
   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllowBot );
   if ( pArtifact->Passed(pgsTypes::BottomGirder) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;
   }

   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,cl.GetID());
   fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
   fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);
   (*pTable)(row,0) << _T("Top of girder at mid-span");
   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllowTop );
   if ( pArtifact->Passed(pgsTypes::TopGirder) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,cl.GetID());
   fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
   fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
   (*pTable)(row,0) << _T("Bottom of girder at mid-span");
   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllowBot );
   if ( pArtifact->Passed(pgsTypes::BottomGirder) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   if ( h_right.GetID() != INVALID_ID )
   {
   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,h_right.GetID());
   fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
   fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);

   (*pTable)(row,0) << _T("Top of girder at h from right end");
   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllowTop );
   if ( pArtifact->Passed(pgsTypes::TopGirder) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,h_right.GetID());
   fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
   fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
   (*pTable)(row,0) << _T("Bottom of girder at h from right end");
   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllowBot );
   if ( pArtifact->Passed(pgsTypes::BottomGirder) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;
  }
}

void bridgesite2_stresses(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetCompositeDeckInterval();

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,_T("Superimposed Dead Load Stage Stresses (Bridge Site 2)"));
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(4, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );

   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(4, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << _T("Location");
   (*pTable)(0,1) << _T("Limit State");
   (*pTable)(0,2) << COLHDR(_T("Demand"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,3) << COLHDR(_T("Allowable"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,4) << _T("Status");

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   // Interfaces
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IBridge,pBridge);

   // Get std::vector<pgsPointOfInterest>
   std::vector<pgsPointOfInterest> vPoi;
   std::vector<pgsPointOfInterest>::iterator iter;

   //    H from end of segment
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   pgsPointOfInterest h_left;
   pgsPointOfInterest h_right;
   vPoi = pIPOI->GetPointsOfInterest(segmentKey,POI_H);
   if ( 0 < vPoi.size() )
   {
      iter = vPoi.begin();
      if ( vPoi.size() == 2 )
      {
         h_left  = *iter++;
         h_right = *iter++;
      }
      else
      {
         if ( iter->GetDistFromStart() < segment_length/2 )
            h_left = *iter;
         else
            h_right = *iter;
      }
   }

   //   Midspan
   vPoi = pIPOI->GetPointsOfInterest(segmentKey,POI_MIDSPAN);
   pgsPointOfInterest cl = *vPoi.begin();

   // Get artifacts
   const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
   const pgsFlexuralStressArtifact* pArtifact;

   Float64 fTop,fBot;
   Float64 fAllowTop, fAllowBot;

   // Populate the table
   RowIndexType row = pTable->GetNumberOfHeaderRows();

   if ( h_left.GetID() != INVALID_ID )
   {
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,h_left.GetID());
      fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
      fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
      (*pTable)(row,0) << _T("Bottom of girder at h from left end");
      (*pTable)(row,1) << _T("Service I");
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllowBot );
      if ( pArtifact->Passed(pgsTypes::BottomGirder) )
        (*pTable)(row,4) << RPT_PASS;
      else
        (*pTable)(row,4) << RPT_FAIL;
      row++;
   }

   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,cl.GetID());
   fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
   fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);
   (*pTable)(row,0) << _T("Top of girder at mid-span");
   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllowTop );
   if ( pArtifact->Passed(pgsTypes::TopGirder) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   if ( h_right.GetID() != INVALID_ID )
   {
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,h_right.GetID());
      fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
      fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
      (*pTable)(row,0) << _T("Bottom of girder at h from right end");
      (*pTable)(row,1) << _T("Service I");
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllowBot );
      if ( pArtifact->Passed(pgsTypes::BottomGirder) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;
   }
}

void bridgesite3_stresses(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,_T("Final with Live Load Stage Stresses (Bridge Site 3)"));
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(4, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );

   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(4, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << _T("Location");
   (*pTable)(0,1) << _T("Limit State");
   (*pTable)(0,2) << COLHDR(_T("Demand"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,3) << COLHDR(_T("Allowable"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,4) << _T("Status");

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   // Interfaces
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IBridge,pBridge);

   // Get std::vector<pgsPointOfInterest>
   std::vector<pgsPointOfInterest> vPoi;
   std::vector<pgsPointOfInterest>::iterator iter;

   //    H from end of segment
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   pgsPointOfInterest h_left;
   pgsPointOfInterest h_right;
   vPoi = pIPOI->GetPointsOfInterest(segmentKey,POI_H);
   if ( 0 < vPoi.size() )
   {
      iter = vPoi.begin();
      if ( vPoi.size() == 2 )
      {
         h_left  = *iter++;
         h_right = *iter++;
      }
      else
      {
         if ( iter->GetDistFromStart() < segment_length/2 )
            h_left = *iter;
         else
            h_right = *iter;
      }
   }

   //   Midspan
   vPoi = pIPOI->GetPointsOfInterest(segmentKey,POI_MIDSPAN);
   ATLASSERT(vPoi.size() == 1);
   iter = vPoi.begin();
   pgsPointOfInterest cl( *iter );

   // Get artifacts
   const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
   const pgsFlexuralStressArtifact* pArtifact;

   Float64 fTop,fBot;
   Float64 fAllowTop, fAllowBot;

   // Populate the table
   RowIndexType row = pTable->GetNumberOfHeaderRows();

   if ( h_left.GetID() != INVALID_ID )
   {
   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(liveLoadIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,h_left.GetID());
   fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
   fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
   (*pTable)(row,0) << _T("Bottom of girder at h from left end");
   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllowBot );
   if ( pArtifact->Passed(pgsTypes::BottomGirder) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(liveLoadIntervalIdx,pgsTypes::ServiceIA,pgsTypes::Compression,h_left.GetID());
      fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
      fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
      (*pTable)(row,0) << _T("Bottom of girder at h from left end");
      (*pTable)(row,1) << _T("Service IA");
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllowBot );
      if ( pArtifact->Passed(pgsTypes::BottomGirder) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;
   }
   else
   {
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(liveLoadIntervalIdx,pgsTypes::FatigueI,pgsTypes::Compression,h_left.GetID());
      fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
      fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
      (*pTable)(row,0) << _T("Bottom of girder at h from left end");
      (*pTable)(row,1) << _T("Fatigue I");
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllowBot );
      if ( pArtifact->Passed(pgsTypes::BottomGirder) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;
   }
  }

   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(liveLoadIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,cl.GetID());
   fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
   fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);
   (*pTable)(row,0) << _T("Top of girder at mid-span");
   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllowTop );
   if ( pArtifact->Passed(pgsTypes::TopGirder) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(liveLoadIntervalIdx,pgsTypes::ServiceIA,pgsTypes::Compression,cl.GetID());
      fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
      fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);
      (*pTable)(row,0) << _T("Top of girder at mid-span");
      (*pTable)(row,1) << _T("Service IA");
      (*pTable)(row,2) << stress.SetValue( fTop );
      (*pTable)(row,3) << stress.SetValue( fAllowTop );
      if ( pArtifact->Passed(pgsTypes::TopGirder) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;
   }
   else
   {
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(liveLoadIntervalIdx,pgsTypes::FatigueI,pgsTypes::Compression,cl.GetID());
      fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
      fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);
      (*pTable)(row,0) << _T("Top of girder at mid-span");
      (*pTable)(row,1) << _T("Fatigue I");
      (*pTable)(row,2) << stress.SetValue( fTop );
      (*pTable)(row,3) << stress.SetValue( fAllowTop );
      if ( pArtifact->Passed(pgsTypes::TopGirder) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;
   }

   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(liveLoadIntervalIdx,pgsTypes::ServiceIII,pgsTypes::Tension,cl.GetID());
   fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
   fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
   (*pTable)(row,0) << _T("Bottom of girder at mid-span");
   (*pTable)(row,1) << _T("Service III");
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllowBot );
   if ( pArtifact->Passed(pgsTypes::BottomGirder) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   if ( h_right.GetID() != INVALID_ID )
   {
   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(liveLoadIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,h_right.GetID());
   fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
   fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);

   (*pTable)(row,0) << _T("Bottom of girder at h from right end");
   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllowBot );
   if ( pArtifact->Passed(pgsTypes::BottomGirder) )
      (*pTable)(row,4) << RPT_PASS;
   else
      (*pTable)(row,4) << RPT_FAIL;
   row++;

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(liveLoadIntervalIdx,pgsTypes::ServiceIA,pgsTypes::Compression,h_right.GetID());
      fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
      fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);

      (*pTable)(row,0) << _T("Bottom of girder at h from right end");
      (*pTable)(row,1) << _T("Service IA");
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllowBot );
      if ( pArtifact->Passed(pgsTypes::BottomGirder) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;
   }
   else
   {
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(liveLoadIntervalIdx,pgsTypes::FatigueI,pgsTypes::Compression,h_right.GetID());
      fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
      fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);

      (*pTable)(row,0) << _T("Bottom of girder at h from right end");
      (*pTable)(row,1) << _T("Fatigue I");
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllowBot );
      if ( pArtifact->Passed(pgsTypes::BottomGirder) )
         (*pTable)(row,4) << RPT_PASS;
      else
         (*pTable)(row,4) << RPT_FAIL;
      row++;
   }
  }
}

void write_shear_capacity(rptRcTable* pTable,RowIndexType row,const std::_tstring& lbl, const pgsVerticalShearArtifact* pArtifact,IEAFDisplayUnits* pDisplayUnits)
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

void shear_capacity(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(4,_T("Shear Capacity"));
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   (*pTable)(0,0) << _T("Location");
   (*pTable)(0,1) << COLHDR(Sub2(_T("V"),_T("u")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*pTable)(0,2) << COLHDR(symbol(phi) << Sub2(_T("V"),_T("n")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*pTable)(0,3) << _T("Status");

   // Setup up some unit value prototypes

   // Interfaces
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   StrandIndexType NhMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Harped);

   // Get Points of Interest
   std::vector<pgsPointOfInterest> vPoi;
   std::vector<pgsPointOfInterest>::iterator iter;

   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   pgsPointOfInterest h_left;
   pgsPointOfInterest h_right;
   vPoi = pIPOI->GetPointsOfInterest(segmentKey,POI_H);
   if ( 0 < vPoi.size() )
   {
      iter = vPoi.begin();
      if ( vPoi.size() == 2 )
      {
         h_left  = *iter++;
         h_right = *iter++;
      }
      else
      {
         if ( iter->GetDistFromStart() < segment_length/2 )
            h_left = *iter;
         else
            h_right = *iter;
      }
   }

   pgsPointOfInterest left_15h;
   pgsPointOfInterest right_15h;
   vPoi = pIPOI->GetPointsOfInterest(segmentKey,POI_15H);
   if ( 0 < vPoi.size() )
   {
      iter = vPoi.begin();
      if ( vPoi.size() == 2 )
      {
         left_15h  = *iter++;
         right_15h = *iter++;
      }
      else
      {
         if ( iter->GetDistFromStart() < segment_length/2 )
            left_15h = *iter;
         else
            right_15h = *iter;
      }
   }

   pgsPointOfInterest hp_left;
   pgsPointOfInterest hp_right;
   IndexType hp_count = 0;
   vPoi = pIPOI->GetPointsOfInterest(segmentKey,POI_HARPINGPOINT);
   if ( 0 < vPoi.size() && 0 < NhMax )
   {
      iter = vPoi.begin();
      if ( vPoi.size() == 2 )
      {
         hp_left  = *iter++;
         hp_right = *iter++;
         hp_count = 2;
      }
      else
      {
         hp_count = 1;
         if ( iter->GetDistFromStart() < segment_length/2 )
            hp_left = *iter;
         else
            hp_right = *iter;
      }
   }

#pragma Reminder("UPDATE: assuming precast girder bridge")
   std::vector<pgsPointOfInterest> vCsPoi(pIPOI->GetCriticalSections(pgsTypes::StrengthI,segmentKey));
   ATLASSERT(vPoi.size() == 2);
   pgsPointOfInterest left_cs( vCsPoi.front() );
   pgsPointOfInterest right_cs( vCsPoi.back() );

   const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
   const pgsStirrupCheckArtifact* pstirrup_artifact = pSegmentArtifact->GetStirrupCheckArtifact();
   ATLASSERT(pstirrup_artifact != NULL);
   const pgsStirrupCheckAtPoisArtifact* pPoiArtifact;
   const pgsVerticalShearArtifact* pArtifact;

   RowIndexType row = 1;

   pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(liveLoadIntervalIdx,pgsTypes::StrengthI,left_cs.GetID());
   if ( pPoiArtifact == NULL )
      return;

   pArtifact = pPoiArtifact->GetVerticalShearArtifact();
   ATLASSERT(pArtifact!=0);
   write_shear_capacity(pTable,row++,_T("Left Critical Section"), pArtifact, pDisplayUnits );

   if ( h_left.GetID() != INVALID_ID )
   {
      pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(liveLoadIntervalIdx,pgsTypes::StrengthI,h_left.GetID());
      pArtifact = pPoiArtifact->GetVerticalShearArtifact();
      ATLASSERT(pArtifact!=0);
      write_shear_capacity(pTable,row++,_T("H from left end"), pArtifact, pDisplayUnits );
   }
   
   if ( left_15h.GetID() != INVALID_ID )
   {
      pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(liveLoadIntervalIdx,pgsTypes::StrengthI,left_15h.GetID());
      pArtifact = pPoiArtifact->GetVerticalShearArtifact();
      ATLASSERT(pArtifact!=0);
      write_shear_capacity(pTable,row++,_T("1.5H from left end"), pArtifact, pDisplayUnits );
  }

   if ( 0 < NhMax )
   {
      if ( 1 < hp_count )
      {
         pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(liveLoadIntervalIdx,pgsTypes::StrengthI,hp_left.GetID());
         pArtifact = pPoiArtifact->GetVerticalShearArtifact();
         ATLASSERT(pArtifact!=0);
         write_shear_capacity(pTable,row++,_T("Left Harping Point"), pArtifact, pDisplayUnits );

         pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(liveLoadIntervalIdx,pgsTypes::StrengthI,hp_right.GetID());
         pArtifact = pPoiArtifact->GetVerticalShearArtifact();
         ATLASSERT(pArtifact!=0);
         write_shear_capacity(pTable,row++,_T("Right Harping Point"), pArtifact, pDisplayUnits );
      }
      else
      {
         pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(liveLoadIntervalIdx,pgsTypes::StrengthI,hp_left.GetID());
         pArtifact = pPoiArtifact->GetVerticalShearArtifact();
         ATLASSERT(pArtifact!=0);
         write_shear_capacity(pTable,row++,_T("Harping Point"), pArtifact, pDisplayUnits );
      }
   }

   if ( right_15h.GetID() != INVALID_ID )
   {
      pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(liveLoadIntervalIdx,pgsTypes::StrengthI,right_15h.GetID());
      pArtifact = pPoiArtifact->GetVerticalShearArtifact();
      ATLASSERT(pArtifact!=0);
      write_shear_capacity(pTable,row++,_T("1.5H from right end"), pArtifact, pDisplayUnits );
   }

   if ( h_right.GetID() != INVALID_ID )
   {
      pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(liveLoadIntervalIdx,pgsTypes::StrengthI,h_right.GetID());
      pArtifact = pPoiArtifact->GetVerticalShearArtifact();
      ATLASSERT(pArtifact!=0);
      write_shear_capacity(pTable,row++,_T("H from right end"), pArtifact, pDisplayUnits );
   }

   pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(liveLoadIntervalIdx,pgsTypes::StrengthI,right_cs.GetID());
   pArtifact = pPoiArtifact->GetVerticalShearArtifact();
   ATLASSERT(pArtifact!=0);
   write_shear_capacity(pTable,row++,_T("Right Critical Section"), pArtifact, pDisplayUnits );
}

void lifting(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsSegmentArtifact* pSegmentArtifact = pArtifacts->GetSegmentArtifact(segmentKey);
   const pgsLiftingAnalysisArtifact* pLiftArtifact = pSegmentArtifact->GetLiftingAnalysisArtifact();

   if (pLiftArtifact==NULL)
   {
      *p<<_T("Lifting check not performed because it is not enabled in the library")<<rptNewLine;
      return;
   }

   // unstable girders are a problem
   if (!pLiftArtifact->IsGirderStable())
   {
      rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pTitle;
      *pTitle<<color(Red)<<_T("Lifting Check Failed - Girder is unstable - CG is higher than pick points")<< color(Black)<<rptNewLine;
   }
   else
   {
      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(4,_T("Lifting in the Casting Yard"));
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

      (*pTable)(0,0) << _T("");
      (*pTable)(0,1) << _T("Demand");
      (*pTable)(0,2) << _T("Allowable");
      (*pTable)(0,3) << _T("Status");

      Float64 min_stress, max_stress;
      Float64 minDistFromStart, maxDistFromStart;
      pLiftArtifact->GetMinMaxStresses(&min_stress, &max_stress, &minDistFromStart, &maxDistFromStart);
      //Float64 max_all_stress = pLiftArtifact->GetAllowableTensileStress();
      //Float64 min_all_stress = pLiftArtifact->GetAllowableCompressionStress();
      //Float64 allow_with_rebar = pLiftArtifact->GetAlternativeTensionAllowableStress();
      //Float64 AsMin = pLiftArtifact->GetAlterantiveTensileStressAsMax();
      GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
      Float64 max_all_stress  = pGirderLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStress(segmentKey);
      Float64 allow_with_rebar = pGirderLiftingSpecCriteria->GetLiftingWithMildRebarAllowableStress(segmentKey);
      Float64 min_all_stress     = pGirderLiftingSpecCriteria->GetLiftingAllowableCompressiveConcreteStress(segmentKey);

      RowIndexType row = pTable->GetNumberOfHeaderRows();

      (*pTable)(row,0) << _T("Tensile Stress (w/o mild rebar)");
      (*pTable)(row,1) <<  stress.SetValue(max_stress);
      (*pTable)(row,2) <<  stress.SetValue(max_all_stress);
      if (max_all_stress>=max_stress)
         (*pTable)(row,3) << RPT_PASS;
      else
         (*pTable)(row,3) << RPT_FAIL;
      row++;

      (*pTable)(row,0) << _T("Tensile Stress (if bonded reinforcement sufficient to resist the tensile force in the concrete is provided)");

      (*pTable)(row,1) <<  stress.SetValue(max_stress);
      (*pTable)(row,2) <<  stress.SetValue(allow_with_rebar);
      if (allow_with_rebar>=max_stress)
         (*pTable)(row,3) << RPT_PASS;
      else
         (*pTable)(row,3) << RPT_FAIL;
      row++;

      (*pTable)(row,0) << _T("Compressive Stress");
      (*pTable)(row,1) <<  stress.SetValue(min_stress);
      (*pTable)(row,2) <<  stress.SetValue(min_all_stress);
      if (min_all_stress<=min_stress)
         (*pTable)(row,3) << RPT_PASS;
      else
         (*pTable)(row,3) << RPT_FAIL;
      row++;


      Float64 fs_crack = pLiftArtifact->GetMinFsForCracking();
      Float64 all_fs_crack = pLiftArtifact->GetAllowableFsForCracking();
      (*pTable)(row,0) << _T("F.S. - Cracking");
      (*pTable)(row,1) <<  scalar.SetValue(fs_crack);
      (*pTable)(row,2) <<  scalar.SetValue(all_fs_crack);
      if (all_fs_crack<=fs_crack)
         (*pTable)(row,3) << RPT_PASS;
      else
         (*pTable)(row,3) << RPT_FAIL;
      row++;

      Float64 fs_fail = pLiftArtifact->GetFsFailure();
      Float64 all_fs_fail = pLiftArtifact->GetAllowableFsForFailure();
      (*pTable)(row,0) << _T("F.S. - Failure");
      (*pTable)(row,1) <<  scalar.SetValue(fs_fail);
      (*pTable)(row,2) <<  scalar.SetValue(all_fs_fail);
      if (pLiftArtifact->PassedFailureCheck())
         (*pTable)(row,3) << RPT_PASS;
      else
         (*pTable)(row,3) << RPT_FAIL;
      row++;
   }
}

void hauling(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsSegmentArtifact* pSegmentArtifact = pArtifacts->GetSegmentArtifact(segmentKey);
   const pgsHaulingAnalysisArtifact* pHaulArtifact_base = pSegmentArtifact->GetHaulingAnalysisArtifact();
   const pgsWsdotHaulingAnalysisArtifact* pHaulArtifact = dynamic_cast<const pgsWsdotHaulingAnalysisArtifact*>(pHaulArtifact_base);

   if (pHaulArtifact==NULL)
   {
      *p<<_T("WSDOT Hauling check not performed because it is not enabled in the library")<<rptNewLine;
      return;
   }

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(4,_T("Hauling to the Bridge Site"));
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

   (*pTable)(0,0) << _T("");
   (*pTable)(0,1) << _T("Demand");
   (*pTable)(0,2) << _T("Allowable");
   (*pTable)(0,3) << _T("Status");

   Float64 min_stress, max_stress;
   pHaulArtifact->GetMinMaxStresses(&min_stress, &max_stress);
   //Float64 max_all_stress = pHaulArtifact->GetAllowableTensileStress();
   //Float64 min_all_stress = pHaulArtifact->GetAllowableCompressionStress();
   //Float64 allow_with_rebar = pHaulArtifact->GetAlternativeTensionAllowableStress();
   //Float64 AsMin = pHaulArtifact->GetAlterantiveTensileStressAsMax();

   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   Float64 max_all_stress  = pGirderHaulingSpecCriteria->GetHaulingAllowableTensileConcreteStress(segmentKey);
   Float64 allow_with_rebar = pGirderHaulingSpecCriteria->GetHaulingWithMildRebarAllowableStress(segmentKey);
   Float64 min_all_stress     = pGirderHaulingSpecCriteria->GetHaulingAllowableCompressiveConcreteStress(segmentKey);

   RowIndexType row = 1;
   (*pTable)(row,0) << _T("Tensile Stress (w/o mild rebar)");
   (*pTable)(row,1) <<  stress.SetValue(max_stress);
   (*pTable)(row,2) <<  stress.SetValue(max_all_stress);
   if (max_all_stress>=max_stress)
      (*pTable)(row,3) << RPT_PASS;
   else
      (*pTable)(row,3) << RPT_FAIL;
   row++;

   (*pTable)(row,0) << _T("Tensile Stress (if bonded reinforcement sufficient to resist the tensile force in the concrete is provided)");

   (*pTable)(row,1) <<  stress.SetValue(max_stress);
   (*pTable)(row,2) <<  stress.SetValue(allow_with_rebar);
   if (allow_with_rebar>=max_stress)
      (*pTable)(row,3) << RPT_PASS;
   else
      (*pTable)(row,3) << RPT_FAIL;
   row++;

   (*pTable)(row,0) << _T("Compressive Stress");
   (*pTable)(row,1) <<  stress.SetValue(min_stress);
   (*pTable)(row,2) <<  stress.SetValue(min_all_stress);
   if (min_all_stress<=min_stress)
      (*pTable)(row,3) << RPT_PASS;
   else
      (*pTable)(row,3) << RPT_FAIL;
   row++;

   Float64 fs_crack = pHaulArtifact->GetMinFsForCracking();
   Float64 all_fs_crack = pHaulArtifact->GetAllowableFsForCracking();
   (*pTable)(row,0) << _T("F.S. - Cracking");
   (*pTable)(row,1) <<  scalar.SetValue(fs_crack);
   (*pTable)(row,2) <<  scalar.SetValue(all_fs_crack);
   if (all_fs_crack<=fs_crack)
      (*pTable)(row,3) << RPT_PASS;
   else
      (*pTable)(row,3) << RPT_FAIL;
   row++;

   Float64 fs_fail = pHaulArtifact->GetFsRollover();
   Float64 all_fs_fail = pHaulArtifact->GetAllowableFsForRollover();
   (*pTable)(row,0) << _T("F.S. - Rollover");
   (*pTable)(row,1) <<  scalar.SetValue(fs_fail);
   (*pTable)(row,2) <<  scalar.SetValue(all_fs_fail);
   if (all_fs_fail<=fs_fail)
      (*pTable)(row,3) << RPT_PASS;
   else
      (*pTable)(row,3) << RPT_FAIL;
   row++;

   Float64 wgt = pHaulArtifact->GetGirderWeight();
   Float64 maxwgt = pHaulArtifact->GetMaxGirderWgt();
   (*pTable)(row,0) << _T("Girder Weight");
   (*pTable)(row,1) <<  force.SetValue(wgt);
   (*pTable)(row,2) <<  force.SetValue(maxwgt);
   if (wgt<=maxwgt)
      (*pTable)(row,3) << RPT_PASS;
   else
      (*pTable)(row,3) << RPT_FAIL;
   row++;
}
