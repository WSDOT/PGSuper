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
rptRcTable* CTimeStepShearDeformationTable::BuildTimeStepShearDeformationTable(IBroker* pBroker, TDSHEARDEFORMATIONDETAILS* pDetails) const
{
    
    /// gets poi where reaction

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

    CGirderKey girderKey = pDetails->reactionLocation.GirderKey;

    pBearing->GetBearingTableParameters(girderKey, &details);

    ColumnIndexType nCols = 16;
    CString label{ pDetails->reactionLocation.PierLabel.c_str() };
    label += _T(" - ");
    label += _T("Interval ");
    label += std::to_string(pDetails->interval).c_str();
    label += _T(" (Previous Interval: ");
    label += std::to_string(pDetails->interval - 1).c_str();
    label += _T(" - ");
    label += pIntervals->GetDescription(pDetails->interval - 1).c_str();
    label += _T(")");

    rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, label);

    p_table->SetNumberOfHeaderRows(2);

    p_table->SetRowSpan(0, 0, 2);
    (*p_table)(0, 0) << COLHDR(_T("Location"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
    p_table->SetRowSpan(0, 1, 2); //?
    (*p_table)(0, 1) << COLHDR(Sub2(symbol(delta), _T("d")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
    p_table->SetRowSpan(0, 2, 1); //?

    p_table->SetColumnSpan(0, 2, 4);
    (*p_table)(0, 2) << pProductLoads->GetProductLoadName(pgsTypes::pftCreep);
    (*p_table)(1, 2) << _T("Incremental") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 3) << _T("Cumulative") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 4) << Sub2(symbol(epsilon), _T("avg")) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 5) << COLHDR(Sub2(symbol(DELTA), _T("s")) << Super2(_T("x10"), _T("3")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

    p_table->SetColumnSpan(0, 6, 4);
    (*p_table)(0, 6) << pProductLoads->GetProductLoadName(pgsTypes::pftShrinkage);
    (*p_table)(1, 6) << _T("Incremental") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 7) << _T("Cumulative") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 8) << Sub2(symbol(epsilon), _T("avg")) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 9) << COLHDR(Sub2(symbol(DELTA), _T("s")) << Super2(_T("x10"), _T("3")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

    p_table->SetColumnSpan(0, 10, 4);
    (*p_table)(0, 10) << pProductLoads->GetProductLoadName(pgsTypes::pftRelaxation);
    (*p_table)(1, 10) << _T("Incremental") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 11) << _T("Cumulative") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("36"));
    (*p_table)(1, 12) << Sub2(symbol(epsilon), _T("avg")) << Super2(_T("x10"), _T("6"));
    (*p_table)(1, 13) << COLHDR(Sub2(symbol(DELTA), _T("s")) << Super2(_T("x10"), _T("3")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

    pgsPointOfInterest p0, p1;

    RowIndexType row = p_table->GetNumberOfHeaderRows();

    (*p_table)(row, 0) << location.SetValue(POI_SPAN, pDetails->rPoi);
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
    (*p_table)(row++, 13) << _T("N/A");



    for (auto& r : pDetails->td_diff_elems)
    {
        (*p_table)(row, 0) << location.SetValue(POI_SPAN, r.poi);
        (*p_table)(row, 1) << deflection.SetValue(r.delta_d * 1E3);
        (*p_table)(row, 2) << std::to_wstring(r.creep[0] * 1E6);
        (*p_table)(row, 3) << std::to_wstring(r.creep[1] * 1E6);
        (*p_table)(row, 4) << std::to_wstring(r.creep[2] * 1E6);
        (*p_table)(row, 5) << deflection.SetValue(r.creep[3] * 1E3);
        (*p_table)(row, 6) << std::to_wstring(r.shrinkage[0] * 1E6);
        (*p_table)(row, 7) << std::to_wstring(r.shrinkage[1] * 1E6);
        (*p_table)(row, 8) << std::to_wstring(r.shrinkage[2] * 1E6);
        (*p_table)(row, 9) << deflection.SetValue(r.shrinkage[3] * 1E3);
        (*p_table)(row, 10) << std::to_wstring(r.relaxation[0] * 1E6);
        (*p_table)(row, 11) << std::to_wstring(r.relaxation[1] * 1E6);
        (*p_table)(row, 12) << std::to_wstring(r.relaxation[2] * 1E6);
        (*p_table)(row, 13) << deflection.SetValue(r.relaxation[3] * 1E3);
        row++;
    }

    p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CJ_LEFT));
    p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableCellStyle(CJ_LEFT));

    p_table->SetColumnSpan(row, 0, 2);
    (*p_table)(row, 0) << symbol(SIGMA) << Sub2(symbol(DELTA), _T("s"));
    p_table->SetColumnSpan(row, 2, 4);
    (*p_table)(row, 2) << deflection.SetValue(pDetails->creep) << rptNewLine;
    p_table->SetColumnSpan(row, 6, 4);
    (*p_table)(row, 6) << deflection.SetValue(pDetails->shrinkage) << rptNewLine;
    p_table->SetColumnSpan(row, 10, 4);
    (*p_table)(row, 10) << deflection.SetValue(pDetails->relaxation) << rptNewLine;

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
