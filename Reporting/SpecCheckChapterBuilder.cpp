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
#include <Reporting\SpecCheckChapterBuilder.h>
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

#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\Allowables.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>
#include <IFace\SplittingChecks.h>

#include <PgsExt\GirderArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void write_splitting_zone_check(IBroker* pBroker,
                               const pgsGirderArtifact* pGirderArtifact,
                               rptChapter* pChapter);

static void write_confinement_check(IBroker* pBroker,
                                    IEAFDisplayUnits* pDisplayUnits,
                                    const pgsGirderArtifact* pGirderArtifact,
                                    rptChapter* pChapter);

/****************************************************************************
CLASS
   CSpecCheckChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSpecCheckChapterBuilder::CSpecCheckChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CSpecCheckChapterBuilder::GetName() const
{
   return TEXT("Specification Checks");
}

rptChapter* CSpecCheckChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;


   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey = pGirderRptSpec->GetGirderKey();

   GET_IFACE2(pBroker,IDocumentType,pDocType);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   *pPara << _T("Specification: ") << pSpec->GetSpecification() << rptNewLine;

   const pgsGirderArtifact* pGirderArtifact = pArtifacts->GetGirderArtifact(girderKey);

   IntervalIndexType lastIntervalIdx          = pIntervals->GetIntervalCount() - 1;
   IntervalIndexType tsRemovalIntervalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(CSegmentKey(girderKey,0));
   IntervalIndexType liftingIntervalIdx       = pIntervals->GetLiftSegmentInterval(CSegmentKey(girderKey,0));
   IntervalIndexType haulingIntervalIdx       = pIntervals->GetHaulSegmentInterval(CSegmentKey(girderKey,0));
   IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
   IntervalIndexType lastTendonStressingIntervalIdx = pIntervals->GetLastGirderTendonStressingInterval(girderKey);

   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(girderKey);

   pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pPara << _T("Stress Limitations on Prestressing Tendons [") << LrfdCw8th(_T("5.9.3"),_T("5.9.2.2")) << _T("]");
   *pChapter << pPara;

   // Stresses in prestressing strands
   CStrandStressCheckTable().Build(pChapter,pBroker,pGirderArtifact,pDisplayUnits);

   // Stresses in tendons
   CTendonStressCheckTable().Build(pChapter,pBroker,pGirderArtifact,pDisplayUnits);

   // report the required concrete strengths for the current bridge configuration
   rptParagraph* p = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << p;
   *p << _T("Required Minimum Concrete Strengths") << rptNewLine;

   p = new rptParagraph;
   if ( pDocType->IsPGSuperDocument() )
   {
      p->SetName(_T("Girder Stresses"));
   }
   else
   {
      p->SetName(_T("Segment Stresses"));
   }
   *pChapter << p;

   p = new rptParagraph;
   *pChapter << p;

   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );

   Float64 fci_reqd = pGirderArtifact->GetRequiredReleaseStrength();
   Float64 fc_reqd  = pGirderArtifact->GetRequiredGirderConcreteStrength();
   if ( 0 <= fci_reqd )
   {
      Float64 fci_rounded = IS_SI_UNITS(pDisplayUnits) ? CeilOffTol(fci_reqd,WBFL::Units::ConvertToSysUnits(6,WBFL::Units::Measure::MPa)) : CeilOffTol(fci_reqd,WBFL::Units::ConvertToSysUnits(100,WBFL::Units::Measure::PSI));
      *p << _T("Required ") << RPT_FCI << _T(" = ") << stress_u.SetValue(fci_reqd);
      *p << _T(" ") << symbol(RIGHT_DOUBLE_ARROW) << _T(" ") << stress_u.SetValue(fci_rounded) << rptNewLine;
   }
   else
   {
      ATLASSERT(fci_reqd == NO_AVAILABLE_CONCRETE_STRENGTH);
      *p << _T("Required ") << RPT_FCI << _T(" = Regardless of the release strength, the stress requirements will not be satisfied.") << rptNewLine;
   }


   if ( pDocType->IsPGSuperDocument() )
   {
      GET_IFACE2(pBroker,IMaterials,pMaterial);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(CSegmentKey(girderKey,0));
      *p << _T("Provided ") << RPT_FCI << _T(" = ") << stress_u.SetValue( pMaterial->GetSegmentFc(CSegmentKey(girderKey,0),releaseIntervalIdx) ) << rptNewLine;
   }

   *p << rptNewLine;

   if ( 0 <= fc_reqd )
   {
      Float64 fc_rounded = IS_SI_UNITS(pDisplayUnits) ? CeilOffTol(fc_reqd,WBFL::Units::ConvertToSysUnits(6,WBFL::Units::Measure::MPa)) : CeilOffTol(fc_reqd,WBFL::Units::ConvertToSysUnits(100,WBFL::Units::Measure::PSI));
      *p << _T("Required ") << RPT_FC  << _T(" = ") << stress_u.SetValue(fc_reqd);
      *p << _T(" ") << symbol(RIGHT_DOUBLE_ARROW) << _T(" ") << stress_u.SetValue(fc_rounded) << rptNewLine;
   }
   else
   {
      ATLASSERT(fc_reqd == NO_AVAILABLE_CONCRETE_STRENGTH);
      *p << _T("Required ") << RPT_FC << _T(" = Regardless of the concrete strength, the stress requirements will not be satisfied.") << rptNewLine;
   }

   if ( pDocType->IsPGSuperDocument() )
   {
      GET_IFACE2(pBroker,IMaterials,pMaterial);
      *p << _T("Provided ") << RPT_FC << _T(" = ") << stress_u.SetValue( pMaterial->GetSegmentFc28(CSegmentKey(girderKey,0))) << rptNewLine;
   }

   // information about continuity and how it impacts the analysis
   CContinuityCheck().Build(pChapter,pBroker,girderKey,pDisplayUnits);

   // report flexural stresses at various intervals
   GET_IFACE2(pBroker, IStressCheck, pStressCheck);
   std::vector<StressCheckTask> vTasks(pStressCheck->GetStressCheckTasks(girderKey));
   for (const auto& task : vTasks)
   {
      IntervalIndexType intervalIdx = task.intervalIdx;

      // skip lifting and hauling... the reporting is different. See below
      if (intervalIdx == liftingIntervalIdx || intervalIdx == haulingIntervalIdx)
      {
         continue;
      }

      CFlexuralStressCheckTable().Build(pChapter,pBroker,pGirderArtifact,pDisplayUnits,task,true/*girder stresses*/);
   } // next task


   if ( lastTendonStressingIntervalIdx != INVALID_INDEX &&         // if there are tendons AND
        lastCompositeDeckIntervalIdx <= lastTendonStressingIntervalIdx // if they are stressed after the deck is composite
      )
   {
      rptParagraph* p = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      p->SetName(_T("Deck Stresses"));
      *p << p->GetName() << rptNewLine;
      *pChapter << p;

      p = new rptParagraph;
      *pChapter << p;

      Float64 fc_reqd = pGirderArtifact->GetRequiredDeckConcreteStrength();
      if ( 0 <= fc_reqd )
      {
         Float64 fc_rounded = IS_SI_UNITS(pDisplayUnits) ? CeilOffTol(fc_reqd,WBFL::Units::ConvertToSysUnits(6,WBFL::Units::Measure::MPa)) : CeilOffTol(fc_reqd,WBFL::Units::ConvertToSysUnits(100,WBFL::Units::Measure::PSI));
         *p << _T("Required ") << RPT_FC  << _T(" = ") << stress_u.SetValue(fc_reqd);
         *p << _T(" ") << symbol(RIGHT_DOUBLE_ARROW) << _T(" ") << stress_u.SetValue(fc_rounded) << rptNewLine;
      }
      else
      {
         *p << _T("Required ") << RPT_FC << _T(" = Regardless of the concrete strength, the stress requirements will not be satisfied.") << rptNewLine;
      }

      // report flexural stresses at various intervals
      for (const auto& task : vTasks)
      {
         IntervalIndexType intervalIdx = task.intervalIdx;

         // skipping everything before the deck is composite
         if (intervalIdx < lastCompositeDeckIntervalIdx)
         {
            continue;
         }

         CFlexuralStressCheckTable().Build(pChapter,pBroker,pGirderArtifact,pDisplayUnits,task,false/*deck stresses*/);
      } // next task
   }

   // Reinforcement fatigue check
   CReinforcementFatigueCheck().Build(pChapter, pBroker, pGirderArtifact, pDisplayUnits);

   // Flexural Capacity
   p = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   p->SetName(_T("Ultimate Moment Capacity"));
   *p << p->GetName() << rptNewLine;
   *pChapter << p;

   // NOTE
   // No longer designing/checking for ultimate moment in temporary construction state
   // per e-mail from Bijan Khaleghi, dated 4/28/1999.  See project log.
   //   p = new rptParagraph;
   //   *p << CFlexuralCapacityCheckTable().Build(pBroker,segmentKey,pDisplayUnits,pgsTypes::BridgeSite1,pgsTypes::StrengthI) << rptNewLine;
   //   *pChapter << p;

   p = new rptParagraph;
   bool bOverReinforced;
   *p << CFlexuralCapacityCheckTable().Build(pBroker,pGirderArtifact,pDisplayUnits,lastIntervalIdx,pgsTypes::StrengthI,true,&bOverReinforced) << rptNewLine;
   if ( bOverReinforced )
   {
      *p << _T("* Over reinforced sections may be adequate if M") << Sub(_T("u")) << _T(" does not exceed the minimum resistance specified in LRFD C5.7.3.3.1") << rptNewLine;
      *p << _T("  Limiting capacity of over reinforced sections are shown in parentheses") << rptNewLine;
      *p << _T("  See Moment Capacity Details chapter for additional information") << rptNewLine;
   }
   *pChapter << p;

   // Strength II if permit load exists
   if (bPermit)
   {
      p = new rptParagraph;
      *p << CFlexuralCapacityCheckTable().Build(pBroker,pGirderArtifact,pDisplayUnits,lastIntervalIdx,pgsTypes::StrengthII,true,&bOverReinforced) << rptNewLine;
      if ( bOverReinforced )
      {
         *p << _T("* Over reinforced sections may be adequate if M") << Sub(_T("u")) << _T(" does not exceed the minimum resistance specified in LRFD C5.7.3.3.1") << rptNewLine;
         *p << _T("  Limiting capacity of over reinforced sections are shown in parentheses") << rptNewLine;
         *p << _T("  See Moment Capacity Details chapter for additional information") << rptNewLine;
      }
      *pChapter << p;
   }
   
   *p << rptNewLine;

   SpanIndexType startSpanIdx = pBridge->GetGirderGroupStartSpan(girderKey.groupIndex);
   SpanIndexType endSpanIdx   = pBridge->GetGirderGroupEndSpan(girderKey.groupIndex);
   bool bProcessNegativeMoments = false;
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      if ( pBridge->ProcessNegativeMoments(spanIdx) )
      {
         bProcessNegativeMoments = true;
         break;
      }
   }

   if ( bProcessNegativeMoments )
   {
      p = new rptParagraph;
      *p << CFlexuralCapacityCheckTable().Build(pBroker,pGirderArtifact,pDisplayUnits,lastIntervalIdx,pgsTypes::StrengthI,false,&bOverReinforced) << rptNewLine;
      if ( bOverReinforced )
      {
         *p << _T("* Over reinforced sections may be adequate if M") << Sub(_T("u")) << _T(" does not exceed the minimum resistance specified in LRFD C5.7.3.3.1") << rptNewLine;
         *p << _T("  Limiting capacity of over reinforced sections are shown in parentheses") << rptNewLine;
         *p << _T("  See Moment Capacity Details chapter for additional information") << rptNewLine;
      }
      *pChapter << p;

      // Strength II if permit load exists
      if (bPermit)
      {
         p = new rptParagraph;
         *p << CFlexuralCapacityCheckTable().Build(pBroker,pGirderArtifact,pDisplayUnits,lastIntervalIdx,pgsTypes::StrengthII,false,&bOverReinforced) << rptNewLine;
         if ( bOverReinforced )
         {
            *p << _T("* Over reinforced sections may be adequate if M") << Sub(_T("u")) << _T(" does not exceed the minimum resistance specified in LRFD C5.7.3.3.1") << rptNewLine;
            *p << _T("  Limiting capacity of over reinforced sections are shown in parentheses") << rptNewLine;
            *p << _T("  See Moment Capacity Details chapter for additional information") << rptNewLine;
         }
         *pChapter << p;
      }
   }

   // Vertical Shear check
   p = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   p->SetName(_T("Ultimate Shear Capacity"));
   *p << p->GetName() << rptNewLine;
   *pChapter << p;

   p = new rptParagraph;
   *pChapter << p;

   bool bStrutAndTieRequired;
   *p << CShearCheckTable().Build(pBroker,pGirderArtifact,pDisplayUnits,lastIntervalIdx,pgsTypes::StrengthI,bStrutAndTieRequired) << rptNewLine;

   CShearCheckTable().BuildNotes(pChapter,pBroker,pGirderArtifact,pDisplayUnits,lastIntervalIdx,pgsTypes::StrengthI,bStrutAndTieRequired);

   if ( bPermit )
   {
      p = new rptParagraph;
      *pChapter << p;
      *p << CShearCheckTable().Build(pBroker,pGirderArtifact,pDisplayUnits,lastIntervalIdx,pgsTypes::StrengthII,bStrutAndTieRequired) << rptNewLine;

      CShearCheckTable().BuildNotes(pChapter,pBroker,pGirderArtifact,pDisplayUnits,lastIntervalIdx,pgsTypes::StrengthII,bStrutAndTieRequired);
   }

   // Longitudinal reinforcement for shear
   CLongReinfShearCheck().Build(pChapter, pBroker, pGirderArtifact, lastIntervalIdx, pgsTypes::StrengthI, pDisplayUnits);
   if (bPermit)
   {
      CLongReinfShearCheck().Build(pChapter, pBroker, pGirderArtifact, lastIntervalIdx, pgsTypes::StrengthII, pDisplayUnits);
   }

   // Interface Shear check
   if ( pBridge->IsCompositeDeck() )
   {
      CInterfaceShearTable().Build(pBroker,pChapter,pGirderArtifact,pDisplayUnits,lastIntervalIdx,pgsTypes::StrengthI);

      if ( bPermit )
      {
         CInterfaceShearTable().Build(pBroker,pChapter,pGirderArtifact,pDisplayUnits,lastIntervalIdx,pgsTypes::StrengthII);
      }
   }

   // Principle tension stress in webs
   CPrincipalTensionStressCheckTable().Build(pChapter, pBroker, pGirderArtifact, pDisplayUnits);

   if (pSpecEntry->IsSplittingCheckEnabled())
   {
      // Splitting Zone check
      write_splitting_zone_check(pBroker,pGirderArtifact,pChapter);
   }

   if (pSpecEntry->IsConfinementCheckEnabled())
   {
      // confinement check
      write_confinement_check(pBroker,pDisplayUnits,pGirderArtifact,pChapter);
   }

   // Optional live load deflection
   COptionalDeflectionCheck().Build(pChapter,pBroker,pGirderArtifact,pDisplayUnits);

   // Lifting
   GET_IFACE2(pBroker,ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   if (pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled() || lrfdVersionMgr::NinthEdition2020 <= lrfdVersionMgr::GetVersion())
   {
      // starting with 9th edition, stability checks are manditory so always report the outcome
      p = new rptParagraph;
      p->SetName(_T("Lifting"));
      *pChapter << p;

      CLiftingCheck().Build(pChapter,pBroker,girderKey,pDisplayUnits);
   }

   // Hauling
   GET_IFACE2(pBroker,ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
   if (pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled() || lrfdVersionMgr::NinthEdition2020 <= lrfdVersionMgr::GetVersion())
   {
      // starting with 9th edition, stability checks are manditory so always report the outcome
      p = new rptParagraph;
      p->SetName(_T("Hauling"));
      *pChapter << p;

      CHaulingCheck().Build(pChapter,pBroker,girderKey,pDisplayUnits);
   }

   // Constructability Checks
   p = new rptParagraph(rptStyleManager::GetHeadingStyle());
   p->SetName(_T("Constructability"));
   *pChapter << p;
   *p << rptNewLine << _T("Constructability Checks") << rptNewLine;

   // Girder Detailing
   CGirderDetailingCheck().Build(pChapter,pBroker,pGirderArtifact,pDisplayUnits);

   // Debonding Checks
   CDebondCheckTable().Build(pChapter, pBroker, pGirderArtifact,  pDisplayUnits);

   // Strand Slope
   CStrandSlopeCheck().Build(pChapter,pBroker,pGirderArtifact,pDisplayUnits);

   // Hold Down Force
   CHoldDownForceCheck().Build(pChapter, pBroker, pGirderArtifact, pDisplayUnits);

   // Plant Handling Weight
   CPlantHandlingCheck().Build(pChapter, pBroker, pGirderArtifact, pDisplayUnits);

   // Duct size
   CDuctGeometryCheckTable().Build(pChapter,pBroker,pGirderArtifact,pDisplayUnits);
   CDuctSizeCheckTable().Build(pChapter,pBroker,pGirderArtifact,pDisplayUnits);

   // Global Stability Check
   CConstructabilityCheckTable().BuildGlobalGirderStabilityCheck(pChapter,pBroker,pGirderArtifact,pDisplayUnits);

   std::vector<CGirderKey> girderList;
   girderList.push_back(girderKey);

   // Bottom Flange Clearance Check
   CConstructabilityCheckTable().BuildBottomFlangeClearanceCheck(pChapter,pBroker,girderList,pDisplayUnits);

   if (pBridge->GetDeckType() == pgsTypes::sdtNone)
   {
      p = new rptParagraph(rptStyleManager::GetHeadingStyle());
      p->SetName(_T("Finished Elevation Checks"));
      *p << rptNewLine << _T("Finished Elevation Checks") << rptNewLine;
      *pChapter << p;

      if (!pSpecEntry->IsSlabOffsetCheckEnabled())
      {
         p = new rptParagraph;
         *p << color(Red) << Bold(_T("Note: Finished elevations specification Checks are Disabled in the Project Criteria.")) << color(Black) << rptNewLine;
         *pChapter << p;
      }
      else
      {
      // Finished Elevation Check
         CConstructabilityCheckTable().BuildFinishedElevationCheck(pChapter,pBroker,girderList,pDisplayUnits);
      }
   }
   else if (pBridge->GetHaunchInputDepthType() != pgsTypes::hidACamber) // direct haunch input
   {
      p = new rptParagraph(rptStyleManager::GetHeadingStyle());
      p->SetName(_T("Finished Elevation and Haunch Checks"));
      *p << rptNewLine << _T("Finished Elevation and Haunch Checks") << rptNewLine;
      *pChapter << p;

      if (!pSpecEntry->IsSlabOffsetCheckEnabled())
      {
         p = new rptParagraph;
         *p << color(Red) << Bold(_T("Note: Finished elevations specification Checks are Disabled in the Project Criteria.")) << color(Black) << rptNewLine;
         *pChapter << p;
      }
      else
      {
         // Finished Elevation Check
         CConstructabilityCheckTable().BuildFinishedElevationCheck(pChapter,pBroker,girderList,pDisplayUnits);

         // Minimum Haunch Check
         CConstructabilityCheckTable().BuildMinimumHaunchCheck(pChapter,pBroker,girderList,pDisplayUnits);

         // Min Haunch at bearing centerlines check
         CConstructabilityCheckTable().BuildMinimumHaunchCLCheck(pChapter,pBroker,girderList,pDisplayUnits);

         // Fillet Check
         CConstructabilityCheckTable().BuildMinimumFilletCheck(pChapter,pBroker,girderList,pDisplayUnits);
      }
   }
   else
   {
      p = new rptParagraph(rptStyleManager::GetHeadingStyle());
      p->SetName(_T("Haunch Geometry"));
      *p << rptNewLine << _T("Haunch Geometry Checks") << rptNewLine;
      *pChapter << p;

      if (!pSpecEntry->IsSlabOffsetCheckEnabled())
      {
         p = new rptParagraph;
         *p << color(Red) << Bold(_T("Note: Specification Checks for Haunch Geometry are Disabled in the Project Criteria. All Status values will be reported as N/A.")) << color(Black) << rptNewLine;
         *pChapter << p;
      }

      // "A" Dimension check
      CConstructabilityCheckTable().BuildSlabOffsetTable(pChapter,pBroker,girderList,pDisplayUnits);

      // Min Haunch at bearing centerlines check
      CConstructabilityCheckTable().BuildMinimumHaunchCLCheck(pChapter,pBroker,girderList,pDisplayUnits);

      // Fillet Check
      CConstructabilityCheckTable().BuildMinimumFilletCheck(pChapter,pBroker,girderList,pDisplayUnits);

      // Haunch Geometry Check
      CConstructabilityCheckTable().BuildHaunchGeometryComplianceCheck(pChapter,pBroker,girderList,pDisplayUnits);
   }

   // Camber Check
   CConstructabilityCheckTable().BuildCamberCheck(pChapter,pBroker,girderKey,pDisplayUnits);

   // Precamber Check
   CConstructabilityCheckTable().BuildPrecamberCheck(pChapter, pBroker, girderList, pDisplayUnits);

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CSpecCheckChapterBuilder::Clone() const
{
   return std::make_unique<CSpecCheckChapterBuilder>();
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

void write_splitting_zone_check(IBroker* pBroker,
                               const pgsGirderArtifact* pGirderArtifact,
                               rptChapter* pChapter)
{
   GET_IFACE2(pBroker, ISplittingChecks, pSplittingChecks);
   pSplittingChecks->ReportSplittingChecks(pBroker, pGirderArtifact, pChapter);
}

void write_confinement_check(IBroker* pBroker,
                             IEAFDisplayUnits* pDisplayUnits,
                             const pgsGirderArtifact* pGirderArtifact,
                             rptChapter* pChapter)
{
   GET_IFACE2(pBroker,IBridge,pBridge);

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    length, pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,    pDisplayUnits->GetComponentDimUnit(), true );

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   (*pPara) << _T("Confinement Stirrup Check [") << LrfdCw8th(_T("5.10.10.2"),_T("5.9.4.4.2")) << _T("]") << rptNewLine;

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      if ( 1 < nSegments )
      {
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }

      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact *pStirrups   = pSegmentArtifact->GetStirrupCheckArtifact();
      const pgsConfinementCheckArtifact& rConfine     = pStirrups->GetConfinementArtifact();

      pPara = new rptParagraph;
      *pChapter << pPara;

      (*pPara) << _T("  Minimum Required Bar Size in Confinement Zone: ")<< lrfdRebarPool::GetBarSize(rConfine.GetMinBar()->GetSize()) << rptNewLine;
      (*pPara) << _T("  Maximum Required Bar Spacing in Confinement Zone = ")<< dim.SetValue(rConfine.GetSMax()) << rptNewLine << rptNewLine;

      (*pPara) << Bold(_T("Left End of Girder:")) << rptNewLine;
      (*pPara) << _T("  Required Confinement Zone Length: ")<<rConfine.GetZoneLengthFactor()<<_T("d = (")
               <<rConfine.GetZoneLengthFactor()<<_T(")(")<<length.SetValue(rConfine.GetStartd())<<_T(") = ")
               << length.SetValue(rConfine.GetStartRequiredZoneLength()) << rptNewLine;
      (*pPara) << _T("  Provided Confinement Zone Length within Required Zone Length = ") << length.SetValue(rConfine.GetStartProvidedZoneLength()) << rptNewLine;
      WBFL::Materials::Rebar::Size size = rConfine.GetStartBar()==nullptr ? WBFL::Materials::Rebar::Size::bsNone : rConfine.GetStartBar()->GetSize();
      (*pPara) << _T("  Bar Size in Zone: ")<< lrfdRebarPool::GetBarSize(size) << rptNewLine;
      (*pPara) << _T("  Bar Spacing in Zone = ")<< dim.SetValue(rConfine.GetStartS()) << rptNewLine;
      (*pPara) << _T("  Status = ");
      if ( rConfine.StartPassed() )
      {
         (*pPara) << RPT_PASS;
      }
      else
      {
         (*pPara) << RPT_FAIL;
      }

      (*pPara) <<rptNewLine<<rptNewLine;

      (*pPara) << Bold(_T("Right End of Girder:")) << rptNewLine;
      (*pPara) << _T("  Required Confinement Zone Length: ")<<rConfine.GetZoneLengthFactor()<<_T("d = (")
               <<rConfine.GetZoneLengthFactor()<<_T(")(")<<length.SetValue(rConfine.GetEndd())<<_T(") = ")
               << length.SetValue(rConfine.GetEndRequiredZoneLength()) << rptNewLine;
      (*pPara) << _T("  Provided Confinement Zone Length within Required Zone Length = ") << length.SetValue(rConfine.GetEndProvidedZoneLength()) << rptNewLine;
      size = rConfine.GetEndBar()==nullptr ? WBFL::Materials::Rebar::Size::bsNone : rConfine.GetEndBar()->GetSize();
      (*pPara) << _T("  Bar Size in Zone: ")<< lrfdRebarPool::GetBarSize(size) << rptNewLine;
      (*pPara) << _T("  Bar Spacing in Zone = ")<< dim.SetValue(rConfine.GetEndS()) << rptNewLine;
      (*pPara) << _T("  Status = ");
      if ( rConfine.EndPassed() )
      {
         (*pPara) << RPT_PASS;
      }
      else
      {
         (*pPara) << RPT_FAIL;
      }
   } // next segment
}
