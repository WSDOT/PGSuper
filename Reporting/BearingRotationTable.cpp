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
#include <Reporting\BearingRotationTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CProductRotationTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CBearingRotationTable::CBearingRotationTable()
{
}

CBearingRotationTable::CBearingRotationTable(const CBearingRotationTable& rOther)
{
    MakeCopy(rOther);
}

CBearingRotationTable::~CBearingRotationTable()
{
}

//======================== OPERATORS  =======================================
CBearingRotationTable& CBearingRotationTable::operator= (const CBearingRotationTable& rOther)
{
    if (this != &rOther)
    {
        MakeAssignment(rOther);
    }

    return *this;
}







ColumnIndexType CBearingRotationTable::GetBearingTableColumnCount(IBroker* pBroker, const CGirderKey& girderKey, 
    pgsTypes::AnalysisType analysisType, bool bDesign, bool bUserLoads, TABLEPARAMETERS* tParam, bool bDetail, DuctIndexType nDucts, bool bTimeStep) const
{

    ColumnIndexType nCols = 1; // location

    if (bDetail)
    {
        nCols = 4; // location, girder, diaphragm, and traffic barrier

        if (bUserLoads)
        {
            nCols += 6;
        }

    }

    GET_IFACE2(pBroker, IProductLoads, pLoads);
    GET_IFACE2(pBroker, IBridge, pBridge);
    GET_IFACE2(pBroker, IUserDefinedLoadData, pUserLoads);

    pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

    tParam->bDeck = false;
    if (deckType != pgsTypes::sdtNone && bDetail)
    {
        tParam->bDeck = true;
        nCols += 2; // slab + slab pad
    }

    tParam->bDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);

    tParam->startGroup = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
    tParam->endGroup = (girderKey.groupIndex == ALL_GROUPS ? pBridge->GetGirderGroupCount() - 1 : tParam->startGroup);

    CGirderKey key(tParam->startGroup, girderKey.girderIndex);
    tParam->bSegments = (1 < pBridge->GetSegmentCount(key) ? true : false);
    tParam->bPedLoading = pLoads->HasPedestrianLoad(key);
    tParam->bSidewalk = pLoads->HasSidewalkLoad(key);
    tParam->bShearKey = pLoads->HasShearKeyLoad(key);
    tParam->bLongitudinalJoint = pLoads->HasLongitudinalJointLoad();
    tParam->bConstruction = !IsZero(pUserLoads->GetConstructionLoad());

    if (tParam->bSegments && bDetail)
    {
        nCols++;
    }

    if (pBridge->HasOverlay() && bDetail)
    {
        nCols++;

        if (analysisType == pgsTypes::Envelope)
            nCols++;
    }

    // determine continuity stage
    GET_IFACE2(pBroker, IIntervals, pIntervals);
    IntervalIndexType continuityIntervalIdx = MAX_INDEX;
    PierIndexType firstPierIdx = pBridge->GetGirderGroupStartPier(tParam->startGroup);
    PierIndexType lastPierIdx = pBridge->GetGirderGroupEndPier(tParam->endGroup);
    for (PierIndexType pierIdx = firstPierIdx; pierIdx <= lastPierIdx; pierIdx++)
    {
        if (pBridge->IsBoundaryPier(pierIdx))
        {
            IntervalIndexType left_interval_index, right_interval_index;
            pIntervals->GetContinuityInterval(pierIdx, &left_interval_index, &right_interval_index);
            continuityIntervalIdx = Min(continuityIntervalIdx, left_interval_index);
            continuityIntervalIdx = Min(continuityIntervalIdx, right_interval_index);
        }
    }

    IntervalIndexType firstCastDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();
    if (tParam->bDeck)
    {
        tParam->bContinuousBeforeDeckCasting = (continuityIntervalIdx <= firstCastDeckIntervalIdx) ? true : false;
    }
    else
    {
        tParam->bContinuousBeforeDeckCasting = false;
    }

    if (tParam->bConstruction && bDetail)
    {
        if (analysisType == pgsTypes::Envelope && tParam->bContinuousBeforeDeckCasting)
        {
            nCols += 2;
        }
        else
        {
            nCols++;
        }
    }

    if (tParam->bDeckPanels && bDetail)
    {
        if (analysisType == pgsTypes::Envelope && tParam->bContinuousBeforeDeckCasting)
        {
            nCols += 2;
        }
        else
        {
            nCols++;
        }
    }

    if (tParam->bDeck && analysisType == pgsTypes::Envelope && tParam->bContinuousBeforeDeckCasting && bDetail)
    {
        nCols += 2; // add one more each for min/max slab and min/max slab pad
    }

    if (analysisType == pgsTypes::Envelope && bDetail)
    {
        nCols++; // add for min/max traffic barrier
    }

    if (!bDetail)
    {
        nCols += 3;
    }

    if (bDesign)
    {
        nCols ++; // design live loads

        if (bDetail)
        {
            nCols++; // accounts for min
        }
    }

    if (tParam->bPedLoading && bDetail)
    {
        nCols += 2;
    }

    if (tParam->bSidewalk && bDetail)
    {
        if (analysisType == pgsTypes::Envelope)
        {
            nCols += 2;
        }
        else
        {
            nCols++;
        }
    }

    if (tParam->bShearKey && bDetail)
    {
        if (analysisType == pgsTypes::Envelope)
        {
            nCols += 2;
        }
        else
        {
            nCols++;
        }
    }

    if (tParam->bLongitudinalJoint && bDetail)
    {
        if (analysisType == pgsTypes::Envelope && tParam->bContinuousBeforeDeckCasting)
        {
            nCols += 2;
        }
        else
        {
            nCols++;
        }
    }

    nCols++;

    if (0 < nDucts)
    {
        nCols++;
    }

    if (bTimeStep)
    {
        nCols += 3;
    }
    else
    {
        nCols++;
    }


    return nCols;
}


template <class M, class T>
RowIndexType ConfigureBearingRotationTableHeading(IBroker* pBroker, rptRcTable* p_table, bool bPierTable, bool bSegments, bool bConstruction, bool bDeck, bool bDeckPanels, bool bSidewalk, bool bShearKey, bool bLongitudinalJoints, bool bOverlay, bool bIsFutureOverlay,
    bool bDesign, bool bUserLoads, bool bPedLoading, pgsTypes::AnalysisType analysisType, bool bContinuousBeforeDeckCasting, IEAFDisplayUnits* pDisplayUnits, const T& unitT, bool bDetail, DuctIndexType nDucts, bool bTimeStep)
{
    p_table->SetNumberOfHeaderRows(2);

    GET_IFACE2(pBroker, IProductLoads, pProductLoads);

    //
    // Set up table headings
    //
    ColumnIndexType col = 0;

    p_table->SetRowSpan(0, col, 2);
    if (bPierTable)
    {
        (*p_table)(0, col++) << _T("");
    }
    else
    {
        (*p_table)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
    }

    if (bSegments && bDetail)
    {
        p_table->SetRowSpan(0, col, 2);
        (*p_table)(0, col++) << COLHDR(_T("Erected") << rptNewLine << _T("Segments"), M, unitT);
    }

    
    if (bDetail)
    {
        p_table->SetRowSpan(0, col, 2);
        (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftGirder), M, unitT);

        p_table->SetRowSpan(0, col, 2);
        (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftDiaphragm), M, unitT);
    }


    if (bShearKey && bDetail)
    {
        if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
        {
            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftShearKey);
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
        }
        else
        {
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftShearKey), M, unitT);
        }
    }

    if (bLongitudinalJoints && bDetail)
    {
        if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
        {
            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftLongitudinalJoint);
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
        }
        else
        {
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftLongitudinalJoint), M, unitT);
        }
    }

    if (bConstruction && bDetail)
    {
        if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
        {
            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftConstruction);
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
        }
        else
        {
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftConstruction), M, unitT);
        }
    }

    if (bDeck && bDetail)
    {
        if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
        {
            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftSlab);
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);

            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad);
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
        }
        else
        {
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftSlab), M, unitT);

            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad), M, unitT);
        }

    }

    if (bDeckPanels && bDetail)
    {
        if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
        {
            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPanel);
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
        }
        else
        {
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftSlabPanel), M, unitT);
        }
    }

    if (analysisType == pgsTypes::Envelope)
    {
        if (bSidewalk && bDetail)
        {
            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk);
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
        }

        if (bDetail)
        {
            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier);
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
        }


        if (bOverlay && bDetail)
        {
            p_table->SetColumnSpan(0, col, 2);
            if (bIsFutureOverlay)
            {
                (*p_table)(0, col) << _T("Future") << rptNewLine << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
            }
            else
            {
                (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
            }
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
        }

    }
    else
    {
        if (bSidewalk && bDetail)
        {
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk), M, unitT);
        }

        p_table->SetRowSpan(0, col, 2);
        (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier), M, unitT);

        if (bOverlay && bDetail)
        {
            p_table->SetRowSpan(0, col, 2);
            if (bIsFutureOverlay)
            {
                (*p_table)(0, col++) << COLHDR(_T("Future") << rptNewLine << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay), M, unitT);
            }
            else
            {
                (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftOverlay), M, unitT);
            }
        }
    }

    if (bUserLoads && bDetail)
    {
        p_table->SetColumnSpan(0, col, 2);
        (*p_table)(0, col) << Sub2(symbol(theta), _T("User DC"));
        (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
        (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);

        p_table->SetColumnSpan(0, col, 2);
        (*p_table)(0, col) << Sub2(symbol(theta), _T("User DW"));
        (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
        (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);

    }

    if (bPedLoading && bDetail)
    {
        p_table->SetColumnSpan(0, col, 2);
        (*p_table)(0, col) << _T("$ Pedestrian");
        (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
        (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
    }

    if (!bDetail)
    {
            (*p_table)(0, col) << Sub2(symbol(theta), _T("DC"));
            (*p_table)(1, col++) << COLHDR(_T("Max"), rptAngleUnitTag, unitT);
            (*p_table)(0, col) << Sub2(symbol(theta), _T("DW"));
            (*p_table)(1, col++) << COLHDR(_T("Max"), rptAngleUnitTag, unitT);

    }


    if (bDesign)
    {
        if (!bDetail)
        {
            (*p_table)(0, col) << Sub2(symbol(theta), _T("LL")); ////////////add to total LL...
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
        }
        else
        {
            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << Sub2(symbol(theta), _T("Design LL"));
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);

            if (bUserLoads)
            {
                p_table->SetColumnSpan(0, col, 2);
                (*p_table)(0, col) << Sub2(symbol(theta), _T("User LL+IM"));
                (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
                (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
            }




        }
    }

    if (!bDetail)
    {
            (*p_table)(0, col) << Sub2(symbol(theta), _T("Service I"));
            (*p_table)(1, col++) << COLHDR(_T("Max"), rptAngleUnitTag, unitT);
    }

    p_table->SetRowSpan(0, col, 2);


    (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftPretension), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());


    p_table->SetRowSpan(0, col, 2);

    if (0 < nDucts)
    {
        (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftPostTensioning), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
    }

    p_table->SetRowSpan(0, col, 2);

    if (bTimeStep)
    {
        (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftCreep), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
        (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftShrinkage), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
        (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftRelaxation), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
    }
    else
    {
        (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftCreep), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
    }



    return p_table->GetNumberOfHeaderRows(); // index of first row to report data
}




//======================== OPERATIONS =======================================
rptRcTable* CBearingRotationTable::BuildBearingRotationTable(IBroker* pBroker, const CGirderKey& girderKey, pgsTypes::AnalysisType analysisType,
    bool bIncludeImpact, bool bIncludeLLDF, bool bDesign, bool bUserLoads, bool bIndicateControllingLoad, IEAFDisplayUnits* pDisplayUnits, bool bDetail, bool isFlexural) const
{

    // Build table
    INIT_UV_PROTOTYPE(rptAngleUnitValue, rotation, pDisplayUnits->GetRadAngleUnit(), false);

    GET_IFACE2(pBroker, IBridge, pBridge);
    bool bHasOverlay = pBridge->HasOverlay();
    bool bFutureOverlay = pBridge->IsFutureOverlay();
    PierIndexType nPiers = pBridge->GetPierCount();

    GET_IFACE2(pBroker, IIntervals, pIntervals);
    IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
    IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;


    GET_IFACE2(pBroker, IPointOfInterest, pPOI);

    TABLEPARAMETERS tParam;


    GET_IFACE2_NOCHECK(pBroker, ICamber, pCamber);

    GET_IFACE2(pBroker, IGirderTendonGeometry, pTendonGeom);
    DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);

    GET_IFACE2(pBroker, ILossParameters, pLossParams);
    bool bTimeStep = (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP ? true : false);

    ColumnIndexType nCols = GetBearingTableColumnCount(pBroker, girderKey, analysisType, bDesign, bUserLoads, &tParam, bDetail, nDucts, bTimeStep);


    CString label = _T("Flexural Rotations");
    if (!isFlexural)
    {
        label = _T("Torsional Rotations");
    }
    rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, label);
    RowIndexType row = ConfigureBearingRotationTableHeading<rptAngleUnitTag, WBFL::Units::AngleData>(
        pBroker, p_table, true, tParam.bSegments, tParam.bConstruction, tParam.bDeck, tParam.bDeckPanels, 
        tParam.bSidewalk, tParam.bShearKey, tParam.bLongitudinalJoint, bHasOverlay, 
        bFutureOverlay, bDesign, bUserLoads, tParam.bPedLoading, analysisType, tParam.bContinuousBeforeDeckCasting, 
        pDisplayUnits, pDisplayUnits->GetRadAngleUnit(), bDetail, nDucts, bTimeStep);

    p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
    p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));


    // get poi where pier rotations occur
    PoiList vPoi;
    std::vector<CGirderKey> vGirderKeys;
    pBridge->GetGirderline(girderKey.girderIndex, tParam.startGroup, tParam.endGroup, &vGirderKeys);
    for (const auto& thisGirderKey : vGirderKeys)
    {
        PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(thisGirderKey.groupIndex);
        PierIndexType endPierIdx = pBridge->GetGirderGroupEndPier(thisGirderKey.groupIndex);
        for (PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++)
        {
            if (pierIdx == startPierIdx)
            {
                CSegmentKey segmentKey(thisGirderKey, 0);
                PoiList segPoi;
                pPOI->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &segPoi);
                vPoi.push_back(segPoi.front());
            }
            else if (pierIdx == endPierIdx)
            {
                SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
                CSegmentKey segmentKey(thisGirderKey, nSegments - 1);
                PoiList segPoi;
                pPOI->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &segPoi);
                vPoi.push_back(segPoi.front());
            }
            else
            {
                Float64 Xgp;
                VERIFY(pBridge->GetPierLocation(thisGirderKey, pierIdx, &Xgp));
                pgsPointOfInterest poi = pPOI->ConvertGirderPathCoordinateToPoi(thisGirderKey, Xgp);
                vPoi.push_back(poi);
            }
        }
    }

    GET_IFACE2(pBroker, IBearingDesign, pBearingDesign);
    IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
    std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, lastCompositeDeckIntervalIdx, girderKey));

    // Fill up the table
    GET_IFACE2(pBroker, IProductForces, pProductForces);
    pgsTypes::BridgeAnalysisType maxBAT = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);
    pgsTypes::BridgeAnalysisType minBAT = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Minimize);


    ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);
    iter.First();
    PierIndexType startPierIdx = (iter.IsDone() ? INVALID_INDEX : iter.CurrentItem().PierIdx);

    ReactionTableType tableType = BearingReactionsTable;

    // Build table
    INIT_UV_PROTOTYPE(rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false);
    INIT_UV_PROTOTYPE(rptForceUnitValue, reactu, pDisplayUnits->GetShearUnit(), false);

    // TRICKY:
    // Use the adapter class to get the reaction response functions we need and to iterate piers
    ReactionUnitValueTool reaction(tableType, reactu);


    GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);


    

    // Use iterator to walk locations
    for (iter.First(); !iter.IsDone(); iter.Next())
    {
        ColumnIndexType col = 0;

        const ReactionLocation& reactionLocation(iter.CurrentItem());
        const CGirderKey& thisGirderKey(reactionLocation.GirderKey);

        (*p_table)(row, col++) << reactionLocation.PierLabel;

        ReactionDecider reactionDecider(BearingReactionsTable, reactionLocation, thisGirderKey, pBridge, pIntervals);

        const pgsPointOfInterest& poi = vPoi[reactionLocation.PierIdx - startPierIdx];

        const CSegmentKey& segmentKey(poi.GetSegmentKey());
        const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
        IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

        IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());


        CComPtr<IAngle> pSkew;
        pBridge->GetPierSkew(reactionLocation.PierIdx, &pSkew);
        Float64 skew;
        pSkew->get_Value(&skew);

        Float64 skewFactor;

        if (isFlexural)
        {
            skewFactor = 1.0;
        }
        else
        {
            skewFactor = tan(skew);
        }


        if (bDetail)
        {
            if (tParam.bSegments)
            {
                (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, maxBAT, rtCumulative, false));
                (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftGirder, poi, maxBAT, rtCumulative, false));
            }
            else
            {
                (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, maxBAT, rtCumulative, false));
            }

            if (reactionDecider.DoReport(lastIntervalIdx))
            {
                (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftDiaphragm, poi, maxBAT, rtCumulative, false));
            }
            else
            {
                (*p_table)(row, col++) << RPT_NA;
            }

            if (tParam.bShearKey)
            {
                if (analysisType == pgsTypes::Envelope)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftShearKey, poi, maxBAT, rtCumulative, false));
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftShearKey, poi, minBAT, rtCumulative, false));
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
                else
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftShearKey, poi, maxBAT, rtCumulative, false));
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
            }


            if (tParam.bLongitudinalJoint)
            {
                if (analysisType == pgsTypes::Envelope && tParam.bContinuousBeforeDeckCasting)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, maxBAT, rtCumulative, false));
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, minBAT, rtCumulative, false));
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
                else
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, maxBAT, rtCumulative, false));
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
            }

            if (tParam.bConstruction)
            {
                if (analysisType == pgsTypes::Envelope && tParam.bContinuousBeforeDeckCasting)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftConstruction, poi, maxBAT, rtCumulative, false));
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftConstruction, poi, minBAT, rtCumulative, false));
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
                else
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftConstruction, poi, maxBAT, rtCumulative, false));
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
            }

            if (tParam.bDeck)
            {
                if (analysisType == pgsTypes::Envelope && tParam.bContinuousBeforeDeckCasting)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlab, poi, maxBAT, rtCumulative, false));
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlab, poi, minBAT, rtCumulative, false));

                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPad, poi, maxBAT, rtCumulative, false));
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPad, poi, minBAT, rtCumulative, false));
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;

                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
                else
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlab, poi, maxBAT, rtCumulative, false));
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPad, poi, maxBAT, rtCumulative, false));
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
            }

            if (tParam.bDeckPanels)
            {
                if (analysisType == pgsTypes::Envelope && tParam.bContinuousBeforeDeckCasting)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPanel, poi, maxBAT, rtCumulative, false));
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPanel, poi, minBAT, rtCumulative, false));
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
                else
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPanel, poi, maxBAT, rtCumulative, false));
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
            }
        }

        if (analysisType == pgsTypes::Envelope)
        {
            if (bDetail)
            {
                if (tParam.bSidewalk)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSidewalk, poi, maxBAT, rtCumulative, false));
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSidewalk, poi, minBAT, rtCumulative, false));
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }

                if (reactionDecider.DoReport(lastIntervalIdx))
                {
                    (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftTrafficBarrier, poi, maxBAT, rtCumulative, false));
                    (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftTrafficBarrier, poi, minBAT, rtCumulative, false));
                }
                else
                {
                    (*p_table)(row, col++) << RPT_NA;
                    (*p_table)(row, col++) << RPT_NA;
                }

                if (bHasOverlay)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, false && !bDesign ? pgsTypes::pftOverlayRating : pgsTypes::pftOverlay, poi, maxBAT, rtCumulative, false));
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, false && !bDesign ? pgsTypes::pftOverlayRating : pgsTypes::pftOverlay, poi, minBAT, rtCumulative, false));
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
            }




            std::unique_ptr<ICmbLsReactionAdapter> pForces;
            GET_IFACE2(pBroker, IBearingDesign, pBearingDesign);
            pForces = std::make_unique<CmbLsBearingDesignReactionAdapter>(pBearingDesign, lastIntervalIdx, girderKey);

            



            Float64 pMin, pMax;
            


            if (!bDetail)
            {
                GET_IFACE2(pBroker, ICombinedForces, comboForces);



                if (!isFlexural)
                {


                    Float64 flexural_rotation_max_DC = comboForces->GetRotation(lastIntervalIdx, lcDC, poi, maxBAT, rtCumulative);
                    Float64 torsional_rotation_max_DC = flexural_rotation_max_DC * tan(skew);
                    (*p_table)(row, col++) << rotation.SetValue(torsional_rotation_max_DC);

                    Float64 flexural_rotation_max_DW = comboForces->GetRotation(lastIntervalIdx, lcDW, poi, maxBAT, rtCumulative);
                    Float64 torsional_rotation_max_DW = flexural_rotation_max_DW * tan(skew);
                    (*p_table)(row, col++) << rotation.SetValue(torsional_rotation_max_DW);

                }
                else
                {
                    Float64 flexural_rotation_max_DC = comboForces->GetRotation(lastIntervalIdx, lcDC, poi, maxBAT, rtCumulative);
                    (*p_table)(row, col++) << rotation.SetValue(flexural_rotation_max_DC);

                    Float64 flexural_rotation_max_DW = comboForces->GetRotation(lastIntervalIdx, lcDW, poi, maxBAT, rtCumulative);
                    (*p_table)(row, col++) << rotation.SetValue(flexural_rotation_max_DW);




                }
                
            }

            if (bDetail && bUserLoads)
            {
                (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDC, poi, maxBAT, rtCumulative, false));
                (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDC, poi, minBAT, rtCumulative, false));
                (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDW, poi, maxBAT, rtCumulative, false));
                (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDW, poi, minBAT, rtCumulative, false));
            }



            Float64 min, max;
            VehicleIndexType minConfig, maxConfig;
            if (bDesign)
            {


                if (reactionDecider.DoReport(lastIntervalIdx))
                {
                    pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltDesign, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
                    (*p_table)(row, col) << rotation.SetValue(skewFactor*max);
                    if (bIndicateControllingLoad && 0 <= maxConfig)
                    {
                        (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
                    }
                    col++;

                    if (bDetail)
                    {
                        pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltDesign, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
                        (*p_table)(row, col) << rotation.SetValue(skewFactor*min);
                        if (bIndicateControllingLoad && 0 <= minConfig)
                        {
                            (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << _T(")");
                        }
                        col++;
                        if (bUserLoads)
                        {
                            (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserLLIM, poi, maxBAT, rtCumulative, false));
                            (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserLLIM, poi, minBAT, rtCumulative, false));
                        }
                   }


                }
                else
                {
                    (*p_table)(row, col++) << RPT_NA;
                    (*p_table)(row, col++) << RPT_NA;

                }

            }


            if (!bDetail)
            {
                GET_IFACE2(pBroker, ILimitStateForces, limitForces);
                limitForces->GetRotation(
                    lastIntervalIdx, pgsTypes::ServiceI, poi, maxBAT, true,
                    true, true, true, true, &pMin, &pMax);
                    (*p_table)(row, col++) << rotation.SetValue(skewFactor*pMax);
            }


        }
        else
        {
            Float64 min, max;
            VehicleIndexType minConfig, maxConfig;
            if (bDetail)
            {
                if (tParam.bSidewalk)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSidewalk, poi, maxBAT, rtCumulative, false));
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }

                if (reactionDecider.DoReport(lastIntervalIdx))
                {
                    (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftTrafficBarrier, poi, maxBAT, rtCumulative, false));
                }
                else
                {
                    (*p_table)(row, col++) << RPT_NA;
                }

                if (bHasOverlay)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, false && !bDesign ? pgsTypes::pftOverlayRating : pgsTypes::pftOverlay, poi, maxBAT, rtCumulative, false));
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }

                if (tParam.bPedLoading)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPedestrian, poi, maxBAT, bIncludeImpact, true, &min, &max);
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*max);
                        (*p_table)(row, col++) << rotation.SetValue(skewFactor*min);
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
            }


            if (bDesign)
            {
                if (reactionDecider.DoReport(lastIntervalIdx))
                {
                    pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltDesign, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
                    (*p_table)(row, col) << rotation.SetValue(skewFactor*max);
                    if (bIndicateControllingLoad && 0 <= maxConfig)
                    {
                        (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
                    }
                    col++;

                    (*p_table)(row, col) << rotation.SetValue(skewFactor*min);
                    if (bIndicateControllingLoad && 0 <= minConfig)
                    {
                        (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << _T(")");
                    }
                    col++;
                }
                else
                {
                    (*p_table)(row, col++) << RPT_NA;
                    (*p_table)(row, col++) << RPT_NA;
                }

            }

        }


        (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(releaseIntervalIdx, pgsTypes::pftPretension, poi, maxBAT, rtCumulative, false));
        if (0 < nDucts)
        {
            (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftPostTensioning, poi, maxBAT, rtCumulative, false));
        }

        if (bTimeStep)
        {
            (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftCreep, poi, maxBAT, rtCumulative, false));
            (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftShrinkage, poi, maxBAT, rtCumulative, false));
            (*p_table)(row, col++) << rotation.SetValue(skewFactor*pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftRelaxation, poi, maxBAT, rtCumulative, false));
        }
        else
        {
            Float64 Dcreep1, Rcreep1;
            pCamber->GetCreepDeflection(poi, ICamber::cpReleaseToDeck, pgsTypes::CreepTime::Max, pgsTypes::pddErected, nullptr, &Dcreep1, &Rcreep1);

            (*p_table)(row, col++) << rotation.SetValue(skewFactor*Rcreep1 /*+ Rcreep2*/);
        }


        row++;
    }

    return p_table;

}






//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CBearingRotationTable::MakeCopy(const CBearingRotationTable& rOther)
{
    // Add copy code here...
}

void CBearingRotationTable::MakeAssignment(const CBearingRotationTable& rOther)
{
    MakeCopy(rOther);
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
