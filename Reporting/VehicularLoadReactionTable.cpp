///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>

#include <PgsExt\PierData2.h>
#include <PgsExt\GirderGroupData.h>

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
rptRcTable* CVehicularLoadReactionTable::Build(IBroker* pBroker,const CGirderKey& girderKey,
                                               pgsTypes::LiveLoadType llType,
                                               const std::_tstring& strLLName,
                                               VehicleIndexType vehicleIdx, 
                                               pgsTypes::AnalysisType analysisType,
                                               bool bReportTruckConfig,
                                               bool bIncludeRotations,
                                               IEAFDisplayUnits* pDisplayUnits) const

{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  reaction, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,  rotation, pDisplayUnits->GetRadAngleUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   bool bPermit = false;

   PierIndexType nPiers = pBridge->GetPierCount();
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);
   PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(startGroupIdx);
   PierIndexType endPierIdx   = pBridge->GetGirderGroupEndPier(  endGroupIdx);

   ColumnIndexType nCols = 3;

   if ( bReportTruckConfig )
   {
      nCols += 4;
   }


   if ( bIncludeRotations )
   {
      nCols += 2;
   }

   std::_tstring strTitle;
   if ( bIncludeRotations )
   {
      strTitle = _T("Live Load Reactions and Rotations for ") + strLLName;
   }
   else
   {
      strTitle = _T("Live Load Reactions for ") + strLLName;
   }

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,strTitle);

   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));


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

   if ( bIncludeRotations )
   {
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
   }

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);

   // Get POI at start and end of the span
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   PoiList vPoi;
   for(const auto& thisGirderKey : vGirderKeys)
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(thisGirderKey,segIdx);
         PoiList vSegPoi;
         pPOI->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | POI_ERECTED_SEGMENT, &vSegPoi);
         ATLASSERT(vSegPoi.size() == 2);
         vPoi.insert(vPoi.end(), vSegPoi.begin(), vSegPoi.end());
      }
   }

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IReactions,pReactions);

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( PierIndexType pier = startPierIdx; pier <= endPierIdx; pier++ )
   {
      CGirderKey thisGirderKey(girderKey);
      const CPierData2* pPier = pIBridgeDesc->GetPier(pier);
      if ( girderKey.groupIndex == ALL_GROUPS )
      {
         if ( pier < endPierIdx )
         {
            thisGirderKey.groupIndex = pPier->GetNextGirderGroup()->GetIndex();
         }
         else
         {
            thisGirderKey.groupIndex = pPier->GetPrevGirderGroup()->GetIndex();
         }
      }

      IntervalIndexType intervalIdx = (IsRatingLiveLoad(llType) ? pIntervals->GetLoadRatingInterval() : pIntervals->GetLiveLoadInterval() );

      col = 0;
      const pgsPointOfInterest& poi = vPoi[pier-startPierIdx];

      (*p_table)(row,col++) << LABEL_PIER_EX(pPier->IsAbutment(), pier);

      if ( analysisType == pgsTypes::Envelope )
      {
         REACTION Rmin, Rmax;
         AxleConfiguration maxConfig, minConfig;
         pReactions->GetVehicularLiveLoadReaction( intervalIdx, llType, vehicleIdx, pier, thisGirderKey, pgsTypes::MaxSimpleContinuousEnvelope,  true, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*p_table)(row,col++) << reaction.SetValue( Rmax.Fy );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(maxConfig,p_table,row,col++,pDisplayUnits);
         }

         pReactions->GetVehicularLiveLoadReaction( intervalIdx, llType, vehicleIdx, pier, thisGirderKey, pgsTypes::MinSimpleContinuousEnvelope,  true, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*p_table)(row,col++) << reaction.SetValue( Rmin.Fy );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(minConfig,p_table,row,col++,pDisplayUnits);
         }

         if ( bIncludeRotations )
         {
            GET_IFACE2(pBroker,IProductForces,pForces);
   
            Float64 RotMax, RotMin;
            pForces->GetVehicularLiveLoadRotation( intervalIdx, llType, vehicleIdx, poi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &RotMin, &RotMax, &minConfig, &maxConfig );
            (*p_table)(row,col++) << rotation.SetValue( RotMax );
   
            if ( bReportTruckConfig )
            {
               CVehicularLoadResultsTable::ReportTruckConfiguration(maxConfig,p_table,row,col++,pDisplayUnits);
            }
   
            pForces->GetVehicularLiveLoadRotation( intervalIdx, llType, vehicleIdx, poi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &RotMin, &RotMax, &minConfig, &maxConfig );
            (*p_table)(row,col++) << rotation.SetValue( RotMin );
   
            if ( bReportTruckConfig )
            {
               CVehicularLoadResultsTable::ReportTruckConfiguration(minConfig,p_table,row,col++,pDisplayUnits);
            }
         }
      }
      else
      {
         REACTION Rmin, Rmax;
         AxleConfiguration maxConfig, minConfig;
         pReactions->GetVehicularLiveLoadReaction( intervalIdx, llType, vehicleIdx, pier, thisGirderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,  true, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*p_table)(row,col++) << reaction.SetValue( Rmax.Fy );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(maxConfig,p_table,row,col++,pDisplayUnits);
         }

         (*p_table)(row,col++) << reaction.SetValue( Rmin.Fy );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(minConfig,p_table,row,col++,pDisplayUnits);
         }

         if ( bIncludeRotations )
         {
            GET_IFACE2(pBroker,IProductForces,pForces);

            Float64 RotMin, RotMax;

            pForces->GetVehicularLiveLoadRotation( intervalIdx, llType, vehicleIdx, poi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &RotMin, &RotMax, &minConfig, &maxConfig );
            (*p_table)(row,col++) << rotation.SetValue( RotMax );
   
            if ( bReportTruckConfig )
            {
               CVehicularLoadResultsTable::ReportTruckConfiguration(maxConfig,p_table,row,col++,pDisplayUnits);
            }
   
            (*p_table)(row,col++) << rotation.SetValue( RotMin );
   
            if ( bReportTruckConfig )
            {
               CVehicularLoadResultsTable::ReportTruckConfiguration(minConfig,p_table,row,col++,pDisplayUnits);
   
            }
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
