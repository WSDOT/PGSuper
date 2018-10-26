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
#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>

#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
#include <IFace\AnalysisResults.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CVehicularLoadResultsTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CVehicularLoadResultsTable::CVehicularLoadResultsTable()
{
}

CVehicularLoadResultsTable::CVehicularLoadResultsTable(const CVehicularLoadResultsTable& rOther)
{
   MakeCopy(rOther);
}

CVehicularLoadResultsTable::~CVehicularLoadResultsTable()
{
}

//======================== OPERATORS  =======================================
CVehicularLoadResultsTable& CVehicularLoadResultsTable::operator= (const CVehicularLoadResultsTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CVehicularLoadResultsTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,pgsTypes::LiveLoadType llType,const std::string& strLLName,VehicleIndexType vehicleIndex, pgsTypes::AnalysisType analysisType,
                                              bool bReportTruckConfig,IDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment, pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, displacement, pDisplayUnits->GetDisplacementUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, span_location, pDisplayUnits->GetSpanLengthUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);

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

   ColumnIndexType nCols = 7;

   if ( bReportTruckConfig )
      nCols += 7;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Live Load Results for " + strLLName);

   // Set up table headings
   ColumnIndexType col = 0;
   (*p_table)(0,col++) << "";

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << COLHDR("X",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   }

   (*p_table)(0,col++) << COLHDR("Moment" << rptNewLine << "Max",   rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << "Moment" << rptNewLine << "Max" << rptNewLine << "Config"; // for max moment
   }

   (*p_table)(0,col++) << COLHDR("Moment" << rptNewLine << "Min",   rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << "Moment" << rptNewLine << "Min" << rptNewLine << "Config"; // for max moment
   }

   (*p_table)(0,col++) << COLHDR("Shear" << rptNewLine << "Max",   rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << "Shear" << rptNewLine << "Max" << rptNewLine << "Config"; // for max moment
   }

   (*p_table)(0,col++) << COLHDR("Shear" << rptNewLine << "Min",   rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << "Shear" << rptNewLine << "Min" << rptNewLine << "Config"; // for max moment
   }
   
   (*p_table)(0,col++) << COLHDR("Displacement" << rptNewLine << "Max", rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << "Displacement" << rptNewLine << "Max" << rptNewLine << "Config"; // for max moment
   }

   (*p_table)(0,col++) << COLHDR("Displacement" << rptNewLine << "Min", rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << "Displacement" << rptNewLine << "Min" << rptNewLine << "Config"; // for max moment
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);

   double cumm_span_length = 0;
   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      // Get all the tabular poi's for flexure and shear
      // Merge the two vectors to form one vector to report on.
      std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(pgsTypes::BridgeSite3,spanIdx,girder, POI_ALLACTIONS | POI_TABULAR | POI_CRITSECTSHEAR1 | POI_CRITSECTSHEAR2, POIFIND_OR);

      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(girder,nGirders-1);

      Float64 end_size = pBridge->GetGirderStartConnectionLength(spanIdx,gdrIdx);

      std::vector<Float64> dummy;
      std::vector<Float64> Mmin, Mmax;
      std::vector<sysSectionValue> Vmin, Vmax, Vdummy;
      std::vector<Float64> Dmin, Dmax;

      std::vector<AxleConfiguration> dummyConfig, MminConfig, MmaxConfig, 
         VminLeftConfig, VmaxLeftConfig, VminRightConfig, VmaxRightConfig, 
         DminConfig, DmaxConfig;

      if ( analysisType == pgsTypes::Envelope )
      {
         pForces2->GetVehicularLiveLoadMoment( llType, vehicleIndex, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &Mmax, bReportTruckConfig ? &dummyConfig  : NULL, bReportTruckConfig ? &MmaxConfig   : NULL );
         pForces2->GetVehicularLiveLoadMoment( llType, vehicleIndex, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &Mmin, &dummy, bReportTruckConfig ? &MminConfig   : NULL, bReportTruckConfig ? &dummyConfig  : NULL  );

         pForces2->GetVehicularLiveLoadShear( llType, vehicleIndex, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &Vdummy, &Vmax, 
            bReportTruckConfig ? &dummyConfig  : NULL, 
            bReportTruckConfig ? &dummyConfig  : NULL, 
            bReportTruckConfig ? &VmaxLeftConfig   : NULL,
            bReportTruckConfig ? &VmaxRightConfig  : NULL );

         pForces2->GetVehicularLiveLoadShear( llType, vehicleIndex, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &Vmin, &Vdummy,
            bReportTruckConfig ? &VminLeftConfig   : NULL, 
            bReportTruckConfig ? &VminRightConfig  : NULL, 
            bReportTruckConfig ? &dummyConfig  : NULL,
            bReportTruckConfig ? &dummyConfig  : NULL  );

         pForces2->GetVehicularLiveLoadDisplacement( llType, vehicleIndex, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &Dmax, bReportTruckConfig ? &dummyConfig  : NULL, bReportTruckConfig ? &DmaxConfig   : NULL );
         pForces2->GetVehicularLiveLoadDisplacement( llType, vehicleIndex, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &Dmin, &dummy, bReportTruckConfig ? &DminConfig   : NULL, bReportTruckConfig ? &dummyConfig  : NULL );
      }
      else
      {
         pForces2->GetVehicularLiveLoadMoment( llType, vehicleIndex, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &Mmin, &Mmax, bReportTruckConfig ? &MminConfig : NULL, bReportTruckConfig ? &MmaxConfig : NULL );
         pForces2->GetVehicularLiveLoadShear( llType, vehicleIndex, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &Vmin, &Vmax, 
                                              bReportTruckConfig ? &VminLeftConfig  : NULL, 
                                              bReportTruckConfig ? &VminRightConfig : NULL, 
                                              bReportTruckConfig ? &VmaxLeftConfig  : NULL,
                                              bReportTruckConfig ? &VmaxRightConfig : NULL );
         pForces2->GetVehicularLiveLoadDisplacement( llType, vehicleIndex, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &Dmin, &Dmax, bReportTruckConfig ? &DminConfig : NULL, bReportTruckConfig ? &DmaxConfig : NULL );
      }

      // Fill up the table
      long index = 0;
      std::vector<pgsPointOfInterest>::const_iterator i;
      for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
      {
         const pgsPointOfInterest& poi = *i;

         col = 0;

         (*p_table)(row,col++) << location.SetValue( poi, end_size );

         if ( bReportTruckConfig )
         {
            (*p_table)(row,col++) << span_location.SetValue(cumm_span_length + poi.GetDistFromStart() - end_size);
         }

         (*p_table)(row,col++) << moment.SetValue( Mmax[index] );

         if ( bReportTruckConfig )
         {
            ReportTruckConfiguration(MmaxConfig[index],p_table,row,col++,pDisplayUnits);
         }

         (*p_table)(row,col++) << moment.SetValue( Mmin[index] );

         if ( bReportTruckConfig )
         {
            ReportTruckConfiguration(MminConfig[index],p_table,row,col++,pDisplayUnits);
         }

         (*p_table)(row,col++) << shear.SetValue( Vmax[index] );
         if ( bReportTruckConfig )
         {
            ReportTruckConfiguration(VmaxLeftConfig[index],p_table,row,col,pDisplayUnits);
            if ( !IsEqual(Vmax[index].Left(),Vmax[index].Right()) )
            {
               (*p_table)(row,col) << "-----------" << rptNewLine;
               ReportTruckConfiguration(VmaxRightConfig[index],p_table,row,col,pDisplayUnits);
            }

            col++;
         }

         (*p_table)(row,col++) << shear.SetValue( Vmin[index] );

         if ( bReportTruckConfig )
         {
            ReportTruckConfiguration(VminLeftConfig[index],p_table,row,col,pDisplayUnits);
            if ( !IsEqual(Vmin[index].Left(),Vmin[index].Right()) )
            {
               (*p_table)(row,col) << "-----------" << rptNewLine;
               ReportTruckConfiguration(VminRightConfig[index],p_table,row,col,pDisplayUnits);
            }

            col++;
         }

         (*p_table)(row,col++) << displacement.SetValue( Dmax[index] );
         if ( bReportTruckConfig )
         {
            ReportTruckConfiguration(DmaxConfig[index],p_table,row,col++,pDisplayUnits);
         }

         (*p_table)(row,col++) << displacement.SetValue( Dmin[index] );
         if ( bReportTruckConfig )
         {
            ReportTruckConfiguration(DminConfig[index],p_table,row,col++,pDisplayUnits);
         }

         row++;
      }

      double span_length = pBridge->GetSpanLength(spanIdx,gdrIdx);
      cumm_span_length += span_length;
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CVehicularLoadResultsTable::MakeCopy(const CVehicularLoadResultsTable& rOther)
{
   // Add copy code here...
}

void CVehicularLoadResultsTable::MakeAssignment(const CVehicularLoadResultsTable& rOther)
{
   MakeCopy( rOther );
}

void CVehicularLoadResultsTable::ReportTruckConfiguration(const AxleConfiguration& config,rptRcTable* pTable,int row,int col,IDisplayUnits* pDisplayUnits)
{
   if ( config.size() == 0 )
   {
      (*pTable)(row,col) << "" ;
      return;
   }

   INIT_UV_PROTOTYPE( rptForceUnitValue, weight, pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, position, pDisplayUnits->GetSpanLengthUnit(), true);

   int nRowsWritten = 0;
   AxleConfiguration::const_iterator axleIter;
   for ( axleIter = config.begin(); axleIter != config.end(); axleIter++ )
   {
      const AxlePlacement& axlePlacement = *axleIter;
      if ( 0 < axlePlacement.Weight )
      {
         (*pTable)(row,col) << weight.SetValue(axlePlacement.Weight) << " @ " << position.SetValue(axlePlacement.Location) << rptNewLine;
         nRowsWritten++;
      }
   }

   if ( nRowsWritten == 0 )
   {
      (*pTable)(row,col) << "" ;
      return;
   }
}
