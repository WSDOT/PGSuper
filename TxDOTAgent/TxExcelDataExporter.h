#pragma once
#include "TxExcel.h"

class CTxExcelDataExporter //  : public CDataExporter
{
public:
   CTxExcelDataExporter(void);
   ~CTxExcelDataExporter(void);

   virtual BOOL InitializeExporter(LPCTSTR strTemplateName);

   void WriteStringToCell(IndexType hTable, IndexType rowIdx, IndexType colIdx, LPCTSTR strString);

   // using named range and row offset
   void WriteStringToCell(IndexType hTable, LPCTSTR strRangeName, IndexType rowIdx, LPCTSTR strString);

   virtual void SetColumnData(IndexType hTable,LPCTSTR strColumnHeading,ColumnIndexType colIdx,const std::vector<Float64>& values) ;
   virtual BOOL Commit(LPCTSTR strFilename) ;
   virtual void Fail() ;

private:

   _Application m_Excel;
   Worksheets m_Worksheets;
};
