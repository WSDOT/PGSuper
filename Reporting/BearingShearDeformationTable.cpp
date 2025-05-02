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

#include "StdAfx.h"
#include <Reporting\BearingShearDeformationTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>

#include <IFace/Tools.h>
#include <EAF/EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>


ColumnIndexType CBearingShearDeformationTable::GetBearingTableColumnCount(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey,
    SHEARDEFORMATIONDETAILS* details, bool bDetail) const
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

RowIndexType ConfigureBearingShearDeformationTableHeading(std::shared_ptr<WBFL::EAF::Broker> pBroker, rptRcTable* p_table, 
    std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, SHEARDEFORMATIONDETAILS* pDetails, bool bDetail)

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
            (*p_table)(rowSpan - 1, col++) << COLHDR(Sub2(_T("y"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
            (*p_table)(rowSpan - 1, col++) << COLHDR(Sub2(_T("e"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
            (*p_table)(rowSpan - 1, col++) << COLHDR(Sub2(_T("I"), _T("xx")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*p_table)(rowSpan - 1, col++) << COLHDR(Sub2(_T("A"), _T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*p_table)(rowSpan - 1, col++) << COLHDR(_T("r"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
        }

        if (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::GENERAL_LUMPSUM)
        {
            p_table->SetColumnSpan(0, col, 2);
            (*p_table)(0, col) << _T("Time-Dependent Deformations");
            (*p_table)(1, col++) << COLHDR(symbol(SUM) << Sub2(symbol(DELTA) << _T("L"), _T("ten")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
            (*p_table)(1, col++) << COLHDR(symbol(SUM) << Sub2(symbol(DELTA), _T("s")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
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
                (*p_table)(1, col) << Sub2(symbol(DELTA), _T("s"));
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
        (*p_table)(0, col) << COLHDR(Sub2(symbol(DELTA), _T("total")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
    }


    return p_table->GetNumberOfHeaderRows();
}

rptRcTable* CBearingShearDeformationTable::BuildBearingShearDeformationTable(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CGirderKey& girderKey,
    std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, bool bDetail, bool bCold, SHEARDEFORMATIONDETAILS* details) const
{
    
    GET_IFACE2(pBroker, IBearingDesignParameters, pBearing);
    
    pBearing->GetTimeDependentShearDeformation(girderKey, details);

    GET_IFACE2(pBroker, IIntervals, pIntervals);
    IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;



    // Build table
    INIT_UV_PROTOTYPE(rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false);
    INIT_UV_PROTOTYPE(rptForceUnitValue, reactu, pDisplayUnits->GetShearUnit(), false);

    INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
    INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), false);
    INIT_UV_PROTOTYPE(rptLength4UnitValue, I, pDisplayUnits->GetMomentOfInertiaUnit(), false);
    INIT_UV_PROTOTYPE(rptLength2UnitValue, A, pDisplayUnits->GetAreaUnit(), false);
    INIT_UV_PROTOTYPE(rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false);
    INIT_UV_PROTOTYPE(rptTemperatureUnitValue, temperature, pDisplayUnits->GetTemperatureUnit(), false);


    ColumnIndexType nCols = GetBearingTableColumnCount(pBroker, girderKey, details, bDetail);

    CString label{_T("")};


    if (bCold)
    {
        label = _T("Bearing Design Shear Deformations - Cold Climate");
    }
    else
    {
        label = _T("Bearing Design Shear Deformations - Moderate Climate");
    }

    
    
    rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, label);
    RowIndexType row = ConfigureBearingShearDeformationTableHeading(pBroker, p_table, pDisplayUnits, details, bDetail);

    p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
    p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

    
    for (auto& bearing:details->brg_details)

    {
        pBearing->GetThermalExpansionDetails(girderKey, &bearing);

        ColumnIndexType col = 0;

        (*p_table)(row, col++) << bearing.reactionLocation.PierLabel;

        INIT_UV_PROTOTYPE(rptLengthUnitValue, Deflection, pDisplayUnits->GetDeflectionUnit(), false);
        INIT_UV_PROTOTYPE(rptLengthUnitValue, Span, pDisplayUnits->GetSpanLengthUnit(), false);
        INIT_UV_PROTOTYPE(rptTemperatureUnitValue, Temp, pDisplayUnits->GetTemperatureUnit(), false);
        INIT_UV_PROTOTYPE(rptStressUnitValue, Stress, pDisplayUnits->GetStressUnit(), false);
        INIT_UV_PROTOTYPE(rptLength4UnitValue, I, pDisplayUnits->GetMomentOfInertiaUnit(), false);
        INIT_UV_PROTOTYPE(rptAreaUnitValue, A, pDisplayUnits->GetAreaUnit(), false);

        if (bDetail)
        {
            (*p_table)(row, col++) << bearing.percentExpansion;
            if (pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US)
            {
                (*p_table)(row, col++) << 1.0 / ((1.0 / bearing.thermal_expansion_coefficient) * 9.0 / 5.0 + 32.0);
            }
            else
            {
                (*p_table)(row, col++) << bearing.thermal_expansion_coefficient;
            }
            (*p_table)(row, col++) << Span.SetValue(bearing.length_pf);
            if (bCold)
            {
                (*p_table)(row, col++) << Temp.SetValue(bearing.max_design_temperature_cold);
                (*p_table)(row, col++) << Temp.SetValue(bearing.min_design_temperature_cold);
                (*p_table)(row, col++) << Deflection.SetValue(bearing.thermal_expansion_cold);
            }
            else
            {
                (*p_table)(row, col++) << Temp.SetValue(bearing.max_design_temperature_moderate);
                (*p_table)(row, col++) << Temp.SetValue(bearing.min_design_temperature_moderate);
                (*p_table)(row, col++) << Deflection.SetValue(bearing.thermal_expansion_moderate);
            }


            GET_IFACE2(pBroker, ILossParameters, pLossParams);
            if (pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP)
            {
                (*p_table)(row, col++) << Deflection.SetValue(bearing.yb);
                (*p_table)(row, col++) << Deflection.SetValue(bearing.ep);
                (*p_table)(row, col++) << I.SetValue(bearing.Ixx);
                (*p_table)(row, col++) << A.SetValue(bearing.Ag);
                (*p_table)(row, col++) << Deflection.SetValue(bearing.r);
            }

            if (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::GENERAL_LUMPSUM)
            {
                (*p_table)(row, col++) << Deflection.SetValue(bearing.tendon_shortening);
                (*p_table)(row, col++) << Deflection.SetValue(bearing.time_dependent);
            }
            else
            {
                if (pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP)
                {
                    (*p_table)(row, col++) << Deflection.SetValue(bearing.tendon_creep);
                    (*p_table)(row, col++) << Deflection.SetValue(bearing.tendon_shrinkage);
                    (*p_table)(row, col++) << Deflection.SetValue(bearing.tendon_relaxation);
                }
                (*p_table)(row, col++) << Deflection.SetValue(bearing.creep);
                (*p_table)(row, col++) << Deflection.SetValue(bearing.shrinkage);
                (*p_table)(row, col) << Deflection.SetValue(bearing.relaxation);
            }
        }
        else
        {
            if (bCold)
            {
                (*p_table)(row, col++) << Deflection.SetValue(bearing.total_shear_deformation_cold);
            }
            else
            {
                (*p_table)(row, col++) << Deflection.SetValue(bearing.total_shear_deformation_moderate);
            }
        }

        row++;
    }
    

    return p_table;

}
