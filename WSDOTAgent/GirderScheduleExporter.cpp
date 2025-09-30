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

HRESULT CGirderScheduleExporter::Export(std::shared_ptr<WBFL::EAF::Broker> pBroker)
{
    // write some bridge data to a text file
    CFileDialog  fildlg(FALSE, _T("xlsx"), _T("PGSuperExport"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("PGSuper Export File (*.xlsx)|*.xlsx||"));
    if (fildlg.DoModal() == IDOK)
    {
        CString file_path = fildlg.GetPathName();

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

        ws.SaveAs(file_path, ovOptional, ovOptional, ovOptional, ovOptional, ovOptional, ovOptional, ovOptional, ovOptional, ovOptional);

    }

    return S_OK;
}

