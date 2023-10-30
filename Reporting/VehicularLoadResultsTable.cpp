///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include <PgsExt\ReportPointOfInterest.h>

#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>
#include <IFace\ReportOptions.h>

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
rptRcTable* CVehicularLoadResultsTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::LiveLoadType llType,const std::_tstring& strLLName,VehicleIndexType vehicleIdx, pgsTypes::AnalysisType analysisType,
                                              bool bReportTruckConfig,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment, pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, axial, pDisplayUnits->GetGeneralForceUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, span_location, pDisplayUnits->GetSpanLengthUnit(), false );

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(girderKey));

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = (IsRatingLiveLoad(llType) ? pIntervals->GetLoadRatingInterval() : pIntervals->GetLiveLoadInterval() );

   GET_IFACE2(pBroker, IBridge, pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey.girderIndex, startGroupIdx, endGroupIdx, &vGirderKeys);

   Float64 end_distance = pBridge->GetSegmentStartEndDistance(CSegmentKey(vGirderKeys.front(),0));
 
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bReportAxial = pProductLoads->ReportAxialResults();

   ColumnIndexType nCols = 7;
   if ( bReportAxial )
   {
      nCols += 2;
   }

   if ( bReportTruckConfig )
   {
      nCols += 7;
      if ( bReportAxial )
      {
         nCols += 2;
      }
   }

   std::_tstring title(_T("Live Load Results for ") + strLLName);
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,title.c_str());

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Set up table headings
   ColumnIndexType col = 0;
   (*p_table)(0,col++) << _T("");

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << COLHDR(_T("X"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   }

   if ( bReportAxial )
   {
      (*p_table)(0,col++) << COLHDR(_T("Axial") << rptNewLine << _T("Max"),   rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );

      if ( bReportTruckConfig )
      {
         (*p_table)(0,col++) << _T("Axial") << rptNewLine << _T("Max") << rptNewLine << _T("Config"); // for max axial
      }

      (*p_table)(0,col++) << COLHDR(_T("Axial") << rptNewLine << _T("Min"),   rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );

      if ( bReportTruckConfig )
      {
         (*p_table)(0,col++) << _T("Axial") << rptNewLine << _T("Min") << rptNewLine << _T("Config"); // for max moment
      }
   }

   (*p_table)(0,col++) << COLHDR(_T("Moment") << rptNewLine << _T("Max"),   rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << _T("Moment") << rptNewLine << _T("Max") << rptNewLine << _T("Config"); // for max moment
   }

   (*p_table)(0,col++) << COLHDR(_T("Moment") << rptNewLine << _T("Min"),   rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << _T("Moment") << rptNewLine << _T("Min") << rptNewLine << _T("Config"); // for max moment
   }

   (*p_table)(0,col++) << COLHDR(_T("Shear") << rptNewLine << _T("Max"),   rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << _T("Shear") << rptNewLine << _T("Max") << rptNewLine << _T("Config"); // for max moment
   }

   (*p_table)(0,col++) << COLHDR(_T("Shear") << rptNewLine << _T("Min"),   rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << _T("Shear") << rptNewLine << _T("Min") << rptNewLine << _T("Config"); // for max moment
   }
   
   (*p_table)(0,col++) << COLHDR(_T("Deflection") << rptNewLine << _T("Max"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << _T("Deflection") << rptNewLine << _T("Max") << rptNewLine << _T("Config"); // for max moment
   }

   (*p_table)(0,col++) << COLHDR(_T("Deflection") << rptNewLine << _T("Min"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << _T("Deflection") << rptNewLine << _T("Min") << rptNewLine << _T("Config"); // for max moment
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for(const auto& thisGirderKey : vGirderKeys)
   {
      PoiList vPoi;
      pIPoi->GetPointsOfInterest(CSegmentKey(thisGirderKey, ALL_SEGMENTS), POI_SPAN, &vPoi);

      PoiList csPois;
      pIPoi->GetCriticalSections(pgsTypes::StrengthI, thisGirderKey, &csPois);
      pIPoi->MergePoiLists(vPoi, csPois, &vPoi);

      std::vector<Float64> dummy;
      std::vector<Float64> Pmin, Pmax;
      std::vector<Float64> Mmin, Mmax;
      std::vector<WBFL::System::SectionValue> Vmin, Vmax, Vdummy;
      std::vector<Float64> Dmin, Dmax;

      std::vector<AxleConfiguration> dummyConfig, 
         PminConfig, PmaxConfig, 
         MminConfig, MmaxConfig, 
         VminLeftConfig, VmaxLeftConfig, VminRightConfig, VmaxRightConfig, 
         DminConfig, DmaxConfig;

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bReportAxial )
         {
            pForces2->GetVehicularLiveLoadAxial( intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &dummy, &Pmax, bReportTruckConfig ? &dummyConfig  : nullptr, bReportTruckConfig ? &PmaxConfig   : nullptr );
            pForces2->GetVehicularLiveLoadAxial( intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Pmin, &dummy, bReportTruckConfig ? &PminConfig   : nullptr, bReportTruckConfig ? &dummyConfig  : nullptr  );
         }

         pForces2->GetVehicularLiveLoadMoment( intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &dummy, &Mmax, bReportTruckConfig ? &dummyConfig  : nullptr, bReportTruckConfig ? &MmaxConfig   : nullptr );
         pForces2->GetVehicularLiveLoadMoment( intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Mmin, &dummy, bReportTruckConfig ? &MminConfig   : nullptr, bReportTruckConfig ? &dummyConfig  : nullptr  );

         pForces2->GetVehicularLiveLoadShear( intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Vdummy, &Vmax, 
            bReportTruckConfig ? &dummyConfig  : nullptr, 
            bReportTruckConfig ? &dummyConfig  : nullptr, 
            bReportTruckConfig ? &VmaxLeftConfig   : nullptr,
            bReportTruckConfig ? &VmaxRightConfig  : nullptr );

         pForces2->GetVehicularLiveLoadShear( intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Vmin, &Vdummy,
            bReportTruckConfig ? &VminLeftConfig   : nullptr, 
            bReportTruckConfig ? &VminRightConfig  : nullptr, 
            bReportTruckConfig ? &dummyConfig  : nullptr,
            bReportTruckConfig ? &dummyConfig  : nullptr  );

         pForces2->GetVehicularLiveLoadDeflection( intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &dummy, &Dmax, bReportTruckConfig ? &dummyConfig  : nullptr, bReportTruckConfig ? &DmaxConfig   : nullptr );
         pForces2->GetVehicularLiveLoadDeflection( intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Dmin, &dummy, bReportTruckConfig ? &DminConfig   : nullptr, bReportTruckConfig ? &dummyConfig  : nullptr );
      }
      else
      {
         if ( bReportAxial )
         {
            pForces2->GetVehicularLiveLoadAxial( intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Pmin, &Pmax, bReportTruckConfig ? &PminConfig : nullptr, bReportTruckConfig ? &PmaxConfig : nullptr );
         }

         pForces2->GetVehicularLiveLoadMoment( intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Mmin, &Mmax, bReportTruckConfig ? &MminConfig : nullptr, bReportTruckConfig ? &MmaxConfig : nullptr );
         pForces2->GetVehicularLiveLoadShear( intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Vmin, &Vmax, 
                                              bReportTruckConfig ? &VminLeftConfig  : nullptr, 
                                              bReportTruckConfig ? &VminRightConfig : nullptr, 
                                              bReportTruckConfig ? &VmaxLeftConfig  : nullptr,
                                              bReportTruckConfig ? &VmaxRightConfig : nullptr );
         pForces2->GetVehicularLiveLoadDeflection( intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Dmin, &Dmax, bReportTruckConfig ? &DminConfig : nullptr, bReportTruckConfig ? &DmaxConfig : nullptr );
      }

      // Fill up the table
      IndexType index = 0;
      for(const pgsPointOfInterest& poi : vPoi)
      {
         col = 0;

         (*p_table)(row,col++) << location.SetValue( POI_SPAN, poi );

         if ( bReportTruckConfig )
         {
            Float64 Xgl = pIPoi->ConvertPoiToGirderlineCoordinate(poi); // measured from start face of first segment... we want measured from CL Brg
            (*p_table)(row,col++) << span_location.SetValue(Xgl - end_distance); // deduct start end distance to get measured from CL Brg
         }

         if ( bReportAxial )
         {
            (*p_table)(row,col++) << axial.SetValue( Pmax[index] );

            if ( bReportTruckConfig )
            {
               ReportTruckConfiguration(PmaxConfig[index],p_table,row,col++,pDisplayUnits);
            }

            (*p_table)(row,col++) << axial.SetValue( Pmin[index] );

            if ( bReportTruckConfig )
            {
               ReportTruckConfiguration(PminConfig[index],p_table,row,col++,pDisplayUnits);
            }
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
               (*p_table)(row,col) << _T("-----------") << rptNewLine;
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
               (*p_table)(row,col) << _T("-----------") << rptNewLine;
               ReportTruckConfiguration(VminRightConfig[index],p_table,row,col,pDisplayUnits);
            }

            col++;
         }

         (*p_table)(row,col++) << deflection.SetValue( Dmax[index] );
         if ( bReportTruckConfig )
         {
            ReportTruckConfiguration(DmaxConfig[index],p_table,row,col++,pDisplayUnits);
         }

         (*p_table)(row,col++) << deflection.SetValue( Dmin[index] );
         if ( bReportTruckConfig )
         {
            ReportTruckConfiguration(DminConfig[index],p_table,row,col++,pDisplayUnits);
         }

         row++;
         index++;
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
void CVehicularLoadResultsTable::MakeCopy(const CVehicularLoadResultsTable& rOther)
{
   // Add copy code here...
}

void CVehicularLoadResultsTable::MakeAssignment(const CVehicularLoadResultsTable& rOther)
{
   MakeCopy( rOther );
}

void CVehicularLoadResultsTable::ReportTruckConfiguration(const AxleConfiguration& config,rptRcTable* pTable,RowIndexType row,ColumnIndexType col,IEAFDisplayUnits* pDisplayUnits)
{
   if ( config.size() == 0 )
   {
      (*pTable)(row,col) << _T("") ;
      return;
   }

   INIT_UV_PROTOTYPE( rptForceUnitValue, weight, pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, position, pDisplayUnits->GetSpanLengthUnit(), true);

   int nRowsWritten = 0;
   for(const auto& axlePlacement : config)
   {
      if ( 0 < axlePlacement.Weight )
      {
         (*pTable)(row,col) << weight.SetValue(axlePlacement.Weight) << _T(" @ ") << position.SetValue(axlePlacement.Location) << rptNewLine;
         nRowsWritten++;
      }
   }

   if ( nRowsWritten == 0 )
   {
      (*pTable)(row,col) << _T("") ;
      return;
   }
}
