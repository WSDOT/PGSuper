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
#include <Reporting\StirrupTable.h>

#include <IFace\Bridge.h>

#include <IFace\Project.h>

#include <Lrfd\RebarPool.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CStirrupTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CStirrupTable::CStirrupTable()
{
}

CStirrupTable::CStirrupTable(const CStirrupTable& rOther)
{
   MakeCopy(rOther);
}

CStirrupTable::~CStirrupTable()
{
}

//======================== OPERATORS  =======================================
CStirrupTable& CStirrupTable::operator= (const CStirrupTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }
   return *this;
}

//======================== OPERATIONS =======================================
void CStirrupTable::Build(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,
                                IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IStirrupGeometry,pStirrupGeometry);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim_u, pDisplayUnits->GetComponentDimUnit(),  true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc_u, pDisplayUnits->GetSpanLengthUnit(), true );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(8,_T("Primary Bars"));
   *pPara << p_table;

   (*p_table)(0,0) << _T("Zone");
   (*p_table)(0,1) << COLHDR(_T("Zone Start"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,2) << COLHDR(_T("Zone End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,3) << _T("Bar Size");
   (*p_table)(0,4) << COLHDR(_T("Bar Spacing"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(0,5) << _T("# of")<< rptNewLine<<_T("Vertical Legs");
   (*p_table)(0,6) << _T("# Legs")<<rptNewLine<<_T("Extended") << rptNewLine << _T("into Deck*");
   (*p_table)(0,7) << _T("Confinement") << rptNewLine << _T("Bar Size");

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   ZoneIndexType nz = pStirrupGeometry->GetPrimaryZoneCount(segmentKey);
   for (ZoneIndexType iz=0; iz<nz; iz++)
   {
      (*p_table)(row,0) << LABEL_STIRRUP_ZONE(iz);

      Float64 zoneStart, zoneEnd;
      pStirrupGeometry->GetPrimaryZoneBounds(segmentKey, iz, &zoneStart, &zoneEnd);

      (*p_table)(row,1) << loc.SetValue(zoneStart);
      (*p_table)(row,2) << loc.SetValue(zoneEnd);

      matRebar::Size barSize;
      Float64 spacing;
      Float64 nStirrups;
      pStirrupGeometry->GetPrimaryVertStirrupBarInfo(segmentKey,iz,&barSize,&nStirrups,&spacing);


      if (barSize != matRebar::bsNone)
      {
         (*p_table)(row,3) << lrfdRebarPool::GetBarSize(barSize).c_str();
         (*p_table)(row,4) << dim.SetValue(spacing);
         (*p_table)(row,5) << nStirrups;

         Float64 num_legs = pStirrupGeometry->GetPrimaryHorizInterfaceBarCount(segmentKey,iz);
         (*p_table)(row,6) << num_legs;

      }
      else
      {
         (*p_table)(row,3) << _T("(None)");
         (*p_table)(row,4) << RPT_NA;
         (*p_table)(row,5) << _T("(None)");
         (*p_table)(row,6) << _T("(None)");
      }

      barSize = pStirrupGeometry->GetPrimaryConfinementBarSize(segmentKey,iz);
      if (barSize != matRebar::bsNone)
      {
         (*p_table)(row,7) << lrfdRebarPool::GetBarSize(barSize).c_str();
      }
      else
      {
         (*p_table)(row,7) << _T("(None)");
      }

      row++;
   }

   *pPara <<_T("* Bars add to horizontal interface shear capacity")<<rptNewLine;

   //
   p_table = pgsReportStyleHolder::CreateDefaultTable(6,_T("Additional Bars for Horizontal Interface Shear Capacity"));
   *pPara << p_table;

   (*p_table)(0,0) << _T("Zone");
   (*p_table)(0,1) << COLHDR(_T("Zone Start"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,2) << COLHDR(_T("Zone End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,3) << _T("Bar Size");
   (*p_table)(0,4) << COLHDR(_T("Bar Spacing"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(0,5) << _T("# of")<< rptNewLine<<_T("Legs");

   row = p_table->GetNumberOfHeaderRows();
   nz = pStirrupGeometry->GetHorizInterfaceZoneCount(segmentKey);
   for (ZoneIndexType iz=0; iz<nz; iz++)
   {
      (*p_table)(row,0) << LABEL_STIRRUP_ZONE(iz);

      Float64 zoneStart, zoneEnd;
      pStirrupGeometry->GetHorizInterfaceZoneBounds(segmentKey, iz, &zoneStart, &zoneEnd);

      (*p_table)(row,1) << loc.SetValue(zoneStart);
      (*p_table)(row,2) << loc.SetValue(zoneEnd);

      matRebar::Size barSize;
      Float64 spacing;
      Float64 nStirrups;
      pStirrupGeometry->GetHorizInterfaceBarInfo(segmentKey,iz,&barSize,&nStirrups,&spacing);

      if (barSize != matRebar::bsNone)
      {
         (*p_table)(row,3) << lrfdRebarPool::GetBarSize(barSize).c_str();
         (*p_table)(row,4) << dim.SetValue(spacing);
         (*p_table)(row,5) << nStirrups;
      }
      else
      {
         (*p_table)(row,3) << _T("(None)");
         (*p_table)(row,4) << RPT_NA;
         (*p_table)(row,5) << _T("(None)");
      }

      row++;
   }

   // Additional Splitting Bars
   matRebar::Size size;
   Float64 zoneLength, nBars, spacing;
   pStirrupGeometry->GetAddSplittingBarInfo(segmentKey, &size, &zoneLength, &nBars, &spacing);

   if (size != matRebar::bsNone)
   {
      p_table = pgsReportStyleHolder::CreateDefaultTable(4,_T("Additional Splitting Stirrups"));
      *pPara << p_table;

      (*p_table)(0,0) << COLHDR(_T("Zone Length"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(1,0) << loc.SetValue(zoneLength);

      (*p_table)(0,1) << _T("Bar Size");
      (*p_table)(1,1) << lrfdRebarPool::GetBarSize(size).c_str();

      (*p_table)(0,2) << COLHDR(_T("Bar Spacing"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*p_table)(1,2) << dim.SetValue(spacing);

      (*p_table)(0,3) << _T("# of")<< rptNewLine<<_T("Legs");
      (*p_table)(1,3) << nBars;
   }
   else
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara <<_T("Additional Splitting Stirrups")<<rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara <<_T("Additional splitting stirrups not present")<<rptNewLine;
   }

   // bottom flange confinement steel
   pStirrupGeometry->GetAddConfinementBarInfo(segmentKey, &size, &zoneLength, &spacing);

   if (size != matRebar::bsNone)
   {
      p_table = pgsReportStyleHolder::CreateDefaultTable(3,_T("Additional Bottom Flange Confinement Stirrups"));
      *pPara << p_table;

      (*p_table)(0,0) << COLHDR(_T("Zone Length"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(1,0) << loc.SetValue(zoneLength);

      (*p_table)(0,1) << _T("Bar Size");
      (*p_table)(1,1) << lrfdRebarPool::GetBarSize(size).c_str();

      (*p_table)(0,2) << COLHDR(_T("Bar Spacing"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*p_table)(1,2) << dim.SetValue(spacing);
   }
   else
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara <<_T("Additional Bottom Flange Confinement Stirrups")<<rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara <<_T("Additional confinement stirrups not present")<<rptNewLine;
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CStirrupTable::MakeCopy(const CStirrupTable& rOther)
{
   // Add copy code here...
}

void CStirrupTable::MakeAssignment(const CStirrupTable& rOther)
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
bool CStirrupTable::AssertValid() const
{
   return true;
}

void CStirrupTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CStirrupTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CStirrupTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CStirrupTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CStirrupTable");

   TESTME_EPILOG("CStirrupTable");
}
#endif // _UNITTEST
