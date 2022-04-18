///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>

#include <IFace\Artifact.h>

#include <PgsExt\HaulingAnalysisArtifact.h>
#include <PgsExt\CapacityToDemand.h>
#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\GirderLabel.h>

#include "PGSuperUnits.h"

#include <Reporting\ReportNotes.h>
#include <EAF\EAFDisplayUnits.h>
#include <PgsExt\GirderArtifact.h>

#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Project.h>
#include <IFace\PointOfInterest.h>
#include <IFace\Bridge.h>

#include <limits>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

pgsWsdotHaulingAnalysisArtifact::pgsWsdotHaulingAnalysisArtifact()
{
}

pgsWsdotHaulingAnalysisArtifact::pgsWsdotHaulingAnalysisArtifact(const pgsWsdotHaulingAnalysisArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsWsdotHaulingAnalysisArtifact::~pgsWsdotHaulingAnalysisArtifact()
{
}

//======================== OPERATORS  =======================================
pgsWsdotHaulingAnalysisArtifact& pgsWsdotHaulingAnalysisArtifact::operator= (const pgsWsdotHaulingAnalysisArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool pgsWsdotHaulingAnalysisArtifact::Passed(bool bIgnoreConfirationLimits) const
{
   return m_HaulingArtifact.Passed(bIgnoreConfirationLimits);
}

bool pgsWsdotHaulingAnalysisArtifact::Passed(pgsTypes::HaulingSlope slope) const
{
   return m_HaulingArtifact.Passed((WBFL::Stability::HaulingSlope)slope);
}

bool pgsWsdotHaulingAnalysisArtifact::PassedStressCheck(pgsTypes::HaulingSlope slope) const
{
   return m_HaulingArtifact.PassedStressCheck((WBFL::Stability::HaulingSlope)slope);
}

pgsHaulingAnalysisArtifact* pgsWsdotHaulingAnalysisArtifact::Clone() const
{
   std::unique_ptr<pgsWsdotHaulingAnalysisArtifact> clone(std::make_unique<pgsWsdotHaulingAnalysisArtifact>());
   *clone = *this;

   return clone.release();
}

void pgsWsdotHaulingAnalysisArtifact::BuildHaulingCheckReport(const CSegmentKey& segmentKey,rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const
{
   WBFL::Stability::HaulingStabilityReporter reporter;
   GET_IFACE2(pBroker,IGirder,pGirder);
   const WBFL::Stability::IGirder* pStabilityModel = pGirder->GetSegmentHaulingStabilityModel(segmentKey);
   const WBFL::Stability::IHaulingStabilityProblem* pStabilityProblem = pGirder->GetSegmentHaulingStabilityProblem(segmentKey);
   Float64 Ll, Lr;
   pStabilityProblem->GetSupportLocations(&Ll,&Lr);
   reporter.BuildSpecCheckChapter(pStabilityModel,pStabilityProblem,&m_HaulingArtifact,pChapter,_T("Location from<BR/>Left Bunk Point"),Ll);
}

void pgsWsdotHaulingAnalysisArtifact::BuildHaulingDetailsReport(const CSegmentKey& segmentKey, rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const
{
   WBFL::Stability::HaulingStabilityReporter reporter;
   GET_IFACE2(pBroker,IGirder,pGirder);
   const WBFL::Stability::IGirder* pStabilityModel = pGirder->GetSegmentHaulingStabilityModel(segmentKey);
   const WBFL::Stability::IHaulingStabilityProblem* pStabilityProblem = pGirder->GetSegmentHaulingStabilityProblem(segmentKey);
   Float64 Ll, Lr;
   pStabilityProblem->GetSupportLocations(&Ll,&Lr);
   const WBFL::Stability::HaulingResults& results = m_HaulingArtifact.GetHaulingResults();
   reporter.BuildDetailsChapter(pStabilityModel,pStabilityProblem,&results,pChapter,_T("Location from<BR/>Left Bunk Point"),Ll);
}

void pgsWsdotHaulingAnalysisArtifact::Write1250Data(const CSegmentKey& segmentKey,std::_tofstream& resultsFile, std::_tofstream& poiFile, IBroker* pBroker,
                                                    const std::_tstring& pid, const std::_tstring& bridgeId) const
{
   GET_IFACE2(pBroker,IGirder,pGirder);
   const WBFL::Stability::HaulingStabilityProblem* pStabilityProblem = pGirder->GetSegmentHaulingStabilityProblem(segmentKey);

   GirderIndexType gdr = segmentKey.girderIndex;

   const WBFL::Stability::HaulingResults& haulingResults = m_HaulingArtifact.GetHaulingResults();
   for (int i = 0; i < 2; i++)
   {
      WBFL::Stability::HaulingSlope slope = (WBFL::Stability::HaulingSlope)i;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100007a, ") << ::ConvertFromSysUnits(haulingResults.MaxDirectStress[slope], unitMeasure::MPa) << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100007b, ") << haulingResults.MaxDirectStressAnalysisPointIndex[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100007c, ") << haulingResults.MaxDirectStressImpactDirection[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100007d, ") << haulingResults.MaxDirectStressCorner[slope] << _T(", 50, ") << gdr << std::endl;

      resultsFile << bridgeId << _T(", ") << pid << _T(", 100008a, ") << ::ConvertFromSysUnits(haulingResults.MinDirectStress[slope], unitMeasure::MPa) << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100008b, ") << haulingResults.MinDirectStressAnalysisPointIndex[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100008c, ") << haulingResults.MinDirectStressImpactDirection[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100008d, ") << haulingResults.MinDirectStressCorner[slope] << _T(", 50, ") << gdr << std::endl;

      resultsFile << bridgeId << _T(", ") << pid << _T(", 100009a, ") << ::ConvertFromSysUnits(haulingResults.MaxStress[slope], unitMeasure::MPa) << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100009b, ") << haulingResults.MaxStressAnalysisPointIndex[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100009c, ") << haulingResults.MaxStressImpactDirection[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100009d, ") << haulingResults.MaxStressCorner[slope] << _T(", 50, ") << gdr << std::endl;

      resultsFile << bridgeId << _T(", ") << pid << _T(", 100010a, ") << ::ConvertFromSysUnits(haulingResults.MinStress[slope], unitMeasure::MPa) << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100010b, ") << haulingResults.MinStressAnalysisPointIndex[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100010c, ") << haulingResults.MinStressImpactDirection[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100010d, ") << haulingResults.MinStressCorner[slope] << _T(", 50, ") << gdr << std::endl;

      resultsFile << bridgeId << _T(", ") << pid << _T(", 100011a, ") << haulingResults.MinFScr[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100011b, ") << haulingResults.FScrAnalysisPointIndex[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100011c, ") << haulingResults.FScrImpactDirection[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100011d, ") << haulingResults.FScrWindDirection[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100011e, ") << haulingResults.FScrCorner[slope] << _T(", 50, ") << gdr << std::endl;

      resultsFile << bridgeId << _T(", ") << pid << _T(", 100012a, ") << haulingResults.MinFsFailure[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100012b, ") << haulingResults.MinAdjFsFailure[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100012c, ") << haulingResults.FSfImpactDirection[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100012d, ") << haulingResults.FSfWindDirection[slope] << _T(", 50, ") << gdr << std::endl;

      resultsFile << bridgeId << _T(", ") << pid << _T(", 100013a, ") << haulingResults.MinFsRollover[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100013b, ") << haulingResults.FSroImpactDirection[slope] << _T(", 50, ") << gdr << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100013c, ") << haulingResults.FSroWindDirection[slope] << _T(", 50, ") << gdr << std::endl;
   }
}


//======================== ACCESS     =======================================

Float64 pgsWsdotHaulingAnalysisArtifact::GetMinFsForCracking(pgsTypes::HaulingSlope slope) const
{
   return m_HaulingArtifact.GetHaulingResults().MinFScr[slope];
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetFsRollover(pgsTypes::HaulingSlope slope) const
{
   return m_HaulingArtifact.GetHaulingResults().MinFsRollover[slope];
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetFsFailure(pgsTypes::HaulingSlope slope) const
{
   return m_HaulingArtifact.GetHaulingResults().MinFsFailure[slope];
}

void pgsWsdotHaulingAnalysisArtifact::GetRequiredConcreteStrength(pgsTypes::HaulingSlope slope,Float64 *pfciComp,Float64 *pfcTension, Float64* pfcTensionWithRebar) const
{
   *pfciComp = m_HaulingArtifact.RequiredFcCompression((WBFL::Stability::HaulingSlope)slope);
   *pfcTension = m_HaulingArtifact.RequiredFcTensionWithoutRebar((WBFL::Stability::HaulingSlope)slope);
   *pfcTensionWithRebar = m_HaulingArtifact.RequiredFcTensionWithRebar((WBFL::Stability::HaulingSlope)slope);
}

void pgsWsdotHaulingAnalysisArtifact::SetHaulingCheckArtifact(const WBFL::Stability::HaulingCheckArtifact& haulingArtifact)
{
   m_HaulingArtifact = haulingArtifact;
}

const WBFL::Stability::HaulingCheckArtifact& pgsWsdotHaulingAnalysisArtifact::GetHaulingCheckArtifact() const
{
   return m_HaulingArtifact;
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================

//======================== OPERATIONS =======================================
void pgsWsdotHaulingAnalysisArtifact::MakeCopy(const pgsWsdotHaulingAnalysisArtifact& rOther)
{
   m_HaulingArtifact = rOther.m_HaulingArtifact;
#if defined _DEBUG
   m_pStabilityProblem = rOther.m_pStabilityProblem;
#endif
}

void pgsWsdotHaulingAnalysisArtifact::MakeAssignment(const pgsWsdotHaulingAnalysisArtifact& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsWsdotHaulingAnalysisArtifact::AssertValid() const
{
   return true;
}

void pgsWsdotHaulingAnalysisArtifact::Dump(dbgDumpContext& os) const
{
   ATLASSERT(m_pStabilityProblem != nullptr);
   os << _T("Dump for pgsWsdotHaulingAnalysisArtifact") << endl;
   os <<_T(" Stress Artifacts - Normal Crown Slope: ")<<endl;
   os << _T("=================================") <<endl;
   WBFL::Stability::WindDirection wind = WBFL::Stability::Left;
   WBFL::Stability::WindDirection cf   = WBFL::Stability::Left;
   const WBFL::Stability::HaulingResults& results = m_HaulingArtifact.GetHaulingResults();

   for (const auto& sectionResult : results.vSectionResults)
   {
      const WBFL::Stability::IAnalysisPoint* pAnalysisPoint = m_pStabilityProblem->GetAnalysisPoint(sectionResult.AnalysisPointIndex);
      Float64 loc = pAnalysisPoint->GetLocation();
      os <<_T("At ") << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << _T(" ft: ");

      os<<endl;
      Float64 fps = sectionResult.fps[WBFL::Stability::TopLeft];
      Float64 fup = sectionResult.f[WBFL::Stability::CrownSlope][WBFL::Stability::ImpactUp][wind][WBFL::Stability::TopLeft];
      Float64 fno = sectionResult.f[WBFL::Stability::CrownSlope][WBFL::Stability::NoImpact][wind][WBFL::Stability::TopLeft];
      Float64 fdown = sectionResult.f[WBFL::Stability::CrownSlope][WBFL::Stability::ImpactDown][wind][WBFL::Stability::TopLeft];
      os<<_T("Top Left Corner       fps=")<<::ConvertFromSysUnits(fps,unitMeasure::KSI)<<_T("ksi, fup=")<<::ConvertFromSysUnits(fup,unitMeasure::KSI)<<_T("ksi, fno=")<<::ConvertFromSysUnits(fno,unitMeasure::KSI)<<_T("ksi, fdown=")<<::ConvertFromSysUnits(fdown,unitMeasure::KSI)<<_T("ksi")<<endl;

      fps = sectionResult.fps[WBFL::Stability::TopRight];
      fup = sectionResult.f[WBFL::Stability::CrownSlope][WBFL::Stability::ImpactUp][wind][WBFL::Stability::TopRight];
      fno = sectionResult.f[WBFL::Stability::CrownSlope][WBFL::Stability::NoImpact][wind][WBFL::Stability::TopRight];
      fdown = sectionResult.f[WBFL::Stability::CrownSlope][WBFL::Stability::ImpactDown][wind][WBFL::Stability::TopRight];
      os << _T("Top Right Corner    fps=") << ::ConvertFromSysUnits(fps, unitMeasure::KSI) << _T("ksi, fup=") << ::ConvertFromSysUnits(fup, unitMeasure::KSI) << _T("ksi, fno=") << ::ConvertFromSysUnits(fno, unitMeasure::KSI) << _T("ksi, fdown=") << ::ConvertFromSysUnits(fdown, unitMeasure::KSI) << _T("ksi") << endl;

      fps = sectionResult.fps[WBFL::Stability::BottomLeft];
      fup = sectionResult.f[WBFL::Stability::CrownSlope][WBFL::Stability::ImpactUp][wind][WBFL::Stability::BottomLeft];
      fno = sectionResult.f[WBFL::Stability::CrownSlope][WBFL::Stability::NoImpact][wind][WBFL::Stability::BottomLeft];
      fdown = sectionResult.f[WBFL::Stability::CrownSlope][WBFL::Stability::ImpactDown][wind][WBFL::Stability::BottomLeft];
      os<<_T("Bottom Left Corner    fps=")<<::ConvertFromSysUnits(fps,unitMeasure::KSI)<<_T("ksi, fup=")<<::ConvertFromSysUnits(fup,unitMeasure::KSI)<<_T("ksi, fno=")<<::ConvertFromSysUnits(fno,unitMeasure::KSI)<<_T("ksi, fdown=")<<::ConvertFromSysUnits(fdown,unitMeasure::KSI)<<_T("ksi")<<endl;

      fps = sectionResult.fps[WBFL::Stability::BottomRight];
      fup = sectionResult.f[WBFL::Stability::CrownSlope][WBFL::Stability::ImpactUp][wind][WBFL::Stability::BottomRight];
      fno = sectionResult.f[WBFL::Stability::CrownSlope][WBFL::Stability::NoImpact][wind][WBFL::Stability::BottomRight];
      fdown = sectionResult.f[WBFL::Stability::CrownSlope][WBFL::Stability::ImpactDown][wind][WBFL::Stability::BottomRight];
      os << _T("Bottom Right Corner fps=") << ::ConvertFromSysUnits(fps, unitMeasure::KSI) << _T("ksi, fup=") << ::ConvertFromSysUnits(fup, unitMeasure::KSI) << _T("ksi, fno=") << ::ConvertFromSysUnits(fno, unitMeasure::KSI) << _T("ksi, fdown=") << ::ConvertFromSysUnits(fdown, unitMeasure::KSI) << _T("ksi") << endl;

      Float64 max_stress = Max(sectionResult.fMax[WBFL::Stability::CrownSlope][WBFL::Stability::Top],sectionResult.fMax[WBFL::Stability::CrownSlope][WBFL::Stability::Bottom]);
      Float64 min_stress = Min(sectionResult.fMin[WBFL::Stability::CrownSlope][WBFL::Stability::Top],sectionResult.fMin[WBFL::Stability::CrownSlope][WBFL::Stability::Bottom]);
      os<<_T("Controlling Stress: Min =")<<::ConvertFromSysUnits(min_stress,unitMeasure::KSI)<<_T("ksi, Max=")<<::ConvertFromSysUnits(max_stress,unitMeasure::KSI)<<_T("ksi")<<endl;
   }

   os <<_T(" Stress Artifacts - Max Superelevation: ")<<endl;
   os << _T("=================================") <<endl;
   for (const auto& sectionResult : results.vSectionResults)
   {
      const WBFL::Stability::IAnalysisPoint* pAnalysisPoint = m_pStabilityProblem->GetAnalysisPoint(sectionResult.AnalysisPointIndex);
      Float64 loc = pAnalysisPoint->GetLocation();
      os <<_T("At ") << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << _T(" ft: ");

      os<<endl;
      Float64 fps = sectionResult.fps[WBFL::Stability::TopLeft];
      Float64 fup = sectionResult.f[WBFL::Stability::Superelevation][WBFL::Stability::ImpactUp][wind][WBFL::Stability::TopLeft];
      Float64 fno = sectionResult.f[WBFL::Stability::Superelevation][WBFL::Stability::NoImpact][wind][WBFL::Stability::TopLeft];
      Float64 fdown = sectionResult.f[WBFL::Stability::Superelevation][WBFL::Stability::ImpactDown][wind][WBFL::Stability::TopLeft];
      os << _T("Top Left Corner     fps=") << ::ConvertFromSysUnits(fps, unitMeasure::KSI) << _T("ksi, fup=") << ::ConvertFromSysUnits(fup, unitMeasure::KSI) << _T("ksi, fno=") << ::ConvertFromSysUnits(fno, unitMeasure::KSI) << _T("ksi, fdown=") << ::ConvertFromSysUnits(fdown, unitMeasure::KSI) << _T("ksi") << endl;

      fps = sectionResult.fps[WBFL::Stability::TopRight];
      fup = sectionResult.f[WBFL::Stability::Superelevation][WBFL::Stability::ImpactUp][wind][WBFL::Stability::TopRight];
      fno = sectionResult.f[WBFL::Stability::Superelevation][WBFL::Stability::NoImpact][wind][WBFL::Stability::TopRight];
      fdown = sectionResult.f[WBFL::Stability::Superelevation][WBFL::Stability::ImpactDown][wind][WBFL::Stability::TopRight];
      os << _T("Top Right Corner    fps=") << ::ConvertFromSysUnits(fps, unitMeasure::KSI) << _T("ksi, fup=") << ::ConvertFromSysUnits(fup, unitMeasure::KSI) << _T("ksi, fno=") << ::ConvertFromSysUnits(fno, unitMeasure::KSI) << _T("ksi, fdown=") << ::ConvertFromSysUnits(fdown, unitMeasure::KSI) << _T("ksi") << endl;

      fps = sectionResult.fps[WBFL::Stability::BottomLeft];
      fup = sectionResult.f[WBFL::Stability::Superelevation][WBFL::Stability::ImpactUp][wind][WBFL::Stability::BottomLeft];
      fno = sectionResult.f[WBFL::Stability::Superelevation][WBFL::Stability::NoImpact][wind][WBFL::Stability::BottomLeft];
      fdown = sectionResult.f[WBFL::Stability::Superelevation][WBFL::Stability::ImpactDown][wind][WBFL::Stability::BottomLeft];
      os << _T("Bottom Left Corner  fps=") << ::ConvertFromSysUnits(fps, unitMeasure::KSI) << _T("ksi, fup=") << ::ConvertFromSysUnits(fup, unitMeasure::KSI) << _T("ksi, fno=") << ::ConvertFromSysUnits(fno, unitMeasure::KSI) << _T("ksi, fdown=") << ::ConvertFromSysUnits(fdown, unitMeasure::KSI) << _T("ksi") << endl;

      fps = sectionResult.fps[WBFL::Stability::BottomRight];
      fup = sectionResult.f[WBFL::Stability::Superelevation][WBFL::Stability::ImpactUp][wind][WBFL::Stability::BottomRight];
      fno = sectionResult.f[WBFL::Stability::Superelevation][WBFL::Stability::NoImpact][wind][WBFL::Stability::BottomRight];
      fdown = sectionResult.f[WBFL::Stability::Superelevation][WBFL::Stability::ImpactDown][wind][WBFL::Stability::BottomRight];
      os << _T("Bottom Right Corner fps=") << ::ConvertFromSysUnits(fps, unitMeasure::KSI) << _T("ksi, fup=") << ::ConvertFromSysUnits(fup, unitMeasure::KSI) << _T("ksi, fno=") << ::ConvertFromSysUnits(fno, unitMeasure::KSI) << _T("ksi, fdown=") << ::ConvertFromSysUnits(fdown, unitMeasure::KSI) << _T("ksi") << endl;

      Float64 max_stress = Max(sectionResult.fMax[WBFL::Stability::Superelevation][WBFL::Stability::Top], sectionResult.fMax[WBFL::Stability::Superelevation][WBFL::Stability::Bottom]);
      Float64 min_stress = Min(sectionResult.fMin[WBFL::Stability::Superelevation][WBFL::Stability::Top], sectionResult.fMin[WBFL::Stability::Superelevation][WBFL::Stability::Bottom]);
      os << _T("Controlling Stress: Min =") << ::ConvertFromSysUnits(min_stress, unitMeasure::KSI) << _T("ksi, Max=") << ::ConvertFromSysUnits(max_stress, unitMeasure::KSI) << _T("ksi") << endl;
   }
   os <<_T(" Dump Complete")<<endl;
   os << _T("=============") <<endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsWsdotHaulingAnalysisArtifact::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsWsdotHaulingAnalysisArtifact");


   TESTME_EPILOG("HaulingAnalysisArtifact");
}
#endif // _UNITTEST


