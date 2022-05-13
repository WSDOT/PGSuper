///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include <Reporting\UserMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>

#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CUserMomentsTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CUserMomentsTable::CUserMomentsTable()
{
}

CUserMomentsTable::CUserMomentsTable(const CUserMomentsTable& rOther)
{
   MakeCopy(rOther);
}

CUserMomentsTable::~CUserMomentsTable()
{
}

//======================== OPERATORS  =======================================
CUserMomentsTable& CUserMomentsTable::operator= (const CUserMomentsTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CUserMomentsTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,IntervalIndexType intervalIdx,
                                      IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment, pDisplayUnits->GetMomentUnit(), false );

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   CString strTitle;
   strTitle.Format(_T("Moment due to User Defined Loads in Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
   rptRcTable* p_table = CreateUserLoadHeading<rptMomentUnitTag,WBFL::Units::MomentData>(strTitle.GetBuffer(),false,analysisType,intervalIdx,pDisplayUnits,pDisplayUnits->GetMomentUnit());

   if (girderKey.groupIndex == ALL_GROUPS)
   {
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   location.IncludeSpanAndGirder(pDocType->IsPGSpliceDocument() || girderKey.groupIndex == ALL_GROUPS);
   PoiAttributeType poiRefAttribute(girderKey.groupIndex == ALL_GROUPS ? POI_SPAN : POI_ERECTED_SEGMENT);

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
      CSegmentKey allSegmentsKey(thisGirderKey, ALL_SEGMENTS);
      PoiList vPoi;
      pIPoi->GetPointsOfInterest(allSegmentsKey, poiRefAttribute, &vPoi);

      // Add PSXFER poi's (but only at the ends... don't need them all from debonding)
      PoiList vPoi2;
      pIPoi->GetPointsOfInterest(allSegmentsKey, POI_PSXFER, &vPoi2);
      if (0 < vPoi2.size())
      {
         vPoi.push_back(vPoi2.front());
         vPoi.push_back(vPoi2.back());
         pIPoi->SortPoiList(&vPoi);
      }

      std::vector<Float64> minDC, maxDC;
      std::vector<Float64> minDW, maxDW;
      std::vector<Float64> minLLIM, maxLLIM;


      maxDC = pForces2->GetMoment( intervalIdx, pgsTypes::pftUserDC, vPoi, maxBAT, rtIncremental );
      minDC = pForces2->GetMoment( intervalIdx, pgsTypes::pftUserDC, vPoi, minBAT, rtIncremental );

      maxDW = pForces2->GetMoment( intervalIdx, pgsTypes::pftUserDW, vPoi, maxBAT, rtIncremental );
      minDW = pForces2->GetMoment( intervalIdx, pgsTypes::pftUserDW, vPoi, minBAT, rtIncremental );

      maxLLIM = pForces2->GetMoment( intervalIdx, pgsTypes::pftUserLLIM, vPoi, maxBAT, rtIncremental );
      minLLIM = pForces2->GetMoment( intervalIdx, pgsTypes::pftUserLLIM, vPoi, minBAT, rtIncremental );

      // Fill up the table
      IndexType index = 0;
      for (const pgsPointOfInterest& poi : vPoi)
      {
         ColumnIndexType col = 0;

         (*p_table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi );

         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col++) << moment.SetValue( maxDC[index] );
            (*p_table)(row,col++) << moment.SetValue( minDC[index] );
            (*p_table)(row,col++) << moment.SetValue( maxDW[index] );
            (*p_table)(row,col++) << moment.SetValue( minDW[index] );
            (*p_table)(row,col++) << moment.SetValue( maxLLIM[index] );
            (*p_table)(row,col++) << moment.SetValue( minLLIM[index] );
         }
         else
         {
            (*p_table)(row,col++) << moment.SetValue( maxDC[index] );
            (*p_table)(row,col++) << moment.SetValue( maxDW[index] );
            (*p_table)(row,col++) << moment.SetValue( maxLLIM[index] );
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
void CUserMomentsTable::MakeCopy(const CUserMomentsTable& rOther)
{
   // Add copy code here...
}

void CUserMomentsTable::MakeAssignment(const CUserMomentsTable& rOther)
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
bool CUserMomentsTable::AssertValid() const
{
   return true;
}

void CUserMomentsTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CUserMomentsTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CUserMomentsTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CUserMomentsTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CUserMomentsTable");

   TESTME_EPILOG("CUserMomentsTable");
}
#endif // _UNITTEST
