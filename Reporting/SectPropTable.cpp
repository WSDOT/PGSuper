///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <Reporting\SectPropTable.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CSectionPropertiesTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSectionPropertiesTable::CSectionPropertiesTable()
{
}

CSectionPropertiesTable::CSectionPropertiesTable(const CSectionPropertiesTable& rOther)
{
   MakeCopy(rOther);
}

CSectionPropertiesTable::~CSectionPropertiesTable()
{
}

//======================== OPERATORS  =======================================
CSectionPropertiesTable& CSectionPropertiesTable::operator= (const CSectionPropertiesTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CSectionPropertiesTable::Build(IBroker* pBroker,const CSegmentKey& segmentKey,bool bComposite,
                                           IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType constructionIntervalIdx        = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;
   IntervalIndexType lastTendonStressingIntervalIdx = pIntervals->GetLastTendonStressingInterval(segmentKey);

#if defined _DEBUG
   {
      IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

      GET_IFACE2(pBroker,IGirder,pGirder);
      ATLASSERT( pGirder->IsPrismatic(erectSegmentIntervalIdx,segmentKey) == true );
      if ( bComposite )
      {
         ATLASSERT( pGirder->IsPrismatic(lastIntervalIdx,segmentKey) == true );
      }
   }
#endif // _DEBUG

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   GET_IFACE2(pBroker,IBridge,pBridge);

   bool bHasDeck = IsStructuralDeck(pBridge->GetDeckType());
   bool bAsymmetricGirders = pBridge->HasAsymmetricGirders();

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);

   ColumnIndexType nColumns = (bComposite ? 3 : 2);

   rptRcTable* xs_table = rptStyleManager::CreateDefaultTable(nColumns,_T("Section Properties"));

   (*xs_table)(0,0) << _T("");

   xs_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   xs_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   (*xs_table)(0,1).SetStyleName(rptStyleManager::GetTableColumnHeadingStyle() );
   (*xs_table)(0,1) << _T("Girder");

   if ( bComposite )
   {
      (*xs_table)(0,2).SetStyleName(rptStyleManager::GetTableColumnHeadingStyle() );
      (*xs_table)(0,2) << _T("Composite");
   }


   // Write labels
   
   INIT_UV_PROTOTYPE( rptAreaUnitValue, l2, pDisplayUnits->GetAreaUnit(), false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, l4, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   INIT_UV_PROTOTYPE( rptLength3UnitValue, l3, pDisplayUnits->GetSectModulusUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, l1, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, force_per_length, pDisplayUnits->GetForcePerLengthUnit(), false );

   // The section is prismatic so any poi will do
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPoi);
   ATLASSERT(vPoi.size() == 1);
   const pgsPointOfInterest& poi(vPoi.front());

    // Write non-composite properties
   RowIndexType row = xs_table->GetNumberOfHeaderRows();

   Float64 depth = pSectProp->GetHg(constructionIntervalIdx,poi);

   Float64 span_length = pBridge->GetSegmentSpanLength(segmentKey);

   (*xs_table)(row, 0) << _T("Area (") << rptAreaUnitTag(&pDisplayUnits->GetAreaUnit().UnitOfMeasure) << _T(")");
   (*xs_table)(row++,1) << l2.SetValue( pSectProp->GetAg(constructionIntervalIdx,poi) );

   (*xs_table)(row, 0) << _T("I") << Sub(_T("x")) << _T(" (") << rptLength4UnitTag(&pDisplayUnits->GetMomentOfInertiaUnit().UnitOfMeasure) << _T(")");
   (*xs_table)(row++,1) << l4.SetValue( pSectProp->GetIxx(constructionIntervalIdx,poi) );
   
   (*xs_table)(row, 0) << _T("I") << Sub(_T("y")) << _T(" (") << rptLength4UnitTag(&pDisplayUnits->GetMomentOfInertiaUnit().UnitOfMeasure) << _T(")");
   (*xs_table)(row++,1) << l4.SetValue( pSectProp->GetIyy(constructionIntervalIdx,poi) );

   if (bAsymmetricGirders)
   {
      Float64 Ixy = pSectProp->GetIxy(constructionIntervalIdx, poi);
      (*xs_table)(row, 0) << _T("I") << Sub(_T("xy")) << _T(" (") << rptLength4UnitTag(&pDisplayUnits->GetMomentOfInertiaUnit().UnitOfMeasure) << _T(")");
      (*xs_table)(row++, 1) << l4.SetValue(Ixy);
   }

   (*xs_table)(row, 0) << _T("d (girder depth)  (") << rptLengthUnitTag(&pDisplayUnits->GetComponentDimUnit().UnitOfMeasure) << _T(")");
   (*xs_table)(row++,1) << l1.SetValue( depth );

   if (bAsymmetricGirders)
   {
      (*xs_table)(row, 0) << RPT_XLEFT_GIRDER << _T(" (") << rptLengthUnitTag(&pDisplayUnits->GetComponentDimUnit().UnitOfMeasure) << _T(")");
      (*xs_table)(row++, 1) << l1.SetValue(pSectProp->GetXleft(constructionIntervalIdx, poi));

      (*xs_table)(row, 0) << RPT_XRIGHT_GIRDER << _T(" (") << rptLengthUnitTag(&pDisplayUnits->GetComponentDimUnit().UnitOfMeasure) << _T(")");
      (*xs_table)(row++, 1) << l1.SetValue(pSectProp->GetXright(constructionIntervalIdx, poi));
   }

   (*xs_table)(row, 0) << RPT_YTOP_GIRDER << _T(" (") << rptLengthUnitTag(&pDisplayUnits->GetComponentDimUnit().UnitOfMeasure) << _T(")");
   (*xs_table)(row++,1) << l1.SetValue( pSectProp->GetY(constructionIntervalIdx,poi,pgsTypes::TopGirder) );

   (*xs_table)(row, 0) << RPT_YBOT_GIRDER << _T(" (") << rptLengthUnitTag(&pDisplayUnits->GetComponentDimUnit().UnitOfMeasure) << _T(")");
   (*xs_table)(row++,1) << l1.SetValue( pSectProp->GetY(constructionIntervalIdx,poi,pgsTypes::BottomGirder) );

   if (bHasDeck)
   {
      (*xs_table)(row, 0) << RPT_YTOP_DECK << _T(" (") << rptLengthUnitTag(&pDisplayUnits->GetComponentDimUnit().UnitOfMeasure) << _T(")");
      (*xs_table)(row++,1) << _T("-"); // Ytd

      (*xs_table)(row, 0) << RPT_YBOT_DECK << _T(" (") << rptLengthUnitTag(&pDisplayUnits->GetComponentDimUnit().UnitOfMeasure) << _T(")");
      (*xs_table)(row++,1) << _T("-"); // Ybd
   }

   (*xs_table)(row, 0) << RPT_STOP_GIRDER << _T(" (") << rptLength3UnitTag(&pDisplayUnits->GetSectModulusUnit().UnitOfMeasure) << _T(")");
   (*xs_table)(row++,1) << l3.SetValue( -pSectProp->GetS(constructionIntervalIdx,poi,pgsTypes::TopGirder) );

   (*xs_table)(row, 0) << RPT_SBOT_GIRDER << _T(" (") << rptLength3UnitTag(&pDisplayUnits->GetSectModulusUnit().UnitOfMeasure) << _T(")");
   (*xs_table)(row++,1) << l3.SetValue( pSectProp->GetS(constructionIntervalIdx,poi,pgsTypes::BottomGirder) );

   if (bHasDeck)
   {
      (*xs_table)(row, 0) << RPT_STOP_DECK << _T(" = n(") << Sub2(_T("I"), _T("x")) << _T("/") << RPT_YTOP_DECK << _T(")") << _T(" (") << rptLength3UnitTag(&pDisplayUnits->GetSectModulusUnit().UnitOfMeasure) << _T(")");
      (*xs_table)(row++, 1) << _T("-"); // Std

      (*xs_table)(row, 0) << RPT_SBOT_DECK << _T(" = n(") << Sub2(_T("I"), _T("x")) << _T("/") << RPT_YBOT_DECK << _T(")") << _T(" (") << rptLength3UnitTag(&pDisplayUnits->GetSectModulusUnit().UnitOfMeasure) << _T(")");
      (*xs_table)(row++, 1) << _T("-"); // Sbd

      (*xs_table)(row, 0) << Sub2(_T("Q"), _T("deck")) << _T(" (") << rptLength3UnitTag(&pDisplayUnits->GetSectModulusUnit().UnitOfMeasure) << _T(")");
      (*xs_table)(row++, 1) << _T("-"); // Qdeck

      (*xs_table)(row, 0) << _T("Effective Flange Width (") << rptLengthUnitTag(&pDisplayUnits->GetComponentDimUnit().UnitOfMeasure) << _T(")");
      (*xs_table)(row++,1) << _T("-"); // Effective flange width
   }

   (*xs_table)(row, 0) << Sub2(_T("k"), _T("t")) << _T(" (") << rptLengthUnitTag(&pDisplayUnits->GetComponentDimUnit().UnitOfMeasure) << _T(") (Top kern point)");
   (*xs_table)(row++,1) << l1.SetValue( pSectProp->GetKt(constructionIntervalIdx,poi) );
   
   (*xs_table)(row, 0) << Sub2(_T("k"), _T("b")) << _T(" (") << rptLengthUnitTag(&pDisplayUnits->GetComponentDimUnit().UnitOfMeasure) << _T(") (Bottom kern point)");
   (*xs_table)(row++,1) << l1.SetValue( pSectProp->GetKb(constructionIntervalIdx,poi) );

   (*xs_table)(row, 0) << _T("Perimeter (") << rptLengthUnitTag(&pDisplayUnits->GetComponentDimUnit().UnitOfMeasure) << _T(")");
   (*xs_table)(row++,1) << l1.SetValue( pSectProp->GetPerimeter(poi) );
   
   (*xs_table)(row, 0) << _T("Span/Depth Ratio");
   (*xs_table)(row++,1) << scalar.SetValue( span_length/depth );

   (*xs_table)(row, 0) << _T("Weight (") << rptForcePerLengthUnitTag(&pDisplayUnits->GetForcePerLengthUnit().UnitOfMeasure) << _T(")");
   (*xs_table)(row++,1) << force_per_length.SetValue(pSectProp->GetSegmentWeightPerLength(segmentKey) );

   (*xs_table)(row, 0) << _T("Total Weight (") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")");
   (*xs_table)(row++,1) << force.SetValue(pSectProp->GetSegmentWeight(segmentKey) );

   if ( bComposite )
   {
      // Write composite properties
      row = xs_table->GetNumberOfHeaderRows();

      (*xs_table)(row++,2) << l2.SetValue( pSectProp->GetAg(lastIntervalIdx,poi) );
      (*xs_table)(row++,2) << l4.SetValue( pSectProp->GetIxx(lastIntervalIdx,poi) );
      (*xs_table)(row++, 2) << _T("-");//l4.SetValue(pSectProp->GetIyy(lastIntervalIdx, poi));

      if (bAsymmetricGirders) 
      {
         (*xs_table)(row++, 2) << _T("-");//l4.SetValue(pSectProp->GetIxy(lastIntervalIdx, poi));
      }

      depth = pSectProp->GetHg(lastIntervalIdx, poi);
      (*xs_table)(row++,2) << l1.SetValue( depth );

      if (bAsymmetricGirders)
      {
         (*xs_table)(row++, 2) << l1.SetValue(pSectProp->GetXleft(lastIntervalIdx, poi));
         (*xs_table)(row++, 2) << l1.SetValue(pSectProp->GetXright(lastIntervalIdx, poi));
      }

      (*xs_table)(row++,2) << l1.SetValue( pSectProp->GetY(lastIntervalIdx,poi,pgsTypes::TopGirder) );
      (*xs_table)(row++,2) << l1.SetValue( pSectProp->GetY(lastIntervalIdx,poi,pgsTypes::BottomGirder) );

      if (bHasDeck)
      {
         (*xs_table)(row++, 2) << l1.SetValue(pSectProp->GetY(lastIntervalIdx, poi, pgsTypes::TopDeck));
         (*xs_table)(row++, 2) << l1.SetValue(pSectProp->GetY(lastIntervalIdx, poi, pgsTypes::BottomDeck));
      }

      (*xs_table)(row++,2) << l3.SetValue( -pSectProp->GetS(lastIntervalIdx,poi,pgsTypes::TopGirder) );
      (*xs_table)(row++,2) << l3.SetValue( pSectProp->GetS(lastIntervalIdx,poi,pgsTypes::BottomGirder) );

      if (bHasDeck)
      {
         (*xs_table)(row++, 2) << l3.SetValue(-pSectProp->GetS(lastIntervalIdx, poi, pgsTypes::TopDeck));
         (*xs_table)(row++, 2) << l3.SetValue(-pSectProp->GetS(lastIntervalIdx, poi, pgsTypes::BottomDeck));
         (*xs_table)(row++, 2) << l3.SetValue(pSectProp->GetQSlab(lastIntervalIdx,poi));
         (*xs_table)(row++, 2) << l1.SetValue(pSectProp->GetEffectiveFlangeWidth(poi));
      }

      if ( lastTendonStressingIntervalIdx != INVALID_INDEX )
      {
         // PT occurs after the deck is composite so kern points are applicable
         (*xs_table)(row++,2) << l1.SetValue( pSectProp->GetKt(lastIntervalIdx,poi) );
         (*xs_table)(row++,2) << l1.SetValue( pSectProp->GetKb(lastIntervalIdx,poi) );
      }
      else
      {
         // No PT after deck is composite so kern points have not value... don't report them
         (*xs_table)(row++,2) << _T("-"); // kt
         (*xs_table)(row++,2) << _T("-"); // kb
      }
      (*xs_table)(row++,2) << _T("-"); // perimeter

      (*xs_table)(row++,2) << scalar.SetValue( span_length/depth );
      (*xs_table)(row++,2) << _T("-"); // wgt/length
      (*xs_table)(row++,2) << _T("-"); // total weight
   }

   return xs_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CSectionPropertiesTable::MakeCopy(const CSectionPropertiesTable& rOther)
{
   // Add copy code here...
}

void CSectionPropertiesTable::MakeAssignment(const CSectionPropertiesTable& rOther)
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
bool CSectionPropertiesTable::AssertValid() const
{
   return true;
}

void CSectionPropertiesTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CSectionPropertiesTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CSectionPropertiesTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CSectionPropertiesTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CSectionPropertiesTable");

   TESTME_EPILOG("StrandEccTable");
}
#endif // _UNITTEST
