///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
rptRcTable* CStrandEccTable::Build(IBroker* pBroker, const CSegmentKey& segmentKey, IntervalIndexType intervalIdx, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker, IBridge, pBridge);
   if (pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing())
   {
      return Build_XY(pBroker, segmentKey, intervalIdx, pDisplayUnits);
   }
   else
   {
      return Build_Y(pBroker, segmentKey, intervalIdx, pDisplayUnits);
   }
}

rptRcTable* CStrandEccTable::Build_Y(IBroker* pBroker, const CSegmentKey& segmentKey, IntervalIndexType intervalIdx, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker, ISectionProperties, pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();
   pgsTypes::SectionPropertyType spType = (spMode == pgsTypes::spmGross ? pgsTypes::sptGrossNoncomposite : pgsTypes::sptNetGirder);

   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP)
   {
      // need eccentricity based on complete transformed section for time-step analysis
      spType = pgsTypes::sptTransformed;
   }

   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeom);
   bool bTempStrands = (0 < pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Temporary) ? true : false);

   // Setup table
   GET_IFACE2(pBroker, IIntervals, pIntervals);
   std::_tostringstream os;
   os << _T("Strand Eccentricity: Interval ") << LABEL_INTERVAL(intervalIdx) << _T(" ") << pIntervals->GetDescription(intervalIdx);

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(bTempStrands ? 9 : 7, os.str().c_str());

   p_table->SetNumberOfHeaderRows(2);

   ColumnIndexType col = 0;

   // build first heading row
   p_table->SetRowSpan(0, col, 2);
   (*p_table)(0, col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   p_table->SetRowSpan(0, col, 2);
   (*p_table)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   // straight/harped/temporary
   p_table->SetColumnSpan(0, col, (bTempStrands ? 5 : 3));
   (*p_table)(0, col) << _T("Eccentricity");
   if (spMode == pgsTypes::spmGross)
   {
      (*p_table)(1, col++) << COLHDR(_T("Straight") << rptNewLine << Sub2(_T("e"), _T("s")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*p_table)(1, col++) << COLHDR(LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey)) << rptNewLine << Sub2(_T("e"), _T("h")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      if (bTempStrands)
      {
         (*p_table)(1, col++) << COLHDR(_T("Temporary") << rptNewLine << Sub2(_T("e"), _T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*p_table)(1, col++) << COLHDR(_T("All") << rptNewLine << _T("(w/ Temp), ") << Sub2(_T("e"), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*p_table)(1, col++) << COLHDR(_T("Permanent") << rptNewLine << _T("(w/o Temp), ") << Sub2(_T("e"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      else
      {
         (*p_table)(1, col++) << COLHDR(_T("All Strands") << rptNewLine << Sub2(_T("e"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
   }
   else
   {
      (*p_table)(1, col++) << COLHDR(_T("Straight") << rptNewLine << Sub2(_T("e"), _T("st")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*p_table)(1, col++) << COLHDR(LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey)) << rptNewLine << Sub2(_T("e"), _T("ht")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      if (bTempStrands)
      {
         (*p_table)(1, col++) << COLHDR(_T("Temporary") << rptNewLine << Sub2(_T("e"), _T("tt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*p_table)(1, col++) << COLHDR(_T("All") << rptNewLine << _T("(w/ Temp), ") << Sub2(_T("e"), _T("pst")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*p_table)(1, col++) << COLHDR(_T("Permanent") << rptNewLine << _T("(w/o Temp), ") << Sub2(_T("e"), _T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      else
      {
         (*p_table)(1, col++) << COLHDR(_T("All") << rptNewLine << _T("Strands, ") << Sub2(_T("e"), _T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
   }

   // strand slope
   p_table->SetColumnSpan(0, col, 2);
   (*p_table)(0, col) << _T("Strand Slope");
   (*p_table)(1, col++) << _T("Average") << rptNewLine << _T("(1:n)");
   (*p_table)(1, col++) << _T("Maximum") << rptNewLine << _T("(1:n)");

   INIT_UV_PROTOTYPE(rptPointOfInterest, rptReleasePoi, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptPointOfInterest, rptErectedPoi, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthSectionValue, ecc, pDisplayUnits->GetComponentDimUnit(), false);

   GET_IFACE2(pBroker, IPointOfInterest, pPoi);

   StrandIndexType Ns, Nh, Nt;
   Ns = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Straight);
   Nh = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Harped);
   Nt = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Temporary);

   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_RELEASED_SEGMENT, &vPoi);
   pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &vPoi);
   pPoi->GetPointsOfInterest(segmentKey, POI_SPECIAL | POI_START_FACE | POI_END_FACE, &vPoi);
   pPoi->SortPoiList(&vPoi);
   pPoi->RemovePointsOfInterest(vPoi, POI_CLOSURE);
   pPoi->RemovePointsOfInterest(vPoi, POI_BOUNDARY_PIER);

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      (*p_table)(row, col++) << rptReleasePoi.SetValue(POI_RELEASED_SEGMENT, poi);
      (*p_table)(row, col++) << rptErectedPoi.SetValue(POI_ERECTED_SEGMENT, poi);

      if (0 < Ns)
      {
         (*p_table)(row, col++) << ecc.SetValue(pStrandGeom->GetEccentricity(spType, intervalIdx, poi, pgsTypes::Straight).Y());
      }
      else
      {
         (*p_table)(row, col++) << _T("-");
      }

      if (0 < Nh)
      {
         (*p_table)(row, col++) << ecc.SetValue(pStrandGeom->GetEccentricity(spType, intervalIdx, poi, pgsTypes::Harped).Y());
      }
      else
      {
         (*p_table)(row, col++) << _T("-");
      }

      if (bTempStrands)
      {
         if (0 < Nt)
         {
            (*p_table)(row, col++) << ecc.SetValue(pStrandGeom->GetEccentricity(spType, intervalIdx, poi, pgsTypes::Temporary).Y());
         }
         else
         {
            (*p_table)(row, col++) << _T("-");
         }

         (*p_table)(row, col++) << ecc.SetValue(pStrandGeom->GetEccentricity(spType, intervalIdx, poi, true /*include temporary strands*/).Y());
      }

      (*p_table)(row, col++) << ecc.SetValue(pStrandGeom->GetEccentricity(spType, intervalIdx, poi, false/*exclude temporary strands*/).Y());

      if (0 < Nh)
      {
         Float64 avg_slope = pStrandGeom->GetAvgStrandSlope(poi);
         avg_slope = fabs(avg_slope);

         if (IsZero(1. / avg_slope))
         {
            (*p_table)(row, col++) << symbol(infinity);
         }
         else
         {
            (*p_table)(row, col++) << avg_slope;
         }

         Float64 max_slope = pStrandGeom->GetMaxStrandSlope(poi);
         max_slope = fabs(max_slope);
         if (IsZero(1. / max_slope))
         {
            (*p_table)(row, col++) << symbol(infinity);
         }
         else
         {
            (*p_table)(row, col++) << max_slope;
         }
      }
      else
      {
         (*p_table)(row, col++) << _T("-");
         (*p_table)(row, col++) << _T("-");
      }

      row++;
   }

   return p_table;
}

rptRcTable* CStrandEccTable::Build_XY(IBroker* pBroker, const CSegmentKey& segmentKey, IntervalIndexType intervalIdx,
      IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();
   pgsTypes::SectionPropertyType spType = (spMode == pgsTypes::spmGross ? pgsTypes::sptGrossNoncomposite : pgsTypes::sptNetGirder);

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

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(bTempStrands ? 14 : 10, os.str().c_str() );

   p_table->SetNumberOfHeaderRows(3);

   ColumnIndexType col = 0;

   // build first heading row
   p_table->SetRowSpan(0,col,3);
   (*p_table)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   p_table->SetRowSpan(0,col,3);
   (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   // straight/harped/temporary
   p_table->SetColumnSpan(0,col, (bTempStrands ? 10 : 6));
   (*p_table)(0,col) << _T("Eccentricity");

   p_table->SetColumnSpan(1, col, 2);
   (*p_table)(1,col) << COLHDR(_T("Straight"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   if (spMode == pgsTypes::spmGross)
   {
      (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("sx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit()); // straight
      (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("sy")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }
   else
   {
      (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("sxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit()); // straight
      (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("syt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }

   p_table->SetColumnSpan(1, col, 2);
   (*p_table)(1,col) << COLHDR(LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey)),   rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   if (spMode == pgsTypes::spmGross)
   {
      (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("hx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit()); // straight
      (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("hy")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }
   else
   {
      (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("hxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit()); // straight
      (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("hyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }

   if ( bTempStrands )
   {
      p_table->SetColumnSpan(1, col, 2);
      (*p_table)(1,col) << COLHDR(_T("Temporary"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      if (spMode == pgsTypes::spmGross)
      {
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("tx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit()); // straight
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("ty")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      else
      {
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("txt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit()); // straight
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("tyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }

      p_table->SetColumnSpan(1, col, 2);
      (*p_table)(1,col) << COLHDR(_T("All") << rptNewLine << _T("(w/ Temp)"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      if (spMode == pgsTypes::spmGross)
      {
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("psx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit()); // all strands with temp
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("psy")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      else
      {
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("psxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit()); // all strands with temp
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("psyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }

      p_table->SetColumnSpan(1, col, 2);
      (*p_table)(1,col) << COLHDR(_T("Permanent") << rptNewLine << _T("(w/o Temp)"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      if (spMode == pgsTypes::spmGross)
      {
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("px")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit()); // all strands with temp
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("py")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      else
      {
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("pxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit()); // all strands with temp
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("pyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
   }
   else
   {
      p_table->SetColumnSpan(1, col, 2);
      (*p_table)(1,col) << COLHDR(_T("All") << rptNewLine << _T("Strands"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      if (spMode == pgsTypes::spmGross)
      {
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("px")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit()); // all strands with temp
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("py")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      else
      {
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("pxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit()); // all strands with temp
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("pyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
   }


   // strand slope
   p_table->SetColumnSpan(0, col, 2);
   (*p_table)(0, col) << _T("Strand Slope");

   p_table->SetRowSpan(1, col, 2);
   (*p_table)(1,col++) << _T("Average") << rptNewLine << _T("(1:n)");

   p_table->SetRowSpan(1, col, 2);
   (*p_table)(1,col++) << _T("Maximum") << rptNewLine << _T("(1:n)");

   INIT_UV_PROTOTYPE( rptPointOfInterest, rptReleasePoi, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptPointOfInterest, rptErectedPoi, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, eccentricity,    pDisplayUnits->GetComponentDimUnit(),  false );

   GET_IFACE2( pBroker, IPointOfInterest, pPoi );

   StrandIndexType Ns = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Straight);
   StrandIndexType Nh = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped);
   StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);

   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_RELEASED_SEGMENT, &vPoi); 
   pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &vPoi); // new pois are appended to vPoi
   pPoi->GetPointsOfInterest(segmentKey, POI_SPECIAL | POI_START_FACE | POI_END_FACE, &vPoi);
   pPoi->SortPoiList(&vPoi); // sorts and removes duplicates
   pPoi->RemovePointsOfInterest(vPoi,POI_CLOSURE);
   pPoi->RemovePointsOfInterest(vPoi,POI_BOUNDARY_PIER);

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      (*p_table)(row,col++) << rptReleasePoi.SetValue(POI_RELEASED_SEGMENT,poi);
      (*p_table)(row,col++) << rptErectedPoi.SetValue(POI_ERECTED_SEGMENT, poi);

      if ( 0 < Ns )
      {
         gpPoint2d ecc = pStrandGeom->GetEccentricity(spType, intervalIdx, poi, pgsTypes::Straight);
         (*p_table)(row, col++) << eccentricity.SetValue(ecc.X());
         (*p_table)(row, col++) << eccentricity.SetValue(ecc.Y());
      }
      else
      {
         (*p_table)(row, col++) << _T("-");
         (*p_table)(row, col++) << _T("-");
      }

      if ( 0 < Nh )
      {
         gpPoint2d ecc = pStrandGeom->GetEccentricity(spType, intervalIdx, poi, pgsTypes::Harped);
         (*p_table)(row, col++) << eccentricity.SetValue(ecc.X());
         (*p_table)(row, col++) << eccentricity.SetValue(ecc.Y());
      }
      else
      {
         (*p_table)(row, col++) << _T("-");
         (*p_table)(row, col++) << _T("-");
      }

      if ( bTempStrands )
      {
         if ( 0 < Nt )
         {
            gpPoint2d ecc = pStrandGeom->GetEccentricity(spType, intervalIdx, poi, pgsTypes::Temporary);
            (*p_table)(row, col++) << eccentricity.SetValue(ecc.X());
            (*p_table)(row, col++) << eccentricity.SetValue(ecc.Y());
         }
         else
         {
            (*p_table)(row, col++) << _T("-");
            (*p_table)(row, col++) << _T("-");
         }

         gpPoint2d ecc = pStrandGeom->GetEccentricity(spType, intervalIdx, poi, true /*include temporary strands*/);
         (*p_table)(row, col++) << eccentricity.SetValue(ecc.X());
         (*p_table)(row, col++) << eccentricity.SetValue(ecc.Y());
      }

      gpPoint2d ecc = pStrandGeom->GetEccentricity(spType, intervalIdx, poi, false/*exclude temporary strands*/);
      (*p_table)(row, col++) << eccentricity.SetValue(ecc.X());
      (*p_table)(row, col++) << eccentricity.SetValue(ecc.Y());

      if ( 0 < Nh )
      {
         Float64 avg_slope = pStrandGeom->GetAvgStrandSlope( poi );
         avg_slope = fabs(avg_slope);

         if ( IsZero( 1./avg_slope ) )
         {
            (*p_table)(row,col++) << symbol(infinity);
         }
         else
         {
            (*p_table)(row,col++) << avg_slope;
         }

         Float64 max_slope = pStrandGeom->GetMaxStrandSlope( poi );
         max_slope = fabs(max_slope);
         if ( IsZero( 1./max_slope ) )
         {
            (*p_table)(row,col++) << symbol(infinity);
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
