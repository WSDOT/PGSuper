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
#include <Reporting\CastingYardMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>

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
rptRcTable* CCastingYardMomentsTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                            IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(span == ALL_SPANS);
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment, pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear, pDisplayUnits->GetShearUnit(), false );

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(3,_T("Casting Yard"));
   p_table->SetNumberOfHeaderRows(2);

   if ( span == ALL_SPANS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Set up table headings
   p_table->SetRowSpan(0,0,2);
   p_table->SetRowSpan(1,0,-1);
   (*p_table)(0,0) << COLHDR(RPT_GDR_END_LOCATION,        rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   
   p_table->SetColumnSpan(0,1,2);
   (*p_table)(0,1) << _T("Girder");
   p_table->SetColumnSpan(0,2,-1);

   (*p_table)(1,1) << COLHDR(_T("Moment"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(1,2) << COLHDR(_T("Shear"),  rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( span, girder, pgsTypes::CastingYard, POI_FLEXURESTRESS | POI_TABULAR );

   GET_IFACE2(pBroker,IProductForces,pForces);

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      (*p_table)(row,0) << location.SetValue( pgsTypes::CastingYard, poi );
      (*p_table)(row,1) << moment.SetValue( pForces->GetMoment( pgsTypes::CastingYard, pftGirder, poi, SimpleSpan ) );
      (*p_table)(row,2) << shear.SetValue( pForces->GetShear( pgsTypes::CastingYard, pftGirder, poi, SimpleSpan ) );

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

void CCastingYardMomentsTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CCastingYardMomentsTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CCastingYardMomentsTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CCastingYardMomentsTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CCastingYardMomentsTable");

   TESTME_EPILOG("CCastingYardMomentsTable");
}
#endif // _UNITTEST
