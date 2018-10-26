///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
   IntervalIndexType constructionIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType compositeIntervalIdx    = pIntervals->GetCompositeDeckInterval();

#if defined _DEBUG
   {
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

      GET_IFACE2(pBroker,IGirder,pGirder);
      ATLASSERT( pGirder->IsPrismatic(erectSegmentIntervalIdx,segmentKey) == true );
      if ( bComposite )
      {
         ATLASSERT( pGirder->IsPrismatic(compositeDeckIntervalIdx,segmentKey) == true );
      }
   }
#endif // _DEBUG

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   GET_IFACE2(pBroker,IBridge,pBridge);

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);

   ColumnIndexType nColumns = (bComposite ? 3 : 2);

   rptRcTable* xs_table = pgsReportStyleHolder::CreateDefaultTable(nColumns,_T("Section Properties"));

   (*xs_table)(0,0) << _T("");

   xs_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   xs_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   (*xs_table)(0,1).SetStyleName(pgsReportStyleHolder::GetTableColumnHeadingStyle() );
   (*xs_table)(0,1) << _T("Girder");

   if ( bComposite )
   {
      (*xs_table)(0,2).SetStyleName(pgsReportStyleHolder::GetTableColumnHeadingStyle() );
      (*xs_table)(0,2) << _T("Composite");
   }


   // Write labels
   RowIndexType row = xs_table->GetNumberOfHeaderRows();

   (*xs_table)(row++,0) << _T("Area (") << rptAreaUnitTag( &pDisplayUnits->GetAreaUnit().UnitOfMeasure ) <<_T(")");
   (*xs_table)(row++,0) << _T("I")<< Sub(_T("x")) << _T(" (") << rptLength4UnitTag( &pDisplayUnits->GetMomentOfInertiaUnit().UnitOfMeasure ) <<_T(")");
   (*xs_table)(row++,0) << _T("I")<< Sub(_T("y")) << _T(" (")  << rptLength4UnitTag( &pDisplayUnits->GetMomentOfInertiaUnit().UnitOfMeasure ) <<_T(")");
   (*xs_table)(row++,0) << _T("d (girder depth)  (") << rptLengthUnitTag( &pDisplayUnits->GetComponentDimUnit().UnitOfMeasure ) <<_T(")");
   (*xs_table)(row++,0) << RPT_YTOP  << Sub(_T(" girder")) << _T(" (") << rptLengthUnitTag( &pDisplayUnits->GetComponentDimUnit().UnitOfMeasure ) <<_T(")");

   if ( bComposite )
   {
      (*xs_table)(row++,0) << RPT_YTOP  << Sub(_T(" slab")) << _T(" (") << rptLengthUnitTag( &pDisplayUnits->GetComponentDimUnit().UnitOfMeasure ) <<_T(")");
   }

   (*xs_table)(row++,0) << RPT_YBOT  << _T(" (") << rptLengthUnitTag( &pDisplayUnits->GetComponentDimUnit().UnitOfMeasure ) <<_T(")");
   
   (*xs_table)(row++,0) << Sub2(_T("k"),_T("t"))  << _T(" (") << rptLengthUnitTag( &pDisplayUnits->GetComponentDimUnit().UnitOfMeasure ) <<_T(") (Top kern point)");
   (*xs_table)(row++,0) << Sub2(_T("k"),_T("b"))  << _T(" (") << rptLengthUnitTag( &pDisplayUnits->GetComponentDimUnit().UnitOfMeasure ) <<_T(") (Bottom kern point)");

   (*xs_table)(row++,0) << RPT_STOP  << Sub(_T(" girder")) << _T(" (") << rptLength3UnitTag( &pDisplayUnits->GetSectModulusUnit().UnitOfMeasure ) <<_T(")");
   (*xs_table)(row++,0) << RPT_SBOT  << _T(" (") << rptLength3UnitTag( &pDisplayUnits->GetSectModulusUnit().UnitOfMeasure ) <<_T(")");

   if ( bComposite )
   {
      (*xs_table)(row++,0) << RPT_STOP  << Sub(_T(" slab")) << _T(" = n(") << Sub2(_T("I"),_T("x")) << _T("/") << Sub2(_T("Y"),_T("t slab")) << _T(")") << _T(" (") << rptLength3UnitTag( &pDisplayUnits->GetSectModulusUnit().UnitOfMeasure ) <<_T(")");
      (*xs_table)(row++,0) << _T("Q") << Sub(_T("slab")) << _T(" (") << rptLength3UnitTag( &pDisplayUnits->GetSectModulusUnit().UnitOfMeasure ) <<_T(")");
      (*xs_table)(row++,0) << _T("Effective Flange Width (") << rptLengthUnitTag( &pDisplayUnits->GetComponentDimUnit().UnitOfMeasure ) <<_T(")");
   }

   (*xs_table)(row++,0) << _T("Perimeter (") << rptLengthUnitTag( &pDisplayUnits->GetComponentDimUnit().UnitOfMeasure ) <<_T(")");
   (*xs_table)(row++,0) << _T("Span/Depth Ratio");
   (*xs_table)(row++,0) << _T("Weight (") << rptForcePerLengthUnitTag( &pDisplayUnits->GetForcePerLengthUnit().UnitOfMeasure ) <<_T(")");
   (*xs_table)(row++,0) << _T("Total Weight (") << rptForceUnitTag( &pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure ) <<_T(")");

   INIT_UV_PROTOTYPE( rptAreaUnitValue, l2, pDisplayUnits->GetAreaUnit(), false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, l4, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   INIT_UV_PROTOTYPE( rptLength3UnitValue, l3, pDisplayUnits->GetSectModulusUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, l1, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, force_per_length, pDisplayUnits->GetForcePerLengthUnit(), false );

   // DUMMY POI, but the section is prismatic so it is as good as any other
   pgsPointOfInterest poi(segmentKey,0.00);

    // Write non-composite properties
   row = xs_table->GetNumberOfHeaderRows();

   Float64 Yt, Yb;
   Yt = pSectProp->GetY(constructionIntervalIdx,poi,pgsTypes::TopDeck);
   Yb = fabs(pSectProp->GetY(constructionIntervalIdx,poi,pgsTypes::BottomGirder));
   Float64 depth = Yt + Yb;

   Float64 span_length = pBridge->GetSegmentSpanLength(segmentKey);

   (*xs_table)(row++,1) << l2.SetValue( pSectProp->GetAg(constructionIntervalIdx,poi) );
   (*xs_table)(row++,1) << l4.SetValue( pSectProp->GetIx(constructionIntervalIdx,poi) );
   (*xs_table)(row++,1) << l4.SetValue( pSectProp->GetIy(constructionIntervalIdx,poi) );
   (*xs_table)(row++,1) << l1.SetValue( depth );
   (*xs_table)(row++,1) << l1.SetValue( Yt );

   if ( bComposite )
   {
      (*xs_table)(row++,1) << _T("-");
   }

   (*xs_table)(row++,1) << l1.SetValue( Yb );

   (*xs_table)(row++,1) << l1.SetValue( fabs(pSectProp->GetKt(constructionIntervalIdx,poi)) );
   (*xs_table)(row++,1) << l1.SetValue( fabs(pSectProp->GetKb(constructionIntervalIdx,poi)) );

   (*xs_table)(row++,1) << l3.SetValue( fabs(pSectProp->GetS(constructionIntervalIdx,poi,pgsTypes::TopDeck)) );
   (*xs_table)(row++,1) << l3.SetValue( pSectProp->GetS(constructionIntervalIdx,poi,pgsTypes::BottomGirder) );

   if ( bComposite )
   {
      (*xs_table)(row++,1) << _T("-");
      (*xs_table)(row++,1) << _T("-");
      (*xs_table)(row++,1) << _T("-");
   }

   (*xs_table)(row++,1) << l1.SetValue( pSectProp->GetPerimeter(poi) );
   (*xs_table)(row++,1) << scalar.SetValue( span_length/depth );

   (*xs_table)(row++,1) << force_per_length.SetValue(pSectProp->GetSegmentWeightPerLength(segmentKey) );
   (*xs_table)(row++,1) << force.SetValue(pSectProp->GetSegmentWeight(segmentKey) );

   if ( bComposite )
   {
      // Write composite properties
      row = xs_table->GetNumberOfHeaderRows();

      Yt = pSectProp->GetY(compositeIntervalIdx,poi,pgsTypes::TopDeck);
      Yb = fabs(pSectProp->GetY(compositeIntervalIdx,poi,pgsTypes::BottomGirder));
      depth = Yt + Yb;

      (*xs_table)(row++,2) << l2.SetValue( pSectProp->GetAg(compositeIntervalIdx,poi) );
      (*xs_table)(row++,2) << l4.SetValue( pSectProp->GetIx(compositeIntervalIdx,poi) );
      (*xs_table)(row++,2) << l4.SetValue( pSectProp->GetIy(compositeIntervalIdx,poi) );
      (*xs_table)(row++,2) << l1.SetValue( depth );
      (*xs_table)(row++,2) << l1.SetValue( pSectProp->GetY(compositeIntervalIdx,poi,pgsTypes::TopGirder) );
      (*xs_table)(row++,2) << l1.SetValue( Yt );
      (*xs_table)(row++,2) << l1.SetValue( Yb );
      (*xs_table)(row++,2) << _T("-");
      (*xs_table)(row++,2) << _T("-");
      (*xs_table)(row++,2) << l3.SetValue( fabs(pSectProp->GetS(compositeIntervalIdx,poi,pgsTypes::TopGirder)) );
      (*xs_table)(row++,2) << l3.SetValue( pSectProp->GetS(compositeIntervalIdx,poi,pgsTypes::BottomGirder) );
      (*xs_table)(row++,2) << l3.SetValue( fabs(pSectProp->GetS(compositeIntervalIdx,poi,pgsTypes::TopDeck)) );
      (*xs_table)(row++,2) << l3.SetValue( pSectProp->GetQSlab(poi) );
      (*xs_table)(row++,2) << l1.SetValue( pSectProp->GetEffectiveFlangeWidth(poi) );
      (*xs_table)(row++,2) << _T("-");
      (*xs_table)(row++,2) << scalar.SetValue( span_length/depth );
      (*xs_table)(row++,2) << _T("-");
      (*xs_table)(row++,2) << _T("-");
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
