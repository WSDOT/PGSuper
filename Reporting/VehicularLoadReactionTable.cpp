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
#include <Reporting\VehicularLoadReactionTable.h>
#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\ReportNotes.h>

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
   CVehicularLoadReactionTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CVehicularLoadReactionTable::CVehicularLoadReactionTable()
{
}

CVehicularLoadReactionTable::CVehicularLoadReactionTable(const CVehicularLoadReactionTable& rOther)
{
   MakeCopy(rOther);
}

CVehicularLoadReactionTable::~CVehicularLoadReactionTable()
{
}

//======================== OPERATORS  =======================================
CVehicularLoadReactionTable& CVehicularLoadReactionTable::operator= (const CVehicularLoadReactionTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CVehicularLoadReactionTable::Build(IBroker* pBroker,
                                               SpanIndexType span,
                                               GirderIndexType girder,
                                               pgsTypes::LiveLoadType llType,
                                               const std::_tstring& strLLName,
                                               VehicleIndexType vehicleIndex, 
                                               pgsTypes::AnalysisType analysisType,
                                               bool bReportTruckConfig,
                                               IEAFDisplayUnits* pDisplayUnits) const

{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  reaction, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,  rotation, pDisplayUnits->GetRadAngleUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);

   bool bPermit = false;

   PierIndexType nPiers = pBridge->GetPierCount();

   PierIndexType startPier = (span == ALL_SPANS ? 0 : span);
   PierIndexType endPier   = (span == ALL_SPANS ? nPiers : startPier+2 );

   pgsTypes::Stage continuity_stage = pgsTypes::BridgeSite2;
   PierIndexType pier;
   for ( pier = startPier; pier < endPier; pier++ )
   {
      pgsTypes::Stage left_stage, right_stage;
      pBridge->GetContinuityStage(pier,&left_stage,&right_stage);
      continuity_stage = _cpp_min(continuity_stage,left_stage);
      continuity_stage = _cpp_min(continuity_stage,right_stage);
   }

   ColumnIndexType nCols = 5;

   if ( bReportTruckConfig )
      nCols += 4;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Live Load Reactions and Rotations for ") + strLLName);

   // Set up table headings
   ColumnIndexType col = 0;
   (*p_table)(0,col++) << _T("");
   (*p_table)(0,col++) << COLHDR(_T("Reaction") << rptNewLine << _T("Max"),   rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << _T("Reaction") << rptNewLine << _T("Max") << rptNewLine << _T("Config");
   }

   (*p_table)(0,col++) << COLHDR(_T("Reaction") << rptNewLine << _T("Min"),   rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << _T("Reaction") << rptNewLine << _T("Min") << rptNewLine << _T("Config");
   }

   (*p_table)(0,col++) << COLHDR(_T("Rotation") << rptNewLine << _T("Max"),   rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << _T("Rotation") << rptNewLine << _T("Max") << rptNewLine << _T("Config");
   }

   (*p_table)(0,col++) << COLHDR(_T("Rotation") << rptNewLine << _T("Min"),   rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit() );


   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << _T("Rotation") << rptNewLine << _T("Min") << rptNewLine << _T("Config");
   }

   // Get POI at start and end of the span
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPoi;

   SpanIndexType nSpans = pBridge->GetSpanCount();
   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType endSpan   = (span == ALL_SPANS ? nSpans : startSpan+1);
   for ( SpanIndexType spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(girder,nGirders-1);
      std::vector<pgsPointOfInterest> vTenthPoints = pPOI->GetTenthPointPOIs(pgsTypes::BridgeSite3,spanIdx,gdrIdx);
      vPoi.push_back(*vTenthPoints.begin());
      vPoi.push_back(*(vTenthPoints.end()-1));
   }

   GET_IFACE2(pBroker,IProductForces,pForces);

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( pier = startPier; pier < endPier; pier++ )
   {
      col = 0;
      pgsPointOfInterest& poi = vPoi[pier-startPier];

      if (pier == 0 || pier == nPiers-1 )
         (*p_table)(row,col++) << _T("Abutment ") << LABEL_PIER(pier);
      else
         (*p_table)(row,col++) << _T("Pier ") << LABEL_PIER(pier);

      if ( analysisType == pgsTypes::Envelope )
      {
         Float64 Rmin, Rmax;
         AxleConfiguration maxConfig, minConfig;
         pForces->GetVehicularLiveLoadReaction( llType, vehicleIndex, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope,  true, false, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*p_table)(row,col++) << reaction.SetValue( Rmax );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(maxConfig,p_table,row,col++,pDisplayUnits);
         }

         pForces->GetVehicularLiveLoadReaction( llType, vehicleIndex, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope,  true, false, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*p_table)(row,col++) << reaction.SetValue( Rmin );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(minConfig,p_table,row,col++,pDisplayUnits);
         }

         pForces->GetVehicularLiveLoadRotation( llType, vehicleIndex, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, true, false, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*p_table)(row,col++) << rotation.SetValue( Rmax );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(maxConfig,p_table,row,col++,pDisplayUnits);
         }

         pForces->GetVehicularLiveLoadRotation( llType, vehicleIndex, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, true, false, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*p_table)(row,col++) << rotation.SetValue( Rmin );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(minConfig,p_table,row,col++,pDisplayUnits);
         }
      }
      else
      {
         Float64 Rmin, Rmax;
         AxleConfiguration maxConfig, minConfig;
         pForces->GetVehicularLiveLoadReaction( llType, vehicleIndex, pgsTypes::BridgeSite3, pier, girder, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan,  true, false, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*p_table)(row,col++) << reaction.SetValue( Rmax );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(maxConfig,p_table,row,col++,pDisplayUnits);
         }

         (*p_table)(row,col++) << reaction.SetValue( Rmin );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(minConfig,p_table,row,col++,pDisplayUnits);
         }

         pForces->GetVehicularLiveLoadRotation( llType, vehicleIndex, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*p_table)(row,col++) << rotation.SetValue( Rmax );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(maxConfig,p_table,row,col++,pDisplayUnits);
         }

         (*p_table)(row,col++) << rotation.SetValue( Rmin );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(minConfig,p_table,row,col++,pDisplayUnits);
         }
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
void CVehicularLoadReactionTable::MakeCopy(const CVehicularLoadReactionTable& rOther)
{
   // Add copy code here...
}

void CVehicularLoadReactionTable::MakeAssignment(const CVehicularLoadReactionTable& rOther)
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
bool CVehicularLoadReactionTable::AssertValid() const
{
   return true;
}

void CVehicularLoadReactionTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CVehicularLoadReactionTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CVehicularLoadReactionTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CVehicularLoadReactionTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CVehicularLoadReactionTable");

   TESTME_EPILOG("CVehicularLoadReactionTable");
}
#endif // _UNITTEST
