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
#include <Reporting\BearingTimeStepShearDeformationTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>


#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///****************************************************************************
//CLASS
//   CTimeStepShearDeformationtionTable
//****************************************************************************/
//
//
//////////////////////////// PUBLIC     ///////////////////////////////////////
//
////======================== LIFECYCLE  =======================================
CTimeStepShearDeformationTable::CTimeStepShearDeformationTable()
{
}

CTimeStepShearDeformationTable::CTimeStepShearDeformationTable(const CTimeStepShearDeformationTable& rOther)
{
    MakeCopy(rOther);
}

CTimeStepShearDeformationTable::~CTimeStepShearDeformationTable()
{
}

//======================== OPERATORS  =======================================
CTimeStepShearDeformationTable& CTimeStepShearDeformationTable::operator= (const CTimeStepShearDeformationTable& rOther)
{
    if (this != &rOther)
    {
        MakeAssignment(rOther);
    }

    return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CTimeStepShearDeformationTable::BuildTimeStepShearDeformationTable(IBroker* pBroker, 
    const ReactionLocation& reactionLocation, IntervalIndexType intervalIdx) const
{
    


    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
    GET_IFACE2(pBroker, IIntervals, pIntervals);
    GET_IFACE2(pBroker, IProductLoads, pProductLoads);


    INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
    INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);
    INIT_UV_PROTOTYPE(rptLength4UnitValue, I, pDisplayUnits->GetMomentOfInertiaUnit(), false);
    INIT_UV_PROTOTYPE(rptLength2UnitValue, A, pDisplayUnits->GetAreaUnit(), false);
    INIT_UV_PROTOTYPE(rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false);
    INIT_UV_PROTOTYPE(rptTemperatureUnitValue, temperature, pDisplayUnits->GetTemperatureUnit(), false);

    // Build table
    INIT_UV_PROTOTYPE(rptForceUnitValue, reactu, pDisplayUnits->GetShearUnit(), false);


    GET_IFACE2(pBroker, IBearingDesignParameters, pBearing);
    GET_IFACE2(pBroker, IBridge, pBridge);
    GET_IFACE2(pBroker, IPointOfInterest, pPOI);

    SHEARDEFORMATIONDETAILS details;

    CGirderKey girderKey = reactionLocation.GirderKey;

    pBearing->GetBearingTableParameters(girderKey, &details);


    PoiList vPoi;
    std::vector<CGirderKey> vGirderKeys;
    pBridge->GetGirderline(girderKey.girderIndex, details.startGroup, details.endGroup, &vGirderKeys);    //start group and end groups not defined
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


    GET_IFACE2(pBroker, IPointOfInterest, pIPoi);
    GET_IFACE2(pBroker, IBearingDesign, pBearingDesign);
    IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
    std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, lastCompositeDeckIntervalIdx, girderKey));

    ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);
    iter.First();
    PierIndexType startPierIdx = (iter.IsDone() ? INVALID_INDEX : iter.CurrentItem().PierIdx);

    const pgsPointOfInterest& poi = vPoi[reactionLocation.PierIdx - startPierIdx];

    Float64 L = pBearing->GetDistanceToPointOfFixity(poi, &details);

    PoiList vPoi2;

    if (pIPoi->ConvertPoiToGirderlineCoordinate(poi) < pIPoi->ConvertPoiToGirderlineCoordinate(details.poi_fixity))
    {
        pIPoi->GetPointsOfInterestInRange(0, poi, L, &vPoi2);
    }
    else
    {
        pIPoi->GetPointsOfInterestInRange(L, poi, 0, &vPoi2);
    }

    vPoi2.erase(std::remove_if(vPoi2.begin(), vPoi2.end(), [](const pgsPointOfInterest& i) {
        return i.HasAttribute(POI_BOUNDARY_PIER);
        }), vPoi2.end());




    ColumnIndexType nCols = 16;
    CString label{ reactionLocation.PierLabel.c_str() };
    label += _T(" - ");
    label += _T("Interval ");
    label += std::to_string(intervalIdx).c_str();
    label += _T(" (Previous Interval: ");
    label += std::to_string(intervalIdx - 1).c_str();
    label += _T(" - ");
    label += pIntervals->GetDescription(intervalIdx - 1).c_str();
    label += _T(")");

    rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, label);

    p_table->SetNumberOfHeaderRows(2);

    p_table->SetRowSpan(0, 0, 2);
    (*p_table)(0, 0) << COLHDR(_T("Location"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
    p_table->SetRowSpan(0, 1, 1);

    p_table->SetColumnSpan(0, 1, 5);
    (*p_table)(0, 1) << pProductLoads->GetProductLoadName(pgsTypes::pftCreep);
    (*p_table)(1, 1) << _T("Incremental") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 2) << _T("Cumulative") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 3) << COLHDR(Sub2(symbol(delta), _T("d")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
    (*p_table)(1, 4) << Sub2(symbol(epsilon), _T("avg")) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 5) << COLHDR(Sub2(symbol(DELTA), _T("s")) << Super2(_T("x10"), _T("3")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

    p_table->SetColumnSpan(0, 6, 5);
    (*p_table)(0, 6) << pProductLoads->GetProductLoadName(pgsTypes::pftShrinkage);
    (*p_table)(1, 6) << _T("Incremental") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 7) << _T("Cumulative") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 8) << COLHDR(Sub2(symbol(delta), _T("d")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
    (*p_table)(1, 9) << Sub2(symbol(epsilon), _T("avg")) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 10) << COLHDR(Sub2(symbol(DELTA), _T("s")) << Super2(_T("x10"), _T("3")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

    p_table->SetColumnSpan(0, 11, 5);
    (*p_table)(0, 11) << pProductLoads->GetProductLoadName(pgsTypes::pftRelaxation);
    (*p_table)(1, 11) << _T("Incremental") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 12) << _T("Cumulative") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("36"));
    (*p_table)(1, 13) << COLHDR(Sub2(symbol(delta), _T("d")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
    (*p_table)(1, 14) << Sub2(symbol(epsilon), _T("avg")) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 15) << COLHDR(Sub2(symbol(DELTA), _T("s")) << Super2(_T("x10"), _T("3")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

    pgsPointOfInterest p0, p1;

    RowIndexType row = p_table->GetNumberOfHeaderRows();

    (*p_table)(row, 0) << location.SetValue(POI_SPAN, vPoi2[0]);
    (*p_table)(row, 1) << _T("N/A");
    (*p_table)(row, 2) << _T("N/A");
    (*p_table)(row, 3) << _T("N/A");
    (*p_table)(row, 4) << _T("N/A");
    (*p_table)(row, 5) << _T("N/A");
    (*p_table)(row, 6) << _T("N/A");
    (*p_table)(row, 7) << _T("N/A");
    (*p_table)(row, 8) << _T("N/A");
    (*p_table)(row, 9) << _T("N/A");
    (*p_table)(row, 10) << _T("N/A");
    (*p_table)(row, 11) << _T("N/A");
    (*p_table)(row, 12) << _T("N/A");
    (*p_table)(row, 13) << _T("N/A");
    (*p_table)(row, 14) << _T("N/A");
    (*p_table)(row++, 15) << _T("N/A");


    for (IndexType idx = 1, nPoi = vPoi2.size(); idx < nPoi; idx++)
    {

        p0 = vPoi2[idx - 1];
        p1 = vPoi2[idx];

        (*p_table)(row, 0) << location.SetValue(POI_SPAN, p1);

        std::vector<pgsTypes::ProductForceType> td_types{
            pgsTypes::ProductForceType::pftCreep, pgsTypes::ProductForceType::pftShrinkage, pgsTypes::ProductForceType::pftRelaxation};


        for (IndexType ty = 0, nTypes = td_types.size(); ty < nTypes; ty++)
        {
            TIMEDEPENDENTSHEARDEFORMATIONPARAMETERS sf_params;
            pBearing->GetBearingTimeDependentShearDeformationParameters(poi, intervalIdx, p0, p1, td_types[ty], &sf_params);

            (*p_table)(row, ty * 5 + 1) << std::to_wstring(sf_params.inc_strain_bot_girder1 * 1E6);
            (*p_table)(row, ty * 5 + 2) << std::to_wstring(sf_params.cum_strain_bot_girder1 * 1E6);
            (*p_table)(row, ty * 5 + 3) << deflection.SetValue(sf_params.delta_d);
            (*p_table)(row, ty * 5 + 4) << std::to_wstring(sf_params.average_cumulative_strain * 1E6);
            (*p_table)(row, ty * 5 + 5) << deflection.SetValue(sf_params.inc_shear_def * 1E3);
        }

        row++;

    }

    pBearing->GetBearingTotalTimeDependentShearDeformation(poi, intervalIdx, &details);

    p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CJ_LEFT));
    p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableCellStyle(CJ_LEFT));

    (*p_table)(row, 0) << symbol(SIGMA) << Sub2(symbol(DELTA), _T("s"));


    p_table->SetColumnSpan(row, 1, 5);
    (*p_table)(row, 1) << deflection.SetValue(details.cumulative_creep) << rptNewLine;
    p_table->SetColumnSpan(row, 6, 5);
    (*p_table)(row, 6) << deflection.SetValue(details.cumulative_shrinkage) << rptNewLine;
    p_table->SetColumnSpan(row, 11, 5);
    (*p_table)(row, 11) << deflection.SetValue(details.cumulative_relaxation) << rptNewLine;


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
void CTimeStepShearDeformationTable::MakeCopy(const CTimeStepShearDeformationTable& rOther)
{
    // Add copy code here...
}

void CTimeStepShearDeformationTable::MakeAssignment(const CTimeStepShearDeformationTable& rOther)
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
