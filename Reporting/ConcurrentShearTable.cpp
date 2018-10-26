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
#include <Reporting\ConcurrentShearTable.h>
#include <Reporting\MVRChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CConcurrentShearTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CConcurrentShearTable::CConcurrentShearTable()
{
}

CConcurrentShearTable::CConcurrentShearTable(const CConcurrentShearTable& rOther)
{
   MakeCopy(rOther);
}

CConcurrentShearTable::~CConcurrentShearTable()
{
}

//======================== OPERATORS  =======================================
CConcurrentShearTable& CConcurrentShearTable::operator= (const CConcurrentShearTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CConcurrentShearTable::Build(IBroker* pBroker,rptChapter* pChapter,
                                       SpanIndexType span,GirderIndexType girder,
                                       IEAFDisplayUnits* pDisplayUnits,
                                       pgsTypes::Stage stage,pgsTypes::AnalysisType analysisType) const
{
   if ( stage == pgsTypes::CastingYard )
      return;

   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetGeneralForceUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment, pDisplayUnits->GetMomentUnit(), false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = 0;

   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType nSpans    = (span == ALL_SPANS ? pBridge->GetSpanCount() : startSpan+1 );
 
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

   ColumnIndexType col = 0;

   p_table = pgsReportStyleHolder::CreateDefaultTable(3,_T("Concurrent Shears"));

   if ( span == ALL_SPANS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,col++) << COLHDR(Sub2(_T("M"),_T("max")) << rptNewLine << _T("Strength I"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,col++) << COLHDR(Sub2(_T("V"),_T("i")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );

   *p << p_table;

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);

   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);

   // Fill up the table
   sysSectionValue Vmin, Vmax;
   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      Float64 end_size = pBridge->GetGirderStartConnectionLength(spanIdx,girder);
      if ( stage == pgsTypes::CastingYard )
         end_size = 0; // don't adjust if CY stage

      std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( spanIdx, girder, stage, POI_ALL, POIFIND_OR );
      std::vector<pgsPointOfInterest>::const_iterator i;
      for ( i = vPoi.begin(); i != vPoi.end(); i++ )
      {
         const pgsPointOfInterest& poi = *i;

         col = 0;

         (*p_table)(row,col++) << location.SetValue( stage, poi, end_size );

         double Vi, Mmax;
         pLsForces->GetViMmax(pgsTypes::StrengthI,stage,poi,bat,&Vi,&Mmax);

         (*p_table)(row,col++) << moment.SetValue( Mmax );
         (*p_table)(row,col++) << shear.SetValue(  Vi );

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
void CConcurrentShearTable::MakeCopy(const CConcurrentShearTable& rOther)
{
   // Add copy code here...
}

void CConcurrentShearTable::MakeAssignment(const CConcurrentShearTable& rOther)
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
bool CConcurrentShearTable::AssertValid() const
{
   return true;
}

void CConcurrentShearTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CConcurrentShearTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CConcurrentShearTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CConcurrentShearTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CConcurrentShearTable");

   TESTME_EPILOG("CConcurrentShearTable");
}
#endif // _UNITTEST
