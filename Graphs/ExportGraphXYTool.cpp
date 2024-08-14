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

#include "stdafx.h"
#include "resource.h"
#include <Graphs\ExportGraphXYTool.h>

#include <Units\UnitValueNumericalFormatTools.h>
#include <MfcTools\ExcelWrapper.h>
#include <Graphing\GraphXY.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// utility functions
static bool DoesFileExist(const CString& filename)
{
   if (filename.IsEmpty())
      return false;
   else
   {
      CFileFind finder;
      BOOL is_file;
      is_file = finder.FindFile(filename);
      return (is_file != 0);
   }
}

struct DataSeriesNameHolder
{
   std::_tstring name;
   mutable std::size_t vectorIndex1; // some series have two series (e.g., min,max). 
   mutable std::size_t vectorIndex2;
   int penStyle,penWidth;
   COLORREF color;

   DataSeriesNameHolder() : vectorIndex1(INVALID_INDEX),vectorIndex2(INVALID_INDEX) {;}

   bool operator<(const DataSeriesNameHolder& other) const { return penStyle+penWidth+color < other.penStyle + other.penWidth + other.color; }
   bool operator==(const DataSeriesNameHolder& other) const { return penStyle == other.penStyle && penWidth == other.penWidth && color == other.color; }
};

bool compare_point (const WBFL::Graphing::Point& i, const WBFL::Graphing::Point& j)
{
   return i.X() < j.X();
}


// Utility class to convert xygraph data into a format we can export.
class GraphExportUtil
{
public:
   GraphExportUtil(bool bAllowDuplicatePoints):
      m_bAllowDuplicatePoints(bAllowDuplicatePoints)
   { ; }

   bool ProcessGraph(const WBFL::Graphing::GraphXY& rGraph);
   bool ExportToExcel(CString filename);
   bool ExportToCSV(CString filename);

private:

   void CleanupSeriesData();

   // Write to Excel
   bool CommitExcel(_Application& excel, Worksheets& worksheets, LPCTSTR strFilename);
   void SetColumnData(Worksheets& worksheets, IndexType hTable,LPCTSTR strColumnHeading,ColumnIndexType colIdx,const std::vector<Float64>& values);
   CString GetColumnLabel(ColumnIndexType colIdx);


private:
   GraphExportUtil();

      // Raw data
   CString m_Title;
   CString m_SubTitle;
   CString m_YTitle;

   bool m_bAllowDuplicatePoints;

   struct SeriesType
   {
      std::vector<Float64> Values;
   };

   // X data stored in first member
   std::vector<CString> m_SeriesNames;
   std::vector< std::vector<Float64> > m_SeriesData;
};


/// <summary>
/// ////////////// Our main class
/// </summary>
CExportGraphXYTool::CExportGraphXYTool()
{
}

CExportGraphXYTool::~CExportGraphXYTool()
{
}

bool CExportGraphXYTool::ExportGraphData(const WBFL::Graphing::GraphXY& rGraph,LPCTSTR strDefaultFileName, bool bAllowDuplicatePoints)
{
   if (rGraph.GetDataSeriesCount() == 0)
   {
      ::AfxMessageBox(_T("No data is selected. There is nothing to export. Please select something to graph and try again."), MB_OK);
      return false;
   }

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Deal with file create name
   CString default_name(strDefaultFileName);
   TCHAR strFilter[] = _T("Excel Worksheet (*.xlsx)|*.xlsx|") _T("Comma Separated Value text File (*.csv)|*.csv|");
   TCHAR strSuffix[] = _T("xlsx") _T("csv");
   enum enSuffixIdx { esiExcel=1,esiCSV } suffIdx(esiExcel);

   // Create file dialog
   CString file_path;
   CFileDialog  fildlg(FALSE,strSuffix,default_name,OFN_HIDEREADONLY,strFilter);
   if (fildlg.DoModal() == IDOK)
   {
      // Get full pathname of selected file 
      file_path = fildlg.GetPathName();
      suffIdx = (enSuffixIdx)fildlg.m_ofn.nFilterIndex; // file type selected in dialog

      // See if the file currently exists 
      if (DoesFileExist(file_path))
      {
         // See if the user wants to overwrite file 
         CString msg(_T(" The file: "));
         msg += file_path + _T(" exists. Overwrite it?");
         int stm = AfxMessageBox(msg,MB_YESNOCANCEL | MB_ICONQUESTION);
         if (stm != IDYES)
         {
            return true;
         }
         else
         {
            if (0 == ::DeleteFile(file_path))
            {
               CString errMsg = CString(_T("Error deleting the file: \" ")) + file_path + CString(_T(" \". Could it be open in another program (e.g., Excel)? Export cannot continue."));
               ::AfxMessageBox(errMsg);
               return false;
            }
         }
      }
   }
   else
   {
      return false;
   }

   // We have the information we need. Process graph data into the structure we need and then write it to file
   GraphExportUtil graphUtil(bAllowDuplicatePoints);
   if (!graphUtil.ProcessGraph(rGraph))
   {
      ::AfxMessageBox(_T("Error Processing Graph Data. This is a permanent error - cannot continue export"),MB_OK | MB_ICONEXCLAMATION);
      return false;
   }

   if (suffIdx == esiExcel)
   {
      graphUtil.ExportToExcel(file_path);
   }
   else if (suffIdx == esiCSV)
   {
      graphUtil.ExportToCSV(file_path);
   }
   else
   {
      ATLASSERT(0); // should never happen
      CString msg;
      msg.Format(_T("Invalid File Type Specified. File suffix must be either \".xlsx\" or \".csv\". Name specified was \"%s\"."),file_path);
      ::AfxMessageBox(msg,MB_OK | MB_ICONERROR);
      return false;
   }

   return true;
}

CString CExportGraphXYTool::GetTruncatedFileName()
{
   // Build default file name
   CString strProjectFileNameNoPath;
   CEAFDocument* pDoc = EAFGetDocument();
   CString strProjectFileName = pDoc->GetPathName();
   if (strProjectFileName.IsEmpty())
   {
      strProjectFileNameNoPath = _T("Untitled");
   }
   else
   {
      CString strProjectFileNameNoExt(strProjectFileName.Left(strProjectFileName.ReverseFind('.')));
      int loc = strProjectFileNameNoExt.ReverseFind('\\');
      if (-1 == loc)
      {
         strProjectFileNameNoPath = strProjectFileNameNoExt;
      }
      else
      {
         strProjectFileNameNoPath = strProjectFileNameNoExt.Right(strProjectFileNameNoExt.GetLength() - (loc+1));
      }
   }

   return strProjectFileNameNoPath;
}

/// //////////// 
/// GraphExportUtil
/// <param name="rGraph"></param>
bool GraphExportUtil::ProcessGraph(const WBFL::Graphing::GraphXY& rGraph)
{
   m_Title = rGraph.GetTitle();
   m_SubTitle = rGraph.GetSubtitle();
   m_YTitle = rGraph.GetYAxisTitle();

   // One challenging part here is that grGraphXY's data series are each based on unique X Values. 
   // We want our export to be based on a single, common X vector. So we need to resolve this.
   struct XValue
   {
      XValue(Float64 val) : Value(val), bIsDouble(false)
      { ; }

      Float64 Value;
      mutable bool bIsDouble; // One hard part is we can have two points at one X location to signify a jump (e.g., shear jump)
      bool operator<(const XValue& other) const  { return Value < other.Value; }
      bool operator==(const XValue& other) const { return Value == other.Value;}
   };

   // Build a list of all X Values in all series. Catch any jumps (duplicate X values), even those between series
   std::set<XValue> xValueSet;

   Float64 lastVal(Float64_Max);
   std::vector<IndexType> cookies = rGraph.GetCookies();
   for (const auto& cookie : cookies)
   {
      std::vector<WBFL::Graphing::Point> vPoints;
      rGraph.GetDataSeriesPoints(cookie, &vPoints);
      std::stable_sort(vPoints.begin(),vPoints.end(),compare_point); // There are some cases where points are not sorted properly

      for (const auto& point : vPoints)
      {
         Float64 xval = point.X();
         bool isDouble = xval == lastVal;

         XValue xvalue(xval);
         auto found = xValueSet.find(xvalue);
         if (found == xValueSet.end())
         {
            xValueSet.insert(xvalue);
         }
         else if (isDouble)
         {
            (*found).bIsDouble = true;
         }

         lastVal = xval;
      }
   }

   // Build a list of all possible X values including duplicates. This will be our X axis
   m_SeriesNames.push_back(rGraph.GetXAxisTitle());
   m_SeriesData.push_back(std::vector<Float64>());
   auto& datavecx = m_SeriesData.back();
   datavecx.reserve(xValueSet.size()*2);
   for (const auto& xvalue : xValueSet)
   {
      datavecx.push_back(xvalue.Value);
      if (xvalue.bIsDouble)
      {
         datavecx.push_back(xvalue.Value);
      }
   }

   // Below is some ugly code to deal with ugly data permutations.
   // Data coming out of the Graphxy class can be screwy. Typically multi-segment graphs are built using
   // one series per segment, with the first series named and subsequent series being nameless, but with the 
   // same brush data. We need to find these cases and link them together into a single series so we can have clean data
   // Also, series often contain redundant data points. We'll clean those up too
   std::vector< std::vector<WBFL::Graphing::Point> > rawSeriesGraphPoints;

   // Use the set below to help connect the series together
   std::set<DataSeriesNameHolder> dataSeriesNames;

   for (const auto& cookie : cookies)
   {
      std::vector<WBFL::Graphing::Point> vOriginalPoints;
      rGraph.GetDataSeriesPoints(cookie,&vOriginalPoints);

      std::stable_sort(vOriginalPoints.begin(),vOriginalPoints.end(),compare_point); // There are some cases where points are not sorted properly

      std::vector<WBFL::Graphing::Point> cleanPoints;
      cleanPoints.reserve(vOriginalPoints.size());

      IndexType numDups = 0;
      Float64 lastVal(Float64_Max);
      for (const auto point : vOriginalPoints)
      {
         Float64 X = point.X();
         if (X != lastVal)
         {
            cleanPoints.push_back(point);
            numDups = 0;
            lastVal = X;
         }
         else
         {
            if (numDups == 0)
            {
               cleanPoints.push_back(point); // We can have only one duplicate
            }

            numDups++;
         }
      }

      DataSeriesNameHolder dsn;
      rGraph.GetDataSeriesData(cookie,&dsn.name,&dsn.penStyle,&dsn.penWidth,&dsn.color);
      if (!dsn.name.empty())
      {
         // Series has a name - save it
         dsn.vectorIndex1 = rawSeriesGraphPoints.size();
         dataSeriesNames.insert(dsn);

         m_SeriesNames.push_back(dsn.name.c_str());
         rawSeriesGraphPoints.emplace_back(cleanPoints);
      }
      else
      {
         // No name. try to find slot based on graph data
         auto iter = dataSeriesNames.find(dsn);
         if (iter != dataSeriesNames.end())
         {
            // Found matching series. Append current series to it and make jump if applicable
            // 
            // Also deal with having possible two series (e.g., top max, min stress). See if data fits in first slot
            std::size_t idx1 = iter->vectorIndex1;

            Float64 endxval = rawSeriesGraphPoints[idx1].back().X();

            if (endxval <= cleanPoints.front().X())
            {
               auto iterc = cleanPoints.begin();
               if (endxval == cleanPoints.front().X())
               {
                  // start of current series exactly at end. See if it can jump
                  auto found = xValueSet.find(endxval);
                  bool isdup = found != xValueSet.end() && found->bIsDouble; // is dual value at end of previous series
                  if (isdup)
                  {
                     // We have a jump. clear the way. remove any duplicate at ends of adjacent series
                     auto iterc2 = iterc;
                     iterc2++;
                     if (iterc2->X() == iterc->X())
                     {
                        iterc++;
                     }

                     auto iterrw = rawSeriesGraphPoints[idx1].end();
                     iterrw--; iterrw--; // iterate to before last item
                     if (iterrw->X() == rawSeriesGraphPoints[idx1].back().X())
                     {
                        rawSeriesGraphPoints[idx1].pop_back();
                     }
                  }
               }
               else
               {
                  // no jump. clean off start of current
                  while (rawSeriesGraphPoints[idx1].back().X() == iterc->X())
                  {
                     iterc++;
                  }
               }

               rawSeriesGraphPoints[idx1].insert(rawSeriesGraphPoints[idx1].end(), iterc, cleanPoints.end());
            }
            else
            {
               if (iter->vectorIndex2 == INVALID_INDEX)
               {
                  // first time second vector has been used
                  iter->vectorIndex2 = rawSeriesGraphPoints.size();
                  CString name2 = iter->name.c_str() + CString(" (2)");
                  m_SeriesNames.push_back(name2);  // use same name as first series with 2 app'd
                  rawSeriesGraphPoints.emplace_back(cleanPoints);
               }
               else
               {
                  // append to list as long as data in correct order
                  std::size_t idx2 = iter->vectorIndex2;

                  Float64 endxval = rawSeriesGraphPoints[idx2].back().X();

                  if (endxval <= cleanPoints.front().X())
                  {
                     auto iterc = cleanPoints.begin();
                     if (endxval == cleanPoints.front().X())
                     {
                        // start of current series exactly at end. See if it can jump
                        auto found = xValueSet.find(endxval);
                        bool isdup = found != xValueSet.end() && found->bIsDouble; // is dual value at end of previous series
                        if (isdup)
                        {
                           // We have a jump. clear the way. remove any duplicate at ends of adjacent series
                           auto iterc2 = iterc;
                           iterc2++;
                           if (iterc2->X() == iterc->X())
                           {
                              iterc++;
                           }

                           auto iterrw = rawSeriesGraphPoints[idx2].end();
                           iterrw--; iterrw--; // iterate to before last item
                           if (iterrw->X() == rawSeriesGraphPoints[idx2].back().X())
                           {
                              rawSeriesGraphPoints[idx2].pop_back();
                           }
                        }
                     }
                     else
                     {
                        // no jump. clean off start of current
                        while (rawSeriesGraphPoints[idx2].back().X() == iterc->X())
                        {
                           iterc++;
                        }
                     }

                     rawSeriesGraphPoints[idx2].insert(rawSeriesGraphPoints[idx2].end(), iterc, cleanPoints.end());
                  }
                  else
                  {
                     ATLASSERT(0); // more than two series with same properties. Didn't expect this.
                  }
               }
            }
         }
         else
         {
            // We have an independent series with no name. Just add it
            m_SeriesNames.push_back(CString(_T("Data")));
            rawSeriesGraphPoints.emplace_back(cleanPoints);
         }
      }
   }

   // Now we have cleaned up series data. We can add to main data structure making sure there is data for all X locations
   for (const auto& vPoints : rawSeriesGraphPoints)
   {
      // Create series
      m_SeriesData.push_back(std::vector<Float64>());
      auto& datavec = m_SeriesData.back();
      datavec.reserve(m_SeriesData.front().size());

      // Need to create a Y val for every X in xValueSet regardless if there is a location in Vpoints to match
      Float64 lastYVal(Float64_Max);
      auto pointIter = vPoints.begin();
      for (const auto& xvalue : xValueSet)
      {
         if (pointIter == vPoints.end())
         {
            // XValues extend beyond end of points. Use last Y value
            datavec.push_back(lastYVal);

            if (xvalue.bIsDouble)
            {
               datavec.push_back(lastYVal);
            }
         }
         else
         {
            lastYVal = pointIter->Y();

            if (xvalue.Value == pointIter->X())
            {
               // easiest case. locations match
               Float64 yVal = pointIter->Y();
               datavec.push_back(yVal);

               pointIter++;

               if (xvalue.bIsDouble)
               {
                  // there is a jump in seriers
                  if (pointIter != vPoints.end())
                  {
                     datavec.push_back(pointIter->Y());
                     pointIter++;
                  }
                  else
                  {
                     datavec.push_back(yVal);
                  }
               }
            }
            else if (xvalue.Value < pointIter->X())
            {
               // We have an X location where there is no point. Need to interpolate between bracketing points
               Float64 xValRight = pointIter->X();
               Float64 yValRight = pointIter->Y();
               if (pointIter != vPoints.begin())
               {
                  auto prevPointIter = pointIter;
                  prevPointIter--; // back up
                  Float64 xValLeft = prevPointIter->X();
                  Float64 yValLeft = prevPointIter->Y();

                  Float64 YatX = LinInterpLine(xValLeft,yValLeft,xValRight,yValRight,xvalue.Value);
                  datavec.push_back(YatX);

                  if (xvalue.bIsDouble)
                  {
                     // there is a jump in seriers
                     datavec.push_back(YatX);
                  }
               }
               else
               {
                  datavec.push_back(pointIter->Y());

                  if (xvalue.bIsDouble)
                  {
                     // there is a jump in seriers
                     datavec.push_back(pointIter->Y());
                  }
               }
            }
            else
            {
               ATLASSERT(0);
               return false;
            }
         }

      } // xvalue loop
   }

   // Now we have a collection of series of the same length. GraphXY can contain unnecessary duplicate values.
   // Let's clean up the garbage if the client asks
   if (!m_bAllowDuplicatePoints)
   {
      CleanupSeriesData();
   }

   return true;
}

void GraphExportUtil::CleanupSeriesData()
{
   if (m_SeriesData.empty())
   {
      ATLASSERT(0); // Should never get here
      return;
   }

   std::size_t numSeries = m_SeriesData.size();

   // create new data structure to put our cleaned data
   std::vector<std::vector<Float64>> newSeriesData;
   newSeriesData.reserve(m_SeriesData.size());
   for (std::size_t iser=0; iser<numSeries; iser++)
   {
      newSeriesData.push_back(std::vector<Float64>());
      newSeriesData.back().reserve(numSeries);
   }

   // Add first row
   std::size_t icol = 0;
   for (auto& series : m_SeriesData)
   {
      if (!series.empty())
      {
         newSeriesData[icol].push_back(series.front());
      }
      else
      {
         newSeriesData[icol].push_back(0.0); // some graphs have no data - just make something up
      }

      icol++;
   }

   // Find any rows where data is duplicate to the next row and skip that row
   bool didClean(false);
   std::size_t numRows = m_SeriesData.front().size(); 
   for (std::size_t irow = 1; irow < numRows; irow++)
   {
      std::size_t iprevrow = irow - 1;

      // if any values are different, then series are different
      bool areDifferent = false;
      for (auto& series : m_SeriesData)
      {
         if (!IsEqual(series.at(iprevrow), series.at(irow)))
         {
            areDifferent = true;
            break;
         }
      }

      if (areDifferent)
      {
         for (std::size_t icol = 0; icol < numSeries; icol++)
         {
            newSeriesData[icol].push_back(m_SeriesData[icol].at(irow));
         }
      }
      else
      {
         didClean = true;
      }
   }

   if (didClean)
   {
      m_SeriesData = newSeriesData;
   }
}

// some useful idispatch variables
COleVariant ovOptional((long)DISP_E_PARAMNOTFOUND,VT_ERROR);  // optional parameter
COleVariant ovTrue((short)TRUE); // true
COleVariant ovFalse((short)FALSE); // false

bool GraphExportUtil::ExportToExcel(CString fileName)
{
   // Excel
   _Application excel;
   if (!excel.CreateDispatch(_T("Excel.Application")))
   {
      AfxMessageBox(_T("An error occured while attempting to run Excel. Excel files cannot be created unless Microsoft Excel is installed. Maybe try a .csv file?"));
      return FALSE;
   }

   // Set up the spreadsheet
   Workbooks workbooks = excel.GetWorkbooks();
   _Workbook workbook = workbooks.Add(ovOptional);
   Worksheets worksheets = workbook.GetWorksheets();

   // Delete "Sheet2" and "Sheet3"
   // Must leave "Sheet1"
   long count = worksheets.GetCount();
   for (long i = count; 1 < i; i--)
   {
      _Worksheet ws = worksheets.GetItem(COleVariant(i));
      ws.Delete();
   }

   // Name worksheet
   _Worksheet ws = worksheets.GetItem(COleVariant(1L));
   ws.SetName(_T("Graph Data"));

   // write the table headings
   CString strCell;
   strCell.Format(_T("%s1"),GetColumnLabel(0));
   Range cell = ws.GetRange(COleVariant(strCell),COleVariant(strCell));
   cell.SetValue2(COleVariant(m_Title));

   strCell.Format(_T("%s2"),GetColumnLabel(0));
   Range cell2 = ws.GetRange(COleVariant(strCell),COleVariant(strCell));
   cell2.SetValue2(COleVariant(m_SubTitle));

   strCell.Format(_T("%s3"),GetColumnLabel(0));
   Range cell3 = ws.GetRange(COleVariant(strCell),COleVariant(strCell));
   cell3.SetValue2(COleVariant(m_YTitle));

   // Write data
   int colidx = 0;
   for (const auto& series : m_SeriesData)
   {
      SetColumnData(worksheets, 1, m_SeriesNames[colidx], colidx, series);
      colidx++;
   }

   // Save file
   while (!CommitExcel(excel, worksheets, fileName))
   {
      CString strMsg;
      CFileStatus status;
      CFile::GetStatus(fileName,status);
      if (status.m_attribute & CFile::readOnly)
      {
         strMsg.Format(_T("Cannot save the file %s\r\nThe file exists and is marked Read-Only.\r\nClick Ok to enter another file name, or Cancel."),fileName);
      }
      else
      {
         strMsg.Format(_T("Unable to save %s because it is already open. \r\nClick Ok to enter another file name, or Cancel."),fileName);
      }

      int st = AfxMessageBox(strMsg,MB_OKCANCEL);

      if (st == IDOK)
      {

         // user wants to rename the file
         CString strExcelFilter(_T("Microsoft Excel Files (*.xls)"));
         CString strFileExtension(_T("xls"));

         CFileDialog fileDlg2(FALSE,strFileExtension,fileName,OFN_HIDEREADONLY,strExcelFilter);
         fileDlg2.DoModal();
         fileName = fileDlg2.GetPathName();
      }
   }

   return true;
}

bool GraphExportUtil::ExportToCSV(CString filename)
{
   std::_tofstream ofile(filename.GetBuffer());

   CString title (m_Title);
   title.Replace(',',';'); // Title typically has comma, replace with semicolon so we don't gum up excel import of CSV's
   ofile << title.GetBuffer() << std::endl;
   if (!m_SubTitle.IsEmpty())
   {
      ofile << m_SubTitle.GetBuffer() << std::endl;
   }

   if (!m_YTitle.IsEmpty())
   {
      ofile << m_YTitle.GetBuffer() << std::endl;
   }

   // Write data one line per series
   int colidx = 0;
   for (const auto& series : m_SeriesData)
   {
      ofile << m_SeriesNames[colidx++].GetBuffer() << _T(",");
      std::size_t nvals = series.size();
      std::size_t ival(0);
      for (auto val : series)
      {
         ofile << val;
         if (++ival < nvals)
         {
            ofile << _T(",");
         }
      }

      ofile << std::endl;
   }

   ofile.close();

   CString msg;
   msg.Format(_T("File successfully written to \"%s\""),filename);
   AfxMessageBox(msg,MB_ICONEXCLAMATION);

   return true;
}


bool GraphExportUtil::CommitExcel(_Application& excel,Worksheets& worksheets,LPCTSTR strFilename)
{
   // save the spreadsheet
   _Worksheet ws;
   TRY
   {
      ws = worksheets.GetItem(COleVariant(1L));
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
   excel.SetVisible(TRUE);

   return TRUE;
}

void GraphExportUtil::SetColumnData(Worksheets& worksheets, IndexType hTable,LPCTSTR strColumnHeading,ColumnIndexType colIdx,const std::vector<Float64>& values)
{
   _Worksheet ws = worksheets.GetItem(COleVariant((long)hTable));

   // write the column heading
   CString strCell;
   strCell.Format(_T("%s4"),GetColumnLabel(colIdx));

   Range cell = ws.GetRange(COleVariant(strCell),COleVariant(strCell));
   cell.SetValue2(COleVariant(strColumnHeading));

   if (!values.empty()) // only write title if no values
   {
      // create a range that will hold all the data points
      strCell.Format(_T("%s5"),GetColumnLabel(colIdx));
      Range range = ws.GetRange(COleVariant(strCell),COleVariant(strCell));
      range = range.GetResize(COleVariant((short)values.size()),COleVariant((short)1));

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
      for (iter = values.begin(); iter != values.end(); iter++)
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
}

CString GraphExportUtil::GetColumnLabel(ColumnIndexType colIdx)
{
   CString strLabel((TCHAR)((colIdx % 26) + _T('A')));
   colIdx = ((colIdx - (colIdx % 26)) / 26);
   if (0 < colIdx)
   {
      CString strTemp = strLabel;
      strLabel = GetColumnLabel(colIdx - 1);
      strLabel += strTemp;
   }

   return strLabel;
}