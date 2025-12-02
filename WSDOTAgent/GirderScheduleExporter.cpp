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

    CString strDefaultLocation;
    if (-1 != str.Find(_T("RegFreeCOM")))
    {
        // application is on a development box
        if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
            if (pBridgeDesc->GetSlabOffsetType() != pgsTypes::sotBridge)
                strDefaultLocation = str.Left(2) + CString(_T("\\ARP\\PGSuper\\WSDOTAgent\\Template US Units\\Varied Haunch\\"));
            else
                strDefaultLocation = str.Left(2) + CString(_T("\\ARP\\PGSuper\\WSDOTAgent\\Template US Units\\"));
        else
            if (pBridgeDesc->GetSlabOffsetType() != pgsTypes::sotBridge)
                strDefaultLocation = str.Left(2) + CString(_T("\\ARP\\PGSuper\\WSDOTAgent\\Template SI Units\\Varied Haunch\\"));
            else
                strDefaultLocation = str.Left(2) + CString(_T("\\ARP\\PGSuper\\WSDOTAgent\\Template SI Units\\"));

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
        AfxMessageBox(_T("An error occured while attempting to run Excel. Excel files cannot be created unless Microsoft Excel is installed. Maybe try a .csv file?"));
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
    std::vector<StrandIndexType> vDebond;

    GET_IFACE2(pBroker, IBridge, pBridge);
    GET_IFACE2(pBroker, IGirder, pIGirder);

    IndexType hits = 0;

    for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
    {
        m_previous_row_data.clear();
        
        GirderIndexType gdrIdx = m_last_same_gdrID = 0;
        const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
        GirderIndexType nGirders = pGroup->GetGirderCount();
        const CGirderGroupData* pPrevGroup = pBridgeDesc->GetGirderGroup((grpIdx == 0? 0: grpIdx - 1));
        GirderIndexType nPrevGirders = pPrevGroup->GetGirderCount();

        IndexType nMaxDebondCombos = 0;

        for (gdrIdx; gdrIdx < nGirders; gdrIdx++)
        {
            m_current_row_data.clear();

            ColumnIndexType col = 0;

            girderKey = CGirderKey(grpIdx, gdrIdx);

            const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
            
            ++col;

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
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, strSeries);

                if (bWFDG)
                {
                    Float64 W = pIGirder->GetTopWidth(poiMidSpan);
                    gdim.SetValue(W);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, strValue);
                }
            }
            else
            {
                Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poiMidSpan);
                gdim.SetValue(Hg);
                const auto& val = gdim.GetValue(true);
                strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, strValue);

                GET_IFACE2(pBroker, IGirder, pIGirder);
                Float64 W = pIGirder->GetTopWidth(poiMidSpan);
                gdim.SetValue(W);
                const auto& val1 = gdim.GetValue(true);
                strValue.Format(_T("%0.0f %s"), val1, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val1, 0.125)).c_str();
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, strValue);
            }

                //Set Plan Length
            if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
            {
                const auto& rptPlanLength = gdim.SetValue(pBridge->GetSegmentPlanLength(segmentKey));
                const auto& planLength = gdim.GetValue(true);
                strValue = FormatFeetInchesFromDecimalInches(RoundOff(planLength, 0.125)).c_str();
            }
            else
            {
                const auto& rptPlanLength = glength.SetValue(pBridge->GetSegmentPlanLength(segmentKey));
                const auto& planLength = glength.GetValue(true);
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
                const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(girderKey.girderIndex)->GetGirderLibraryEntry();
                Float64 nVoids = pGdrEntry->GetDimension(_T("Number_of_Voids"));
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, nVoids);
                Float64 ExtVoidDiameter = pGdrEntry->GetDimension(_T("D1"));
                gdim.SetValue(ExtVoidDiameter);
                const auto& extVoidDiameter = gdim.GetValue(true);
                Float64 IntVoidDiameter = pGdrEntry->GetDimension(_T("D2"));
                gdim.SetValue(IntVoidDiameter);
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
            

            CComPtr<IAngle> objAngle1, objAngle2;
            pBridge->GetSegmentSkewAngle(segmentKey, pgsTypes::metStart, &objAngle1);
            pBridge->GetSegmentSkewAngle(segmentKey, pgsTypes::metEnd, &objAngle2);
            Float64 t1, t2;
            objAngle1->get_Value(&t1);
            objAngle2->get_Value(&t2);

            angle.SetValue(t1);
            const auto& ft1 = angle.GetValue(true);
            strValue.Format(_T("%0.0f"), ft1);
            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
            angle.SetValue(t2);
            const auto& ft2 = angle.GetValue(true);
            strValue.Format(_T("%0.0f"), ft2);
            SetColumnData(&ws, ++col, nPrevGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

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
                    // start end distance is a plan view dimension that needs to
                    // be adjusted for the installed girder slope and the height of the girder
                    Float64 D = pBridge->GetSegmentStartEndDistance(segmentKey);
                    Float64 slope = pBridge->GetSegmentSlope(segmentKey);
                    Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poiMidSpan);
                    Float64 P1 = D * sqrt(1 + slope * slope) - slope * Hg;

                    gdim.SetValue(P1);
                    P1 = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), P1, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(P1, 0.125)).c_str();
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
                    // start end distance is a plan view dimension that needs to
                    // be adjusted for the installed girder slope and the height of the girder
                    Float64 D = pBridge->GetSegmentEndEndDistance(segmentKey);
                    Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poiMidSpan);
                    Float64 slope = pBridge->GetSegmentSlope(segmentKey);
                    Float64 P2 = D * sqrt(1 + slope * slope) + slope * Hg;

                    gdim.SetValue(P2);
                    P2 = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), P2, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(P2, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, strValue);
                }
            }

            GET_IFACE2(pBroker, IMaterials, pMaterial);

            
            stress.SetValue(pMaterial->GetSegmentDesignFc(segmentKey, finalIntervalIdx));
            const auto& fc = stress.GetValue(true);
            strValue.Format(_T("%0.1f"), fc);
            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

            stress.SetValue(pMaterial->GetSegmentDesignFc(segmentKey, releaseIntervalIdx));
            const auto& fci = stress.GetValue(true);
            strValue.Format(_T("%0.1f"), fci);
            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);


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

            std::vector<CDebondResults::DebondInformation> debondInfo;
            CDebondResults results;
            int debondResults = results.GetDebondDetails(pBroker, segmentKey, debondInfo);

            if (bCanReportPrestressInformation)
            {

                if (!bSlab)
                {
                    StrandIndexType nDebonded = pStrandGeometry->GetNumDebondedStrands(segmentKey, pgsTypes::Straight, pgsTypes::dbetEither);
                    if (nDebonded != 0)
                    {
                        if (std::find(vDebond.begin(), vDebond.end(), nDebonded) == vDebond.end())
                        {
                            vDebond.emplace_back(nDebonded);
                        }

                        auto it = std::find(vDebond.begin(), vDebond.end(), nDebonded);
                        IndexType idx = std::distance(vDebond.begin(), it);

                        CString strAsterisks;
                        for (int i = 0; i < idx + 1; ++i)
                        {
                            strAsterisks += _T('*');
                        }
                        strValue.Format(_T("%d%s"), Ns, strAsterisks);
                        SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, strValue);
                    }
                    else
                    {
                        SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, Ns);
                    }

                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, Nh);


                    StrandIndexType Nt = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Temporary);
                    CString strNt;
                    strNt.Format(_T("%d"), Nt);
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, Nt);
                    
                    
                }
                else
                {

                    RowIndexType nRows = 3;


                    for (RowIndexType rowIdx = 0; rowIdx < nRows; rowIdx++)
                    {
                        StrandIndexType nStrandsInRow = pStrandGeometry->GetNumStrandInRow(poiStart, rowIdx, pgsTypes::Straight);
                        SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, nStrandsInRow);

                        if (rowIdx <= 1)
                        {
                            StrandIndexType nExtended = 0;

                            std::vector<StrandIndexType>vStrandsInRow = pStrandGeometry->GetStrandsInRow(poiStart, rowIdx, pgsTypes::Straight);

                            if (Ns > 0)
                            {

                                std::map<Float64, StrandIndexType> mCountPerDebondLength;
                                
                                for (const auto& strandIdx : vStrandsInRow)
                                {
                                    bool bExtended = pStrandGeometry->IsExtendedStrand(poiStart, strandIdx, pgsTypes::Straight);
                                    if (bExtended)
                                    {
                                        nExtended++;
                                    }

                                    bool bDebonded = pStrandGeometry->IsStrandDebonded(poiStart, strandIdx, pgsTypes::Straight);
                                    if (bDebonded)
                                    {
                                        int groupCount = 0;
                                        std::vector<CDebondResults::DebondInformation>::iterator iter(debondInfo.begin());
                                        std::vector<CDebondResults::DebondInformation>::iterator end(debondInfo.end());

                                        for (; iter != end; iter++)
                                        {
                                            CDebondResults::DebondInformation& dbInfo = *iter;

                                            if (std::find(dbInfo.Strands.begin(), dbInfo.Strands.end(), (strandIdx + 1)) != dbInfo.Strands.end())
                                            {
                                                mCountPerDebondLength[dbInfo.Length]++;
                                            }

                                        }

                                    }

                                }

                                IndexType nDebondedCombos = mCountPerDebondLength.size();

                                nMaxDebondCombos = max(nMaxDebondCombos, nDebondedCombos);

                                strValue.Empty();

                                for (const auto& count : mCountPerDebondLength)
                                {
                                    gdim.SetValue(count.first);
                                    const auto& val = gdim.GetValue(true);

                                    CString strLocValue;

                                    strLocValue.Format(_T("%d @ %0.0f %s \n"), count.second, val, gdim.GetUnitTag().c_str());
                                    CString debondLength;
                                    debondLength = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                                        strLocValue.Format(_T("%d @ %s \n"), count.second, debondLength);

                                    strValue.Append(strLocValue);
                                }

                                strValue.TrimRight('\n');

                            }

                            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, nExtended);
                            if (strValue.IsEmpty())
                            {
                                SetColumnData(&ws, ++col, nPrevGirders* grpIdx + gdrIdx + 5, 0);
                            }
                            else
                            {
                                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, strValue);
                            }

                        }
                        else
                        {
                            StrandIndexType Nt = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Temporary);
                            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, Nt);
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

                Float64 ybg = pSectProp->GetY(releaseIntervalIdx, poiMidSpan, pgsTypes::BottomGirder);
                Float64 sse = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poiMidSpan, pgsTypes::Straight).Y();
                if (0 < Ns)
                {
                    gdim.SetValue(ybg - sse);
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

                Float64 hse = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poiMidSpan, pgsTypes::Harped).Y();
                if (0 < Nh)
                {
                    gdim.SetValue(ybg - hse);
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


                Float64 ytg = pSectProp->GetY(releaseIntervalIdx, poiStart, pgsTypes::TopGirder);
                Float64 hss = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poiStart, pgsTypes::Harped).Y();
                if (0 < Nh)
                {
                    gdim.SetValue(ytg + hss);
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
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 4, _T("-"));

                    SetColumnData(&ws, ++col, nPrevGirders* grpIdx + gdrIdx + 4, _T("-"));
                    
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
                    gdim.SetValue(pBridgeDesc->GetSlabOffset());
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                }
                else
                {
                    CString strVal1, strVal2;
                    
                    gdim.SetValue(pSegment->GetSlabOffset(pgsTypes::metStart));
                    const auto& val = gdim.GetValue(true);
                    strVal1.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    gdim.SetValue(pSegment->GetSlabOffset(pgsTypes::metEnd));
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

                C = pCamber->GetScreedCamber(poiMidSpan, pgsTypes::CreepTime::Max);
                gdim.SetValue(C);
                const auto& val = gdim.GetValue(true);
                strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
            }


            // get # of days for creep
            Float64 Dmax_UpperBound, Dmax_Average, Dmax_LowerBound;
            Float64 Dmin_UpperBound, Dmin_Average, Dmin_LowerBound;
            pCamber->GetDCamberForGirderScheduleEx(poiMidSpan, pgsTypes::CreepTime::Max, &Dmax_UpperBound, &Dmax_Average, &Dmax_LowerBound);
            pCamber->GetDCamberForGirderScheduleEx(poiMidSpan, pgsTypes::CreepTime::Min, &Dmin_UpperBound, &Dmin_Average, &Dmin_LowerBound);

            gdim.SetValue(Dmin_LowerBound);
            const auto& val = gdim.GetValue(true);
            strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
            if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

            gdim.SetValue(Dmax_UpperBound);
            const auto& val1 = gdim.GetValue(true);
            strValue.Format(_T("%0.0f %s"), val1, gdim.GetUnitTag().c_str());
            if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                strValue = FormatFeetInchesFromDecimalInches(RoundOff(val1, 0.125)).c_str();
            SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

            // Stirrups
            Float64 z1Spacing, z1Length, z2Spacing, z2Length, z3Spacing, z3Length;
            WBFL::Materials::Rebar::Size z1Size, z2Size, z3Size;
            CWSDOTReinforcement details;
            int reinfDetailsResult = details.GetWSDOTReinforcementDetails(pBroker, segmentKey, familyCLSID,
                &z1Size, &z1Spacing, &z1Length, &z2Size, &z2Spacing, &z2Length, &z3Size, &z3Spacing, &z3Length);
            if (reinfDetailsResult < 0)
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
                    gdim.SetValue(z1Length);
                    const auto& z1Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z1Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z1Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    gdim.SetValue(z1Spacing);
                    const auto& z1Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z1Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z1Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

                    gdim.SetValue(z2Length);
                    const auto& z2Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z2Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z2Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    gdim.SetValue(z2Spacing);
                    const auto& z2Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z2Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z2Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

                    gdim.SetValue(z3Length);
                    const auto& z3Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z3Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z3Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    gdim.SetValue(z3Spacing);
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
                        SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, WBFL::LRFD::RebarPool::GetBarSize(z1Size).c_str());
                    }
                    gdim.SetValue(z1Spacing);
                    const auto& z1Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z1Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z1Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    gdim.SetValue(z1Length);
                    const auto& z1Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z1Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z1Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    if (bSlab)
                    {
                        SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, WBFL::LRFD::RebarPool::GetBarSize(z2Size).c_str());
                    }
                    gdim.SetValue(z2Spacing);
                    const auto& z2Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z2Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z2Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    gdim.SetValue(z2Length);
                    const auto& z2Length = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z2Length, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z2Length, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    if (bSlab)
                    {
                        SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, WBFL::LRFD::RebarPool::GetBarSize(z3Size).c_str());
                    }
                    gdim.SetValue(z3Spacing);
                    const auto& z3Spacing = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), z3Spacing, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(z3Spacing, 0.25), 4).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                    gdim.SetValue(z3Length);
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
                    Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poiStart);
                    Float64 H1 = pBridgeDesc->GetSlabOffset() + Hg + WBFL::Units::ConvertToSysUnits(3.0, WBFL::Units::Measure::Inch);
                    gdim.SetValue(H1);
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
                    CString strVal1, strVal2;

                    strVal1.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    strVal2.Format(_T("%0.0f %s"), val1, gdim.GetUnitTag().c_str());

                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    {
                        strVal1 = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                        strVal2 = FormatFeetInchesFromDecimalInches(RoundOff(val1, 0.125)).c_str();
                    }
                    SetColumnData(&ws, ++col, nPrevGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), strVal1);
                    SetColumnData(&ws, ++col, nPrevGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), strVal2);
                }
            }

            //longitudinal rebar
            if (familyCLSID == CLSID_SlabBeamFamily)
            {
                GET_IFACE2(pBroker, ILongitudinalRebar, pLongRebar);
                const CLongitudinalRebarData* pLRD = pLongRebar->GetSegmentLongitudinalRebarData(segmentKey);

                ATLASSERT(pLRD->RebarRows.size() <= 2);

                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, _T("-"));
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, _T("-"));

                for (const auto& row : pLRD->RebarRows)
                {
                    if (row.Face == pgsTypes::FaceType::TopFace)
                    {
                        SetColumnData(&ws, --col, nPrevGirders * grpIdx + gdrIdx + 5, WBFL::LRFD::RebarPool::GetBarSize(row.BarSize).c_str());
                        SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, row.NumberOfBars);
                    }
                }

                SetColumnData(&ws, ++col, nPrevGirders* grpIdx + gdrIdx + 5, _T("-"));
                SetColumnData(&ws, ++col, nPrevGirders* grpIdx + gdrIdx + 5, _T("-"));

                for (const auto& row : pLRD->RebarRows)
                {
                    if (row.Face == pgsTypes::FaceType::BottomFace)
                    {
                        SetColumnData(&ws, --col, nPrevGirders * grpIdx + gdrIdx + 5, WBFL::LRFD::RebarPool::GetBarSize(row.BarSize).c_str());
                        SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + 5, row.NumberOfBars);
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
                strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                SetColumnData(&ws, ++col, nPrevGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
            }

            auto pLiftArtifact = pSegmentArtifact->GetLiftingCheckArtifact();
            if (pLiftArtifact != nullptr)
            {
                GET_IFACE2(pBroker, ISegmentLifting, pSegmentLifting);
                Float64 L = pSegmentLifting->GetLeftLiftingLoopLocation(segmentKey);
                gdim.SetValue(L);
                const auto& val = gdim.GetValue(true);
                strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
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
                strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                gdim.SetValue(trailingOverhang);
                const auto& val1 = gdim.GetValue(true);
                strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                    strValue = FormatFeetInchesFromDecimalInches(RoundOff(val1, 0.125)).c_str();
                SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);

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

                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nPrevGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                }

                if (pSegment->HandlingData.pHaulTruckLibraryEntry)
                {
                    gdim.SetValue(pSegment->HandlingData.pHaulTruckLibraryEntry->GetAxleWidth());
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.0f %s"), val, gdim.GetUnitTag().c_str());
                    if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                        strValue = FormatFeetInchesFromDecimalInches(RoundOff(val, 0.125)).c_str();
                    SetColumnData(&ws, ++col, nPrevGirders * grpIdx + gdrIdx + (bSlab ? 5 : 4), strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nPrevGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), _T("-"));
                }
            }

            GET_IFACE2(pBroker, ILibrary, pLib);
            GET_IFACE2(pBroker, ISpecification, pSpec);
            std::_tstring spec_name = pSpec->GetSpecification();
            const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(spec_name.c_str());
            const auto& creep_criteria = pSpecEntry->GetCreepCriteria();
            const auto& limits_criteria = pSpecEntry->GetLimitsCriteria();

            Float64 min_days = WBFL::Units::ConvertFromSysUnits(creep_criteria.CreepDuration2Min, WBFL::Units::Measure::Day);
            Float64 max_days = WBFL::Units::ConvertFromSysUnits(creep_criteria.CreepDuration2Max, WBFL::Units::Measure::Day);

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
                        var.lVal = RGB(255,0,0);
                        CString msg;
                        msg.Format(_T("Girder %s WARNING: Final camber is downward. The girder may end up with a sag."), GIRDER_LABEL(girderKey));
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
                        msg.Format(_T("Girder %s WARNING: Screed camber (C) is greater than the %s camber at time of deck casting, D. The girder may end up with a sag."), 
                            GIRDER_LABEL(girderKey), camberType.c_str());
                        m_warnings.emplace_back(msg, _T("Color"), var);
                    }
                    else if (IsEqual(C, D, WBFL::Units::ConvertToSysUnits(0.25, WBFL::Units::Measure::Inch)))
                    {
                        VariantInit(&var);
                        var.vt = VT_I4;
                        var.lVal = RGB(255, 0, 0);
                        msg.Format(_T("Girder %s WARNING: Screed camber (C) is nearly equal to the %s camber at time of deck casting, D. The girder may end up with a sag."),
                            GIRDER_LABEL(girderKey), camberType.c_str());
                        m_warnings.emplace_back(msg, _T("Color"), var);
                    }

                    if (Dmin_LowerBound < C && limits_criteria.SagCamber != pgsTypes::SagCamber::LowerBoundCamber)
                    {
                        Float64 Cfactor = pCamber->GetLowerBoundCamberVariabilityFactor();
                        VariantInit(&var);
                        var.vt = VT_I4;
                        var.lVal = RGB(0, 0, 0);
                        msg.Format(_T("Girder %s Screed camber (C) is greater than the lower bound camber at time of deck casting (%0.0f%% of D%0.0f). The girder may end up with a sag if the deck is placed at day %0.0f and the actual camber is a lower bound value."), 
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

            if (reinfDetailsResult == STIRRUP_ERROR_ZONES)
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
            else if (reinfDetailsResult == STIRRUP_ERROR_SYMMETRIC)
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
            else if (reinfDetailsResult == STIRRUP_ERROR_STARTZONE)
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
            else if (reinfDetailsResult == STIRRUP_ERROR_LASTZONE)
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
            else if (reinfDetailsResult == STIRRUP_ERROR_BARSIZE)
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
            else if (reinfDetailsResult == STIRRUP_ERROR_V6)
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
            else if (reinfDetailsResult < 0)
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
                SetColumnData(&ws, 1, nPrevGirders * grpIdx + m_last_same_gdrID + (bSlab ? 5 : 4), strGirder);
                
                CString strCell;

                strCell.Format(_T("C%d:%s%d"), nPrevGirders * grpIdx + gdrIdx + (bSlab ? 6 : 5),
                    GetColumnLabel(m_current_row_data.size()), nPrevGirders * grpIdx + gdrIdx + (bSlab ? 6 : 5));

                Range cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
                if (nMaxDebondCombos > 0)
                {
                    cell.SetRowHeight(COleVariant((long)20.5 * nMaxDebondCombos));
                }

                strCell.Format(_T("C%d:%s%d"), nPrevGirders * grpIdx + m_last_same_gdrID + (bSlab ? 6 : 5),
                    GetColumnLabel(m_current_row_data.size()), nPrevGirders* grpIdx + gdrIdx - 1 + (bSlab ? 6 : 5));
                cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
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
                SetColumnData(&ws, 1, nPrevGirders* grpIdx + gdrIdx + (bSlab ? 5 : 4), GetColumnLabel(gdrIdx));
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

                    hits++;

                    if (hits == 1)
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

                    SetColumnData(&ws2, 0, 2 + hits, SEGMENT_LABEL(segmentKey));

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
                            SetColumnData(&ws2, cdx++, 2 + hits, strValue);
                        }

                    }
                    
                }
            }

        } // gdrIdx

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


    //thicken table border
    strCell.Format(_T("A1:%s%d"), GetColumnLabel(m_previous_row_data.size() + 1), tableOffset - 1);
    cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
    cell.BorderAround(
        COleVariant((long)1),
        (long)4,
        (long)-4105,
        COleVariant((long)0)
    );

    cell.BorderAround(
        COleVariant((long)1),
        (long)4,
        (long)-4105,
        COleVariant((long)0)
    );

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

    int nPatternHeadings = 0;

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

        nPatternHeadings = headerInfo.size();

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

            strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(0), tableOffset + nPatternHeadings + idx, GetColumnLabel(2), tableOffset + nPatternHeadings + idx);
            Range cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
            cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
            cell.SetValue2(COleVariant(GetColumnLabel(idx)));
            cell.BorderAround(
                COleVariant((long)1),
                (long)3,
                (long)-4105,
                COleVariant((long)0)
            );

            strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(3), tableOffset + nPatternHeadings + idx, GetColumnLabel((bUbeam? 5 : 4)), tableOffset + nPatternHeadings + idx);
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
    }

    int nDebondHeadings = 0;

    if (vDebond.size() != 0)
    {

        nDebondHeadings = 2;

        strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(0), tableOffset + nPatternHeadings + (vStrandPatterns.size() != 0 ? 1 : 0) + vStrandPatterns.size(),
            GetColumnLabel(4), tableOffset + nPatternHeadings + (vStrandPatterns.size() != 0 ? 1 : 0) + vStrandPatterns.size());
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
            strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(0), tableOffset + nPatternHeadings + nDebondHeadings + vStrandPatterns.size() + idx,
                GetColumnLabel(4), tableOffset + nPatternHeadings + nDebondHeadings + vStrandPatterns.size() + idx);
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


    if (m_warnings.size() != 0)
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

        newSheet.SetName(_T("Designer Notes"));


        strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(0), 1, GetColumnLabel(0), 1);
        Range cell = newSheet.GetRange(COleVariant(strCell), COleVariant(strCell));
        cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
        COleVariant vLeft((long)-4131, VT_I4);
        cell.SetHorizontalAlignment(vLeft);
        cell.SetValue2(COleVariant(_T("NOTES TO DESIGNER:")));

        for (IndexType idx = 0; idx < m_warnings.size(); idx++)
        {
            strCell.Format(_T("%s%d:%s%d"), GetColumnLabel(0), 2, GetColumnLabel(0), 2);
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

        m_warnings.clear();
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

