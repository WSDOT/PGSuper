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
#include <Reporting\CamberTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderData.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CCamberTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCamberTable::CCamberTable()
{
}

CCamberTable::CCamberTable(const CCamberTable& rOther)
{
   MakeCopy(rOther);
}

CCamberTable::~CCamberTable()
{
}

//======================== OPERATORS  =======================================
CCamberTable& CCamberTable::operator= (const CCamberTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CCamberTable::MakeCopy(const CCamberTable& rOther)
{
   // Add copy code here...
}

void CCamberTable::MakeAssignment(const CCamberTable& rOther)
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
bool CCamberTable::AssertValid() const
{
   return true;
}

void CCamberTable::Dump(dbgDumpContext& os) const
{
   os << "Dump for CCamberTable" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CCamberTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CCamberTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CCamberTable");

   TESTME_EPILOG("CCamberTable");
}
#endif // _UNITTEST


void CCamberTable::Build_CIP_TempStrands(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                         IEAFDisplayUnits* pDisplayUnits,Int16 constructionRate,
                                         rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(span == ALL_SPANS);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, displacement, pDisplayUnits->GetDisplacementUnit(), false );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::set<pgsTypes::Stage> stages;
   stages.insert(pgsTypes::CastingYard);
   stages.insert(pgsTypes::BridgeSite3);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( stages, span, girder, POI_DISPLACEMENT | POI_TABULAR );

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IProductLoads,pProduct);
   GET_IFACE2(pBroker,IProductForces,pProductForces);
   GET_IFACE2(pBroker,IBridge,pBridge);

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);

   bool bSidewalk = pProduct->HasSidewalkLoad(span,girder);
   bool bShearKey = pProduct->HasShearKeyLoad(span,girder);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // create the tables
   rptRcTable* table1;
   rptRcTable* table2;
   rptRcTable* table3;

   table1 = pgsReportStyleHolder::CreateDefaultTable(7,"Camber - Part 1 (upwards is positive)");
   table2 = pgsReportStyleHolder::CreateDefaultTable(7 + (bSidewalk ? 1 : 0) + (!pBridge->IsFutureOverlay() ? 1 : 0) + (bShearKey ? 1 : 0),"Camber - Part 2 (upwards is positive)");
   table3 = pgsReportStyleHolder::CreateDefaultTable(8,"Camber - Part 3 (upwards is positive)");

   if ( span == ALL_SPANS )
   {
      table1->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   int col = 0;
   (*table1)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"ps") << " " << Sub2("L","g"),         rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"ps") << " " << Sub2("L","s"),         rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"tpsi"),       rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"tpsr"),       rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"girder"),     rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"creep1"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   col = 0;
   (*table2)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"diaphragm"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( bShearKey )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"shear key"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"creep2"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"deck"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"User1") << " = " << rptNewLine << Sub2(symbol(DELTA),"UserDC") << " + " << rptNewLine << Sub2(symbol(DELTA),"UserDW") , rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( bSidewalk )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"sidewalk"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"barrier"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( !pBridge->IsFutureOverlay() )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"overlay"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"User2") << " = " << rptNewLine << Sub2(symbol(DELTA),"UserDC") << " + " << rptNewLine << Sub2(symbol(DELTA),"UserDW") , rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   col = 0;
   (*table3)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"1"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"2"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"3"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   double days = (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max());
   days = ::ConvertFromSysUnits(days,unitMeasure::Day);
   std::ostringstream os;
   os << days;
   (*table3)(0,col++) << COLHDR(Sub2("D",os.str().c_str()) << " = " << Sub2(symbol(DELTA),"4"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"5"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"excess") << " = " << rptNewLine << Sub2(symbol(DELTA),"6"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR("C = " << Sub2(symbol(DELTA),"4") << " - " << Sub2(symbol(DELTA),"6"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : analysisType == pgsTypes::Continuous ? ContinuousSpan : MinSimpleContinuousEnvelope);

   // Fill up the tables
   RowIndexType row1 = table1->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      double Dps1, Dps, Dtpsi, Dtpsr, Dgirder, Dcreep1, Ddiaphragm, Dshearkey, Ddeck, Dcreep2, Duser1, Dbarrier, Dsidewalk, Doverlay, Duser2;
      Dps1       = pCamber->GetPrestressDeflection( poi, false );
      Dps        = pCamber->GetPrestressDeflection( poi, true );
      Dtpsi      = pCamber->GetInitialTempPrestressDeflection( poi,true );
      Dtpsr      = pCamber->GetReleaseTempPrestressDeflection( poi );
      Dgirder    = pProductForces->GetGirderDeflectionForCamber( poi );
      Dcreep1    = pCamber->GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate );
      Ddiaphragm = pCamber->GetDiaphragmDeflection( poi );
      Dshearkey  = pProductForces->GetDisplacement(pgsTypes::BridgeSite1,pftShearKey,poi,bat);
      Ddeck      = pProductForces->GetDisplacement(pgsTypes::BridgeSite1,pftSlab,poi,bat);
      Dcreep2    = pCamber->GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate );
      Duser1     = pProductForces->GetDisplacement(pgsTypes::BridgeSite1,pftUserDC,poi,bat) + pProductForces->GetDisplacement(pgsTypes::BridgeSite1,pftUserDW,poi,bat);
      Duser2     = pProductForces->GetDisplacement(pgsTypes::BridgeSite2,pftUserDC,poi,bat) + pProductForces->GetDisplacement(pgsTypes::BridgeSite2,pftUserDW,poi,bat);
      Dbarrier   = pProductForces->GetDisplacement(pgsTypes::BridgeSite2,pftTrafficBarrier,poi,bat);
      Dsidewalk  = pProductForces->GetDisplacement(pgsTypes::BridgeSite2,pftSidewalk,      poi,bat);
      Doverlay   = pProductForces->GetDisplacement(pgsTypes::BridgeSite2,pftOverlay,poi,bat);

      // if we have a future overlay, the deflection due to the overlay in BridgeSite2 must be zero
      ATLASSERT( pBridge->IsFutureOverlay() ? IsZero(Doverlay) : true );

      // Table 1
      col = 0;
      (*table1)(row1,col++) << location.SetValue( poi,end_size );
      (*table1)(row1,col++) << displacement.SetValue( Dps1 );
      (*table1)(row1,col++) << displacement.SetValue( Dps );
      (*table1)(row1,col++) << displacement.SetValue( Dtpsi );
      (*table1)(row1,col++) << displacement.SetValue( Dtpsr );
      (*table1)(row1,col++) << displacement.SetValue( Dgirder );
      (*table1)(row1,col++) << displacement.SetValue( Dcreep1 );

      row1++;

      // Table 2
      col = 0;
      (*table2)(row2,col++) << location.SetValue( poi,end_size );
      (*table2)(row2,col++) << displacement.SetValue( Ddiaphragm );
      if ( bShearKey )
         (*table2)(row2,col++) << displacement.SetValue( Dshearkey );

      (*table2)(row2,col++) << displacement.SetValue( Dcreep2 );
      (*table2)(row2,col++) << displacement.SetValue( Ddeck );
      (*table2)(row2,col++) << displacement.SetValue( Duser1 );
      if ( bSidewalk )
         (*table2)(row2,col++) << displacement.SetValue( Dsidewalk );

      (*table2)(row2,col++) << displacement.SetValue( Dbarrier );

      if ( !pBridge->IsFutureOverlay() )
         (*table2)(row2,col++) << displacement.SetValue(Doverlay);

      (*table2)(row2,col++) << displacement.SetValue( Duser2 );

      row2++;

      // Table 3
      col = 0;
      double D1 = Dgirder + Dps + Dtpsi;
      double D2 = D1 + Dcreep1;
      double D3 = D2 + Ddiaphragm + Dshearkey + Dtpsr;
      double D4 = D3 + Dcreep2;
      double D5 = D4 + Ddeck + Duser1;
      double D6 = D5 + Dsidewalk + Dbarrier + Doverlay + Duser2;

      (*table3)(row3,col++) << location.SetValue( poi,end_size );
      (*table3)(row3,col++) << displacement.SetValue( D1 );
      (*table3)(row3,col++) << displacement.SetValue( D2 );
      (*table3)(row3,col++) << displacement.SetValue( D3 );

      D4 = IsZero(D4) ? 0 : D4;
      if ( D4 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << displacement.SetValue(D4) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << displacement.SetValue( D4 );
      }

      (*table3)(row3,col++) << displacement.SetValue( D5 );

      D6 = IsZero(D6) ? 0 : D6;
      if ( D6 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << displacement.SetValue(D6) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << displacement.SetValue( D6 );
      }
      (*table3)(row3,col++) << displacement.SetValue( D4 - D6 );

      row3++;
   }

   *pTable1 = table1;
   *pTable2 = table2;
   *pTable3 = table3;
}


void CCamberTable::Build_CIP(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                             IEAFDisplayUnits* pDisplayUnits,Int16 constructionRate,
                             rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(span == ALL_SPANS);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, displacement, pDisplayUnits->GetDisplacementUnit(), false );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::set<pgsTypes::Stage> stages;
   stages.insert(pgsTypes::CastingYard);
   stages.insert(pgsTypes::BridgeSite3);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( stages, span, girder, POI_DISPLACEMENT | POI_TABULAR );

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IProductForces,pProduct);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IBridge,pBridge);

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);

   bool bSidewalk = pProductLoads->HasSidewalkLoad(span,girder);
   bool bShearKey = pProductLoads->HasShearKeyLoad(span,girder);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // create the tables
   rptRcTable* table1;
   rptRcTable* table2;
   rptRcTable* table3;

   table1 = pgsReportStyleHolder::CreateDefaultTable(5,"Camber - Part 1 (upwards is positive)");
   table2 = pgsReportStyleHolder::CreateDefaultTable(6 + (bSidewalk ? 1 : 0) + (!pBridge->IsFutureOverlay() ? 1 : 0)+ (bShearKey ? 1 : 0),"Camber - Part 2 (upwards is positive)");
   table3 = pgsReportStyleHolder::CreateDefaultTable(6,"Camber - Part 3 (upwards is positive)");

   if ( span == ALL_SPANS )
   {
      table1->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   int col = 0;
   (*table1)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"ps") << " " << Sub2("L","g"),         rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"ps") << " " << Sub2("L","s"),         rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"girder"),     rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"creep"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   col = 0;
   (*table2)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"diaphragm"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( bShearKey )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"shear key"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"deck"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"User1") << " = " << rptNewLine << Sub2(symbol(DELTA),"UserDC") << " + " << rptNewLine  << Sub2(symbol(DELTA),"UserDW") , rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( bSidewalk )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"sidewalk"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"barrier"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( !pBridge->IsFutureOverlay() )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"overlay"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"User2") << " = " << rptNewLine << Sub2(symbol(DELTA),"UserDC") << " + " << rptNewLine  << Sub2(symbol(DELTA),"UserDW") , rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   col = 0;
   (*table3)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"1"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   double days = (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max());
   days = ::ConvertFromSysUnits(days,unitMeasure::Day);
   std::ostringstream os;
   (*table3)(0,col++) << COLHDR(Sub2("D",os.str().c_str()) << " = " << Sub2(symbol(DELTA),"2"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"3"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"excess") << " = " << rptNewLine << Sub2(symbol(DELTA),"4"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR("C = " << Sub2(symbol(DELTA),"2") << " - " << Sub2(symbol(DELTA),"4"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : analysisType == pgsTypes::Continuous ? ContinuousSpan : MinSimpleContinuousEnvelope);

   // Fill up the tables
   RowIndexType row1 = table1->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      double Dps1, Dps, Dgirder, Dcreep, Ddiaphragm, Dshearkey, Ddeck, Duser1, Dbarrier, Dsidewalk, Doverlay, Duser2;
      Dps1       = pCamber->GetPrestressDeflection( poi, false );
      Dps        = pCamber->GetPrestressDeflection( poi, true );
      Dgirder    = pProduct->GetGirderDeflectionForCamber( poi );
      Dcreep     = pCamber->GetCreepDeflection( poi, ICamber::cpReleaseToDeck, constructionRate );
      Ddiaphragm = pCamber->GetDiaphragmDeflection( poi );
      Dshearkey  = pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftShearKey,poi,bat);
      Ddeck      = pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftSlab,poi,bat);
      Duser1     = pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftUserDC,poi,bat) + pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftUserDW,poi,bat);
      Duser2     = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftUserDC,poi,bat) + pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftUserDW,poi,bat);
      Dbarrier   = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftTrafficBarrier,poi,bat);
      Dsidewalk  = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftSidewalk,      poi,bat);
      Doverlay   = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftOverlay,poi,bat);

      // Table 1
      col = 0;
      (*table1)(row1,col++) << location.SetValue( poi,end_size );
      (*table1)(row1,col++) << displacement.SetValue( Dps1 );
      (*table1)(row1,col++) << displacement.SetValue( Dps );
      (*table1)(row1,col++) << displacement.SetValue( Dgirder );
      (*table1)(row1,col++) << displacement.SetValue( Dcreep );
      row1++;

      // Table 2
      col = 0;
      (*table2)(row2,col++) << location.SetValue( poi,end_size );
      (*table2)(row2,col++) << displacement.SetValue( Ddiaphragm );

      if ( bShearKey )
         (*table2)(row2,col++) << displacement.SetValue( Dshearkey );

      (*table2)(row2,col++) << displacement.SetValue( Ddeck );
      (*table2)(row2,col++) << displacement.SetValue( Duser1 );

      if ( bSidewalk )
         (*table2)(row2,col++) << displacement.SetValue( Dsidewalk );

      (*table2)(row2,col++) << displacement.SetValue( Dbarrier);

      if ( !pBridge->IsFutureOverlay() )
         (*table2)(row2,col++) << displacement.SetValue(Doverlay);

      (*table2)(row2,col++) << displacement.SetValue( Duser2 );

      row2++;

      // Table 3
      col = 0;
      double D1 = Dgirder + Dps;
      double D2 = D1 + Dcreep;
      double D3 = D2 + Ddiaphragm + Ddeck + Dshearkey + Duser1;
      double D4 = D3 + Dsidewalk + Dbarrier + Duser2 + Doverlay;

      (*table3)(row3,col++) << location.SetValue( poi,end_size );
      (*table3)(row3,col++) << displacement.SetValue( D1 );

      D2 = IsZero(D2) ? 0 : D2;
      if ( D2 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << displacement.SetValue(D2) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << displacement.SetValue( D2 );
      }

      (*table3)(row3,col++) << displacement.SetValue( D3 );

      D4 = IsZero(D4) ? 0 : D4;
      if ( D4 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << displacement.SetValue(D4) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << displacement.SetValue( D4 );
      }

      (*table3)(row3,col++) << displacement.SetValue( D2 - D4 );

      row3++;
   }

   *pTable1 = table1;
   *pTable2 = table2;
   *pTable3 = table3;
}

void CCamberTable::Build_SIP_TempStrands(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                         IEAFDisplayUnits* pDisplayUnits,Int16 constructionRate,
                                         rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(span == ALL_SPANS);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, displacement, pDisplayUnits->GetDisplacementUnit(), false );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::set<pgsTypes::Stage> stages;
   stages.insert(pgsTypes::CastingYard);
   stages.insert(pgsTypes::BridgeSite3);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( stages, span, girder, POI_DISPLACEMENT | POI_TABULAR );

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IProductForces,pProduct);
   GET_IFACE2(pBroker,IBridge,pBridge);

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   bool bSidewalk = pProductLoads->HasSidewalkLoad(span,girder);
   bool bShearKey = pProductLoads->HasShearKeyLoad(span,girder);

   // create the tables
   rptRcTable* table1;
   rptRcTable* table2;
   rptRcTable* table3;

   table1 = pgsReportStyleHolder::CreateDefaultTable(7,"Camber - Part 1 (upwards is positive)");
   table2 = pgsReportStyleHolder::CreateDefaultTable(8 + (bSidewalk ? 1 : 0) + (!pBridge->IsFutureOverlay() ? 1 : 0) + (bShearKey ? 1 : 0),"Camber - Part 2 (upwards is positive)");
   table3 = pgsReportStyleHolder::CreateDefaultTable(8,"Camber - Part 3 (upwards is positive)");

   if ( span == ALL_SPANS )
   {
      table1->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   int col = 0;
   (*table1)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"ps") << " " << Sub2("L","g"),         rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"ps") << " " << Sub2("L","s"),         rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"tpsi"),       rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"tpsr"),       rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"girder"),     rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"creep1"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   col = 0;
   (*table2)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"diaphragm"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( bShearKey )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"shear key"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"creep2"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"panels"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"deck"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"User1") << " = " << rptNewLine << Sub2(symbol(DELTA),"UserDC") << " + " << rptNewLine  << Sub2(symbol(DELTA),"UserDW") , rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( bSidewalk )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"sidewalk"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"barrier"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( !pBridge->IsFutureOverlay() )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"overlay"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"User2") << " = " << rptNewLine << Sub2(symbol(DELTA),"UserDC") << " + " << rptNewLine  << Sub2(symbol(DELTA),"UserDW") , rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   col = 0;
   (*table3)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"1"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"2"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"3"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   double days = (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max());
   days = ::ConvertFromSysUnits(days,unitMeasure::Day);
   std::ostringstream os;
   os << days;
   (*table3)(0,col++) << COLHDR(Sub2("D",os.str().c_str()) << " = " << Sub2(symbol(DELTA),"4"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"5"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"excess") << " = " << rptNewLine << Sub2(symbol(DELTA),"6"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR("C = " << Sub2(symbol(DELTA),"4") << " - " << Sub2(symbol(DELTA),"6"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : analysisType == pgsTypes::Continuous ? ContinuousSpan : MinSimpleContinuousEnvelope);

   // Fill up the tables
   RowIndexType row1 = table1->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      double Dps1, Dps, Dtpsi, Dtpsr, Dgirder, Dcreep1, Ddiaphragm, Dshearkey, Dpanel, Ddeck, Dcreep2, Duser1, Dsidewalk, Dbarrier, Doverlay, Duser2;
      Dps1       = pCamber->GetPrestressDeflection( poi, false );
      Dps        = pCamber->GetPrestressDeflection( poi, true );
      Dtpsi      = pCamber->GetInitialTempPrestressDeflection( poi,true );
      Dtpsr      = pCamber->GetReleaseTempPrestressDeflection( poi );
      Dgirder    = pProduct->GetGirderDeflectionForCamber( poi );
      Dcreep1    = pCamber->GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate );
      Ddiaphragm = pCamber->GetDiaphragmDeflection( poi );
      Ddeck      = pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftSlab,poi,bat);
      Dshearkey  = pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftShearKey,poi,bat);
      Dcreep2    = pCamber->GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate );
      Duser1     = pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftUserDC,poi,bat) + pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftUserDW,poi,bat);
      Duser2     = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftUserDC,poi,bat) + pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftUserDW,poi,bat);
      Dbarrier   = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftTrafficBarrier,poi,bat);
      Dsidewalk  = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftSidewalk,      poi,bat);
      Doverlay   = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftOverlay,poi,bat);
      Dpanel     = pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftSlabPanel,poi,bat);

      // Table 1
      col = 0;
      (*table1)(row1,col++) << location.SetValue( poi,end_size );
      (*table1)(row1,col++) << displacement.SetValue( Dps1 );
      (*table1)(row1,col++) << displacement.SetValue( Dps );
      (*table1)(row1,col++) << displacement.SetValue( Dtpsi );
      (*table1)(row1,col++) << displacement.SetValue( Dtpsr );
      (*table1)(row1,col++) << displacement.SetValue( Dgirder );
      (*table1)(row1,col++) << displacement.SetValue( Dcreep1 );
      row1++;

      // Table 2
      col = 0;
      (*table2)(row2,col++) << location.SetValue( poi,end_size );
      (*table2)(row2,col++) << displacement.SetValue( Ddiaphragm );

      if ( bShearKey )
         (*table2)(row2,col++) << displacement.SetValue(Dshearkey);

      (*table2)(row2,col++) << displacement.SetValue( Dcreep2 );
      (*table2)(row2,col++) << displacement.SetValue( Dpanel );
      (*table2)(row2,col++) << displacement.SetValue( Ddeck );
      (*table2)(row2,col++) << displacement.SetValue( Duser1 );

      if ( bSidewalk )
         (*table2)(row2,col++) << displacement.SetValue(Dsidewalk);

      (*table2)(row2,col++) << displacement.SetValue( Dbarrier );
   
      if (!pBridge->IsFutureOverlay() )
         (*table2)(row2,col++) << displacement.SetValue(Doverlay);

      (*table2)(row2,col++) << displacement.SetValue( Duser2 );

      row2++;

      // Table 3
      col = 0;
      double D1 = Dgirder + Dps + Dtpsi;
      double D2 = D1 + Dcreep1;
      double D3 = D2 + Ddiaphragm + + Dshearkey + Dtpsr + Dpanel;
      double D4 = D3 + Dcreep2;
      double D5 = D4 + Ddeck + Duser1;
      double D6 = D5 + Dbarrier + Dsidewalk + Doverlay + Duser2;

      (*table3)(row3,col++) << location.SetValue( poi,end_size );
      (*table3)(row3,col++) << displacement.SetValue( D1 );
      (*table3)(row3,col++) << displacement.SetValue( D2 );
      (*table3)(row3,col++) << displacement.SetValue( D3 );

      D4 = IsZero(D4) ? 0 : D4;
      if ( D4 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << displacement.SetValue(D4) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << displacement.SetValue( D4 );
      }

      (*table3)(row3,col++) << displacement.SetValue( D5 );

      D6 = IsZero(D6) ? 0 : D6;
      if ( D6 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << displacement.SetValue(D6) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << displacement.SetValue( D6 );
      }
      (*table3)(row3,col++) << displacement.SetValue( D4 - D6 );

      row3++;
   }

   *pTable1 = table1;
   *pTable2 = table2;
   *pTable3 = table3;
}


void CCamberTable::Build_SIP(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                             IEAFDisplayUnits* pDisplayUnits,Int16 constructionRate,
                             rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(span == ALL_SPANS);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, displacement, pDisplayUnits->GetDisplacementUnit(), false );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::set<pgsTypes::Stage> stages;
   stages.insert(pgsTypes::CastingYard);
   stages.insert(pgsTypes::BridgeSite3);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( stages, span, girder, POI_DISPLACEMENT | POI_TABULAR );

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IProductForces,pProduct);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IBridge,pBridge);

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   bool bSidewalk = pProductLoads->HasSidewalkLoad(span,girder);
   bool bShearKey = pProductLoads->HasShearKeyLoad(span,girder);

   // create the tables
   rptRcTable* table1;
   rptRcTable* table2;
   rptRcTable* table3;

   table1 = pgsReportStyleHolder::CreateDefaultTable(5,"Camber - Part 1 (upwards is positive)");
   table2 = pgsReportStyleHolder::CreateDefaultTable(7 + (bSidewalk ? 1 : 0) + (!pBridge->IsFutureOverlay() ? 1 : 0) + (bShearKey ? 1 : 0),"Camber - Part 2 (upwards is positive)");
   table3 = pgsReportStyleHolder::CreateDefaultTable(6,"Camber - Part 3 (upwards is positive)");

   if ( span == ALL_SPANS )
   {
      table1->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   int col = 0;
   (*table1)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"ps") << " " << Sub2("L","g"),         rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"ps") << " " << Sub2("L","s"),         rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"girder"),     rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"creep"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   col = 0;
   (*table2)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"diaphragm"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( bShearKey )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"shear key"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"panel"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"deck"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"User1") << " = " << rptNewLine << Sub2(symbol(DELTA),"UserDC") << " + " << rptNewLine  << Sub2(symbol(DELTA),"UserDW") , rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( bSidewalk )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"sidewalk"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"barrier"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( !pBridge->IsFutureOverlay() )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"overlay"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"User2") << " = " << rptNewLine << Sub2(symbol(DELTA),"UserDC") << " + " << rptNewLine  << Sub2(symbol(DELTA),"UserDW") , rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   col = 0;
   (*table3)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"1"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   double days = (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max());
   days = ::ConvertFromSysUnits(days,unitMeasure::Day);
   std::ostringstream os;
   os << days;
   (*table3)(0,col++) << COLHDR(Sub2("D",os.str().c_str()) << " = " << Sub2(symbol(DELTA),"2"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"3"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"excess") << " = " << rptNewLine << Sub2(symbol(DELTA),"4"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR("C = " << Sub2(symbol(DELTA),"2") << " - " << Sub2(symbol(DELTA),"4"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : analysisType == pgsTypes::Continuous ? ContinuousSpan : MinSimpleContinuousEnvelope);

   // Fill up the tables
   RowIndexType row1 = table1->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      double Dps1, Dps, Dgirder, Dcreep, Ddiaphragm, Dshearkey, Dpanel, Ddeck, Duser1, Dsidewalk, Dbarrier, Doverlay, Duser2;
      Dps1       = pCamber->GetPrestressDeflection( poi, false );
      Dps        = pCamber->GetPrestressDeflection( poi, true );
      Dgirder    = pProduct->GetGirderDeflectionForCamber( poi );
      Dcreep     = pCamber->GetCreepDeflection( poi, ICamber::cpReleaseToDeck, constructionRate );
      Ddiaphragm = pCamber->GetDiaphragmDeflection( poi );
      Dshearkey  = pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftShearKey,poi,bat);
      Ddeck      = pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftSlab,poi,bat);
      Duser1     = pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftUserDC,poi,bat) + pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftUserDW,poi,bat);
      Duser2     = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftUserDC,poi,bat) + pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftUserDW,poi,bat);
      Dbarrier   = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftTrafficBarrier,poi,bat);
      Dsidewalk  = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftSidewalk,      poi,bat);
      Doverlay   = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftOverlay,poi,bat);
      Dpanel     = pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftSlabPanel,poi,bat);

      // Table 1
      col = 0;
      (*table1)(row1,col++) << location.SetValue( poi,end_size );
      (*table1)(row1,col++) << displacement.SetValue( Dps1 );
      (*table1)(row1,col++) << displacement.SetValue( Dps );
      (*table1)(row1,col++) << displacement.SetValue( Dgirder );
      (*table1)(row1,col++) << displacement.SetValue( Dcreep );
      row1++;

      // Table 2
      col = 0;
      (*table2)(row2,col++) << location.SetValue( poi,end_size );
      (*table2)(row2,col++) << displacement.SetValue( Ddiaphragm );

      if ( bShearKey )
         (*table2)(row2,col++) << displacement.SetValue( Dshearkey);

      (*table2)(row2,col++) << displacement.SetValue( Dpanel );
      (*table2)(row2,col++) << displacement.SetValue( Ddeck );
      (*table2)(row2,col++) << displacement.SetValue( Duser1 );

      if ( bSidewalk )
         (*table2)(row2,col++) << displacement.SetValue( Dsidewalk);

      (*table2)(row2,col++) << displacement.SetValue( Dbarrier );

      if (!pBridge->IsFutureOverlay() )
         (*table2)(row2,col++) << displacement.SetValue(Doverlay);

      (*table2)(row2,col++) << displacement.SetValue( Duser2 );

      row2++;

      // Table 3
      col = 0;
      double D1 = Dgirder + Dps;
      double D2 = D1 + Dcreep;
      double D3 = D2 + Ddiaphragm + Dshearkey + Dpanel;
      double D4 = D3 + Ddeck + Duser1 + Dsidewalk + Dbarrier + Doverlay + Duser2;

      (*table3)(row3,col++) << location.SetValue( poi,end_size );
      (*table3)(row3,col++) << displacement.SetValue( D1 );

      D2 = IsZero(D2) ? 0 : D2;
      if ( D2 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << displacement.SetValue(D2) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << displacement.SetValue( D2 );
      }

      (*table3)(row3,col++) << displacement.SetValue( D3 );

      D4 = IsZero(D4) ? 0 : D4;
      if ( D4 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << displacement.SetValue(D4) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << displacement.SetValue( D4 );
      }
      (*table3)(row3,col++) << displacement.SetValue( D2 - D4 );

      row3++;
   }

   *pTable1 = table1;
   *pTable2 = table2;
   *pTable3 = table3;
}

void CCamberTable::Build_NoDeck_TempStrands(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                            IEAFDisplayUnits* pDisplayUnits,Int16 constructionRate,
                                            rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(span == ALL_SPANS);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, displacement, pDisplayUnits->GetDisplacementUnit(), false );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::set<pgsTypes::Stage> stages;
   stages.insert(pgsTypes::CastingYard);
   stages.insert(pgsTypes::BridgeSite3);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( stages, span, girder, POI_DISPLACEMENT | POI_TABULAR );

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IProductForces,pProduct);
   GET_IFACE2(pBroker,IBridge,pBridge);

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   bool bSidewalk = pProductLoads->HasSidewalkLoad(span,girder);
   bool bShearKey = pProductLoads->HasShearKeyLoad(span,girder);

   // create the tables
   rptRcTable* table1;
   rptRcTable* table2;
   rptRcTable* table3;

   table1 = pgsReportStyleHolder::CreateDefaultTable(7,"Camber - Part 1 (upwards is positive)");
   table2 = pgsReportStyleHolder::CreateDefaultTable(8 + (bSidewalk ? 1 : 0) + (!pBridge->IsFutureOverlay() ? 1 : 0) + (bShearKey ? 1 : 0),"Camber - Part 2 (upwards is positive)");
   table3 = pgsReportStyleHolder::CreateDefaultTable(7,"Camber - Part 3 (upwards is positive)");

   if ( span == ALL_SPANS )
   {
      table1->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   int col = 0;
   (*table1)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"ps") << " " << Sub2("L","g"),         rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"ps") << " " << Sub2("L","s"),         rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"tpsi"),       rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"tpsr"),       rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"girder"),     rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"creep1"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   col = 0;
   (*table2)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"diaphragm"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( bShearKey )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"shear key"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"creep2"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"deck"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"User1") << " = " << rptNewLine << Sub2(symbol(DELTA),"UserDC") << " + " << rptNewLine  << Sub2(symbol(DELTA),"UserDW") , rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if (bSidewalk)
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"sidewalk"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"barrier"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( !pBridge->IsFutureOverlay() )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"overlay"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"User2") << " = " << rptNewLine << Sub2(symbol(DELTA),"UserDC") << " + " << rptNewLine  << Sub2(symbol(DELTA),"UserDW") , rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"creep3"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   col = 0;
   (*table3)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"1"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"2"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"3"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   
   double days = (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max());
   days = ::ConvertFromSysUnits(days,unitMeasure::Day);
   std::ostringstream os;
   os << days;
   (*table3)(0,col++) << COLHDR(Sub2("D",os.str().c_str()) << " = " << Sub2(symbol(DELTA),"4"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"5"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR("C = " << Sub2(symbol(DELTA),"excess") << " = " << rptNewLine << Sub2(symbol(DELTA),"6"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : analysisType == pgsTypes::Continuous ? ContinuousSpan : MinSimpleContinuousEnvelope);

   // Fill up the tables
   RowIndexType row1 = table1->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      double Dps1, Dps, Dtpsi, Dtpsr, Dgirder, Dcreep1, Ddiaphragm, Dshearkey, Ddeck, Dcreep2, Duser1, Dsidewalk, Dbarrier, Doverlay, Duser2, Dcreep3;
      Dps1       = pCamber->GetPrestressDeflection( poi, false );
      Dps        = pCamber->GetPrestressDeflection( poi, true );
      Dtpsi      = pCamber->GetInitialTempPrestressDeflection( poi,true );
      Dtpsr      = pCamber->GetReleaseTempPrestressDeflection( poi );
      Dgirder    = pProduct->GetGirderDeflectionForCamber( poi );
      Dcreep1    = pCamber->GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate );
      Ddiaphragm = pCamber->GetDiaphragmDeflection( poi );
      Dshearkey  = pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftShearKey,poi,bat);
      Ddeck      = pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftSlab,poi,bat);
      Dcreep2    = pCamber->GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate );
      Duser1     = pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftUserDC,poi,bat) + pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftUserDW,poi,bat);
      Duser2     = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftUserDC,poi,bat) + pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftUserDW,poi,bat);
      Dsidewalk  = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftSidewalk,      poi,bat);
      Dbarrier   = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftTrafficBarrier,poi,bat);
      Doverlay   = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftOverlay,poi,bat);
      Dcreep3    = pCamber->GetCreepDeflection( poi, ICamber::cpDeckToFinal, constructionRate );

      // Table 1
      col = 0;
      (*table1)(row1,col++) << location.SetValue( poi,end_size );
      (*table1)(row1,col++) << displacement.SetValue( Dps1 );
      (*table1)(row1,col++) << displacement.SetValue( Dps );
      (*table1)(row1,col++) << displacement.SetValue( Dtpsi );
      (*table1)(row1,col++) << displacement.SetValue( Dtpsr );
      (*table1)(row1,col++) << displacement.SetValue( Dgirder );
      (*table1)(row1,col++) << displacement.SetValue( Dcreep1 );
      row1++;

      // Table 2
      col = 0;
      (*table2)(row2,col++) << location.SetValue( poi,end_size );
      (*table2)(row2,col++) << displacement.SetValue( Ddiaphragm );

      if (bShearKey)
      (*table2)(row2,col++) << displacement.SetValue( Dshearkey );

      (*table2)(row2,col++) << displacement.SetValue( Dcreep2 );
      (*table2)(row2,col++) << displacement.SetValue( Ddeck );
      (*table2)(row2,col++) << displacement.SetValue( Duser1 );

      if (bSidewalk)
         (*table2)(row2,col++) << displacement.SetValue(Dsidewalk);

      (*table2)(row2,col++) << displacement.SetValue( Dbarrier );

      if (!pBridge->IsFutureOverlay())
         (*table2)(row2,col++) << displacement.SetValue(Doverlay);

      (*table2)(row2,col++) << displacement.SetValue( Duser2 );
      (*table2)(row2,col++) << displacement.SetValue( Dcreep3 );

      row2++;

      // Table 3
      col = 0;
      double D1 = Dgirder + Dps + Dtpsi;
      double D2 = D1 + Dcreep1;
      double D3 = D2 + Ddiaphragm + Dshearkey + Dtpsr + Duser1;
      double D4 = D3 + Dcreep2;
      double D5 = D4 + Dsidewalk + Dbarrier + Doverlay + Duser2;
      double D6 = D5 + Dcreep3;

      (*table3)(row3,col++) << location.SetValue( poi,end_size );
      (*table3)(row3,col++) << displacement.SetValue( D1 );
      (*table3)(row3,col++) << displacement.SetValue( D2 );
      (*table3)(row3,col++) << displacement.SetValue( D3 );

      D4 = IsZero(D4) ? 0 : D4;
      if ( D4 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << displacement.SetValue(D4) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << displacement.SetValue( D4 );
      }
      
      (*table3)(row3,col++) << displacement.SetValue( D5 );

      D6 = IsZero(D6) ? 0 : D6;
      if ( D6 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << displacement.SetValue(D6) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << displacement.SetValue( D6 );
      }

      row3++;
   }

   *pTable1 = table1;
   *pTable2 = table2;
   *pTable3 = table3;
}

void CCamberTable::Build_NoDeck(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                IEAFDisplayUnits* pDisplayUnits,Int16 constructionRate,
                                rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(span == ALL_SPANS);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, displacement, pDisplayUnits->GetDisplacementUnit(), false );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::set<pgsTypes::Stage> stages;
   stages.insert(pgsTypes::CastingYard);
   stages.insert(pgsTypes::BridgeSite3);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( stages, span, girder, POI_DISPLACEMENT | POI_TABULAR );

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IProductForces,pProduct);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IBridge,pBridge);

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   bool bSidewalk = pProductLoads->HasSidewalkLoad(span,girder);
   bool bShearKey = pProductLoads->HasShearKeyLoad(span,girder);

   // create the tables
   rptRcTable* table1;
   rptRcTable* table2;
   rptRcTable* table3;

   table1 = pgsReportStyleHolder::CreateDefaultTable(5,"Camber - Part 1 (upwards is positive)");
   table2 = pgsReportStyleHolder::CreateDefaultTable(7 + (bSidewalk ? 1 : 0) + (!pBridge->IsFutureOverlay() ? 1 : 0) + (bShearKey ? 1 : 0),"Camber - Part 2 (upwards is positive)");
   table3 = pgsReportStyleHolder::CreateDefaultTable(7,"Camber - Part 3 (upwards is positive)");

   if ( span == ALL_SPANS )
   {
      table1->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   int col = 0;
   (*table1)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"ps") << " " << Sub2("L","g"),         rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"ps") << " " << Sub2("L","s"),         rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"girder"),     rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table1)(0,col++) << COLHDR(Sub2(symbol(DELTA),"creep1"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   col = 0;
   (*table2)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"diaphragm"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( bShearKey )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"shear key"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"User1") << " = " << rptNewLine << Sub2(symbol(DELTA),"UserDC") << " + " << rptNewLine  << Sub2(symbol(DELTA),"UserDW") , rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"creep2"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( bSidewalk )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"sidewalk"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"barrier"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( !pBridge->IsFutureOverlay() )
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"overlay"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"User2") << " = " << rptNewLine << Sub2(symbol(DELTA),"UserDC") << " + " << rptNewLine  << Sub2(symbol(DELTA),"UserDW") , rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),"creep3"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   col = 0;
   (*table3)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"1"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"2"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"3"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   double days = (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max());
   days = ::ConvertFromSysUnits(days,unitMeasure::Day);
   std::ostringstream os;
   os << days;
   (*table3)(0,col++) << COLHDR(Sub2("D",os.str().c_str()) << " = " << Sub2(symbol(DELTA),"4"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),"5"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*table3)(0,col++) << COLHDR("C = " << Sub2(symbol(DELTA),"excess") << " = " << rptNewLine << Sub2(symbol(DELTA),"6"),  rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : analysisType == pgsTypes::Continuous ? ContinuousSpan : MinSimpleContinuousEnvelope);

   // Fill up the tables
   RowIndexType row1 = table1->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      double Dps1, Dps, Dgirder, Dcreep1, Ddiaphragm, Dshearkey, Dcreep2, Duser1, Dsidewalk, Dbarrier, Doverlay, Duser2, Dcreep3;
      Dps1       = pCamber->GetPrestressDeflection( poi, false );
      Dps        = pCamber->GetPrestressDeflection( poi, true );
      Dgirder    = pProduct->GetGirderDeflectionForCamber( poi );
      Dcreep1    = pCamber->GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate );
      Ddiaphragm = pCamber->GetDiaphragmDeflection( poi );
      Dshearkey  = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftShearKey,      poi,bat);
      Dcreep2    = pCamber->GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate );
      Duser1     = pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftUserDC,poi,bat) + pProduct->GetDisplacement(pgsTypes::BridgeSite1,pftUserDW,poi,bat);
      Duser2     = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftUserDC,poi,bat) + pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftUserDW,poi,bat);
      Dsidewalk  = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftSidewalk,      poi,bat);
      Dbarrier   = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftTrafficBarrier,poi,bat);
      Doverlay   = pProduct->GetDisplacement(pgsTypes::BridgeSite2,pftOverlay,poi,bat);
      Dcreep3    = pCamber->GetCreepDeflection( poi, ICamber::cpDeckToFinal, constructionRate );

      // Table 1
      col = 0;
      (*table1)(row1,col++) << location.SetValue( poi,end_size );
      (*table1)(row1,col++) << displacement.SetValue( Dps1 );
      (*table1)(row1,col++) << displacement.SetValue( Dps );
      (*table1)(row1,col++) << displacement.SetValue( Dgirder );
      (*table1)(row1,col++) << displacement.SetValue( Dcreep1 );

      row1++;

      // Table 2
      col = 0;
      (*table2)(row2,col++) << location.SetValue( poi,end_size );
      (*table2)(row2,col++) << displacement.SetValue( Ddiaphragm );

      if ( bShearKey )
         (*table2)(row2,col++) << displacement.SetValue( Dshearkey );

      (*table2)(row2,col++) << displacement.SetValue( Duser1 );
      (*table2)(row2,col++) << displacement.SetValue( Dcreep2 );

      if ( bSidewalk )
         (*table2)(row2,col++) << displacement.SetValue(Dsidewalk);

      (*table2)(row2,col++) << displacement.SetValue( Dbarrier );

      if (!pBridge->IsFutureOverlay() )
         (*table2)(row2,col++) << displacement.SetValue(Doverlay);

      (*table2)(row2,col++) << displacement.SetValue( Duser2 );
      (*table2)(row2,col++) << displacement.SetValue( Dcreep3 );

      row2++;

      // Table 3
      col = 0;
      double D1 = Dgirder + Dps;
      double D2 = D1 + Dcreep1;
      double D3 = D2 + Ddiaphragm + Dshearkey + Duser1;
      double D4 = D3 + Dcreep2;
      double D5 = D4 + Dsidewalk + Dbarrier + Doverlay + Duser2;
      double D6 = D5 + Dcreep3;

      (*table3)(row3,col++) << location.SetValue( poi,end_size );
      (*table3)(row3,col++) << displacement.SetValue( D1 );
      (*table3)(row3,col++) << displacement.SetValue( D2 );
      (*table3)(row3,col++) << displacement.SetValue( D3 );

      D4 = IsZero(D4) ? 0 : D4;
      if ( D4 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << displacement.SetValue(D4) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << displacement.SetValue( D4 );
      }

      (*table3)(row3,col++) << displacement.SetValue( D5 );

      D6 = IsZero(D6) ? 0 : D6;
      if ( D6 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << displacement.SetValue(D6) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << displacement.SetValue( D6 );
      }

      row3++;
   }

   *pTable1 = table1;
   *pTable2 = table2;
   *pTable3 = table3;
}
