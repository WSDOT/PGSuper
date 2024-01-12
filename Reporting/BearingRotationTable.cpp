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
#include <IFace\BearingDesignParameters.h>

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

    //nCols++;

    if (bDetail)
    {
        nCols = 4; // location, girder, diaphragm, and traffic barrier

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

    if (!bDetail)
    {
        if (pgsTypes::Envelope)
        {
            nCols += 3;
        }
        else
        {
            nCols += 4;
        }

        
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
        else
        {
            nCols ++;
        }
        
    }




    return nCols;
}


template <class M, class T>
RowIndexType ConfigureBearingRotationTableHeading(IBroker* pBroker, rptRcTable* p_table, bool bPierTable, bool bSegments, bool bConstruction, bool bDeck, bool bDeckPanels, bool bSidewalk, bool bShearKey, bool bLongitudinalJoints, bool bOverlay, bool bIsFutureOverlay,
    bool bDesign, bool bUserLoads, bool bPedLoading, pgsTypes::AnalysisType analysisType, bool bContinuousBeforeDeckCasting, IEAFDisplayUnits* pDisplayUnits, const T& unitT, bool bDetail, DuctIndexType nDucts, bool bTimeStep)
{
    p_table->SetNumberOfHeaderRows(2);


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
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftUserDC), M, unitT);
            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftUserDW), M, unitT);
        }

    }



    if (bPedLoading && bDetail)
    {
        p_table->SetColumnSpan(0, col, 2);
        (*p_table)(0, col) << _T("Pedestrian");
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
            (*p_table)(0, col) << Sub2(symbol(theta), _T("cy"));
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
        }
        else
        {
            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << Sub2(symbol(theta), _T("Design LL"));
            (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
            (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);

            if (bUserLoads && analysisType == pgsTypes::Envelope)
            {
                GET_IFACE2(pBroker, IProductLoads, pProductLoads);
                p_table->SetColumnSpan(0, col, 2);
                (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftUserLLIM);
                (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
                (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
            }
            else
            {
                if (bUserLoads)
                {
                    GET_IFACE2(pBroker, IProductLoads, pProductLoads);
                    (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftUserLLIM), M, unitT);
                }
            }




        }
    }

    if (!bDetail)
    {
            (*p_table)(0, col) << Sub2(symbol(theta), _T("Service I"));
            (*p_table)(1, col++) << COLHDR(_T("Max"), rptAngleUnitTag, unitT);
    }

    p_table->SetRowSpan(0, col, 2);

    if (bDetail)
    {

        GET_IFACE2(pBroker, IProductLoads, pProductLoads);


        (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftPretension), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
    }
   
    p_table->SetRowSpan(0, col, 2);

    if (bDetail)
    {
        if (0 < nDucts)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);

            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftPostTensioning), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
        }
    }


    

    if (bTimeStep)
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
        else
        {
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(_T("Time Dependent"), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
        }
    }
    else
    {
        if (bDetail)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);


            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftCreep), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
        }
        else
        {
            (*p_table)(0, col++) << COLHDR(_T("Time Dependent"), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
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

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        GET_IFACE2(pBroker, IBearingDesignParameters, pBearingDesignParameters);
        ROTATIONDETAILS details;
        pBearingDesignParameters->GetBearingRotationDetails(analysisType, poi, reactionLocation, bIncludeImpact, bIncludeLLDF, isFlexural, &details);


        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


        if (bDetail)
        {
            if (tParam.bSegments)
            {
                (*p_table)(row, col++) << rotation.SetValue(pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, maxBAT, rtCumulative, false));
                (*p_table)(row, col++) << rotation.SetValue(details.girderRotation);
            }
            else
            {
                (*p_table)(row, col++) << rotation.SetValue(details.girderRotation);
            }

            if (reactionDecider.DoReport(lastIntervalIdx))
            {
                (*p_table)(row, col++) << rotation.SetValue(details.diaphragmRotation);
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
                        (*p_table)(row, col++) << rotation.SetValue(pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftShearKey, poi, maxBAT, rtCumulative, false));
                        (*p_table)(row, col++) << rotation.SetValue(pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftShearKey, poi, minBAT, rtCumulative, false));
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
                        (*p_table)(row, col++) << rotation.SetValue(pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftShearKey, poi, maxBAT, rtCumulative, false));
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
                        (*p_table)(row, col++) << rotation.SetValue(pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, maxBAT, rtCumulative, false));
                        (*p_table)(row, col++) << rotation.SetValue(pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, minBAT, rtCumulative, false));
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
                        (*p_table)(row, col++) << rotation.SetValue(details.longitudinalJointRotation);
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



            if (tParam.bDeck)
            {
                if (analysisType == pgsTypes::Envelope && tParam.bContinuousBeforeDeckCasting)
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

            if (tParam.bDeckPanels)
            {
                if (analysisType == pgsTypes::Envelope && tParam.bContinuousBeforeDeckCasting)
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

        if (!bDetail)
        {
            (*p_table)(row, col++) << rotation.SetValue(details.maxDCrotation);
            (*p_table)(row, col++) << rotation.SetValue(details.maxDWrotation);
        }

        if (bDetail && bUserLoads)
        {
            if (analysisType == pgsTypes::Envelope)
            {
                (*p_table)(row, col++) << rotation.SetValue(details.maxUserDCRotation);
                (*p_table)(row, col++) << rotation.SetValue(details.minUserDCRotation);
                (*p_table)(row, col++) << rotation.SetValue(details.maxUserDWRotation);
                (*p_table)(row, col++) << rotation.SetValue(details.minUserDWRotation);
            }
            else
            {
                (*p_table)(row, col++) << rotation.SetValue(details.maxUserDCRotation);
                (*p_table)(row, col++) << rotation.SetValue(details.maxUserDWRotation);
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

                if (bHasOverlay)
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

                if (tParam.bPedLoading)
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
                        (*p_table)(row, col) << rotation.SetValue(details.minDesignLLrotation);
                        if (bIndicateControllingLoad && 0 <= details.maxConfig)
                        {
                            (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << details.maxConfig << _T(")");
                        }
                        col++;

                        (*p_table)(row, col) << rotation.SetValue(details.maxDesignLLrotation);
                        if (bIndicateControllingLoad && 0 <= details.minConfig)
                        {
                            (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << details.minConfig << _T(")");
                        }
                        col++;
                        if (bUserLoads)
                        {
                            (*p_table)(row, col++) << rotation.SetValue(details.maxUserLLrotation);
                            (*p_table)(row, col++) << rotation.SetValue(details.minUserLLrotation);
                        }
                    }
                    else
                    {    
                        (*p_table)(row, col) << rotation.SetValue(details.cyclicRotation);
                        if (bIndicateControllingLoad && 0 <= details.maxConfig)
                        {
                            (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << details.maxConfig << _T(")");
                        }
                        col++;
                        (*p_table)(row, col++) << rotation.SetValue(details.serviceIRotation);
                        
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

                if (bHasOverlay)
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

                if (tParam.bPedLoading)
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
                    (*p_table)(row, col) << rotation.SetValue(details.minDesignLLrotation);
                    if (bIndicateControllingLoad && 0 <= details.maxConfig)
                    {
                        (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << details.maxConfig << _T(")");
                    }
                    col++;

                    if (bDetail)
                    {
                        (*p_table)(row, col) << rotation.SetValue(details.minDesignLLrotation);
                    }
                    
                    if (bIndicateControllingLoad && 0 <= details.minConfig && bDetail)
                    {
                        (*p_table)(row, col++) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << details.minConfig << _T(")");
                    }
                    if (bDetail && bUserLoads)
                    {
                        (*p_table)(row, col++) << rotation.SetValue(details.maxUserLLrotation);
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
                (*p_table)(row, col++) << rotation.SetValue(details.serviceIRotation);
            }



        }

        if (bDetail)
        {
            (*p_table)(row, col++) << rotation.SetValue(details.preTensionRotation);
            if (0 < nDucts)
            {
                (*p_table)(row, col++) << rotation.SetValue(details.postTensionRotation);
            }
            (*p_table)(row, col++) << rotation.SetValue(details.creepRotation);
            if (bTimeStep)
            {
                (*p_table)(row, col++) << rotation.SetValue(details.shrinkageRotation);
                (*p_table)(row, col++) << rotation.SetValue(details.relaxationRotation);
            }

        }
        else
        {
            (*p_table)(row, col++) << rotation.SetValue(details.totalTimeDependentRotation);
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
