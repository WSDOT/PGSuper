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
#include <Reporting\UserShearTable.h>
#include <Reporting\UserMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>

#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\ReportOptions.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CUserShearTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CUserShearTable::CUserShearTable()
{
}

CUserShearTable::CUserShearTable(const CUserShearTable& rOther)
{
   MakeCopy(rOther);
}

CUserShearTable::~CUserShearTable()
{
}

//======================== OPERATORS  =======================================
CUserShearTable& CUserShearTable::operator= (const CUserShearTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CUserShearTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,IntervalIndexType intervalIdx,
                                      IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear, pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(girderKey));

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   CString strTitle;
   strTitle.Format(_T("Shears due to User Defined Loads in Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx).c_str());
   rptRcTable* p_table = CreateUserLoadHeading<rptForceUnitTag,WBFL::Units::ForceData>(strTitle.GetBuffer(),false,analysisType,intervalIdx,pDisplayUnits,pDisplayUnits->GetShearUnit());

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);
   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IProductForces,pForces);
   pgsTypes::BridgeAnalysisType maxBAT = pForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for(const auto& thisGirderKey : vGirderKeys)
   {
      PoiList vPoi;
      pIPoi->GetPointsOfInterest(CSegmentKey(thisGirderKey, ALL_SEGMENTS), POI_ERECTED_SEGMENT, &vPoi);

      std::vector<WBFL::System::SectionValue> minDC, maxDC;
      std::vector<WBFL::System::SectionValue> minDW, maxDW;
      std::vector<WBFL::System::SectionValue> minLLIM, maxLLIM;


      maxDC = pForces2->GetShear( intervalIdx, pgsTypes::pftUserDC, vPoi, maxBAT, rtIncremental );
      minDC = pForces2->GetShear( intervalIdx, pgsTypes::pftUserDC, vPoi, minBAT, rtIncremental );

      maxDW = pForces2->GetShear( intervalIdx, pgsTypes::pftUserDW, vPoi, maxBAT, rtIncremental );
      minDW = pForces2->GetShear( intervalIdx, pgsTypes::pftUserDW, vPoi, minBAT, rtIncremental );

      maxLLIM = pForces2->GetShear( intervalIdx, pgsTypes::pftUserLLIM, vPoi, maxBAT, rtIncremental );
      minLLIM = pForces2->GetShear( intervalIdx, pgsTypes::pftUserLLIM, vPoi, minBAT, rtIncremental );

      // Fill up the table
      IndexType index = 0;
      for (const pgsPointOfInterest& poi : vPoi)
      {
         ColumnIndexType col = 0;

         (*p_table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi );

         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col++) << shear.SetValue( maxDC[index] );
            (*p_table)(row,col++) << shear.SetValue( minDC[index] );
            (*p_table)(row,col++) << shear.SetValue( maxDW[index] );
            (*p_table)(row,col++) << shear.SetValue( minDW[index] );
            (*p_table)(row,col++) << shear.SetValue( maxLLIM[index] );
            (*p_table)(row,col++) << shear.SetValue( minLLIM[index] );
         }
         else
         {
            (*p_table)(row,col++) << shear.SetValue( maxDC[index] );
            (*p_table)(row,col++) << shear.SetValue( maxDW[index] );
            (*p_table)(row,col++) << shear.SetValue( maxLLIM[index] );
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
void CUserShearTable::MakeCopy(const CUserShearTable& rOther)
{
   // Add copy code here...
}

void CUserShearTable::MakeAssignment(const CUserShearTable& rOther)
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
