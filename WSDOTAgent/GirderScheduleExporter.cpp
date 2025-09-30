///////////////////////////////////////////////////////////////////////
// IEPluginExample
// Copyright © 1999-2025  Washington State Department of Transportation
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

// PGSuperExporter.cpp : Implementation of CPGSuperExporter
#include "stdafx.h"
#include "GirderScheduleExporter.h"






CGirderScheduleExporter::CGirderScheduleExporter()
{
}

STDMETHODIMP CGirderScheduleExporter::Init(UINT nCmdID)
{
    return S_OK;
}

CString CGirderScheduleExporter::GetMenuText() const
{
    return CString("Girder Schedule to Excel File");
}

HBITMAP CGirderScheduleExporter::GetBitmapHandle() const
{
    return m_Bitmap;
}

CString CGirderScheduleExporter::GetCommandHintText() const
{
    return CString("Status line hint text\nTool tip text");
}

// Some constants to make the IDispatch calls easier
COleVariant ovOptional((long)DISP_E_PARAMNOTFOUND, VT_ERROR);  // optional parameter
COleVariant ovTrue((short)TRUE); // true
COleVariant ovFalse((short)FALSE); // false

bool CGirderScheduleExporter::CommitExcel(_Application& excel, Worksheets& worksheets, LPCTSTR strFilename)
{
    // save the spreadsheet
    _Worksheet ws;
    TRY
    {
       ws = worksheets.GetItem(COleVariant(1L));
       ws.SaveAs(strFilename,ovOptional,ovOptional,ovOptional,ovOptional,ovOptional,ovOptional,ovOptional,ovOptional,ovOptional);
    }
        CATCH(COleDispatchException, pException)
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

bool CGirderScheduleExporter::DoesFileExist(const CString& filename)
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

CString CGirderScheduleExporter::GetColumnLabel(ColumnIndexType colIdx)
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

HRESULT CGirderScheduleExporter::Export(std::shared_ptr<WBFL::EAF::Broker> pBroker)
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
    ws.SetName(_T("Girder Schedule"));



    //////////////////////////////////////////////////



   // write the table headings
    CString strCell;
    strCell.Format(_T("%s1"), GetColumnLabel(0));
    Range cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
    cell.SetValue2(COleVariant(_T("Title")));

    strCell.Format(_T("%s2"), GetColumnLabel(0));
    Range cell2 = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
    cell2.SetValue2(COleVariant(_T("SubTitle")));

    strCell.Format(_T("%s3"), GetColumnLabel(0));
    Range cell3 = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
    cell3.SetValue2(COleVariant(_T("Ytable")));


    //////////////////////////////////////////////////



    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Deal with file create name
    CString default_name(_T("Girder_Schedule"));
    TCHAR strFilter[] = _T("Excel Worksheet (*.xlsx)|*.xlsx|") _T("Comma Separated Value text File (*.csv)|*.csv|");
    TCHAR strSuffix[] = _T("xlsx") _T("csv");
    enum enSuffixIdx { esiExcel = 1, esiCSV } suffIdx(esiExcel);

    // Create file dialog
    CString file_path;
    CFileDialog  fildlg(FALSE, strSuffix, default_name, OFN_HIDEREADONLY, strFilter);
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
            int stm = AfxMessageBox(msg, MB_YESNOCANCEL | MB_ICONQUESTION);
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


    // Save file
    while (!CommitExcel(excel, worksheets, file_path))
    {
        CString strMsg;
        CFileStatus status;
        CFile::GetStatus(file_path, status);
        if (status.m_attribute & CFile::readOnly)
        {
            strMsg.Format(_T("Cannot save the file %s\r\nThe file exists and is marked Read-Only.\r\nClick Ok to enter another file name, or Cancel."), file_path);
        }
        else
        {
            strMsg.Format(_T("Unable to save %s because it is already open. \r\nClick Ok to enter another file name, or Cancel."), file_path);
        }

        int st = AfxMessageBox(strMsg, MB_OKCANCEL);

        if (st == IDOK)
        {

            // user wants to rename the file
            CString strExcelFilter(_T("Microsoft Excel Files (*.xlsx)"));
            CString strFileExtension(_T("xlsx"));

            CFileDialog fileDlg2(FALSE, strFileExtension, file_path, OFN_HIDEREADONLY, strExcelFilter);
            fileDlg2.DoModal();
            file_path = fileDlg2.GetPathName();
        }
    }

    return S_OK;
}

