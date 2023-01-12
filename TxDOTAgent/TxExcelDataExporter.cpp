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
#include "StdAfx.h"
#include "TxExcelDataExporter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

   // see http://support.microsoft.com/kb/186120
   // http://support.microsoft.com/kb/186122  How to Use MFC to Automate Excel and Obtain an Array from a Range
   // http://support.microsoft.com/kb/184633  How to Embed and Automate a Microsoft Excel Worksheet with MFC
   // http://support.microsoft.com/kb/179706  Use MFC to Automate Excel and Create/Format a New Workbook
   // http://support.microsoft.com/kb/186783  How to Use MFC to Create a Microsoft Excel Chart

CString GetColumnLabel(IndexType colIdx)
{
   CString strLabel((TCHAR)((colIdx % 26) + _T('A')));
   colIdx = ((colIdx - (colIdx % 26))/26);
   if ( 0 < colIdx )
   {
      CString strTemp = strLabel;
      strLabel = GetColumnLabel(colIdx-1);
      strLabel += strTemp;
   }

   return strLabel;
}

CString GetRowColLabel(IndexType rowIdx,IndexType colIdx)
{
   CString Colnam = GetColumnLabel(colIdx);

   CString str;
   str.Format(_T("%s%d"),Colnam, rowIdx);

   return str;
}


// Some constants to make the IDispatch calls easier
COleVariant ovOptional((long)DISP_E_PARAMNOTFOUND,VT_ERROR);  // optional parameter
COleVariant ovTrue((short)TRUE); // true
COleVariant ovFalse((short)FALSE); // false

CTxExcelDataExporter::CTxExcelDataExporter(void)
{
}

CTxExcelDataExporter::~CTxExcelDataExporter(void)
{
}

BOOL CTxExcelDataExporter::InitializeExporter(LPCTSTR strTemplateName)
{
   if ( !m_Excel.CreateDispatch(_T("Excel.Application")) )
   {
      AfxMessageBox(_T("An error occured while attempting to run Excel. Excel files cannot be created unless Microsoft Excel is installed. Maybe try a .csv file?"));
      return FALSE;
   }

   // get the spreadsheet setup
   Workbooks workbooks = m_Excel.GetWorkbooks();
   _Workbook workbook = workbooks.Add(COleVariant(strTemplateName));

   m_Worksheets = workbook.GetWorksheets();

   long count = m_Worksheets.GetCount();

   return TRUE;
}

void CTxExcelDataExporter::WriteStringToCell(IndexType hTable, IndexType rowIdx, IndexType colIdx, LPCTSTR strString)
{
   _Worksheet ws = m_Worksheets.GetItem(COleVariant((long)hTable));

   CString strCell = GetRowColLabel(rowIdx, colIdx);
   Range range = ws.GetRange(COleVariant(strCell),COleVariant(strCell));

   range.SetValue2(COleVariant(strString));
}

Range CTxExcelDataExporter::GetRangeAtLocation(IndexType hTable, LPCTSTR strRangeName, IndexType rowIdx)
{
   _Worksheet ws = m_Worksheets.GetItem(COleVariant((long)hTable));

   Range range;
   try
   {
      range = ws.GetRange(COleVariant(strRangeName), COleVariant(strRangeName));
   }
//   catch(CException* e) 
   catch(...) 
   {
      CString msg;
      msg.Format(_T("Error - The range \'%s\' was not found on sheet %d of the template spreadsheet."), strRangeName, hTable);
      ::AfxMessageBox(msg);
      ATLASSERT(0);
      throw;
   }  

   // Get the address of the range in absolute excel format. will look something like: "'[CADExport-Straight]StandardPattern'!$A$5"
   CString address = range.GetAddressLocal(CComVariant(TRUE), CComVariant(TRUE), 1, CComVariant(TRUE), CComVariant());

   // Strip out the file and worksheet names. range name can only contain like $A$4 in the GetRange call
   int cnt = address.GetLength();
   int pn = address.ReverseFind(_T('!'));

   CString trunc_address = address.Right(cnt - pn - 1);

   cnt = trunc_address.GetLength();

   // The row number is the last number in the address
   pn = trunc_address.ReverseFind(_T('$'));

   CString strrow = trunc_address.Right(cnt - pn - 1);

   long rownum;
   if (sysTokenizer::ParseLong(strrow, &rownum)) 
   {
      rownum += (long)rowIdx;

      CString strrow;
      strrow.Format(_T("%d"), rownum);

      // Build range name using new row number
      CString strNewRow = trunc_address.Left(pn+1) + strrow;

      try
      {
         Range rangenew = ws.GetRange(COleVariant(strNewRow), COleVariant(strNewRow));

         return rangenew;
      }
      catch(...) 
      {
         CString msg;
         msg.Format(_T("Error - The range \'%s\' at row %d was not found on sheet %d of the template spreadsheet."), strRangeName, rownum, hTable);
         ::AfxMessageBox(msg);
         ATLASSERT(0);
         throw;
      }  
   }
   else
   {
      ATLASSERT(0);
      throw;
   }
}


void CTxExcelDataExporter::WriteStringToCell(IndexType hTable, LPCTSTR strRangeName, IndexType rowIdx, LPCTSTR strString)
{
   try
   {
      Range range = CTxExcelDataExporter::GetRangeAtLocation(hTable, strRangeName, rowIdx);
      range.SetValue2(COleVariant(strString));
   }
   catch(...) 
   {
      return;
   }  
}

void CTxExcelDataExporter::WriteWarningToCell(IndexType hTable, LPCTSTR strRangeName, IndexType rowIdx, LPCTSTR strString)
{
   try
   {
      Range range = CTxExcelDataExporter::GetRangeAtLocation(hTable, strRangeName, rowIdx);

      range.SetValue2(COleVariant(strString));

      // warnings are red
      _Font font = range.GetFont();
      font.SetColor(COleVariant((long)RGB(200, 0, 0)));
      
      range.SetWrapText(COleVariant(VARIANT_FALSE));
      range.SetHorizontalAlignment(COleVariant((LONGLONG)-4131)); // align left
   }
   catch(...) 
   {
      return;
   }  
}

void CTxExcelDataExporter::SetColumnData(IndexType hTable,LPCTSTR strColumnHeading,ColumnIndexType colIdx,const std::vector<Float64>& values)
{
   _Worksheet ws = m_Worksheets.GetItem(COleVariant((long)hTable));

   // write the column heading
   CString strCell;
   strCell.Format(_T("%s2"),GetColumnLabel(colIdx));

   Range cell = ws.GetRange(COleVariant(strCell),COleVariant(strCell));
   cell.SetValue2(COleVariant(strColumnHeading));

   // create a range that will hold all the data points
   strCell.Format(_T("%s3"),GetColumnLabel(colIdx));
   Range range = ws.GetRange(COleVariant(strCell),COleVariant(strCell));
   range = range.GetResize(COleVariant( (short)values.size() ), COleVariant((short)1) );

   // create a safe array to hold the data points before putting them into excel
   COleSafeArray array;
   DWORD numElements[2];
   numElements[0] = (DWORD)values.size(); // number of rows
   numElements[1] = 2; // number of columns

   array.Create(VT_R8,2,numElements); // 1 dim array of doubles


   // fill up the array
   long index[2];
   long row = 0;

   std::vector<Float64>::const_iterator iter;
   for ( iter = values.begin(); iter != values.end(); iter++ )
   {
      Float64 value = *iter;
      
      index[0] = row++;
      index[1] = 0;

      array.PutElement(index,&value);
   }

   // pump the array into the range in excel
   range.SetValue2(COleVariant(array));
   array.Detach(); // done with the array
}

BOOL CTxExcelDataExporter::Commit(LPCTSTR strFilename)
{
   // save the spreadsheet
   _Worksheet ws;
   TRY
   {
      ws = m_Worksheets.GetItem(COleVariant(1L));
      ws.SaveAs(strFilename,ovOptional,ovOptional,ovOptional,ovOptional,ovOptional,ovOptional,ovOptional,ovOptional,ovOptional);
   }
   CATCH(COleDispatchException,pException)
   {
      return FALSE;
   }
   END_CATCH
   
   // select the first worksheet
   ws.Select(ovTrue); 

   // show Excel
   m_Excel.SetVisible(TRUE);

   return TRUE;
}

void CTxExcelDataExporter::Fail()
{
  m_Excel.Quit();
}