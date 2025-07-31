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
#include <Reporting\BearingSpecCheckChapterBuilder.h>
#include <Reporting\SpanGirderBearingReportSpecification.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\StrandStressCheckTable.h>
#include <Reporting\TendonStressCheckTable.h>
#include <Reporting\FlexuralStressCheckTable.h>
#include <Reporting\FlexuralCapacityCheckTable.h>
#include <Reporting\ShearCheckTable.h>
#include <Reporting\InterfaceShearTable.h>
#include <Reporting\StrandSlopeCheck.h>
#include <Reporting\HoldDownForceCheck.h>
#include <Reporting\PlantHandlingCheck.h>
#include <Reporting\ConstructabilityCheckTable.h>
#include <Reporting\CamberTable.h>
#include <Reporting\GirderDetailingCheck.h>
#include <Reporting\LiftingCheck.h>
#include <Reporting\HaulingCheck.h>
#include <Reporting\LongReinfShearCheck.h>
#include <Reporting\OptionalDeflectionCheck.h>
#include <Reporting\DebondCheckTable.h>
#include <Reporting\ContinuityCheck.h>
#include <Reporting\DuctSizeCheckTable.h>
#include <Reporting\DuctGeometryCheckTable.h>
#include <Reporting\PrincipalTensionStressCheckTable.h>
#include <Reporting\RatingSummaryTable.h>
#include <Reporting\ReinforcementFatigueCheck.h>

#include <MathEx.h>

#include <EAF\EAFApp.h>

#include <IFace\BearingDesignParameters.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace/Limits.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>
#include <IFace\SplittingChecks.h>

#include <psgLib/EndZoneCriteria.h>
#include <psgLib/SlabOffsetCriteria.h>
#include <psgLib/BearingCriteria.h>
#include <EngTools/Bearing.h>
#include <EngTools/BearingLoads.h>
#include <EngTools/BearingCalculator.h>
#include <EngTools/BearingReporter.h>
#include <AgentTools.h>



/****************************************************************************
CLASS
   CBearingSpecCheckChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CBearingSpecCheckChapterBuilder::CBearingSpecCheckChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CBearingSpecCheckChapterBuilder::GetName() const
{
   return TEXT("Bearing Specification Check Details");
}

rptChapter* CBearingSpecCheckChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   auto pBrgRptSpec = std::dynamic_pointer_cast<const CBearingReportSpecification>(pRptSpec);

   const ReactionLocation& reactionLocation = pBrgRptSpec->GetReactionLocation();

   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   *pPara << _T("Specification: ") << pSpec->GetSpecification() << rptNewLine;

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

        const auto& pBearingData = pIBridgeDesc->GetBearingData(
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
            *pPara << _T("Bearing specification check not available for the selected BDS version") << rptNewLine;
        }
        else if (pBearingData->Shape == BearingShape::bsRound)
        {
            *pPara << _T("Bearing specification check not available for round bearings") << rptNewLine;
        }
        else if (pBearingData->DefinitionType == BearingDefinitionType::btBasic)
        {
            *pPara << _T("Could not evaluate bearing because it is not defined in sufficient detail") << rptNewLine;
        }
        else
        {

           CEAFApp* pApp = EAFGetApp();
           const WBFL::Units::IndirectMeasure* pDispUnits = pApp->GetDisplayUnits();
           std::unique_ptr<WBFL::EngTools::BearingReporter> brgReporter;

           brgReporter->ReportIntroduction(pPara, artifact, &tArtifact);
           brgReporter->ReportBearingProperties(pDispUnits, pChapter, pPara, artifact, &tArtifact);

           const auto& criteria = artifact.GetBearingDesignCriteria();

           if (criteria.AnalysisMethod == WBFL::EngTools::BearingAnalysisMethod::MethodB)
           {
               brgReporter->ReportBearingSpecificationCheckB(pDispUnits, pChapter, pPara, artifact, &tArtifact);
           }
           else
           {
               brgReporter->ReportBearingSpecificationCheckA(pDispUnits, pChapter, pPara, artifact);
           }
       }
   }
   else
   {
       *pPara << _T("Bearing Spec Check is not enabled. See 'Spec Checking & Design' tab in Project Criteria Library") << rptNewLine;
   }

   return pChapter;
}




