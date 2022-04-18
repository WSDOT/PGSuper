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

   rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable(1);

   RowIndexType layoutTableRow = 0;
   GET_IFACE2(pBroker,IBridge,pBridge);
   PierIndexType nPiers = pBridge->GetPierCount();
   for ( PierIndexType pierIdx = 1; pierIdx < nPiers-1; pierIdx++, layoutTableRow++ )
   {
      if ( pBridge->GetPierModelType(pierIdx) == pgsTypes::pmtIdealized )
      {
         continue;
      }

      CString strLabel;
      strLabel.Format(_T("Pier %s"),LABEL_PIER(pierIdx));

      rptRcTable* xs_table = rptStyleManager::CreateDefaultTable(4,strLabel);
      (*pLayoutTable)(layoutTableRow++,0) << xs_table;

      ColumnIndexType tableColIdx = 0;
      (*xs_table)(0,tableColIdx++) << _T("Column");
      (*xs_table)(0,tableColIdx++) << COLHDR(_T("Height"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*xs_table)(0,tableColIdx++) << COLHDR(_T("A"), rptLength2UnitTag, pDisplayUnits->GetAreaUnit() );
      (*xs_table)(0,tableColIdx++) << COLHDR(_T("I"), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );

      RowIndexType rowIdx = xs_table->GetNumberOfHeaderRows();
      ColumnIndexType nColumns = pBridge->GetColumnCount(pierIdx);
      for ( ColumnIndexType colIdx = 0; colIdx < nColumns; colIdx++, rowIdx++ )
      {
         tableColIdx = 0;
         Float64 H, A, I;
         pBridge->GetColumnProperties(pierIdx,colIdx,false,&H,&A,&I);
         (*xs_table)(rowIdx,tableColIdx++) << LABEL_COLUMN(colIdx);
         (*xs_table)(rowIdx,tableColIdx++) << l.SetValue(H);
         (*xs_table)(rowIdx,tableColIdx++) << l2.SetValue(A);
         (*xs_table)(rowIdx,tableColIdx++) << l4.SetValue(I);
      }
   }

   return pLayoutTable;
}

void CColumnPropertiesTable::MakeCopy(const CColumnPropertiesTable& rOther)
{
   // Add copy code here...
}

void CColumnPropertiesTable::MakeAssignment(const CColumnPropertiesTable& rOther)
{
   MakeCopy( rOther );
}
