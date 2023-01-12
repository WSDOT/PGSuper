///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include "TxExcel.h"

class CTxExcelDataExporter : public CTxDataExporter
{
public:
   CTxExcelDataExporter(void);
   ~CTxExcelDataExporter(void);

   // CTxDataExporter virtual overrides
   // write to cell using named range and row offset
   virtual void WriteStringToCell(IndexType hTable, LPCTSTR strRangeName, IndexType rowIdx, LPCTSTR strString) override;
   virtual void WriteWarningToCell(IndexType hTable, LPCTSTR strRangeName, IndexType rowIdx, LPCTSTR strString) override;

   virtual BOOL Commit(LPCTSTR strFilename) ;
   virtual void Fail() ;

   void WriteStringToCell(IndexType hTable, IndexType rowIdx, IndexType colIdx, LPCTSTR strString);
   virtual void SetColumnData(IndexType hTable,LPCTSTR strColumnHeading,ColumnIndexType colIdx,const std::vector<Float64>& values) ;

   // Our functions
   virtual BOOL InitializeExporter(LPCTSTR strTemplateName);

private:
   Range GetRangeAtLocation(IndexType hTable, LPCTSTR strRangeName, IndexType rowIdx);

   _Application m_Excel;
   Worksheets m_Worksheets;
};
