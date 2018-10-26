///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include <PgsExt\ReportPointOfInterest.h>

#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Project.h>


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
rptRcTable* CStrandEccTable::Build(IBroker* pBroker,const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,
                                   IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyType spType = (pSectProp->GetSectionPropertiesMode() == pgsTypes::spmGross ? pgsTypes::sptGrossNoncomposite : pgsTypes::sptNetGirder );

   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      // need eccentricity based on complete transformed section for time-step analysis
      spType = pgsTypes::sptTransformed;
   }

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   bool bTempStrands = (0 < pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary) ? true : false);

   // Setup table
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   std::_tostringstream os;
   os << _T("Strand Eccentricity: Interval ") << LABEL_INTERVAL(intervalIdx) << _T(" ") << pIntervals->GetDescription(intervalIdx);

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(bTempStrands ? 9 : 7, os.str().c_str() );

   p_table->SetNumberOfHeaderRows(2);

   ColumnIndexType col = 0;

   // build first heading row
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   // straight/harped/temporary
   p_table->SetColumnSpan(0,col, (bTempStrands ? 5 : 3));
   (*p_table)(0,col++) << _T("Eccentricity");

   // strand slope
   p_table->SetColumnSpan(0,col, 2);
   (*p_table)(0,col++) << _T("Strand Slope");

   ColumnIndexType i;
   for ( i = col; i < p_table->GetNumberOfColumns(); i++ )
   {
      p_table->SetColumnSpan(0,i,SKIP_CELL);
   }

   // build second hearing row
   col = 0;
   p_table->SetRowSpan(1,col++,SKIP_CELL);
   p_table->SetRowSpan(1,col++,SKIP_CELL);

   (*p_table)(1,col++) << COLHDR(_T("Straight"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(1,col++) << COLHDR(LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey)),   rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   if ( bTempStrands )
   {
      (*p_table)(1,col++) << COLHDR(_T("Temporary"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*p_table)(1,col++) << COLHDR(_T("All") << rptNewLine << _T("(w/ Temp)"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*p_table)(1,col++) << COLHDR(_T("Permanent") << rptNewLine << _T("(w/o Temp)"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }
   else
   {
      (*p_table)(1,col++) << COLHDR(_T("All") << rptNewLine << _T("Strands"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }

   (*p_table)(1,col++) << _T("Average") << rptNewLine << _T("(1:n)");
   (*p_table)(1,col++) << _T("Maximum") << rptNewLine << _T("(1:n)");

   INIT_UV_PROTOTYPE( rptPointOfInterest, rptReleasePoi, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptPointOfInterest, rptErectedPoi, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, ecc,    pDisplayUnits->GetComponentDimUnit(),  false );

   GET_IFACE2( pBroker, IPointOfInterest, pPoi );

   StrandIndexType Ns, Nh, Nt;
   Ns = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Straight);
   Nh = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped);
   Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);

   std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(segmentKey,POI_RELEASED_SEGMENT));
   std::vector<pgsPointOfInterest> vPoi2(pPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT));
   vPoi.insert(vPoi.end(),vPoi2.begin(),vPoi2.end());
   std::vector<pgsPointOfInterest> vPoi3(pPoi->GetPointsOfInterest(segmentKey,POI_SPECIAL | POI_START_FACE | POI_END_FACE));
   vPoi.insert(vPoi.end(),vPoi3.begin(),vPoi3.end());
   std::sort(vPoi.begin(),vPoi.end());
   pPoi->RemovePointsOfInterest(vPoi,POI_CLOSURE);
   pPoi->RemovePointsOfInterest(vPoi,POI_BOUNDARY_PIER);
   vPoi.erase(std::unique(vPoi.begin(),vPoi.end()),vPoi.end());

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      col = 0;

      (*p_table)(row,col++) << rptReleasePoi.SetValue(POI_RELEASED_SEGMENT,poi);
      (*p_table)(row,col++) << rptErectedPoi.SetValue(POI_ERECTED_SEGMENT, poi);

      Float64 nEff;
      if ( 0 < Ns )
      {
         (*p_table)(row,col++) << ecc.SetValue( pStrandGeom->GetEccentricity( spType, intervalIdx, poi, pgsTypes::Straight, &nEff ) );
      }
      else
      {
         (*p_table)(row,col++) << _T("-");
      }

      if ( 0 < Nh )
      {
         (*p_table)(row,col++) << ecc.SetValue( pStrandGeom->GetEccentricity( spType, intervalIdx, poi, pgsTypes::Harped, &nEff ) );
      }
      else
      {
         (*p_table)(row,col++) << _T("-");
      }

      if ( bTempStrands )
      {
         if ( 0 < Nt )
         {
            (*p_table)(row,col++) << ecc.SetValue( pStrandGeom->GetEccentricity( spType, intervalIdx, poi, pgsTypes::Temporary, &nEff ) );
         }
         else
         {
            (*p_table)(row,col++) << _T("-");
         }

         (*p_table)(row,col++) << ecc.SetValue( pStrandGeom->GetEccentricity( spType, intervalIdx, poi, true /*include temporary strands*/, &nEff ) );
      }

      (*p_table)(row,col++) << ecc.SetValue( pStrandGeom->GetEccentricity( spType, intervalIdx, poi, false/*exclude temporary strands*/, &nEff ) );

      if ( 0 < Nh )
      {
         Float64 avg_slope = pStrandGeom->GetAvgStrandSlope( poi );
         avg_slope = fabs(avg_slope);

         if ( IsZero( 1./avg_slope ) )
         {
            (*p_table)(row,col++) << symbol(INFINITY);
         }
         else
         {
            (*p_table)(row,col++) << avg_slope;
         }

         Float64 max_slope = pStrandGeom->GetMaxStrandSlope( poi );
         max_slope = fabs(max_slope);
         if ( IsZero( 1./max_slope ) )
         {
            (*p_table)(row,col++) << symbol(INFINITY);
         }
         else
         {
            (*p_table)(row,col++) << max_slope;
         }
      }
      else
      {
         (*p_table)(row,col++) << _T("-");
         (*p_table)(row,col++) << _T("-");
      }

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
   os << _T("Dump for CStrandEccTable") << endl;
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
