///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include <Reporting\BearingDesignSummaryChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\ProductReactionTable.h>
#include <Reporting\BearingRotationTable.h>
#include <Reporting\BearingDesignPropertiesTable.h>
#include <Reporting\BearingReactionTable.h>
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
#include <Reporting/BearingShearDeformationTable.h>

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
CBearingDesignSummaryChapterBuilder::CBearingDesignSummaryChapterBuilder(bool bSelect) :
    CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CBearingDesignSummaryChapterBuilder::GetName() const
{
    return TEXT("Bearing Design Summary");
}

rptChapter* CBearingDesignSummaryChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec, Uint16 level) const
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

    rptParagraph* p = new rptParagraph;
    *pChapter << p;

    *p << _T("-Live loads do not include impact") << rptNewLine << rptNewLine;

    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

    *p << CBearingDesignPropertiesTable().BuildBearingDesignPropertiesTable(pBroker, pDisplayUnits) << rptNewLine;

    *p << Sub2(_T("F"), _T("y")) << _T(" = Steel reinforcement yield strength") << rptNewLine;
    *p << Sub2(_T("F"), _T("th")) << _T(" = Steel reinforcement constant-amplitude fatigue threshold for Detail Category A") << rptNewLine;

    GET_IFACE2(pBroker, ISpecification, pSpec);

    *p << CBearingReactionTable().BuildBearingReactionTable(pBroker, girderKey, pSpec->GetAnalysisType(), bIncludeImpact,
        true, true, are_user_loads, true, pDisplayUnits, false) << rptNewLine;

    *p << Sub2(_T("P"), _T("D")) << _T(" = Dead Load") << rptNewLine;

    *p << Sub2(_T("P"), _T("L")) << _T(" = Vertical Live Load") << rptNewLine;

    GET_IFACE2(pBroker, IBearingDesignParameters, pBearingDesignParameters);
    SHEARDEFORMATIONDETAILS sfDetails;
    pBearingDesignParameters->GetBearingParameters(girderKey, &sfDetails);

    *p << CBearingRotationTable().BuildBearingRotationTable(pBroker, girderKey, pSpec->GetAnalysisType(), bIncludeImpact,
        true, true, are_user_loads,  true, pDisplayUnits, false, true) << rptNewLine;

    *p << Sub2(symbol(theta), _T("s-st")) << _T(" = static component of maximum service limit design rotation") << rptNewLine;
    *p << Sub2(symbol(theta), _T("s-cy")) << _T(" = cyclic component of maximum service limit design rotation") << rptNewLine;
    *p << Sub2(symbol(theta), _T("s")) << _T(" = maximum total service limit design rotation") << rptNewLine;
    *p << _T("*Static rotations include ") << symbol(PLUS_MINUS) << _T("0.005 radians tolerance for uncertainties") << rptNewLine;

    *p << CBearingRotationTable().BuildBearingRotationTable(pBroker, girderKey, pSpec->GetAnalysisType(), bIncludeImpact,
        true, true, are_user_loads, true, pDisplayUnits, false, false) << rptNewLine;

    *p << Sub2(symbol(theta), _T("s-st")) << _T(" = static component of maximum service limit design rotation") << rptNewLine;
    *p << Sub2(symbol(theta), _T("s-cy")) << _T(" = cyclic component of maximum service limit design rotation") << rptNewLine;
    *p << Sub2(symbol(theta), _T("s")) << _T(" = maximum total service limit design rotation") << rptNewLine;
    *p << _T("*Static rotations include ") << symbol(PLUS_MINUS) << _T("0.005 radians tolerance for uncertainties") << rptNewLine;


    bool bCold;
    GET_IFACE2(pBroker, IEnvironment, pEnvironment);
    if (pEnvironment->GetClimateCondition() == pgsTypes::ClimateCondition::Cold)
    {
        bCold = true;
    }
    else
    {
        bCold = false;
    }



    *p << CBearingShearDeformationTable().BuildBearingShearDeformationTable(pBroker, girderKey, pSpec->GetAnalysisType(), true, pDisplayUnits, false, bCold, &sfDetails) << rptNewLine;

    *p << _T("-Deck shrinkage effects are ignored") << rptNewLine;
    *p << _T("-Bearing reset effects are ignored") << rptNewLine;
    *p << _T("-Temperature range is computed based on Procedure A (Article 3.12.2.1)") << rptNewLine << rptNewLine;

    return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder>CBearingDesignSummaryChapterBuilder::Clone() const
{
    return std::make_unique<CBearingDesignSummaryChapterBuilder>();
}
