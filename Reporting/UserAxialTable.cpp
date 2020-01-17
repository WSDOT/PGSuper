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
#include <Reporting\UserAxialTable.h>
#include <Reporting\UserMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>

#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CUserAxialTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CUserAxialTable::CUserAxialTable()
{
}

CUserAxialTable::CUserAxialTable(const CUserAxialTable& rOther)
{
   MakeCopy(rOther);
}

CUserAxialTable::~CUserAxialTable()
{
}

//======================== OPERATORS  =======================================
CUserAxialTable& CUserAxialTable::operator= (const CUserAxialTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CUserAxialTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,IntervalIndexType intervalIdx,
                                      IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, axial, pDisplayUnits->GetGeneralForceUnit(), false );

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   location.IncludeSpanAndGirder(pDocType->IsPGSpliceDocument() || girderKey.groupIndex == ALL_GROUPS);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   CString strTitle;
   strTitle.Format(_T("Axial due to User Defined Loads in Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
   rptRcTable* p_table = CreateUserLoadHeading<rptForceUnitTag,unitmgtForceData>(strTitle.GetBuffer(),false,analysisType,intervalIdx,pDisplayUnits,pDisplayUnits->GetGeneralForceUnit());

   if (girderKey.groupIndex == ALL_GROUPS)
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

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = (nGirders <= girderKey.girderIndex ? nGirders-1 : girderKey.girderIndex);

      PoiList vPoi;
      pIPoi->GetPointsOfInterest(CSegmentKey(grpIdx, gdrIdx, ALL_SEGMENTS), POI_ERECTED_SEGMENT, &vPoi);

      std::vector<Float64> minDC, maxDC;
      std::vector<Float64> minDW, maxDW;
      std::vector<Float64> minLLIM, maxLLIM;


      maxDC = pForces2->GetAxial( intervalIdx, pgsTypes::pftUserDC, vPoi, maxBAT, rtIncremental );
      minDC = pForces2->GetAxial( intervalIdx, pgsTypes::pftUserDC, vPoi, minBAT, rtIncremental );

      maxDW = pForces2->GetAxial( intervalIdx, pgsTypes::pftUserDW, vPoi, maxBAT, rtIncremental );
      minDW = pForces2->GetAxial( intervalIdx, pgsTypes::pftUserDW, vPoi, minBAT, rtIncremental );

      maxLLIM = pForces2->GetAxial( intervalIdx, pgsTypes::pftUserLLIM, vPoi, maxBAT, rtIncremental );
      minLLIM = pForces2->GetAxial( intervalIdx, pgsTypes::pftUserLLIM, vPoi, minBAT, rtIncremental );

      // Fill up the table
      IndexType index = 0;
      for (const pgsPointOfInterest& poi : vPoi)
      {
         ColumnIndexType col = 0;

         (*p_table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi );

         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col++) << axial.SetValue( maxDC[index] );
            (*p_table)(row,col++) << axial.SetValue( minDC[index] );
            (*p_table)(row,col++) << axial.SetValue( maxDW[index] );
            (*p_table)(row,col++) << axial.SetValue( minDW[index] );
            (*p_table)(row,col++) << axial.SetValue( maxLLIM[index] );
            (*p_table)(row,col++) << axial.SetValue( minLLIM[index] );
         }
         else
         {
            (*p_table)(row,col++) << axial.SetValue( maxDC[index] );
            (*p_table)(row,col++) << axial.SetValue( maxDW[index] );
            (*p_table)(row,col++) << axial.SetValue( maxLLIM[index] );
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
void CUserAxialTable::MakeCopy(const CUserAxialTable& rOther)
{
   // Add copy code here...
}

void CUserAxialTable::MakeAssignment(const CUserAxialTable& rOther)
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
bool CUserAxialTable::AssertValid() const
{
   return true;
}

void CUserAxialTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CUserAxialTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CUserAxialTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CUserAxialTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CUserAxialTable");

   TESTME_EPILOG("CUserAxialTable");
}
#endif // _UNITTEST
