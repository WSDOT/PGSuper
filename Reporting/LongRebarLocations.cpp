///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include <Reporting\LongRebarLocations.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>

#include <Lrfd\RebarPool.h>

#include <WBFLGenericBridgeTools.h>

#include <PgsExt\LongitudinalRebarData.h>

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
void CLongRebarLocations::Build(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,
                                IEAFDisplayUnits* pDisplayUnits) const
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,IBridge,pBridge);

   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,    pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(),   false );

   rptParagraph* pHead = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<_T("Longitudinal Rebar Locations")<<rptNewLine;

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,ILongitudinalRebar,pLongRebar);
   const CLongitudinalRebarData* pRebarData = pLongRebar->GetSegmentLongitudinalRebarData(segmentKey);

   const std::vector<CLongitudinalRebarData::RebarRow>& rebar_rows = pRebarData->RebarRows;

   CollectionIndexType count = rebar_rows.size();
   if ( count == 0 )
   {
      *pPara<<_T("No Longitudinal Rebar Defined")<<rptNewLine;
      return;
   }

   *pPara<<_T("Bar start and end locations measured from left end of girder")<<rptNewLine;

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(8,_T(""));
   *pPara << p_table;

   (*p_table)(0,0) << _T("Row");
   (*p_table)(0,1) << COLHDR(_T("Bar Start"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,2) << COLHDR(_T("Bar End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,3) << _T("Girder") << rptNewLine << _T("Face");
   (*p_table)(0,4) << COLHDR(_T("Cover"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(0,5) << _T("Bar") << rptNewLine << _T("Size");
   (*p_table)(0,6) << _T("# of") << rptNewLine << _T("Bars");
   (*p_table)(0,7) << COLHDR(_T("Spacing"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   std::vector<CLongitudinalRebarData::RebarRow>::const_iterator iter(rebar_rows.begin());
   std::vector<CLongitudinalRebarData::RebarRow>::const_iterator end(rebar_rows.end());
   for ( ; iter != end; iter++ )
   {
      const CLongitudinalRebarData::RebarRow& rowData = *iter;

      Float64 startLoc, endLoc;
      bool onGirder = rowData.GetRebarStartEnd(segment_length, &startLoc, &endLoc);

      const matRebar* pRebar = lrfdRebarPool::GetInstance()->GetRebar(pRebarData->BarType, pRebarData->BarGrade, rowData.BarSize);
      if (pRebar)
      {
         (*p_table)(row,0) << row;

         if (onGirder)
            (*p_table)(row,1) << length.SetValue(startLoc);
         else
            (*p_table)(row,1) << color(Red) << _T("Off") << color(Black);

         if (onGirder)
            (*p_table)(row,2) << length.SetValue(endLoc);
         else
            (*p_table)(row,2)  << color(Red) << _T("Girder") << color(Black);

         (*p_table)(row,3) << (rowData.Face==pgsTypes::TopFace ? _T("Top") : _T("Bottom"));
         (*p_table)(row,4) << dim.SetValue(rowData.Cover);
         (*p_table)(row,5) << pRebar->GetName();
         (*p_table)(row,6) << rowData.NumberOfBars;
         if(rowData.NumberOfBars > 1)
            (*p_table)(row,7) << dim.SetValue(rowData.BarSpacing);
         else
            (*p_table)(row,7) << _T("-");
         
         row++;
      }
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
   os << _T("Dump for CLongRebarLocations") << endl;
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
