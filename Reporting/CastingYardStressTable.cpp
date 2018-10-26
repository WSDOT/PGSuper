///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#include <Reporting\CastingYardStressTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>

#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
#include <IFace\AnalysisResults.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CCastingYardStressTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCastingYardStressTable::CCastingYardStressTable()
{
}

CCastingYardStressTable::CCastingYardStressTable(const CCastingYardStressTable& rOther)
{
   MakeCopy(rOther);
}

CCastingYardStressTable::~CCastingYardStressTable()
{
}

//======================== OPERATORS  =======================================
CCastingYardStressTable& CCastingYardStressTable::operator= (const CCastingYardStressTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CCastingYardStressTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                            IDisplayUnits* pDispUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDispUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDispUnits->GetStressUnit(), false );

   location.MakeGirderPoi();

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(3,"Casting Yard Stresses");

   // Set up table headings
   (*p_table)(0,0) << COLHDR(RPT_GDR_END_LOCATION,                  rptLengthUnitTag, pDispUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR(RPT_FTOP << rptNewLine << "Girder",    rptStressUnitTag, pDispUnits->GetStressUnit() );
   (*p_table)(0,2) << COLHDR(RPT_FBOT << rptNewLine << "Girder",    rptStressUnitTag, pDispUnits->GetStressUnit() );

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( pgsTypes::CastingYard, span, girder, POI_FLEXURESTRESS | POI_TABULAR );

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   GET_IFACE2(pBroker,IPrestressStresses,pPrestress);

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      (*p_table)(row,0) << location.SetValue( poi );

      double fTop, fBot;
      pProductForces->GetStress(pgsTypes::CastingYard,pftGirder, poi, SimpleSpan, &fTop, &fBot);
      (*p_table)(row,1) << stress.SetValue( fTop );
      (*p_table)(row,2) << stress.SetValue( fBot );

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
void CCastingYardStressTable::MakeCopy(const CCastingYardStressTable& rOther)
{
   // Add copy code here...
}

void CCastingYardStressTable::MakeAssignment(const CCastingYardStressTable& rOther)
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

