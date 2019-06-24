#pragma once

////////////////////////////////////////////////////
// CTxDataExporter
//
// Abstract base class for a data exporter
class CTxDataExporter
{
public:
   CTxDataExporter(void);
   virtual ~CTxDataExporter(void);

   // Called by the framework when the data exporter is created
   virtual BOOL InitializeExporter() = 0;

   // Called by the framework when a table needs to be created. The table handle is returned
   virtual IndexType CreateTable(LPCTSTR strName) = 0;

   // Puts a column of data into a table
   virtual void SetColumnData(IndexType hTable,LPCTSTR strColumnHeading,ColumnIndexType colIdx,const std::vector<Float64>& values) = 0;

   // commits the data to a file
   virtual BOOL Commit(LPCTSTR strFilename) = 0;

   // called if the data export fails.
   virtual void Fail() = 0;
};
