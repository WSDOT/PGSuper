///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include <IFace\Allowables.h>

#include <psgLib\SpecLibraryEntry.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
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
   
   rptParagraph* p = new rptParagraph( rptStyleManager::GetHeadingStyle() );
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
         rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
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
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;

   GET_IFACE2(pBroker,IGirder,pGirder);
   bool bPrismaticNonComp = pGirder->IsPrismatic(releaseIntervalIdx,segmentKey);
   bool bPrismaticComp    = pGirder->IsPrismatic(lastIntervalIdx,segmentKey);

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
         *pPara << CSectionPropertiesTable2().Build(pBroker,pgsTypes::sptGross,segmentKey,lastIntervalIdx,pDisplayUnits);
      }
   }
   else
   {
      // non-prismatic, non-composite properties
      *pPara << CSectionPropertiesTable2().Build(pBroker,pgsTypes::sptGross,segmentKey,releaseIntervalIdx,pDisplayUnits);

      // non-prismatic, composite properties
      *pPara << CSectionPropertiesTable2().Build(pBroker,pgsTypes::sptGross,segmentKey,lastIntervalIdx,pDisplayUnits);
   }
}

void creep_and_losses(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(2,_T("Creep Coefficients"));
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetWholeDaysUnit(), false );

   // Get the interfaces we need
   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
   bool bTempStrands = (0 < pStrands->GetStrandCount(pgsTypes::Temporary) && pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPTBeforeShipping) ? true : false;

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
         case pgsTypes::sdtNonstructuralOverlay:
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

         default:
            ATLASSERT(false);
      }
   }

   GET_IFACE2(pBroker,IPretensionForce,pPretensionForce);

   pTable = rptStyleManager::CreateTableNoHeading(2,_T("Effective Prestress at Mid Span"));
   *p << pTable << rptNewLine;

   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   PoiList vPoi;
   pIPOI->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPoi);
   ATLASSERT(vPoi.size()==1);
   const pgsPointOfInterest& poi( vPoi.front() );

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType lastIntervalIdx        = pIntervals->GetIntervalCount()-1;

   (*pTable)(0,0) << _T("Effective Prestress at Lifting");
   (*pTable)(0,1) << stress.SetValue( pPretensionForce->GetEffectivePrestress(poi,pgsTypes::Permanent,liftSegmentIntervalIdx,pgsTypes::Middle) );

   (*pTable)(1,0) << _T("Effective Prestress at Shipping");
   (*pTable)(1,1) << stress.SetValue( pPretensionForce->GetEffectivePrestress(poi,pgsTypes::Permanent,haulSegmentIntervalIdx,pgsTypes::Middle) );

   (*pTable)(2,0) << _T("Effective Prestress at Final (without live load)");
   (*pTable)(2,1) << stress.SetValue( pPretensionForce->GetEffectivePrestress(poi,pgsTypes::Permanent,lastIntervalIdx,pgsTypes::Middle) );

   (*pTable)(3,0) << _T("Effective Prestress at Final with Live Load (Service I)");
   (*pTable)(3,1) << stress.SetValue( pPretensionForce->GetEffectivePrestress(poi,pgsTypes::Permanent,lastIntervalIdx,pgsTypes::Middle) );

   (*pTable)(4,0) << _T("Effective Prestress at Final with Live Load (Service III)");
   (*pTable)(4,1) << stress.SetValue( pPretensionForce->GetEffectivePrestress(poi,pgsTypes::Permanent,lastIntervalIdx,pgsTypes::Middle) );
}

void deflection_and_camber(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2,_T("Camber and Deflection"));
   *p << pTable << rptNewLine;

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue, disp, pDisplayUnits->GetDeflectionUnit(), true );
   INIT_FRACTIONAL_LENGTH_PROTOTYPE( camber, IS_US_UNITS(pDisplayUnits), 8, pDisplayUnits->GetDeflectionUnit(), true, false );

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
   PoiList vPoi;
   pIPOI->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPoi);
   ATLASSERT(vPoi.size()==1);
   const pgsPointOfInterest& poi( vPoi.front() );

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
   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();
   IntervalIndexType castShearKeyIntervalIdx  = pIntervals->GetCastShearKeyInterval();
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType lastIntervalIdx          = pIntervals->GetIntervalCount()-1;

   delta_gdr = pProductForces->GetGirderDeflectionForCamber( poi );

   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   delta_dl = pProductForces->GetDeflection(castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm, poi, bat, rtIncremental, false);
   if (castDeckIntervalIdx != INVALID_INDEX)
   {
      delta_dl += pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi, bat, rtIncremental, false);
   }

   if (castShearKeyIntervalIdx != INVALID_INDEX)
   {
      delta_dl += pProductForces->GetDeflection(castShearKeyIntervalIdx, pgsTypes::pftShearKey, poi, bat, rtIncremental, false);
   }

   if ( overlayIntervalIdx == INVALID_INDEX )
   {
      delta_overlay = 0;
   }
   else
   {
      delta_overlay = pProductForces->GetDeflection(overlayIntervalIdx, pgsTypes::pftOverlay, poi, bat, rtIncremental, false );
   }

   delta_sidl = pProductForces->GetDeflection(railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtIncremental, false );
   delta_sidewalk = pProductForces->GetDeflection(railingSystemIntervalIdx, pgsTypes::pftSidewalk, poi, bat, rtIncremental, false );

   pProductForces->GetLiveLoadDeflection(lastIntervalIdx, pgsTypes::lltDesign, poi, bat, true, false, &delta_ll, &temp );

   pProductForces->GetDeflLiveLoadDeflection(IProductForces::DeflectionLiveLoadEnvelope, poi, bat, &delta_oll, &temp );

   // get # of days for creep

   Float64 min_days = ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Min(), unitMeasure::Day);
   Float64 max_days = ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day);
   Float64 tFinal_days = ::ConvertFromSysUnits(pSpecEntry->GetTotalCreepDuration(), unitMeasure::Day);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   // Populate the table
   RowIndexType row = 0;
   if ( IsNonstructuralDeck(deckType) )
   {
      (*pTable)(row,0) << _T("Estimated camber immediately before superimposed dead loads at ")<< min_days<<_T(" days, D");
   }
   else
   {
      (*pTable)(row,0) << _T("Estimated camber immediately before deck casting at ")<< min_days<<_T(" days, D");
   }

   Float64 D = pCamber->GetDCamberForGirderSchedule( poi,CREEP_MINTIME);
   if ( D < 0 )
   {
      (*pTable)(row,1) << color(Red) << camber.SetValue( D ) << color(Black);
   }
   else
   {
      (*pTable)(row,1) << camber.SetValue( D );
   }
   row++;

   if ( IsNonstructuralDeck(deckType) )
   {
      (*pTable)(row,0) << _T("Estimated camber immediately before superimposed dead loads at ")<< max_days<<_T(" days, D");
   }
   else
   {
      (*pTable)(row,0) << _T("Estimated camber immediately before deck casting  at ")<< max_days<<_T(" days, D");
   }

   D = pCamber->GetDCamberForGirderSchedule( poi,CREEP_MAXTIME);
   if ( D < 0 )
   {
      (*pTable)(row,1) << color(Red) << camber.SetValue( D ) << color(Black);
   }
   else
   {
      (*pTable)(row,1) << camber.SetValue( D );
   }
   row++;

   if ( 0 < pStrands->GetStrandCount(pgsTypes::Temporary) && pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPTBeforeShipping )
   {
      (*pTable)(row,0) << _T("Deflection (Prestressing including temp strands)");
      (*pTable)(row,1) << disp.SetValue( pCamber->GetPrestressDeflection(poi,pgsTypes::pddErected) );
   }
   else
   {
      (*pTable)(row,0) << _T("Deflection (Prestressing)");
      (*pTable)(row,1) << disp.SetValue( pCamber->GetPrestressDeflection(poi,pgsTypes::pddRelease) );
   }
   row++;

   if ( 0 < pStrands->GetStrandCount(pgsTypes::Temporary) && pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPTBeforeShipping )
   {
      (*pTable)(row,0) << _T("Deflection (Temporary Strand Removal)");
      (*pTable)(row,1) << disp.SetValue( pCamber->GetReleaseTempPrestressDeflection(poi) );
      row++;
   }

   (*pTable)(row,0) << _T("Deflection (Girder)");
   (*pTable)(row,1) << disp.SetValue( delta_gdr );
   row++;

   (*pTable)(row,0) << _T("Deflection (Deck and Diaphragms)");
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

   if ( IsNonstructuralDeck(deckType) )
   {
      (*pTable)(row,0) << _T("Initial Camber, ") << Sub2(_T("C"),_T("i"));
      (*pTable)(row,1) << camber.SetValue( pCamber->GetInitialCamber(poi) );
      row++;

      (*pTable)(row, 0) << _T("Deflection due to Concrete Topping, Barrier, and Overlay, ") << Sub2(_T("C"), _T("SIDL"));
      (*pTable)(row, 1) << camber.SetValue(pCamber->GetSlabBarrierOverlayDeflection(poi));
      row++;

      Float64 C = pCamber->GetExcessCamber(poi, CREEP_MAXTIME);
      (*pTable)(row, 0) << _T("Camber at ") << tFinal_days << _T(" days, ") << Sub2(_T("C"), _T("F"));
      if (C < 0)
      {
         (*pTable)(row, 1) << color(Red) << camber.SetValue(C) << color(Black);
      }
      else
      {
         (*pTable)(row, 1) << camber.SetValue(C);
      }
      row++;
   }
   else
   {
      (*pTable)(row,0) << _T("Screed Camber, C");
      (*pTable)(row,1) << camber.SetValue( pCamber->GetScreedCamber(poi,CREEP_MAXTIME) );
      row++;

      (*pTable)(row, 0) << _T("Excess Camber") << rptNewLine << _T("(based on D at ") << max_days << _T(" days)");
      Float64 excess_camber = pCamber->GetExcessCamber(poi, CREEP_MAXTIME);
      if (excess_camber < 0)
      {
         (*pTable)(row, 1) << color(Red) << camber.SetValue(excess_camber) << color(Black);
      }
      else
      {
         (*pTable)(row, 1) << camber.SetValue(excess_camber);
      }
      row++;
   }


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

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(5,pIntervals->GetDescription(releaseIntervalIdx));
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(4, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );

   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(4, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << _T("Location");
   (*pTable)(0,1) << _T("Limit State");
   (*pTable)(0,2) << COLHDR(_T("Demand"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,3) << COLHDR(_T("Limit"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,4) << _T("Status");

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,   pDisplayUnits->GetAreaUnit(),   true  );

   // Interfaces
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   StrandIndexType NhMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Harped);

   //    PS Xfer from end of segment
   PoiList vPoi;
   pIPOI->GetPointsOfInterest(segmentKey, POI_PSXFER, &vPoi);
   const pgsPointOfInterest& PSXFR_left( vPoi.front() );
   const pgsPointOfInterest& PSXFR_right( vPoi.back() );

   //   Harping points
   pgsPointOfInterest hp_left;
   pgsPointOfInterest hp_right;
   std::vector<pgsPointOfInterest>::size_type hp_count = 0;
   if ( 0 < NhMax )
   {
      vPoi.clear();
      pIPOI->GetPointsOfInterest(segmentKey,POI_HARPINGPOINT,&vPoi);
      ATLASSERT( 0 <= vPoi.size() && vPoi.size() <= 2 );
      hp_count = vPoi.size();
      auto iter = vPoi.begin();
      hp_left = *iter;
      hp_right = hp_left;
      if ( 1 < hp_count )
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


   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,PSXFR_left.GetID());
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
   {
      (*pTable)(row,4) << RPT_PASS;
   }
   else
   {
      (*pTable)(row,4) << RPT_FAIL;
   }
   row++;

   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,PSXFR_left.GetID());

   fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
   fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);

   (*pTable)(row,0) << _T("Bottom of girder compression at prestress transfer length from left end");
   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllowBot );
   if ( pArtifact->Passed(pgsTypes::BottomGirder) )
   {
      (*pTable)(row,4) << RPT_PASS;
   }
   else
   {
      (*pTable)(row,4) << RPT_FAIL;
   }
   row++;

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
         {
            (*pTable)(row,4) << RPT_PASS;
         }
         else
         {
            (*pTable)(row,4) << RPT_FAIL;
         }
         row++;

         pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,hp_left.GetID());
         fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
         fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
         (*pTable)(row,0) << _T("Bottom of girder compression at harping point");
         (*pTable)(row,1) << _T("Service I");
         (*pTable)(row,2) << stress.SetValue( fBot );
         (*pTable)(row,3) << stress.SetValue( fAllowBot );
         if ( pArtifact->Passed(pgsTypes::BottomGirder) )
         {
            (*pTable)(row,4) << RPT_PASS;
         }
         else
         {
            (*pTable)(row,4) << RPT_FAIL;
         }
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
         {
            (*pTable)(row,4) << RPT_PASS;
         }
         else
         {
            (*pTable)(row,4) << RPT_FAIL;
         }
         row++;

         pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,hp_left.GetID());
         fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
         fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
         (*pTable)(row,0) << _T("Bottom of girder compression at left harping point");
         (*pTable)(row,1) << _T("Service I");
         (*pTable)(row,2) << stress.SetValue( fBot );
         (*pTable)(row,3) << stress.SetValue( fAllowBot );
         if ( pArtifact->Passed(pgsTypes::BottomGirder) )
         {
            (*pTable)(row,4) << RPT_PASS;
         }
         else
         {
            (*pTable)(row,4) << RPT_FAIL;
         }
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
         {
            (*pTable)(row,4) << RPT_PASS;
         }
         else
         {
            (*pTable)(row,4) << RPT_FAIL;
         }
         row++;

         pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,hp_right.GetID());
         fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
         fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
         (*pTable)(row,0) << _T("Bottom of girder compression at right harping point");
         (*pTable)(row,1) << _T("Service I");
         (*pTable)(row,2) << stress.SetValue( fBot );
         (*pTable)(row,3) << stress.SetValue( fAllowBot );
         if ( pArtifact->Passed(pgsTypes::BottomGirder) )
         {
            (*pTable)(row,4) << RPT_PASS;
         }
         else
         {
            (*pTable)(row,4) << RPT_FAIL;
         }
         row++;
      }
   }

   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,PSXFR_right.GetID());
   fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
   fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);

   if (pArtifact->WasWithRebarAllowableStressUsed(pgsTypes::BottomGirder))
   {
      (*pTable)(row,0) << _T("Top of girder tension at prestress transfer length from right end (w/ mild rebar) ");
   }
   else
   {
      (*pTable)(row,0) << _T("Top of girder tension at prestress transfer length from right end (w/o mild rebar) ");
   }

   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllowTop );
   if ( pArtifact->Passed(pgsTypes::TopGirder) )
   {
      (*pTable)(row,4) << RPT_PASS;
   }
   else
   {
      (*pTable)(row,4) << RPT_FAIL;
   }
   row++;

   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,PSXFR_right.GetID());

   fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
   fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);

   (*pTable)(row,0) << _T("Bottom of girder compression at prestress transfer length from right end");
   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllowBot );
   if ( pArtifact->Passed(pgsTypes::BottomGirder) )
   {
      (*pTable)(row,4) << RPT_PASS;
   }
   else
   {
      (*pTable)(row,4) << RPT_FAIL;
   }
   row++;
}


void bridgesite1_stresses(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowable);
   if ( !pAllowable->CheckTemporaryStresses() )
   {
      return;
   }


   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType noncompositeIntervalIdx = pIntervals->GetLastNoncompositeInterval();

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(5,pIntervals->GetDescription(noncompositeIntervalIdx));
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(4, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );

   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(4, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << _T("Location");
   (*pTable)(0,1) << _T("Limit State");
   (*pTable)(0,2) << COLHDR(_T("Demand"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,3) << COLHDR(_T("Limit"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,4) << _T("Status");

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   // Interfaces
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IBridge,pBridge);

   // PSXFR from end of segment (used to be at H)
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   pgsPointOfInterest h_left;
   pgsPointOfInterest h_right;
   PoiList vPoi;
   pIPOI->GetPointsOfInterest(segmentKey, POI_PSXFER, &vPoi);
   if ( 0 < vPoi.size() )
   {
      auto iter = vPoi.begin();
      if ( vPoi.size() == 2 )
      {
         h_left  = *iter++;
         h_right = *iter++;
      }
      else
      {
         if ( iter->get().GetDistFromStart() < segment_length/2 )
         {
            h_left = *iter;
         }
         else
         {
            h_right = *iter;
         }
      }
   }

   //   Midspan
   vPoi.clear();
   pIPOI->GetPointsOfInterest(segmentKey, POI_5L | POI_SPAN, &vPoi);
   const pgsPointOfInterest& cl( vPoi.front() );

   // Get artifacts
   const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
   const pgsFlexuralStressArtifact* pArtifact;

   Float64 fTop,fBot;
   Float64 fAllowTop, fAllowBot;

   // Populate the table
   RowIndexType row = pTable->GetNumberOfHeaderRows();


   if ( h_left.GetID() != INVALID_ID )
   {
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(noncompositeIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,h_left.GetID());
      fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
      fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);

      (*pTable)(row,0) << _T("Top of girder at PSXFR from left end");
      (*pTable)(row,1) << _T("Service I");
      (*pTable)(row,2) << stress.SetValue( fTop );
      (*pTable)(row,3) << stress.SetValue( fAllowTop );
      if ( pArtifact->Passed(pgsTypes::TopGirder) )
      {
         (*pTable)(row,4) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,4) << RPT_FAIL;
      }
      row++;

      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(noncompositeIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,h_left.GetID());
      fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
      fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
      (*pTable)(row,0) << _T("Bottom of girder at PSXFR from left end");
      (*pTable)(row,1) << _T("Service I");
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllowBot );
      if ( pArtifact->Passed(pgsTypes::BottomGirder) )
      {
         (*pTable)(row,4) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,4) << RPT_FAIL;
      }
      row++;
   }

   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(noncompositeIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,cl.GetID());
   fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
   fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);
   (*pTable)(row,0) << _T("Top of girder at mid-span");
   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fTop );
   (*pTable)(row,3) << stress.SetValue( fAllowTop );
   if ( pArtifact->Passed(pgsTypes::TopGirder) )
   {
      (*pTable)(row,4) << RPT_PASS;
   }
   else
   {
      (*pTable)(row,4) << RPT_FAIL;
   }
   row++;

   pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(noncompositeIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,cl.GetID());
   fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
   fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
   (*pTable)(row,0) << _T("Bottom of girder at mid-span");
   (*pTable)(row,1) << _T("Service I");
   (*pTable)(row,2) << stress.SetValue( fBot );
   (*pTable)(row,3) << stress.SetValue( fAllowBot );
   if ( pArtifact->Passed(pgsTypes::BottomGirder) )
   {
      (*pTable)(row,4) << RPT_PASS;
   }
   else
   {
      (*pTable)(row,4) << RPT_FAIL;
   }
   row++;

   if ( h_right.GetID() != INVALID_ID )
   {
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(noncompositeIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,h_right.GetID());
      fTop      = pArtifact->GetDemand(pgsTypes::TopGirder);
      fAllowTop = pArtifact->GetCapacity(pgsTypes::TopGirder);

      (*pTable)(row,0) << _T("Top of girder at PSXFR from right end");
      (*pTable)(row,1) << _T("Service I");
      (*pTable)(row,2) << stress.SetValue( fTop );
      (*pTable)(row,3) << stress.SetValue( fAllowTop );
      if ( pArtifact->Passed(pgsTypes::TopGirder) )
      {
         (*pTable)(row,4) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,4) << RPT_FAIL;
      }
      row++;

      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(noncompositeIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,h_right.GetID());
      fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
      fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
      (*pTable)(row,0) << _T("Bottom of girder at PSXFR from right end");
      (*pTable)(row,1) << _T("Service I");
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllowBot );
      if ( pArtifact->Passed(pgsTypes::BottomGirder) )
      {
         (*pTable)(row,4) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,4) << RPT_FAIL;
      }
      row++;
  }
}

void bridgesite2_stresses(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();

   IntervalIndexType intervalIdx = (overlayIntervalIdx == INVALID_INDEX ? railingSystemIntervalIdx : Max(railingSystemIntervalIdx, overlayIntervalIdx));

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(5,pIntervals->GetDescription(intervalIdx));
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(4, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );

   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(4, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << _T("Location");
   (*pTable)(0,1) << _T("Limit State");
   (*pTable)(0,2) << COLHDR(_T("Demand"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,3) << COLHDR(_T("Limit"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,4) << _T("Status");

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   // Interfaces
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IBridge,pBridge);

   // PSXFR from end of segment (used to be H)
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   pgsPointOfInterest h_left;
   pgsPointOfInterest h_right;
   PoiList vPoi;
   pIPOI->GetPointsOfInterest(segmentKey, POI_PSXFER, &vPoi);
   if ( 0 < vPoi.size() )
   {
      auto iter = vPoi.begin();
      if ( vPoi.size() == 2 )
      {
         h_left  = *iter++;
         h_right = *iter++;
      }
      else
      {
         if ( iter->get().GetDistFromStart() < segment_length/2 )
         {
            h_left = *iter;
         }
         else
         {
            h_right = *iter;
         }
      }
   }

   //   Midspan
   vPoi.clear();
   pIPOI->GetPointsOfInterest(segmentKey, POI_5L | POI_SPAN, &vPoi);
   const pgsPointOfInterest& cl(vPoi.front());

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
      (*pTable)(row,0) << _T("Bottom of girder at PSXFR from left end");
      (*pTable)(row,1) << _T("Service I");
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllowBot );
      if ( pArtifact->Passed(pgsTypes::BottomGirder) )
      {
        (*pTable)(row,4) << RPT_PASS;
      }
      else
      {
        (*pTable)(row,4) << RPT_FAIL;
      }
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
   {
      (*pTable)(row,4) << RPT_PASS;
   }
   else
   {
      (*pTable)(row,4) << RPT_FAIL;
   }
   row++;

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowable);
   if ( pAllowable->CheckFinalDeadLoadTensionStress() )
   {
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,pgsTypes::ServiceI,pgsTypes::Tension,cl.GetID());
      fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
      fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
      (*pTable)(row,0) << _T("Bottom of girder at mid-span");
      (*pTable)(row,1) << _T("Service I");
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllowBot );
      if ( pArtifact->Passed(pgsTypes::BottomGirder) )
      {
         (*pTable)(row,4) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,4) << RPT_FAIL;
      }
      row++;
   }

   if ( h_right.GetID() != INVALID_ID )
   {
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,h_right.GetID());
      fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
      fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
      (*pTable)(row,0) << _T("Bottom of girder at PSXFR from right end");
      (*pTable)(row,1) << _T("Service I");
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllowBot );
      if ( pArtifact->Passed(pgsTypes::BottomGirder) )
      {
         (*pTable)(row,4) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,4) << RPT_FAIL;
      }
      row++;
   }
}

void bridgesite3_stresses(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(5,pIntervals->GetDescription(liveLoadIntervalIdx));
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(4, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );

   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(4, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << _T("Location");
   (*pTable)(0,1) << _T("Limit State");
   (*pTable)(0,2) << COLHDR(_T("Demand"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,3) << COLHDR(_T("Limit"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,4) << _T("Status");

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   // Interfaces
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IBridge,pBridge);

   // PSXFR from end of segment (used to be H)
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   pgsPointOfInterest h_left;
   pgsPointOfInterest h_right;
   PoiList vPoi;
   pIPOI->GetPointsOfInterest(segmentKey, POI_PSXFER, &vPoi);
   if ( 0 < vPoi.size() )
   {
      auto iter = vPoi.begin();
      if ( vPoi.size() == 2 )
      {
         h_left  = *iter++;
         h_right = *iter++;
      }
      else
      {
         if ( iter->get().GetDistFromStart() < segment_length/2 )
         {
            h_left = *iter;
         }
         else
         {
            h_right = *iter;
         }
      }
   }

   //   Midspan
   vPoi.clear();
   pIPOI->GetPointsOfInterest(segmentKey, POI_5L | POI_SPAN, &vPoi);
   ATLASSERT(vPoi.size() == 1);
   const pgsPointOfInterest& cl(vPoi.front());

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
      (*pTable)(row,0) << _T("Bottom of girder at PSXFR from left end");
      (*pTable)(row,1) << _T("Service I");
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllowBot );
      if ( pArtifact->Passed(pgsTypes::BottomGirder) )
      {
         (*pTable)(row,4) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,4) << RPT_FAIL;
      }
      row++;

      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(liveLoadIntervalIdx,pgsTypes::ServiceIA,pgsTypes::Compression,h_left.GetID());
         fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
         fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
         (*pTable)(row,0) << _T("Bottom of girder at PSXFR from left end");
         (*pTable)(row,1) << _T("Service IA");
         (*pTable)(row,2) << stress.SetValue( fBot );
         (*pTable)(row,3) << stress.SetValue( fAllowBot );
         if ( pArtifact->Passed(pgsTypes::BottomGirder) )
         {
            (*pTable)(row,4) << RPT_PASS;
         }
         else
         {
            (*pTable)(row,4) << RPT_FAIL;
         }
         row++;
      }
      else
      {
         pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(liveLoadIntervalIdx,pgsTypes::FatigueI,pgsTypes::Compression,h_left.GetID());
         fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
         fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);
         (*pTable)(row,0) << _T("Bottom of girder at PSXFR from left end");
         (*pTable)(row,1) << _T("Fatigue I");
         (*pTable)(row,2) << stress.SetValue( fBot );
         (*pTable)(row,3) << stress.SetValue( fAllowBot );
         if ( pArtifact->Passed(pgsTypes::BottomGirder) )
         {
            (*pTable)(row,4) << RPT_PASS;
         }
         else
         {
            (*pTable)(row,4) << RPT_FAIL;
         }
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
   {
      (*pTable)(row,4) << RPT_PASS;
   }
   else
   {
      (*pTable)(row,4) << RPT_FAIL;
   }
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
      {
         (*pTable)(row,4) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,4) << RPT_FAIL;
      }
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
      {
         (*pTable)(row,4) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,4) << RPT_FAIL;
      }
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
   {
      (*pTable)(row,4) << RPT_PASS;
   }
   else
   {
      (*pTable)(row,4) << RPT_FAIL;
   }
   row++;

   if ( h_right.GetID() != INVALID_ID )
   {
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(liveLoadIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,h_right.GetID());
      fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
      fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);

      (*pTable)(row,0) << _T("Bottom of girder at PSXFR from right end");
      (*pTable)(row,1) << _T("Service I");
      (*pTable)(row,2) << stress.SetValue( fBot );
      (*pTable)(row,3) << stress.SetValue( fAllowBot );
      if ( pArtifact->Passed(pgsTypes::BottomGirder) )
      {
         (*pTable)(row,4) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,4) << RPT_FAIL;
      }
      row++;

      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(liveLoadIntervalIdx,pgsTypes::ServiceIA,pgsTypes::Compression,h_right.GetID());
         fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
         fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);

         (*pTable)(row,0) << _T("Bottom of girder at PSXFR from right end");
         (*pTable)(row,1) << _T("Service IA");
         (*pTable)(row,2) << stress.SetValue( fBot );
         (*pTable)(row,3) << stress.SetValue( fAllowBot );
         if ( pArtifact->Passed(pgsTypes::BottomGirder) )
         {
            (*pTable)(row,4) << RPT_PASS;
         }
         else
         {
            (*pTable)(row,4) << RPT_FAIL;
         }
         row++;
      }
      else
      {
         pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(liveLoadIntervalIdx,pgsTypes::FatigueI,pgsTypes::Compression,h_right.GetID());
         fBot      = pArtifact->GetDemand(pgsTypes::BottomGirder);
         fAllowBot = pArtifact->GetCapacity(pgsTypes::BottomGirder);

         (*pTable)(row,0) << _T("Bottom of girder at PSXFR from right end");
         (*pTable)(row,1) << _T("Fatigue I");
         (*pTable)(row,2) << stress.SetValue( fBot );
         (*pTable)(row,3) << stress.SetValue( fAllowBot );
         if ( pArtifact->Passed(pgsTypes::BottomGirder) )
         {
            (*pTable)(row,4) << RPT_PASS;
         }
         else
         {
            (*pTable)(row,4) << RPT_FAIL;
         }
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
   {
      (*pTable)(row,3) << RPT_PASS;
   }
   else
   {
      (*pTable)(row,3) << RPT_FAIL;
   }
}

void shear_capacity(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(4,_T("Shear Capacity"));
   *p << pTable << rptNewLine;

   // Setup the table
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
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

   // this method assumes a precast-prestressed girder bridge
   ATLASSERT(pBridge->GetSegmentCount(segmentKey) == 1);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;

   StrandIndexType NhMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Harped);

   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   pgsPointOfInterest h_left;
   pgsPointOfInterest h_right;
   PoiList vPoi;
   pIPOI->GetPointsOfInterest(segmentKey, POI_H, &vPoi);
   if ( 0 < vPoi.size() )
   {
      auto iter = vPoi.begin();
      if ( vPoi.size() == 2 )
      {
         h_left  = *iter++;
         h_right = *iter++;
      }
      else
      {
         if ( iter->get().GetDistFromStart() < segment_length/2 )
         {
            h_left = *iter;
         }
         else
         {
            h_right = *iter;
         }
      }
   }

   pgsPointOfInterest left_15h;
   pgsPointOfInterest right_15h;
   vPoi.clear();
   pIPOI->GetPointsOfInterest(segmentKey, POI_15H,&vPoi);
   if ( 0 < vPoi.size() )
   {
      auto iter = vPoi.begin();
      if ( vPoi.size() == 2 )
      {
         left_15h  = *iter++;
         right_15h = *iter++;
      }
      else
      {
         if ( iter->get().GetDistFromStart() < segment_length/2 )
         {
            left_15h = *iter;
         }
         else
         {
            right_15h = *iter;
         }
      }
   }

   pgsPointOfInterest hp_left;
   pgsPointOfInterest hp_right;
   IndexType hp_count = 0;
   vPoi.clear();
   pIPOI->GetPointsOfInterest(segmentKey, POI_HARPINGPOINT,&vPoi);
   ATLASSERT( 0 <= vPoi.size() && vPoi.size() <= 2 );
   if ( 0 < vPoi.size() && 0 < NhMax )
   {
      auto iter = vPoi.begin();
      if ( vPoi.size() == 2 )
      {
         hp_left  = *iter++;
         hp_right = *iter++;
         hp_count = 2;
      }
      else
      {
         hp_count = 1;
         if ( iter->get().GetDistFromStart() < segment_length/2 )
         {
            hp_left = *iter;
         }
         else
         {
            hp_right = *iter;
         }
      }
   }

   PoiList vCsPoi;
   pIPOI->GetCriticalSections(pgsTypes::StrengthI, segmentKey, &vCsPoi);
   const pgsPointOfInterest& left_cs( vCsPoi.front() );
   const pgsPointOfInterest& right_cs( vCsPoi.back() );

   const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
   const pgsStirrupCheckArtifact* pstirrup_artifact = pSegmentArtifact->GetStirrupCheckArtifact();
   ATLASSERT(pstirrup_artifact != nullptr);

   RowIndexType row = 1;

   const pgsStirrupCheckAtPoisArtifact*pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(lastIntervalIdx,pgsTypes::StrengthI,left_cs.GetID());
   if ( pPoiArtifact == nullptr )
   {
      return;
   }

   const pgsVerticalShearArtifact*pArtifact = pPoiArtifact->GetVerticalShearArtifact();
   ATLASSERT(pArtifact  != nullptr);
   write_shear_capacity(pTable,row++,_T("Left Critical Section"), pArtifact, pDisplayUnits );

   if ( h_left.GetID() != INVALID_ID )
   {
      pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(lastIntervalIdx,pgsTypes::StrengthI,h_left.GetID());
      pArtifact = pPoiArtifact->GetVerticalShearArtifact();
      ATLASSERT(pArtifact!=0);
      write_shear_capacity(pTable,row++,_T("H from left end"), pArtifact, pDisplayUnits );
   }
   
   if ( left_15h.GetID() != INVALID_ID )
   {
      pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(lastIntervalIdx,pgsTypes::StrengthI,left_15h.GetID());
      pArtifact = pPoiArtifact->GetVerticalShearArtifact();
      ATLASSERT(pArtifact!=0);
      write_shear_capacity(pTable,row++,_T("1.5H from left end"), pArtifact, pDisplayUnits );
  }

   if ( 0 < NhMax )
   {
      if ( 1 < hp_count )
      {
         pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(lastIntervalIdx,pgsTypes::StrengthI,hp_left.GetID());
         pArtifact = pPoiArtifact->GetVerticalShearArtifact();
         ATLASSERT(pArtifact!=0);
         write_shear_capacity(pTable,row++,_T("Left Harping Point"), pArtifact, pDisplayUnits );

         pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(lastIntervalIdx,pgsTypes::StrengthI,hp_right.GetID());
         pArtifact = pPoiArtifact->GetVerticalShearArtifact();
         ATLASSERT(pArtifact!=0);
         write_shear_capacity(pTable,row++,_T("Right Harping Point"), pArtifact, pDisplayUnits );
      }
      else
      {
         pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(lastIntervalIdx,pgsTypes::StrengthI,hp_left.GetID());
         pArtifact = pPoiArtifact->GetVerticalShearArtifact();
         ATLASSERT(pArtifact!=0);
         write_shear_capacity(pTable,row++,_T("Harping Point"), pArtifact, pDisplayUnits );
      }
   }

   if ( right_15h.GetID() != INVALID_ID )
   {
      pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(lastIntervalIdx,pgsTypes::StrengthI,right_15h.GetID());
      pArtifact = pPoiArtifact->GetVerticalShearArtifact();
      ATLASSERT(pArtifact!=0);
      write_shear_capacity(pTable,row++,_T("1.5H from right end"), pArtifact, pDisplayUnits );
   }

   if ( h_right.GetID() != INVALID_ID )
   {
      pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(lastIntervalIdx,pgsTypes::StrengthI,h_right.GetID());
      pArtifact = pPoiArtifact->GetVerticalShearArtifact();
      ATLASSERT(pArtifact!=0);
      write_shear_capacity(pTable,row++,_T("H from right end"), pArtifact, pDisplayUnits );
   }

   pPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI(lastIntervalIdx,pgsTypes::StrengthI,right_cs.GetID());
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
   const stbLiftingCheckArtifact* pLiftArtifact = pSegmentArtifact->GetLiftingCheckArtifact();

   if (pLiftArtifact == nullptr)
   {
      *p<<_T("Lifting check not performed because it is not enabled in the library")<<rptNewLine;
      return;
   }

   // unstable girders are a problem
   const stbLiftingResults& liftingResults = pLiftArtifact->GetLiftingResults();
   if ( !liftingResults.bIsStable[stbTypes::NoImpact] || !liftingResults.bIsStable[stbTypes::ImpactUp] || !liftingResults.bIsStable[stbTypes::ImpactUp] )
   {
      rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      *pChapter << pTitle;
      *pTitle<<color(Red)<<_T("Lifting Check Failed - Girder is unstable - CG is higher than pick points")<< color(Black)<<rptNewLine;
   }
   else
   {
      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(4,_T("Lifting in the Casting Yard"));
      *p << pTable << rptNewLine;

      // Setup the table
      pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );
      pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
      pTable->SetColumnStyle(2, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
      pTable->SetColumnStyle(3, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );

      pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
      pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
      pTable->SetStripeRowColumnStyle(2, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
      pTable->SetStripeRowColumnStyle(3, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

      // Setup up some unit value prototypes
      INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());
      INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true );
      INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,   pDisplayUnits->GetAreaUnit(),   true );

      (*pTable)(0,0) << _T("");
      (*pTable)(0,1) << _T("Demand");
      (*pTable)(0,2) << _T("Limit");
      (*pTable)(0,3) << _T("Status");

      GET_IFACE2(pBroker,ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
      Float64 max_all_stress   = pSegmentLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStress(segmentKey);
      Float64 allow_with_rebar = pSegmentLiftingSpecCriteria->GetLiftingWithMildRebarAllowableStress(segmentKey);
      Float64 min_all_stress_global = pSegmentLiftingSpecCriteria->GetLiftingAllowableGlobalCompressiveConcreteStress(segmentKey);
      Float64 min_all_stress_peak = pSegmentLiftingSpecCriteria->GetLiftingAllowablePeakCompressiveConcreteStress(segmentKey);

      Float64 min_stress_global = liftingResults.MinDirectStress;
      Float64 min_stress_peak = liftingResults.MinStress;
      Float64 max_stress = liftingResults.MaxStress;

      RowIndexType row = pTable->GetNumberOfHeaderRows();

      (*pTable)(row,0) << _T("Tensile Stress (w/o mild rebar)");
      (*pTable)(row,1) <<  stress.SetValue(max_stress);
      (*pTable)(row,2) <<  stress.SetValue(max_all_stress);
      if (max_stress <= max_all_stress)
      {
         (*pTable)(row,3) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,3) << RPT_FAIL;
      }
      row++;

      (*pTable)(row,0) << _T("Tensile Stress (if bonded reinforcement sufficient to resist the tensile force in the concrete is provided)");

      (*pTable)(row,1) <<  stress.SetValue(max_stress);
      (*pTable)(row,2) <<  stress.SetValue(allow_with_rebar);
      if (max_stress <= allow_with_rebar)
      {
         (*pTable)(row,3) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,3) << RPT_FAIL;
      }
      row++;

      (*pTable)(row,0) << _T("Compressive Stress - General");
      (*pTable)(row,1) <<  stress.SetValue(min_stress_global);
      (*pTable)(row,2) <<  stress.SetValue(min_all_stress_global);
      if (min_all_stress_global <= min_stress_global)
      {
         (*pTable)(row,3) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,3) << RPT_FAIL;
      }
      row++;

      (*pTable)(row, 0) << _T("Compressive Stress - With lateral bending");
      (*pTable)(row, 1) << stress.SetValue(min_stress_peak);
      (*pTable)(row, 2) << stress.SetValue(min_all_stress_peak);
      if (min_all_stress_peak <= min_stress_peak)
      {
         (*pTable)(row, 3) << RPT_PASS;
      }
      else
      {
         (*pTable)(row, 3) << RPT_FAIL;
      }
      row++;


      Float64 fs_crack = liftingResults.FScrMin;
      Float64 all_fs_crack = pSegmentLiftingSpecCriteria->GetLiftingCrackingFs();
      (*pTable)(row,0) << _T("F.S. - Cracking");
      (*pTable)(row,1) <<  scalar.SetValue(fs_crack);
      (*pTable)(row,2) <<  scalar.SetValue(all_fs_crack);
      if (all_fs_crack <= fs_crack)
      {
         (*pTable)(row,3) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,3) << RPT_FAIL;
      }
      row++;

      Float64 fs_fail = liftingResults.MinAdjFsFailure;
      Float64 all_fs_fail = pSegmentLiftingSpecCriteria->GetLiftingFailureFs();
      (*pTable)(row,0) << _T("F.S. - Failure");
      (*pTable)(row,1) <<  scalar.SetValue(fs_fail);
      (*pTable)(row,2) <<  scalar.SetValue(all_fs_fail);
      if (pLiftArtifact->PassedFailureCheck())
      {
         (*pTable)(row,3) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,3) << RPT_FAIL;
      }
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

   if (pHaulArtifact == nullptr)
   {
      *p << _T("Hauling check not performed because it is not enabled in the Project Criteria") << rptNewLine;
      return;
   }

   GET_IFACE2(pBroker,ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
   const stbHaulingCheckArtifact& haulingArtifact = pHaulArtifact->GetHaulingCheckArtifact();
   const stbHaulingResults& haulingResults = haulingArtifact.GetHaulingResults();
   for ( int s = 0; s < 2; s++ )
   {
      pgsTypes::HaulingSlope slope = (pgsTypes::HaulingSlope)s;

      rptRcTable* pTable;
      if ( slope == pgsTypes::CrownSlope )
      {
         pTable = rptStyleManager::CreateDefaultTable(4,_T("Hauling to the Bridge Site - Normal Crown Slope"));
      }
      else
      {
         pTable = rptStyleManager::CreateDefaultTable(4,_T("Hauling to the Bridge Site - Maximum Superelevation"));
      }

      *p << pTable << rptNewLine;

      // Setup the table
      pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );
      pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT) );
      pTable->SetColumnStyle(2, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT) );
      pTable->SetColumnStyle(3, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );

      pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
      pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT) );
      pTable->SetStripeRowColumnStyle(2, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT) );
      pTable->SetStripeRowColumnStyle(3, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

      // Setup up some unit value prototypes
      INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());
      INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true );
      INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetShearUnit(),  true );
      INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,   pDisplayUnits->GetAreaUnit(),   true );

      (*pTable)(0,0) << _T("");
      (*pTable)(0,1) << _T("Demand");
      (*pTable)(0,2) << _T("Limit");
      (*pTable)(0,3) << _T("Status");

      Float64 min_stress_global = haulingResults.MinDirectStress[slope];
      Float64 min_stress_peak = haulingResults.MinStress[slope];
      Float64 max_stress = haulingResults.MaxStress[slope];

      Float64 max_all_stress, allow_with_rebar;
      if ( slope == pgsTypes::CrownSlope )
      {
         max_all_stress   = pSegmentHaulingSpecCriteria->GetHaulingAllowableTensileConcreteStressNormalCrown(segmentKey);
         allow_with_rebar = pSegmentHaulingSpecCriteria->GetHaulingWithMildRebarAllowableStressNormalCrown(segmentKey);
      }
      else
      {
         max_all_stress   = pSegmentHaulingSpecCriteria->GetHaulingAllowableTensileConcreteStressMaxSuper(segmentKey);
         allow_with_rebar = pSegmentHaulingSpecCriteria->GetHaulingWithMildRebarAllowableStressMaxSuper(segmentKey);
      }
      Float64 min_all_stress_global = pSegmentHaulingSpecCriteria->GetHaulingAllowableGlobalCompressiveConcreteStress(segmentKey);
      Float64 min_all_stress_peak = pSegmentHaulingSpecCriteria->GetHaulingAllowablePeakCompressiveConcreteStress(segmentKey);

      RowIndexType row = 1;
      (*pTable)(row,0) << _T("Tensile Stress (w/o mild rebar)");
      (*pTable)(row,1) <<  stress.SetValue(max_stress);
      (*pTable)(row,2) <<  stress.SetValue(max_all_stress);
      if (max_stress <= max_all_stress)
      {
         (*pTable)(row,3) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,3) << RPT_FAIL;
      }
      row++;

      (*pTable)(row,0) << _T("Tensile Stress (if bonded reinforcement sufficient to resist the tensile force in the concrete is provided)");

      (*pTable)(row,1) <<  stress.SetValue(max_stress);
      (*pTable)(row,2) <<  stress.SetValue(allow_with_rebar);
      if (max_stress <= allow_with_rebar)
      {
         (*pTable)(row,3) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,3) << RPT_FAIL;
      }
      row++;

      (*pTable)(row,0) << _T("Compressive Stress - General");
      (*pTable)(row,1) <<  stress.SetValue(min_stress_global);
      (*pTable)(row,2) <<  stress.SetValue(min_all_stress_global);
      if (min_all_stress_global <= min_stress_global)
      {
         (*pTable)(row,3) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,3) << RPT_FAIL;
      }
      row++;


      (*pTable)(row, 0) << _T("Compressive Stress - With lateral bending");
      (*pTable)(row, 1) << stress.SetValue(min_stress_peak);
      (*pTable)(row, 2) << stress.SetValue(min_all_stress_peak);
      if (min_all_stress_peak <= min_stress_peak)
      {
         (*pTable)(row, 3) << RPT_PASS;
      }
      else
      {
         (*pTable)(row, 3) << RPT_FAIL;
      }
      row++;

      Float64 fs_crack = pHaulArtifact->GetMinFsForCracking(slope);
      Float64 all_fs_crack = pSegmentHaulingSpecCriteria->GetHaulingCrackingFs();
      (*pTable)(row,0) << _T("F.S. - Cracking");
      (*pTable)(row,1) <<  scalar.SetValue(fs_crack);
      (*pTable)(row,2) <<  scalar.SetValue(all_fs_crack);
      if (all_fs_crack <= fs_crack)
      {
         (*pTable)(row,3) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,3) << RPT_FAIL;
      }
      row++;

      Float64 fs_roll = pHaulArtifact->GetFsRollover(slope);
      Float64 all_fs_fail = pSegmentHaulingSpecCriteria->GetHaulingRolloverFs();
      (*pTable)(row,0) << _T("F.S. - Rollover");
      (*pTable)(row,1) <<  scalar.SetValue(fs_roll);
      (*pTable)(row,2) <<  scalar.SetValue(all_fs_fail);
      if (all_fs_fail <= fs_roll)
      {
         (*pTable)(row,3) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,3) << RPT_FAIL;
      }
      row++;
   }
}
