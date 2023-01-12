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
#include "TogaStressChecksChapterBuilder.h"
#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignUtilities.h"


#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\FlexuralStressCheckTable.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\Allowables.h>
#include <IFace\Intervals.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\CapacityToDemand.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CTogaStressChecksChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTogaStressChecksChapterBuilder::CTogaStressChecksChapterBuilder():
CPGSuperChapterBuilder(true)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTogaStressChecksChapterBuilder::GetName() const
{
   return TEXT("Stress Checks");
}

rptChapter* CTogaStressChecksChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   // This is a single segment report
   CSegmentKey segmentKey(girderKey,0);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,ISpecification,pSpec);
   *pPara << _T("Specification = ") << pSpec->GetSpecification() << rptNewLine;

   *pPara << Bold(_T("Notes: "))<< rptNewLine;
   *pPara <<symbol(DOT)<<_T(" Calculated total external load top and bottom stresses are multiplied by the appropriate (Top or Bottom) ratio of (Input Design Load Stress)/(Calculated Stress).");
   *pPara << _T(" This results in the Analysis Stress")<<rptNewLine;
   *pPara <<symbol(DOT)<<_T(" Stress Checks reflect the following sign convention: Compressive stress is negative. Tensile stress is positive.");

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IStressCheck, pStressCheck);
   std::vector<StressCheckTask> vStressCheckTasks(pStressCheck->GetStressCheckTasks(segmentKey));
   for (const auto& task : vStressCheckTasks)
   {
      BuildTableAndNotes(pChapter, pBroker, pDisplayUnits, task);
   }

   return pChapter;
}

void CTogaStressChecksChapterBuilder::BuildTableAndNotes(rptChapter* pChapter, IBroker* pBroker,
                      IEAFDisplayUnits* pDisplayUnits, const StressCheckTask& task) const
{
   // We need the artifact that we've doctored for txdot reasons
   GET_IFACE2(pBroker,IGetTogaResults,pGetTogaResults);
   const pgsGirderArtifact* pFactoredGdrArtifact = pGetTogaResults->GetFabricatorDesignArtifact();
   const pgsSegmentArtifact* pSegmentArtifact = pFactoredGdrArtifact->GetSegmentArtifact(0);

   // Write notes from pgsuper default table, then our notes, then table
   CFlexuralStressCheckTable().BuildNotes(pChapter, pBroker, pFactoredGdrArtifact, 0, pDisplayUnits, task, true);

   CSegmentKey fabrSegmentKey(TOGA_SPAN,TOGA_FABR_GDR,0);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(fabrSegmentKey);

   if ( task.intervalIdx != releaseIntervalIdx)
   {
     // Toga Special notes
      rptParagraph* p = new rptParagraph;
      *pChapter << p;

      Float64 stress_val, stress_fac, stress_loc;
      pGetTogaResults-> GetControllingCompressiveStress(&stress_val, &stress_fac, &stress_loc);
      *p<<_T("Ratio applied to Top Stresses = ")<< stress_fac << rptNewLine;

      pGetTogaResults->GetControllingTensileStress(&stress_val, &stress_fac, &stress_loc);
      *p<<_T("Ratio applied to Bottom Stresses = ")<< stress_fac << rptNewLine;
   }

   CFlexuralStressCheckTable().BuildTable(pChapter, pBroker, pFactoredGdrArtifact, fabrSegmentKey.segmentIndex, pDisplayUnits, task, true/*girder stresses*/);
}

CChapterBuilder* CTogaStressChecksChapterBuilder::Clone() const
{
   return new CTogaStressChecksChapterBuilder;
}
