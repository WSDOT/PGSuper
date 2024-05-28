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
#include <IFace\PrestressForce.h>
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
        nCols += 6; // thermal expansion parameters


        GET_IFACE2(pBroker, ILossParameters, pLossParams);
        if (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::GENERAL_LUMPSUM)
        {
            nCols += 7; // lump sum time-dependent
        }
        else
        {
            GET_IFACE2(pBroker, ILossParameters, pLossParams);
            if (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP)
            {
                nCols += 3;
            }
            else
            {
                nCols += 11; // creep, shrinkage & relaxation parameters
            }
        }
    }
    else
    {
        nCols += 1; // total deformation
    }

    return nCols;
}


RowIndexType ConfigureBearingShearDeformationTableHeading(IBroker* pBroker, rptRcTable* p_table, pgsTypes::AnalysisType analysisType, 
    IEAFDisplayUnits* pDisplayUnits, SHEARDEFORMATIONDETAILS* pDetails, bool bDetail)

{


    RowIndexType rowSpan = 3;
    GET_IFACE2(pBroker, ILossParameters, pLossParams);
    if (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::GENERAL_LUMPSUM)
    {
        rowSpan = 2;
    }

    if (bDetail)
    {
        p_table->SetNumberOfHeaderRows(rowSpan);
    }
    else
    {
        p_table->SetNumberOfHeaderRows(1);
    }

    ColumnIndexType col = 0;

    if (bDetail)
    {
        p_table->SetRowSpan(0, col, rowSpan);
    }
    else
    {
        p_table->SetRowSpan(0, col, 1);
    }
    
    (*p_table)(0, col++) << _T("");

    if (bDetail)
    {

        p_table->SetRowColumnSpan(0, col, rowSpan-1, 5);
        (*p_table)(0, col) << _T("Thermal Deformation Parameters");
        (*p_table)(rowSpan - 1, col++) << Sub2(symbol(DELTA), _T("0"));
        (*p_table)(rowSpan - 1, col++) << symbol(alpha) << rptNewLine << _T("(") << pDisplayUnits->GetTemperatureUnit().UnitOfMeasure.UnitTag() << _T(")") << Super(_T("-1"));
        (*p_table)(rowSpan - 1, col++) << COLHDR(Sub2(_T("L"), _T("pf")), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
        (*p_table)(rowSpan - 1, col++) << COLHDR(Sub2(_T("T"), _T("max")), rptTemperatureUnitTag, pDisplayUnits->GetTemperatureUnit());
        (*p_table)(rowSpan - 1, col++) << COLHDR(Sub2(_T("T"), _T("min")), rptTemperatureUnitTag, pDisplayUnits->GetTemperatureUnit());

        p_table->SetRowSpan(0, col, rowSpan);
        (*p_table)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("thermal")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());


        if (pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP)
        {
            p_table->SetRowColumnSpan(0, col, rowSpan - 1, 5);
            (*p_table)(0, col) << _T("Girder Properties");
            (*p_table)(rowSpan - 1, col++) << COLHDR(Sub2(_T("e"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
            (*p_table)(rowSpan - 1, col++) << COLHDR(Sub2(_T("I"), _T("xx")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*p_table)(rowSpan - 1, col++) << COLHDR(Sub2(_T("y"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
            (*p_table)(rowSpan - 1, col++) << COLHDR(Sub2(_T("A"), _T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*p_table)(rowSpan - 1, col++) << COLHDR(_T("r"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
        }

        if (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::GENERAL_LUMPSUM)
        {
            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << _T("Time-Dependent Deformations");
            (*p_table)(1, col++) << COLHDR(symbol(SUM) << Sub2(symbol(DELTA) << _T("L"), _T("ten")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
            (*p_table)(1, col++) << COLHDR(symbol(SUM) << Sub2(symbol(DELTA) << _T("L"), _T("bf")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
        }
        else
        {
            if (pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP)
            {
                p_table->SetColumnSpan(0, col, 6);
                (*p_table)(0, col) << _T("Time-Dependent Deformations");
                p_table->SetColumnSpan(1, col, 3);
                (*p_table)(1, col) << Sub2(symbol(DELTA) << _T("L"), _T("ten"));
                (*p_table)(2, col++) << COLHDR(_T("Creep"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
                (*p_table)(2, col++) << COLHDR(_T("Shrinkage"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
                (*p_table)(2, col++) << COLHDR(_T("Relaxation"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
                p_table->SetColumnSpan(1, col, 3);
                (*p_table)(1, col) << Sub2(symbol(DELTA) << _T("L"), _T("bf"));
                (*p_table)(2, col++) << COLHDR(_T("Creep"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
                (*p_table)(2, col++) << COLHDR(_T("Shrinkage"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
                (*p_table)(2, col++) << COLHDR(_T("Relaxation"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
            }
            else
            {
                p_table->SetColumnSpan(0, col, 3);
                (*p_table)(0, col) << _T("Time-Dependent Deformations");
                p_table->SetRowSpan(1, col, 2);
                (*p_table)(1, col++) << COLHDR(_T("Creep"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
                p_table->SetRowSpan(1, col, 2);
                (*p_table)(1, col++) << COLHDR(_T("Shrinkage"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
                p_table->SetRowSpan(1, col, 2);
                (*p_table)(1, col++) << COLHDR(_T("Relaxation"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
            }

            
        }
    }
    else
    {
        (*p_table)(0, col) << Sub2(symbol(DELTA),_T("total"));
    }


    return p_table->GetNumberOfHeaderRows();
}





//======================== OPERATIONS =======================================
rptRcTable* CBearingShearDeformationTable::BuildBearingShearDeformationTable(IBroker* pBroker, const CGirderKey& girderKey, pgsTypes::AnalysisType analysisType,
    bool bDesign, IEAFDisplayUnits* pDisplayUnits, bool bDetail, bool bCold, SHEARDEFORMATIONDETAILS* details) const
{

    GET_IFACE2(pBroker, IBridge, pBridge);

    GET_IFACE2(pBroker, IIntervals, pIntervals);
    IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
    IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;


    GET_IFACE2(pBroker, IPointOfInterest, pPOI);



    INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
    INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), false);
    INIT_UV_PROTOTYPE(rptLength4UnitValue, I, pDisplayUnits->GetMomentOfInertiaUnit(), false);
    INIT_UV_PROTOTYPE(rptLength2UnitValue, A, pDisplayUnits->GetAreaUnit(), false);
    INIT_UV_PROTOTYPE(rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false);
    INIT_UV_PROTOTYPE(rptTemperatureUnitValue, temperature, pDisplayUnits->GetTemperatureUnit(), false);


    ColumnIndexType nCols = GetBearingTableColumnCount(pBroker, girderKey, analysisType, details, bDetail);

    CString label{_T("")};


    if (bCold)
    {
        label = _T("Shear Deformations - Cold Climate");
    }
    else
    {
        label = _T("Shear Deformations - Moderate Climate");
    }

    
    
    rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, label);
    RowIndexType row = ConfigureBearingShearDeformationTableHeading(pBroker, p_table, analysisType, pDisplayUnits, details, bDetail);

    p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
    p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));


    



    // get poi where pier Reactions occur
    PoiList vPoi;
    std::vector<CGirderKey> vGirderKeys;
    pBridge->GetGirderline(girderKey.girderIndex, details->startGroup, details->endGroup, &vGirderKeys);
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

        GET_IFACE2(pBroker, IBearingDesignParameters, pBearing);


        details->time_dependent = pBearing->GetTimeDependentShearDeformation(poi, startPierIdx, details);
        pBearing->GetThermalExpansionDetails(poi, details);


        INIT_UV_PROTOTYPE(rptLengthUnitValue, Deflection, pDisplayUnits->GetDeflectionUnit(), false);
        INIT_UV_PROTOTYPE(rptLengthUnitValue, Span, pDisplayUnits->GetSpanLengthUnit(), false);
        INIT_UV_PROTOTYPE(rptTemperatureUnitValue, Temp, pDisplayUnits->GetTemperatureUnit(), false);
        INIT_UV_PROTOTYPE(rptStressUnitValue, Stress, pDisplayUnits->GetStressUnit(), false);
        INIT_UV_PROTOTYPE(rptLength4UnitValue, I, pDisplayUnits->GetMomentOfInertiaUnit(), false);
        INIT_UV_PROTOTYPE(rptAreaUnitValue, A, pDisplayUnits->GetAreaUnit(), false);

        if (bDetail)
        {
            (*p_table)(row, col++) << details->percentExpansion;
            if (pDisplayUnits->GetUnitMode() == eafTypes::UnitMode::umUS)
            {
                (*p_table)(row, col++) << 1.0 / ((1.0 / details->thermal_expansion_coefficient) * 9.0 / 5.0 + 32.0);
            }
            else
            {
                (*p_table)(row, col++) << details->thermal_expansion_coefficient;
            }
            (*p_table)(row, col++) << Span.SetValue(details->length_pf);
            if (bCold)
            {
                (*p_table)(row, col++) << Temp.SetValue(details->max_design_temperature_cold);
                (*p_table)(row, col++) << Temp.SetValue(details->min_design_temperature_cold);
                (*p_table)(row, col++) << Deflection.SetValue(details->thermal_expansion_cold);
            }
            else
            {
                (*p_table)(row, col++) << Temp.SetValue(details->max_design_temperature_moderate);
                (*p_table)(row, col++) << Temp.SetValue(details->min_design_temperature_moderate);
                (*p_table)(row, col++) << Deflection.SetValue(details->thermal_expansion_moderate);
            }


            GET_IFACE2(pBroker, ILossParameters, pLossParams);
            if (pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP)
            {
                (*p_table)(row, col++) << Deflection.SetValue(details->yb);
                (*p_table)(row, col++) << Deflection.SetValue(details->ep);
                (*p_table)(row, col++) << I.SetValue(details->Ixx);
                (*p_table)(row, col++) << A.SetValue(details->Ag);
                (*p_table)(row, col++) << Deflection.SetValue(details->r);
            }

            if (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::GENERAL_LUMPSUM)
            {
                (*p_table)(row, col++) << Deflection.SetValue(details->tendon_shortening);
                (*p_table)(row, col++) << Deflection.SetValue(details->time_dependent);
            }
            else
            {
                if (pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP)
                {
                    (*p_table)(row, col++) << Deflection.SetValue(details->tendon_creep);
                    (*p_table)(row, col++) << Deflection.SetValue(details->tendon_shrinkage);
                    (*p_table)(row, col++) << Deflection.SetValue(details->tendon_relaxation);
                }

                (*p_table)(row, col++) << Deflection.SetValue(details->creep);
                (*p_table)(row, col++) << Deflection.SetValue(details->shrinkage);
                (*p_table)(row, col) << Deflection.SetValue(details->relaxation);
            }
        }
        else
        {
            if (bCold)
            {
                (*p_table)(row, col++) << Deflection.SetValue(details->total_shear_deformation_cold);
            }
            else
            {
                (*p_table)(row, col++) << Deflection.SetValue(details->total_shear_deformation_moderate);
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
