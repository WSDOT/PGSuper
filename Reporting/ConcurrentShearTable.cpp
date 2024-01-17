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
#include <Reporting\ConcurrentShearTable.h>
#include <Reporting\MVRChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>

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
   CConcurrentShearTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CConcurrentShearTable::CConcurrentShearTable()
{
}

CConcurrentShearTable::CConcurrentShearTable(const CConcurrentShearTable& rOther)
{
   MakeCopy(rOther);
}

CConcurrentShearTable::~CConcurrentShearTable()
{
}

//======================== OPERATORS  =======================================
CConcurrentShearTable& CConcurrentShearTable::operator= (const CConcurrentShearTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CConcurrentShearTable::Build(IBroker* pBroker,rptChapter* pChapter,
                                       const CGirderKey& girderKey,
                                       IEAFDisplayUnits* pDisplayUnits,
                                       IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetGeneralForceUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment, pDisplayUnits->GetMomentUnit(), false );

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(girderKey));

   GET_IFACE2(pBroker,IBridge,pBridge);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = 0;

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);
 
   ColumnIndexType col = 0;

   p_table = rptStyleManager::CreateDefaultTable(3,_T("Concurrent Shears"));

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,col++) << COLHDR(Sub2(_T("M"),_T("max")) << rptNewLine << _T("Strength I"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,col++) << COLHDR(Sub2(_T("V"),_T("i")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );

   *p << p_table;

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);

   pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);

   // Fill up the table
   WBFL::System::SectionValue Vmin, Vmax;
   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);

      PoiList vPoi;
      pPoi->GetPointsOfInterest(CSegmentKey(thisGirderKey, ALL_SEGMENTS), POI_ERECTED_SEGMENT, &vPoi);
      for (const pgsPointOfInterest& poi : vPoi)
      {
         col = 0;

         (*p_table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi );

         Float64 Vi, Mmax;
         pLsForces->GetViMmax(intervalIdx,pgsTypes::StrengthI,poi,bat,&Vi,&Mmax);

         (*p_table)(row,col++) << moment.SetValue( Mmax );
         (*p_table)(row,col++) << shear.SetValue(  Vi );

         row++;
      }

   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CConcurrentShearTable::MakeCopy(const CConcurrentShearTable& rOther)
{
   // Add copy code here...
}

void CConcurrentShearTable::MakeAssignment(const CConcurrentShearTable& rOther)
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

