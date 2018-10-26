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
#include <Reporting\StrandEccTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>

#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CStrandEccTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CStrandEccTable::CStrandEccTable()
{
}

CStrandEccTable::CStrandEccTable(const CStrandEccTable& rOther)
{
   MakeCopy(rOther);
}

CStrandEccTable::~CStrandEccTable()
{
}

//======================== OPERATORS  =======================================
CStrandEccTable& CStrandEccTable::operator= (const CStrandEccTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CStrandEccTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                   IDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   bool bTempStrands = (0 < pStrandGeom->GetMaxStrands(span,girder,pgsTypes::Temporary) ? true : false);

   // Setup table
   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(bTempStrands ? 9 : 7,"Strand Eccentricity");

   p_table->SetNumberOfHeaderRows(2);

   int col = 0;

   // build first heading row
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   // straight/harped/temporary
   p_table->SetColumnSpan(0,col, (bTempStrands ? 5 : 3));
   (*p_table)(0,col++) << "Eccentricity";

   // strand slope
   p_table->SetColumnSpan(0,col, 2);
   (*p_table)(0,col++) << "Strand Slope";

   ColumnIndexType i;
   for ( i = col; i < p_table->GetNumberOfColumns(); i++ )
      p_table->SetColumnSpan(0,i,-1);

   // build second hearing row
   col = 0;
   p_table->SetRowSpan(1,col++,-1);
   p_table->SetRowSpan(1,col++,-1);

   (*p_table)(1,col++) << COLHDR("Straight", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(1,col++) << COLHDR("Harped",   rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   if ( bTempStrands )
   {
      (*p_table)(1,col++) << COLHDR("Temporary", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*p_table)(1,col++) << COLHDR("All" << rptNewLine << "(w/ Temp)", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*p_table)(1,col++) << COLHDR("Permanent" << rptNewLine << "(w/o Temp)", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }
   else
   {
      (*p_table)(1,col++) << COLHDR("All" << rptNewLine << "Strands", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }

   (*p_table)(1,col++) << "Average" << rptNewLine << "(1:n)";
   (*p_table)(1,col++) << "Maximum" << rptNewLine << "(1:n)";

   INIT_UV_PROTOTYPE( rptPointOfInterest, gdrloc, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptPointOfInterest, spanloc, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, ecc,    pDisplayUnits->GetComponentDimUnit(),  false );

   gdrloc.MakeGirderPoi();
   spanloc.MakeSpanPoi();

   GET_IFACE2( pBroker, IPointOfInterest, pPoi );
   GET_IFACE2(pBroker,IBridge,pBridge);

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);

   StrandIndexType Ns, Nh, Nt;
   Ns = pStrandGeom->GetNumStrands(span,girder,pgsTypes::Straight);
   Nh = pStrandGeom->GetNumStrands(span,girder,pgsTypes::Harped);
   Nt = pStrandGeom->GetNumStrands(span,girder,pgsTypes::Temporary);

   // Get all the tabular poi's for flexure and shear
   // Merge the two vectors to form one vector to report on.
   std::vector<pgsPointOfInterest> poi[6];
   poi[0] = pPoi->GetPointsOfInterest(pgsTypes::BridgeSite1,span,girder,POI_FLEXURESTRESS   | POI_TABULAR);
   poi[1] = pPoi->GetPointsOfInterest(pgsTypes::BridgeSite1,span,girder,POI_FLEXURECAPACITY | POI_TABULAR);
   poi[2] = pPoi->GetPointsOfInterest(pgsTypes::BridgeSite1,span,girder,POI_SHEAR           | POI_TABULAR);
   
   poi[3] = pPoi->GetPointsOfInterest(pgsTypes::CastingYard,span,girder,POI_FLEXURESTRESS   | POI_TABULAR);
   poi[4] = pPoi->GetPointsOfInterest(pgsTypes::CastingYard,span,girder,POI_PICKPOINT);
   poi[5] = pPoi->GetPointsOfInterest(pgsTypes::CastingYard,span,girder,POI_BUNKPOINT);

   std::set< std::pair<pgsPointOfInterest,pgsTypes::Stage> > pois; // use a set to eliminate duplicates
   std::set< std::pair<pgsPointOfInterest,pgsTypes::Stage> >::iterator iter;

   for ( i = 0; i < 6; i++ )
   {
      pgsTypes::Stage stage = ( i < 3 ? pgsTypes::BridgeSite1 : pgsTypes::CastingYard );
      for ( std::vector<pgsPointOfInterest>::iterator k = poi[i].begin(); k != poi[i].end(); k++ )
      {
         pois.insert( std::make_pair(*k,stage) );
      }
   }

   pgsPointOfInterest prev_poi(0,0,0);
   bool bSkipToNextRow = false;

   RowIndexType firstRow = p_table->GetNumberOfHeaderRows();
   RowIndexType row = firstRow;
   for ( iter = pois.begin(); iter != pois.end(); iter++ )
   {
      bSkipToNextRow = false;

      const pgsPointOfInterest& poi = (*iter).first;
      pgsTypes::Stage stage = (*iter).second;

      col = 0;

      if ( row != firstRow && IsEqual(poi.GetDistFromStart(),prev_poi.GetDistFromStart()) )
      {
         row--;
         bSkipToNextRow = true;
      }

      if ( stage == pgsTypes::CastingYard )
      {
         (*p_table)(row,col++) << gdrloc.SetValue( poi);
         (*p_table)(row,col++) << "";
      }
      else
      {
         (*p_table)(row,col++) << "";
         (*p_table)(row,col++) << spanloc.SetValue( poi, end_size );
      }

      if ( !bSkipToNextRow )
      {
         Float64 nEff;
         if ( 0 < Ns )
            (*p_table)(row,col++) << ecc.SetValue( pStrandGeom->GetSsEccentricity( poi, &nEff ) );
         else
            (*p_table)(row,col++) << "-";

         if ( 0 < Nh )
            (*p_table)(row,col++) << ecc.SetValue( pStrandGeom->GetHsEccentricity( poi, &nEff ) );
         else
            (*p_table)(row,col++) << "-";

         if ( bTempStrands )
         {
            if ( 0 < Nt )
               (*p_table)(row,col++) << ecc.SetValue( pStrandGeom->GetTempEccentricity( poi, &nEff ) );
            else
               (*p_table)(row,col++) << "-";

            (*p_table)(row,col++) << ecc.SetValue( pStrandGeom->GetEccentricity( poi, true, &nEff ) );
         }

         (*p_table)(row,col++) << ecc.SetValue( pStrandGeom->GetEccentricity( poi, false, &nEff ) );

         if ( 0 < Nh )
         {
            Float64 avg_slope = pStrandGeom->GetAvgStrandSlope( poi );
            if ( IsZero( 1./avg_slope ) )
               (*p_table)(row,col++) << symbol(INFINITY);
            else
               (*p_table)(row,col++) << avg_slope;

            Float64 max_slope = pStrandGeom->GetMaxStrandSlope( poi );
            if ( IsZero( 1./max_slope ) )
               (*p_table)(row,col++) << symbol(INFINITY);
            else
               (*p_table)(row,col++) << max_slope;
         }
         else
         {
            (*p_table)(row,col++) << "-";
            (*p_table)(row,col++) << "-";
         }
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
void CStrandEccTable::MakeCopy(const CStrandEccTable& rOther)
{
   // Add copy code here...
}

void CStrandEccTable::MakeAssignment(const CStrandEccTable& rOther)
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
bool CStrandEccTable::AssertValid() const
{
   return true;
}

void CStrandEccTable::Dump(dbgDumpContext& os) const
{
   os << "Dump for CStrandEccTable" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CStrandEccTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CStrandEccTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CStrandEccTable");

   TESTME_EPILOG("StrandEccTable");
}
#endif // _UNITTEST
