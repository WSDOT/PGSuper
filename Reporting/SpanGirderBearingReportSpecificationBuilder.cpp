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

#include "stdafx.h"
#include <Reporting\SpanGirderBearingReportSpecificationBuilder.h>
#include <Reporting\SpanGirderBearingReportSpecification.h>
#include "BearingMultiViewReportDlg.h"
#include <Reporting\MultiBearingReportDlg.h>

#include <IFace\Selection.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>

#include <PsgLib\BridgeDescription2.h>
#include <Reporting/ReactionInterfaceAdapters.h>
#include <AgentTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




CBearingReportSpecificationBuilder::CBearingReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker, const ReactionLocation& reactionLocation) :
    CBrokerReportSpecificationBuilder(pBroker)
{
    m_ReactionLocation = reactionLocation;
}

CBearingReportSpecificationBuilder::~CBearingReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CBearingReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc, std::shared_ptr<WBFL::Reporting::ReportSpecification> pOldRptSpec) const
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ReactionLocation reactionLocation(m_ReactionLocation);



    //CSpanGirderReportDlg dlg(m_pBroker, rptDesc, CSpanGirderReportDlg::Mode::GroupGirderAndChapters, pOldRptSpec);
    //dlg.m_SegmentKey = CSegmentKey(girderKey, ALL_SEGMENTS);

    //if (dlg.DoModal() == IDOK)
    //{
    //    girderKey = dlg.m_SegmentKey;

    //    // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
    //    std::shared_ptr<CGirderReportSpecification> pOldGRptSpec = std::dynamic_pointer_cast<CGirderReportSpecification>(pOldRptSpec);

    //    std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
    //    if (pOldGRptSpec)
    //    {
    //        std::shared_ptr<CGirderReportSpecification> pNewGRptSpec(std::make_shared<CGirderReportSpecification>(*pOldGRptSpec));
    //        pNewGRptSpec->SetGirderKey(girderKey);

    //        pNewRptSpec = std::static_pointer_cast<WBFL::Reporting::ReportSpecification>(pNewGRptSpec);
    //    }
    //    else
    //    {
    //        pNewRptSpec = std::make_shared<CGirderReportSpecification>(rptDesc.GetReportName(), m_pBroker, girderKey);
    //    }

    //    std::vector<std::_tstring> chList = dlg.m_ChapterList;
    //    rptDesc.ConfigureReportSpecification(chList, pNewRptSpec);

    //    return pNewRptSpec;
    //}

    return nullptr;
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CBearingReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
{
//    // Get the selected group and girder
//    GET_IFACE(ISelection, pSelection);
//    CSelection selection = pSelection->GetSelection();
     ReactionLocation reactionLocation(m_ReactionLocation);
//    if (selection.Type == CSelection::Girder || selection.Type == CSelection::Segment)
//    {
//        GET_IFACE(IBridge, pBridge);
//        GroupIndexType nGroups = pBridge->GetGirderGroupCount();
//        if (selection.GroupIdx == ALL_GROUPS && selection.GirderIdx != ALL_GIRDERS && nGroups == 1)
//        {
//            // there is exactly one group and we have a valid girder number so take ALL_GROUPS to mean group 0... no prompting required
//            girderKey.groupIndex = 0;
//            girderKey.girderIndex = selection.GirderIdx;
//        }
//        else if (selection.GroupIdx == ALL_GROUPS || selection.GirderIdx == ALL_GIRDERS)
//        {
//            // we need a specific girder and don't have it.... going to have to prompt the user
//
//            // lets try to get the girder key close
//            if (selection.GroupIdx != ALL_GROUPS)
//            {
//                m_GirderKey.groupIndex = selection.GroupIdx;
//            }
//
//            if (selection.GirderIdx != ALL_GIRDERS)
//            {
//                m_GirderKey.girderIndex = selection.GirderIdx;
//            }
//
//            std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec = CreateReportSpec(rptDesc, std::shared_ptr<WBFL::Reporting::ReportSpecification>());
//
//            // put the girder key back the way it was
//            m_GirderKey = girderKey;
//
//            return pRptSpec;
//        }
//        else
//        {
//            girderKey.groupIndex = selection.GroupIdx;
//            girderKey.girderIndex = selection.GirderIdx;
//        }
//    }
    std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec(std::make_shared<CBearingReportSpecification>(rptDesc.GetReportName(), m_pBroker, reactionLocation));
//
//    rptDesc.ConfigureReportSpecification(pRptSpec);
//
//
    return pRptSpec;
}
//
//
//
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//
CMultiBearingReportSpecificationBuilder::CMultiBearingReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker) :
    CBrokerReportSpecificationBuilder(pBroker)
{
}

CMultiBearingReportSpecificationBuilder::~CMultiBearingReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CMultiBearingReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc, std::shared_ptr<WBFL::Reporting::ReportSpecification> pOldRptSpec) const
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    std::vector<ReactionLocation> reactionLocations;

    // Get information from old spec if we had one
    std::shared_ptr<CMultiBearingReportSpecification> pOldGRptSpec = std::dynamic_pointer_cast<CMultiBearingReportSpecification>(pOldRptSpec);
    if (pOldGRptSpec)
    {
        reactionLocations = pOldGRptSpec->GetReactionLocations();
    }
    else
    {
        // Prompt for span, girder, and chapter list
        GET_IFACE2(GetBroker(), ISelection, pSelection);
        CSelection selection = pSelection->GetSelection();

        CGirderKey girderKey;

        if (selection.Type == CSelection::Span)
        {
            GET_IFACE2(GetBroker(), IBridge, pBridge);
            girderKey.groupIndex = pBridge->GetGirderGroupIndex(selection.SpanIdx);
            girderKey.girderIndex = 0;
        }
        else if (selection.Type == CSelection::Girder || selection.Type == CSelection::Segment || selection.Type == CSelection::ClosureJoint)
        {
            girderKey.groupIndex = selection.GroupIdx;
            girderKey.girderIndex = selection.GirderIdx;
        }
        else
        {
            girderKey.groupIndex = 0;
            girderKey.girderIndex = 0;
        }

        GET_IFACE2(GetBroker(), IBearingDesign, pBearingDesign);
        GET_IFACE2(GetBroker(), IIntervals, pIntervals);
        GET_IFACE2(GetBroker(), IBridge, pBridge);

        IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();

        std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(
            pBearingDesign, lastCompositeDeckIntervalIdx, girderKey));

        ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);
        iter.First();
        PierIndexType startPierIdx = (iter.IsDone() ? INVALID_INDEX : iter.CurrentItem().PierIdx);

        std::vector<ReactionLocation> data;

        // Use iterator to walk locations
        for (iter.First(); !iter.IsDone(); iter.Next())
        {

            const ReactionLocation& reactionLocation(iter.CurrentItem());

            data.push_back(reactionLocation);

        }
    }

    //if (reactionLocations.size() == 1 && (girderKeys.front().groupIndex == ALL_GROUPS || girderKeys.front().girderIndex == ALL_GIRDERS))
    //{
    //    // multiple girders are selected... fill up the girder key vector
    //    CGirderKey girderKey = girderKeys.front();
    //    girderKeys.clear();
    //    GET_IFACE(IBridge, pBridge);
    //    GroupIndexType nGroups = pBridge->GetGirderGroupCount();
    //    GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
    //    GroupIndexType lastGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? nGroups - 1 : firstGroupIdx);
    //    for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++)
    //    {
    //        GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
    //        GirderIndexType firstGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
    //        GirderIndexType lastGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? nGirders - 1 : firstGirderIdx);
    //        for (GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++)
    //        {
    //            girderKeys.emplace_back(grpIdx, gdrIdx);
    //        }
    //    }
    //}



    // Prompt for span, girder, and chapter list
    CMultiBearingReportDlg dlg(GetBroker(), rptDesc, pOldRptSpec);
    dlg.m_Bearings = reactionLocations;

    if (dlg.DoModal() == IDOK)
    {
        // If possible, get information from old spec. Otherwise header/footer and other info will be lost
        std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
        if (pOldGRptSpec)
        {
            std::shared_ptr<CMultiBearingReportSpecification> pNewGRptSpec = std::make_shared<CMultiBearingReportSpecification>(*pOldGRptSpec);

            pNewGRptSpec->SetReactionLocations(dlg.m_Bearings);

            pNewRptSpec = std::static_pointer_cast<WBFL::Reporting::ReportSpecification>(pNewGRptSpec);
        }
        else
        {
            pNewRptSpec = std::make_shared<CMultiBearingReportSpecification>(rptDesc.GetReportName(), m_pBroker, dlg.m_Bearings);
        }

        std::vector<std::_tstring> chList = dlg.m_ChapterList;
        rptDesc.ConfigureReportSpecification(chList, pNewRptSpec);

        return pNewRptSpec;
    }

    return nullptr;
}

CMultiViewSpanGirderBearingReportSpecificationBuilder::CMultiViewSpanGirderBearingReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker) :
    CSpanReportSpecificationBuilder(pBroker)
{
}

CMultiViewSpanGirderBearingReportSpecificationBuilder::~CMultiViewSpanGirderBearingReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CMultiViewSpanGirderBearingReportSpecificationBuilder::CreateReportSpec(
    const WBFL::Reporting::ReportDescription& rptDesc, std::shared_ptr<WBFL::Reporting::ReportSpecification> pOldRptSpec) const
{
    // First check if we are getting a CGirderReportSpecification. If so, use our bro to take care of this
    std::shared_ptr<CBearingReportSpecification> pBearingRptSpec = std::dynamic_pointer_cast<CBearingReportSpecification, 
        WBFL::Reporting::ReportSpecification>(pOldRptSpec);
    if (pBearingRptSpec != nullptr)
    {
        std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pBearingRptSpecBuilder(
            std::make_shared<CBearingReportSpecificationBuilder>(m_pBroker, pBearingRptSpec->GetReactionLocation()));
        return pBearingRptSpecBuilder->CreateReportSpec(rptDesc, pOldRptSpec);
    }
    else
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        // Prompt for span, girder, and chapter list
        GET_IFACE2(GetBroker(), ISelection, pSelection);
        CSelection selection = pSelection->GetSelection();

        CGirderKey girderKey;

        if (selection.Type == CSelection::Span)
        {
            GET_IFACE2(GetBroker(), IBridge, pBridge);
            girderKey.groupIndex = pBridge->GetGirderGroupIndex(selection.SpanIdx);
            girderKey.girderIndex = 0;
        }
        else if (selection.Type == CSelection::Girder || selection.Type == CSelection::Segment || selection.Type == CSelection::ClosureJoint)
        {
            girderKey.groupIndex = (selection.GroupIdx == INVALID_INDEX ? 0 : selection.GroupIdx);
            girderKey.girderIndex = selection.GirderIdx;
        }
        else
        {
            girderKey.groupIndex = 0;
            girderKey.girderIndex = 0;
        }

        GET_IFACE2(GetBroker(), IBearingDesign, pBearingDesign);
        GET_IFACE2(GetBroker(), IIntervals, pIntervals);
        GET_IFACE2(GetBroker(), IBridge, pBridge);

        IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();

        std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, lastCompositeDeckIntervalIdx, girderKey));

        ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);
        iter.First();
        PierIndexType startPierIdx = (iter.IsDone() ? INVALID_INDEX : iter.CurrentItem().PierIdx);

        std::vector<ReactionLocation> data;

        // Use iterator to walk locations
        for (iter.First(); !iter.IsDone(); iter.Next())
        {

            const ReactionLocation& reactionLocation(iter.CurrentItem());

            data.push_back(reactionLocation);

        }

        CBearingMultiViewReportDlg dlg(GetBroker(), rptDesc, pOldRptSpec, girderKey, data[0]);

        if (dlg.DoModal() == IDOK)
        {
            std::vector<ReactionLocation> reactionLocations = dlg.GetReactionLocations();

            // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
            std::shared_ptr<CMultiViewSpanGirderBearingReportSpecification> pOldGRptSpec = std::dynamic_pointer_cast<CMultiViewSpanGirderBearingReportSpecification>(pOldRptSpec);

            std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
            if (pOldGRptSpec)
            {
                std::shared_ptr<CMultiViewSpanGirderBearingReportSpecification> pNewGRptSpec(std::make_shared<CMultiViewSpanGirderBearingReportSpecification>(*pOldGRptSpec));

                pNewGRptSpec->SetReactionLocations(reactionLocations);

                pNewRptSpec = std::static_pointer_cast<WBFL::Reporting::ReportSpecification>(pNewGRptSpec);
            }
            else
            {
                pNewRptSpec = std::make_shared<CMultiViewSpanGirderBearingReportSpecification>(rptDesc.GetReportName(), m_pBroker, reactionLocations);
            }

            std::vector<std::_tstring> chList = dlg.m_ChapterList;
            rptDesc.ConfigureReportSpecification(chList, pNewRptSpec);

            return pNewRptSpec;
        }

        return nullptr;
    }
}

//std::shared_ptr<WBFL::Reporting::ReportSpecification> CMultiViewSpanGirderBearingReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
//{
//    GET_IFACE(ISelection, pSelection);
//    CSelection selection = pSelection->GetSelection();
//
//    CGirderKey girderKey;
//
//    CBearingData2 bearing;
// 
//    if (selection.Type == CSelection::Span)
//    {
//        GET_IFACE(IBridge, pBridge);
//        girderKey.groupIndex = pBridge->GetGirderGroupIndex(selection.SpanIdx);
//        girderKey.girderIndex = 0;
//    }
//    else if (selection.Type == CSelection::Girder || selection.Type == CSelection::Segment || selection.Type == CSelection::ClosureJoint)
//    {
//        girderKey.groupIndex = selection.GroupIdx;
//        girderKey.girderIndex = selection.GirderIdx;
//    }
//    else
//    {
//        girderKey.groupIndex = 0;
//        girderKey.girderIndex = 0;
//    }
//
//    GET_IFACE(IBridge, pBridge);
//    GroupIndexType nGroups = pBridge->GetGirderGroupCount();
//    if (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex != ALL_GIRDERS && nGroups == 1)
//    {
//        // there is exactly one group and we have a valid girder number so take ALL_GROUPS to mean group 0... no prompting required
//        girderKey.groupIndex = 0;
//    }
//    else if (girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS)
//    {
//        AFX_MANAGE_STATE(AfxGetStaticModuleState());
//        // we don't have a proper girder key.... prompt the user
//        CSpanGirderReportDlg dlg(m_pBroker, rptDesc, CSpanGirderReportDlg::Mode::GroupGirderAndChapters, std::shared_ptr<WBFL::Reporting::ReportSpecification>());
//        dlg.m_SegmentKey.groupIndex = girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex;
//        dlg.m_SegmentKey.girderIndex = girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex;
//
//        if (dlg.DoModal() == IDOK)
//        {
//            girderKey = dlg.m_SegmentKey;
//        }
//        else
//        {
//            return nullptr;
//        }
//    }
//
//    std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec(std::make_shared<CBearingReportSpecification>(rptDesc.GetReportName(), m_pBroker, bearing));
//
//    rptDesc.ConfigureReportSpecification(pRptSpec);
//
//    return pRptSpec;
//}
//
//
