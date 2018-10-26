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
#include <Reporting\StirrupTable.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
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
void CStirrupTable::Build(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IStirrupGeometry,pStirrupGeometry);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim_u, pDisplayUnits->GetComponentDimUnit(),  true );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   // top flange horizontal interface stirrups
   pgsPointOfInterest poi(span,girder,0.0);
   BarSizeType size = pStirrupGeometry->GetTopFlangeBarSize(poi);
   Float64 tfs = pStirrupGeometry->GetTopFlangeS(poi);
   if (size!=0)
      *pPara <<_T("Top flange stirrups are #")<<size<<_T(" at ")<<dim_u.SetValue(tfs)<<_T(" spacing.")<<rptNewLine;
   else
      *pPara <<_T("Top flange stirrups not present")<<rptNewLine;

   // bottom flange confinement steel
   size = pStirrupGeometry->GetConfinementBarSize(span,girder);
   Uint32 lz = pStirrupGeometry->GetNumConfinementZones(span,girder);
   if (lz!=0 && size!=0)
      *pPara <<_T("Bottom flange confinement stirrups are #")<<size<<_T(" ending in Zone ")<<lz<<rptNewLine;
   else
      *pPara<<_T("Bottom flange confinement steel not present")<<rptNewLine;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(8,_T(""));
   *pPara << p_table;

   (*p_table)(0,0) << _T("Zone");
   (*p_table)(0,1) << COLHDR(_T("Zone Start"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,2) << COLHDR(_T("Zone End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,3) << COLHDR(_T("Bar Spacing"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(0,4) << _T("Vert Bar") << rptNewLine << _T("Size");
   (*p_table)(0,5) << _T("# Vert") << rptNewLine << _T("Bars");
   (*p_table)(0,6) << _T("Horz Bar") << rptNewLine << _T("Size");
   (*p_table)(0,7) << _T("# Horz") << rptNewLine << _T("Bars");

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   Uint32 nz = pStirrupGeometry->GetNumZones(span,girder);
   for (Uint32 iz=0; iz<nz; iz++)
   {
      (*p_table)(row,0) << pStirrupGeometry->GetZoneId(span,girder,iz);
      (*p_table)(row,1) << loc.SetValue(pStirrupGeometry->GetZoneStart(span,girder,iz));
      (*p_table)(row,2) << loc.SetValue(pStirrupGeometry->GetZoneEnd(span,girder,iz));
      (*p_table)(row,3) << dim.SetValue(pStirrupGeometry->GetS(span,girder,iz));

      BarSizeType barSize = pStirrupGeometry->GetVertStirrupBarSize(span,girder,iz);
      if (barSize != 0)
      {
         (*p_table)(row,4) << _T("#") << barSize;
         (*p_table)(row,5) << pStirrupGeometry->GetVertStirrupBarCount(span,girder,iz);
      }
      else
      {
         (*p_table)(row,4) << _T("(None)");
         (*p_table)(row,5) << _T("(None)");
      }


      barSize = pStirrupGeometry->GetHorzStirrupBarSize(span,girder,iz);
      if (barSize != 0)
      {
         (*p_table)(row,6) << _T("#") << barSize;
         (*p_table)(row,7) << pStirrupGeometry->GetHorzStirrupBarCount(span,girder,iz);
      }
      else
      {
         (*p_table)(row,6) << _T("(None)");
         (*p_table)(row,7) << _T("(None)");
      }

      row++;
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
