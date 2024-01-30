/////////////////////////////////////////////////////////////////////////
//// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
//// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <Reporting\BearingReactionTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>
#include <IFace\BearingDesignParameters.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

#include <PgsExt\PierData2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///****************************************************************************
//CLASS
//   CBearingReactiontionTable
//****************************************************************************/
//
//
//////////////////////////// PUBLIC     ///////////////////////////////////////
//
////======================== LIFECYCLE  =======================================
CBearingReactionTable::CBearingReactionTable()
{
}

CBearingReactionTable::CBearingReactionTable(const CBearingReactionTable& rOther)
{
    MakeCopy(rOther);
}

CBearingReactionTable::~CBearingReactionTable()
{
}

//======================== OPERATORS  =======================================
CBearingReactionTable& CBearingReactionTable::operator= (const CBearingReactionTable& rOther)
{
    if (this != &rOther)
    {
        MakeAssignment(rOther);
    }

    return *this;
}







ColumnIndexType CBearingReactionTable::GetBearingTableColumnCount(IBroker* pBroker, const CGirderKey& girderKey,
    pgsTypes::AnalysisType analysisType, bool bDesign, bool bUserLoads, TABLEPARAMETERS* tParam, bool bDetail, DuctIndexType nDucts, bool bTimeStep) const
{

    ColumnIndexType nCols = 1; // location


    if (bDetail)
    {
        nCols = 4; // location, girder, diaphragm, and traffic barrier

        if (bUserLoads)
        {
            if (analysisType == pgsTypes::Envelope)
            {
                nCols += 6;
            }
            else
            {
                nCols += 3;
            }

        }

    }
    else
    {
        nCols++; //Pdl
    }

    //GET_IFACE2(pBroker, IProductLoads, pLoads);
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
    GET_IFACE2(pBroker, IProductLoads, pLoads);
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

    if (bDesign)
    {
        nCols++; // design live loads

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




    if (0 < nDucts && bDetail)
    {
        nCols++;  //add for post-tensioning
    }

    if (bTimeStep && bDetail)
    {

        nCols += 4;

    }
    else
    {
        if (bDetail)
        {
            nCols += 2;
        }
    }

    return nCols;
}


template <class M, class T>
RowIndexType ConfigureBearingReactionTableHeading(IBroker* pBroker, rptRcTable* p_table, bool bPierTable, bool bSegments, 
    bool bConstruction, bool bDeck, bool bDeckPanels, bool bSidewalk, bool bShearKey, bool bLongitudinalJoints, bool bOverlay, 
    bool bIsFutureOverlay, bool bDesign, bool bUserLoads, bool bPedLoading, pgsTypes::AnalysisType analysisType, 
    bool bContinuousBeforeDeckCasting, IEAFDisplayUnits* pDisplayUnits, const T& unitT, bool bDetail, DuctIndexType nDucts, 
    bool bTimeStep)
{
   

    ColumnIndexType col = 0;

    if (bDetail)
    {
        p_table->SetNumberOfHeaderRows(2);
        p_table->SetRowSpan(0, col, 2);
    }
    
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

        GET_IFACE2(pBroker, IProductLoads, pProductLoads);


        p_table->SetRowSpan(0, col, 2);
        (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftGirder), M, unitT);

        p_table->SetRowSpan(0, col, 2);
        (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftDiaphragm), M, unitT);
    }


    if (bShearKey && bDetail)
    {

        GET_IFACE2(pBroker, IProductLoads, pProductLoads);


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

        GET_IFACE2(pBroker, IProductLoads, pProductLoads);

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

        GET_IFACE2(pBroker, IProductLoads, pProductLoads);

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

        GET_IFACE2(pBroker, IProductLoads, pProductLoads);

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

        GET_IFACE2(pBroker, IProductLoads, pProductLoads);

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


        if (bOverlay && bDetail)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);

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
        }

    }
    else
    {
        if (bSidewalk && bDetail)
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


        if (bOverlay && bDetail)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);

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



        if (bUserLoads && bDetail)
        {
            GET_IFACE2(pBroker, IProductLoads, pProductLoads);
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftUserDC), M, unitT);
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftUserDW), M, unitT);
        }

    }







    if (!bDetail)
    {
        (*p_table)(0, col++) << COLHDR(Sub2(_T("P"), _T("DL")), M, unitT);
    }


    if (bDesign)
    {

        if (bUserLoads && analysisType == pgsTypes::Envelope && bDetail)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);
            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftUserLLIM);
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);

            if (bPedLoading)
            {
                p_table->SetColumnSpan(0, col, 2);
                (*p_table)(0, col) << _T("Pedestrian");
                (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
                (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
            }


        }
        else
        {
            if (bUserLoads && bDetail)
            {
                GET_IFACE2(pBroker, IProductLoads, pProductLoads);
                p_table->SetRowSpan(0, col, 2);
                (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftUserLLIM), M, unitT);
            }

            if (bPedLoading && bDetail)
            {
                p_table->SetColumnSpan(0, col, 2);
                (*p_table)(0, col) << _T("Pedestrian");
                (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
                (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
            }
        }


        if (!bDetail)
        {
            (*p_table)(0, col) << COLHDR(Sub2(_T("P"), _T("LL")), M, unitT);
        }
        else
        {
            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << Sub2(_T("P"), _T("LL"));
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);


        }
    }


    

    if (bDetail)
    {
        p_table->SetRowSpan(0, col, 2);

        GET_IFACE2(pBroker, IProductLoads, pProductLoads);


        (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftPretension), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
    }

    

    if (bDetail)
    {

        p_table->SetRowSpan(0, col, 2);
        if (0 < nDucts)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);

            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftPostTensioning), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
        }
    }




    if (bTimeStep)
    {
        if (bDetail)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);


            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftCreep), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftShrinkage), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftRelaxation), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
        }
    }
    else
    {
        if (bDetail)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);


            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftCreep), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
        }

    }



    return p_table->GetNumberOfHeaderRows(); // index of first row to report data
}





//======================== OPERATIONS =======================================
rptRcTable* CBearingReactionTable::BuildBearingReactionTable(IBroker* pBroker, const CGirderKey& girderKey, pgsTypes::AnalysisType analysisType,
    bool bIncludeImpact, bool bIncludeLLDF, bool bDesign, bool bUserLoads, bool bIndicateControllingLoad, IEAFDisplayUnits* pDisplayUnits, bool bDetail) const
{


    // Build table
    INIT_UV_PROTOTYPE(rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false);
    INIT_UV_PROTOTYPE(rptForceUnitValue, reactu, pDisplayUnits->GetShearUnit(), false);

    // Tricky: the reaction tool below will dump out two lines per cell for bearing reactions with more than one bearing
    ReactionUnitValueTool Reaction(BearingReactionsTable, reactu);

    GET_IFACE2_NOCHECK(pBroker, IBridgeDescription, pIBridgeDesc);
    GET_IFACE2(pBroker, IBridge, pBridge);
    bool bHasOverlay = pBridge->HasOverlay();
    bool bFutureOverlay = pBridge->IsFutureOverlay();
    PierIndexType nPiers = pBridge->GetPierCount();

    bIncludeLLDF = false; // this table never distributes live load

    GET_IFACE2(pBroker, IIntervals, pIntervals);
    IntervalIndexType diaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();
    IntervalIndexType lastCastDeckIntervalIdx = pIntervals->GetLastCastDeckInterval(); // deck cast be cast in multiple stages, use interval after entire deck is cast
    IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
    IntervalIndexType ljIntervalIdx = pIntervals->GetCastLongitudinalJointInterval();
    IntervalIndexType shearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();
    IntervalIndexType constructionIntervalIdx = pIntervals->GetConstructionLoadInterval();
    IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
    IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

    bool bSegments, bConstruction, bDeck, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bLongitudinalJoint, bPermit;
    bool bContinuousBeforeDeckCasting;
    GroupIndexType startGroup, endGroup;

    GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

    GET_IFACE2(pBroker, IGirderTendonGeometry, pTendonGeom);
    DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);

    GET_IFACE2(pBroker, ILossParameters, pLossParams);
    bool bTimeStep = (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP ? true : false);

    TABLEPARAMETERS tParam;

    ColumnIndexType nCols = GetBearingTableColumnCount(pBroker, girderKey, analysisType, bDesign, bUserLoads, &tParam, bDetail, nDucts, bTimeStep);


    CString label = _T("Reactions");
    rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, label);
    RowIndexType row = ConfigureBearingReactionTableHeading<rptForceUnitTag, WBFL::Units::ForceData>(
        pBroker, p_table, true, tParam.bSegments, tParam.bConstruction, tParam.bDeck, tParam.bDeckPanels,
        tParam.bSidewalk, tParam.bShearKey, tParam.bLongitudinalJoint, bHasOverlay,
        bFutureOverlay, bDesign, bUserLoads, tParam.bPedLoading, analysisType, tParam.bContinuousBeforeDeckCasting,
        pDisplayUnits, pDisplayUnits->GetGeneralForceUnit(), bDetail, nDucts, bTimeStep);


    p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
    p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));

    GET_IFACE2(pBroker, IProductForces, pProdForces);
    pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);
    pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType, pgsTypes::Minimize);

    pgsTypes::BridgeAnalysisType batSS = pgsTypes::SimpleSpan;
    pgsTypes::BridgeAnalysisType batCS = pgsTypes::ContinuousSpan;

    // TRICKY: use adapter class to get correct reaction interfaces
    std::unique_ptr<IProductReactionAdapter> pForces;

    GET_IFACE2(pBroker, IReactions, pReactions);
    pForces = std::make_unique<ProductForcesReactionAdapter>(pReactions, girderKey);


    ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);


    // Use iterator to walk locations
    for (iter.First(); !iter.IsDone(); iter.Next())

    {

        const ReactionLocation& reactionLocation(iter.CurrentItem());

        const CGirderKey& thisGirderKey(reactionLocation.GirderKey);
        IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetLastSegmentErectionInterval(thisGirderKey);

        const CBearingData2* pbd = pIBridgeDesc->GetBearingData(reactionLocation.PierIdx, (reactionLocation.Face == rftBack ? pgsTypes::Back : pgsTypes::Ahead), girderKey.girderIndex);
        IndexType numBearings = pbd->BearingCount;

        Reaction.SetNumBearings(numBearings); // class will dump per-bearing reaction if applicable:

        ReactionDecider reactionDecider(BearingReactionsTable, reactionLocation, thisGirderKey, pBridge, pIntervals);

        GET_IFACE2(pBroker, IBearingDesignParameters, pBearingDesignParameters);
        REACTIONDETAILS details;
        pBearingDesignParameters->GetBearingReactionDetails(erectSegmentIntervalIdx, lastIntervalIdx, reactionLocation, girderKey, analysisType, &details);


        ColumnIndexType col = 0;

        Reaction.SetNumBearings(numBearings); // class will dump per-bearing reaction if applicable:

        (*p_table)(row, col) << reactionLocation.PierLabel;
        if (numBearings > 1) // add second line for per-bearing value
        {
            (*p_table)(row, col) << _T(" - Total") << rptNewLine << _T("Per Bearing");
        }

        col++;


        if (bDetail)
        {
            if (tParam.bSegments)
            {
                (*p_table)(row, col++) << Reaction.SetValue(details.erectedSegmentReaction);
                (*p_table)(row, col++) << Reaction.SetValue(details.maxGirderReaction);
            }
            else
            {
                (*p_table)(row, col++) << Reaction.SetValue(details.maxGirderReaction);
            }

            if (reactionDecider.DoReport(lastIntervalIdx))
            {
                (*p_table)(row, col++) << Reaction.SetValue(details.diaphragmReaction);
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
                        (*p_table)(row, col++) << Reaction.SetValue(0);
                        (*p_table)(row, col++) << Reaction.SetValue(0);
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
                        (*p_table)(row, col++) << Reaction.SetValue(0);
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
                        (*p_table)(row, col++) << Reaction.SetValue(0);
                        (*p_table)(row, col++) << Reaction.SetValue(0);
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
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxLongitudinalJointReaction);
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
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxConstructionReaction);
                        (*p_table)(row, col++) << Reaction.SetValue(details.minConstructionReaction);
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
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxConstructionReaction);
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
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxSlabReaction);
                        (*p_table)(row, col++) << Reaction.SetValue(details.minSlabReaction);

                        (*p_table)(row, col++) << Reaction.SetValue(details.maxHaunchReaction);
                        (*p_table)(row, col++) << Reaction.SetValue(details.minHaunchReaction);
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
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxSlabReaction);
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxHaunchReaction);
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
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxSlabPanelReaction);
                        (*p_table)(row, col++) << Reaction.SetValue(details.minSlabPanelReaction);
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
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxSlabPanelReaction);
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

        if (!bDetail)
        {
            (*p_table)(row, col++) << Reaction.SetValue(0);
        }



        if (analysisType == pgsTypes::Envelope)
        {
            if (bDetail)
            {
                if (tParam.bSidewalk)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxSidewalkReaction);
                        (*p_table)(row, col++) << Reaction.SetValue(details.minSidewalkReaction);
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }

                if (reactionDecider.DoReport(lastIntervalIdx))
                {
                    (*p_table)(row, col++) << Reaction.SetValue(details.maxRailingSystemReaction);
                    (*p_table)(row, col++) << Reaction.SetValue(details.minRailingSystemReaction);
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
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxFutureOverlayReaction);
                        (*p_table)(row, col++) << Reaction.SetValue(details.minFutureOverlayReaction);
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }


                if (bUserLoads)
                {
                    (*p_table)(row, col++) << Reaction.SetValue(details.maxUserDCReaction);
                    (*p_table)(row, col++) << Reaction.SetValue(details.minUserDCReaction);
                    (*p_table)(row, col++) << Reaction.SetValue(details.maxUserDWReaction);
                    (*p_table)(row, col++) << Reaction.SetValue(details.minUserDWReaction);
                }


            }




            if (bDesign)
            {

                if (reactionDecider.DoReport(lastIntervalIdx))
                {

                    if (bDetail)
                    {
                        
                        if (bUserLoads)
                        {
                            (*p_table)(row, col++) << Reaction.SetValue(details.maxUserLLReaction);
                            (*p_table)(row, col++) << Reaction.SetValue(details.minUserLLReaction);
                        }
                        col++;


                        if (tParam.bPedLoading)
                        {
                            if (reactionDecider.DoReport(lastIntervalIdx))
                            {
                                (*p_table)(row, col++) << Reaction.SetValue(details.maxPedReaction);
                                (*p_table)(row, col++) << Reaction.SetValue(details.minPedReaction);
                            }
                            else
                            {
                                (*p_table)(row, col++) << RPT_NA;
                                (*p_table)(row, col++) << RPT_NA;
                            }
                        }


                        (*p_table)(row, col) << Reaction.SetValue(details.maxDesignLLReaction);
                        if (bIndicateControllingLoad && 0 <= details.maxConfig)
                        {
                            (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << details.maxConfig << _T(")");
                        }
                        col++;

                        (*p_table)(row, col) << Reaction.SetValue(details.minDesignLLReaction);
                        if (bIndicateControllingLoad && 0 <= details.minConfig)
                        {
                            (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << details.minConfig << _T(")");
                        }


                    }
                    else
                    {
                        (*p_table)(row, col) << Reaction.SetValue(details.maxDesignLLReaction);
                        if (bIndicateControllingLoad && 0 <= details.maxConfig)
                        {
                            (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << details.maxConfig << _T(")");
                        }
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
                if (tParam.bSidewalk)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxSidewalkReaction);
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }

                if (reactionDecider.DoReport(lastIntervalIdx))
                {
                    (*p_table)(row, col++) << Reaction.SetValue(details.maxRailingSystemReaction);
                }
                else
                {
                    (*p_table)(row, col++) << RPT_NA;
                }

                if (bHasOverlay)
                {
                    if (reactionDecider.DoReport(lastIntervalIdx))
                    {
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxFutureOverlayReaction);
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
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxPedReaction);
                        (*p_table)(row, col++) << Reaction.SetValue(details.minPedReaction);
                    }
                    else
                    {
                        (*p_table)(row, col++) << RPT_NA;
                        (*p_table)(row, col++) << RPT_NA;
                    }
                }

                if (bUserLoads)
                {
                    (*p_table)(row, col++) << Reaction.SetValue(details.maxUserDCReaction);
                    (*p_table)(row, col++) << Reaction.SetValue(details.maxUserDWReaction);
                }
            }


            if (bDesign)
            {
                if (reactionDecider.DoReport(lastIntervalIdx))
                {
                    (*p_table)(row, col) << Reaction.SetValue(details.maxDesignLLReaction);
                    if (bIndicateControllingLoad && 0 <= details.maxConfig)
                    {
                        (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << details.maxConfig << _T(")");
                    }
                    col++;

                    if (bDetail)
                    {
                        (*p_table)(row, col) << Reaction.SetValue(details.minDesignLLReaction);
                    }

                    if (bIndicateControllingLoad && 0 <= details.minConfig && bDetail)
                    {
                        (*p_table)(row, col++) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << details.minConfig << _T(")");
                    }
                    if (bDetail && bUserLoads)
                    {
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxUserLLReaction);
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
            (*p_table)(row, col++) << Reaction.SetValue(details.preTensionReaction);
            if (0 < nDucts)
            {
                (*p_table)(row, col++) << Reaction.SetValue(details.postTensionReaction);
            }
            (*p_table)(row, col++) << Reaction.SetValue(details.creepReaction);
            if (bTimeStep)
            {
                (*p_table)(row, col++) << Reaction.SetValue(details.shrinkageReaction);
                (*p_table)(row, col++) << Reaction.SetValue(details.relaxationReaction);
            }

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
void CBearingReactionTable::MakeCopy(const CBearingReactionTable& rOther)
{
    // Add copy code here...
}

void CBearingReactionTable::MakeAssignment(const CBearingReactionTable& rOther)
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
