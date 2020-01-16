///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
   CUserDeflectionsTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CUserDeflectionsTable::CUserDeflectionsTable()
{
}

CUserDeflectionsTable::CUserDeflectionsTable(const CUserDeflectionsTable& rOther)
{
   MakeCopy(rOther);
}

CUserDeflectionsTable::~CUserDeflectionsTable()
{
}

//======================== OPERATORS  =======================================
CUserDeflectionsTable& CUserDeflectionsTable::operator= (const CUserDeflectionsTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CUserDeflectionsTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,IntervalIndexType intervalIdx,
                                              IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false );

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   location.IncludeSpanAndGirder(pDocType->IsPGSpliceDocument() || girderKey.groupIndex == ALL_GROUPS);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   CString strTitle;
   strTitle.Format(_T("Deflections due to User Defined Loads in Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
   rptRcTable* p_table = CreateUserLoadHeading<rptLengthUnitTag,unitmgtLengthData>(strTitle.GetBuffer(),false,analysisType,intervalIdx,pDisplayUnits,pDisplayUnits->GetDeflectionUnit());

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

      std::vector<Float64> minDC, maxDC;
      std::vector<Float64> minDW, maxDW;
      std::vector<Float64> minLLIM, maxLLIM;


      maxDC = pForces2->GetDeflection( intervalIdx, pgsTypes::pftUserDC, vPoi, maxBAT, rtCumulative, false );
      minDC = pForces2->GetDeflection( intervalIdx, pgsTypes::pftUserDC, vPoi, minBAT, rtCumulative, false );

      maxDW = pForces2->GetDeflection( intervalIdx, pgsTypes::pftUserDW, vPoi, maxBAT, rtCumulative, false );
      minDW = pForces2->GetDeflection( intervalIdx, pgsTypes::pftUserDW, vPoi, minBAT, rtCumulative, false );

      maxLLIM = pForces2->GetDeflection( intervalIdx, pgsTypes::pftUserLLIM, vPoi, maxBAT, rtCumulative, false );
      minLLIM = pForces2->GetDeflection( intervalIdx, pgsTypes::pftUserLLIM, vPoi, minBAT, rtCumulative, false );

      // Fill up the table
      IndexType index = 0;
      for (const pgsPointOfInterest& poi : vPoi)
      {
         ColumnIndexType col = 0;

         (*p_table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi );

         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col++) << deflection.SetValue( maxDC[index] );
            (*p_table)(row,col++) << deflection.SetValue( minDC[index] );
            (*p_table)(row,col++) << deflection.SetValue( maxDW[index] );
            (*p_table)(row,col++) << deflection.SetValue( minDW[index] );
            (*p_table)(row,col++) << deflection.SetValue( maxLLIM[index] );
            (*p_table)(row,col++) << deflection.SetValue( minLLIM[index] );
         }
         else
         {
            (*p_table)(row,col++) << deflection.SetValue( maxDC[index] );
            (*p_table)(row,col++) << deflection.SetValue( maxDW[index] );
            (*p_table)(row,col++) << deflection.SetValue( maxLLIM[index] );
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
void CUserDeflectionsTable::MakeCopy(const CUserDeflectionsTable& rOther)
{
   // Add copy code here...
}

void CUserDeflectionsTable::MakeAssignment(const CUserDeflectionsTable& rOther)
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
bool CUserDeflectionsTable::AssertValid() const
{
   return true;
}

void CUserDeflectionsTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CUserDeflectionsTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CUserDeflectionsTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CUserDeflectionsTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CUserDeflectionsTable");

   TESTME_EPILOG("CUserDeflectionsTable");
}
#endif // _UNITTEST
