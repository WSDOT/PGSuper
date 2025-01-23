///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software you can redistribute it and/or modify
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
#include <PgsExt\LRFDSplittingCheckEngineer.h>
#include <PgsExt\StrandData.h>
#include <PgsExt\DesignConfigUtil.h>

#include <psgLib/EndZoneCriteria.h>

#include <IFace\Project.h> // for ISpecification
#include <IFace\Bridge.h>
#include <IFace\SplittingChecks.h>
#include <IFace\PrestressForce.h>
#include <IFace\Intervals.h>
#include <IFace/Limits.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

pgsLRFDSplittingCheckEngineer::pgsLRFDSplittingCheckEngineer()
{
}

pgsLRFDSplittingCheckEngineer::pgsLRFDSplittingCheckEngineer(IBroker* pBroker) :
   pgsSplittingCheckEngineer(pBroker)
{
}

pgsLRFDSplittingCheckEngineer::~pgsLRFDSplittingCheckEngineer()
{
}

std::shared_ptr<pgsSplittingCheckArtifact> pgsLRFDSplittingCheckEngineer::Check(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig) const
{
   std::shared_ptr<pgsSplittingCheckArtifact> pArtifact(std::make_shared<pgsSplittingCheckArtifact>(segmentKey));

   GET_IFACE(IBridge, pBridge);
   GET_IFACE(IMaterials, pMaterials);
   GET_IFACE(IGirder, pGdr);
   GET_IFACE(ILosses, pLosses);
   GET_IFACE(IPretensionForce, pPrestressForce);

   // get POI at point of prestress transfer
   // this is where the prestress force is fully transfered
   std::array<pgsPointOfInterest, 2> poi = GetPointsOfInterest(segmentKey);
   pArtifact->SetPointsOfInterest(poi);

   Float64 start_h = pGdr->GetSplittingZoneHeight(poi[pgsTypes::metStart]);
   pArtifact->SetH(pgsTypes::metStart, start_h);

   Float64 end_h = pGdr->GetSplittingZoneHeight(poi[pgsTypes::metEnd]);
   pArtifact->SetH(pgsTypes::metEnd, end_h);

   // basically this is h/4 except that the 4 is a parametric value
   Float64 n = GetSplittingZoneLengthFactor();
   pArtifact->SetSplittingZoneLengthFactor(n);

   Float64 start_zl = start_h / n;
   Float64 end_zl = end_h / n;
   pArtifact->SetSplittingZoneLength(pgsTypes::metStart, start_zl);
   pArtifact->SetSplittingZoneLength(pgsTypes::metEnd, end_zl);
   ATLASSERT(IsEqual(start_zl, GetSplittingZoneLength(segmentKey, pgsTypes::metStart)));
   ATLASSERT(IsEqual(end_zl, GetSplittingZoneLength(segmentKey, pgsTypes::metEnd)));

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType jackingIntervalIdx = pIntervals->GetStressStrandInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   // Get the splitting force parameters (the artifact actually computes the splitting force)
   std::array<std::array<Float64, 3>, 2> Fpj;
   std::array<std::array<Float64, 3>, 2> dFpT;
   for (int i = 0; i < 2; i++)
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
      for (int j = 0; j < 3; j++)
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)j;
         Fpj[endType][strandType] = pPrestressForce->GetEffectivePrestress(poi[endType], strandType, jackingIntervalIdx, pgsTypes::Start, pConfig);
         dFpT[endType][strandType] = pLosses->GetEffectivePrestressLoss(poi[endType], strandType, releaseIntervalIdx, pgsTypes::End, pConfig);
      }
   }

#pragma Reminder("Simplify code - change methods on IStrandGeometry to work with pConfig=null so the code below can be refactored into a single set of logic and combined with the loop above")
   std::array<StrandIndexType, 3> Nstrands;
   std::array<std::array<StrandIndexType, 3>, 2> NdbStrands;
   if (pConfig == nullptr)
   {
      GET_IFACE(IStrandGeometry, pStrandGeometry);
      for (int i = 0; i < 2; i++)
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
         for (int j = 0; j < 3; j++)
         {
            pgsTypes::StrandType strandType = (pgsTypes::StrandType)j;
            Nstrands[strandType] = pStrandGeometry->GetStrandCount(segmentKey, strandType);
            NdbStrands[endType][strandType] = pStrandGeometry->GetNumDebondedStrands(segmentKey, strandType, (endType == pgsTypes::metStart ? pgsTypes::dbetStart : pgsTypes::dbetEnd));
         }
      }
   }
   else
   {
      for (int i = 0; i < 2; i++)
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
         for (int j = 0; j < 3; j++)
         {
            pgsTypes::StrandType strandType = (pgsTypes::StrandType)j;
            Nstrands[strandType] = pConfig->PrestressConfig.GetStrandCount(strandType);
            NdbStrands[endType][strandType] = pConfig->PrestressConfig.Debond[strandType].size();
         }
      }
   }

#pragma Reminder("Simplify code - change methods on IStrandGeometry to work with pConfig=null so the code below can be refactored into a single set of logic and combined with the loop above")
   // if the temporary strands aren't pretensioned, then they aren't in the section
   // when Splitting is checked!!!
   if (pConfig == nullptr)
   {
      GET_IFACE(ISegmentData, pSegmentData);
      const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
      if (pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned)
      {
         Nstrands[pgsTypes::Temporary] = 0;
         NdbStrands[pgsTypes::metStart][pgsTypes::Temporary] = 0;
         NdbStrands[pgsTypes::metEnd][pgsTypes::Temporary] = 0;
      }
   }
   else
   {
      if (pConfig->PrestressConfig.TempStrandUsage != pgsTypes::ttsPretensioned)
      {
         Nstrands[pgsTypes::Temporary] = 0;
         NdbStrands[pgsTypes::metStart][pgsTypes::Temporary] = 0;
         NdbStrands[pgsTypes::metEnd][pgsTypes::Temporary] = 0;
      }
   }

   for (int i = 0; i < 2; i++)
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
      for (int j = 0; j < 3; j++)
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)j;

         Float64 Aps = (Nstrands[strandType] - NdbStrands[endType][strandType]) * pMaterials->GetStrandMaterial(segmentKey, strandType)->GetNominalArea();

         Float64 Ps = 0.04 * Aps * (Fpj[endType][strandType] - dFpT[endType][strandType]);

         pArtifact->SetAps(endType, strandType, Aps);
         pArtifact->SetFpj(endType, strandType, Fpj[endType][strandType]);
         pArtifact->SetLossesAfterTransfer(endType, strandType, dFpT[endType][strandType]);
         pArtifact->SetSplittingForce(endType, strandType, Ps);
      }
   }

   // Compute Splitting resistance
   Float64 Es, fy, fu;
   pMaterials->GetSegmentTransverseRebarProperties(segmentKey, &Es, &fy, &fu);
   Float64 fs = GetMaxSplittingStress(fy);
   pArtifact->SetFs(pgsTypes::metStart, fs);
   pArtifact->SetFs(pgsTypes::metEnd, fs);

   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);


   // splitting direction must be set prior to getting Avs so we get the right value
   pgsTypes::SplittingDirection splittingDirection = pGdr->GetSplittingDirection(segmentKey);
   pArtifact->SetSplittingDirection(splittingDirection);

   std::array<Float64, 2> Avs;
   if (pConfig == nullptr)
   {
      GET_IFACE(IStirrupGeometry, pStirrupGeometry);
      Avs[pgsTypes::metStart] = pStirrupGeometry->GetSplittingAv(segmentKey, 0.0, start_zl);
      Avs[pgsTypes::metEnd] = pStirrupGeometry->GetSplittingAv(segmentKey, segment_length - end_zl, segment_length);
   }
   else
   {
      WBFL::Materials::Rebar::Type barType;
      WBFL::Materials::Rebar::Grade barGrade;
      pMaterials->GetSegmentTransverseRebarMaterial(segmentKey, &barType, &barGrade);
      GetSplittingAvFromStirrupConfig(pConfig->StirrupConfig, barType, barGrade, segment_length,start_zl, &Avs[pgsTypes::metStart], end_zl, &Avs[pgsTypes::metEnd]);
   }

   for (int i = 0; i < 2; i++)
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
      pArtifact->SetAvs(endType, Avs[endType]);

      Float64 Pr = fs * Avs[endType];
      pArtifact->SetSplittingResistance(endType, Pr);
   }

   return pArtifact;
}

Float64 pgsLRFDSplittingCheckEngineer::GetAsRequired(const pgsSplittingCheckArtifact* pArtifact) const
{
   std::array<Float64, 2> avs_reqd;
   for (int i = 0; i < 2; i++)
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
      Float64 Pr = pArtifact->GetSplittingForce(endType);
      Float64 fs = pArtifact->GetFs(endType);
      Float64 Lz = pArtifact->GetSplittingZoneLength(endType);

      Float64 Pc = GetConcreteCapacity(endType, pArtifact);

      avs_reqd[endType] = (Pr - Pc) / (fs * Lz);
   }

   return Max(avs_reqd[pgsTypes::metStart], avs_reqd[pgsTypes::metEnd]);
}

void pgsLRFDSplittingCheckEngineer::ReportSpecCheck(rptChapter* pChapter, const pgsSplittingCheckArtifact* pArtifact) const
{
   GET_IFACE(IEAFDisplayUnits, pDisplayUnits);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true);
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);

   GET_IFACE(IBridge, pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(pArtifact->GetSegmentKey());
   std::_tstring strSegment(1 < nSegments ? _T("Segment") : _T("Girder"));

   std::_tstring strName = pgsSplittingCheckEngineer::GetCheckName();


   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   (*pPara) << GetSpecReference() << rptNewLine;

   for (int i = 0; i < 2; i++)
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
      (*pPara) << bold(ON) << (endType == pgsTypes::metStart ? _T("Left") : _T("Right")) << _T(" end of ") << strSegment << bold(OFF) << rptNewLine;

      (*pPara) << strName << _T(" Zone Length = ") << length.SetValue(pArtifact->GetSplittingZoneLength(endType)) << rptNewLine;
      (*pPara) << strName << _T(" Demand = ") << force.SetValue(pArtifact->GetSplittingForce(endType)) << rptNewLine;
      (*pPara) << strName << _T(" Resistance = ") << force.SetValue(pArtifact->GetSplittingResistance(endType)) << rptNewLine;

      (*pPara) << _T("Status = ");
      if (pArtifact->Passed(endType))
      {
         (*pPara) << RPT_PASS;
      }
      else
      {
         (*pPara) << RPT_FAIL;
      }
      (*pPara) << rptNewLine << rptNewLine;
   }
}

void pgsLRFDSplittingCheckEngineer::ReportDetails(rptChapter* pChapter, const pgsSplittingCheckArtifact* pArtifact) const
{
   GET_IFACE(IEAFDisplayUnits, pDisplayUnits);

   GET_IFACE(ILosses, pLosses);
   bool bInitialRelaxation = pLosses->LossesIncludeInitialRelaxation();

   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, short_length, pDisplayUnits->GetComponentDimUnit(), true);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   std::_tstring strName = pgsSplittingCheckEngineer::GetCheckName();

   (*pPara) << GetSpecReference() << rptNewLine;

   for (int i = 0; i < 2; i++)
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;

      bool bTempStrands = IsZero(pArtifact->GetAps(endType, pgsTypes::Temporary)) ? false : true;

      if (endType == pgsTypes::metStart)
      {
         (*pPara) << Bold(_T("Left End:")) << rptNewLine;
      }
      else
      {
         (*pPara) << Bold(_T("Right End:")) << rptNewLine;
      }

      ReportDimensions(pPara,pDisplayUnits, strName, pArtifact, endType);
      (*pPara) << strName << _T(" Direction: ") << (pArtifact->GetSplittingDirection() == pgsTypes::sdVertical ? _T("Vertical") : _T("Horizontal")) << rptNewLine;

      ReportDemand(pPara, pDisplayUnits, strName, pArtifact, endType);
      ReportResistance(pPara, pDisplayUnits, strName, pArtifact, endType);
      (*pPara) << rptNewLine;
   }
}

Float64 pgsLRFDSplittingCheckEngineer::GetSplittingZoneLength(const CSegmentKey& segmentKey, pgsTypes::MemberEndType endType) const
{
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   const auto& end_zone_criteria = pSpecEntry->GetEndZoneCriteria();
   Float64 n = end_zone_criteria.SplittingZoneLengthFactor;

   std::array<pgsPointOfInterest, 2> poi = GetPointsOfInterest(segmentKey);

   GET_IFACE(IGirder, pGdr);
   Float64 h = pGdr->GetSplittingZoneHeight(poi[endType]);
   return h / n;
}

std::_tstring pgsLRFDSplittingCheckEngineer::GetSpecReference() const
{
   std::_tostringstream os;
   os << _T("AASHTO LRFD BDS ") << WBFL::LRFD::LrfdCw8th(_T("5.10.10.1"), _T("5.9.4.4.1"));
   return os.str();
}

void pgsLRFDSplittingCheckEngineer::ReportDimensions(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact,pgsTypes::MemberEndType endType) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, short_length, pDisplayUnits->GetComponentDimUnit(), true);
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   (*pPara) << strSplittingType << _T(" Dimension: h = ") << length.SetValue(pArtifact->GetH(endType)) << _T(" = ") << short_length.SetValue(pArtifact->GetH(endType)) << rptNewLine;
   (*pPara) << strSplittingType << _T(" Length: h/") << scalar.SetValue(pArtifact->GetSplittingZoneLengthFactor()) << _T(" = ") << length.SetValue(pArtifact->GetSplittingZoneLength(endType)) << _T(" = ") << short_length.SetValue(pArtifact->GetSplittingZoneLength(endType)) << rptNewLine;
}

void pgsLRFDSplittingCheckEngineer::ReportDemand(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const
{
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);

   GET_IFACE_NOCHECK(ISpecification, pSpecification); // not used if bInitialRelaxation is false

   GET_IFACE(ILosses, pLosses);
   bool bInitialRelaxation = pLosses->LossesIncludeInitialRelaxation();

   bool bTempStrands = IsZero(pArtifact->GetAps(endType, pgsTypes::Temporary)) ? false : true;

   if (bTempStrands)
   {
      (*pPara) << strSplittingType << _T(" Force at PSXFR: ") << Sub2(_T("P"),_T("s")) << _T(" = ") << Sub2(_T("P"), _T("perm")) << _T(" + ") << Sub2(_T("P"), _T("temp")) << rptNewLine;
      (*pPara) << Sub2(_T("P"), _T("perm")) << _T(" = 0.04(") << Sub2(_T("A"), _T("ps")) << _T(")(") << RPT_FPJ << _T(" - ");

      if (bInitialRelaxation)
      {
         if (pSpecification->GetSpecificationType() <= WBFL::LRFD::BDSManager::Edition::ThirdEdition2004)
         {
            (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pR1")) << _T(" - ");
         }
         else
         {
            (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pR0")) << _T(" - ");
         }
      }

      (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pES")) << _T(") = ");
      if (bInitialRelaxation)
      {
         (*pPara) << _T(" 0.04(") << Sub2(_T("A"), _T("ps")) << _T(")(") << RPT_FPJ << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pT")) << _T(") = ");
      }

      (*pPara) << _T("0.04(") << area.SetValue(pArtifact->GetAps(endType, pgsTypes::Permanent)) << _T(")(") << stress.SetValue(pArtifact->GetFpj(endType, pgsTypes::Permanent)) << _T(" - ");
      (*pPara) << stress.SetValue(pArtifact->GetLossesAfterTransfer(endType, pgsTypes::Permanent)) << _T(" ) = ") << force.SetValue(pArtifact->GetSplittingForce(endType, pgsTypes::Permanent)) << rptNewLine;

      (*pPara) << Sub2(_T("P"), _T("temp")) << _T(" = 0.04(") << Sub2(_T("A"), _T("ps")) << _T(")(") << RPT_FPJ << _T(" - ");

      if (bInitialRelaxation)
      {
         if (pSpecification->GetSpecificationType() <= WBFL::LRFD::BDSManager::Edition::ThirdEdition2004)
         {
            (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pR1")) << _T(" - ");
         }
         else
         {
            (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pR0")) << _T(" - ");
         }
      }

      (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pES")) << _T(") = ");
      if (bInitialRelaxation)
      {
         (*pPara) << _T(" 0.04(") << Sub2(_T("A"), _T("ps")) << _T(")(") << RPT_FPJ << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pT")) << _T(") = ");
      }

      (*pPara) << _T("0.04(") << area.SetValue(pArtifact->GetAps(endType, pgsTypes::Temporary)) << _T(")(") << stress.SetValue(pArtifact->GetFpj(endType, pgsTypes::Temporary)) << _T(" - ");
      (*pPara) << stress.SetValue(pArtifact->GetLossesAfterTransfer(endType, pgsTypes::Temporary)) << _T(" ) = ") << force.SetValue(pArtifact->GetSplittingForce(endType, pgsTypes::Temporary)) << rptNewLine;

      (*pPara) << Sub2(_T("P"),_T("s")) << _T(" = ") << force.SetValue(pArtifact->GetSplittingForce(endType)) << rptNewLine;
   }
   else
   {
      (*pPara) << strSplittingType << _T(" Force at PSXFR: ") << Sub2(_T("P"), _T("s")) << _T(" = ") << _T("0.04(") << Sub2(_T("A"), _T("ps")) << _T(")(") << RPT_FPJ << _T(" - ");

      if (bInitialRelaxation)
      {
         if (pSpecification->GetSpecificationType() <= WBFL::LRFD::BDSManager::Edition::ThirdEdition2004)
         {
            (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pR1")) << _T(" - ");
         }
         else
         {
            (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pR0")) << _T(" - ");
         }
      }

      (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pES")) << _T(") = ");
      if (bInitialRelaxation)
      {
         (*pPara) << _T(" 0.04(") << Sub2(_T("A"), _T("ps")) << _T(")(") << RPT_FPJ << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pT")) << _T(") = ");
      }

      (*pPara) << _T("0.04(") << area.SetValue(pArtifact->GetAps(endType, pgsTypes::Permanent)) << _T(")(") << stress.SetValue(pArtifact->GetFpj(endType, pgsTypes::Permanent)) << _T(" - ");
      (*pPara) << stress.SetValue(pArtifact->GetLossesAfterTransfer(endType, pgsTypes::Permanent)) << _T(" ) = ") << force.SetValue(pArtifact->GetSplittingForce(endType)) << rptNewLine;
   }
}

void pgsLRFDSplittingCheckEngineer::ReportResistance(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const
{
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);

   (*pPara) << strSplittingType << _T(" Resistance: ") << Sub2(_T("P"), _T("r")) << _T(" = ") << RPT_STRESS(_T("s")) << Sub2(_T("A"), _T("s"));
   (*pPara) << _T(" = ");
   (*pPara) << _T("(") << stress.SetValue(pArtifact->GetFs(endType)) << _T(")(") << area.SetValue(pArtifact->GetAvs(endType)) << _T(")");
   (*pPara) << _T(" = ") << force.SetValue(pArtifact->GetSplittingResistance(endType)) << rptNewLine;
}

Float64 pgsLRFDSplittingCheckEngineer::GetConcreteCapacity(pgsTypes::MemberEndType endType, const pgsSplittingCheckArtifact* pArtifact) const
{
   return 0;
}

Float64 pgsLRFDSplittingCheckEngineer::GetMaxSplittingStress(Float64 fy) const
{
   return WBFL::LRFD::Rebar::GetMaxBurstingStress(fy);
}

Float64 pgsLRFDSplittingCheckEngineer::GetSplittingZoneLengthFactor() const
{
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   return pSpecEntry->GetEndZoneCriteria().SplittingZoneLengthFactor;
}

std::array<pgsPointOfInterest, 2> pgsLRFDSplittingCheckEngineer::GetPointsOfInterest(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IPointOfInterest, pPOI);
   PoiList vPOI;
   pPOI->GetPointsOfInterest(segmentKey, POI_PSXFER, &vPOI);
   ATLASSERT(vPOI.size() != 0);
   std::array<pgsPointOfInterest, 2> poi{ vPOI.front(),vPOI.back() };
   return poi;
}

///////////////////////////////////
///////////////////////////////////
///////////////////////////////////

pgsPCIUHPCSplittingCheckEngineer::pgsPCIUHPCSplittingCheckEngineer() :
pgsLRFDSplittingCheckEngineer()
{
}

pgsPCIUHPCSplittingCheckEngineer::pgsPCIUHPCSplittingCheckEngineer(IBroker* pBroker) :
   pgsLRFDSplittingCheckEngineer(pBroker)
{
}

pgsPCIUHPCSplittingCheckEngineer::~pgsPCIUHPCSplittingCheckEngineer()
{
}

std::shared_ptr<pgsSplittingCheckArtifact> pgsPCIUHPCSplittingCheckEngineer::Check(const CSegmentKey& segmentKey,const GDRCONFIG* pConfig) const
{
   std::shared_ptr<pgsSplittingCheckArtifact> pArtifact = pgsLRFDSplittingCheckEngineer::Check(segmentKey,pConfig);

   GET_IFACE(IMaterials, pMaterials);
   ATLASSERT(pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC);

   const auto* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(pMaterials->GetSegmentConcrete(segmentKey).get());
   Float64 f_rr = pConcrete->GetPostCrackingTensileStrength();
   pArtifact->SetUHPCDesignTensileStrength(f_rr);

   GET_IFACE(IGirder, pGdr);
   GET_IFACE(IPretensionForce, pPrestress);

   for (int i = 0; i < 2; i++)
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;

      // Add the contribution of the fibers to the splitting resistance
      Float64 Pr = pArtifact->GetSplittingResistance(endType); // get basic splitting resistance

      const pgsPointOfInterest& poi = pArtifact->GetPointOfInterest(endType);

      Float64 bv = pGdr->GetShearWidth(poi);
      pArtifact->SetShearWidth(endType, bv);
      Float64 splitting_zone_length = pArtifact->GetSplittingZoneLength(endType); // (h/n)

      Pr += (f_rr / 2) * splitting_zone_length * bv; // add fiber contribution (PCI UHPC Eq 9.3.3.1-2)
      pArtifact->SetSplittingResistance(endType, Pr); // update the artifact


      // Compute the PCI UHPC bursting force
      Float64 Ps = pArtifact->GetSplittingForce(endType); // this is 0.04*Ppo
      Float64 Ppo = Ps / 0.04;

      Float64 h = pArtifact->GetH(endType);
      pgsTypes::TransferLengthType xferType = pgsTypes::TransferLengthType::Minimum; // using minimum so the splitting force is applied over the shorter distance, even thought this is a strength check
      // also, it doesn't really matter since PCI UHPC only has one transfer length
      Float64 lt = Max(pPrestress->GetTransferLength(segmentKey, pgsTypes::Straight, xferType), pPrestress->GetTransferLength(segmentKey, pgsTypes::Harped,xferType));
      pArtifact->SetTransferLength(endType, lt);
      Float64 Pb = (0.021 * h / lt)*Ppo; // PCI UHPC Eq 9.3.3.1-1
      pArtifact->SetBurstingForce(endType, Pb);
   }
   return pArtifact;
}

std::_tstring pgsPCIUHPCSplittingCheckEngineer::GetSpecReference() const
{
   std::_tostringstream os;
   os << __super::GetSpecReference() << _T(" and PCI UHPC E.SDG 9.3.3.1");
   return os.str();
}

void pgsPCIUHPCSplittingCheckEngineer::ReportDimensions(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, short_length, pDisplayUnits->GetComponentDimUnit(), true);

   __super::ReportDimensions(pPara, pDisplayUnits, strSplittingType, pArtifact, endType);

   (*pPara) << strSplittingType << _T(" Shear Width: ") << Sub2(_T("b"),_T("v")) << _T(" = ") << short_length.SetValue(pArtifact->GetShearWidth(endType)) << rptNewLine;
   (*pPara) << _T("Transfer Length: ") << Sub2(_T("l"), _T("t")) << _T(" = ") << short_length.SetValue(pArtifact->GetTransferLength(endType)) << rptNewLine;
}

void pgsPCIUHPCSplittingCheckEngineer::ReportDemand(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, short_length, pDisplayUnits->GetComponentDimUnit(), true);
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);
   __super::ReportDemand(pPara, pDisplayUnits, strSplittingType, pArtifact, endType);
   (*pPara) << _T("Bursting Force: ") << Sub2(_T("P"), _T("b")) << _T(" = ") << _T("0.021") << _T("(h/") << Sub2(_T("l"), _T("t")) << _T(")") << Sub2(_T("P"), _T("po"));
   (*pPara) << _T(" = ") << _T("0.021") << _T("(h/") << Sub2(_T("l"), _T("t")) << _T(")(") << Sub2(_T("P"), _T("s")) << _T("/0.04)");
                           
   Float64 h = pArtifact->GetH(endType);
   Float64 lt = pArtifact->GetTransferLength(endType);
   Float64 Ps = pArtifact->GetSplittingForce(endType);
   Float64 Pb = pArtifact->GetBurstingForce(endType);

   (*pPara) << _T(" = ") << _T("0.021(") << short_length.SetValue(h) << _T("/");
   (*pPara) << short_length.SetValue(lt) << _T(")(") << force.SetValue(Ps) << _T("/0.04)");
   (*pPara) << _T(" = ") << force.SetValue(Pb) << rptNewLine;

   (*pPara) << strSplittingType << _T(" Demand: Lessor of ") << Sub2(_T("P"), _T("s")) << _T(" and ") << Sub2(_T("P"), _T("b")) << _T(" = ") << force.SetValue(pArtifact->GetSplittingForce(endType)) << rptNewLine;
}

void pgsPCIUHPCSplittingCheckEngineer::ReportResistance(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const
{
   // NOTE: This method does not call it's superclass method
   INIT_UV_PROTOTYPE(rptLengthUnitValue, short_length, pDisplayUnits->GetComponentDimUnit(), true);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   (*pPara) << strSplittingType << _T(" Resistance: ") << Sub2(_T("P"), _T("r")) << _T(" = ") << RPT_STRESS(_T("s")) << Sub2(_T("A"), _T("s")) << _T(" + (") << RPT_STRESS(_T("rr")) << _T(" / 2)(h /") << scalar.SetValue(pArtifact->GetSplittingZoneLengthFactor()) << _T(")") << Sub2(_T("b"), _T("v"));
   (*pPara) << _T(" = ");
   (*pPara) << _T("(") << stress.SetValue(pArtifact->GetFs(endType)) << _T(")(") << area.SetValue(pArtifact->GetAvs(endType)) << _T(")");
   (*pPara) << _T(" + (") << stress.SetValue(pArtifact->GetUHPCDesignTensileStrength()) << _T(" /2)(") << short_length.SetValue(pArtifact->GetH(endType)) << _T("/") << scalar.SetValue(pArtifact->GetSplittingZoneLengthFactor()) << _T(")");
   (*pPara) << _T("(") << short_length.SetValue(pArtifact->GetShearWidth(endType)) << _T(")");
   (*pPara) << _T(" = ") << force.SetValue(pArtifact->GetSplittingResistance(endType)) << rptNewLine;
}

Float64 pgsPCIUHPCSplittingCheckEngineer::GetConcreteCapacity(pgsTypes::MemberEndType endType, const pgsSplittingCheckArtifact* pArtifact) const
{
   Float64 f_rr = pArtifact->GetUHPCDesignTensileStrength();
   Float64 Lz = pArtifact->GetSplittingZoneLength(endType); // this is h/n
   Float64 bv = pArtifact->GetShearWidth(endType);

   Float64 Pc = (f_rr / 2) * Lz * bv;
   return Pc;
}

///////////////////////////////////
///////////////////////////////////
///////////////////////////////////

pgsUHPCSplittingCheckEngineer::pgsUHPCSplittingCheckEngineer() :
   pgsLRFDSplittingCheckEngineer()
{
}

pgsUHPCSplittingCheckEngineer::pgsUHPCSplittingCheckEngineer(IBroker* pBroker) :
   pgsLRFDSplittingCheckEngineer(pBroker)
{
}

pgsUHPCSplittingCheckEngineer::~pgsUHPCSplittingCheckEngineer()
{
}

std::shared_ptr<pgsSplittingCheckArtifact> pgsUHPCSplittingCheckEngineer::Check(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig) const
{
   std::shared_ptr<pgsSplittingCheckArtifact> pArtifact = pgsLRFDSplittingCheckEngineer::Check(segmentKey, pConfig);

   GET_IFACE(IMaterials, pMaterials);
   ATLASSERT(pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC);

   Float64 ft_cri = pMaterials->GetSegmentConcreteInitialEffectiveCrackingStrength(segmentKey);
   pArtifact->SetUHPCDesignTensileStrength(ft_cri);

   GET_IFACE(IConcreteStressLimits, pLimits);
   Float64 gamma_u = pLimits->GetUHPCTensionStressLimitCoefficient(segmentKey);

   GET_IFACE(IGirder, pGdr);
   for (int i = 0; i < 2; i++)
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;

      // Add the contribution of the fibers to the splitting resistance
      Float64 Pr = pArtifact->GetSplittingResistance(endType); // get basic splitting resistance

      const pgsPointOfInterest& poi = pArtifact->GetPointOfInterest(endType);

      Float64 bv = pGdr->GetShearWidth(poi);
      pArtifact->SetShearWidth(endType, bv);
      Float64 splitting_zone_length = pArtifact->GetSplittingZoneLength(endType); // (h/n)

      Float64 Pr_UHPC = gamma_u * ft_cri * bv * splitting_zone_length;

      pArtifact->SetSplittingResistance(endType, Pr + Pr_UHPC); // update the artifact
   }
   return pArtifact;
}

std::_tstring pgsUHPCSplittingCheckEngineer::GetSpecReference() const
{
   std::_tostringstream os;
   os << __super::GetSpecReference() << _T(" and GS 1.9.4.4.1");
   return os.str();
}

void pgsUHPCSplittingCheckEngineer::ReportDimensions(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, short_length, pDisplayUnits->GetComponentDimUnit(), true);

   __super::ReportDimensions(pPara, pDisplayUnits, strSplittingType, pArtifact, endType);

   (*pPara) << strSplittingType << _T(" Shear Width: ") << Sub2(_T("b"), _T("v")) << _T(" = ") << short_length.SetValue(pArtifact->GetShearWidth(endType)) << rptNewLine;
}

void pgsUHPCSplittingCheckEngineer::ReportDemand(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const
{
   __super::ReportDemand(pPara, pDisplayUnits, strSplittingType, pArtifact, endType);
}

void pgsUHPCSplittingCheckEngineer::ReportResistance(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const
{
   // NOTE: This method does not call it's superclass method - that's by design.
   INIT_UV_PROTOTYPE(rptLengthUnitValue, short_length, pDisplayUnits->GetComponentDimUnit(), true);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   const auto& segmentKey = pArtifact->GetSegmentKey();

   GET_IFACE(IConcreteStressLimits, pLimits);
   Float64 gamma_u = pLimits->GetUHPCTensionStressLimitCoefficient(segmentKey);

   (*pPara) << strSplittingType << _T(" Resistance: ") << Sub2(_T("P"), _T("r")) << _T(" = ") << RPT_STRESS(_T("s")) << Sub2(_T("A"), _T("s")) << _T(" + ") << Sub2(symbol(gamma),_T("u")) << RPT_STRESS(_T("t,cri")) << Sub2(_T("b"),_T("v")) << _T("h/") << scalar.SetValue(pArtifact->GetSplittingZoneLengthFactor());
   (*pPara) << _T(" = ");
   (*pPara) << _T("(") << stress.SetValue(pArtifact->GetFs(endType)) << _T(")(") << area.SetValue(pArtifact->GetAvs(endType)) << _T(")");
   (*pPara) << _T(" + (") << scalar.SetValue(gamma_u) << _T(")(") << stress.SetValue(pArtifact->GetUHPCDesignTensileStrength()) << _T(")(") << short_length.SetValue(pArtifact->GetShearWidth(endType)) << _T(")");
   (*pPara) << _T("(") << short_length.SetValue(pArtifact->GetH(endType)) << _T("/") << scalar.SetValue(pArtifact->GetSplittingZoneLengthFactor()) << _T(")");
   (*pPara) << _T(" = ") << force.SetValue(pArtifact->GetSplittingResistance(endType)) << rptNewLine;
}

Float64 pgsUHPCSplittingCheckEngineer::GetConcreteCapacity(pgsTypes::MemberEndType endType, const pgsSplittingCheckArtifact* pArtifact) const
{
   Float64 ft_cr = pArtifact->GetUHPCDesignTensileStrength();
   Float64 Lz = pArtifact->GetSplittingZoneLength(endType); // this is h/n
   Float64 bv = pArtifact->GetShearWidth(endType);

   Float64 Pc = ft_cr * Lz * bv;
   return Pc;
}
