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
#include <Reporting\DevLengthDetailsChapterBuilder.h>

#include <IFace\DisplayUnits.h>
#include <IFace\PrestressForce.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDevLengthDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CDevLengthDetailsChapterBuilder::CDevLengthDetailsChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CDevLengthDetailsChapterBuilder::GetName() const
{
   return TEXT("Transfer and Development Length Details");
}

rptChapter* CDevLengthDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IDisplayUnits,pDispUnits);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length,  pDispUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,  pDispUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDispUnits->GetSpanLengthUnit(), false );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   GET_IFACE2(pBroker,IPrestressForce,pPSForce);

   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(pgsTypes::BridgeSite3,span,girder,POI_ALL,POIFIND_OR);
   ATLASSERT( vPOI.size() != 0 );
   pgsPointOfInterest dummy_poi = vPOI[0];

   STRANDDEVLENGTHDETAILS bonded_details   = pPSForce->GetDevLengthDetails(dummy_poi,false); // not debonded
   STRANDDEVLENGTHDETAILS debonded_details = pPSForce->GetDevLengthDetails(dummy_poi,true);  // debonded

   // Transfer Length
   GET_IFACE2(pBroker,ISpecification, pSpec );
   std::string spec_name = pSpec->GetSpecification();
   GET_IFACE2(pBroker,ILibrary, pLib );
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   rptParagraph* pParagraph_h = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph_h;
   rptParagraph* pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if (pSpecEntry->GetPrestressTransferComputationType()!=pgsTypes::ptMinuteValue)
   {
      *pParagraph_h << "Transfer Length [5.11.4.1]" << rptNewLine;
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "TransferLength.gif") << rptNewLine;
      *pParagraph << Sub2("d","b") << " = " << length.SetValue(bonded_details.db) << rptNewLine;
      *pParagraph << Sub2("l","t") << " = " << length.SetValue(bonded_details.lt) << rptNewLine;
   }
   else
   {
      *pParagraph_h << "Zero Transfer Length Selected in Project Criteria" << rptNewLine;
      *pParagraph << "Actual length used "<< Sub2("l","t") << " = " << length.SetValue(bonded_details.lt) << rptNewLine;
   }

   // Development Length
   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(13,"Development Length [5.11.4.2]");
   (*pParagraph) << pTable << rptNewLine;

   if ( IS_US_UNITS(pDispUnits) )
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "DevLength_US.gif") << rptNewLine;
   else
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "DevLength_SI.gif") << rptNewLine;


   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "DevLengthReduction.png") << rptNewLine;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetRowSpan(0,0,2);
   pTable->SetRowSpan(1,0,-1);
   (*pTable)(0,0) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDispUnits->GetSpanLengthUnit());

   pTable->SetColumnSpan(0,1, 6);
   pTable->SetColumnSpan(0,2,-1);
   pTable->SetColumnSpan(0,3,-1);
   pTable->SetColumnSpan(0,4,-1);
   pTable->SetColumnSpan(0,5,-1);
   pTable->SetColumnSpan(0,6,-1);
   (*pTable)(0,1) << "Bonded Strands "   << symbol(kappa) << " = " << bonded_details.k;

   pTable->SetColumnSpan(0,7, 6);
   pTable->SetColumnSpan(0,8,-1);
   pTable->SetColumnSpan(0,9,-1);
   pTable->SetColumnSpan(0,10,-1);
   pTable->SetColumnSpan(0,11,-1);
   pTable->SetColumnSpan(0,12,-1);
   (*pTable)(0,7) << "Debonded Strands " << symbol(kappa) << " = " << debonded_details.k;

   (*pTable)(1,1) << COLHDR(Sub2("f","ps"), rptStressUnitTag, pDispUnits->GetStressUnit() );
   (*pTable)(1,2) << COLHDR(Sub2("f","pe"), rptStressUnitTag, pDispUnits->GetStressUnit() );
   (*pTable)(1,3) << COLHDR(Sub2("d","b"),  rptLengthUnitTag, pDispUnits->GetComponentDimUnit() );
   (*pTable)(1,4) << COLHDR(Sub2("l","d"),  rptLengthUnitTag, pDispUnits->GetComponentDimUnit() );
   (*pTable)(1,5) << COLHDR(Sub2("l","px"),  rptLengthUnitTag, pDispUnits->GetComponentDimUnit() );
   (*pTable)(1,6) << Sub2("f","px") << "/" << Sub2("f","ps");

   (*pTable)(1,7) << COLHDR(Sub2("f","ps"), rptStressUnitTag, pDispUnits->GetStressUnit() );
   (*pTable)(1,8) << COLHDR(Sub2("f","pe"), rptStressUnitTag, pDispUnits->GetStressUnit() );
   (*pTable)(1,9) << COLHDR(Sub2("d","b"),  rptLengthUnitTag, pDispUnits->GetComponentDimUnit() );
   (*pTable)(1,10)<< COLHDR(Sub2("l","d"),  rptLengthUnitTag, pDispUnits->GetComponentDimUnit() );
   (*pTable)(1,11)<< COLHDR(Sub2("l","px"),  rptLengthUnitTag, pDispUnits->GetComponentDimUnit() );
   (*pTable)(1,12)<< Sub2("f","px") << "/" << Sub2("f","ps");

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   Float64 girder_length = pBridge->GetGirderLength(span,girder);
   Float64 half_girder_length = girder_length/2;

   stress.ShowUnitTag(false);
   length.ShowUnitTag(false);

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   for ( std::vector<pgsPointOfInterest>::iterator iter = vPOI.begin(); iter != vPOI.end(); iter++ )
   {
      pgsPointOfInterest poi = *iter;
      STRANDDEVLENGTHDETAILS bonded_details   = pPSForce->GetDevLengthDetails(poi,false); // not debonded
      STRANDDEVLENGTHDETAILS debonded_details = pPSForce->GetDevLengthDetails(poi,true); // debonded

      Float64 lpx;
      if ( poi.GetDistFromStart() < half_girder_length )
         lpx = poi.GetDistFromStart();
      else
         lpx = girder_length - poi.GetDistFromStart();

      Float64 bond_factor;
      if ( lpx < bonded_details.lt )
         bond_factor = bonded_details.fpe*lpx/(bonded_details.fps*bonded_details.lt);
      else if ( lpx < bonded_details.ld )
         bond_factor = (bonded_details.fpe + ((lpx - bonded_details.lt)/(bonded_details.ld - bonded_details.lt))*(bonded_details.fps - bonded_details.fpe))/bonded_details.fps;
      else
         bond_factor = 1.0;

      bond_factor = ForceIntoRange(0.0,bond_factor,1.0);

      (*pTable)(row,0) << location.SetValue( poi, end_size );
      (*pTable)(row,1) << stress.SetValue(bonded_details.fps);
      (*pTable)(row,2) << stress.SetValue(bonded_details.fpe);
      (*pTable)(row,3) << length.SetValue(bonded_details.db);
      (*pTable)(row,4) << length.SetValue(bonded_details.ld);
      (*pTable)(row,5) << length.SetValue(lpx);
      (*pTable)(row,6) << scalar.SetValue(bond_factor);

      if ( lpx < debonded_details.lt )
         bond_factor = debonded_details.fpe*lpx/(debonded_details.fps*debonded_details.lt);
      else if ( lpx < debonded_details.ld )
         bond_factor = (debonded_details.fpe + ((lpx - debonded_details.lt)/(debonded_details.ld - debonded_details.lt))*(debonded_details.fps - debonded_details.fpe))/debonded_details.fps;
      else
         bond_factor = 1.0;

      bond_factor = ForceIntoRange(0.0,bond_factor,1.0);

      (*pTable)(row,7) << stress.SetValue(debonded_details.fps);
      (*pTable)(row,8) << stress.SetValue(debonded_details.fpe);
      (*pTable)(row,9) << length.SetValue(debonded_details.db);
      (*pTable)(row,10) << length.SetValue(debonded_details.ld);
      (*pTable)(row,11) << length.SetValue(lpx);
      (*pTable)(row,12) << scalar.SetValue(bond_factor);

      row++;
   }

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   (*pChapter) << pParagraph;
   (*pParagraph) << Sub2("f","px") << "/" << Sub2("f","ps") << " = Development Length Reduction Factor (See LRFD Eqn. 5.11.4.2-2 and -3)" << rptNewLine;

   return pChapter;
}

CChapterBuilder* CDevLengthDetailsChapterBuilder::Clone() const
{
   return new CDevLengthDetailsChapterBuilder;
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

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
