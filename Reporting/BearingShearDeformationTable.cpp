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
#include <Reporting\BearingShearDeformationTable.h>
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

///****************************************************************************
//CLASS
//   CBearingShearDeformationtionTable
//****************************************************************************/
//
//
//////////////////////////// PUBLIC     ///////////////////////////////////////
//
////======================== LIFECYCLE  =======================================
CBearingShearDeformationTable::CBearingShearDeformationTable()
{
}

CBearingShearDeformationTable::CBearingShearDeformationTable(const CBearingShearDeformationTable& rOther)
{
    MakeCopy(rOther);
}

CBearingShearDeformationTable::~CBearingShearDeformationTable()
{
}

//======================== OPERATORS  =======================================
CBearingShearDeformationTable& CBearingShearDeformationTable::operator= (const CBearingShearDeformationTable& rOther)
{
    if (this != &rOther)
    {
        MakeAssignment(rOther);
    }

    return *this;
}







ColumnIndexType CBearingShearDeformationTable::GetBearingTableColumnCount(IBroker* pBroker, const CGirderKey& girderKey,
    pgsTypes::AnalysisType analysisType, SHEARDEFORMATIONDETAILS* details, bool bDetail) const
{

    ColumnIndexType nCols = 1; // location

    if (bDetail)
    {
        nCols += 2; // thermal expansion cold or moderate

        if (0 < details->nDucts)
        {
            nCols++;  //post-tensioning
        }
        nCols += 3; // creep, shrinkage & relaxation
    }
    else
    {
        nCols += 2; // total deformation cold or moderate
    }

    return nCols;
}


template <class M, class T>
RowIndexType ConfigureBearingShearDeformationTableHeading(IBroker* pBroker, rptRcTable* p_table, pgsTypes::AnalysisType analysisType, 
    IEAFDisplayUnits* pDisplayUnits, const T& unitT, SHEARDEFORMATIONDETAILS* pDetails, bool bDetail)
{
    p_table->SetNumberOfHeaderRows(2);

    ColumnIndexType col = 0;

    p_table->SetRowSpan(0, col, 2);
    (*p_table)(0, col++) << _T("");

    if (bDetail)
    {
        p_table->SetColumnSpan(0, col, 2);
        (*p_table)(0, col) << Sub2(symbol(DELTA),_T("thermal"));
        (*p_table)(1, col++) << COLHDR(_T("Cold"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
        (*p_table)(1, col++) << COLHDR(_T("Moderate"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
        if (0 < pDetails->nDucts)
        {
            p_table->SetRowSpan(0, col, 2);
            (*p_table)(0, col++) << COLHDR(_T("Post-Tensioning"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
        }
        p_table->SetColumnSpan(0, col, 3);
        (*p_table)(0, col) << Sub2(symbol(DELTA), _T("time-dependent"));
        (*p_table)(1, col++) << COLHDR(_T("Creep"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
        (*p_table)(1, col++) << COLHDR(_T("Shrinkage"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
        (*p_table)(1, col++) << COLHDR(_T("Relaxation"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
    }
    else
    {
        p_table->SetColumnSpan(0, col, 2);
        (*p_table)(0, col) << Sub2(symbol(DELTA),_T("total"));
        (*p_table)(1, col++) << COLHDR(_T("Cold"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
        (*p_table)(1, col++) << COLHDR(_T("Moderate"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
    }


    return p_table->GetNumberOfHeaderRows(); // index of first row to report data
}





//======================== OPERATIONS =======================================
rptRcTable* CBearingShearDeformationTable::BuildBearingShearDeformationTable(IBroker* pBroker, const CGirderKey& girderKey, pgsTypes::AnalysisType analysisType,
    bool bIncludeImpact, bool bIncludeLLDF, bool bDesign, bool bUserLoads, bool bIndicateControllingLoad, IEAFDisplayUnits* pDisplayUnits, bool bDetail) const
{

    // Build table
    INIT_UV_PROTOTYPE(rptLengthUnitValue, Reaction, pDisplayUnits->GetDeflectionUnit(), false);

    GET_IFACE2(pBroker, IBridge, pBridge);

    GET_IFACE2(pBroker, IIntervals, pIntervals);
    IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
    IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;


    GET_IFACE2(pBroker, IPointOfInterest, pPOI);

    GET_IFACE2(pBroker, IBearingDesignParameters, pBearingDesignParameters);
    SHEARDEFORMATIONDETAILS details;
    pBearingDesignParameters->GetBearingTableParameters(girderKey, &details);


    ColumnIndexType nCols = GetBearingTableColumnCount(pBroker, girderKey, analysisType, &details, bDetail);


    //CString label = _T("Shear Deformations");
    rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, _T(""));
    RowIndexType row = ConfigureBearingShearDeformationTableHeading<rptAngleUnitTag, WBFL::Units::AngleData>(
        pBroker, p_table, analysisType, pDisplayUnits, pDisplayUnits->GetRadAngleUnit(), &details, bDetail);

    p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
    p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));






    // get poi where pier Reactions occur
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


    // Use iterator to walk locations
    for (iter.First(); !iter.IsDone(); iter.Next())
    {
        ColumnIndexType col = 0;

        const ReactionLocation& reactionLocation(iter.CurrentItem());
        const CGirderKey& thisGirderKey(reactionLocation.GirderKey);

        (*p_table)(row, col++) << reactionLocation.PierLabel;

        ReactionDecider reactionDecider(BearingReactionsTable, reactionLocation, thisGirderKey, pBridge, pIntervals);

        const pgsPointOfInterest& poi = vPoi[reactionLocation.PierIdx - startPierIdx];

        pBearingDesignParameters->GetBearingShearDeformationDetails(analysisType, startPierIdx, poi, reactionLocation, girderKey, bIncludeImpact, bIncludeLLDF, &details);

        if (bDetail)
        {
            (*p_table)(row, col++) << Reaction.SetValue(details.thermal_expansion_cold);
            (*p_table)(row, col++) << Reaction.SetValue(details.thermal_expansion_moderate);
            if (0 < details.nDucts)
            {
                (*p_table)(row, col++) << Reaction.SetValue(details.postTension);
            }
            (*p_table)(row, col++) << Reaction.SetValue(details.creep);
            (*p_table)(row, col++) << Reaction.SetValue(details.shrinkage);
            (*p_table)(row, col) << Reaction.SetValue(details.relaxation);
        }
        else
        {
            (*p_table)(row, col++) << Reaction.SetValue(details.total_shear_deformation_cold);
            (*p_table)(row, col++) << Reaction.SetValue(details.total_shear_deformation_moderate);
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
void CBearingShearDeformationTable::MakeCopy(const CBearingShearDeformationTable& rOther)
{
    // Add copy code here...
}

void CBearingShearDeformationTable::MakeAssignment(const CBearingShearDeformationTable& rOther)
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
