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
#include <Reporting\PrestressStressTable.h>
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
   CPrestressStressTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CPrestressStressTable::CPrestressStressTable()
{
}

CPrestressStressTable::CPrestressStressTable(const CPrestressStressTable& rOther)
{
   MakeCopy(rOther);
}

CPrestressStressTable::~CPrestressStressTable()
{
}

//======================== OPERATORS  =======================================
CPrestressStressTable& CPrestressStressTable::operator= (const CPrestressStressTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CPrestressStressTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                            IDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, gdrpoi, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptPointOfInterest, spanpoi, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   gdrpoi.MakeGirderPoi();
   spanpoi.MakeSpanPoi();

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   long NtMax = pStrandGeom->GetMaxStrands(span,girder,pgsTypes::Temporary);

   int nColumns = ( 0 < NtMax ? 7 : 6 );
   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nColumns,"Prestress Stresses");

   // Set up table headings
   int col = 0;
   (*p_table)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,col++) << COLHDR("Casting Yard",       rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   
   if ( 0 < NtMax )
      (*p_table)(0,col++) << COLHDR("Temporary" << rptNewLine << "Strand" << rptNewLine << "Removal",      rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   (*p_table)(0,col++) << COLHDR("Bridge Site 1",      rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,col++) << COLHDR("Bridge Site 2",      rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,col++) << COLHDR("Bridge Site 3",      rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi1 = pIPoi->GetPointsOfInterest( pgsTypes::CastingYard, span, girder, POI_FLEXURESTRESS | POI_TABULAR );
   std::vector<pgsPointOfInterest> vPoi2 = pIPoi->GetPointsOfInterest( pgsTypes::BridgeSite1, span, girder, POI_FLEXURESTRESS | POI_TABULAR );
   
   std::set< std::pair<pgsPointOfInterest,pgsTypes::Stage> > pois; // use a set to eliminate duplicates

   std::vector<pgsPointOfInterest>::iterator k;
   for ( k = vPoi1.begin(); k != vPoi1.end(); k++ )
   {
      const pgsPointOfInterest& poi = *k;
      pois.insert( std::make_pair(poi,pgsTypes::CastingYard) );
   }
   for ( k = vPoi2.begin(); k != vPoi2.end(); k++ )
   {
      const pgsPointOfInterest& poi = *k;
      pois.insert( std::make_pair(poi,pgsTypes::BridgeSite1) );
   }

   GET_IFACE2(pBroker,IPrestressStresses,pPrestress);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);

   // Fill up the table
   pgsPointOfInterest prev_poi(0,0,0);
   bool bSkipToNextRow = false;

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   std::set< std::pair<pgsPointOfInterest,pgsTypes::Stage> >::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      bSkipToNextRow = false;
      col = 0;

      pgsTypes::Stage stage = (*i).second;
      const pgsPointOfInterest& poi = (*i).first;

      if ( row != 1 && IsEqual(poi.GetDistFromStart(),prev_poi.GetDistFromStart()) )
      {
         bSkipToNextRow = true;
         row--;
      }

      if ( stage == pgsTypes::CastingYard )
      {
         (*p_table)(row,col++) << gdrpoi.SetValue( poi );
         (*p_table)(row,col++) << "";
      }
      else
      {
         (*p_table)(row,col++) << "";
         (*p_table)(row,col++) << spanpoi.SetValue( poi, end_size  );
      }

      if ( !bSkipToNextRow )
      {
         double fTop, fBot;
         fTop = pPrestress->GetStress(pgsTypes::CastingYard,poi,pgsTypes::TopGirder);
         fBot = pPrestress->GetStress(pgsTypes::CastingYard,poi,pgsTypes::BottomGirder);
         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue( fTop ) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue( fBot );
         col++;

         if ( 0 < NtMax )
         {
            fTop = pPrestress->GetStress(pgsTypes::TemporaryStrandRemoval,poi,pgsTypes::TopGirder);
            fBot = pPrestress->GetStress(pgsTypes::TemporaryStrandRemoval,poi,pgsTypes::BottomGirder);
            (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue( fTop ) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue( fBot );
            col++;
         }

         fTop = pPrestress->GetStress(pgsTypes::BridgeSite1,poi,pgsTypes::TopGirder);
         fBot = pPrestress->GetStress(pgsTypes::BridgeSite1,poi,pgsTypes::BottomGirder);
         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue( fTop ) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue( fBot );
         col++;

         fTop = pPrestress->GetStress(pgsTypes::BridgeSite2,poi,pgsTypes::TopGirder);
         fBot = pPrestress->GetStress(pgsTypes::BridgeSite2,poi,pgsTypes::BottomGirder);
         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue( fTop ) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue( fBot );
         col++;

         fTop = pPrestress->GetStress(pgsTypes::BridgeSite3,poi,pgsTypes::TopGirder);
         fBot = pPrestress->GetStress(pgsTypes::BridgeSite3,poi,pgsTypes::BottomGirder);
         (*p_table)(row,col) << RPT_FTOP << " = " << stress.SetValue( fTop ) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << " = " << stress.SetValue( fBot );
         col++;
      }

      row++;
      prev_poi = poi;
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CPrestressStressTable::MakeCopy(const CPrestressStressTable& rOther)
{
   // Add copy code here...
}

void CPrestressStressTable::MakeAssignment(const CPrestressStressTable& rOther)
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

