///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include <Reporting\PrincipalTensionStressCheckTable.h>
#include <PgsExt\PgsExt.h>

#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace/Limits.h>
#include <IFace\Project.h>
#include <IFace\ReportOptions.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPrincipalTensionStressCheckTable::CPrincipalTensionStressCheckTable()
{
}

CPrincipalTensionStressCheckTable::CPrincipalTensionStressCheckTable(const CPrincipalTensionStressCheckTable& rOther)
{
   MakeCopy(rOther);
}

CPrincipalTensionStressCheckTable::~CPrincipalTensionStressCheckTable()
{
}

CPrincipalTensionStressCheckTable& CPrincipalTensionStressCheckTable::operator= (const CPrincipalTensionStressCheckTable& rOther)
{
   if (this != &rOther)
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CPrincipalTensionStressCheckTable::Build(rptChapter* pChapter, IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   pPara->SetName(_T("Principal Tensile Stresses in Webs"));
   *pPara << pPara->GetName() << rptNewLine;
   *pChapter << pPara;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   GET_IFACE2(pBroker, IBridge, pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   // check applicability
   if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::EighthEdition2017)
   {
      *pPara << _T("Principal tensile stresses in webs are not limited for the ") << WBFL::LRFD::BDSManager::GetSpecificationName() << _T(", ") << WBFL::LRFD::BDSManager::GetEditionAsString() << rptNewLine;
      return;
   }

   bool bApplicable = false;
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      const auto* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const auto* pArtifact = pSegmentArtifact->GetPrincipalTensionStressArtifact();
      if (pArtifact->IsApplicable())
      {
         // only has to be applicable for one segment to cause reportin
         bApplicable = true;
      }
      else
      {
         if (1 < nSegments)
         {
            *pPara << _T("Principal Tensile Stresses in Webs limitations are not applicable to Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
         }
         else
         {
            *pPara << _T("Principal Tensile Stresses in Webs limitations are not applicable.") << rptNewLine;
         }

         if (pArtifact->GetApplicability() == pgsPrincipalTensionStressArtifact::Specification)
         {
            *pPara << _T("Principal tensile stresses in webs are not limited for the ") << WBFL::LRFD::BDSManager::GetSpecificationName() << _T(", ") << WBFL::LRFD::BDSManager::GetEditionAsString() << rptNewLine;
         }
         else if (pArtifact->GetApplicability() == pgsPrincipalTensionStressArtifact::ConcreteStrength)
         {
            INIT_UV_PROTOTYPE(rptStressUnitValue, stress_u, pDisplayUnits->GetStressUnit(), true);
            *pPara << _T("Concrete strength does not exceed the ") << stress_u.SetValue(WBFL::Units::ConvertToSysUnits(10.0, WBFL::Units::Measure::KSI)) << _T(" threshold") << rptNewLine;
         }
         else
         {
            ATLASSERT(false); // is there a new applicability reason?
         }
      }
   }

   if (!bApplicable)
   {
      return;
   }

   BuildTable(pChapter, pBroker, pGirderArtifact, pDisplayUnits);
}

void CPrincipalTensionStressCheckTable::BuildTable(rptChapter* pChapter, IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, IEAFDisplayUnits* pDisplayUnits) const
{
   std::_tstring strImagePath = rptStyleManager::GetImagePath();

   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, l, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress_u, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);

   rptCapacityToDemand cap_demand;

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(girderKey));

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount() - 1;

   GET_IFACE2(pBroker, IMaterials, pMaterials);

   GET_IFACE2(pBroker, ISpecification, pSpec);
   GET_IFACE2(pBroker, ILibrary, pLib);
   std::_tstring specName = pSpec->GetSpecification();
   const auto* pSpecEntry = pLib->GetSpecEntry(specName.c_str());
   const auto& principal_tension_stress_criteria = pSpecEntry->GetPrincipalTensionStressCriteria();

   GET_IFACE2(pBroker, IConcreteStressLimits, pLimits);

   IntervalIndexType liveLoadInterval = pIntervals->GetLiveLoadInterval();

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   *pChapter << pPara;
   *pPara << _T("Interval ") << LABEL_INTERVAL(liveLoadInterval) << _T(" - ") << pIntervals->GetDescription(liveLoadInterval) << _T(", Service III") << rptNewLine;
   
   bool bIsUHPC = false;
   if (pMaterials->GetSegmentConcreteType(CSegmentKey(girderKey, 0)) == pgsTypes::UHPC)
   {
      bIsUHPC = true;
      ATLASSERT(nSegments == 1); // UHPC isn't available for spliced girders yet, so there better be only one segment
   }

   pPara = new rptParagraph;
   *pChapter << pPara;
   if (bIsUHPC)
   {
      *pPara << _T("AASHTO UHPC GS 1.9.2.3.3") << rptNewLine;
   }
   else
   {
      *pPara << _T("AASHTO LRFD BDS 5.9.2.3.3") << rptNewLine;
   }

   pPara = new rptParagraph;
   *pChapter << pPara;

   // list stress limits
   rptParagraph* pSegmentPara = nullptr;
   rptParagraph* pClosurePara = nullptr;
   if (1 < nSegments)
   {
      rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable(2);
      *pPara << pLayoutTable << rptNewLine;
      pSegmentPara = &(*pLayoutTable)(0, 0);
      pClosurePara = &(*pLayoutTable)(0, 1);
   }
   else
   {
      pSegmentPara = pPara;
   }

   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);

      const auto* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const auto* pArtifact = pSegmentArtifact->GetPrincipalTensionStressArtifact();
      const auto* pvArtifacts = pArtifact->GetPrincipalTensionStressArtifacts();

      if (1 < nSegments)
      {
         *pSegmentPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }
      Float64 fc = pMaterials->GetSegmentDesignFc(segmentKey, intervalIdx);
      *pSegmentPara << RPT_FC << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;

      pLimits->ReportSegmentConcreteWebPrincipalTensionStressLimit(segmentKey, pSegmentPara, pDisplayUnits);

      Float64 fc_reqd = pArtifact->GetRequiredSegmentConcreteStrength();
      
      GET_IFACE2(pBroker, IMaterials, pMaterials);
      GET_IFACE2(pBroker, IConcreteStressLimits, pLimits);
      auto name = pLimits->GetConcreteStressLimitParameterName(pgsTypes::Tension, pMaterials->GetSegmentConcreteType(segmentKey));

      if (0 < fc_reqd)
      {
         name[0] = std::toupper(name[0]);
         *pSegmentPara << name << _T(" required to satisfy this requirement = ") << stress_u.SetValue(fc_reqd) << rptNewLine;
      }
      else if (IsZero(fc_reqd))
      {
         // do nothing if exactly zero
      }
      else
      {
         *pSegmentPara << _T("Regardless of the ") << name << _T(", the stress requirements will not be satisfied.") << rptNewLine;
      }
      *pSegmentPara << rptNewLine;

      if (1 < nSegments && segIdx < nSegments-1)
      {
         CClosureKey closureKey(segmentKey);
         *pClosurePara << _T("Closure Joint ") << LABEL_SEGMENT(segIdx) << rptNewLine;

         Float64 fc = pMaterials->GetClosureJointDesignFc(closureKey, intervalIdx);
         *pClosurePara << RPT_FC << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;

         pLimits->ReportClosureJointConcreteWebPrincipalTensionStressLimit(closureKey, pClosurePara, pDisplayUnits);

         Float64 fc_reqd = pArtifact->GetRequiredClosureJointConcreteStrength();
         if (0 < fc_reqd)
         {
            *pClosurePara << _T("Concrete strength required to satisfy this requirement = ") << stress_u.SetValue(fc_reqd) << rptNewLine;
         }
         else if (IsZero(fc_reqd))
         {
            // do nothing if exactly zero
         }
         else
         {
            *pClosurePara << _T("Regardless of the concrete strength, the stress requirements will not be satisfied.") << rptNewLine;
         }
         *pClosurePara << rptNewLine;
      }
   }

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(5, _T(""));
   *pPara << pTable << rptNewLine;

   ColumnIndexType col = 0;

   (*pTable)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0, col++) << _T("Web") << rptNewLine << _T("Location");
   (*pTable)(0, col++) << COLHDR(_T("Y"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(RPT_STRESS(_T("max")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(0, col++) << _T("Status") << rptNewLine << _T("(C/D)");

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      const auto* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const auto* pArtifact = pSegmentArtifact->GetPrincipalTensionStressArtifact();
      const auto* pvArtifacts = pArtifact->GetPrincipalTensionStressArtifacts();

      for (const auto& artifact : *pvArtifacts)
      {
         col = 0;

         (*pTable)(row, col++) << location.SetValue(POI_SPAN, artifact.GetPointOfInterest());

         Float64 f_max, Yg;
         std::_tstring strLocation;
         artifact.GetfmaxDetails(&f_max, &Yg, &strLocation);

         (*pTable)(row, col++) << strLocation << rptNewLine;
         (*pTable)(row, col++) << l.SetValue(-Yg) << rptNewLine; // this is in girder section coordinates so negate to show positive value to user
         (*pTable)(row, col++) << stress.SetValue(f_max);

         bool bPassed = artifact.Passed();
         if (bPassed)
         {
            (*pTable)(row, col) << RPT_PASS;
         }
         else
         {
            (*pTable)(row, col) << RPT_FAIL;
         }

         Float64 fLimit = artifact.GetStressLimit();
         (*pTable)(row, col) << rptNewLine << _T("(") << cap_demand.SetValue(fLimit, f_max, bPassed) << _T(")");
         col++;

         row++;
      } // next artifact
   } // next segment

}

void CPrincipalTensionStressCheckTable::MakeCopy(const CPrincipalTensionStressCheckTable& rOther)
{
   // Add copy code here...
}

void CPrincipalTensionStressCheckTable::MakeAssignment(const CPrincipalTensionStressCheckTable& rOther)
{
   MakeCopy(rOther);
}
