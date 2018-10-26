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
#include <Reporting\SectPropTable.h>

#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>

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
#pragma optimize( "", off )
rptRcTable* CSectionPropertiesTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,bool bComposite,
                                           IDisplayUnits* pDispUnit) const
{
#if defined _DEBUG
   GET_IFACE2(pBroker,IGirder,pGirder);
   ATLASSERT( pGirder->IsPrismatic(pgsTypes::CastingYard,span,girder) == true );
   if ( bComposite )
   {
      ATLASSERT( pGirder->IsPrismatic(pgsTypes::BridgeSite3,span,girder) == true );
   }
#endif // _DEBUG

   GET_IFACE2(pBroker,ISectProp2,pSectProp);
   GET_IFACE2(pBroker,IBridge,pBridge);

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);

   ColumnIndexType nColumns = (bComposite ? 3 : 2);

   rptRcTable* xs_table = pgsReportStyleHolder::CreateDefaultTable(nColumns,"Section Properties");

   (*xs_table)(0,0) << "";

   xs_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   xs_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   (*xs_table)(0,1).SetStyleName(pgsReportStyleHolder::GetTableColumnHeadingStyle() );
   (*xs_table)(0,1) << "Girder";

   if ( bComposite )
   {
      (*xs_table)(0,2).SetStyleName(pgsReportStyleHolder::GetTableColumnHeadingStyle() );
      (*xs_table)(0,2) << "Composite";
   }


   // Write labels
   RowIndexType row = xs_table->GetNumberOfHeaderRows();

   (*xs_table)(row++,0) << "Area (" << rptAreaUnitTag( &pDispUnit->GetAreaUnit().UnitOfMeasure ) <<")";
   (*xs_table)(row++,0) << "I"<< Sub("x") << " (" << rptLength4UnitTag( &pDispUnit->GetMomentOfInertiaUnit().UnitOfMeasure ) <<")";
   (*xs_table)(row++,0) << "I"<< Sub("y") << " ("  << rptLength4UnitTag( &pDispUnit->GetMomentOfInertiaUnit().UnitOfMeasure ) <<")";
   (*xs_table)(row++,0) << "d (girder depth)  (" << rptLengthUnitTag( &pDispUnit->GetComponentDimUnit().UnitOfMeasure ) <<")";
   (*xs_table)(row++,0) << RPT_YTOP  << Sub(" girder") << " (" << rptLengthUnitTag( &pDispUnit->GetComponentDimUnit().UnitOfMeasure ) <<")";

   if ( bComposite )
   {
      (*xs_table)(row++,0) << RPT_YTOP  << Sub(" slab") << " (" << rptLengthUnitTag( &pDispUnit->GetComponentDimUnit().UnitOfMeasure ) <<")";
   }

   (*xs_table)(row++,0) << RPT_YBOT  << " (" << rptLengthUnitTag( &pDispUnit->GetComponentDimUnit().UnitOfMeasure ) <<")";
   
   (*xs_table)(row++,0) << Sub2("k","t")  << " (" << rptLengthUnitTag( &pDispUnit->GetComponentDimUnit().UnitOfMeasure ) <<") (Top kern point)";
   (*xs_table)(row++,0) << Sub2("k","b")  << " (" << rptLengthUnitTag( &pDispUnit->GetComponentDimUnit().UnitOfMeasure ) <<") (Bottom kern point)";

   (*xs_table)(row++,0) << RPT_STOP  << Sub(" girder") << " (" << rptLength3UnitTag( &pDispUnit->GetSectModulusUnit().UnitOfMeasure ) <<")";
   (*xs_table)(row++,0) << RPT_SBOT  << " (" << rptLength3UnitTag( &pDispUnit->GetSectModulusUnit().UnitOfMeasure ) <<")";

   if ( bComposite )
   {
      (*xs_table)(row++,0) << RPT_STOP  << Sub(" slab") << " = n(" << Sub2("I","x") << "/" << Sub2("Y","t slab") << ")" << " (" << rptLength3UnitTag( &pDispUnit->GetSectModulusUnit().UnitOfMeasure ) <<")";
      (*xs_table)(row++,0) << "Q" << Sub("slab") << " (" << rptLength3UnitTag( &pDispUnit->GetSectModulusUnit().UnitOfMeasure ) <<")";
      (*xs_table)(row++,0) << "Effective Flange Width (" << rptLengthUnitTag( &pDispUnit->GetComponentDimUnit().UnitOfMeasure ) <<")";
   }

   (*xs_table)(row++,0) << "Perimeter (" << rptLengthUnitTag( &pDispUnit->GetComponentDimUnit().UnitOfMeasure ) <<")";
   (*xs_table)(row++,0) << "Span/Depth Ratio";
   (*xs_table)(row++,0) << "Weight (" << rptForcePerLengthUnitTag( &pDispUnit->GetForcePerLengthUnit().UnitOfMeasure ) <<")";
   (*xs_table)(row++,0) << "Total Weight (" << rptForceUnitTag( &pDispUnit->GetGeneralForceUnit().UnitOfMeasure ) <<")";

   INIT_UV_PROTOTYPE( rptAreaUnitValue, l2, pDispUnit->GetAreaUnit(), false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, l4, pDispUnit->GetMomentOfInertiaUnit(), false );
   INIT_UV_PROTOTYPE( rptLength3UnitValue, l3, pDispUnit->GetSectModulusUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, l1, pDispUnit->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, force, pDispUnit->GetGeneralForceUnit(), false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, force_per_length, pDispUnit->GetForcePerLengthUnit(), false );

   // DUMMY POI, but the section is prismatic so it is as good as any other
   pgsPointOfInterest poi(span,girder,0.00);

    // Write non-composite properties
   row = xs_table->GetNumberOfHeaderRows();

   double Yt, Yb;
   Yt = pSectProp->GetYt(pgsTypes::CastingYard,poi);
   Yb = fabs(pSectProp->GetYb(pgsTypes::CastingYard,poi));
   double depth = Yt + Yb;

   double span_length = pBridge->GetSpanLength(span,girder);

   (*xs_table)(row++,1) << l2.SetValue( pSectProp->GetAg(pgsTypes::CastingYard,poi) );
   (*xs_table)(row++,1) << l4.SetValue( pSectProp->GetIx(pgsTypes::CastingYard,poi) );
   (*xs_table)(row++,1) << l4.SetValue( pSectProp->GetIy(pgsTypes::CastingYard,poi) );
   (*xs_table)(row++,1) << l1.SetValue( depth );
   (*xs_table)(row++,1) << l1.SetValue( Yt );

   if ( bComposite )
   {
      (*xs_table)(row++,1) << "-";
   }

   (*xs_table)(row++,1) << l1.SetValue( Yb );

   (*xs_table)(row++,1) << l1.SetValue( fabs(pSectProp->GetKt(poi)) );
   (*xs_table)(row++,1) << l1.SetValue( fabs(pSectProp->GetKb(poi)) );

   (*xs_table)(row++,1) << l3.SetValue( fabs(pSectProp->GetSt(pgsTypes::CastingYard,poi)) );
   (*xs_table)(row++,1) << l3.SetValue( pSectProp->GetSb(pgsTypes::CastingYard,poi) );

   if ( bComposite )
   {
      (*xs_table)(row++,1) << "-";
      (*xs_table)(row++,1) << "-";
      (*xs_table)(row++,1) << "-";
   }

   (*xs_table)(row++,1) << l1.SetValue( pSectProp->GetPerimeter(poi) );
   (*xs_table)(row++,1) << scalar.SetValue( span_length/depth );

   (*xs_table)(row++,1) << force_per_length.SetValue(pSectProp->GetGirderWeightPerLength(span,girder) );
   (*xs_table)(row++,1) << force.SetValue(pSectProp->GetGirderWeight(span,girder) );

   if ( bComposite )
   {
      // Write composite properties
      row = xs_table->GetNumberOfHeaderRows();

      Yt = pSectProp->GetYt(pgsTypes::BridgeSite3,poi);
      Yb = fabs(pSectProp->GetYb(pgsTypes::BridgeSite3,poi));
      depth = Yt + Yb;

      (*xs_table)(row++,2) << l2.SetValue( pSectProp->GetAg(pgsTypes::BridgeSite3,poi) );
      (*xs_table)(row++,2) << l4.SetValue( pSectProp->GetIx(pgsTypes::BridgeSite3,poi) );
      (*xs_table)(row++,2) << l4.SetValue( pSectProp->GetIy(pgsTypes::BridgeSite3,poi) );
      (*xs_table)(row++,2) << l1.SetValue( depth );
      (*xs_table)(row++,2) << l1.SetValue( pSectProp->GetYtGirder(pgsTypes::BridgeSite3,poi) );
      (*xs_table)(row++,2) << l1.SetValue( Yt );
      (*xs_table)(row++,2) << l1.SetValue( Yb );
      (*xs_table)(row++,2) << "-";
      (*xs_table)(row++,2) << "-";
      (*xs_table)(row++,2) << l3.SetValue( fabs(pSectProp->GetStGirder(pgsTypes::BridgeSite3,poi)) );
      (*xs_table)(row++,2) << l3.SetValue( pSectProp->GetSb(pgsTypes::BridgeSite3,poi) );
      (*xs_table)(row++,2) << l3.SetValue( fabs(pSectProp->GetSt(pgsTypes::BridgeSite3,poi)) );
      (*xs_table)(row++,2) << l3.SetValue( pSectProp->GetQSlab(poi) );
      (*xs_table)(row++,2) << l1.SetValue( pSectProp->GetEffectiveFlangeWidth(poi) );
      (*xs_table)(row++,2) << "-";
      (*xs_table)(row++,2) << scalar.SetValue( span_length/depth );
      (*xs_table)(row++,2) << "-";
      (*xs_table)(row++,2) << "-";
   }

   return xs_table;
}
#pragma optimize( "", on )

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
   os << "Dump for CSectionPropertiesTable" << endl;
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
