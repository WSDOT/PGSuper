///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
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

// 
// Do not include stdafx.h here - this control is to be shared among multiple dll projects
#include <WBFLVersion.h>

#include <afxwin.h>
#include <StdAfx.h>  ///// why??

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdisp.h>
#endif // _AFX_NO_OLE_SUPPORT

#include "SharedCTrls\MultiBearingSelectGrid.h" 
#include <PsgLib\Keys.h>
#include <PsgLib\BridgeDescription2.h>

#include <EAF\EAFUtilities.h>
#include <IFace\Tools.h>
#include <IFace\Project.h>
#include <IFace\DocumentType.h>

#if defined _DEBUG
#include <IFace\Bridge.h>
#endif


#include <IFace/Intervals.h>
#include <Reporting/ReactionInterfaceAdapters.h>
#include <psgLib\GirderLabel.h>
#include <unordered_set>




/////////////////////////////////////////////////////////////////////////////
// CMultiBearingSelectGrid

CMultiBearingSelectGrid::CMultiBearingSelectGrid()
{
    //   RegisterClass();
}


void CMultiBearingSelectGrid::CustomInit(const GroupGirderCollection& groupGirderCollection)
{
    // Initialize the grid. For CWnd based grids this call is // 
    // essential. For view based grids this initialization is done
    // in OnInitialUpdate.

    Initialize();

    GetParam()->EnableUndo(FALSE);

    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IBridge, pBridge);
    GET_IFACE2(pBroker, IBearingDesign, pBearingDesign);
    GET_IFACE2(pBroker, IIntervals, pIntervals);

    GirderIndexType nGirdersTotal = 0;
    CGirderKey gkey(0, 0); // girderKey with max no. RLs

    std::unordered_set<std::_tstring> seen;

    for (GroupIndexType igg = 0; igg < groupGirderCollection.size(); igg++)
    {
        for (GirderIndexType igRL = 0; igRL < groupGirderCollection[igg].size(); igRL++)
        {
            nGirdersTotal += 1;
            auto grlSize = groupGirderCollection[igg][igRL].size();

            gkey = CGirderKey(igg, igRL);

            IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();

            std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(
                pBearingDesign, lastCompositeDeckIntervalIdx, gkey));

            ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

            for (iter.First(); !iter.IsDone(); iter.Next())
            {
                if (seen.insert(iter.CurrentItem().PierLabel).second)
                {
                    m_vRL.emplace_back(iter.CurrentItem());
                }
            }
            

        }
    }




    const ROWCOL num_rows = (ROWCOL)nGirdersTotal;
    const ROWCOL num_cols = (ROWCOL)m_vRL.size();

    SetRowCount(num_rows);
    SetColCount(num_cols);

    // mimic excel selection behavior
    GetParam()->SetExcelLikeSelectionFrame(TRUE);

    // no row or column moving
    GetParam()->EnableMoveRows(FALSE);
    GetParam()->EnableMoveCols(FALSE);

    // disable left side
    SetStyleRange(CGXRange(0, 0, num_rows, 0), CGXStyle()
        .SetControl(GX_IDS_CTRL_HEADER)
        .SetHorizontalAlignment(DT_CENTER)
        .SetEnabled(FALSE)          // disables usage as current cell
    );

    // disable top row
    SetStyleRange(CGXRange(0, 0, 0, num_cols), CGXStyle()
        .SetWrapText(TRUE)
        .SetHorizontalAlignment(DT_CENTER)
        .SetVerticalAlignment(DT_VCENTER)
        .SetEnabled(FALSE)          // disables usage as current cell
    );

    // top row labels


    GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);

    const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();


    auto idx = 0;
    for (const auto& rl : m_vRL)
    {

        const CPierData2* pPier = pBridgeDesc->GetPier(rl.PierIdx);
        CString bcType;

        if (pPier->IsBoundaryPier())
        {
            bool bNoDeck = IsNonstructuralDeck(pBridgeDesc->GetDeckDescription()->GetDeckType());
            bcType = CPierData2::AsString(pPier->GetBoundaryConditionType(), bNoDeck);
        }
        else
        {
            bcType = CPierData2::AsString(pPier->GetSegmentConnectionType());
        }


        CString lbl = rl.PierLabel.c_str();

        if (bcType == _T("Hinge"))
        {
            CString hingeBrgStr;
            hingeBrgStr.Format(_T("%s (Hinge)"), lbl);
            lbl = hingeBrgStr;
        }

        SetStyleRange(CGXRange(0, ROWCOL(idx + 1)), CGXStyle()
            .SetWrapText(TRUE)
            .SetEnabled(FALSE)          // disables usage as current cell
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
            .SetValue(lbl)
        );

        ++idx;
    }

    // left column labels

    GET_IFACE2(pBroker, IDocumentType, pDocType);
    bool bIsPGSuper = pDocType->IsPGSuperDocument();

    RowIndexType row = 0;

    GroupIndexType nGroups = pBridge->GetGirderGroupCount();
    for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
    {

        GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
        for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
        {

            CString lbl;
            if (bIsPGSuper)
            {
                lbl.Format(_T("Span %s - Girder %s"), LABEL_SPAN(grpIdx), LABEL_GIRDER(gdrIdx));
            }
            else
            {
                lbl.Format(_T("Group %d - Girder %s"), LABEL_GROUP(grpIdx), LABEL_GIRDER(gdrIdx));
            }

            SetStyleRange(CGXRange(ROWCOL(row+1), 0), CGXStyle()
                .SetWrapText(TRUE)
                .SetEnabled(FALSE)          // disables usage as current cell
                .SetHorizontalAlignment(DT_CENTER)
                .SetVerticalAlignment(DT_VCENTER)
                .SetValue(lbl));

            ++row;

        }

    }

    // fill controls for reaction locations

    nGirdersTotal = 0;

    for (GroupIndexType grpIdx = 0; grpIdx < groupGirderCollection.size(); grpIdx++)
    {
        GirderIndexType nGirders = groupGirderCollection[grpIdx].size();

        for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++) // for each girder, check if an RL in vRL exists for it. If so, get the girder-relative index of it
        {

            GET_IFACE2(pBroker, IIntervals, pIntervals);
            IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
            std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(
                pBearingDesign, lastCompositeDeckIntervalIdx, CGirderKey(grpIdx, gdrIdx)));

            ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

            for (IndexType rlIdx = 0; rlIdx < m_vRL.size(); rlIdx++)
            {

                ROWCOL nCol = ROWCOL(rlIdx + 1);
                ROWCOL nRow = ROWCOL(nGirdersTotal + 1);

                IndexType grlIdx = 0;
                bool found = false;
                for (iter.First(); !iter.IsDone(); iter.Next())
                {
                    if (iter.CurrentItem().PierLabel == m_vRL[rlIdx].PierLabel) // trying to find the RL index relative to the girder, not vRL
                    {
                        found = true;
                        break;
                    }
                    grlIdx++;
                }

                if (found)
                {
                    UINT enabled = groupGirderCollection[grpIdx][gdrIdx][grlIdx] ? 1 : 0;
                    // unchecked check box for bearing
                    SetStyleRange(CGXRange(nRow, nCol), CGXStyle()
                        .SetControl(GX_IDS_CTRL_CHECKBOX3D)
                        .SetValue(enabled)
                        .SetHorizontalAlignment(DT_CENTER)
                    );
                }
                else
                {
                    // no bearing here - disable cell
                    SetStyleRange(CGXRange(nRow, nCol), CGXStyle()
                        .SetEnabled(FALSE)
                    );
                }
            }

            nGirdersTotal += 1;
        }
    }

    // Make it so that text fits correctly in header rows/cols
    ResizeColWidthsToFit(CGXRange(0, 0, num_rows, num_cols));

    EnableIntelliMouse();

    // Get grid started in dialog navigation:
    SetFocus();
    SetCurrentCell(1, 1);

    GetParam()->EnableUndo(TRUE);

}


std::vector<ReactionLocation> CMultiBearingSelectGrid::GetData()
{
    std::vector<ReactionLocation> data;

    ROWCOL nRows = GetRowCount();
    ROWCOL nCols = GetColCount();

    for (ROWCOL col = 0; col < nCols; col++)
    {
        for (ROWCOL row = 0; row < nRows; row++)
        {
            bool bDat = GetCellValue(row + 1, col + 1);

            if (bDat)
            {

                auto pBroker = EAFGetBroker();
                GET_IFACE2(pBroker, IBearingDesign, pBearingDesign);
                GET_IFACE2(pBroker, IIntervals, pIntervals);
                GET_IFACE2(pBroker, IBridge, pBridge);
                IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();

                IndexType ggIdx = 0;

                GroupIndexType nGroups = pBridge->GetGirderGroupCount();
                for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
                {

                    GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
                    for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
                    {

                        std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(
                            pBearingDesign, lastCompositeDeckIntervalIdx, CGirderKey(grpIdx, gdrIdx)));

                        ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);


                        for (iter.First(); !iter.IsDone(); iter.Next())
                        {

                            if (iter.CurrentItem().PierLabel == m_vRL[col].PierLabel && row == ggIdx)
                            {
                                const ReactionLocation& reactionLocation(iter.CurrentItem());
                                data.push_back(reactionLocation);
                            }


                        }

                        ggIdx++;

                    }
                }
 
            }
        }
    }

    return data;
}

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

