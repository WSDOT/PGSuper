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
#include <Reporting\BearingDesignDetailsChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\BearingReactionTable.h>
#include <Reporting\BearingRotationTable.h>
#include <Reporting\BearingShearDeformationTable.h>
#include <Reporting\PrestressRotationTable.h>
#include <Reporting\UserReactionTable.h>
#include <Reporting\UserRotationTable.h>

#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\VehicularLoadReactionTable.h>
#include <Reporting\CombinedReactionTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\BearingDesignParameters.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Intervals.h>
#include <IFace\DistributionFactors.h>

#include <PgsExt\PierData2.h>
#include <Reporting/BearingDesignPropertiesTable.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CBearingDesignParametersChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CBearingDesignDetailsChapterBuilder::CBearingDesignDetailsChapterBuilder(bool bSelect) :
    CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CBearingDesignDetailsChapterBuilder::GetName() const
{
    return TEXT("Bearing Design Details");
}

rptChapter* CBearingDesignDetailsChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec, Uint16 level) const
{
    auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
    CComPtr<IBroker> pBroker;
    pGirderRptSpec->GetBroker(&pBroker);
    const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());


    rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec, level);

    GET_IFACE2(pBroker, IUserDefinedLoads, pUDL);
    bool are_user_loads = pUDL->DoUserLoadsExist(girderKey);

    GET_IFACE2(pBroker, IBearingDesign, pBearingDesign);

    bool bIncludeImpact = pBearingDesign->BearingLiveLoadReactionsIncludeImpact();

    GET_IFACE2(pBroker, ISpecification, pSpec);

    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
    INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);
    INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true);
    INIT_UV_PROTOTYPE(rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), true);

    GET_IFACE2(pBroker, IBearingDesignParameters, pBearingDesignParameters);
    DESIGNPROPERTIES details;
    pBearingDesignParameters->GetBearingDesignProperties(&details);

    rptParagraph* p = new rptParagraph(rptStyleManager::GetHeadingStyle());
    *pChapter << p;

    *p << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("bearing_orientation_description.png")) << rptNewLine;

    *p << _T("Bearing Design Properties") << rptNewLine;
    p = new rptParagraph;
    *pChapter << p;
    *p << Sub2(_T("F"), _T("y")) << _T(" = ") << stress.SetValue(details.Fy);
    *p << _T(" for steel shims per AASHTO M251 (ASTM A 1011 Gr. 36)") << rptNewLine;
    *p << Sub2(_T("F"), _T("th")) << _T(" = ") << stress.SetValue(details.Fth);
    *p << _T(" LRFD Article 6.6 (Table 6.6.1.2.3-1 for Category A)") << rptNewLine;
    *p << _T("Method B is used per WSDOT Policy (BDM Ch. 9.2)") << rptNewLine;

    ColumnIndexType nCols = 7;

    CString label = _T("Elastomer Shear Modulus (From Table 14.7.6.2-1)");
    rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, label);

    p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
    p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

    ColumnIndexType col = 0;
    p_table->SetNumberOfHeaderRows(2);
    p_table->SetRowSpan(0, col, 2);
    (*p_table)(0, col++) << _T("");
    p_table->SetColumnSpan(0, col, 2);
    (*p_table)(0, col) << _T("50 Hardness");
    (*p_table)(1, col++) << COLHDR(Sub2(_T("G"), _T("min")),
        rptStressUnitTag, pDisplayUnits->GetStressUnit());
    (*p_table)(1, col++) << COLHDR(Sub2(_T("G"), _T("max")),
        rptStressUnitTag, pDisplayUnits->GetStressUnit());
    p_table->SetColumnSpan(0, col, 2);
    (*p_table)(0, col) << _T("60 Hardness");
    (*p_table)(1, col++) << COLHDR(Sub2(_T("G"), _T("min")),
        rptStressUnitTag, pDisplayUnits->GetStressUnit());
    (*p_table)(1, col++) << COLHDR(Sub2(_T("G"), _T("max")),
        rptStressUnitTag, pDisplayUnits->GetStressUnit());
    p_table->SetColumnSpan(0, col, 2);
    (*p_table)(0, col) << _T("70 Hardness");
    (*p_table)(1, col++) << COLHDR(Sub2(_T("G"), _T("min")),
        rptStressUnitTag, pDisplayUnits->GetStressUnit());
    (*p_table)(1, col++) << COLHDR(Sub2(_T("G"), _T("max")),
        rptStressUnitTag, pDisplayUnits->GetStressUnit());


    col = 0;
    (*p_table)(2, col++) << _T("Design Value");
    (*p_table)(2, col++) << stress.SetValue(details.Gmin50);
    (*p_table)(2, col++) << stress.SetValue(details.Gmax50);
    (*p_table)(2, col++) << stress.SetValue(details.Gmin60);
    (*p_table)(2, col++) << stress.SetValue(details.Gmax60);
    (*p_table)(2, col++) << stress.SetValue(details.Gmin70);
    (*p_table)(2, col++) << stress.SetValue(details.Gmax70);

    GET_IFACE2(pBroker, IBridge, pBridge);

    *p << p_table;

    *p << CBearingReactionTable().BuildBearingReactionTable(pBroker, girderKey, pSpec->GetAnalysisType(), bIncludeImpact,
        true, true, are_user_loads, true, pDisplayUnits, true) << rptNewLine;

    *p << CBearingRotationTable().BuildBearingRotationTable(pBroker, girderKey, pSpec->GetAnalysisType(), bIncludeImpact,
        true, true,are_user_loads, true, pDisplayUnits, true, true) << rptNewLine;

    *p << CBearingRotationTable().BuildBearingRotationTable(pBroker, girderKey, pSpec->GetAnalysisType(), bIncludeImpact,
        true, true, are_user_loads, true, pDisplayUnits, true, false) << rptNewLine;

    *p << CBearingShearDeformationTable().BuildBearingShearDeformationTable(pBroker, girderKey, pSpec->GetAnalysisType(), bIncludeImpact,
        true, true, are_user_loads, true, pDisplayUnits, true) << rptNewLine;
    
    SHEARDEFORMATIONDETAILS sf_details;
    pBearingDesignParameters->GetThermalExpansionDetails(girderKey, &sf_details);


    *p << Sub2(symbol(DELTA), _T("temp")) << _T(" = ") << Sub2(symbol(DELTA), _T("0")) << _T(" ") << symbol(TIMES) << _T(" ") << symbol(alpha) << _T(" ") << symbol(TIMES) << _T(" ") << _T("L") << _T(" ");
    *p << symbol(TIMES) << _T(" (") << Sub2(_T("T"),_T("Max Design")) << _T(" - ") << Sub2(_T("T"), _T("Max Design")) << _T(")") << rptNewLine;
    *p << _T("L = ") << length.SetValue(pBridge->GetLength()) << rptNewLine;
    *p << _T("From AASHTO LRFD Sect. 14.7.5.3.2: ") << Sub2(symbol(DELTA),_T("0")) << _T(" = ") << sf_details.percentExpansion << rptNewLine;
    //*p << _T("Moderate Climate: ") << symbol(RIGHT_SINGLE_ARROW) << Sub2(symbol(DELTA), _T("temp")) << _T(" = ") << deflection.SetValue(sf_details.thermalLRFDModerate) << rptNewLine;
    //*p << _T("Cold Climate: ") << symbol(RIGHT_SINGLE_ARROW) << Sub2(symbol(DELTA), _T("temp")) << _T(" = ") << deflection.SetValue(sf_details.thermalLRFDCold) << rptNewLine;
    //*p << _T("From WSDOT BDM Ch. 9: ") << Sub2(symbol(DELTA), _T("0")) << _T(" = 0.75") << rptNewLine;
    //*p << _T("Moderate Climate: ") << symbol(RIGHT_SINGLE_ARROW) << Sub2(symbol(DELTA), _T("temp")) << _T(" = ") << deflection.SetValue(sf_details.thermalBDMModerate) << rptNewLine;
    //*p << _T("Cold Climate: ") << symbol(RIGHT_SINGLE_ARROW) << Sub2(symbol(DELTA), _T("temp")) << _T(" = ") << deflection.SetValue(sf_details.thermalBDMCold) << rptNewLine;

    return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder>CBearingDesignDetailsChapterBuilder::Clone() const
{
    return std::make_unique<CBearingDesignDetailsChapterBuilder>();
}
