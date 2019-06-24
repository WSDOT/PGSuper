#include "StdAfx.h"
#include "TxCSVDataExporter.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTxCSVDataExporter::CTxCSVDataExporter(void)
{
}

CTxCSVDataExporter::~CTxCSVDataExporter(void)
{
}

BOOL CTxCSVDataExporter::InitializeExporter()
{
   // no special initialization required
   return TRUE;
}

IndexType CTxCSVDataExporter::CreateTable(LPCTSTR strName)
{
   IndexType hTable = m_Tables.size();
   CTable table;
   table.m_strName = strName;
   m_Tables.insert(std::make_pair(hTable,table));
   return hTable;
}

void CTxCSVDataExporter::SetColumnData(IndexType hTable,LPCTSTR strColumnHeading,ColumnIndexType colIdx,const std::vector<Float64>& values)
{
   Tables::iterator found = m_Tables.find(hTable);
   ATLASSERT( found != m_Tables.end() );

   CTable& table = found->second;

   // must added column data left to right
   ATLASSERT(table.m_ColumnHeadings.size() == colIdx);
   ATLASSERT(table.m_Values.size() == colIdx);

   // Need to replace any commas in heading with :'s
   std::_tstring header(strColumnHeading);
   std::replace(header.begin(), header.end(), _T(','), _T(':'));

   table.m_ColumnHeadings.push_back(std::_tstring(header));
   table.m_Values.push_back(values);
}

BOOL CTxCSVDataExporter::Commit(LPCTSTR strFilename)
{
   // write the data into a text file
   std::_tofstream ofile(strFilename);

   if ( !ofile )
      return FALSE;

   Tables::iterator iter;
   for ( iter = m_Tables.begin(); iter != m_Tables.end(); iter++ )
   {
      CTable& table = iter->second;
      ofile << table.m_strName << std::endl;

      // write column headings
      std::vector<std::_tstring>::iterator colIter;
      for ( colIter = table.m_ColumnHeadings.begin(); colIter != table.m_ColumnHeadings.end(); colIter++ )
      {
         const std::_tstring& str = *colIter;

         if ( colIter != table.m_ColumnHeadings.begin() )
            ofile << ",";

         ofile << str.c_str();
      }
      ofile << std::endl;

      // write values
      RowIndexType nRows = table.m_Values[0].size(); // assume all vectors are same length
      for ( RowIndexType row = 0; row < nRows; row++ )
      {
         std::vector< std::vector<Float64> >::iterator valIter;
         for ( valIter = table.m_Values.begin(); valIter != table.m_Values.end(); valIter++ )
         {
            const std::vector<Float64>& values = *valIter;

            if ( valIter != table.m_Values.begin() )
               ofile << ",";

            ofile << values[row];
         }
         ofile << std::endl;
      }

      ofile << std::endl;
   }

   ofile.close();

   return TRUE;
}

void CTxCSVDataExporter::Fail()
{
}
