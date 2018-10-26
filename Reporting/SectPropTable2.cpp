///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <Reporting\SectPropTable2.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\StageCompare.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>

#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CSectionPropertiesTable2
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSectionPropertiesTable2::CSectionPropertiesTable2()
{
}

CSectionPropertiesTable2::~CSectionPropertiesTable2()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
rptRcTable* CSectionPropertiesTable2::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,pgsTypes::Stage stage,
                                           IEAFDisplayUnits* pDisplayUnits) const
{
   USES_CONVERSION;
   GET_IFACE2(pBroker,IStageMap,pStageMap);
   GET_IFACE2(pBroker,IBridge,pBridge);
   std::_tostringstream os;
   os << _T("Stage: ") << OLE2T(pStageMap->GetStageName(stage)) << _T(", ");

   if ( StageCompare(stage,pgsTypes::BridgeSite2) < 0 )
      os << _T("Section: Non-composite") << std::endl;
   else
      os << _T("Section: Composite") << std::endl;

   Uint16 nCol;
   if ( stage == pgsTypes::CastingYard || stage == pgsTypes::BridgeSite1 )
      nCol = 12;
   else if ( (stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3) && pBridge->IsCompositeDeck() )
      nCol = 15;
   else
      nCol = 12; // BS2 or BS3 and noncomposite deck

   rptRcTable* xs_table = pgsReportStyleHolder::CreateDefaultTable(nCol,os.str().c_str());

   if ( span == ALL_SPANS )
   {
      xs_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      xs_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup column headers
   Uint16 col = 0;
   if ( stage == pgsTypes::CastingYard )
      (*xs_table)(0,col++) << COLHDR(RPT_GDR_END_LOCATION,   rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   else
      (*xs_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,   rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   (*xs_table)(0,col++) << COLHDR(_T("Area"),   rptAreaUnitTag,    pDisplayUnits->GetAreaUnit() );
   (*xs_table)(0,col++) << COLHDR(_T("Depth"),  rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );
   (*xs_table)(0,col++) << COLHDR(RPT_IX,   rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   (*xs_table)(0,col++) << COLHDR(RPT_IY,   rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   (*xs_table)(0,col++) << COLHDR(RPT_YTOP, rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );

   if ( pgsTypes::BridgeSite2 <= stage && pBridge->IsCompositeDeck() )
      (*xs_table)(0,col++) << COLHDR(Sub2(_T("Y"),_T("t")) << _T("") << rptNewLine << _T("Girder"), rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );
   
   (*xs_table)(0,col++) << COLHDR(RPT_YBOT, rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );
   (*xs_table)(0,col++) << COLHDR(RPT_STOP, rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
   
   if ( pgsTypes::BridgeSite2 <= stage && pBridge->IsCompositeDeck() )
      (*xs_table)(0,col++) << COLHDR(Sub2(_T("S"),_T("t")) << _T("") << rptNewLine << _T("Girder"), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );

   (*xs_table)(0,col++) << COLHDR(RPT_SBOT, rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
   (*xs_table)(0,col++) << Sub2(_T("k"),_T("t")) << _T(" (") << rptLengthUnitTag( &pDisplayUnits->GetComponentDimUnit().UnitOfMeasure ) <<_T(")") << rptNewLine << _T("(Top") << rptNewLine << _T("kern") << rptNewLine << _T("point)");
   (*xs_table)(0,col++) << Sub2(_T("k"),_T("b"))  << _T(" (") << rptLengthUnitTag( &pDisplayUnits->GetComponentDimUnit().UnitOfMeasure ) <<_T(")") << rptNewLine << _T("(Bottom") << rptNewLine << _T("kern") << rptNewLine << _T("point)");

   if ( pgsTypes::BridgeSite2 <= stage && pBridge->IsCompositeDeck() )
   {
      (*xs_table)(0,col++) << COLHDR(Sub2(_T("Q"),_T("slab")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
      (*xs_table)(0,col++) << COLHDR(_T("Effective") << rptNewLine << _T("Flange") << rptNewLine << _T("Width"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }
   else
   {
      (*xs_table)(0,col++) << COLHDR(_T("Perimeter"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  l1, pDisplayUnits->GetComponentDimUnit(),     false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    l2, pDisplayUnits->GetAreaUnit(),             false );
   INIT_UV_PROTOTYPE( rptLength3UnitValue, l3, pDisplayUnits->GetSectModulusUnit(),      false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, l4, pDisplayUnits->GetMomentOfInertiaUnit(),  false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,ISectProp2,pSectProp);

   // Get all the tabular poi's for flexure and shear
   // Merge the two vectors to form one vector to report on.
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(span,girder, stage,POI_TABULAR);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0;

   RowIndexType row = xs_table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++, row++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = *i;

      Float64 Yt, Yb, depth;
      Yt = pSectProp->GetYt(stage,poi);
      Yb = pSectProp->GetYb(stage,poi);
      depth = Yt + Yb;

      (*xs_table)(row,col++) << location.SetValue( stage, poi, end_size );
      (*xs_table)(row,col++) << l2.SetValue(pSectProp->GetAg(stage,poi));
      (*xs_table)(row,col++) << l1.SetValue(depth);
      (*xs_table)(row,col++) << l4.SetValue(pSectProp->GetIx(stage,poi));
      (*xs_table)(row,col++) << l4.SetValue(pSectProp->GetIy(stage,poi));
      (*xs_table)(row,col++) << l1.SetValue(Yt);

      if ( pgsTypes::BridgeSite2 <= stage && pBridge->IsCompositeDeck() )
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetYtGirder(stage,poi));

      (*xs_table)(row,col++) << l1.SetValue(Yb);
      (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetSt(stage,poi));

      if ( pgsTypes::BridgeSite2 <= stage && pBridge->IsCompositeDeck() )
         (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetStGirder(stage,poi));

      (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetSb(stage,poi));
      (*xs_table)(row,col++) << l1.SetValue(fabs(pSectProp->GetKt(poi)));
      (*xs_table)(row,col++) << l1.SetValue(fabs(pSectProp->GetKb(poi)));

      if ( pgsTypes::BridgeSite2 <= stage && pBridge->IsCompositeDeck() )
      {
         (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetQSlab(poi));
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetEffectiveFlangeWidth(poi));
      }
      else
      {
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetPerimeter(poi));
      }
   }

   return xs_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
