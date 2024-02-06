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


#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>


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
    pgsTypes::AnalysisType analysisType, bool bDesign, bool bUserLoads, REACTIONDETAILS* details, bool bDetail) const
{

    ColumnIndexType nCols = 1; // location


    if (bDetail)
    {

        nCols += 3; // girder, diaphragm, and max rail

        if (details->bDeck)
        {
            nCols += 2;
        }

        

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

    


    if (details->bSegments && bDetail)
    {
        nCols++;
    }

    if (details->bHasOverlay && bDetail)
    {
        nCols++;  //overlay max

        if (analysisType == pgsTypes::Envelope)
            nCols++; //overlay min
    }

   



    if (details->bConstruction && bDetail)
    {
        if (analysisType == pgsTypes::Envelope && details->bContinuousBeforeDeckCasting)
        {
            nCols += 2;
        }
        else
        {
            nCols++;
        }
    }

    if (details->bDeckPanels && bDetail)
    {
        if (analysisType == pgsTypes::Envelope && details->bContinuousBeforeDeckCasting)
        {
            nCols += 2;
        }
        else
        {
            nCols++;
        }
    }

    if (details->bDeck && analysisType == pgsTypes::Envelope && details->bContinuousBeforeDeckCasting && bDetail)
    {
        nCols += 2; // add one more each for min/max slab and min/max slab pad
    }

    if (analysisType == pgsTypes::Envelope && bDetail)
    {
        nCols++; // add for min traffic barrier
    }

    if (bDesign)
    {
        nCols++; // design live loads

        if (bDetail)
        {
            nCols++; // accounts for min
        }
    }

    if (details->bPedLoading && bDetail)
    {
        nCols += 2;
    }

    if (details->bSidewalk && bDetail)
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

    if (details->bShearKey && bDetail)
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

    if (details->bLongitudinalJoint && bDetail)
    {
        if (analysisType == pgsTypes::Envelope && details->bContinuousBeforeDeckCasting)
        {
            nCols += 2;
        }
        else
        {
            nCols++;
        }
    }




    if (0 < details->nDucts && bDetail)
    {
        nCols++;  //add for post-tensioning
    }

    if (details->bTimeStep && bDetail)
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
RowIndexType ConfigureBearingReactionTableHeading(IBroker* pBroker, rptRcTable* p_table,
    bool bDesign, bool bUserLoads, pgsTypes::AnalysisType analysisType, IEAFDisplayUnits* pDisplayUnits, const T& unitT, bool bDetail, REACTIONDETAILS* pDetails)
{
   

    ColumnIndexType col = 0;

    if (bDetail)
    {
        p_table->SetNumberOfHeaderRows(2);
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

            if (pDetails->bPedLoading)
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

            if (pDetails->bPedLoading && bDetail)
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
        if (0 < pDetails->nDucts)
        {

            GET_IFACE2(pBroker, IProductLoads, pProductLoads);

            (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftPostTensioning), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
        }
    }




    if (pDetails->bTimeStep)
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




    GET_IFACE2(pBroker, IBearingDesignParameters, pBearingDesignParameters);
    REACTIONDETAILS details;
    pBearingDesignParameters->GetBearingTableParameters(girderKey, &details);

    ColumnIndexType nCols = GetBearingTableColumnCount(pBroker, girderKey, analysisType, bDesign, bUserLoads, &details, bDetail);


    CString label = _T("Reactions");
    rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, label);


    RowIndexType row = ConfigureBearingReactionTableHeading<rptForceUnitTag, WBFL::Units::ForceData>(
        pBroker, p_table, bDesign, bUserLoads, analysisType, pDisplayUnits, pDisplayUnits->GetGeneralForceUnit(), bDetail, &details);


    p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
    p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));



    // TRICKY: use adapter class to get correct reaction interfaces
    std::unique_ptr<IProductReactionAdapter> pForces;

    GET_IFACE2(pBroker, IReactions, pReactions);
    pForces = std::make_unique<ProductForcesReactionAdapter>(pReactions, girderKey);


    ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);



    


    // Use iterator to walk locations
    for (iter.First(); !iter.IsDone(); iter.Next())

    {

        const ReactionLocation& reactionLocation(iter.CurrentItem());

        const CBearingData2* pbd = pIBridgeDesc->GetBearingData(reactionLocation.PierIdx, (reactionLocation.Face == rftBack ? pgsTypes::Back : pgsTypes::Ahead), girderKey.girderIndex);
        IndexType numBearings = pbd->BearingCount;

        Reaction.SetNumBearings(numBearings); // class will dump per-bearing reaction if applicable:

        pBearingDesignParameters->GetBearingReactionDetails(reactionLocation, girderKey, analysisType, bIncludeImpact, bIncludeLLDF, &details);


        const CGirderKey& thisGirderKey(reactionLocation.GirderKey);

        ReactionDecider reactionDecider(BearingReactionsTable, reactionLocation, thisGirderKey, pBridge, pIntervals);


        ColumnIndexType col = 0;

        (*p_table)(row, col) << reactionLocation.PierLabel;
        if (numBearings > 1) // add second line for per-bearing value
        {
            (*p_table)(row, col) << _T(" - Total") << rptNewLine << _T("Per Bearing");
        }

        col++;


        if (bDetail)
        {
            if (details.bSegments)
            {
                (*p_table)(row, col++) << Reaction.SetValue(details.erectedSegmentReaction);
                (*p_table)(row, col++) << Reaction.SetValue(details.maxGirderReaction);
            }
            else
            {
                (*p_table)(row, col++) << Reaction.SetValue(details.maxGirderReaction);
            }

            if (reactionDecider.DoReport(diaphragmIntervalIdx) && reactionDecider.DoReport(lastIntervalIdx))
            {
                (*p_table)(row, col++) << Reaction.SetValue(details.diaphragmReaction);
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
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxShearKeyReaction);
                        (*p_table)(row, col++) << Reaction.SetValue(details.minShearKeyReaction);
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
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxShearKeyReaction);
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
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxLongitudinalJointReaction);
                        (*p_table)(row, col++) << Reaction.SetValue(details.minLongitudinalJointReaction);
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

            if (details.bConstruction)
            {
                if (analysisType == pgsTypes::Envelope && details.bContinuousBeforeDeckCasting)
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



            if (details.bDeck)
            {
                if (analysisType == pgsTypes::Envelope && details.bContinuousBeforeDeckCasting)
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

            if (details.bDeckPanels)
            {
                if (analysisType == pgsTypes::Envelope && details.bContinuousBeforeDeckCasting)
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
                if (details.bSidewalk)
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

                if (details.bHasOverlay)
                {
                    if (reactionDecider.DoReport(overlayIntervalIdx))
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
 //                       col++;


                        if (details.bPedLoading)
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
                        col++;


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
                if (details.bSidewalk)
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

                if (details.bHasOverlay)
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

                if (bUserLoads)
                {
                    (*p_table)(row, col++) << Reaction.SetValue(details.maxUserDCReaction);
                    (*p_table)(row, col++) << Reaction.SetValue(details.maxUserDWReaction);
                    if (bDesign)
                    {
                        (*p_table)(row, col++) << Reaction.SetValue(details.maxUserLLReaction);
                    }
                    
                }

                if (details.bPedLoading)
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
            if (0 < details.nDucts)
            {
                (*p_table)(row, col++) << Reaction.SetValue(details.postTensionReaction);
            }
            (*p_table)(row, col++) << Reaction.SetValue(details.creepReaction);
            if (details.bTimeStep)
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
