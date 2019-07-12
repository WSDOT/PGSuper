///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include "TxCSVDataExporter.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTxCSVDataExporter::CTxCSVDataExporter(void):
   m_Separator(_T(","))
{
   for(IndexType nt=0; nt<nTables; nt++)
   {
      m_MaxRows[nt] = 0;
   }
}

CTxCSVDataExporter::~CTxCSVDataExporter(void)
{
}

BOOL CTxCSVDataExporter::InitializeTable(IndexType hTable, const std::vector<std::_tstring>& columnNames)
{
   ATLASSERT(hTable > 0 && hTable <= nTables);

   hTable -= 1; // zero-based access

   m_MaxRows[hTable] = 0;

   if (!m_Tables[hTable].m_Columns.empty())
   {
      ATLASSERT(0); //probably should only be initializing once, but this might be Ok?
      m_Tables[hTable].m_Columns.clear();
      m_Tables[hTable].m_ColumnNameMap.clear();
   }

   IndexType colno = 0;
   for (auto column : columnNames)
   {
      m_Tables[hTable].m_ColumnNameMap.insert(std::make_pair(column, colno));

      m_Tables[hTable].m_Columns.push_back(std::vector<std::_tstring>());
      m_Tables[hTable].m_Columns.back().reserve(40); // 100 rows is about as many as expected normally

      colno++;
   }

   return TRUE;
}

void CTxCSVDataExporter::SetSeparator(LPCTSTR strSeparator)
{
   m_Separator = strSeparator;
}

void CTxCSVDataExporter::WriteStringToCell(IndexType hTable, LPCTSTR strRangeName, IndexType rowIdx, LPCTSTR strString)
{
   ATLASSERT(hTable > 0 && hTable <= nTables);
   hTable -= 1; // zero-based access

   CCsvTable::ColumnMapNameType::iterator it = m_Tables[hTable].m_ColumnNameMap.find(strRangeName);
   if (it == m_Tables[hTable].m_ColumnNameMap.end())
   {
      CString msg;
      msg.Format(_T("Error - The range \'%s\' was not found on sheet %d of the template spreadsheet."), strRangeName, hTable);
      ::AfxMessageBox(msg);
      ATLASSERT(0);
      return;
   }
   else
   {
      IndexType idx = it->second;
      std::vector<std::_tstring>& rColumn = m_Tables[hTable].m_Columns.at(idx);

      // We can have blank rows in columns. make sure we have room for our string
      // This will put a blank string at our target location
      size_t ncols = rColumn.size();
      if (rowIdx + 1 > ncols)
      {
         rColumn.resize(rowIdx + 1);
      }

      // Finally assign our string
      rColumn.at(rowIdx) = strString;

      // Keep track of the maximum row used thusfar
      m_MaxRows[hTable] = max(m_MaxRows[hTable], rowIdx + 1);
   }
}

void CTxCSVDataExporter::WriteWarningToCell(IndexType hTable, LPCTSTR strRangeName, IndexType rowIdx, LPCTSTR strString)
{
   WriteStringToCell(hTable, strRangeName, rowIdx, strString);
}

BOOL CTxCSVDataExporter::Commit(LPCTSTR strFilename)
{
   // write the data into a text file
   std::_tofstream ofile(strFilename);

   if ( !ofile )
      return FALSE;

   // Write out tables
   for (IndexType table = 0; table < nTables; table++)
   {
      size_t ncols = m_Tables[table].m_Columns.size();

      for (IndexType row = 0; row < m_MaxRows[table]; row++)
      {
         size_t icol = 0;
         for (auto& column : m_Tables[table].m_Columns)
         {
            size_t nrows = column.size();
            if (row < nrows)
            {
               const std::_tstring& rstr = column[row];
               ofile << rstr.c_str();
            }

            // Don't put comma after last row
            if (icol+1 < ncols)
            {
               ofile << m_Separator;
            }

            icol++;
         }

         ofile << std::endl;
      }

      // Write four blank lines between each table
      if (table+1 < nTables)
      {
         ofile << std::endl << std::endl << std::endl << std::endl;
      }
   }

   ofile.close();

   return TRUE;
}

void CTxCSVDataExporter::Fail()
{
}
