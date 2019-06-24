#pragma once
#include "TxDataExporter.h"

// CTable
// Helper data struct for CSV tabular data
struct CTable
{
   std::_tstring m_strName;
   std::vector<std::_tstring> m_ColumnHeadings;
   std::vector< std::vector<Float64> > m_Values;
};

//////////////////////////////////////////
// CTxCSVDataExporter
//
// Exports analysis results in command seperated value (CSV) format
class CTxCSVDataExporter :
   public CTxDataExporter
{
public:
   CTxCSVDataExporter(void);
   ~CTxCSVDataExporter(void);

   virtual BOOL InitializeExporter() override;
   virtual IndexType CreateTable(LPCTSTR strName) override;
   virtual void SetColumnData(IndexType hTable,LPCTSTR strColumnHeading,ColumnIndexType colIdx,const std::vector<Float64>& values) override;
   virtual BOOL Commit(LPCTSTR strFilename) override;
   virtual void Fail() override;

private:
   typedef std::map<IndexType,CTable> Tables;
   Tables m_Tables;
};
