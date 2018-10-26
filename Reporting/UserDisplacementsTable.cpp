///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <Reporting\UserDisplacementsTable.h>
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
   CUserDisplacementsTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CUserDisplacementsTable::CUserDisplacementsTable()
{
}

CUserDisplacementsTable::CUserDisplacementsTable(const CUserDisplacementsTable& rOther)
{
   MakeCopy(rOther);
}

CUserDisplacementsTable::~CUserDisplacementsTable()
{
}

//======================== OPERATORS  =======================================
CUserDisplacementsTable& CUserDisplacementsTable::operator= (const CUserDisplacementsTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CUserDisplacementsTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,pgsTypes::AnalysisType analysisType,
                                              IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, displacement, pDisplayUnits->GetDisplacementUnit(), false );
   location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptRcTable* p_table = CreateUserLoadHeading<rptLengthUnitTag,unitmgtLengthData>(_T("Displacements - User Defined Loads"),false,analysisType,pDisplayUnits,pDisplayUnits->GetDisplacementUnit());

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

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( SpanIndexType spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = (nGirders <= girder ? nGirders-1 : girder);

      std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( spanIdx, gdrIdx, pgsTypes::BridgeSite1, POI_ALL, POIFIND_OR );

      Float64 end_size = pBridge->GetGirderStartConnectionLength(spanIdx,gdrIdx);

      std::vector<Float64> minDC1, maxDC1, minDC2, maxDC2;
      std::vector<Float64> minDW1, maxDW1, minDW2, maxDW2;
      std::vector<Float64> minLL3, maxLL3;


      if ( analysisType == pgsTypes::Envelope )
      {
         maxDC1 = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftUserDC, vPoi, MaxSimpleContinuousEnvelope );
         minDC1 = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftUserDC, vPoi, MinSimpleContinuousEnvelope );
         maxDC2 = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftUserDC, vPoi, MaxSimpleContinuousEnvelope );
         minDC2 = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftUserDC, vPoi, MinSimpleContinuousEnvelope );
      }
      else
      {
         maxDC1 = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftUserDC, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         maxDC2 = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftUserDC, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
      }


      if ( analysisType == pgsTypes::Envelope )
      {
         maxDW1 = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftUserDW, vPoi, MaxSimpleContinuousEnvelope );
         minDW1 = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftUserDW, vPoi, MinSimpleContinuousEnvelope );
         maxDW2 = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftUserDW, vPoi, MaxSimpleContinuousEnvelope );
         minDW2 = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftUserDW, vPoi, MinSimpleContinuousEnvelope );
         maxLL3 = pForces2->GetDisplacement( pgsTypes::BridgeSite3, pftUserLLIM, vPoi, MaxSimpleContinuousEnvelope );
         minLL3 = pForces2->GetDisplacement( pgsTypes::BridgeSite3, pftUserLLIM, vPoi, MinSimpleContinuousEnvelope );
      }
      else
      {
         maxDW1 = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftUserDW, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         maxDW2 = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftUserDW, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         maxLL3 = pForces2->GetDisplacement( pgsTypes::BridgeSite3, pftUserLLIM, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
      }

      // Fill up the table
      long index = 0;
      std::vector<pgsPointOfInterest>::const_iterator i;
      for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
      {
         int col = 0;
         const pgsPointOfInterest& poi = *i;

         (*p_table)(row,col++) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );

         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col++) << displacement.SetValue( maxDC1[index] );
            (*p_table)(row,col++) << displacement.SetValue( minDC1[index] );
            (*p_table)(row,col++) << displacement.SetValue( maxDW1[index] );
            (*p_table)(row,col++) << displacement.SetValue( minDW1[index] );

            (*p_table)(row,col++) << displacement.SetValue( maxDC2[index] );
            (*p_table)(row,col++) << displacement.SetValue( minDC2[index] );
            (*p_table)(row,col++) << displacement.SetValue( maxDW2[index] );
            (*p_table)(row,col++) << displacement.SetValue( minDW2[index] );
            
            (*p_table)(row,col++) << displacement.SetValue( maxLL3[index] );
            (*p_table)(row,col++) << displacement.SetValue( minLL3[index] );
         }
         else
         {
            (*p_table)(row,col++) << displacement.SetValue( maxDC1[index] );
            (*p_table)(row,col++) << displacement.SetValue( maxDW1[index] );
            (*p_table)(row,col++) << displacement.SetValue( maxDC2[index] );
            (*p_table)(row,col++) << displacement.SetValue( maxDW2[index] );
            (*p_table)(row,col++) << displacement.SetValue( maxLL3[index] );
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
void CUserDisplacementsTable::MakeCopy(const CUserDisplacementsTable& rOther)
{
   // Add copy code here...
}

void CUserDisplacementsTable::MakeAssignment(const CUserDisplacementsTable& rOther)
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
bool CUserDisplacementsTable::AssertValid() const
{
   return true;
}

void CUserDisplacementsTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CUserDisplacementsTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CUserDisplacementsTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CUserDisplacementsTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CUserDisplacementsTable");

   TESTME_EPILOG("CUserDisplacementsTable");
}
#endif // _UNITTEST
