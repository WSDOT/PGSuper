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
#include <Reporting\UserRotationTable.h>
#include <Reporting\UserMomentsTable.h>

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
   CUserRotationTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CUserRotationTable::CUserRotationTable()
{
}

CUserRotationTable::CUserRotationTable(const CUserRotationTable& rOther)
{
   MakeCopy(rOther);
}

CUserRotationTable::~CUserRotationTable()
{
}

//======================== OPERATORS  =======================================
CUserRotationTable& CUserRotationTable::operator= (const CUserRotationTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CUserRotationTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,pgsTypes::AnalysisType analysisType,
                                      IEAFDisplayUnits* pDisplayUnits) const
{
   ATLASSERT(girder != ALL_GIRDERS);

   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, rotation, pDisplayUnits->GetRadAngleUnit(), false );

   rptRcTable* p_table = CreateUserLoadHeading<rptAngleUnitTag,unitmgtAngleData>("Rotations - User Defined Loads",true,analysisType,pDisplayUnits,pDisplayUnits->GetRadAngleUnit());

   GET_IFACE2(pBroker,IProductForces,pForces);
   GET_IFACE2(pBroker,IBridge,pBridge);

   PierIndexType nPiers = pBridge->GetPierCount();

   PierIndexType startPier = (span == ALL_SPANS ? 0 : span);
   PierIndexType endPier   = (span == ALL_SPANS ? nPiers : startPier+2 );

   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPoi;

   SpanIndexType nSpans = pBridge->GetSpanCount();
   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType endSpan   = (span == ALL_SPANS ? nSpans : startSpan+1);
   for ( SpanIndexType spanIdx = startSpan; spanIdx < endSpan; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = (girder < nGirders ? girder : nGirders-1);

      std::vector<pgsPointOfInterest> vTenthPoints = pPOI->GetTenthPointPOIs(pgsTypes::BridgeSite3,spanIdx,gdrIdx);
      vPoi.push_back(*vTenthPoints.begin());
      vPoi.push_back(*(vTenthPoints.end()-1));
   }

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( PierIndexType pier = startPier; pier < endPier; pier++ )
   {
      ColumnIndexType col = 0;
      pgsPointOfInterest& poi = vPoi[pier-startPier];

      if ( pier == 0 || pier == pBridge->GetPierCount()-1 )
         (*p_table)(row,col++) << "Abutment " << LABEL_PIER(pier);
      else
         (*p_table)(row,col++) << "Pier " << LABEL_PIER(pier);


      if ( analysisType == pgsTypes::Envelope )
      {
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftUserDC, poi, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftUserDC, poi, MinSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftUserDW, poi, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftUserDW, poi, MinSimpleContinuousEnvelope ) );

         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite2, pftUserDC,    poi, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite2, pftUserDC,    poi, MinSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite2, pftUserDW,    poi, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite2, pftUserDW,    poi, MinSimpleContinuousEnvelope ) );
         
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite3, pftUserLLIM, poi, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite3, pftUserLLIM, poi, MinSimpleContinuousEnvelope ) );
      }
      else
      {
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftUserDC, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftUserDW, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite2, pftUserDC, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite2, pftUserDW,    poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite3, pftUserLLIM, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
      }

      row++;
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CUserRotationTable::MakeCopy(const CUserRotationTable& rOther)
{
   // Add copy code here...
}

void CUserRotationTable::MakeAssignment(const CUserRotationTable& rOther)
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
bool CUserRotationTable::AssertValid() const
{
   return true;
}

void CUserRotationTable::Dump(dbgDumpContext& os) const
{
   os << "Dump for CUserRotationTable" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CUserRotationTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CUserRotationTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CUserRotationTable");

   TESTME_EPILOG("CUserRotationTable");
}
#endif // _UNITTEST
