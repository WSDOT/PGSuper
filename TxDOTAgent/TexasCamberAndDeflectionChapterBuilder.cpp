///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
#include <Reporting\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\ConstructabilityCheckTable.h>

#include "TexasCamberAndDeflectionChapterBuilder.h"
#include "TexasIBNSParagraphBuilder.h"

#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\PierData.h>

#include <psgLib\ConnectionLibraryEntry.h>

#include <WBFLCogo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CTexasCamberAndDeflectionChapterBuilder
****************************************************************************/


static void deflection_and_camber(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTexasCamberAndDeflectionChapterBuilder::CTexasCamberAndDeflectionChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTexasCamberAndDeflectionChapterBuilder::GetName() const
{
   return TEXT("Camber and Deflections");
}

rptChapter* CTexasCamberAndDeflectionChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);
   SpanIndexType span = pSpec->GetSpan();
   GirderIndexType girder = pSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   deflection_and_camber( pChapter, pBroker, span, girder, pDisplayUnits );

   // Constructability check
   GET_IFACE2(pBroker,IBridge,pBridge);

   rptRcTable* hdtable = CConstructabilityCheckTable().BuildSlabOffsetTable(pBroker,span,girder,pDisplayUnits);
   if (hdtable!=NULL)
   {
      rptParagraph* p = new rptParagraph;
      *p << hdtable << rptNewLine;
      *pChapter << p;
   }

   return pChapter;
}

CChapterBuilder* CTexasCamberAndDeflectionChapterBuilder::Clone() const
{
   return new CTexasCamberAndDeflectionChapterBuilder;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////
void deflection_and_camber(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(3,_T("Camber and Deflection"));
   *p << pTable << rptNewLine;

   // Right justify columns with numbers
   pTable->SetColumnStyle( 1, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT) );
   pTable->SetColumnStyle( 2, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT) );
   pTable->SetStripeRowColumnStyle( 1, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT) );
   pTable->SetStripeRowColumnStyle( 2, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT) );

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue, disp,   pDisplayUnits->GetDisplacementUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dispft, pDisplayUnits->GetSpanLengthUnit(),   true );

   // Get the interfaces we need
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IProductForces, pProductForces);
   GET_IFACE2(pBroker,IProductLoads, pProductLoads);
   GET_IFACE2( pBroker, ILibrary, pLib );
   GET_IFACE2( pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,girder);

   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   bool do_defl = pSpecEntry->GetDoEvaluateLLDeflection();
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // Get Midspan std::vector<pgsPointOfInterest>
   std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(span,girder,pgsTypes::BridgeSite3,POI_MIDSPAN);
   CHECK(vPoi.size()==1);
   pgsPointOfInterest poi = *vPoi.begin();

   // Compute mid span deflections
   Float64 delta_gdr;  // due to girder self weight
   Float64 delta_dl;   // due to dead loads on girder
   Float64 delta_sk;   // due to shear key
   Float64 delta_ol;   // due to overlay
   Float64 delta_tb;   // due to traffic barrier
   Float64 delta_sw;   // due to sidewalk
   Float64 delta_ll;   // due to live load
   Float64 delta_oll;  // due to optional live load
   Float64 temp;

   delta_gdr = pProductForces->GetGirderDeflectionForCamber( poi );

   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : analysisType == pgsTypes::Continuous ? ContinuousSpan : MinSimpleContinuousEnvelope);

   delta_dl = pProductForces->GetDisplacement(pgsTypes::BridgeSite1, pftSlab, poi, bat )
            + pProductForces->GetDisplacement(pgsTypes::BridgeSite1, pftSlabPad, poi, bat )
            + pProductForces->GetDisplacement(pgsTypes::BridgeSite1, pftDiaphragm, poi, bat );

   delta_sk = pProductForces->GetDisplacement(pgsTypes::BridgeSite1, pftShearKey, poi, bat );

   pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;
   
   delta_ol = pProductForces->GetDisplacement(overlay_stage, pftOverlay, poi, bat );

   delta_tb = pProductForces->GetDisplacement(pgsTypes::BridgeSite2, pftTrafficBarrier, poi, bat );
   delta_sw = pProductForces->GetDisplacement(pgsTypes::BridgeSite2, pftTrafficBarrier, poi, bat );

   Float64 delta_dcu = pProductForces->GetDisplacement(pgsTypes::BridgeSite1,pftUserDC, poi, bat);
   delta_dcu        += pProductForces->GetDisplacement(pgsTypes::BridgeSite2,pftUserDC, poi, bat);

   Float64 delta_dwu = pProductForces->GetDisplacement(pgsTypes::BridgeSite1,pftUserDW, poi, bat);
   delta_dwu        += pProductForces->GetDisplacement(pgsTypes::BridgeSite2,pftUserDW, poi, bat);

   pProductForces->GetLiveLoadDisplacement(pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, bat, true, false, &delta_ll, &temp );

   pProductForces->GetDeflLiveLoadDisplacement(IProductForces::DeflectionLiveLoadEnvelope, poi, &delta_oll, &temp );

   // get # of days for creep
   Float64 min_days = ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Min(), unitMeasure::Day);
   Float64 max_days = ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day);

   // Populate the table
   bool is_negative_camber = false;
   Uint16 row = 0;
   (*pTable)(row,0) << _T("Estimated camber at ")<< min_days<<_T(" days, D");
   double D = pCamber->GetDCamberForGirderSchedule( poi,CREEP_MINTIME);
   if ( D < 0 )
   {
      (*pTable)(row,1) << color(Red) << disp.SetValue( D ) << color(Black);
      (*pTable)(row,2) << color(Red) << dispft.SetValue( D ) << color(Black);
   }
   else
   {
      (*pTable)(row,1) << disp.SetValue( D );
      (*pTable)(row,2) << dispft.SetValue( D );
   }
   row++;

   (*pTable)(row,0) << _T("Estimated camber at ")<< max_days<<_T(" days, D");
   D = pCamber->GetDCamberForGirderSchedule( poi,CREEP_MAXTIME);
   if ( D < 0 )
   {
      (*pTable)(row,1) << color(Red) << disp.SetValue( D ) << color(Black);
      (*pTable)(row,2) << color(Red) << dispft.SetValue( D ) << color(Black);
   }
   else
   {
      (*pTable)(row,1) << disp.SetValue( D );
      (*pTable)(row,2) << dispft.SetValue( D );
   }
   row++;

   if ( 0 < girderData.Nstrands[pgsTypes::Temporary] && girderData.TempStrandUsage != pgsTypes::ttsPTBeforeShipping )
   {
      (*pTable)(row,0) << _T("Deflection (Prestressing including temp strands)");
      (*pTable)(row,1) << disp.SetValue( pCamber->GetPrestressDeflection(poi,true) );
      (*pTable)(row,2) << dispft.SetValue( pCamber->GetPrestressDeflection(poi,true) );
   }
   else
   {
      (*pTable)(row,0) << _T("Deflection (Prestressing)");
      (*pTable)(row,1) << disp.SetValue( pCamber->GetPrestressDeflection(poi,false) );
      (*pTable)(row,2) << dispft.SetValue( pCamber->GetPrestressDeflection(poi,false) );
   }
   row++;

   if ( 0 < girderData.Nstrands[pgsTypes::Temporary] && girderData.TempStrandUsage != pgsTypes::ttsPTBeforeShipping )
   {
      (*pTable)(row,0) << _T("Deflection (Temporary Strand Removal)");
      (*pTable)(row,1) << disp.SetValue( pCamber->GetReleaseTempPrestressDeflection(poi) );
      (*pTable)(row,2) << dispft.SetValue( pCamber->GetReleaseTempPrestressDeflection(poi) );
      row++;
   }

   (*pTable)(row,0) << _T("Deflection (Girder)");
   (*pTable)(row,1) << disp.SetValue( delta_gdr );
   (*pTable)(row,2) << dispft.SetValue( delta_gdr );
   row++;

   (*pTable)(row,0) << _T("Deflection (Slab and Diaphragms)");
   (*pTable)(row,1) << disp.SetValue( delta_dl );
   (*pTable)(row,2) << dispft.SetValue( delta_dl );
   row++;

   if ( pProductLoads->HasShearKeyLoad(span,girder) )
   {
      (*pTable)(row,0) << _T("Deflection (Shear Key)");
      (*pTable)(row,1) << disp.SetValue( delta_sk );
      (*pTable)(row,2) << dispft.SetValue( delta_sk );
      row++;
   }

   if ( pProductLoads->HasSidewalkLoad(span,girder) )
   {
      (*pTable)(row,0) << _T("Deflection (Sidewalk)");
      (*pTable)(row,1) << disp.SetValue( delta_sw );
      (*pTable)(row,2) << dispft.SetValue( delta_sw );
      row++;
   }

   (*pTable)(row,0) << _T("Deflection (Traffic Barrier)");
   (*pTable)(row,1) << disp.SetValue( delta_tb );
   (*pTable)(row,2) << dispft.SetValue( delta_tb );
   row++;

   (*pTable)(row,0) << _T("Deflection (Overlay)");
   (*pTable)(row,1) << disp.SetValue( delta_ol );
   (*pTable)(row,2) << dispft.SetValue( delta_ol );
   row++;

   (*pTable)(row,0) << _T("Deflection (User Defined DC)");
   (*pTable)(row,1) << disp.SetValue( delta_dcu );
   (*pTable)(row,2) << dispft.SetValue( delta_dcu );
   row++;

   (*pTable)(row,0) << _T("Deflection (User Defined DW)");
   (*pTable)(row,1) << disp.SetValue( delta_dwu );
   (*pTable)(row,2) << dispft.SetValue( delta_dwu );
   row++;

   (*pTable)(row,0) << _T("Screed Camber, C");
   (*pTable)(row,1) << disp.SetValue( pCamber->GetScreedCamber(poi) );
   (*pTable)(row,2) << dispft.SetValue( pCamber->GetScreedCamber(poi) );
   row++;

   (*pTable)(row,0) << _T("Excess Camber") << rptNewLine << _T("(based on D at ") << max_days << _T(" days)");
   double excess_camber = pCamber->GetExcessCamber(poi,CREEP_MAXTIME);
   if ( excess_camber < 0 )
   {
      (*pTable)(row,1) << color(Red) << disp.SetValue( excess_camber ) << color(Black);
      (*pTable)(row,2) << color(Red) << dispft.SetValue( excess_camber ) << color(Black);
      is_negative_camber = true;
   }
   else
   {
      (*pTable)(row,1) << disp.SetValue( excess_camber );
      (*pTable)(row,2) << dispft.SetValue( excess_camber );
   }
   row++;

   (*pTable)(row,0) << _T("Live Load Deflection (HL93 - Per Lane)");
   (*pTable)(row,1) << disp.SetValue( delta_ll );
   (*pTable)(row,2) << dispft.SetValue( delta_ll );
   row++;

   if (do_defl)
   {
      (*pTable)(row,0) << _T("Optional Live Load Deflection (LRFD 3.6.1.3.2)");
      (*pTable)(row,1) << disp.SetValue( delta_oll );
      (*pTable)(row,2) << dispft.SetValue( delta_oll );
      row++;
   }

   if (is_negative_camber)
   {
      *p<<color(Red) << _T("Warning:  Excess camber is negative indicating a potential sag in the beam.") << color(Black) << rptNewLine;
   }
}



//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
