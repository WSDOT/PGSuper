///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   CString strTitle;
   strTitle.Format(_T("Shears due to User Defined Loads in Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(CGirderKey(girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex,girderKey.girderIndex),intervalIdx));
   rptRcTable* p_table = CreateUserLoadHeading<rptForceUnitTag,unitmgtForceData>(strTitle.GetBuffer(),false,analysisType,intervalIdx,pDisplayUnits,pDisplayUnits->GetShearUnit());

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);
   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IProductForces,pForces);
   pgsTypes::BridgeAnalysisType maxBAT = pForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = (nGirders <= girderKey.girderIndex ? nGirders-1 : girderKey.girderIndex);

      std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(CSegmentKey(grpIdx,gdrIdx,ALL_SEGMENTS),POI_ERECTED_SEGMENT) );

      std::vector<sysSectionValue> minDC, maxDC;
      std::vector<sysSectionValue> minDW, maxDW;
      std::vector<sysSectionValue> minLLIM, maxLLIM;


      maxDC = pForces2->GetShear( intervalIdx, pftUserDC, vPoi, maxBAT, rtIncremental );
      minDC = pForces2->GetShear( intervalIdx, pftUserDC, vPoi, minBAT, rtIncremental );

      maxDW = pForces2->GetShear( intervalIdx, pftUserDW, vPoi, maxBAT, rtIncremental );
      minDW = pForces2->GetShear( intervalIdx, pftUserDW, vPoi, minBAT, rtIncremental );

      maxLLIM = pForces2->GetShear( intervalIdx, pftUserLLIM, vPoi, maxBAT, rtIncremental );
      minLLIM = pForces2->GetShear( intervalIdx, pftUserLLIM, vPoi, minBAT, rtIncremental );

      // Fill up the table
      IndexType index = 0;
      std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      for ( ; i != end; i++, index++ )
      {
         ColumnIndexType col = 0;
         const pgsPointOfInterest& poi = *i;

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

//======================== DEBUG      =======================================
#if defined _DEBUG
bool CUserShearTable::AssertValid() const
{
   return true;
}

void CUserShearTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CUserShearTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CUserShearTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CUserShearTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CUserShearTable");

   TESTME_EPILOG("CUserShearTable");
}
#endif // _UNITTEST
