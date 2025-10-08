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
    cell.SetWrapText(COleVariant((short)VARIANT_TRUE, VT_BOOL));
    cell.BorderAround(
        COleVariant((long)1),
        (long)2,
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
    if constexpr (std::is_convertible_v<T, LPCTSTR> )
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
    cell.SetValue2(COleVariant(strValue));
    cell.SetWrapText(COleVariant((short)VARIANT_TRUE, VT_BOOL));
    cell.BorderAround(
        COleVariant((long)1),
        (long)2,
        (long)-4105,
        COleVariant((long)0)
    );
}

HRESULT CGirderScheduleExporter::Export(std::shared_ptr<WBFL::EAF::Broker> pBroker)
{
    GET_IFACE2(pBroker, IEAFProgress, pProgress);
    WBFL::EAF::AutoProgress ap(pProgress);


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
    INIT_FRACTIONAL_LENGTH_PROTOTYPE(glength, IS_US_UNITS(pDisplayUnits), 4, RoundOff, pDisplayUnits->GetSpanLengthUnit(), true, true);

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

    const std::vector<ScheduleHeaderInfo>& headerInfo =
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
    
    for (const auto& info : headerInfo)
    {
        SetColumnHeader(&ws, info.colIdx, info.colSpan, info.rowIdx, info.rowSpan, info.orientation, info.strValue);
    }

    CComPtr<IAnnotatedDisplayUnitFormatter> pADUF;
    pADUF.CoCreateInstance(CLSID_AnnotatedDisplayUnitFormatter);

    //Set girder data
    GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
    const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
    
    GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
    CGirderKey girderKey;
    CComBSTR bstr;

    for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
    {
        GirderIndexType gdrIdx = 0;
        const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
        GirderIndexType nGirders = pGroup->GetGirderCount();
        for (gdrIdx; gdrIdx < nGirders; gdrIdx++)
        {
            girderKey = CGirderKey(grpIdx, gdrIdx);

            ColumnIndexType col = 0;

            //Set Girder Label
            CString strGirder = GetColumnLabel(gdrIdx);
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strGirder);

            //Set Girder Series
            const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
            CString strSeries = pGirder->GetGirderName();
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strSeries);

            //Set Plan Length
            GET_IFACE2(pBroker, IBridge, pBridge);
            const CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
            CSegmentKey segmentKey(pSegment->GetSegmentKey());

            const auto& rptPlanLength = glength.SetValue(pBridge->GetSegmentPlanLength(segmentKey));
            const auto& planLength = glength.GetValue(true);

            CString strValue;

            strValue.Format(_T("%0.3f %s"), planLength, glength.GetUnitTag().c_str());

            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);

            SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("-"));
            SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("-"));
            SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("-"));
            SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("-"));

            CComPtr<IAngle> objAngle1, objAngle2;
            pBridge->GetSegmentSkewAngle(segmentKey, pgsTypes::metStart, &objAngle1);
            pBridge->GetSegmentSkewAngle(segmentKey, pgsTypes::metEnd, &objAngle2);
            Float64 t1, t2;
            objAngle1->get_Value(&t1);
            objAngle2->get_Value(&t2);

            angle.SetValue(t1);
            const auto& ft1 = angle.GetValue(true);
            strValue.Format(_T("%0.2f"), ft1);
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
            angle.SetValue(t2);
            const auto& ft2 = angle.GetValue(true);
            strValue.Format(_T("%0.2f"), ft2);
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);

            GET_IFACE2(pBroker, ISectionProperties, pSectProp);

            GET_IFACE2(pBroker, IIntervals, pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
            IntervalIndexType finalIntervalIdx = pIntervals->GetIntervalCount() - 1;

            const GirderLibraryEntry* pGdrLibEntry = pGirder->GetGirderLibraryEntry();
            auto factory = pGdrLibEntry->GetBeamFactory();
            CLSID familyCLSID = factory->GetFamilyCLSID();

            GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest);
            PoiList pmid;
            pPointOfInterest->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT, &pmid);
            ATLASSERT(pmid.size() == 1);
            const pgsPointOfInterest& poiMidSpan(pmid.front());

            pgsPointOfInterest poiStart(segmentKey, 0.0);

            if (familyCLSID != CLSID_SlabBeamFamily)
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
                    strValue.Format(_T("%0.3f %s"), P1, gdim.GetUnitTag().c_str());
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
                    strValue.Format(_T("%0.3f %s"), P2, gdim.GetUnitTag().c_str());
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                }
            }

            GET_IFACE2(pBroker, IMaterials, pMaterial);

            
            stress.SetValue(pMaterial->GetSegmentDesignFc(segmentKey, finalIntervalIdx));
            const auto& fc = stress.GetValue(true);
            strValue.Format(_T("%0.3f"), fc);
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);

            stress.SetValue(pMaterial->GetSegmentDesignFc(segmentKey, releaseIntervalIdx));
            const auto& fci = stress.GetValue(true);
            strValue.Format(_T("%0.3f"), fci);
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);

            //set number of strands
            GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry);


            GET_IFACE2(pBroker, IArtifact, pIArtifact);
            const pgsGirderArtifact* pArtifact = pIArtifact->GetGirderArtifact(girderKey);
            const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);

            bool bCanReportPrestressInformation = true;

            // WsDOT reports don't support Straight-Web strand option (except for slab beams)
            if (pStrandGeometry->GetAreHarpedStrandsForcedStraight(segmentKey) && CLSID_SlabBeamFamily != familyCLSID)
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
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, Ns);

                if (CLSID_SlabBeamFamily != familyCLSID)
                {
                    StrandIndexType nDebonded = pStrandGeometry->GetNumDebondedStrands(segmentKey, pgsTypes::Straight, pgsTypes::dbetEither);
                    if (nDebonded != 0)
                    {
                        ///add debonded...................
                    }

                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, Nh);
                }

                if (0 < pStrandGeometry->GetMaxStrands(segmentKey, pgsTypes::Temporary))
                {
                    StrandIndexType Nt = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Temporary);
                    CString strNt;
                    strNt.Format(_T("%d"), Nt);
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, Nt);
                }
            }
            else
            {

                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("-"));

                if (CLSID_SlabBeamFamily != familyCLSID)
                {
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("-"));
                }

                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("-"));
            }

            Float64 ybg = pSectProp->GetY(releaseIntervalIdx, poiMidSpan, pgsTypes::BottomGirder);
            Float64 sse = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poiMidSpan, pgsTypes::Straight).Y();
            if (0 < Ns)
            {
                gdim.SetValue(ybg - sse);
                const auto& val = gdim.GetValue(true);
                strValue.Format(_T("%0.3f %s"), val, gdim.GetUnitTag().c_str());
                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, strValue);
            }
            else
            {
                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("N/A"));
            }

            if (CLSID_SlabBeamFamily != familyCLSID)
            {
                Float64 hse = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poiMidSpan, pgsTypes::Harped).Y();
                if (0 < Nh)
                {
                    gdim.SetValue(ybg - hse);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.3f %s"), val, gdim.GetUnitTag().c_str());
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("N/A"));
                }


                Float64 ytg = pSectProp->GetY(releaseIntervalIdx, poiStart, pgsTypes::TopGirder);
                Float64 hss = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poiStart, pgsTypes::Harped).Y();
                if (0 < Nh)
                {
                    gdim.SetValue(ytg + hss);
                    const auto& val = gdim.GetValue(true);
                    strValue.Format(_T("%0.3f %s"), val, gdim.GetUnitTag().c_str());
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("N/A"));
                }
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
                    val.Format(_T("%d "), strandIdx + 1);
                    strExt1.Append(val);
                }
            }

            if (nExtended == 0)
            {
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
            }
            else
            {
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strExt1);
                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("-"));
            }

            nExtended = 0;
            CString strExt2;
            for (StrandIndexType strandIdx = 0; strandIdx < Ns; strandIdx++)
            {
                if (pStrandGeometry->IsExtendedStrand(segmentKey, pgsTypes::metEnd, strandIdx, pgsTypes::Straight))
                {
                    nExtended++;
                    CString val;
                    val.Format(_T("%d "), strandIdx + 1);
                    strExt2.Append(val);
                }
            }
            
            if (nExtended == 0)
            {
                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("-"));
                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("-"));
            }
            else
            {
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strExt2);
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
            }

            //int debondResults = DEBOND_ERROR_NONE;
            //if (CLSID_SlabBeamFamily == familyCLSID)
            //{
            //    // Debonding for slab beams
            //    pTable->SetColumnSpan(++row, 0, 2);
            //    (*pTable)(row, 0) << _T("Straight Strands to Debond");

            //    std::vector<DebondInformation> debondInfo;
            //    debondResults = GetDebondDetails(pBroker, segmentKey, debondInfo);

            //    if (debondResults < 0)
            //    {
            //        for (int i = 0; i < 3; i++)
            //        {
            //            pTable->SetColumnSpan(++row, 0, 2);
            //            (*pTable)(row, 0) << _T("Group ") << (i + 1);

            //            (*pTable)(++row, 0) << _T("Strands to Debond");
            //            (*pTable)(row, 1) << _T("%");
            //            (*pTable)(++row, 0) << _T("Sleeved Length at Ends to Prevent Bond");
            //            (*pTable)(row, 1) << _T("%");
            //        }
            //    }
            //    else
            //    {
            //        int groupCount = 0;
            //        std::vector<DebondInformation>::iterator iter(debondInfo.begin());
            //        std::vector<DebondInformation>::iterator end(debondInfo.end());
            //        for (; iter != end; iter++, groupCount++)
            //        {
            //            DebondInformation& dbInfo = *iter;

            //            pTable->SetColumnSpan(++row, 0, 2);
            //            (*pTable)(row, 0) << _T("Group ") << (groupCount + 1);

            //            (*pTable)(++row, 0) << _T("Strands to Debond");
            //            std::vector<StrandIndexType>::iterator strandIter(dbInfo.Strands.begin());
            //            std::vector<StrandIndexType>::iterator strandIterEnd(dbInfo.Strands.end());
            //            for (; strandIter != strandIterEnd; strandIter++)
            //            {
            //                StrandIndexType strandIdx = *strandIter;
            //                (*pTable)(row, 1) << _T(" ") << (strandIdx + 1);
            //            }

            //            (*pTable)(++row, 0) << _T("Sleeved Length at Ends to Prevent Bond");
            //            (*pTable)(row, 1) << glength.SetValue(dbInfo.Length);
            //        }

            //        for (int i = groupCount; i < 3; i++)
            //        {
            //            pTable->SetColumnSpan(++row, 0, 2);
            //            (*pTable)(row, 0) << _T("Group ") << (i + 1);

            //            (*pTable)(++row, 0) << _T("Strands to Debond");
            //            (*pTable)(row, 1) << _T("-");
            //            (*pTable)(++row, 0) << _T("Sleeved Length at Ends to Prevent Bond");
            //            (*pTable)(row, 1) << _T("-");
            //        }
            //    }
            //}

            GET_IFACE2(pBroker, ICamber, pCamber);
            pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->GetDeckType();
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
                    const auto& A = gdim.GetValue(true);
                    strValue.Format(_T("%0.3f %s"), A, gdim.GetUnitTag().c_str());
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                }
                else
                {
                    CString str;
                    str.Format(_T("%d"), gdim.GetValue(pSegment->GetSlabOffset(pgsTypes::metStart)));
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, str);

                    str.Format(_T("%d"), gdim.GetValue(pSegment->GetSlabOffset(pgsTypes::metEnd)));
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, str);
                }

                gdim.SetValue(pCamber->GetScreedCamber(poiMidSpan, pgsTypes::CreepTime::Max));
                const auto& sCamber = gdim.GetValue(true);
                if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
                {
                    strValue.Format(_T("%0.3f\""), sCamber);
                }
                else
                {
                    strValue.Format(_T("%0.3fm"), sCamber);
                }
                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, strValue);
            }

            // get # of days for creep
            Float64 Dmax_UpperBound, Dmax_Average, Dmax_LowerBound;
            Float64 Dmin_UpperBound, Dmin_Average, Dmin_LowerBound;
            pCamber->GetDCamberForGirderScheduleEx(poiMidSpan, pgsTypes::CreepTime::Max, &Dmax_UpperBound, &Dmax_Average, &Dmax_LowerBound);
            pCamber->GetDCamberForGirderScheduleEx(poiMidSpan, pgsTypes::CreepTime::Min, &Dmin_UpperBound, &Dmin_Average, &Dmin_LowerBound);

            gdim.SetValue(Dmin_LowerBound);
            const auto& dMin_LowerBound = gdim.GetValue(true);
            strValue.Format(_T("%0.3f %s"), dMin_LowerBound, gdim.GetUnitTag().c_str());
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);

            gdim.SetValue(Dmax_UpperBound);
            const auto& dMax_UpperBound = gdim.GetValue(true);
            strValue.Format(_T("%0.3f %s"), dMax_UpperBound, gdim.GetUnitTag().c_str());
            SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);

            // Stirrups
            Float64 z1Spacing, z1Length;
            Float64 z2Spacing, z2Length;
            Float64 z3Spacing, z3Length;
            CWSDOTReinforcement details;
            int reinfDetailsResult = details.GetWSDOTReinforcementDetails(pBroker, segmentKey, familyCLSID, 
                &z1Spacing, &z1Length, &z2Spacing, &z2Length, &z3Spacing, &z3Length);
            if (reinfDetailsResult < 0)
            {
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, _T("-"));
            }
            else
            {
                gdim.SetValue(z1Spacing);
                const auto& z1Spacing = gdim.GetValue(true);
                strValue.Format(_T("%0.3f %s"), z1Spacing, gdim.GetUnitTag().c_str());
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                glength.SetValue(z1Length);
                const auto& z1Length = glength.GetValue(true);
                strValue.Format(_T("%0.3f %s"), z1Length, glength.GetUnitTag().c_str());
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                gdim.SetValue(z2Spacing);
                const auto& z2Spacing = gdim.GetValue(true);
                strValue.Format(_T("%0.3f %s"), z2Spacing, gdim.GetUnitTag().c_str());
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                glength.SetValue(z2Length);
                const auto& z2Length = glength.GetValue(true);
                strValue.Format(_T("%0.3f %s"), z2Length, glength.GetUnitTag().c_str());
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                gdim.SetValue(z3Spacing);
                const auto& z3Spacing = gdim.GetValue(true);
                strValue.Format(_T("%0.3f %s"), z3Spacing, gdim.GetUnitTag().c_str());
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                glength.SetValue(z3Length);
                const auto& z3Length = glength.GetValue(true);
                strValue.Format(_T("%0.3f %s"), z3Length, glength.GetUnitTag().c_str());
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
            }

            // Stirrup Height

            if (familyCLSID == CLSID_WFBeamFamily || familyCLSID == CLSID_UBeamFamily)
            {
                // H1 (Hg + "A" + 3")
                if (pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge)
                {
                    Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poiStart);
                    Float64 H1 = pBridgeDesc->GetSlabOffset() + Hg + WBFL::Units::ConvertToSysUnits(3.0, WBFL::Units::Measure::Inch);
                    glength.SetValue(H1);
                    const auto& fH1 = glength.GetValue(true);
                    strValue.Format(_T("%0.3f %s"), fH1, glength.GetUnitTag().c_str());
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                }
                else
                {
                    Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poiStart);
                    Float64 H1 = pSegment->GetSlabOffset(pgsTypes::metStart) + Hg + WBFL::Units::ConvertToSysUnits(3.0, WBFL::Units::Measure::Inch);
                    glength.SetValue(H1);
                    const auto& fH1s = glength.GetValue(true);
                    strValue.Format(_T("%0.3f %s"), fH1s, glength.GetUnitTag().c_str());
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);

                    pgsPointOfInterest poiEnd(poiStart);
                    poiEnd.SetDistFromStart(pBridge->GetSegmentLength(segmentKey));
                    Hg = pSectProp->GetHg(releaseIntervalIdx, poiEnd);
                    H1 = pSegment->GetSlabOffset(pgsTypes::metEnd) + Hg + WBFL::Units::ConvertToSysUnits(3.0, WBFL::Units::Measure::Inch);
                    glength.SetValue(H1);
                    const auto& fH1e = glength.GetValue(true);
                    strValue.Format(_T("%0.3f %s"), fH1e, glength.GetUnitTag().c_str());
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
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
                strValue.Format(_T("%0.3f %s"), val, gdim.GetUnitTag().c_str());
                SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, strValue);
            }

            auto pLiftArtifact = pSegmentArtifact->GetLiftingCheckArtifact();
            if (pLiftArtifact != nullptr)
            {
                GET_IFACE2(pBroker, ISegmentLifting, pSegmentLifting);
                Float64 L = pSegmentLifting->GetLeftLiftingLoopLocation(segmentKey);
                glength.SetValue(L);
                const auto& val = glength.GetValue(true);
                strValue.Format(_T("%0.3f %s"), val, glength.GetUnitTag().c_str());
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
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

                glength.SetValue(leadingOverhang);
                const auto& val = glength.GetValue(true);
                strValue.Format(_T("%0.3f %s"), val, glength.GetUnitTag().c_str());
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                glength.SetValue(trailingOverhang);
                const auto& val1 = glength.GetValue(true);
                strValue.Format(_T("%0.3f %s"), val1, glength.GetUnitTag().c_str());
                SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);

                if (pSegment->HandlingData.pHaulTruckLibraryEntry)
                {
                    spring.SetValue(pSegment->HandlingData.pHaulTruckLibraryEntry->GetRollStiffness());
                    const auto& val2 = spring.GetValue(true);
                    strValue.Format(_T("%0.3f"), val2);
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("-"));
                }

                if (pSegment->HandlingData.pHaulTruckLibraryEntry)
                {
                    glength.SetValue(pSegment->HandlingData.pHaulTruckLibraryEntry->GetAxleWidth());
                    const auto& val = glength.GetValue(true);
                    strValue.Format(_T("%0.3f %s"), val, glength.GetUnitTag().c_str());
                    SetColumnData(&ws, ++col, nGirders * grpIdx + gdrIdx + 4, strValue);
                }
                else
                {
                    SetColumnData(&ws, ++col, nGirders* grpIdx + gdrIdx + 4, _T("-"));
                }
            }



        } // gdrIdx

        //set span
        SpanIndexType spanIdx = girderKey.groupIndex;
        CString strSpan;
        strSpan.Format(_T("%d"), spanIdx + 1);
        SetColumnData(&ws, 0, 4 + grpIdx * (nGirders+1), (CString)strSpan);

        //merge and format span cells
        CString strCell;
        strCell.Format(_T("A%d:A%d"), (grpIdx * nGirders) + 5, (grpIdx * nGirders) + (4 + nGirders));
        Range cell = ws.GetRange(COleVariant(strCell), COleVariant(strCell));
        cell.Merge(COleVariant((short)VARIANT_FALSE, VT_BOOL));
        cell.BorderAround(
            COleVariant((long)1),
            (long)2,
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

