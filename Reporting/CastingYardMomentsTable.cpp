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
#include <Reporting\CastingYardMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\TimelineEvent.h>

#include <IFace\Project.h>
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
   CCastingYardMomentsTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCastingYardMomentsTable::CCastingYardMomentsTable()
{
}

CCastingYardMomentsTable::CCastingYardMomentsTable(const CCastingYardMomentsTable& rOther)
{
   MakeCopy(rOther);
}

CCastingYardMomentsTable::~CCastingYardMomentsTable()
{
}

//======================== OPERATORS  =======================================
CCastingYardMomentsTable& CCastingYardMomentsTable::operator= (const CCastingYardMomentsTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CCastingYardMomentsTable::Build(IBroker* pBroker,const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,PoiAttributeType poiRefAttribute,LPCTSTR strTableTitle,
                                            IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment, pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, deflection, pDisplayUnits->GetDeflectionUnit(), false );

   location.IncludeSpanAndGirder(segmentKey.groupIndex == ALL_GROUPS ? true : false);

   GET_IFACE2(pBroker, IBridge, pBridge);
   bool bReportXDeflection = pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing();

   ColumnIndexType nColumns = 4;
   if (bReportXDeflection)
   {
      nColumns++;
   }

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nColumns,strTableTitle);

   if (segmentKey.groupIndex == ALL_GROUPS)
   {
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Set up table headings
   ColumnIndexType col = 0;
   (*p_table)(0, col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0, col++) << COLHDR(_T("Moment"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0, col++) << COLHDR(_T("Shear"),  rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
   if (bReportXDeflection)
   {
      (*p_table)(0, col++) << COLHDR(_T("Deflection X"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
      (*p_table)(0, col++) << COLHDR(_T("Deflection Y"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }
   else
   {
      (*p_table)(0, col++) << COLHDR(_T("Deflection"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(segmentKey, poiRefAttribute, &vPoi);
   PoiList vPoi2;
   pIPoi->GetPointsOfInterest(segmentKey, POI_START_FACE | POI_END_FACE | POI_HARPINGPOINT | POI_PSXFER | POI_DEBOND, &vPoi2, POIFIND_OR);
   pIPoi->MergePoiLists(vPoi, vPoi2, &vPoi);
   pIPoi->RemovePointsOfInterest(vPoi,POI_CLOSURE);
   pIPoi->RemovePointsOfInterest(vPoi,POI_BOUNDARY_PIER);

   GET_IFACE2(pBroker,IProductForces,pProductForces);

   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for (const auto& poi : vPoi)
   {
      col = 0;

      (*p_table)(row, col++) << location.SetValue( poiRefAttribute, poi );
      (*p_table)(row, col++) << moment.SetValue( pProductForces->GetMoment( intervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative ) );
      (*p_table)(row, col++) << shear.SetValue(  pProductForces->GetShear(  intervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative ) );
      
      if (bReportXDeflection)
      {
         (*p_table)(row, col++) << deflection.SetValue(pProductForces->GetXDeflection(intervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative));
      }
      
      (*p_table)(row, col++) << deflection.SetValue(pProductForces->GetDeflection(intervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative, false));

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
void CCastingYardMomentsTable::MakeCopy(const CCastingYardMomentsTable& rOther)
{
   // Add copy code here...
}

void CCastingYardMomentsTable::MakeAssignment(const CCastingYardMomentsTable& rOther)
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
bool CCastingYardMomentsTable::AssertValid() const
{
   return true;
}

void CCastingYardMomentsTable::Dump(WBFL::Debug::LogContext& os) const
{
   os << _T("Dump for CCastingYardMomentsTable") << WBFL::Debug::endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CCastingYardMomentsTable::TestMe(WBFL::Debug::Log& rlog)
{
   TESTME_PROLOGUE("CCastingYardMomentsTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CCastingYardMomentsTable");

   TESTME_EPILOG("CCastingYardMomentsTable");
}
#endif // _UNITTEST
