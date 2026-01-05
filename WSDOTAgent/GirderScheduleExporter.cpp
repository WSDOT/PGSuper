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
#include <psgLib\SpecLibraryEntry.h>
#include <psgLib/CreepCriteria.h>
#include <psgLib/LimitsCriteria.h>
#include <psgLib/GirderLibraryEntry.h>
#include <Plugins/BeamFamilyCLSID.h>
#include "WSDOTReinforcement.h"
#include "DebondResults.h"



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

bool CGirderScheduleExporter::IsSameGeometry(const ScheduleRowData& a, const ScheduleRowData& b)
    {
        // Same series
        if (a.girderSeries.CompareNoCase(b.girderSeries) != 0)
            return false;

        const auto& tol = WBFL::Units::ConvertToSysUnits(0.125, WBFL::Units::Measure::Inch);

        if (!IsEqual(a.planLength, b.planLength, tol)) return false;

        // Basic geometric properties
        if (!IsEqual(a.topWidth, b.topWidth, tol)) return false;
        if (!IsEqual(a.Hg, b.Hg, tol)) return false;
        if (!IsEqual(a.nVoids, b.nVoids)) return false;
        if (!IsEqual(a.ExtVoidDiameter, b.ExtVoidDiameter, tol)) return false;
        if (!IsEqual(a.IntVoidDiameter, b.IntVoidDiameter, tol)) return false;
        if (!IsEqual(a.P1, b.P1, tol)) return false;
        if (!IsEqual(a.P2, b.P2, tol)) return false;
        if (!IsEqual(a.t1, b.t1, tol)) return false;
        if (!IsEqual(a.t2, b.t2, tol)) return false;

        return true;
    };

void CGirderScheduleExporter::AddDesignerNudges()
{


    auto same_rebar =
        [&](const ScheduleRowData& a, const ScheduleRowData& b) -> bool
        {
            // stirrup height (H1)
            if (!IsEqual(a.H1, b.H1)) return false;
            if (!IsEqual(a.H1end1, b.H1end1)) return false;
            if (!IsEqual(a.H1end2, b.H1end2)) return false;

            //shear reinforcement
            if (!IsEqual(a.z1Length, b.z1Length)) return false;
            if (!IsEqual(a.z2Length, b.z2Length)) return false;
            if (!IsEqual(a.z3Length, b.z3Length)) return false;

            if (!IsEqual(a.z1Spacing, b.z1Spacing)) return false;
            if (!IsEqual(a.z2Spacing, b.z2Spacing)) return false;
            if (!IsEqual(a.z3Spacing, b.z3Spacing)) return false;

            if (a.z1Size != b.z1Size) return false;
            if (a.z2Size != b.z2Size) return false;
            if (a.z3Size != b.z3Size) return false;

            //longitudinal reinforcement
            if (a.vG1LongBarSize.size() > 0 && b.vG1LongBarSize.size() > 0)
            {
                // G1 (top flange)
                if (a.vG1LongBarSize.size() != b.vG1LongBarSize.size()) return false;
                if (a.vG1NumLongBars.size() != b.vG1NumLongBars.size()) return false;

                for (int i = 0; i < a.vG1LongBarSize.size(); ++i)
                {
                    if (a.vG1LongBarSize[i] != b.vG1LongBarSize[i]) return false;
                    if (a.vG1NumLongBars[i] != b.vG1NumLongBars[i]) return false;
                }

                // G2 (bottom flange)
                if (a.vG2LongBarSize.size() != b.vG2LongBarSize.size()) return false;
                if (a.vG2NumLongBars.size() != b.vG2NumLongBars.size()) return false;

                for (int i = 0; i < a.vG2LongBarSize.size(); ++i)
                {
                    if (a.vG2LongBarSize[i] != b.vG2LongBarSize[i]) return false;
                    if (a.vG2NumLongBars[i] != b.vG2NumLongBars[i]) return false;
                }
            }

            return true;
        };


    auto same_shipping =
        [&](const ScheduleRowData& a, const ScheduleRowData& b) -> bool
        {
            if (!IsEqual(a.liftingLoopLocation, b.liftingLoopLocation)) return false;
            if (!IsEqual(a.trailingOverhang, b.trailingOverhang)) return false;
            if (!IsEqual(a.leadingOverhang, b.leadingOverhang)) return false;
            if (!IsEqual(a.rollStiffness, b.rollStiffness)) return false;
            if (!IsEqual(a.wheelSpacing, b.wheelSpacing)) return false;
            return true;
        };



    // add designer nudges to consider unifying girders
    for (int i = 0; i < m_schedule_data.size(); ++i)
    {
        const auto& girder_i = m_schedule_data[i];

        for (int j = i + 1; j < m_schedule_data.size(); ++j)
        {
            const auto& girder_j = m_schedule_data[j];

            // Only care about pairs with exact same basic geometry
            if (!IsSameGeometry(girder_i, girder_j))
                continue;

            // Permanent strand count differs by <= 2
            StrandIndexType total_i = girder_i.Ns + girder_i.Nh;
            StrandIndexType total_j = girder_j.Ns + girder_j.Nh;

            StrandIndexType diff = static_cast<StrandIndexType>(
                std::abs(static_cast<long>(total_i) -
                    static_cast<long>(total_j)));

            if (diff > 0 && diff <= 2)
            {
                CString msg;
                msg.Format(
                    _T("%s and %s have the same basic geometry but permanent strand counts differ by %d. "
                        "Consider using the higher count for both girders if the design is acceptable."),
                    GIRDER_LABEL(girder_i.girderKey), GIRDER_LABEL(girder_j.girderKey), diff
                );

                VARIANT var;
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = RGB(0, 0, 0);
                m_optimizations.emplace_back(msg, _T("Color"), var);
            }

            // Temporary strand count differs by <= 2
            StrandIndexType t_diff = static_cast<StrandIndexType>(
                std::abs(static_cast<long>(girder_i.Nt) -
                    static_cast<long>(girder_j.Nt)));

            if (diff == 0 && t_diff > 0 && t_diff <= 2)
            {
                CString msg;
                msg.Format(
                    _T("%s and %s have the same basic geometry and permanent strand count but temporary strand counts differ by %d. "
                        "Consider using the higher count for both girders if the design is acceptable."),
                    GIRDER_LABEL(girder_i.girderKey), GIRDER_LABEL(girder_j.girderKey), t_diff
                );

                VARIANT var;
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = RGB(0, 0, 0);
                m_optimizations.emplace_back(msg, _T("Color"), var);
            }

            // extended strand count differs by <= 2
            StrandIndexType l_diff = static_cast<StrandIndexType>(
                std::abs(static_cast<long>(girder_i.nExtendedL) -
                    static_cast<long>(girder_j.nExtendedL)));

            StrandIndexType r_diff = static_cast<StrandIndexType>(
                std::abs(static_cast<long>(girder_i.nExtendedR) -
                    static_cast<long>(girder_j.nExtendedR)));

            if (diff == 0 && t_diff == 0 && l_diff > 0 && l_diff <= 2 && r_diff > 0 && r_diff <= 2)
            {
                CString msg;
                msg.Format(
                    _T("%s and %s have the same basic geometry and total strand count but extended strand counts on either side differ by %d. "
                        "Consider using the same count for both girders if the design is acceptable."),
                    GIRDER_LABEL(girder_i.girderKey), GIRDER_LABEL(girder_j.girderKey), l_diff
                );

                VARIANT var;
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = RGB(0, 0, 0);
                m_optimizations.emplace_back(msg, _T("Color"), var);
            }


            if (!same_rebar(girder_i, girder_j))
            {
                CString msg;
                msg.Format(
                    _T("%s and %s have the same basic geometry but but have different rebar configurations. "
                        "Consider using the same rebar configuration for both girders if the design is acceptable."),
                    GIRDER_LABEL(girder_i.girderKey), GIRDER_LABEL(girder_j.girderKey)
                );

                VARIANT var;
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = RGB(0, 0, 0);
                m_optimizations.emplace_back(msg, _T("Color"), var);
            }

            // shipping and handling details
            if (!same_shipping(girder_i, girder_j))
            {
                CString msg;
                msg.Format(
                    _T("%s and %s have the same basic geometry, but differ in shipping and handling details. "
                        "Consider using the same shipping and handling details if the design is acceptable."),
                    GIRDER_LABEL(girder_i.girderKey), GIRDER_LABEL(girder_j.girderKey)
                );

                VARIANT var;
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = RGB(0, 0, 0);
                m_optimizations.emplace_back(msg, _T("Color"), var);
            }

        }
    }
}


void CGirderScheduleExporter::NormalizeCamber(std::shared_ptr<WBFL::EAF::Broker> pBroker)
{
    const auto tol = WBFL::Units::ConvertToSysUnits(0.125, WBFL::Units::Measure::Inch);

    int chainStart = 0;
    int firstMatchedPrevIdx = -1; // first row that matched its previous row in the current chain

    auto in_same_chain = [&](int i, int j)
        {
            const auto& a = m_schedule_data[i];
            const auto& b = m_schedule_data[j];

            // only chain if they are the same basic girder
            if (!IsSameGeometry(a, b)) return false;

			Float64 Ci = RoundOff(a.C, tol);
			Float64 Cj = RoundOff(b.C, tol);
            if (!IsLE(abs(Ci - Cj), tol)) return false;
            Float64 D40i = RoundOff(a.DminLowerBound, tol);
            Float64 D40j = RoundOff(b.DminLowerBound, tol);
            if (!IsLE(abs(D40i - D40j), tol)) return false;
            Float64 D120i = RoundOff(a.DmaxUpperBound, tol);
            Float64 D120j = RoundOff(b.DmaxUpperBound, tol);
            if (!IsLE(abs(D120i - D120j), tol)) return false;

            GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
            INIT_FRACTIONAL_LENGTH_PROTOTYPE(gdim, IS_US_UNITS(pDisplayUnits), 8, RoundUp, pDisplayUnits->GetComponentDimUnit(), true, true);

            VARIANT var;
            VariantInit(&var);
            var.vt = VT_I4;
            var.lVal = RGB(0, 0, 0);
            CString strValue;

            if (!IsEqual(a.C, b.C, 0.00001))
            {
                strValue.Format(_T("%s and %s are nearly identical except that the Screed Camber (C) differs by %f %s. The maximum C is used so girders can be normalized."),
                    GIRDER_LABEL(a.girderKey), GIRDER_LABEL(b.girderKey), abs(a.C - b.C), gdim.GetUnitTag().c_str());
                m_warnings.emplace_back(strValue, _T("Color"), var);
            }

            if (!IsEqual(a.DminLowerBound, b.DminLowerBound, 0.00001))
            {
                strValue.Format(_T("%s and %s are nearly identical except that the Lower Bound Camber (D40) differs by %f %s. The maximum D40 is used so girders can be normalized."),
                    GIRDER_LABEL(a.girderKey), GIRDER_LABEL(b.girderKey), abs(a.DminLowerBound - b.DminLowerBound), gdim.GetUnitTag().c_str());
                m_warnings.emplace_back(strValue, _T("Color"), var);
            }

            if (!IsEqual(a.DmaxUpperBound, b.DmaxUpperBound, 0.00001))
            {
                strValue.Format(_T("%s and %s are nearly identical except that the Upper Bound Camber (D120) differs by %f %s. The maximum D120 is used so girders can be normalized."),
                    GIRDER_LABEL(a.girderKey), GIRDER_LABEL(b.girderKey), abs(a.DmaxUpperBound - b.DmaxUpperBound), gdim.GetUnitTag().c_str());
                m_warnings.emplace_back(strValue, _T("Color"), var);
            }

            return true;
        };

    const int n = static_cast<int>(m_schedule_data.size());
    for (int i = 1; i <= n; ++i)
    {
        // consecutive-pair match
        const bool pairMatches = (i < n) && in_same_chain(i - 1, i);

        // remember the first row that matched its previous row in this chain
        if (pairMatches && firstMatchedPrevIdx == -1)
            firstMatchedPrevIdx = i - 1;

        // chain continues if either the immediate previous matches,
        // or this row matches the first row that previously matched its previous row
        const bool chainContinues =
            (i < n) && (pairMatches || (firstMatchedPrevIdx != -1 && in_same_chain(firstMatchedPrevIdx, i)));

        if (chainContinues)
        {
            // finalize [chainStart, i)
            Float64 C = m_schedule_data[chainStart].C;
            Float64 D40 = m_schedule_data[chainStart].DminLowerBound;
            Float64 D120 = m_schedule_data[chainStart].DmaxUpperBound;

            for (int k = chainStart + 1; k < i; ++k)
            {
                C = max(C, m_schedule_data[k].C);
                D40 = max(D40, m_schedule_data[k].DminLowerBound);
                D120 = max(D120, m_schedule_data[k].DmaxUpperBound);
            }

            // push the max back onto everyone in the chain
            for (int k = chainStart; k < i; ++k)
            {
                m_schedule_data[k].C = C;
                m_schedule_data[k].DminLowerBound = D40;
                m_schedule_data[k].DmaxUpperBound = D120;
            }
        }
        else
        {
            // start a new chain at i
            chainStart = i;
            firstMatchedPrevIdx = -1; // reset remembered first-match index for next chain
        }
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
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
    GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
    const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
    pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->GetDeckType();

    WBFL::EAF::AutoProgress ap(pProgress);

    // Get the Excel template file folder
    CEAFApp* pApp = EAFGetApp();
    CString str = pApp->GetAppLocation();

    CString strExcelTemplateFolder;
    if (-1 != str.Find(_T("RegFreeCOM")))
    {
        // application is on a development box
        if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
            if (pBridgeDesc->GetSlabOffsetType() != pgsTypes::sotBridge)
               strExcelTemplateFolder = str.Left(2) + CString(_T("\\ARP\\PGSuper\\WSDOTAgent\\Template US Units\\Varied Haunch\\"));
            else
               strExcelTemplateFolder = str.Left(2) + CString(_T("\\ARP\\PGSuper\\WSDOTAgent\\Template US Units\\"));
        else
            if (pBridgeDesc->GetSlabOffsetType() != pgsTypes::sotBridge)
               strExcelTemplateFolder = str.Left(2) + CString(_T("\\ARP\\PGSuper\\WSDOTAgent\\Template SI Units\\Varied Haunch\\"));
            else
               strExcelTemplateFolder = str.Left(2) + CString(_T("\\ARP\\PGSuper\\WSDOTAgent\\Template SI Units\\"));

    }
    else
    {
        // make sure we have a trailing backslash
        if (_T('\\') != str.GetAt(str.GetLength() - 1))
        {
            str += _T("\\");
        }

        if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
           if (pBridgeDesc->GetSlabOffsetType() != pgsTypes::sotBridge)
              strExcelTemplateFolder = str + CString(_T("Template US Units\\Varied Haunch\\"));
           else
              strExcelTemplateFolder = str + CString(_T("Template US Units\\"));
        else
           if (pBridgeDesc->GetSlabOffsetType() != pgsTypes::sotBridge)
              strExcelTemplateFolder = str + CString(_T("Template SI Units\\Varied Haunch\\"));
           else
              strExcelTemplateFolder = str + CString(_T("Template SI Units\\"));
    }

    //Set girder data

    GroupIndexType grpHeaderIdx = 0;
    const CGirderGroupData* pGroupHeader = pBridgeDesc->GetGirderGroup(grpHeaderIdx);
    const CSplicedGirderData* pGirderHeader = pGroupHeader->GetGirder(0);
    const GirderLibraryEntry* pGdrLibEntry = pGirderHeader->GetGirderLibraryEntry();
    auto factory = pGdrLibEntry->GetBeamFactory();
    auto girder_name = pGdrLibEntry->GetName();
    CLSID familyCLSID = factory->GetFamilyCLSID();

    bool bIbeam = (familyCLSID == CLSID_WFBeamFamily);
    bool bSlab = (familyCLSID == CLSID_SlabBeamFamily);
    bool bUbeam = (familyCLSID == CLSID_UBeamFamily);
    bool bWFDG = (familyCLSID == CLSID_DeckBulbTeeBeamFamily);
    bool bWFTDG = (familyCLSID == CLSID_DeckBulbTeeBeamFamily);

    CString strTemplateName;

    if (bWFDG && girder_name.find(L"TDG") != std::wstring::npos)
        strTemplateName = strExcelTemplateFolder + _T("WFTDG_Girder_Schedule.xltx");
    else if (bWFDG && girder_name.find(L"DG") != std::wstring::npos)
        strTemplateName = strExcelTemplateFolder + _T("WFDG_Girder_Schedule.xltx");
    else if (bSlab)
        strTemplateName = strExcelTemplateFolder + _T("Slab_Girder_Schedule.xltx");
    else if (bUbeam)
        strTemplateName = strExcelTemplateFolder + _T("U_Beam_Schedule.xltx");
    else 
        strTemplateName = strExcelTemplateFolder + _T("WF_Girder_Schedule.xltx");


    _Application excel;
    if (!excel.CreateDispatch(_T("Excel.Application")))
    {
        AfxMessageBox(_T("An error occurred while attempting to run Excel. Excel files cannot be created unless Microsoft Excel is installed."));
        return FALSE;
    }

    // get the spreadsheet setup
    Workbooks workbooks = excel.GetWorkbooks();
    _Workbook workbook = workbooks.Add(COleVariant(strTemplateName)); // creates a new Excel file from the template

    Worksheets worksheets = workbook.GetWorksheets();


    // Format cells
    _Worksheet ws = worksheets.GetItem(COleVariant(1L));
    Range allCells = ws.GetCells();
    COleVariant vCenter((long)-4108, VT_I4);
    allCells.SetHorizontalAlignment(vCenter);

    _Worksheet ws2;
    Range allCells2;

    INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);
    INIT_UV_PROTOTYPE(rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), true);
    INIT_UV_PROTOTYPE(rptMomentPerAngleUnitValue, spring, pDisplayUnits->GetMomentPerAngleUnit(), true);

    INIT_FRACTIONAL_LENGTH_PROTOTYPE(gdim, IS_US_UNITS(pDisplayUnits), 8, RoundUp, pDisplayUnits->GetComponentDimUnit(), true, true);
    INIT_FRACTIONAL_LENGTH_PROTOTYPE(glength, IS_US_UNITS(pDisplayUnits), 8, RoundOff, pDisplayUnits->GetSpanLengthUnit(), true, true);




    if (!(bSlab || bUbeam || bWFDG || bIbeam))
    {
        AfxMessageBox(_T("There is no schedule for the selected girder type"));
        return E_FAIL;
    }

    CComPtr<IAnnotatedDisplayUnitFormatter> pADUF;
    pADUF.CoCreateInstance(CLSID_AnnotatedDisplayUnitFormatter);
    
    GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
    CGirderKey girderKey;

    std::vector<CString> vStrandPatterns;
    ConfigStrandFillVector vDebond;

    GET_IFACE2(pBroker, IBridge, pBridge);
    GET_IFACE2(pBroker, IGirder, pIGirder);

    IndexType tf_rows = 0;

    m_schedule_data.clear();
    m_debond_schedule.clear();

    for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
    {
        m_previous_row_data.clear();
        
        GirderIndexType gdrIdx = m_last_same_gdrID = 0;
        const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
        GirderIndexType nGirders = pGroup->GetGirderCount();
        const CGirderGroupData* pPrevGroup = pBridgeDesc->GetGirderGroup((grpIdx == 0? 0: grpIdx - 1));
        GirderIndexType nPrevGirders = pPrevGroup->GetGirderCount();

        // write data
        for (gdrIdx; gdrIdx < nGirders; gdrIdx++)
        {
            ScheduleRowData rowData;

            girderKey = CGirderKey(grpIdx, gdrIdx);
            rowData.girderKey = girderKey;

            const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);

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
                rowData.girderSeries = strSeries;

                if (bWFDG)
                {
                    Float64 W = pIGirder->GetTopWidth(poiMidSpan);
                    rowData.topWidth = W;
                }
            }
            else
            {
                Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poiMidSpan);
                rowData.Hg = Hg;

                GET_IFACE2(pBroker, IGirder, pIGirder);
                Float64 W = pIGirder->GetTopWidth(poiMidSpan);
                rowData.topWidth = W;
            }

            //Set Plan Length
            rowData.planLength = pBridge->GetSegmentPlanLength(segmentKey);

            //int. diaphragm type or voids
            if (bSlab)
            {
                const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(girderKey.girderIndex)->GetGirderLibraryEntry();
                Float64 nVoids = pGdrEntry->GetDimension(_T("Number_of_Voids"));
                rowData.nVoids = nVoids;
                Float64 ExtVoidDiameter = pGdrEntry->GetDimension(_T("D1"));
                rowData.ExtVoidDiameter = ExtVoidDiameter;
                Float64 IntVoidDiameter = pGdrEntry->GetDimension(_T("D2"));
                rowData.IntVoidDiameter = IntVoidDiameter;
            }

            CComPtr<IAngle> objAngle1, objAngle2;
            pBridge->GetSegmentSkewAngle(segmentKey, pgsTypes::metStart, &objAngle1);
            pBridge->GetSegmentSkewAngle(segmentKey, pgsTypes::metEnd, &objAngle2);
            Float64 t1, t2;
            objAngle1->get_Value(&t1);
            objAngle2->get_Value(&t2);
            rowData.t1 = t1;
            rowData.t2 = t2;

            if (!bSlab)
            {
                // P1 & P2...
                // does not apply to slabs
                PierIndexType prevPierIdx = (PierIndexType)grpIdx;
                PierIndexType nextPierIdx = prevPierIdx + 1;
                bool bContinuousLeft, bContinuousRight, bIntegralLeft, bIntegralRight;

                pBridge->IsContinuousAtPier(prevPierIdx, &bContinuousLeft, &bContinuousRight);
                pBridge->IsIntegralAtPier(prevPierIdx, &bIntegralLeft, &bIntegralRight);

                if (!(bContinuousLeft || bIntegralLeft))
                {
                    // start end distance is a plan view dimension that needs to
                    // be adjusted for the installed girder slope and the height of the girder
                    Float64 D = pBridge->GetSegmentStartEndDistance(segmentKey);
                    Float64 slope = pBridge->GetSegmentSlope(segmentKey);
                    Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poiMidSpan);
                    Float64 P1 = D * sqrt(1 + slope * slope) - slope * Hg;

                    rowData.P1 = P1;
                }

                pBridge->IsContinuousAtPier(nextPierIdx, &bContinuousLeft, &bContinuousRight);
                pBridge->IsIntegralAtPier(nextPierIdx, &bIntegralLeft, &bIntegralRight);

                if (!(bContinuousRight || bIntegralRight))
                {
                    // start end distance is a plan view dimension that needs to
                    // be adjusted for the installed girder slope and the height of the girder
                    Float64 D = pBridge->GetSegmentEndEndDistance(segmentKey);
                    Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poiMidSpan);
                    Float64 slope = pBridge->GetSegmentSlope(segmentKey);
                    Float64 P2 = D * sqrt(1 + slope * slope) + slope * Hg;

                    rowData.P2 = P2;
                }
            }

            GET_IFACE2(pBroker, IMaterials, pMaterial);

            rowData.fc = pMaterial->GetSegmentDesignFc(segmentKey, finalIntervalIdx);
            rowData.fci = pMaterial->GetSegmentDesignFc(segmentKey, releaseIntervalIdx);


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

            rowData.bCanReportPrestressInformation = bCanReportPrestressInformation;

            StrandIndexType Ns = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Straight);
            StrandIndexType Nh = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Harped);

            std::vector<CDebondResults::DebondInformation> debondInfo;
            CDebondResults results;
            int debondResults = results.GetDebondDetails(pBroker, segmentKey, debondInfo);

            if (bCanReportPrestressInformation)
            {

                if (!bSlab)
                {
                    StrandIndexType nDebonded = pStrandGeometry->GetNumDebondedStrands(segmentKey, pgsTypes::Straight, pgsTypes::dbetEither);
                    rowData.nDebonded = nDebonded;

                    rowData.Ns = Ns;
                    rowData.Nh = Nh;

                    StrandIndexType Nt = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Temporary);
                    rowData.Nt = Nt;
                }
                else
                {

                    RowIndexType nRows = 3;

                    for (RowIndexType rowIdx = 0; rowIdx < nRows; rowIdx++)
                    {

                        StrandIndexType nStrandsInRow = pStrandGeometry->GetNumStrandInRow(poiStart, rowIdx, pgsTypes::Straight);

                        rowData.nStrandsInRows[rowIdx] = nStrandsInRow;

                        ConfigStrandFillVector vStrandsInRow = pStrandGeometry->GetStrandsInRow(poiStart, rowIdx, pgsTypes::Straight);

                        rowData.vStrandsInRow[rowIdx] = vStrandsInRow;

                        StrandIndexType nDebonded = 0;

                        if (rowIdx <= 1)
                        {
                            std::map<Float64, StrandIndexType> mCountPerDebondLength;
                            std::map<Float64, ConfigStrandFillVector> mStrandIdsPerDebondLength;

                            ConfigStrandFillVector vStrandsInRow = pStrandGeometry->GetStrandsInRow(poiStart, rowIdx, pgsTypes::Straight);
                            ConfigStrandFillVector vDebondStrandsInRow;

                            if (Ns > 0)
                            {

                                for (const auto& strandIdx : vStrandsInRow)
                                {
                                    bool bExtended = pStrandGeometry->IsExtendedStrand(poiStart, strandIdx, pgsTypes::Straight);
                                    if (bExtended)
                                    {
                                        rowData.vExtStrandsInRow[rowIdx].emplace_back(strandIdx);
                                    }

                                    bool bDebonded = pStrandGeometry->IsStrandDebonded(poiStart, strandIdx, pgsTypes::Straight);

                                    if (bDebonded)
                                    {
                                        nDebonded++;
                                        rowData.vDebStrandsInRow[rowIdx].emplace_back(strandIdx);

                                        std::vector<CDebondResults::DebondInformation>::iterator iter(debondInfo.begin());
                                        std::vector<CDebondResults::DebondInformation>::iterator end(debondInfo.end());

                                        for (; iter != end; iter++)
                                        {
                                            CDebondResults::DebondInformation& dbInfo = *iter;

                                            if (std::find(dbInfo.Strands.begin(), dbInfo.Strands.end(), (strandIdx + 1)) != dbInfo.Strands.end())
                                            {
                                                mCountPerDebondLength[dbInfo.Length]++;
                                                mStrandIdsPerDebondLength[dbInfo.Length].emplace_back(strandIdx + 1);
                                            }
                                        }

                                    }

                                }

                                rowData.nDebondedPerLength[rowIdx] = mCountPerDebondLength;
                                rowData.strandIdsPerLength[rowIdx] = mStrandIdsPerDebondLength;

                                if (nDebonded > 0 && mStrandIdsPerDebondLength.size() > 0)
                                {
                                    if (std::find(m_debond_schedule.begin(), m_debond_schedule.end(), 
                                        mStrandIdsPerDebondLength) == m_debond_schedule.end())
                                    {
                                        m_debond_schedule.emplace_back(mStrandIdsPerDebondLength);
                                    }

                                }

                            }

                        }
                        else
                        {
                            StrandIndexType Nt = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Temporary);
                            rowData.Nt = Nt;
                        }

                    }

                }

            }

            if (CLSID_SlabBeamFamily != familyCLSID)
            {

                Float64 ybg = pSectProp->GetY(releaseIntervalIdx, poiMidSpan, pgsTypes::BottomGirder);
                Float64 sse = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poiMidSpan, pgsTypes::Straight).Y();
                if (0 < Ns)
                {
                    rowData.E = ybg - sse;
                }

                Float64 hse = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poiMidSpan, pgsTypes::Harped).Y();
                if (0 < Nh)
                {
                    rowData.Fcl = ybg - hse;
                }

                Float64 ytg = pSectProp->GetY(releaseIntervalIdx, poiStart, pgsTypes::TopGirder);
                Float64 hss = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poiStart, pgsTypes::Harped).Y();
                if (0 < Nh)
                {
                    rowData.Fo = ytg + hss;
                }

                //strand extensions
                StrandIndexType nExtendedL = 0;
				rowData.vExtendedL.clear();

                for (StrandIndexType strandIdx = 0; strandIdx < Ns; strandIdx++)
                {
                    if (pStrandGeometry->IsExtendedStrand(segmentKey, pgsTypes::metStart, strandIdx, pgsTypes::Straight))
                    {
                        rowData.vExtendedL.emplace_back(strandIdx);
                        nExtendedL++;
                    }
                }

                rowData.nExtendedL = nExtendedL;


                StrandIndexType nExtendedR = 0;
                rowData.vExtendedR.clear();

                for (StrandIndexType strandIdx = 0; strandIdx < Ns; strandIdx++)
                {
                    if (pStrandGeometry->IsExtendedStrand(segmentKey, pgsTypes::metEnd, strandIdx, pgsTypes::Straight))
                    {
                        rowData.vExtendedR.emplace_back(strandIdx);
                        nExtendedR++;
                    }
                }

                rowData.nExtendedR = nExtendedR;

            }


            GET_IFACE2(pBroker, ICamber, pCamber);

            Float64 C;
            if (IsNonstructuralDeck(deckType))
            {
                C = pCamber->GetExcessCamber(poiMidSpan, pgsTypes::CreepTime::Max);
            }
            else
            {
                if (pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge)
                {
                    rowData.A = pBridgeDesc->GetSlabOffset();
                }
                else
                {
                    rowData.Aend1 = pSegment->GetSlabOffset(pgsTypes::metStart);
                    rowData.Aend2 = pSegment->GetSlabOffset(pgsTypes::metEnd);
                }

                C = pCamber->GetScreedCamber(poiMidSpan, pgsTypes::CreepTime::Max);
                rowData.C = C;
            }

            // get # of days for creep
            Float64 Dmax_UpperBound, Dmax_Average, Dmax_LowerBound;
            Float64 Dmin_UpperBound, Dmin_Average, Dmin_LowerBound;
            pCamber->GetDCamberForGirderScheduleEx(poiMidSpan, pgsTypes::CreepTime::Max, &Dmax_UpperBound, &Dmax_Average, &Dmax_LowerBound);
            pCamber->GetDCamberForGirderScheduleEx(poiMidSpan, pgsTypes::CreepTime::Min, &Dmin_UpperBound, &Dmin_Average, &Dmin_LowerBound);

            rowData.DminLowerBound = Dmin_LowerBound;
            rowData.DmaxUpperBound = Dmax_UpperBound;

            // Stirrups
            Float64 z1Spacing, z1Length, z2Spacing, z2Length, z3Spacing, z3Length;
            WBFL::Materials::Rebar::Size z1Size, z2Size, z3Size;
            CWSDOTReinforcement details;
            rowData.reinfDetailsResult = details.GetWSDOTReinforcementDetails(pBroker, segmentKey, familyCLSID,
                &z1Size, &z1Spacing, &z1Length, &z2Size, &z2Spacing, &z2Length, &z3Size, &z3Spacing, &z3Length);
            if (rowData.reinfDetailsResult >= 0)
            {
                if (bUbeam)
                {
                    rowData.z1Length = z1Length;
                    rowData.z1Spacing = z1Spacing;
                    rowData.z2Length = z2Length;
                    rowData.z2Spacing = z2Spacing;
                    rowData.z3Length = z3Length;
                    rowData.z3Spacing = z3Spacing;
                }
                else
                {
                    if (bSlab)
                    {
                        rowData.z1Size = z1Size;
                    }
                    rowData.z1Spacing = z1Spacing;
                    rowData.z1Length = z1Length;
                    if (bSlab)
                    {
                        rowData.z2Size = z2Size;
                    }
                    rowData.z2Spacing = z2Spacing;
                    rowData.z2Length = z2Length;
                    if (bSlab)
                    {
                        rowData.z3Size = z3Size;
                    }
                    rowData.z3Spacing = z3Spacing;
                    rowData.z3Length = z3Length;
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
                    rowData.H1 = H1;
                }
                else
                {
                    Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poiStart);
                    Float64 H1 = pSegment->GetSlabOffset(pgsTypes::metStart) + Hg +
                        WBFL::Units::ConvertToSysUnits(3.0, WBFL::Units::Measure::Inch);
                    rowData.H1end1 = H1;

                    pgsPointOfInterest poiEnd(poiStart);
                    poiEnd.SetDistFromStart(pBridge->GetSegmentLength(segmentKey));
                    Hg = pSectProp->GetHg(releaseIntervalIdx, poiEnd);
                    H1 = pSegment->GetSlabOffset(pgsTypes::metEnd) + Hg + 
                        WBFL::Units::ConvertToSysUnits(3.0, WBFL::Units::Measure::Inch);
                    rowData.H1end2 = H1;
                }
            }

            //longitudinal rebar
            if (familyCLSID == CLSID_SlabBeamFamily)
            {
                GET_IFACE2(pBroker, ILongitudinalRebar, pLongRebar);
                const CLongitudinalRebarData* pLRD = pLongRebar->GetSegmentLongitudinalRebarData(segmentKey);

                ATLASSERT(pLRD->RebarRows.size() <= 2);

                for (const auto& row : pLRD->RebarRows)
                {
                    if (row.Face == pgsTypes::FaceType::TopFace)
                    {
                        rowData.vG1LongBarSize.emplace_back(row.BarSize);
                        rowData.vG1NumLongBars.emplace_back(row.NumberOfBars);
                    }
                }

                for (const auto& row : pLRD->RebarRows)
                {
                    if (row.Face == pgsTypes::FaceType::BottomFace)
                    {
                        rowData.vG2LongBarSize.emplace_back(row.BarSize);
                        rowData.vG2NumLongBars.emplace_back(row.NumberOfBars);
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
                rowData.midspanDeflection = camber + precamber;
            }

            auto pLiftArtifact = pSegmentArtifact->GetLiftingCheckArtifact();
            if (pLiftArtifact != nullptr)
            {
                GET_IFACE2(pBroker, ISegmentLifting, pSegmentLifting);
                Float64 L = pSegmentLifting->GetLeftLiftingLoopLocation(segmentKey);
                rowData.liftingLoopLocation = L;
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
                rowData.trailingOverhang = trailingOverhang;
                rowData.leadingOverhang = leadingOverhang;

                if (pSegment->HandlingData.pHaulTruckLibraryEntry)
                {
                    rowData.rollStiffness = pSegment->HandlingData.pHaulTruckLibraryEntry->GetRollStiffness();
                }

                if (pSegment->HandlingData.pHaulTruckLibraryEntry)
                {
                    rowData.wheelSpacing = pSegment->HandlingData.pHaulTruckLibraryEntry->GetAxleWidth();
                }

            }

            m_schedule_data.emplace_back(rowData);

            GET_IFACE2(pBroker, ILibrary, pLib);
            GET_IFACE2(pBroker, ISpecification, pSpec);
            std::_tstring spec_name = pSpec->GetSpecification();
            const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(spec_name.c_str());
            const auto& creep_criteria = pSpecEntry->GetCreepCriteria();
            const auto& limits_criteria = pSpecEntry->GetLimitsCriteria();

            Float64 min_days = WBFL::Units::ConvertFromSysUnits(creep_criteria.CreepDuration2Min, WBFL::Units::Measure::Day);

            CString msg;
            VARIANT var;

            if (limits_criteria.bCheckSag)
            {
                if (IsNonstructuralDeck(deckType))
                {
                    if (C < 0)
                    {
                        VariantInit(&var);
                        var.vt = VT_I4;
                        var.lVal = RGB(255, 0, 0);
                        CString msg;
                        msg.Format(_T("%s WARNING: Final camber is downward. The girder may end up with a sag."), GIRDER_LABEL(girderKey));
                        m_warnings.emplace_back(msg, _T("Color"), var);
                    }
                }
                else
                {
                    std::_tstring camberType;
                    Float64 D = 0;

                    switch (limits_criteria.SagCamber)
                    {
                    case pgsTypes::SagCamber::LowerBoundCamber:
                        D = Dmin_LowerBound;
                        camberType = _T("lower bound");
                        break;
                    case pgsTypes::SagCamber::AverageCamber:
                        D = Dmin_Average;
                        camberType = _T("average");
                        break;
                    case pgsTypes::SagCamber::UpperBoundCamber:
                        D = Dmin_UpperBound;
                        camberType = _T("upper bound");
                        break;
                    }

                    if (D < C)
                    {
                        VariantInit(&var);
                        var.vt = VT_I4;
                        var.lVal = RGB(255, 0, 0);
                        msg.Format(_T("%s WARNING: Screed camber (C) is greater than the %s camber at time of deck casting, D. The girder may end up with a sag."),
                            GIRDER_LABEL(girderKey), camberType.c_str());
                        m_warnings.emplace_back(msg, _T("Color"), var);
                    }
                    else if (IsEqual(C, D, WBFL::Units::ConvertToSysUnits(0.25, WBFL::Units::Measure::Inch)))
                    {
                        VariantInit(&var);
                        var.vt = VT_I4;
                        var.lVal = RGB(255, 0, 0);
                        msg.Format(_T("%s WARNING: Screed camber (C) is nearly equal to the %s camber at time of deck casting, D. The girder may end up with a sag."),
                            GIRDER_LABEL(girderKey), camberType.c_str());
                        m_warnings.emplace_back(msg, _T("Color"), var);
                    }

                    if (Dmin_LowerBound < C && limits_criteria.SagCamber != pgsTypes::SagCamber::LowerBoundCamber)
                    {
                        Float64 Cfactor = pCamber->GetLowerBoundCamberVariabilityFactor();
                        VariantInit(&var);
                        var.vt = VT_I4;
                        var.lVal = RGB(0, 0, 0);
                        msg.Format(_T("%s Screed camber (C) is greater than the lower bound camber at time of deck casting (%0.0f%% of D%0.0f). The girder may end up with a sag if the deck is placed at day %0.0f and the actual camber is a lower bound value."),
                            GIRDER_LABEL(girderKey), Cfactor * 100, min_days, min_days);
                        m_warnings.emplace_back(msg, _T("Color"), var);
                    }
                }
            }


            if (!bCanReportPrestressInformation)
            {
                VARIANT var;
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = RGB(0, 0, 0);
                msg.Format(_T("-Girder %s Prestressing information could not be included in the girder schedule because strand input is not consistent with the standard WSDOT details."),
                    GIRDER_LABEL(girderKey));
                m_warnings.emplace_back(
                    msg,
                    _T("Color"), var);
            }

            if (debondResults == DEBOND_ERROR_SYMMETRIC)
            {
                VARIANT var;
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = RGB(0, 0, 0);
                msg.Format(_T("-Girder %s Debonding must be symmetric for this girder schedule."),
                    GIRDER_LABEL(girderKey));
                m_warnings.emplace_back(
                    msg,
                    _T("Color"), var);
            }

            if (rowData.reinfDetailsResult == STIRRUP_ERROR_ZONES)
            {
                VARIANT var;
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = RGB(0, 0, 0);
                msg.Format(_T("-Girder %s Reinforcement details could not be listed because the number of transverse reinforcement zones is not consistent with the girder schedule."),
                    GIRDER_LABEL(girderKey));
                m_warnings.emplace_back(
                    msg,
                    _T("Color"), var);
            }
            else if (rowData.reinfDetailsResult == STIRRUP_ERROR_SYMMETRIC)
            {
                VARIANT var;
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = RGB(0, 0, 0);
                msg.Format(_T("-Girder %s Reinforcement details could not be listed because the girder schedule is for symmetric transverse reinforcement layouts."),
                    GIRDER_LABEL(girderKey));
                m_warnings.emplace_back(
                    msg,
                    _T("Color"), var);
            }
            else if (rowData.reinfDetailsResult == STIRRUP_ERROR_STARTZONE)
            {
                VARIANT var;
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = RGB(0, 0, 0);
                msg.Format(_T("-Girder %s Reinforcement details could not be listed because first zone of transverse reinforcement is not consistent with standard WSDOT details."),
                    GIRDER_LABEL(girderKey));
                m_warnings.emplace_back(
                    msg,
                    _T("Color"), var);
            }
            else if (rowData.reinfDetailsResult == STIRRUP_ERROR_LASTZONE)
            {
                VARIANT var;
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = RGB(0, 0, 0);
                if (familyCLSID == CLSID_SlabBeamFamily)
                {
                    msg.Format(_T("-Girder %s Reinforcement details could not be listed because spacing in the last zone is not equal to the smaller of H-3\" or 1'-6\" per the standard WSDOT details."),
                        GIRDER_LABEL(girderKey));
                    m_warnings.emplace_back(
                        msg,
                        _T("Color"), var);
                }
                else
                {
                    msg.Format(_T("-Girder %s Reinforcement details could not be listed because spacing in the last zone is not 1'-6\" per the standard WSDOT details."),
                        GIRDER_LABEL(girderKey));
                    m_warnings.emplace_back(
                        msg,
                        _T("Color"), var);
                }
            }
            else if (rowData.reinfDetailsResult == STIRRUP_ERROR_BARSIZE)
            {
                VARIANT var;
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = RGB(0, 0, 0);
                msg.Format(_T("-Girder %s Reinforcement details could not be listed because the bar size is not #5 in one or more zones."),
                    GIRDER_LABEL(girderKey));
                m_warnings.emplace_back(
                    msg,
                    _T("Color"), var);
            }
            else if (rowData.reinfDetailsResult == STIRRUP_ERROR_V6)
            {
                VARIANT var;
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = RGB(0, 0, 0);
                msg.Format(_T("-Girder %s Reinforcement details could not be listed because two different values are needed for V6."),
                    GIRDER_LABEL(girderKey));
                m_warnings.emplace_back(
                    msg,
                    _T("Color"), var);
            }
            else if (rowData.reinfDetailsResult < 0)
            {
                VARIANT var;
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = RGB(0, 0, 0);
                msg.Format(_T("-Girder %s Reinforcement details could not be listed."),
                    GIRDER_LABEL(girderKey));
                m_warnings.emplace_back(
                    msg,
                    _T("Color"), var);
            }

            if (pSegment->HandlingData.pHaulTruckLibraryEntry == nullptr)
            {
                VARIANT var;
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = RGB(0, 0, 0);
                msg.Format(_T("-Girder %s Shipping analysis not performed."),
                    GIRDER_LABEL(girderKey));
                m_warnings.emplace_back(
                    msg,
                    _T("Color"), var);
            }

            SegmentIndexType segIdx = 0;

            SegmentIndexType nSegments = pGirder->GetSegmentCount();

            for (segIdx; segIdx < nSegments; segIdx++)
            {
                const auto& segmentKey = CSegmentKey(girderKey, segIdx);

                Float64 tft = pIGirder->GetTopFlangeThickening(segmentKey);

                FlangeIndexType nFlanges = pIGirder->GetTopFlangeCount(girderKey);

                if (pIGirder->CanTopFlangeBeLongitudinallyThickened(segmentKey) && !IsZero(tft))
                {

                    // The table currently in the WSDOT girder standard drawing 5.6-A6-10 lists top flange thickness at 10th points
                    // with the first and last point being at the CL Bearing. This implies an erected segment. However, this is girder
                    // fabrication data so it makes more sence to list based on 10th points of the actual segment. For this reason,
                    // the reference_type variable is introduced. The reporting can be easily changed by changing this variable.
                    PoiAttributeType reference_type = POI_ERECTED_SEGMENT; // POI_RELEASED_SEGMENT;
                    PoiList tfPoi;
                    GET_IFACE2(pBroker, IPointOfInterest, pPoi);
                    pPoi->GetPointsOfInterest(segmentKey, POI_TENTH_POINTS | reference_type, &tfPoi);
                    IndexType cdx = 1;

                    tf_rows++;

                    if (tf_rows == 1)
                    {

                        ws2 = worksheets.GetItem(COleVariant(2L));
                        allCells2 = ws2.GetCells();
                        allCells2.SetHorizontalAlignment(vCenter);
                        allCells2.SetVerticalAlignment(vCenter);


                        ///top flange thickening table
                        const std::vector<ScheduleHeaderInfo>& headerInfo =
                        {
                            {0, 1, 0, 3, 0, _T("SEGMENT")},
                            {1, 11, 0, 1, 0, _T("TOP FLANGE THICKNESS, TF")},
                            {1, 11, 1, 1, 0, _T("SEG. 10TH PT.")}
                        };

                        int nFlangeThicknessHeadings = 2;



                        IndexType idx = 0;

                        for (const auto& info : headerInfo)
                        {
                            CString strCell;
                            strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(info.colIdx), info.rowIdx + 1, GetColumnLabel(info.colIdx + info.colSpan - 1), info.rowIdx + info.rowSpan);
                            Range cell = ws2.GetRange(COleVariant(strCell), COleVariant(strCell));
                            cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
                            cell.SetValue2(COleVariant(info.strValue));
                            cell.BorderAround(
                                COleVariant((long)1),
                                (long)3,
                                (long)-4105,
                                COleVariant((long)0)
                            );


                            COleDispatchDriver font(cell.GetFont(), FALSE);

                            if (idx == 1)
                            {
                                font.SetProperty(0x60, VT_BOOL, TRUE);
                                font.SetProperty(0x68, VT_R8, 32.0);
                            }

                            idx++;

                        }

                        for (const pgsPointOfInterest& poi : tfPoi)
                        {
                            auto tenth_point = poi.IsTenthPoint(reference_type);
                            CHECK(tenth_point != 0); // expecting only 10th Points
                            if (tenth_point == 1 || tenth_point == 11)
                                SetColumnData(&ws2, cdx++, 2, _T("\u2104 Brg."));
                            else
                                SetColumnData(&ws2, cdx++, 2, (tenth_point - 1) / 10.);
                        }

                    }

                    SetColumnData(&ws2, 0, 2 + tf_rows, SEGMENT_LABEL(segmentKey));

                    INIT_UV_PROTOTYPE(rptLengthUnitValue, thickness, pDisplayUnits->GetComponentDimUnit(), true);

                    cdx = 1;
                    for (const pgsPointOfInterest& poi : tfPoi)
                    {

                        for (FlangeIndexType i = 0; i < nFlanges; i++)
                        {
                            thickness.SetValue(pIGirder->GetTopFlangeThickness(poi, i));
                            const auto& val = thickness.GetValue(true);
                            strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                            if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                                strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                            SetColumnData(&ws2, cdx++, 2 + tf_rows, strValue);
                        }

                    }

                }
            }

        } // gdrIdx

        NormalizeCamber(pBroker);
        
        //write to Excel
        for (gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
        {
            m_current_row_data.clear();

            const auto& rowData = m_schedule_data[gdrIdx];

            ColumnIndexType col = 0;

            ++col;

            CString strValue;

            //Set Girder Series or Height and Width
            if (!bSlab)
            {
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, rowData.girderSeries);

                if (bWFDG)
                {
                    gdim.SetValue(rowData.topWidth);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, strValue);
                }
            }
            else
            {
                gdim.SetValue(rowData.Hg);
                const auto& val = gdim.GetValue(true);
                strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, strValue);

                gdim.SetValue(rowData.topWidth);
                const auto& val1 = gdim.GetValue(true);
                strValue.Format(_T("%0.0f %s"), val1, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val1, 0.125)).c_str();
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, strValue);
            }

            //Set Plan Length
            const auto& rptPlanLength = gdim.SetValue(rowData.planLength);
            const auto& planLength = gdim.GetValue(true);
            if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
            {
                strValue = FormatFeetInchesFromDecimalInches(RoundOff(planLength, 0.125)).c_str();
            }
            else
            {
                strValue.Format(_T("%0.1f %s"), planLength, glength.GetUnitTag().c_str());
            }

            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

            //int. diaphragm type or voids
            if (!bSlab)
            {
                if (!bWFDG && !bUbeam)
                {
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, _T("-"));
                }
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, _T("-"));
            }
            else
            {
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, rowData.nVoids);
                gdim.SetValue(rowData.ExtVoidDiameter);
                const auto& extVoidDiameter = gdim.GetValue(true);
                gdim.SetValue(rowData.IntVoidDiameter);
                const auto& intVoidDiameter = gdim.GetValue(true);
                if (intVoidDiameter != 0)
                {
                    strValue.Format(_T("Ext: %0.0f %s Int: %0.0f %s"), extVoidDiameter, gdim.GetUnitTag().c_str(),
                        intVoidDiameter, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    {
                        CString strExt;
                        strExt = FormatFeetInchesFromDecimalInches(RoundOff(extVoidDiameter, 0.125)).c_str();
                        CString strInt;
                        strInt = FormatFeetInchesFromDecimalInches(RoundOff(intVoidDiameter, 0.125)).c_str();
                        strValue.Format(_T("Ext: %s Int: %s"), strExt, strInt);
                    }
                }
                else
                {
                    strValue.Format(_T("%0.0f %s"), extVoidDiameter, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(extVoidDiameter, 0.125)).c_str();
                }



                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, strValue);
            }

            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));

            angle.SetValue(rowData.t1);
            const auto& ft1 = angle.GetValue(true);
            strValue.Format(_T("%0.0f"), ft1);
            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
            angle.SetValue(rowData.t2);
            const auto& ft2 = angle.GetValue(true);
            strValue.Format(_T("%0.0f"), ft2);
            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

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
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, _T("-"));
                }
                else
                {
                    gdim.SetValue(rowData.P1);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, strValue);
                }

                pBridge->IsContinuousAtPier(nextPierIdx, &bContinuousLeft, &bContinuousRight);
                pBridge->IsIntegralAtPier(nextPierIdx, &bIntegralLeft, &bIntegralRight);

                if (bContinuousRight || bIntegralRight)
                {
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, _T("-"));
                }
                else
                {
                    gdim.SetValue(rowData.P2);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, strValue);
                }
            }

            stress.SetValue(rowData.fc);
            const auto& fc = stress.GetValue(true);
            strValue.Format(_T("%0.1f"), fc);
            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

            stress.SetValue(rowData.fci);
            const auto& fci = stress.GetValue(true);
            strValue.Format(_T("%0.1f"), fci);
            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);


            //set number of strands

            if (rowData.bCanReportPrestressInformation)
            {

                if (!bSlab)
                {

                    if (rowData.nDebonded != 0)
                    {
                        if (std::find(vDebond.begin(), vDebond.end(), rowData.nDebonded) == vDebond.end())
                        {
                            vDebond.emplace_back(rowData.nDebonded);
                        }

                        auto it = std::find(vDebond.begin(), vDebond.end(), rowData.nDebonded);
                        IndexType idx = std::distance(vDebond.begin(), it);

                        CString strAsterisks;
                        for (int i = 0; i < idx + 1; ++i)
                        {
                            strAsterisks += _T('*');
                        }
                        strValue.Format(_T("%d%s"), rowData.Ns, strAsterisks);
                        SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, strValue);
                    }
                    else
                    {
                        SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, rowData.Ns);
                    }

                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, rowData.Nh);

                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, rowData.Nt);

                }
                else
                {

                    RowIndexType nRows = 3;

                    for (RowIndexType rowIdx = 0; rowIdx < nRows; rowIdx++)
                    {
                        CString strExt;

                        SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, rowData.nStrandsInRows[rowIdx]);

                        StrandIndexType nExtended = 0;
                        StrandIndexType nDebonded = 0;

                        if (rowIdx <= 1)
                        {
                            std::map<Float64, StrandIndexType> mCountPerDebondLength;
                            std::map<Float64, ConfigStrandFillVector> mStrandIdsPerDebondLength;

                            ConfigStrandFillVector vDebondStrandsInRow;

                            if (rowData.Ns > 0)
                            {
                                for (const auto& strandIdx : rowData.vStrandsInRow[rowIdx])
                                {
                                    bool bExtended = std::find(rowData.vExtStrandsInRow[rowIdx].begin(),
                                        rowData.vExtStrandsInRow[rowIdx].end(), strandIdx) == rowData.vExtStrandsInRow[rowIdx].end();

                                    if (bExtended)
                                    {
                                        nExtended++;
                                        CString val;
                                        val.Format(_T("%d, "), strandIdx + 1);
                                        strExt.Append(val);
                                    }

                                    bool bDebonded = std::find(rowData.vDebStrandsInRow[rowIdx].begin(),
                                        rowData.vExtStrandsInRow[rowIdx].end(), strandIdx) == rowData.vExtStrandsInRow[rowIdx].end();

                                    if (bDebonded)
                                    {
                                        nDebonded++;
                                    }

                                }

                                if (nExtended > 0)
                                {
                                    int pos = strExt.ReverseFind(_T(','));
                                    if (pos != -1)
                                    {
                                        strExt.Delete(pos, 1);
                                        strExt.TrimRight();
                                    }

                                    if (std::find(vStrandPatterns.begin(), vStrandPatterns.end(), strExt) == vStrandPatterns.end())
                                    {
                                        vStrandPatterns.emplace_back(strExt);
                                    }

                                    auto it = std::find(vStrandPatterns.begin(), vStrandPatterns.end(), strExt);
                                    IndexType idx = std::distance(vStrandPatterns.begin(), it);

                                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, GetColumnLabel(idx));
                                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, _T("-"));

                                }
                                else
                                {
                                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, _T("-"));
                                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, _T("-"));
                                }

                                strValue.Empty();

                                if (nDebonded > 0 && mStrandIdsPerDebondLength.size() > 0)
                                {
                                    auto it = std::find(m_debond_schedule.begin(), m_debond_schedule.end(), mStrandIdsPerDebondLength);
                                    IndexType idx = std::distance(m_debond_schedule.begin(), it);

                                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, GetColumnLabel(idx));
                                }
                                else
                                {
                                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, _T("-"));
                                }

                            }

                        }
                        else
                        {
                            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, rowData.Nt);
                        }

                    }

                }

            }
            else
            {

                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));

                if (!bSlab)
                {
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                }

                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
            }

            if (CLSID_SlabBeamFamily != familyCLSID)
            {
                if (0 < rowData.Ns)
                {
                    gdim.SetValue(rowData.E);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, _T("N/A"));
                }

                if (0 < rowData.Nh)
                {
                    gdim.SetValue(rowData.Fcl);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, _T("N/A"));
                }

                if (0 < rowData.Nh)
                {
                    gdim.SetValue(rowData.Fo);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, _T("N/A"));
                }


                //strand extensions
                StrandIndexType nExtendedL = 0;
                CString strExt1;
                for (StrandIndexType strandIdx : rowData.vExtendedL)
                {
                    nExtendedL++;
                    CString val;
                    val.Format(_T("%d, "), strandIdx + 1);
                    strExt1.Append(val);
                }

                int pos = strExt1.ReverseFind(_T(','));
                if (pos != -1)
                {
                    strExt1.Delete(pos, 1);
                    strExt1.TrimRight();
                }

                if (nExtendedL == 0)
                {
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, _T("-"));

                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, _T("-"));

                }
                else
                {
                    if (std::find(vStrandPatterns.begin(), vStrandPatterns.end(), strExt1) == vStrandPatterns.end())
                    {
                        vStrandPatterns.emplace_back(strExt1);
                    }

                    auto it = std::find(vStrandPatterns.begin(), vStrandPatterns.end(), strExt1);
                    IndexType idx = std::distance(vStrandPatterns.begin(), it);

                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, GetColumnLabel(idx));

                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, _T("-"));
                }

                StrandIndexType nExtendedR = 0;
                CString strExt2;
                for (StrandIndexType strandIdx : rowData.vExtendedR)
                {
                    nExtendedR++;
                    CString val;
                    val.Format(_T("%d, "), strandIdx + 1);
                    strExt2.Append(val);
                }

                pos = strExt2.ReverseFind(_T(','));
                if (pos != -1)
                {
                    strExt2.Delete(pos, 1);
                    strExt2.TrimRight();
                }

                if (nExtendedR == 0)
                {
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, _T("-"));

                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, _T("-"));

                }
                else
                {
                    if (std::find(vStrandPatterns.begin(), vStrandPatterns.end(), strExt2) == vStrandPatterns.end())
                    {
                        vStrandPatterns.emplace_back(strExt2);
                    }

                    auto it = std::find(vStrandPatterns.begin(), vStrandPatterns.end(), strExt2);
                    IndexType idx = std::distance(vStrandPatterns.begin(), it);

                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, GetColumnLabel(idx));

                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, _T("-"));

                }

            }



            if (!IsNonstructuralDeck(deckType))
            {
                if (pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge)
                {
                    gdim.SetValue(rowData.A);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                }
                else
                {
                    CString strVal1, strVal2;
                    gdim.SetValue(rowData.Aend1);
                    const auto& val = gdim.GetValue(true);
                    strVal1.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    gdim.SetValue(rowData.Aend2);
                    const auto& val1 = gdim.GetValue(true);
                    strVal2.Format(_T("%0.0f %s"), val1, gdim.GetUnitTag().c_str());

                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    {
                        strVal1 = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                        strVal2 = FormatFeetInchesFromDecimalInches(RoundOff(val1, 0.125)).c_str();
                    }

                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strVal1);
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strVal2);
                }

                gdim.SetValue(rowData.C);
                const auto& val = gdim.GetValue(true);
                strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
            }

            gdim.SetValue(rowData.DminLowerBound);
            const auto& val = gdim.GetValue(true);
            strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
            if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

            gdim.SetValue(rowData.DmaxUpperBound);
            const auto& val1 = gdim.GetValue(true);
            strValue.Format(_T("%0.0f %s"), val1, gdim.GetUnitTag().c_str());
            if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                strValue = FormatFeetInchesFromDecimalInches(RoundOff(val1, 0.125)).c_str();
            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

            // Stirrups
            if (rowData.reinfDetailsResult < 0)
            {
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                if (bSlab)
                {
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                }
            }
            else
            {
                if (bUbeam)
                {
                    gdim.SetValue(rowData.z1Length);
                    const auto& z1Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z1Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z1Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

                    gdim.SetValue(rowData.z1Spacing);
                    const auto& z1Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z1Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z1Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

                    gdim.SetValue(rowData.z2Length);
                    const auto& z2Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z2Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z2Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

                    gdim.SetValue(rowData.z2Spacing);
                    const auto& z2Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z2Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z2Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

                    gdim.SetValue(rowData.z3Length);
                    const auto& z3Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z3Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z3Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

                    gdim.SetValue(rowData.z3Spacing);
                    const auto& z3Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z3Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z3Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                }
                else
                {
                    if (bSlab)
                    {
                        SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, WBFL::LRFD::RebarPool::GetBarSize(rowData.z1Size).c_str());
                    }
                    gdim.SetValue(rowData.z1Spacing);
                    const auto& z1Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z1Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z1Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

                    gdim.SetValue(rowData.z1Length);
                    const auto& z1Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z1Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z1Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    if (bSlab)
                    {
                        SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, WBFL::LRFD::RebarPool::GetBarSize(rowData.z2Size).c_str());
                    }
                    gdim.SetValue(rowData.z2Spacing);
                    const auto& z2Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z2Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z2Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

                    gdim.SetValue(rowData.z2Length);
                    const auto& z2Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z2Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z2Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    if (bSlab)
                    {
                        SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, WBFL::LRFD::RebarPool::GetBarSize(rowData.z3Size).c_str());
                    }
                    gdim.SetValue(rowData.z3Spacing);
                    const auto& z3Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z3Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z3Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    gdim.SetValue(rowData.z3Length);
                    const auto& z3Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z3Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z3Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                }
            }

            // Stirrup Height

            if (familyCLSID == CLSID_WFBeamFamily || bUbeam)
            {
                // H1 (Hg + "A" + 3")
                if (pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge)
                {
                    gdim.SetValue(rowData.H1);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, strValue);
                }
                else
                {
                    gdim.SetValue(rowData.H1end1);
                    const auto& val = gdim.GetValue(true);
                    gdim.SetValue(rowData.H1end2);
                    const auto& val1 = gdim.GetValue(true);
                    CString strVal1, strVal2;

                    strVal1.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    strVal2.Format(_T("%0.0f %s"), val1, gdim.GetUnitTag().c_str());

                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    {
                        strVal1 = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                        strVal2 = FormatFeetInchesFromDecimalInches(RoundOff(val1, 0.125)).c_str();
                    }
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strVal1);
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strVal2);
                }
            }

            //longitudinal rebar
            if (familyCLSID == CLSID_SlabBeamFamily)
            {

                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, _T("-"));
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, _T("-"));

                for (int i = 0; i < rowData.vG1LongBarSize.size(); i++)
                {
                    SetColumnData(&ws, --col, nPrevGirders * grpIdx + gdrIdx + 5, WBFL::LRFD::RebarPool::GetBarSize(rowData.vG1LongBarSize[i]).c_str());
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, rowData.vG1NumLongBars[i]);
                }

                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, _T("-"));
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, _T("-"));

                for (int i = 0 ; i < rowData.vG2LongBarSize.size() ; i++)
                {
                    SetColumnData(&ws, --col, nPrevGirders * grpIdx + gdrIdx + 5, WBFL::LRFD::RebarPool::GetBarSize(rowData.vG2LongBarSize[i]).c_str());
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, rowData.vG2NumLongBars[i]);
                }

            }

            if (rowData.midspanDeflection)
            { 
                gdim.SetValue(rowData.midspanDeflection);
                const auto& val = gdim.GetValue(true);
                strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
            }

            if (rowData.liftingLoopLocation)
            {
                gdim.SetValue(rowData.liftingLoopLocation);
                const auto& val = gdim.GetValue(true);
                strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
            }

            if (rowData.leadingOverhang && rowData.trailingOverhang)
            {
                gdim.SetValue(rowData.leadingOverhang);
                const auto& val = gdim.GetValue(true);
                strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                gdim.SetValue(rowData.trailingOverhang);
                const auto& val1 = gdim.GetValue(true);
                strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val1, 0.125)).c_str();
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

                if (rowData.rollStiffness)
                {
                    spring.SetValue(rowData.rollStiffness);
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

                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                }

                if (rowData.wheelSpacing)
                {
                    gdim.SetValue(rowData.wheelSpacing);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                }
            }


            //set girder range
            CString strGirder;

            bool bSame = true;

            if (m_previous_row_data.size() != m_current_row_data.size())
                bSame = false;
            else
            {
                for (int i = 1; i < m_previous_row_data.size(); ++i)
                {
                    if (m_previous_row_data[i].CompareNoCase(m_current_row_data[i]) != 0)
                        bSame = false;
                }
            }

            m_previous_row_data = m_current_row_data;

            if (bSame)
            {
                strGirder.Format(_T("%s-%s"), GetColumnLabel(m_last_same_gdrID), GetColumnLabel(gdrIdx));
                SetColumnData(&ws, 1, nPrevGirders * grpIdx + m_last_same_gdrID + (bSlab ? 5 : 4), strGirder);

                CString strCell;

                strCell.Format(_T("C%d:%s%d"), nPrevGirders * grpIdx + m_last_same_gdrID + (bSlab ? 6 : 5),
                    GetColumnLabel(m_current_row_data.size()), nPrevGirders * grpIdx + gdrIdx - 1 + (bSlab ? 6 : 5));
                Range cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
                cell.ClearContents();

                //merge and format cells
                for (IndexType i = 1; i <= m_previous_row_data.size() + 1; i++)
                {
                    CString strCol = GetColumnLabel(i);
                    CString strCell;
                    strCell.Format(_T("%s%d:%s%d"), strCol, nPrevGirders * grpIdx + m_last_same_gdrID + (bSlab ? 6 : 5),
                        strCol, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 6 : 5));
                    Range cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
                    cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
                    cell.BorderAround(
                        COleVariant((long)1),
                        (long)3,
                        (long)-4105,
                        COleVariant((long)0)
                    );
                    strCell.Format(_T("%s%d:%s%d"), strCol, nPrevGirders * grpIdx + m_last_same_gdrID + (bSlab ? 6 : 5),
                        strCol, nPrevGirders * grpIdx + gdrIdx - 1 + (bSlab ? 6 : 5));
                    cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
                    cell.SetRowHeight(COleVariant((long)0));
                }
            }
            else
            {
                m_last_same_gdrID = gdrIdx;
                SetColumnData(&ws, 1, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), GetColumnLabel(gdrIdx));
            }
        }

        //set span
        SpanIndexType spanIdx = girderKey.groupIndex;
        CString strSpan;
        strSpan.Format(_T("%s"), LABEL_SPAN(spanIdx));
        SetColumnData(&ws, 0, (bSlab ? 5 : 4) + grpIdx * (nPrevGirders + 1), strSpan);

        //merge and format span cells
        CString strCell;
        strCell.Format(_T("A%d:A%d"), (grpIdx* nPrevGirders) + (bSlab ? 6 : 5), (grpIdx* nPrevGirders) + ((bSlab ? 5 : 4) + nGirders));
        Range cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
        cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
        cell.BorderAround(
            COleVariant((long)1),
            (long)3,
            (long)-4105,
            COleVariant((long)0)
        );

    }


    RowIndexType tableOffset = (bSlab ? 6 : 5);

    CString strCell;

    //set bottom of header to normal line weight
    strCell.Format(_T("A1:%s%d"), GetColumnLabel(m_previous_row_data.size() + 1), tableOffset - 1);
    Range cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
    Borders borders = cell.GetBorders();
    Border bottomBorder = borders.GetItem(9);
    bottomBorder.SetWeight(COleVariant((long)3));

    for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
    {
        tableOffset += pBridge->GetGirderCount(grpIdx);
    }


    //thicken table borders
    strCell.Format(_T("A1:%s%d"), GetColumnLabel(m_previous_row_data.size() + 1), tableOffset - 1);
    cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
    cell.BorderAround(
        COleVariant((long)1),
        (long)4,
        (long)-4105,
        COleVariant((long)0)
    );

    //thicken flange thickening table
    if (tf_rows > 0)
    {
        strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(0), 1, GetColumnLabel(11), 3 + tf_rows);
        cell = ws2.GetRange(COleVariant(strCell), COleVariant(strCell));
        cell.BorderAround(
            COleVariant((long)1),
            (long)4,
            (long)-4105,
            COleVariant((long)0)
        );
    }

    VARIANT var;
    VariantInit(&var);
    var.vt = VT_I4;
    var.lVal = RGB(0, 0, 0);
    m_warnings.emplace_back(_T("-End Types to be determined by the Designer."), _T("Color"), var);
    if (!bSlab)
    {
        if (bIbeam)
        {
            m_warnings.emplace_back(_T("-Intermediate Diaphragm Type to be determined by the Designer."), _T("Color"), var);
        }
        m_warnings.emplace_back(_T("-Ld to be determined by the Designer."), _T("Color"), var);
    }

    if (familyCLSID == CLSID_WFBeamFamily || familyCLSID == CLSID_UBeamFamily)
    {
        VARIANT var;
        VariantInit(&var);
        var.vt = VT_I4;
        var.lVal = RGB(0, 0, 0);
        m_warnings.emplace_back(
            _T("-H1 is computed as the height of the girder H + 3\" + \"A\" Dimension. Designers shall check H1 for the effect of vertical curve and increase as necessary."),
            _T("Color"), var);
    }


    //build strand extension table

    CString strValue;

    if (vStrandPatterns.size() != 0)
    {
        std::vector<ScheduleHeaderInfo> headerInfo;

        if (bUbeam)
        {
            headerInfo =
            {
                { 0, 6, tableOffset, 1, 0, _T("EXTENDED STRAND SCHEDULE") },
                { 0, 3, tableOffset + 1, 1, 0, _T("STRAND PATTERN") },
                { 3, 3, tableOffset + 1, 1, 0, _T("STRANDS TO EXTEND") }
            };
        }
        else
        {
            headerInfo =
            {
                { 0, 5, tableOffset, 1, 0, _T("EXTENDED STRAND SCHEDULE") },
                { 0, 3, tableOffset + 1, 1, 0, _T("STRAND PATTERN") },
                { 3, 2, tableOffset + 1, 1, 0, _T("STRANDS TO EXTEND") }
            };
        }

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


        for (IndexType idx = 0; idx < vStrandPatterns.size(); idx++)
        {

            strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(0), tableOffset + 3 + idx, 
                GetColumnLabel(2), tableOffset + 3 + idx);
            Range cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
            cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
            cell.SetValue2(COleVariant(GetColumnLabel(idx)));
            cell.BorderAround(
                COleVariant((long)1),
                (long)3,
                (long)-4105,
                COleVariant((long)0)
            );

            strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(3), tableOffset + 3 + idx, 
                GetColumnLabel((bUbeam? 5 : 4)), tableOffset + 3 + idx);
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

        //make outline bold
        strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(0), tableOffset + 1, GetColumnLabel((bUbeam ? 5 : 4)), 
            tableOffset + 2 + vStrandPatterns.size());
        cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
        cell.BorderAround(
            COleVariant((long)1),
            (long)4,
            (long)-4105,
            COleVariant((long)0)
        );
    }

    int nDebondHeadings = 0;

    if (vDebond.size() != 0)
    {

        nDebondHeadings = 2;

        strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(0), tableOffset + 3 + (vStrandPatterns.size() != 0 ? 1 : 0) + vStrandPatterns.size(),
            GetColumnLabel(4), tableOffset + 3 + (vStrandPatterns.size() != 0 ? 1 : 0) + vStrandPatterns.size());
        Range cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
        cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
        COleVariant vLeft((long)-4131, VT_I4);
        cell.SetHorizontalAlignment(vLeft);
        cell.SetValue2(COleVariant(_T("SCHEDULE LEGEND:")));
        COleDispatchDriver font(cell.GetFont(), FALSE);
        font.SetProperty(0x60, VT_BOOL, TRUE);
        font.SetProperty(0x68, VT_R8, 20.0);
        font.SetProperty(0x6A, VT_I4, (long)2);

        for (IndexType idx = 0; idx < vDebond.size(); idx++)
        {
            strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(0), tableOffset + 3 + nDebondHeadings + vStrandPatterns.size() + idx,
                GetColumnLabel(4), tableOffset + 3 + nDebondHeadings + vStrandPatterns.size() + idx);
            cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));

            CString strAsterisks;
            for (int i = 0; i < idx + 1; ++i)
            {
                strAsterisks += _T('*');
            }
            strValue.Format(_T("%s %d STRAND%s"), strAsterisks, vDebond[idx], (vDebond[idx] == 1? _T(" DEBONDED") :_T("S DEBONDED")));
            cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
            COleVariant vLeft((long)-4131, VT_I4);
            cell.SetHorizontalAlignment(vLeft);
            cell.SetValue2(COleVariant(strValue));
        }
    }

    if (m_debond_schedule.size() > 0)
    {
        std::vector<ScheduleHeaderInfo> headerInfo;

        headerInfo =
        {
            { 6, 9, tableOffset, 1, 0, _T("DEBONDED STRAND SCHEDULE") },
            { 6, 2, tableOffset + 1, 1, 0, _T("STRAND PATTERN") },
            { 8, 4, tableOffset + 1, 1, 0, _T("STRANDS TO DEBOND") },
            { 12, 3, tableOffset + 1, 1, 0, _T("DEBONDED LENGTH") }
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

        IndexType nCombos = 0;
        int prev_offset = 0;

        for (IndexType idx = 0; idx < m_debond_schedule.size(); idx++)
        {

            if (m_debond_schedule[idx].size() > 0)
            {

                int curr_offset = m_debond_schedule[idx].size() - 1;

                strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(6), tableOffset + 3 + prev_offset,
                    GetColumnLabel(7), tableOffset + 3 + prev_offset + curr_offset);
                Range cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
                cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
                cell.SetValue2(COleVariant(GetColumnLabel(idx)));
                cell.BorderAround(
                    COleVariant((long)1),
                    (long)3,
                    (long)-4105,
                    COleVariant((long)0)
                );

                prev_offset += m_debond_schedule[idx].size();

                for (const auto& map : m_debond_schedule[idx])
                {
                    strValue.Empty();

                    for (StrandIndexType strandIdx : map.second)
                    {
                        CString val;
                        val.Format(_T("%d, "), strandIdx + 1);
                        strValue.Append(val);
                    }

                    int pos = strValue.ReverseFind(_T(','));
                    if (pos != -1)
                    {
                        strValue.Delete(pos, 1);
                        strValue.TrimRight();
                    }

                    strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(8), tableOffset + 3 + nCombos,
                        GetColumnLabel(11), tableOffset + 3 + nCombos);
                    cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
                    cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));

                    cell.SetValue2(COleVariant(strValue));
                    cell.BorderAround(
                        COleVariant((long)1),
                        (long)3,
                        (long)-4105,
                        COleVariant((long)0)
                    );

                    strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(12), tableOffset + 3 + nCombos,
                        GetColumnLabel(14), tableOffset + 3 + nCombos);
                    cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
                    cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));


                    gdim.SetValue(map.first);
                    const auto& val = gdim.GetValue(true);

                    strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    CString debondLength;
                    debondLength = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue.Format(_T("%s"), debondLength);

                    cell.SetValue2(COleVariant(strValue));
                    cell.BorderAround(
                        COleVariant((long)1),
                        (long)3,
                        (long)-4105,
                        COleVariant((long)0)
                    );

                    nCombos++;
                }
            }

        }

        //make outline bold
        strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(6), tableOffset + 1, GetColumnLabel(14),
            tableOffset + 2 + nCombos);
        cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
        cell.BorderAround(
            COleVariant((long)1),
            (long)4,
            (long)-4105,
            COleVariant((long)0)
        );
        
    }


    AddDesignerNudges();

    if (m_warnings.size() != 0 || m_optimizations.size() != 0)
    {

        // Prepare VARIANTs
        VARIANT vtMissing, vtAfter;
        VariantInit(&vtMissing);
        vtMissing.vt = VT_ERROR;
        vtMissing.scode = DISP_E_PARAMNOTFOUND; // Excel "missing" parameter

        // Get last sheet so we can add after it
        long count = worksheets.GetCount();
        VARIANT vtIndex;
        vtIndex.vt = VT_I4;
        vtIndex.lVal = count;

        auto lastSheet = worksheets.GetItem(vtIndex);

        // Set vtAfter = lastSheet
        vtAfter.vt = VT_DISPATCH;
        vtAfter.pdispVal = lastSheet;

        // Add new worksheet AFTER last sheet
        IDispatch* newSheetDisp = worksheets.Add(vtMissing, vtAfter, vtMissing, vtMissing);

        // cast to Worksheet wrapper class
        _Worksheet newSheet = newSheetDisp;

        newSheet.SetName(_T("DESIGNER NOTES"));

        if (m_warnings.size() != 0)
        {
            strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(0), 1, GetColumnLabel(0), 1);
            Range cell = newSheet.GetRange(COleVariant(strCell), COleVariant(strCell));
            cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
            COleVariant vLeft((long)-4131, VT_I4);
            cell.SetHorizontalAlignment(vLeft);
            cell.SetValue2(COleVariant(_T("NOTES TO DESIGNER:")));

            for (IndexType idx = 0; idx < m_warnings.size(); idx++)
            {
                strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(0), 2 + idx, GetColumnLabel(30), 2 + idx);
                Range cell = newSheet.GetRange(COleVariant(strCell), COleVariant(strCell));

                COleDispatchDriver font(cell.GetFont(), FALSE);

                // Resolve property name -> DISPID
                LPOLESTR name = (LPOLESTR)(LPCTSTR)m_warnings[idx].prop;
                DISPID dispid = 0;
                HRESULT hr = font.m_lpDispatch->GetIDsOfNames(
                    IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid);

                if (SUCCEEDED(hr)) {
                    font.SetProperty(dispid, VT_VARIANT, &m_warnings[idx].var);
                }

                cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
                COleVariant vLeft((long)-4131, VT_I4);
                cell.SetHorizontalAlignment(vLeft);
                cell.SetValue2(COleVariant(m_warnings[idx].msg));
            }

        }

        if (m_optimizations.size() != 0)
        {
            strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(0), m_warnings.size() + 3, GetColumnLabel(0), m_warnings.size() + 3);
            Range cell = newSheet.GetRange(COleVariant(strCell), COleVariant(strCell));
            cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
            COleVariant vLeft((long)-4131, VT_I4);
            cell.SetHorizontalAlignment(vLeft);
            cell.SetValue2(COleVariant(_T("POSSIBLE FABRICATION OPTIMIZATIONS:")));

            for (IndexType idx = 0; idx < m_optimizations.size(); idx++)
            {
                strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(0), m_warnings.size() + 4 + idx, GetColumnLabel(30), m_warnings.size() + 4 + idx);
                Range cell = newSheet.GetRange(COleVariant(strCell), COleVariant(strCell));

                COleDispatchDriver font(cell.GetFont(), FALSE);

                // Resolve property name -> DISPID
                LPOLESTR name = (LPOLESTR)(LPCTSTR)m_optimizations[idx].prop;
                DISPID dispid = 0;
                HRESULT hr = font.m_lpDispatch->GetIDsOfNames(
                    IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid);

                if (SUCCEEDED(hr)) {
                    font.SetProperty(dispid, VT_VARIANT, &m_optimizations[idx].var);
                }

                cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
                COleVariant vLeft((long)-4131, VT_I4);
                cell.SetHorizontalAlignment(vLeft);
                cell.SetValue2(COleVariant(m_optimizations[idx].msg));
            }
            
        }

        m_warnings.clear();
        m_optimizations.clear();
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

