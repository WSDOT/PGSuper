///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <Reporting\ColumnPropertiesTable.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CColumnPropertiesTable::CColumnPropertiesTable()
{
}

CColumnPropertiesTable::CColumnPropertiesTable(const CColumnPropertiesTable& rOther)
{
   MakeCopy(rOther);
}

CColumnPropertiesTable::~CColumnPropertiesTable()
{
}

//======================== OPERATORS  =======================================
CColumnPropertiesTable& CColumnPropertiesTable::operator= (const CColumnPropertiesTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CColumnPropertiesTable::Build(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue, l, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE( rptAreaUnitValue, l2, pDisplayUnits->GetAreaUnit(), false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, l4, pDisplayUnits->GetMomentOfInertiaUnit(), false );

   rptRcTable* xs_table = pgsReportStyleHolder::CreateDefaultTable(4,_T("Column Section Properties"));

   ColumnIndexType colIdx = 0;
   (*xs_table)(0,colIdx++) << _T("Pier");
   (*xs_table)(0,colIdx++) << COLHDR(_T("Height"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*xs_table)(0,colIdx++) << COLHDR(_T("A"), rptLength2UnitTag, pDisplayUnits->GetAreaUnit() );
   (*xs_table)(0,colIdx++) << COLHDR(_T("I"), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );

   RowIndexType rowIdx = xs_table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IBridge,pBridge);
   PierIndexType nPiers = pBridge->GetPierCount();
   for ( PierIndexType pierIdx = 1; pierIdx < nPiers-1; pierIdx++, rowIdx++ )
   {
      colIdx = 0;
      (*xs_table)(rowIdx,colIdx++) << LABEL_PIER(pierIdx);
      if ( pBridge->GetPierModelType(pierIdx) == pgsTypes::pmtPhysical )
      {
         Float64 H, A, I, E;
         pBridge->GetColumnProperties(pierIdx,false,&H,&A,&I,&E);
         (*xs_table)(rowIdx,colIdx++) << l.SetValue(H);
         (*xs_table)(rowIdx,colIdx++) << l2.SetValue(A);
         (*xs_table)(rowIdx,colIdx++) << l4.SetValue(I);
      }
      else
      {
         (*xs_table)(rowIdx,colIdx++) << RPT_NA; // H
         (*xs_table)(rowIdx,colIdx++) << RPT_NA; // A
         (*xs_table)(rowIdx,colIdx++) << RPT_NA; // I
      }
   }

   return xs_table;
}

void CColumnPropertiesTable::MakeCopy(const CColumnPropertiesTable& rOther)
{
   // Add copy code here...
}

void CColumnPropertiesTable::MakeAssignment(const CColumnPropertiesTable& rOther)
{
   MakeCopy( rOther );
}
