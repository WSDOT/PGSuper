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

/****************************************************************************
CLASS
   CSpecCheckSummaryChapterBuilder
****************************************************************************/




#include <Reporting\BearingSpecCheckSummaryChapterBuilder.h>
#include <Reporting\SpanGirderBearingReportSpecification.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PsgLib\BridgeDescription2.h>

#include <psgLib/LimitsCriteria.h>
#include <EAF\EAFApp.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>

#include <IFace\DocumentType.h>
#include <IFace\AnalysisResults.h>
#include <IFace\BearingDesignParameters.h>
#include <psgLib/BearingCriteria.h>
#include <Lrfd/BDSManager.h>
#include <EngTools\BearingReporter.h>
#include <AgentTools.h>


CBearingSpecCheckSummaryChapterBuilder::CBearingSpecCheckSummaryChapterBuilder(bool referToDetailsReport,bool bSelect):
CPGSuperChapterBuilder(bSelect),
m_ReferToDetailsReport(referToDetailsReport)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CBearingSpecCheckSummaryChapterBuilder::GetName() const
{
   return TEXT("Bearing Specification Check Summary");
}

rptChapter* CBearingSpecCheckSummaryChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec, Uint16 level) const
{

    auto pBrgRptSpec = std::dynamic_pointer_cast<const CBearingReportSpecification>(pRptSpec);

    rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec, level);

    if (pBrgRptSpec != nullptr)
    {

        rptParagraph* pPara = new rptParagraph;
        *pChapter << pPara;


        const ReactionLocation& reactionLocation = pBrgRptSpec->GetReactionLocation();

        auto pBroker = EAFGetBroker();
        GET_IFACE2(pBroker, ILibrary, pLib);
        GET_IFACE2(pBroker, ISpecification, pSpec);
        const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

        const WBFL::EngTools::BearingProjectCriteria& proj_criteria = pSpecEntry->GetBearingCriteria();

        const BearingCriteria pgs_criteria = pSpecEntry->GetBearingCriteria();

        if (pgs_criteria.bCheck)
        {

            GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);

            pgsTypes::PierFaceType face;
            if (reactionLocation.Face == PierReactionFaceType::rftAhead)
            {
                face = pgsTypes::PierFaceType::Ahead;
            }
            else
            {
                face = pgsTypes::PierFaceType::Back;
            }

            const auto pBearingData = pIBridgeDesc->GetBearingData(
                reactionLocation.PierIdx, face,
                reactionLocation.GirderKey.girderIndex);

            GET_IFACE2(pBroker, IBearingDesignParameters, pBearingDesignParameters);

            WBFL::EngTools::Bearing Bearing, tBearing;
            WBFL::EngTools::BearingLoads BearingLoads, tBearingLoads;


            WBFL::EngTools::BearingCalculator bearingCalculator;
            const auto& spec = WBFL::LRFD::BDSManager::GetEdition();

            pBearingDesignParameters->SetBearingDesignData(*pBearingData, reactionLocation, true, &Bearing, &BearingLoads);
            pBearingDesignParameters->SetBearingDesignData(*pBearingData, reactionLocation, false, &tBearing, &tBearingLoads);

            const WBFL::EngTools::BearingDesignCriteria criteria(Bearing, BearingLoads, spec, proj_criteria);
            const WBFL::EngTools::BearingDesignCriteria tCriteria(tBearing, tBearingLoads, spec, proj_criteria);

            const WBFL::EngTools::BearingCheckArtifact artifact = bearingCalculator.CheckBearing(Bearing, BearingLoads, criteria);
            const WBFL::EngTools::BearingCheckArtifact tArtifact = bearingCalculator.CheckBearing(tBearing, tBearingLoads, tCriteria);

            if (spec < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims)
            {
                *pPara << rptNewLine << _T("Bearing specification check not available for the selected BDS version");
            }
            else if (pBearingData->Shape == BearingShape::bsRound)
            {
                *pPara << rptNewLine << _T("Bearing specification check not available for round bearings");
            }
            else if (pBearingData->DefinitionType == BearingDefinitionType::btBasic)
            {
                *pPara << rptNewLine << _T("Could not evaluate bearing because it is not defined in sufficient detail");
            }
            else
            {

                CEAFApp* pApp = EAFGetApp();
                const WBFL::Units::IndirectMeasure* pDispUnits = pApp->GetDisplayUnits();
                std::unique_ptr<WBFL::EngTools::BearingReporter> brgReporter;

                if (criteria.AnalysisMethod == WBFL::EngTools::BearingAnalysisMethod::MethodA)
                {
                    brgReporter->ReportBearingSpecCheckSummaryA(pChapter, pPara, artifact);

                }
                else
                {
                    brgReporter->ReportBearingSpecCheckSummaryB(pChapter, pPara, artifact, &tArtifact);
                }

            }
        }
        else
        {
            *pPara << rptNewLine << _T("Bearing Spec Check is not enabled. See 'Spec Checking & Design' tab in Project Criteria Library");
        }

        return pChapter;
    }
    

    // Report multiple bearings
    auto pMultiBearingRptSpec = std::dynamic_pointer_cast<const CMultiBearingReportSpecification>(pRptSpec);
    if (pMultiBearingRptSpec != nullptr)
    {
        const std::vector<ReactionLocation>& reactionLocations(pMultiBearingRptSpec->GetReactionLocations());

        // Build chapter and fill it
        pChapter = CPGSuperChapterBuilder::Build(pMultiBearingRptSpec, level);

        auto pBroker = EAFGetBroker();
        GET_IFACE2(pBroker, ILibrary, pLib);
        GET_IFACE2(pBroker, ISpecification, pSpec);
        const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

        const WBFL::EngTools::BearingProjectCriteria& proj_criteria = pSpecEntry->GetBearingCriteria();

        const BearingCriteria pgs_criteria = pSpecEntry->GetBearingCriteria();

        rptParagraph* pParagraph = new rptParagraph;
        *pChapter << pParagraph;

        if (pgs_criteria.bCheck)
        {
            for (std::vector<ReactionLocation>::const_iterator it =
                reactionLocations.begin(); it != reactionLocations.end(); it++)
            {
                const auto& reactionLocation(*it);

                *pParagraph << _T("Results for ") << GIRDER_LABEL(reactionLocation.GirderKey);
                *pParagraph << _T(" - ") << reactionLocation.PierLabel;
                *pParagraph << rptNewLine;

                GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);

                pgsTypes::PierFaceType face;
                if (reactionLocation.Face == PierReactionFaceType::rftAhead)
                {
                    face = pgsTypes::PierFaceType::Ahead;
                }
                else
                {
                    face = pgsTypes::PierFaceType::Back;
                }

                const auto pBearingData = pIBridgeDesc->GetBearingData(
                    reactionLocation.PierIdx, face,
                    reactionLocation.GirderKey.girderIndex);

                GET_IFACE2(pBroker, IBearingDesignParameters, pBearingDesignParameters);

                WBFL::EngTools::Bearing Bearing, tBearing;
                WBFL::EngTools::BearingLoads BearingLoads, tBearingLoads;


                WBFL::EngTools::BearingCalculator bearingCalculator;
                const auto& spec = WBFL::LRFD::BDSManager::GetEdition();

                pBearingDesignParameters->SetBearingDesignData(*pBearingData, reactionLocation, true, &Bearing, &BearingLoads);
                pBearingDesignParameters->SetBearingDesignData(*pBearingData, reactionLocation, false, &tBearing, &tBearingLoads);

                const WBFL::EngTools::BearingDesignCriteria criteria(Bearing, BearingLoads, spec, proj_criteria);
                const WBFL::EngTools::BearingDesignCriteria tCriteria(tBearing, tBearingLoads, spec, proj_criteria);

                const WBFL::EngTools::BearingCheckArtifact artifact = bearingCalculator.CheckBearing(Bearing, BearingLoads, criteria);
                const WBFL::EngTools::BearingCheckArtifact tArtifact = bearingCalculator.CheckBearing(tBearing, tBearingLoads, tCriteria);

                if (spec < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims)
                {
                    *pParagraph << rptNewLine << _T("Bearing specification check not available for the selected BDS version");
                }
                else if (pBearingData->Shape == BearingShape::bsRound)
                {
                    *pParagraph << rptNewLine << _T("Bearing specification check not available for round bearings");
                }
                else if (pBearingData->DefinitionType == BearingDefinitionType::btBasic)
                {
                    *pParagraph << rptNewLine << _T("Could not evaluate bearing because it is not defined in sufficient detail");
                }
                else
                {
                    std::unique_ptr<WBFL::EngTools::BearingReporter> brgReporter;

                    if (criteria.AnalysisMethod == WBFL::EngTools::BearingAnalysisMethod::MethodB)
                    {
                        brgReporter->ReportBearingSpecCheckSummaryB(pChapter, pParagraph, artifact, &tArtifact);
                    }
                    else
                    {
                        brgReporter->ReportBearingSpecCheckSummaryA(pChapter, pParagraph, artifact);
                    }
                }
            }
        }
        else
        {
            *pParagraph << rptNewLine << _T("Bearing Spec Check is not enabled. See 'Spec Checking & Design' tab in Project Criteria Library");
        }

    }

    return pChapter;
    
}


