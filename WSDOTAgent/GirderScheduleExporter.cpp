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
#include <AgentTools.h>
#include <IFace/Project.h>
#include <IFace/Bridge.h>
#include <IFace/Artifact.h>
#include <IFace/AnalysisResults.h>
#include <IFace/Intervals.h>
#include <IFace/PointOfInterest.h>
#include <IFace\GirderHandling.h>
#include <PsgLib\BridgeDescription2.h>
#include <PgsExt\GirderArtifact.h>
#include <EAF\EAFApp.h>
#include <EAF\EAFDisplayUnits.h>
#include <psgLib/GirderLibraryEntry.h>
#include <Plugins/BeamFamilyCLSID.h>
#include "WSDOTReinforcement.h"



CGirderScheduleExporter::CGirderScheduleExporter()
{
}

STDMETHODIMP CGirderScheduleExporter::Init(UINT nCmdID)
{
    return S_OK;
}

CString CGirderScheduleExporter::GetMenuText() const
{
    return CString("WSDOT Girder Schedule to Excel File");
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

std::string CGirderScheduleExporter::FormatFeetInchesFromDecimalInches(double totalInches, int denom = 8)
{
    {
        bool isNeg = totalInches < 0;
        if (isNeg) totalInches = -totalInches;

        // Break into feet and inches
        int feet = static_cast<int>(std::floor(totalInches / 12.0));
        double inchValue = totalInches - feet * 12.0;

        int inches = static_cast<int>(std::floor(inchValue));
        double frac = inchValue - inches;

        int num = static_cast<int>(std::round(frac * denom));

        // Carry handling (e.g. 11.99" → 12")
        if (num == denom) { num = 0; ++inches; }
        if (inches == 12) { inches = 0; ++feet; }

        // Simplify fraction
        if (num != 0) {
            int g = std::gcd(num, denom);
            num /= g;
            denom /= g;
        }

        // Build the formatted string
        std::ostringstream oss;
        if (isNeg) oss << "-";

        if (feet > 0)
            oss << feet << "'-";

        oss << inches;

        if (num != 0)
            oss << " " << num << "/" << denom;

        oss << "\"";

        return oss.str();
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

void CGirderScheduleExporter::SetColumnHeader(_Worksheet* pWorksheet, ColumnIndexType colIdx, 
    ColumnIndexType colSpan, RowIndexType rowIdx, RowIndexType rowSpan, Float64 orientation, CString strValue)
{
    CString strCell;
    strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(colIdx), rowIdx + 1, GetColumnLabel(colIdx + colSpan - 1), rowIdx + rowSpan);
    Range cell = pWorksheet->GetRange(COleVariant(strCell), COleVariant(strCell));
    cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
    cell.SetValue2(COleVariant(strValue));
    cell.SetOrientation(COleVariant((short)orientation));
    cell.BorderAround(
        COleVariant((long)1),
        (long)3,
        (long)-4105,
        COleVariant((long)0)
    );
}
template <typename T>
void CGirderScheduleExporter::SetColumnData(_Worksheet* pWorksheet, ColumnIndexType colIdx,
    RowIndexType rowIdx, T tValue)
{
    CString strCell;
    strCell.Format(_T("%s%d"), GetColumnLabel(colIdx), rowIdx + 1);
    Range cell = pWorksheet->GetRange(COleVariant(strCell), COleVariant(strCell));
    CString strValue;
    if constexpr (std::is_convertible_v<T, LPCTSTR>)
        strValue = tValue;
    else if constexpr (std::is_same_v<T, Float64>)
        strValue.Format(_T("%f"), tValue);
    else if constexpr (std::is_convertible_v<T, int>)
        strValue.Format(_T("%d"), tValue);
    else if constexpr (std::is_same_v<T, rptTimeUnitValue>)
    {
        Float64 v = tValue.GetValue(true);
        strValue.Format(_T("%f"), v);
    }
    else
    {
        strValue = _T("unknown type");
    }

    m_current_row_data.emplace_back(strValue);

    cell.SetValue2(COleVariant(strValue));

    cell.BorderAround(
        COleVariant((long)1),
        (long)3,
        (long)-4105,
        COleVariant((long)0)
    );

}

HRESULT CGirderScheduleExporter::Export(std::shared_ptr<WBFL::EAF::Broker> pBroker)
{
    GET_IFACE2(pBroker, IEAFProgress, pProgress);
    WBFL::EAF::AutoProgress ap(pProgress);

    // Get the Excel template file folder
    CEAFApp* pApp = EAFGetApp();
    CString str = pApp->GetAppLocation();

    CString strDefaultLocation;
    if (-1 != str.Find(_T("RegFreeCOM")))
    {
        // application is on a development box
        strDefaultLocation = str.Left(2) + CString(_T("\\ARP\\PGSuper\\WSDOTAgent\\"));
    }
    else
    {
        // make sure we have a trailing backslash
        if (_T('\\') != str.GetAt(str.GetLength() - 1))
        {
            str += _T("\\");
        }
        strDefaultLocation = str;
    }

    // Get the user's setting, using the local machine setting as the default if not present
    CString strExcelTemplateFolder = pApp->GetProfileString(_T("Settings"), _T("ExcelTemplateFolder"), strDefaultLocation);

    // make sure we have a trailing backslash
    if (_T('\\') != strExcelTemplateFolder.GetAt(strExcelTemplateFolder.GetLength() - 1))
    {
        strExcelTemplateFolder += _T("\\");
    }

    CString strTemplateName = strExcelTemplateFolder + _T("WF_Girder_Schedule_Template.xltx");

    _Application excel;
    if (!excel.CreateDispatch(_T("Excel.Application")))
    {
        AfxMessageBox(_T("An error occured while attempting to run Excel. Excel files cannot be created unless Microsoft Excel is installed. Maybe try a .csv file?"));
        return FALSE;
    }

      // get the spreadsheet setup
      Workbooks workbooks = excel.GetWorkbooks();
      _Workbook workbook = workbooks.Add(COleVariant(strTemplateName)); // creates a new Excel file from the template

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

    // Format cells
    Range allCells = ws.GetCells();
    COleVariant vCenter((long)-4108, VT_I4);
    allCells.SetHorizontalAlignment(vCenter);
    allCells.SetVerticalAlignment(vCenter);

    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

    INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);
    INIT_UV_PROTOTYPE(rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), true);
    INIT_UV_PROTOTYPE(rptMomentPerAngleUnitValue, spring, pDisplayUnits->GetMomentPerAngleUnit(), true);

    INIT_FRACTIONAL_LENGTH_PROTOTYPE(gdim, IS_US_UNITS(pDisplayUnits), 8, RoundUp, pDisplayUnits->GetComponentDimUnit(), true, true);
    //INIT_FRACTIONAL_LENGTH_PROTOTYPE(glength, IS_US_UNITS(pDisplayUnits), 4, RoundOff, pDisplayUnits->GetSpanLengthUnit(), true, true);

    // write the table headings

    CString strAngle1;
    strAngle1.Format(_T("\u03B8\u2081 (%s)"), angle.GetUnitTag().c_str());
    CString strAngle2;
    strAngle2.Format(_T("\u03B8\u2082 (%s)"), angle.GetUnitTag().c_str());
    CString strFc;
    strFc.Format(_T("@ 28 DAYS F'C (%s)"), stress.GetUnitTag().c_str());
    CString strFci;
    strFci.Format(_T("@ RELEASE F'CI (%s)"), stress.GetUnitTag().c_str());
    CString strSpring;
    strSpring.Format(_T("K\u03B8 MINIMUM SHIPPING SUPPORT ROTATIONAL SPRING CONSTANT (%s)"), spring.GetUnitTag().c_str());


    const std::vector<ScheduleHeaderInfo>& headerInfoWFFamily =
    { 
        {0, 41, 0, 1, 0, _T("GIRDER SCHEDULE")},
        {0, 1, 1, 3, 90, _T("SPAN")},
        {1, 1, 1, 3, 90, _T("GIRDER")},
        {2, 1, 1, 3, 90, _T("GIRDER SERIES")},
        {3, 1, 1, 3, 0, _T("PLAN LENGTH (ALONG GIRDER GRADE) (SEE GIRDER NOTE 1)")},
        {4, 1, 1, 3, 0, _T("INT. DIAPHRAGM TYPE (FULL OR PARTIAL)")},
        {5, 7, 1, 1, 0, _T("GIRDER END DETAILS")},
        {5, 1, 2, 2, 90, _T("END 1 TYPE")},
        {6, 1, 2, 2, 90, _T("END 2 TYPE")},
        {7, 1, 2, 2, 0, _T("Ld")},
        {8, 1, 2, 2, 0, strAngle1},
        {9, 1, 2, 2, 0, strAngle2},
        {10, 1, 2, 2, 0, _T("P\u2081")},
        {11, 1, 2, 2, 0, _T("P\u2082")},
        {12, 2, 1, 1, 0, _T("MIN. CONC. COMP. STRENGTH")},
        {12, 1, 2, 2, 90, strFc},
        {13, 1, 2, 2, 90, strFci},
        {14, 3, 1, 1, 0, _T("NUMBER OF STRANDS (SEE GIRDER NOTE 2)")},
        {14, 1, 2, 2, 90, _T("STRAIGHT")},
        {15, 1, 2, 2, 90, _T("HARPED")},
        {16, 1, 2, 2, 90, _T("TEMPORARY")},
        {17, 3, 1, 1, 0, _T("LOCATION OF C.G. STRANDS")},
        {17, 1, 2, 2, 0, _T("E")},
        {18, 1, 2, 2, 0, _T("F\u2104")},
        {19, 1, 2, 2, 0, _T("F\u2080")},
        {20, 4, 1, 1, 0, _T("STRAIGHT STRANDS TO EXTEND")},
        {20, 2, 2, 1, 0, _T("END 1")},
        {22, 2, 2, 1, 0, _T("END 2")},
        {20, 1, 3, 1, 90, _T("STRANDS")},
        {21, 1, 3, 1, 90, _T("EXTENSION LENGTH")},
        {22, 1, 3, 1, 90, _T("STRANDS")},
        {23, 1, 3, 1, 90, _T("EXTENSION LENGTH")},
        {24, 1, 1, 3, 90, _T("\"A\" DIMENSION AT \u2104 BEARINGS")},
        {25, 1, 1, 3, 90, _T("DECK SCREED CAMBER C")},
        {26, 2, 1, 2, 0, _T("MIDSPAN VERTICAL DEFLECTION D")},
        {26, 1, 3, 1, 90, _T("LOWER BOUND @ 40 DAYS")},
        {27, 1, 3, 1, 90, _T("UPPER BOUND @ 120 DAYS")},
        {28, 7, 1, 1, 0, _T("REINFORCEMENT DETAILS")},
        {28, 2, 2, 1, 0, _T("ZONE 1")},
        {30, 2, 2, 1, 0, _T("ZONE 2")},
        {32, 2, 2, 1, 0, _T("ZONE 3")},
        {28, 1, 3, 1, 90, _T("SPACING")},
        {29, 1, 3, 1, 90, _T("LENGTH")},
        {30, 1, 3, 1, 90, _T("SPACING")},
        {31, 1, 3, 1, 90, _T("LENGTH")},
        {32, 1, 3, 1, 90, _T("SPACING")},
        {33, 1, 3, 1, 90, _T("LENGTH")},
        {34, 1, 2, 2, 0, _T("H1")},
        {35, 6, 1, 1, 0, _T("SHIPPING AND HANDLING DETAILS")},
        {35, 1, 2, 2, 0, _T("MIDSPAN VERTICAL DEFLECTION AT SHIPPING")},
        {36, 1, 2, 2, 0, _T("L")},
        {37, 1, 2, 2, 0, _T("L\u2081")},
        {38, 1, 2, 2, 0, _T("L\u2082")},
        {39, 1, 2, 2, 0, strSpring},
        {40, 1, 2, 2, 0, _T("Wcc MINIMUM SHIPPING SUPPORT CNTR.-TO-CNTR. WHEEL SPACING")},
    };

    const std::vector<ScheduleHeaderInfo>& headerInfoWFDGFamily =
    {
        {0, 40, 0, 1, 0, _T("GIRDER SCHEDULE")},
        {0, 1, 1, 3, 90, _T("SPAN")},
        {1, 1, 1, 3, 90, _T("GIRDER")},
        {2, 1, 1, 3, 90, _T("GIRDER SERIES")},
        {3, 1, 1, 3, 0, _T("TOP FLANGE WIDTH W")},
        {4, 1, 1, 3, 0, _T("PLAN LENGTH (ALONG GIRDER GRADE) (SEE GIRDER NOTE 1)")},
        {5, 7, 1, 1, 0, _T("GIRDER END DETAILS")},
        {5, 1, 2, 2, 90, _T("END 1 TYPE")},
        {6, 1, 2, 2, 90, _T("END 2 TYPE")},
        {7, 1, 2, 2, 0, _T("Ld")},
        {8, 1, 2, 2, 0, strAngle1},
        {9, 1, 2, 2, 0, strAngle2},
        {10, 1, 2, 2, 0, _T("P\u2081")},
        {11, 1, 2, 2, 0, _T("P\u2082")},
        {12, 2, 1, 1, 0, _T("MIN. CONC. COMP. STRENGTH")},
        {12, 1, 2, 2, 90, strFc},
        {13, 1, 2, 2, 90, strFci},
        {14, 3, 1, 1, 0, _T("NUMBER OF STRANDS (SEE GIRDER NOTE 2)")},
        {14, 1, 2, 2, 90, _T("STRAIGHT")},
        {15, 1, 2, 2, 90, _T("HARPED")},
        {16, 1, 2, 2, 90, _T("TEMPORARY")}, //
        {17, 3, 1, 1, 0, _T("LOCATION OF C.G. STRANDS")},
        {17, 1, 2, 2, 0, _T("E")},
        {18, 1, 2, 2, 0, _T("F\u2104")},
        {19, 1, 2, 2, 0, _T("F\u2080")},
        {20, 4, 1, 1, 0, _T("STRAIGHT STRANDS TO EXTEND")},
        {20, 2, 2, 2, 0, _T("END 1")},
        {22, 2, 2, 2, 0, _T("END 2")},
        {20, 1, 3, 1, 90, _T("STRANDS")},
        {21, 1, 3, 1, 90, _T("EXTENSION LENGTH")},
        {22, 1, 3, 1, 90, _T("STRANDS")},
        {23, 1, 3, 1, 90, _T("EXTENSION LENGTH")},
        {24, 1, 1, 3, 90, _T("\"A\" DIMENSION AT \u2104 BEARINGS")},
        {25, 1, 1, 3, 90, _T("DECK SCREED CAMBER C")},
        {26, 2, 1, 2, 0, _T("MIDSPAN VERTICAL DEFLECTION D")},
        {26, 1, 3, 1, 90, _T("LOWER BOUND @ 40 DAYS")},
        {27, 1, 3, 1, 90, _T("UPPER BOUND @ 120 DAYS")},
        {28, 6, 1, 1, 0, _T("REINFORCEMENT DETAILS")},
        {28, 2, 2, 1, 0, _T("ZONE 1")},
        {30, 2, 2, 1, 0, _T("ZONE 2")},
        {32, 2, 2, 1, 0, _T("ZONE 3")},
        {28, 1, 3, 1, 90, _T("SPACING")},
        {29, 1, 3, 1, 90, _T("LENGTH")},
        {30, 1, 3, 1, 90, _T("SPACING")},
        {31, 1, 3, 1, 90, _T("LENGTH")},
        {32, 1, 3, 1, 90, _T("SPACING")},
        {33, 1, 3, 1, 90, _T("LENGTH")},
        {34, 6, 1, 1, 0, _T("SHIPPING AND HANDLING DETAILS")},
        {34, 1, 2, 2, 0, _T("MIDSPAN VERTICAL DEFLECTION AT SHIPPING")},
        {35, 1, 2, 2, 0, _T("L")},
        {36, 1, 2, 2, 0, _T("L\u2081")},
        {37, 1, 2, 2, 0, _T("L\u2082")},
        {38, 1, 2, 2, 0, strSpring},
        {39, 1, 2, 2, 0, _T("Wcc MINIMUM SHIPPING SUPPORT CNTR.-TO-CNTR. WHEEL SPACING")},
    };

    const std::vector<ScheduleHeaderInfo>& headerInfoUBeamFamily =
    {
        {0, 39, 0, 1, 0, _T("GIRDER SCHEDULE")},
        {0, 1, 1, 3, 90, _T("SPAN")},
        {1, 1, 1, 3, 90, _T("GIRDER")},
        {2, 1, 1, 3, 90, _T("GIRDER SERIES")},
        {3, 1, 1, 3, 0, _T("PLAN LENGTH (ALONG GIRDER GRADE) (SEE GIRDER NOTE 1)")},
        {4, 1, 1, 3, 0, _T("INT. DIAPHRAGM TYPE (FULL OR PARTIAL)")},
        {5, 7, 1, 1, 0, _T("GIRDER END DETAILS")},
        {5, 1, 2, 2, 90, _T("END 1 TYPE")},
        {6, 1, 2, 2, 90, _T("END 2 TYPE")},
        {7, 1, 2, 2, 0, _T("Ld")},
        {8, 1, 2, 2, 0, strAngle1},
        {9, 1, 2, 2, 0, strAngle2},
        {10, 1, 2, 2, 0, _T("P\u2081")},
        {11, 1, 2, 2, 0, _T("P\u2082")},
        {12, 2, 1, 1, 0, _T("MIN. CONC. COMP. STRENGTH")},
        {12, 1, 2, 2, 90, strFc},
        {13, 1, 2, 2, 90, strFci},
        {14, 3, 1, 1, 0, _T("NUMBER OF STRANDS (SEE GIRDER NOTE 2)")},
        {14, 1, 2, 2, 90, _T("STRAIGHT")},
        {15, 1, 2, 2, 90, _T("HARPED")},
        {16, 1, 2, 2, 90, _T("TEMPORARY")},
        {17, 3, 1, 1, 0, _T("LOCATION OF C.G. STRANDS")},
        {17, 1, 2, 2, 0, _T("E")},
        {18, 1, 2, 2, 0, _T("F\u2104")},
        {19, 1, 2, 2, 0, _T("F\u2080")},
        {20, 2, 1, 1, 0, _T("STRAIGHT STRANDS TO EXTEND")},
        {20, 1, 2, 2, 0, _T("END 1")},
        {21, 1, 2, 2, 0, _T("END 2")},
        {22, 1, 1, 3, 90, _T("\"A\" DIMENSION AT \u2104 BEARINGS")},
        {23, 1, 1, 3, 90, _T("DECK SCREED CAMBER C")},
        {24, 2, 1, 2, 0, _T("MIDSPAN VERTICAL DEFLECTION D")},
        {24, 1, 3, 1, 90, _T("LOWER BOUND @ 40 DAYS")},
        {25, 1, 3, 1, 90, _T("UPPER BOUND @ 120 DAYS")},
        {26, 7, 1, 1, 0, _T("REINFORCEMENT DETAILS")},
        {26, 1, 2, 2, 0, _T("V1")},
        {27, 1, 2, 2, 0, _T("V2")},
        {28, 1, 2, 2, 0, _T("V3")},
        {29, 1, 2, 2, 0, _T("V4")},
        {30, 1, 2, 2, 0, _T("V5")},
        {31, 1, 2, 2, 0, _T("V6")},
        {32, 1, 2, 2, 0, _T("H1")},
        {33, 6, 1, 1, 0, _T("SHIPPING AND HANDLING DETAILS")},
        {33, 1, 2, 2, 0, _T("MIDSPAN VERTICAL DEFLECTION AT SHIPPING")},
        {34, 1, 2, 2, 0, _T("L")},
        {35, 1, 2, 2, 0, _T("L\u2081")},
        {36, 1, 2, 2, 0, _T("L\u2082")},
        {37, 1, 2, 2, 0, strSpring},
        {38, 1, 2, 2, 0, _T("Wcc MINIMUM SHIPPING SUPPORT CNTR.-TO-CNTR. WHEEL SPACING")},
    };

    const std::vector<ScheduleHeaderInfo>& headerInfoSlabFamily =
    {
        {0, 41, 0, 1, 0, _T("GIRDER SCHEDULE")},
        {0, 1, 1, 4, 90, _T("SPAN")},
        {1, 1, 1, 4, 90, _T("GIRDER")},
        {2, 1, 1, 4, 0, _T("GIRDER HEIGHT H")},
        {3, 1, 1, 4, 0, _T("GIRDER WIDTH W")},
        {4, 1, 1, 4, 0, _T("PLAN LENGTH (ALONG GIRDER GRADE) (SEE GIRDER NOTE 1)")},
        {5, 2, 1, 2, 0, _T("VOIDS")},
        {5, 1, 3, 2, 90, _T("NUMBER")},
        {6, 1, 3, 2, 90, _T("DIAMETER")},
        {7, 4, 1, 2, 0, _T("GIRDER END DETAILS")},

        {7, 1, 3, 2, 90, _T("END 1 TYPE")},
        {8, 1, 3, 2, 90, _T("END 2 TYPE")},
        {9, 1, 3, 2, 0, strAngle1},
        {10, 1, 3, 2, 0, strAngle2},

        {11, 2, 1, 2, 0, _T("MIN. CONC. COMP. STRENGTH")},
        {11, 1, 3, 2, 90, strFc},
        {12, 1, 3, 2, 90, strFci},

        {13, 8, 1, 1, 0, _T("PRESTRESSING STRANDS (SEE GIRDER NOTES 2-4)")},
        {13, 3, 2, 1, 0, _T("ROW 1")},
        {16, 3, 2, 1, 0, _T("ROW 2")},
        {19, 2, 2, 1, 0, _T("TOP ROW")},

        {13, 1, 3, 2, 90, _T("PERMANENT STRANDS")},
        {14, 1, 3, 2, 90, _T("EXTENDED NUMBER AND LENGTH")},
        {15, 1, 3, 2, 90, _T("DEBONDED NUMBER AND LENGTH")},
        {16, 1, 3, 2, 90, _T("PERMANENT STRANDS")},
        {17, 1, 3, 2, 90, _T("EXTENDED NUMBER AND LENGTH")},
        {18, 1, 3, 2, 90, _T("DEBONDED NUMBER AND LENGTH")},
        {19, 1, 3, 2, 90, _T("PERMANENT STRANDS")},
        {20, 1, 3, 2, 90, _T("TEMPORARY STRANDS")},

        {21, 1, 1, 4, 90, _T("\"A\" DIMENSION AT \u2104 BEARINGS")},
        {22, 1, 1, 4, 90, _T("DECK SCREED CAMBER C")},

        {23, 2, 1, 3, 0, _T("MIDSPAN VERTICAL DEFLECTION D")},
        {23, 1, 4, 1, 90, _T("LOWER BOUND @ 40 DAYS")},
        {24, 1, 4, 1, 90, _T("UPPER BOUND @ 120 DAYS")},
         
        {25, 6, 1, 2, 0, _T("TRANSVERSE REINFORCEMENT")},
        {25, 2, 3, 1, 0, _T("ZONE 1")},
        {27, 2, 3, 1, 0, _T("ZONE 2")},
        {29, 2, 3, 1, 0, _T("ZONE 3")},
        {25, 1, 4, 1, 90, _T("SPACING")},
        {26, 1, 4, 1, 90, _T("LENGTH")},
        {27, 1, 4, 1, 90, _T("SPACING")},
        {28, 1, 4, 1, 90, _T("LENGTH")},
        {29, 1, 4, 1, 90, _T("SPACING")},
        {30, 1, 4, 1, 90, _T("LENGTH")},

        {31, 4, 1, 2, 0, _T("LONGITUDINAL REINFORCEMENT") },
        {31, 2, 3, 1, 0, _T("G1")},
        {33, 2, 3, 1, 0, _T("G2")},
        {31, 1, 4, 1, 90, _T("BAR SIZE")},
        {32, 1, 4, 1, 90, _T("NO. OF BARS")},
        {33, 1, 4, 1, 90, _T("BAR SIZE")},
        {34, 1, 4, 1, 90, _T("NO. OF BARS")},

        {35, 6, 1, 2, 0, _T("SHIPPING AND HANDLING DETAILS")},
        {35, 1, 3, 2, 0, _T("MIDSPAN VERTICAL DEFLECTION AT SHIPPING")},
        {36, 1, 3, 2, 0, _T("L")},
        {37, 1, 3, 2, 0, _T("L\u2081")},
        {38, 1, 3, 2, 0, _T("L\u2082")},
        {39, 1, 3, 2, 0, strSpring},
        {40, 1, 3, 2, 0, _T("Wcc MINIMUM SHIPPING SUPPORT CNTR.-TO-CNTR. WHEEL SPACING")},
    };

    //Set girder data
    GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
    const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

    GroupIndexType grpHeaderIdx = 0;
    const CGirderGroupData* pGroupHeader = pBridgeDesc->GetGirderGroup(grpHeaderIdx);
    const CSplicedGirderData* pGirderHeader = pGroupHeader->GetGirder(0);
    const GirderLibraryEntry* pGdrLibEntry = pGirderHeader->GetGirderLibraryEntry();
    auto factory = pGdrLibEntry->GetBeamFactory();
    CLSID familyCLSID = factory->GetFamilyCLSID();

    bool bIbeam = (familyCLSID == CLSID_WFBeamFamily);
    bool bSlab = (familyCLSID == CLSID_SlabBeamFamily);
    bool bUbeam = (familyCLSID == CLSID_UBeamFamily);
    bool bWFDG = (familyCLSID == CLSID_DeckBulbTeeBeamFamily);

    if (bSlab)
        m_HeaderInfo = headerInfoSlabFamily;
    else if (bUbeam)
        m_HeaderInfo = headerInfoUBeamFamily;
    else if (bWFDG)
        m_HeaderInfo = headerInfoWFDGFamily;
    else if (bIbeam)
        m_HeaderInfo = headerInfoWFFamily;
    else
    {
        AfxMessageBox(_T("There is no schedule for the selected girder type"));
        return E_FAIL;
    }

    CComPtr<IAnnotatedDisplayUnitFormatter> pADUF;
    pADUF.CoCreateInstance(CLSID_AnnotatedDisplayUnitFormatter);
    
    GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
    CGirderKey girderKey;

    std::vector<CString> vStrandPatterns;

    for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
    {
        m_previous_row_data.clear();
        
        GirderIndexType gdrIdx = m_last_same_gdrID = 0;
        const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
        GirderIndexType nGirders = pGroup->GetGirderCount();

        for (gdrIdx; gdrIdx < nGirders; gdrIdx++)
        {
            m_current_row_data.clear();

            ColumnIndexType col = 0;

            girderKey = CGirderKey(grpIdx, gdrIdx);

            const CSplicedGirderData* pGirder = pGroupHeader->GetGirder(gdrIdx);
            
            ++col;

            GET_IFACE2(pBroker, IBridge, pBridge);
            const CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
            CSegmentKey segmentKey(pSegment->GetSegmentKey());
            pgsPointOfInterest poiStart(segmentKey, 0.0);

            GET_IFACE2(pBroker, ISectionProperties, pSectProp);

            GET_IFACE2(pBroker, IIntervals, pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
            IntervalIndexType finalIntervalIdx = pIntervals->GetIntervalCount() - 1;

            GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest);
            PoiList pmid;
            pPointOfInterest->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT, &pmid);
            ATLASSERT(pmid.size() == 1);
            const pgsPointOfInterest& poiMidSpan(pmid.front());

            CString strValue;

            //Set Girder Series or Height and Width
            if (!bSlab)
            {
                CString strSeries = pGirder->GetGirderName();
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strSeries);

                if (bWFDG)
                {
                    GET_IFACE2(pBroker, IGirder, pIGirder);
                    Float64 W = pIGirder->GetTopWidth(poiMidSpan);
                    gdim.SetValue(W);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, strValue);
                }
            }
            else
            {
                Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poiMidSpan);
                gdim.SetValue(Hg);
                const auto& val = gdim.GetValue(true);
                strValue.Format(_T("%0.1f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 5, strValue);

                GET_IFACE2(pBroker, IGirder, pIGirder);
                Float64 W = pIGirder->GetTopWidth(poiMidSpan);
                gdim.SetValue(W);
                const auto& val1 = gdim.GetValue(true);
                strValue.Format(_T("%0.1f %s"), val1, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val1, 0.125)).c_str();
                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 5, strValue);
            }

            //Set Plan Length
            const auto& rptPlanLength = gdim.SetValue(pBridge->GetSegmentPlanLength(segmentKey));
            const auto& planLength = gdim.GetValue(true);
            strValue.Format(_T("%0.1f %s"), planLength, gdim.GetUnitTag().c_str());
            if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                strValue = FormatFeetInchesFromDecimalInches(RoundOff(planLength, 0.125)).c_str();
           
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

            //int. diaphragm type or voids
            if (!bSlab)
            {
                if (!bWFDG)
                {
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
                }
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
            }
            else
            {
                const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(girderKey.girderIndex)->GetGirderLibraryEntry();
                Float64 nVoids = pGdrEntry->GetDimension(_T("Number_of_Voids"));
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 5, nVoids);
                Float64 ExtVoidDiameter = pGdrEntry->GetDimension(_T("D1"));
                Float64 IntVoidDiameter = pGdrEntry->GetDimension(_T("D2"));
                if (IntVoidDiameter != 0)
                {
                    strValue.Format(_T("Ext: %0.1f %s Int: %0.1f %s"), ExtVoidDiameter, gdim.GetUnitTag().c_str(), 
                        IntVoidDiameter, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    {
                        CString strExt;
                        strExt = FormatFeetInchesFromDecimalInches(RoundOff(ExtVoidDiameter, 0.125)).c_str();
                        CString strInt;
                        strInt = FormatFeetInchesFromDecimalInches(RoundOff(IntVoidDiameter, 0.125)).c_str();
                        strValue.Format(_T("Ext: %s Int: %s"), strExt, strInt);
                    }
                }
                else
                {
                    strValue.Format(_T("%0.1f %s"), ExtVoidDiameter, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(ExtVoidDiameter, 0.125)).c_str();
                }


                
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 5, strValue);
            }
            
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
            

            CComPtr<IAngle> objAngle1, objAngle2;
            pBridge->GetSegmentSkewAngle(segmentKey, pgsTypes::metStart, &objAngle1);
            pBridge->GetSegmentSkewAngle(segmentKey, pgsTypes::metEnd, &objAngle2);
            Float64 t1, t2;
            objAngle1->get_Value(&t1);
            objAngle2->get_Value(&t2);

            angle.SetValue(t1);
            const auto& ft1 = angle.GetValue(true);
            strValue.Format(_T("%0.0f"), ft1);
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
            angle.SetValue(t2);
            const auto& ft2 = angle.GetValue(true);
            strValue.Format(_T("%0.0f"), ft2);
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

            if (!bSlab)
            {
                // P1 & P2...
                // does not apply to slabs
                PierIndexType prevPierIdx = (PierIndexType)grpIdx;
                PierIndexType nextPierIdx = prevPierIdx + 1;
                bool bContinuousLeft, bContinuousRight, bIntegralLeft, bIntegralRight;

                pBridge->IsContinuousAtPier(prevPierIdx, &bContinuousLeft, &bContinuousRight);
                pBridge->IsIntegralAtPier(prevPierIdx, &bIntegralLeft, &bIntegralRight);

                if (bContinuousLeft || bIntegralLeft)
                {
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("-"));
                }
                else
                {
                    // start end distance is a plan view dimension that needs to
                    // be adjusted for the installed girder slope and the height of the girder
                    Float64 D = pBridge->GetSegmentStartEndDistance(segmentKey);
                    Float64 slope = pBridge->GetSegmentSlope(segmentKey);
                    Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poiMidSpan);
                    Float64 P1 = D * sqrt(1 + slope * slope) - slope * Hg;

                    gdim.SetValue(P1);
                    P1 = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), P1, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(P1, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                }

                pBridge->IsContinuousAtPier(nextPierIdx, &bContinuousLeft, &bContinuousRight);
                pBridge->IsIntegralAtPier(nextPierIdx, &bIntegralLeft, &bIntegralRight);

                if (bContinuousRight || bIntegralRight)
                {
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
                }
                else
                {
                    // start end distance is a plan view dimension that needs to
                    // be adjusted for the installed girder slope and the height of the girder
                    Float64 D = pBridge->GetSegmentEndEndDistance(segmentKey);
                    Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poiMidSpan);
                    Float64 slope = pBridge->GetSegmentSlope(segmentKey);
                    Float64 P2 = D * sqrt(1 + slope * slope) + slope * Hg;

                    gdim.SetValue(P2);
                    P2 = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), P2, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(P2, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                }
            }

            GET_IFACE2(pBroker, IMaterials, pMaterial);

            
            stress.SetValue(pMaterial->GetSegmentDesignFc(segmentKey, finalIntervalIdx));
            const auto& fc = stress.GetValue(true);
            strValue.Format(_T("%0.1f"), fc);
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

            stress.SetValue(pMaterial->GetSegmentDesignFc(segmentKey, releaseIntervalIdx));
            const auto& fci = stress.GetValue(true);
            strValue.Format(_T("%0.1f"), fci);
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);


            //set number of strands
            GET_IFACE2(pBroker, IArtifact, pIArtifact);
            const pgsGirderArtifact* pArtifact = pIArtifact->GetGirderArtifact(girderKey);
            const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);

            bool bCanReportPrestressInformation = true;

            GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry);

            // WsDOT reports don't support Straight-Web strand option (except for slab beams)
            if (pStrandGeometry->GetAreHarpedStrandsForcedStraight(segmentKey) && !bSlab)
            {
                bCanReportPrestressInformation = false;
            }

            if (pSegment->Strands.GetStrandDefinitionType() == pgsTypes::sdtDirectSelection)
            {
                bCanReportPrestressInformation = false;
            }

            StrandIndexType Ns = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Straight);
            StrandIndexType Nh = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Harped);

            if (bCanReportPrestressInformation)
            {

                if (!bSlab)
                {
                    StrandIndexType nDebonded = pStrandGeometry->GetNumDebondedStrands(segmentKey, pgsTypes::Straight, pgsTypes::dbetEither);
                    if (nDebonded != 0)
                    {
                        strValue.Format(_T("%d (%d debonded)"), Ns, nDebonded);
                        SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                    }
                    else
                    {
                        SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, Ns);
                    }

                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, Nh);


                    StrandIndexType Nt = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Temporary);
                    CString strNt;
                    strNt.Format(_T("%d"), Nt);
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, Nt);
                    
                    
                }
                else
                {
                    RowIndexType nRows = 3;

                    for (RowIndexType rowIdx = 0; rowIdx < nRows; rowIdx++)
                    {
                        StrandIndexType nStrandsInRow = pStrandGeometry->GetNumStrandInRow(poiStart, rowIdx, pgsTypes::Straight);
                        SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 5, nStrandsInRow);

                        if (rowIdx <= 1)
                        {
                            RowIndexType nStrandsInRowDebonded = pStrandGeometry->GetNumDebondedStrandsInRow(poiStart, rowIdx, pgsTypes::Straight);
                            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 5, nStrandsInRowDebonded);

                            StrandIndexType nExtended = 0;

                            if (Ns > 0)
                            {
                                std::vector<StrandIndexType>vStrandsInRow = pStrandGeometry->GetStrandsInRow(poiStart, rowIdx, pgsTypes::Straight);

                                for (const auto& strandIdx : vStrandsInRow)
                                {
                                    bool bExtended = pStrandGeometry->IsExtendedStrand(poiStart, strandIdx, pgsTypes::Straight);
                                    if (bExtended)
                                    {
                                        nExtended++;
                                    }
                                }
                            }

                            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 5, nExtended);
                        }
                        else
                        {
                            StrandIndexType Nt = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Temporary);
                            SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 5, Nt);
                        }

                    }

                }

            }
            else
            {

                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));

                if (!bSlab)
                {
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                }

                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
            }

            if (CLSID_SlabBeamFamily != familyCLSID)
            {

                Float64 ybg = pSectProp->GetY(releaseIntervalIdx, poiMidSpan, pgsTypes::BottomGirder);
                Float64 sse = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poiMidSpan, pgsTypes::Straight).Y();
                if (0 < Ns)
                {
                    gdim.SetValue(ybg - sse);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("N/A"));
                }

                Float64 hse = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poiMidSpan, pgsTypes::Harped).Y();
                if (0 < Nh)
                {
                    gdim.SetValue(ybg - hse);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("N/A"));
                }


                Float64 ytg = pSectProp->GetY(releaseIntervalIdx, poiStart, pgsTypes::TopGirder);
                Float64 hss = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poiStart, pgsTypes::Harped).Y();
                if (0 < Nh)
                {
                    gdim.SetValue(ytg + hss);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("N/A"));
                }


                //strand extensions
                StrandIndexType nExtended = 0;
                CString strExt1;
                for (StrandIndexType strandIdx = 0; strandIdx < Ns; strandIdx++)
                {
                    if (pStrandGeometry->IsExtendedStrand(segmentKey, pgsTypes::metStart, strandIdx, pgsTypes::Straight))
                    {
                        nExtended++;
                        CString val;
                        val.Format(_T("%d, "), strandIdx + 1);
                        strExt1.Append(val);
                    }
                }

                int pos = strExt1.ReverseFind(_T(','));
                if (pos != -1)
                {
                    strExt1.Delete(pos, 1);
                    strExt1.TrimRight();
                }

                if (nExtended == 0)
                {
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
                    if (!bUbeam/* && !bWFDG*/)
                    {
                        SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
                    }
                }
                else
                {
                    if (std::find(vStrandPatterns.begin(), vStrandPatterns.end(), strExt1) == vStrandPatterns.end())
                    {
                        vStrandPatterns.emplace_back(strExt1);
                    }

                    auto it = std::find(vStrandPatterns.begin(), vStrandPatterns.end(), strExt1);
                    IndexType idx = std::distance(vStrandPatterns.begin(), it);
                        
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, GetColumnLabel(idx));

                    if (!bUbeam /*&& !bWFDG*/)
                    {
                        SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
                    }
                }

                nExtended = 0;
                CString strExt2;
                for (StrandIndexType strandIdx = 0; strandIdx < Ns; strandIdx++)
                {
                    if (pStrandGeometry->IsExtendedStrand(segmentKey, pgsTypes::metEnd, strandIdx, pgsTypes::Straight))
                    {
                        nExtended++;
                        CString val;
                        val.Format(_T("%d, "), strandIdx + 1);
                        strExt2.Append(val);
                    }
                }

                pos = strExt2.ReverseFind(_T(','));
                if (pos != -1)
                {
                    strExt2.Delete(pos, 1);
                    strExt2.TrimRight();
                }

                if (nExtended == 0)
                {
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
                    if (!bUbeam/* && !bWFDG*/)
                    {
                        SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
                    }
                }
                else
                {
                    if (std::find(vStrandPatterns.begin(), vStrandPatterns.end(), strExt2) == vStrandPatterns.end())
                    {
                        vStrandPatterns.emplace_back(strExt2);
                    }

                    auto it = std::find(vStrandPatterns.begin(), vStrandPatterns.end(), strExt2);
                    IndexType idx = std::distance(vStrandPatterns.begin(), it);

                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, GetColumnLabel(idx));

                    if (!bUbeam/* && !bWFDG*/)
                    {
                        SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
                    }
                }

            }

            GET_IFACE2(pBroker, ICamber, pCamber);


            pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->GetDeckType();
            Float64 C;
            if (IsNonstructuralDeck(deckType))
            {

                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));

            }
            else
            {
                if (pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge)
                {
                    gdim.SetValue(pBridgeDesc->GetSlabOffset());
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                }
                else
                {
                    gdim.SetValue(pSegment->GetSlabOffset(pgsTypes::metStart));
                    const auto& val = gdim.GetValue(true);
                    gdim.SetValue(pSegment->GetSlabOffset(pgsTypes::metEnd));
                    const auto& val1 = gdim.GetValue(true);
                    strValue.Format(_T("End 1: %0.1f %s End 2: %0.1f %s"), 
                        val, gdim.GetUnitTag().c_str(), val1, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    {
                        CString strVal;
                        strVal = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                        CString strVal1;
                        strVal1 = FormatFeetInchesFromDecimalInches(RoundOff(val1, 0.125)).c_str();
                        strValue.Format(_T("End 1: %s End 2: %s"), strVal, strVal1);
                    }
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                }

                gdim.SetValue(pCamber->GetScreedCamber(poiMidSpan, pgsTypes::CreepTime::Max));
                const auto& val = gdim.GetValue(true);
                strValue.Format(_T("%0.1f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
            }


            // get # of days for creep
            Float64 Dmax_UpperBound, Dmax_Average, Dmax_LowerBound;
            Float64 Dmin_UpperBound, Dmin_Average, Dmin_LowerBound;
            pCamber->GetDCamberForGirderScheduleEx(poiMidSpan, pgsTypes::CreepTime::Max, &Dmax_UpperBound, &Dmax_Average, &Dmax_LowerBound);
            pCamber->GetDCamberForGirderScheduleEx(poiMidSpan, pgsTypes::CreepTime::Min, &Dmin_UpperBound, &Dmin_Average, &Dmin_LowerBound);

            gdim.SetValue(Dmin_LowerBound);
            const auto& val = gdim.GetValue(true);
            strValue.Format(_T("%0.1f %s"), val, gdim.GetUnitTag().c_str());
            if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

            gdim.SetValue(Dmax_UpperBound);
            const auto& val1 = gdim.GetValue(true);
            strValue.Format(_T("%0.1f %s"), val1, gdim.GetUnitTag().c_str());
            if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                strValue = FormatFeetInchesFromDecimalInches(RoundOff(val1, 0.125)).c_str();
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

            // Stirrups
            Float64 z1Spacing, z1Length;
            Float64 z2Spacing, z2Length;
            Float64 z3Spacing, z3Length;
            CWSDOTReinforcement details;
            int reinfDetailsResult = details.GetWSDOTReinforcementDetails(pBroker, segmentKey, familyCLSID, 
                &z1Spacing, &z1Length, &z2Spacing, &z2Length, &z3Spacing, &z3Length);
            if (reinfDetailsResult < 0)
            {
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
            }
            else
            {
                if (bUbeam)
                {
                    gdim.SetValue(z1Length);
                    const auto& z1Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), z1Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z1Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    gdim.SetValue(z1Spacing);
                    const auto& z1Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), z1Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z1Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

                    gdim.SetValue(z2Length);
                    const auto& z2Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), z2Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z2Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    gdim.SetValue(z2Spacing);
                    const auto& z2Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), z2Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z2Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

                    gdim.SetValue(z3Length);
                    const auto& z3Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), z3Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z3Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    gdim.SetValue(z3Spacing);
                    const auto& z3Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), z3Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z3Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                }
                else
                {
                    gdim.SetValue(z1Spacing);
                    const auto& z1Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), z1Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z1Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    gdim.SetValue(z1Length);
                    const auto& z1Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), z1Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z1Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

                    gdim.SetValue(z2Spacing);
                    const auto& z2Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), z2Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z2Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    gdim.SetValue(z2Length);
                    const auto& z2Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), z2Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z2Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

                    gdim.SetValue(z3Spacing);
                    const auto& z3Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), z3Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z3Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    gdim.SetValue(z3Length);
                    const auto& z3Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), z3Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z3Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                }
            }

            // Stirrup Height

            if (familyCLSID == CLSID_WFBeamFamily || bUbeam)
            {
                // H1 (Hg + "A" + 3")
                if (pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge)
                {
                    Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poiStart);
                    Float64 H1 = pBridgeDesc->GetSlabOffset() + Hg + WBFL::Units::ConvertToSysUnits(3.0, WBFL::Units::Measure::Inch);
                    gdim.SetValue(H1);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    strValue.Format(_T("%0.1f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                }
                else
                {
                    Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poiStart);
                    Float64 H1 = pSegment->GetSlabOffset(pgsTypes::metStart) + Hg + 
                        WBFL::Units::ConvertToSysUnits(3.0, WBFL::Units::Measure::Inch);
                    gdim.SetValue(H1);
                    const auto& val = gdim.GetValue(true);

                    pgsPointOfInterest poiEnd(poiStart);
                    poiEnd.SetDistFromStart(pBridge->GetSegmentLength(segmentKey));
                    Hg = pSectProp->GetHg(releaseIntervalIdx, poiEnd);
                    H1 = pSegment->GetSlabOffset(pgsTypes::metEnd) + Hg + WBFL::Units::ConvertToSysUnits(3.0, WBFL::Units::Measure::Inch);
                    gdim.SetValue(H1);
                    const auto& val1 = gdim.GetValue(true);
                    strValue.Format(_T("End 1: %0.1f %s End 2: %0.1f %s"), val, gdim.GetUnitTag().c_str(), val1, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    {
                        CString strVal;
                        strVal = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                        CString strVal1;
                        strVal1 = FormatFeetInchesFromDecimalInches(RoundOff(val1, 0.125)).c_str();
                        strValue.Format(_T("End 1: %s End 2: %s"), strVal, strVal1);
                    }
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                }
            }

            //longitudinal rebar
            if (familyCLSID == CLSID_SlabBeamFamily)
            {
                GET_IFACE2(pBroker, ILongitudinalRebar, pLongRebar);
                const CLongitudinalRebarData* pLRD = pLongRebar->GetSegmentLongitudinalRebarData(segmentKey);

                ATLASSERT(pLRD->RebarRows.size() <= 2);

                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 5, _T("-"));
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 5, _T("-"));

                for (const auto& row : pLRD->RebarRows)
                {
                    if (row.Face == pgsTypes::FaceType::TopFace)
                    {
                        SetColumnData(&ws, --col, nGirders * grpIdx + gdrIdx + 5, WBFL::LRFD::RebarPool::GetBarSize(row.BarSize).c_str());
                        SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 5, row.NumberOfBars);
                    }
                }

                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 5, _T("-"));
                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 5, _T("-"));

                for (const auto& row : pLRD->RebarRows)
                {
                    if (row.Face == pgsTypes::FaceType::BottomFace)
                    {
                        SetColumnData(&ws, --col, nGirders * grpIdx + gdrIdx + 5, WBFL::LRFD::RebarPool::GetBarSize(row.BarSize).c_str());
                        SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 5, row.NumberOfBars);
                    }
                }

            }

            auto pHaulingArtifact = pSegmentArtifact->GetHaulingAnalysisArtifact();
            if (pHaulingArtifact != nullptr)
            {
                GET_IFACE2(pBroker, IGirder, pIGirder);
                const WBFL::Stability::HaulingStabilityProblem* pHaulProblem = pIGirder->GetSegmentHaulingStabilityProblem(segmentKey);
                Float64 camber = pHaulProblem->GetCamber();
                Float64 precamber = pIGirder->GetPrecamber(segmentKey);
                gdim.SetValue(camber + precamber);
                const auto& val = gdim.GetValue(true);
                strValue.Format(_T("%0.1f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
            }

            auto pLiftArtifact = pSegmentArtifact->GetLiftingCheckArtifact();
            if (pLiftArtifact != nullptr)
            {
                GET_IFACE2(pBroker, ISegmentLifting, pSegmentLifting);
                Float64 L = pSegmentLifting->GetLeftLiftingLoopLocation(segmentKey);
                gdim.SetValue(L);
                const auto& val = gdim.GetValue(true);
                strValue.Format(_T("%0.1f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
            }

            if (pHaulingArtifact != nullptr)
            {
                GET_IFACE2(pBroker, IIntervals, pIntervals);
                IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
                IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);

                GET_IFACE2(pBroker, IProductForces, pProduct);
                pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

                GET_IFACE2(pBroker, ISegmentHauling, pSegmentHauling);
                Float64 trailingOverhang = pSegmentHauling->GetTrailingOverhang(segmentKey);
                Float64 leadingOverhang = pSegmentHauling->GetLeadingOverhang(segmentKey);

                gdim.SetValue(leadingOverhang);
                const auto& val = gdim.GetValue(true);
                strValue.Format(_T("%0.1f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                gdim.SetValue(trailingOverhang);
                const auto& val1 = gdim.GetValue(true);
                strValue.Format(_T("%0.1f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val1, 0.125)).c_str();
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

                if (pSegment->HandlingData.pHaulTruckLibraryEntry)
                {
                    spring.SetValue(pSegment->HandlingData.pHaulTruckLibraryEntry->GetRollStiffness());
                    const auto& val2 = spring.GetValue(true);
                    strValue.Format(_T("%0.0f"), val2);
                    // Where to stop inserting (skip leading minus, if any)
                    const int start = (!strValue.IsEmpty() && strValue[0] == _T('-')) ? 1 : 0;

                    // Position of decimal point (or end if none)
                    int dot = strValue.Find(_T('.'));
                    if (dot < 0) dot = strValue.GetLength();

                    // Insert commas every 3 to the left of the decimal
                    for (int i = dot - 3; i > start; i -= 3)
                        strValue.Insert(i, _T(','));

                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                }

                if (pSegment->HandlingData.pHaulTruckLibraryEntry)
                {
                    gdim.SetValue(pSegment->HandlingData.pHaulTruckLibraryEntry->GetAxleWidth());
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.1f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                }
            }

            //set girder range
            CString strGirder;

            bool bSame = true;

            if (m_previous_row_data.size() != m_current_row_data.size())
                bSame = false;
            else
            {
                for (size_t i = 1; i < m_previous_row_data.size(); ++i)
                {
                    if (m_previous_row_data[i].CompareNoCase(m_current_row_data[i]) != 0)
                          bSame = false;
                }
            }

            m_previous_row_data = m_current_row_data;

            if (bSame)
            {
                strGirder.Format(_T("%s-%s"), GetColumnLabel(m_last_same_gdrID), GetColumnLabel(gdrIdx));
                SetColumnData(&ws, 1, nGirders * grpIdx + m_last_same_gdrID + (bSlab ? 5 : 4), strGirder);
            }
            else
            {
                m_last_same_gdrID = gdrIdx;
                SetColumnData(&ws, 1, nGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), GetColumnLabel(gdrIdx));
            }
            
            //merge and format girder cells
            CString strCell;
            strCell.Format(_T("B%d:B%d"), nGirders* grpIdx + m_last_same_gdrID + (bSlab ? 6 : 5), nGirders * grpIdx + gdrIdx + (bSlab ? 6 : 5));
            Range cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
            cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
            cell.BorderAround(
                COleVariant((long)1),
                (long)3,
                (long)-4105,
                COleVariant((long)0)
            );



        } // gdrIdx

        //set span
        SpanIndexType spanIdx = girderKey.groupIndex;
        CString strSpan;
        strSpan.Format(_T("%d"), spanIdx + 1);
        SetColumnData(&ws, 0, (bSlab ? 5 : 4) + grpIdx * (nGirders + 1), strSpan);

        //merge and format span cells
        CString strCell;
        strCell.Format(_T("A%d:A%d"), (grpIdx* nGirders) + (bSlab ? 6 : 5), (grpIdx* nGirders) + ((bSlab ? 5 : 4) + nGirders));
        Range cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
        cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
        cell.BorderAround(
            COleVariant((long)1),
            (long)3,
            (long)-4105,
            COleVariant((long)0)
        );

    }

    //build strand extension table

    RowIndexType tableOffset = (bSlab ? 6 : 5);
    GET_IFACE2(pBroker, IBridge, pBridge);
    for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
    {
        tableOffset += pBridge->GetGirderCount(grpIdx);
    }

    const std::vector<ScheduleHeaderInfo>& headerInfo =
    {
        {0, 5, tableOffset, 1, 0, _T("EXTENDED STRAND SCHEDULE")},
        {0, 3, tableOffset + 1, 1, 0, _T("STRAND PATTERN")},
        {3, 2, tableOffset + 1, 1, 0, _T("STRANDS TO EXTEND")}
    };

    for (const auto& info : headerInfo)
    {
        CString strCell;
        strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(info.colIdx), info.rowIdx + 1, GetColumnLabel(info.colIdx + info.colSpan - 1), info.rowIdx + info.rowSpan);
        Range cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
        cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
        cell.SetValue2(COleVariant(info.strValue));
        cell.BorderAround(
            COleVariant((long)1),
            (long)3,
            (long)-4105,
            COleVariant((long)0)
        );

        COleDispatchDriver font(cell.GetFont(), FALSE);

        font.SetProperty(0x60, VT_BOOL, TRUE);


        if (info.rowIdx == tableOffset)
        {
            font.SetProperty(0x68, VT_R8, 20.0);
        }
        else
        {
            font.SetProperty(0x68, VT_R8, 16.0);
        }
    }


    for (IndexType idx = 0; idx < vStrandPatterns.size() ; idx++)
    {
        CString strCell;

        strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(0), tableOffset + 3 + idx, GetColumnLabel(2), tableOffset + 3 + idx);
        Range cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
        cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
        cell.SetValue2(COleVariant(GetColumnLabel(idx)));
        cell.BorderAround(
            COleVariant((long)1),
            (long)3,
            (long)-4105,
            COleVariant((long)0)
        );

        strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(3), tableOffset + 3 + idx, GetColumnLabel(4), tableOffset + 3 + idx);
        cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
        cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
        cell.SetValue2(COleVariant(vStrandPatterns[idx]));
        cell.BorderAround(
            COleVariant((long)1),
            (long)3,
            (long)-4105,
            COleVariant((long)0)
        );

    }


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

