/////////////////////////////////////////////////////////////////////////
//// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
//// Copyright © 1999-2024  Washington State Department of Transportation
////                        Bridge and Structures Office
////
//// This program is free software; you can redistribute it and/or modify
//// it under the terms of the Alternate Route Open Source License as 
//// published by the Washington State Department of Transportation, 
//// Bridge and Structures Office.
////
//// This program is distributed in the hope that it will be useful, but 
//// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
//// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
//// the Alternate Route Open Source License for more details.
////
//// You should have received a copy of the Alternate Route Open Source 
//// License along with this program; if not, write to the Washington 
//// State Department of Transportation, Bridge and Structures Office, 
//// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
//// Bridge_Support@wsdot.wa.gov
/////////////////////////////////////////////////////////////////////////
//
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
//
///****************************************************************************
//CLASS
//   CProductRotationTable
//****************************************************************************/
//
//
//////////////////////////// PUBLIC     ///////////////////////////////////////
//
////======================== LIFECYCLE  =======================================
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
    pgsTypes::AnalysisType analysisType, bool bDesign, bool bUserLoads, ROTATIONDETAILS* pDetails, bool bDetail) const
{

    ColumnIndexType nCols = 1; // location


    if (bDetail)
    {
        nCols += 3; // girder, diaphragm, and traffic barrier

        if (bUserLoads)
        {
            if (analysisType == pgsTypes::Envelope)
            {
                nCols += 6; // user loads for DC DW and LL
            }
            else
            {
                nCols += 3;
            }
            
        }

    }


    GET_IFACE2(pBroker, IBridge, pBridge);
    GET_IFACE2(pBroker, IUserDefinedLoadData, pUserLoads);

    if (pDetails->bDeck && bDetail)
    {
        nCols += 2; // slab + slab pad
    }

    
    pDetails->bConstruction = !IsZero(pUserLoads->GetConstructionLoad());

    if (pDetails->bSegments && bDetail)
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
    PierIndexType firstPierIdx = pBridge->GetGirderGroupStartPier(pDetails->startGroup);
    PierIndexType lastPierIdx = pBridge->GetGirderGroupEndPier(pDetails->endGroup);
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
    if (pDetails->bDeck)
    {
        pDetails->bContinuousBeforeDeckCasting = (continuityIntervalIdx <= firstCastDeckIntervalIdx) ? true : false;
    }
    else
    {
        pDetails->bContinuousBeforeDeckCasting = false;
    }

    if (pDetails->bConstruction && bDetail)
    {
        if (analysisType == pgsTypes::Envelope && pDetails->bContinuousBeforeDeckCasting)
        {
            nCols += 2;
        }
        else
        {
            nCols++;
        }
    }

    if (pDetails->bDeckPanels && bDetail)
    {
        if (analysisType == pgsTypes::Envelope && pDetails->bContinuousBeforeDeckCasting)
        {
            nCols += 2;
        }
        else
        {
            nCols++;
        }
    }

    if (pDetails->bDeck && analysisType == pgsTypes::Envelope && pDetails->bContinuousBeforeDeckCasting && bDetail)
    {
        nCols += 2; // add one more each for min/max slab and min/max slab pad
    }

    if (analysisType == pgsTypes::Envelope && bDetail)
    {
        nCols++; // add for min/max traffic barrier
    }

    if (!bDetail)
    {
        nCols ++;
    }

    if (bDesign)
    {
        nCols ++; // design live loads

        if (bDetail)
        {
            nCols++; // accounts for min
        }
    }

    if (pDetails->bPedLoading && bDetail)
    {
        nCols += 2;
    }

    if (pDetails->bSidewalk && bDetail)
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

    if (pDetails->bShearKey && bDetail)
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

    if (pDetails->bLongitudinalJoint && bDetail)
    {
        if (analysisType == pgsTypes::Envelope && pDetails->bContinuousBeforeDeckCasting)
        {
            nCols += 2;
        }
        else
        {
            nCols++;
        }
    }




    if (0 < pDetails->nDucts && bDetail)
    {
        nCols++;  //add for post-tensioning
    }
                
    if (pDetails->bTimeStep && bDetail)
    {

        nCols += 4;
     
    }
    else
    {
        if (bDetail)
        {
            nCols += 2;
        }
        else
        {
            nCols ++;
        }
        
    }


    return nCols;
}


template <class M, class T>
RowIndexType ConfigureBearingRotationTableHeading(IBroker* pBroker, rptRcTable* p_table,
    bool bDesign, bool bUserLoads, pgsTypes::AnalysisType analysisType, IEAFDisplayUnits* pDisplayUnits, 
    const T& unitT, bool bDetail, ROTATIONDETAILS* pDetails)
{
    if (bDetail)
    {
        p_table->SetNumberOfHeaderRows(2);
    }

    ColumnIndexType col = 0;

    if (bDetail)
    {
        p_table->SetRowSpan(0, col, 2);
    }
    

    (*p_table)(0, col++) << _T("");
    


    if (pDetails->bSegments && bDetail)
    {
        p_table->SetRowSpan(0, col, 2);
        (*p_table)(0, col++) << COLHDR(_T("Erected") << rptNewLine << _T("Segments"), M, unitT);
    }

    
    if (bDetail)
    {

        GET_IFACE2(pBroker, IProductLoads, pProductLoads);


        p_table->SetRowSpan(0, col, 2);
        (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftGirder), M, unitT);

        p_table->SetRowSpan(0, col, 2);
        (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftDiaphragm), M, unitT);
    }


    if (pDetails->bShearKey && bDetail)
    {

        GET_IFACE2(pBroker, IProductLoads, pProductLoads);


        if (analysisType == pgsTypes::Envelope && pDetails->bContinuousBeforeDeckCasting)
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

    if (pDetails->bLongitudinalJoint && bDetail)
    {

        GET_IFACE2(pBroker, IProductLoads, pProductLoads);

        if (analysisType == pgsTypes::Envelope && pDetails->bContinuousBeforeDeckCasting)
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

    if (pDetails->bConstruction && bDetail)
    {

        GET_IFACE2(pBroker, IProductLoads, pProductLoads);

        if (analysisType == pgsTypes::Envelope && pDetails->bContinuousBeforeDeckCasting)
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

    if (pDetails->bDeck && bDetail)
    {

        GET_IFACE2(pBroker, IProductLoads, pProductLoads);

        if (analysisType == pgsTypes::Envelope && pDetails->bContinuousBeforeDeckCasting)
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

    if (pDetails->bDeckPanels && bDetail)
    {

        GET_IFACE2(pBroker, IProductLoads, pProductLoads);

        if (analysisType == pgsTypes::Envelope && pDetails->bContinuousBeforeDeckCasting)
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



        if (pDetails->bSidewalk && bDetail)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);


            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk);
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
        }

        if (bDetail)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);

            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier);
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
        }


        if (pDetails->bHasOverlay && bDetail)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);

            p_table->SetColumnSpan(0, col, 2);
            if (pDetails->bFutureOverlay)
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

        if (bUserLoads && bDetail)
        {
            GET_IFACE2(pBroker, IProductLoads, pProductLoads);

            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftUserDC);
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftUserDW);
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
            if (bDesign)
            {
                GET_IFACE2(pBroker, IProductLoads, pProductLoads);
                p_table->SetColumnSpan(0, col, 2);
                (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftUserLLIM);
                (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
                (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
            }

        }

    }
    else
    {
        if (pDetails->bSidewalk && bDetail)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);

            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk), M, unitT);
        }

        if (bDetail)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);

            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier), M, unitT);
        }


        if (pDetails->bHasOverlay && bDetail)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);

            p_table->SetRowSpan(0, col, 2);
            if (pDetails->bFutureOverlay)
            {
                (*p_table)(0, col++) << COLHDR(_T("Future") << rptNewLine << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay), M, unitT);
            }
            else
            {
                (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftOverlay), M, unitT);
            }
        }

        if (bUserLoads && bDetail)
        {
            GET_IFACE2(pBroker, IProductLoads, pProductLoads);
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftUserDC), M, unitT);
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftUserDW), M, unitT);
            if (bDesign)
            {
                p_table->SetRowSpan(0, col, 2);
                (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftUserLLIM), M, unitT);
            }
 
        }

    }



    if (pDetails->bPedLoading && bDetail)
    {
        p_table->SetColumnSpan(0, col, 2);
        (*p_table)(0, col) << _T("Pedestrian");
        (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
        (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
    }

    if (!bDetail)
    {
            (*p_table)(0, col++) << COLHDR(Sub2(symbol(theta), _T("s-st*")), 
                    rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
    }


    if (bDesign)
    {
        if (!bDetail)
        {
            (*p_table)(0, col++) << COLHDR(Sub2(symbol(theta), _T("s-cy")), 
                    rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
        }
        else
        {
            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << Sub2(symbol(theta), _T("Design LL")) << _T("*");
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);

        }
    }

    if (!bDetail)
    {
            (*p_table)(0, col++) << COLHDR(Sub2(symbol(theta), _T("s") << _T("**")),
                    rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
    }

    

    if (bDetail)
    {
        p_table->SetRowSpan(0, col, 2);

        GET_IFACE2(pBroker, IProductLoads, pProductLoads);


        (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftPretension), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
    
   
        p_table->SetRowSpan(0, col, 2);

        if (0 < pDetails->nDucts)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);

            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftPostTensioning), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
        }
    }


    

    if (pDetails->bTimeStep)
    {
        if (bDetail)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);


            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftCreep), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftShrinkage), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftRelaxation), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
        }

    }
    else
    {
        if (bDetail)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);


            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftCreep), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
        }
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

    GET_IFACE2(pBroker, IIntervals, pIntervals);
    IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
    IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

    GET_IFACE2(pBroker, IPointOfInterest, pPOI);

    GET_IFACE2(pBroker, IBearingDesignParameters, pBearingDesignParameters);
    ROTATIONDETAILS details;

    pBearingDesignParameters->GetBearingTableParameters(girderKey, &details);

    ColumnIndexType nCols = GetBearingTableColumnCount(pBroker, girderKey, analysisType, bDesign, bUserLoads, &details, bDetail);

    CString label = _T("Flexural Rotations");
    if (!isFlexural)
    {
        label = _T("Torsional Rotations**");
    }


    rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, label);
    RowIndexType row = ConfigureBearingRotationTableHeading<rptAngleUnitTag, WBFL::Units::AngleData>(
        pBroker, p_table, bDesign, bUserLoads, analysisType, pDisplayUnits, pDisplayUnits->GetRadAngleUnit(), 
        bDetail, &details);

    p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
    p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));


    // get poi where pier rotations occur
    PoiList vPoi;
    std::vector<CGirderKey> vGirderKeys;
    pBridge->GetGirderline(girderKey.girderIndex, details.startGroup, details.endGroup, &vGirderKeys);
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

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


        pBearingDesignParameters->GetBearingRotationDetails(analysisType, poi, reactionLocation, girderKey, bIncludeImpact, bIncludeLLDF, isFlexural, &details);


        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        

        if (bDetail)
        {
            if (details.bSegments)
            {
                (*p_table)(row, col++) << rotation.SetValue(details.erectedSegmentRotation);
                (*p_table)(row, col++) << rotation.SetValue(details.maxGirderRotation);
            }
            else
            {
                (*p_table)(row, col++) << rotation.SetValue(details.erectedSegmentRotation);
            }

            if (reactionDecider.DoReport(lastIntervalIdx))
            {
                (*p_table)(row, col++) << rotation.SetValue(details.diaphragmRotation);
            }
            else
            {
                (*p_table)(row, col++) << RPT_NA;
            }

            if (details.bShearKey)
            {
                if (analysisType == pgsTypes::Envelope)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(details.maxShearKeyRotation);
                        (*p_table)(row, col++) << rotation.SetValue(details.minShearKeyRotation);
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
                        (*p_table)(row, col++) << rotation.SetValue(details.maxShearKeyRotation);
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
            }


            if (details.bLongitudinalJoint)
            {
                if (analysisType == pgsTypes::Envelope && details.bContinuousBeforeDeckCasting)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(details.maxLongitudinalJointRotation);
                        (*p_table)(row, col++) << rotation.SetValue(details.minLongitudinalJointRotation);
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
                        (*p_table)(row, col++) << rotation.SetValue(details.maxLongitudinalJointRotation);
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
            }

            if (details.bConstruction)
            {
                if (analysisType == pgsTypes::Envelope && details.bContinuousBeforeDeckCasting)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(details.maxConstructionRotation);
                        (*p_table)(row, col++) << rotation.SetValue(details.minConstructionRotation);
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
                        (*p_table)(row, col++) << rotation.SetValue(details.maxConstructionRotation);
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
            }



            if (details.bDeck)
            {
                if (analysisType == pgsTypes::Envelope && details.bContinuousBeforeDeckCasting)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(details.maxSlabRotation);
                        (*p_table)(row, col++) << rotation.SetValue(details.minSlabRotation);

                        (*p_table)(row, col++) << rotation.SetValue(details.maxHaunchRotation);
                        (*p_table)(row, col++) << rotation.SetValue(details.minHaunchRotation);
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
                        (*p_table)(row, col++) << rotation.SetValue(details.maxSlabRotation);
                        (*p_table)(row, col++) << rotation.SetValue(details.maxHaunchRotation);
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
            }

            if (details.bDeckPanels)
            {
                if (analysisType == pgsTypes::Envelope && details.bContinuousBeforeDeckCasting)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(details.maxSlabPanelRotation);
                        (*p_table)(row, col++) << rotation.SetValue(details.minSlabPanelRotation);
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
                        (*p_table)(row, col++) << rotation.SetValue(details.maxSlabPanelRotation);
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }
            }
        }

        std::unique_ptr<ICmbLsReactionAdapter> pForces;
        GET_IFACE2(pBroker, IBearingDesign, pBearingDesign);
        pForces = std::make_unique<CmbLsBearingDesignReactionAdapter>(pBearingDesign, lastIntervalIdx, girderKey);



        if (analysisType == pgsTypes::Envelope)
        {
            if (bDetail)
            {
                if (details.bSidewalk)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(details.maxSidewalkRotation);
                        (*p_table)(row, col++) << rotation.SetValue(details.minSidewalkRotation);
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }

                if (reactionDecider.DoReport(lastIntervalIdx))
                {
                    (*p_table)(row, col++) << rotation.SetValue(details.maxRailingSystemRotation);
                    (*p_table)(row, col++) << rotation.SetValue(details.minRailingSystemRotation);
                }
                else
                {
                    (*p_table)(row, col++) << RPT_NA;
                    (*p_table)(row, col++) << RPT_NA;
                }

                if (details.bHasOverlay)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(details.maxFutureOverlayRotation);
                        (*p_table)(row, col++) << rotation.SetValue(details.minFutureOverlayRotation);
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }



                if (bUserLoads)
                {
                    (*p_table)(row, col++) << rotation.SetValue(details.maxUserDCRotation);
                    (*p_table)(row, col++) << rotation.SetValue(details.minUserDCRotation);
                    (*p_table)(row, col++) << rotation.SetValue(details.maxUserDWRotation);
                    (*p_table)(row, col++) << rotation.SetValue(details.minUserDWRotation);
                }


                if (bDesign && bUserLoads)
                {
                    (*p_table)(row, col++) << rotation.SetValue(details.maxUserLLrotation);
                    (*p_table)(row, col++) << rotation.SetValue(details.minUserLLrotation);
                }

                if (details.bPedLoading)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(details.maxPedRotation);
                        (*p_table)(row, col++) << rotation.SetValue(details.minPedRotation);
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

                    if (bDetail)
                    {
                        (*p_table)(row, col) << rotation.SetValue(details.maxDesignLLrotation);
                        if (bIndicateControllingLoad && 0 <= details.maxConfigRotation)
                        {
                            (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << details.maxConfigRotation << _T(")");
                        }
                        col++;

                        (*p_table)(row, col) << rotation.SetValue(details.minDesignLLrotation);
                        if (bIndicateControllingLoad && 0 <= details.minConfigRotation)
                        {
                            (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << details.minConfigRotation << _T(")");
                        }
                        col++;
                    }



                }
                else
                {
                    (*p_table)(row, col++) << RPT_NA;
                    (*p_table)(row, col++) << RPT_NA;

                }

            }

        }
        else
        {
            if (bDetail)
            {
                if (details.bSidewalk)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(details.maxSidewalkRotation);
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }

                if (reactionDecider.DoReport(lastIntervalIdx))
                {
                    (*p_table)(row, col++) << rotation.SetValue(details.maxRailingSystemRotation);
                }
                else
                {
                    (*p_table)(row, col++) << RPT_NA;
                }

                if (details.bHasOverlay)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(details.maxFutureOverlayRotation);
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }


                if (bUserLoads)
                {
                    (*p_table)(row, col++) << rotation.SetValue(details.maxUserDCRotation);
                    (*p_table)(row, col++) << rotation.SetValue(details.maxUserDWRotation);

                    if (bDesign)
                    {
                        (*p_table)(row, col++) << rotation.SetValue(details.maxUserLLrotation);
                    }
                }


                if (details.bPedLoading)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << rotation.SetValue(details.maxPedRotation);
                        (*p_table)(row, col++) << rotation.SetValue(details.minPedRotation);
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
                    (*p_table)(row, col) << rotation.SetValue(details.maxDesignLLrotation);
                    if (bIndicateControllingLoad && 0 <= details.maxConfigRotation)
                    {
                        (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << details.maxConfigRotation << _T(")");
                    }
                    col++;

                    if (bDetail)
                    {
                        (*p_table)(row, col) << rotation.SetValue(details.minDesignLLrotation);
                    }
                    
                    if (bIndicateControllingLoad && 0 <= details.minConfigRotation && bDetail)
                    {
                        (*p_table)(row, col++) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << details.minConfigRotation << _T(")");
                    }


                }
                else
                {
                    (*p_table)(row, col++) << RPT_NA;
                    (*p_table)(row, col++) << RPT_NA;
                }

            }

        }

        if (bDetail)
        {
            (*p_table)(row, col++) << rotation.SetValue(details.preTensionRotation);
            if (0 < details.nDucts)
            {
                (*p_table)(row, col++) << rotation.SetValue(details.postTensionRotation);
            }
            (*p_table)(row, col++) << rotation.SetValue(details.creepRotation);
            if (details.bTimeStep)
            {
                (*p_table)(row, col++) << rotation.SetValue(details.shrinkageRotation);
                (*p_table)(row, col++) << rotation.SetValue(details.relaxationRotation);
            }

        }
        else
        {
            (*p_table)(row, col++) << rotation.SetValue(details.staticRotation);
            (*p_table)(row, col) << rotation.SetValue(details.cyclicRotation);
            col++;
            (*p_table)(row, col++) << rotation.SetValue(details.totalRotation);
        }

        row++;
    }

    return p_table;

}






////======================== ACCESS     =======================================
////======================== INQUIRY    =======================================
//
//////////////////////////// PROTECTED  ///////////////////////////////////////
//
////======================== LIFECYCLE  =======================================
////======================== OPERATORS  =======================================
////======================== OPERATIONS =======================================
void CBearingRotationTable::MakeCopy(const CBearingRotationTable& rOther)
{
    // Add copy code here...
}

void CBearingRotationTable::MakeAssignment(const CBearingRotationTable& rOther)
{
    MakeCopy(rOther);
}
//
////======================== ACCESS     =======================================
////======================== INQUIRY    =======================================
//
//////////////////////////// PRIVATE    ///////////////////////////////////////
//
////======================== LIFECYCLE  =======================================
////======================== OPERATORS  =======================================
////======================== OPERATIONS =======================================
////======================== ACCESS     =======================================
////======================== INQUERY    =======================================
