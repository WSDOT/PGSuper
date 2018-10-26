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
#include <Reporting\UserStressTable.h>
#include <Reporting\UserMomentsTable.h>
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
   CUserStressTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CUserStressTable::CUserStressTable()
{
}

CUserStressTable::CUserStressTable(const CUserStressTable& rOther)
{
   MakeCopy(rOther);
}

CUserStressTable::~CUserStressTable()
{
}

//======================== OPERATORS  =======================================
CUserStressTable& CUserStressTable::operator= (const CUserStressTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CUserStressTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,pgsTypes::AnalysisType analysisType,
                                      IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );
   location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptRcTable* p_table = CreateUserLoadHeading<rptStressUnitTag,unitmgtStressData>(_T("User Defined Loads"),false,analysisType,pDisplayUnits,pDisplayUnits->GetStressUnit());

   if ( span == ALL_SPANS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);
   GET_IFACE2(pBroker,IBridge,pBridge);

   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType nSpans    = (span == ALL_SPANS ? pBridge->GetSpanCount() : startSpan+1 );

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   for ( SpanIndexType spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      // Get all the tabular poi's for flexure and shear
      // Merge the two vectors to form one vector to report on.
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = (nGirders <= girder ? nGirders-1 : girder);

      std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(spanIdx,gdrIdx, pgsTypes::BridgeSite1,POI_ALL, POIFIND_OR);

      Float64 end_size = pBridge->GetGirderStartConnectionLength(spanIdx,gdrIdx);

      std::vector<Float64> dummy;
      std::vector<Float64> fTopMaxDC1, fBotMaxDC1;
      std::vector<Float64> fTopMinDC1, fBotMinDC1;
      std::vector<Float64> fTopMaxDC2, fBotMaxDC2;
      std::vector<Float64> fTopMinDC2, fBotMinDC2;
      std::vector<Float64> fTopMaxDW1, fBotMaxDW1;
      std::vector<Float64> fTopMinDW1, fBotMinDW1;
      std::vector<Float64> fTopMaxDW2, fBotMaxDW2;
      std::vector<Float64> fTopMinDW2, fBotMinDW2;
      std::vector<Float64> fTopMaxLL3, fBotMaxLL3;
      std::vector<Float64> fTopMinLL3, fBotMinLL3;

      if ( analysisType == pgsTypes::Envelope )
      {
         pForces2->GetStress(pgsTypes::BridgeSite1, pftUserDC, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxDC1, &fBotMaxDC1);
         pForces2->GetStress(pgsTypes::BridgeSite1, pftUserDC, vPoi, MinSimpleContinuousEnvelope, &fTopMinDC1, &fBotMinDC1);
         pForces2->GetStress(pgsTypes::BridgeSite2, pftUserDC, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxDC2, &fBotMaxDC2);
         pForces2->GetStress(pgsTypes::BridgeSite2, pftUserDC, vPoi, MinSimpleContinuousEnvelope, &fTopMinDC2, &fBotMinDC2);
      }
      else
      {
         pForces2->GetStress(pgsTypes::BridgeSite1, pftUserDC, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxDC1, &fBotMaxDC1);
         pForces2->GetStress(pgsTypes::BridgeSite2, pftUserDC, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxDC2, &fBotMaxDC2);
      }

      if ( analysisType == pgsTypes::Envelope )
      {
         pForces2->GetStress(pgsTypes::BridgeSite1, pftUserDW, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxDW1, &fBotMaxDW1);
         pForces2->GetStress(pgsTypes::BridgeSite1, pftUserDW, vPoi, MinSimpleContinuousEnvelope, &fTopMinDW1, &fBotMinDW1);

         pForces2->GetStress(pgsTypes::BridgeSite2, pftUserDW, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxDW2, &fBotMaxDW2);
         pForces2->GetStress(pgsTypes::BridgeSite2, pftUserDW, vPoi, MinSimpleContinuousEnvelope, &fTopMinDW2, &fBotMinDW2);

         pForces2->GetStress(pgsTypes::BridgeSite3, pftUserLLIM, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxLL3, &fBotMaxLL3);
         pForces2->GetStress(pgsTypes::BridgeSite3, pftUserLLIM, vPoi, MinSimpleContinuousEnvelope, &fTopMinLL3, &fBotMinLL3);
      }
      else
      {
         pForces2->GetStress(pgsTypes::BridgeSite1, pftUserDW, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxDW1, &fBotMaxDW1);
         pForces2->GetStress(pgsTypes::BridgeSite2, pftUserDW, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxDW2, &fBotMaxDW2);
         pForces2->GetStress(pgsTypes::BridgeSite3, pftUserLLIM, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxLL3, &fBotMaxLL3);
      }


      std::vector<pgsPointOfInterest>::const_iterator i;
      long index = 0;
      for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
      {
         int col = 0;

         const pgsPointOfInterest& poi = *i;

         (*p_table)(row,col++) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );

         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxDC1[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxDC1[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMinDC1[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMinDC1[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxDW1[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxDW1[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMinDW1[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMinDW1[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxDC2[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxDC2[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMinDC2[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMinDC2[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxDW2[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxDW2[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMinDW2[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMinDW2[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxLL3[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxLL3[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMinLL3[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMinLL3[index] );
         }
         else
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxDC1[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxDC1[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxDW1[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxDW1[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxDC2[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxDC2[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxDW2[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxDW2[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxLL3[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxLL3[index] );
         }

         row++;
      }
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CUserStressTable::MakeCopy(const CUserStressTable& rOther)
{
   // Add copy code here...
}

void CUserStressTable::MakeAssignment(const CUserStressTable& rOther)
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
