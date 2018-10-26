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
#include <Reporting\LongRebarLocations.h>

#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
#include <IFace\Project.h>

#include <Lrfd\RebarPool.h>

#include <WBFLGenericBridgeTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLongRebarLocations
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CLongRebarLocations::CLongRebarLocations()
{
}

CLongRebarLocations::CLongRebarLocations(const CLongRebarLocations& rOther)
{
   MakeCopy(rOther);
}

CLongRebarLocations::~CLongRebarLocations()
{
}

//======================== OPERATORS  =======================================
CLongRebarLocations& CLongRebarLocations::operator= (const CLongRebarLocations& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }
   return *this;
}

//======================== OPERATIONS =======================================
void CLongRebarLocations::Build(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                IDisplayUnits* pDispUnit) const
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,ILongRebarGeometry,pLongRebarGeometry);
   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType nspans = pBridge->GetSpanCount();
   CHECK(span<nspans);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDispUnit->GetComponentDimUnit(),  false );

   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<"Longitudinal Rebar Locations"<<rptNewLine;

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   // get bars a girder middle (really doesn't matter).
   Float64 mid = pBridge->GetGirderLength(span,girder)/2.0;
   pgsPointOfInterest poi(span,girder,mid);
   CComPtr<IRebarSection> rebars;
   pLongRebarGeometry->GetRebars(poi,&rebars);

   CollectionIndexType count;
   rebars->get_Count(&count);

   if ( count == 0 )
   {
      *pPara<<"No Longitudinal Rebar Defined"<<rptNewLine;
      return;
   }

   *pPara<<"All longitudinal rebars run the entire length of the girder"<<rptNewLine;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(3,"");
   *pPara << p_table;

   (*p_table)(0,0) << COLHDR("X",rptLengthUnitTag, pDispUnit->GetComponentDimUnit() );
   (*p_table)(0,1) << COLHDR("Y",rptLengthUnitTag, pDispUnit->GetComponentDimUnit() );
   (*p_table)(0,2) << "Bar" << rptNewLine << "Size";

   int row=1;

   CComPtr<IEnumRebarSectionItem> enumRebarSectionItem;
   rebars->get__EnumRebarSectionItem(&enumRebarSectionItem);

   CComPtr<IRebarSectionItem> rebar_section_item;
   while ( enumRebarSectionItem->Next(1,&rebar_section_item,0) != S_FALSE )
   {
      CComPtr<IPoint2d> location;
      rebar_section_item->get_Location(&location);
      double x,y;
      location->get_X(&x);
      location->get_Y(&y);

      CComPtr<IRebar> rebar;
      rebar_section_item->get_Rebar(&rebar);

      CComBSTR bstrName;
      rebar->get_Name(&bstrName);

      (*p_table)(row,0) << dim.SetValue(x);
      (*p_table)(row,1) << dim.SetValue(y);
      (*p_table)(row,2) << OLE2A(bstrName);
      
      
      row++;
      rebar_section_item.Release();
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CLongRebarLocations::MakeCopy(const CLongRebarLocations& rOther)
{
   // Add copy code here...
}

void CLongRebarLocations::MakeAssignment(const CLongRebarLocations& rOther)
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
bool CLongRebarLocations::AssertValid() const
{
   return true;
}

void CLongRebarLocations::Dump(dbgDumpContext& os) const
{
   os << "Dump for CLongRebarLocations" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CLongRebarLocations::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CLongRebarLocations");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CLongRebarLocations");

   TESTME_EPILOG("CLongRebarLocations");
}
#endif // _UNITTEST
