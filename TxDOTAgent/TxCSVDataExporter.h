///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#pragma once
#include "TxDataExporter.h"

// CCsvTable
// Helper data struct for CSV tabular data
struct CCsvTable
{
   typedef std::map< std::_tstring, IndexType > ColumnMapNameType;
   ColumnMapNameType m_ColumnNameMap; // index points to column

   typedef std::vector< std::vector<std::_tstring> > ColumnsType;
   ColumnsType m_Columns;
};

//////////////////////////////////////////
// CTxCSVDataExporter
//
// Exports analysis results in comma seperated value (CSV) format
class CTxCSVDataExporter :
   public CTxDataExporter
{
public:
   CTxCSVDataExporter(void);
   ~CTxCSVDataExporter(void);

   // CTxDataExporter virtual overrides
   void WriteStringToCell(IndexType hTable, LPCTSTR strRangeName, IndexType rowIdx, LPCTSTR strString) override;
   virtual void WriteWarningToCell(IndexType hTable, LPCTSTR strRangeName, IndexType rowIdx, LPCTSTR strString) override;
   virtual BOOL Commit(LPCTSTR strFilename) override;
   virtual void Fail() override;

   // Intialize table column names.
   BOOL InitializeTable(IndexType hTable, const std::vector<std::_tstring>& columnNames);

   // By default this class separates values using commas. This can be changed by setting below
   void SetSeparator(LPCTSTR strSeparator);

private:
   // Only allow two tables (could use vector but don't need the overhead for a feature we'll never use)
   static const IndexType nTables = 2;
   CCsvTable m_Tables[nTables];

   IndexType m_MaxRows[nTables]; //  length of longest column

   std::_tstring m_Separator;
};
