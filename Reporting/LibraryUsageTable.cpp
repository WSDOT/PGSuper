///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include "stdafx.h"
#include <Reporting\LibraryUsageTable.h>

#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLibraryUsageTable
****************************************************************************/


CLibraryUsageTable::CLibraryUsageTable()
{
}

CLibraryUsageTable::~CLibraryUsageTable()
{
}

rptRcTable* CLibraryUsageTable::Build(IBroker* pBroker) const
{
   rptRcTable* table = rptStyleManager::CreateDefaultTable(3,_T(""));
   table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   table->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   table->SetColumnStyle(2, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT) );

   table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   table->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   table->SetStripeRowColumnStyle(2, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );

   (*table)(0,0) << _T("Library");
   (*table)(0,1) << _T("Entry");
   (*table)(0,2) << _T("Source");

   GET_IFACE2(pBroker,ILibrary,pLibrary);
   std::vector<libEntryUsageRecord> records = pLibrary->GetLibraryUsageRecords();

   std::vector<libEntryUsageRecord>::iterator iter;
   RowIndexType row = table->GetNumberOfHeaderRows();

   for ( iter = records.begin(); iter != records.end(); iter++ )
   {
      libEntryUsageRecord record = *iter;
      (*table)(row,0) << record.LibName;
      (*table)(row,1) << record.EntryName;
      
      if ( record.bEditable )
      {
         (*table)(row,2) << _T("Project Library");
      }
      else
      {
         (*table)(row,2) << _T("Master Library");
      }

      row++;
   }

   return table;
}
