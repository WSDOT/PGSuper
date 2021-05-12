///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include <Reporting\PrincipalTensionStressDetailsChapterBuilder.h>

#include <IFace\Artifact.h>
#include <IFace\PrincipalWebStress.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>

#include <PgsExt\GirderArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPrincipalTensionStressDetailsChapterBuilder::CPrincipalTensionStressDetailsChapterBuilder(bool bSelect) :
   CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CPrincipalTensionStressDetailsChapterBuilder::GetName() const
{
   return TEXT("Principal Tension Stresses in Webs Details");
}

rptChapter* CPrincipalTensionStressDetailsChapterBuilder::Build(CReportSpecification* pRptSpec, Uint16 level) const
{
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   if (pGdrRptSpec)
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      girderKey = pGdrLineRptSpec->GetGirderKey();
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec, level);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,ISpecification,pSpec);
   ISpecification::PrincipalWebStressCheckType  checkType = pSpec->GetPrincipalWebStressCheckType(CSegmentKey(girderKey,0));

   if (ISpecification::pwcNCHRPTimeStepMethod == checkType)
   {
      *pPara << _T("Details for Time-Dependent Principal Web Stress computations may be found in the Principal Web Stress Details report.") << rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bTimeStepMethod = pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP;

   // This report does not apply to the time step method
   if (false) //bTimeStepMethod)
   {
      *pPara << _T("Principal web stress details for time-step analyses can be found in the Principal Web Stress Details report.") << rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker, IArtifact, pIArtifact);
   const pgsGirderArtifact* pGirderArtifact = pIArtifact->GetGirderArtifact(girderKey);

   GET_IFACE2(pBroker, IBridge, pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   // check applicability
   if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::EighthEdition2017)
   {
      *pPara << _T("Principal tensile stresses in webs are not limited for the ") << lrfdVersionMgr::GetCodeString() << _T(", ") << lrfdVersionMgr::GetVersionString() << rptNewLine;
      return pChapter;
   }

   GET_IFACE2_NOCHECK(pBroker, IEAFDisplayUnits, pDisplayUnits);

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
            *pPara << _T("Principal tensile stresses in webs are not limited for the ") << lrfdVersionMgr::GetCodeString() << _T(", ") << lrfdVersionMgr::GetVersionString() << rptNewLine;
         }
         else if (pArtifact->GetApplicability() == pgsPrincipalTensionStressArtifact::ConcreteStrength)
         {
            INIT_UV_PROTOTYPE(rptStressUnitValue, stress_u, pDisplayUnits->GetStressUnit(), true);
            *pPara << _T("Concrete strength does not exceed the ") << stress_u.SetValue(::ConvertToSysUnits(10.0, unitMeasure::KSI)) << _T(" threshold") << rptNewLine;
         }
         else
         {
            ATLASSERT(false); // is there a new applicabilty reason?
         }
      }
   }

   if (!bApplicable)
   {
      return pChapter;
   }

   std::_tstring strImagePath = rptStyleManager::GetImagePath();

   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, l, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE(rptLength3UnitValue, l3, pDisplayUnits->GetSectModulusUnit(), false);
   INIT_UV_PROTOTYPE(rptLength4UnitValue, l4, pDisplayUnits->GetMomentOfInertiaUnit(), false);
   INIT_UV_PROTOTYPE(rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);

   location.IncludeSpanAndGirder(1 < nSegments);

   // need to know if there are any tendons. if so, we need a footnote
   // if nGirderDucts + nSegmentDucts > 0, we need the footnote... the actual sum doesn't have to be accurate
   GET_IFACE2(pBroker, IGirderTendonGeometry, pGirderTendonGeom);
   GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeom);
   DuctIndexType nGirderDucts = pGirderTendonGeom->GetDuctCount(girderKey);
   DuctIndexType nSegmentDucts = 0; // will add to this as we loop over segments

   pgsTypes::PrincipalTensileStressMethod method;
   Float64 coefficient, ductDiameterFactor,  ungroutedMultiplier, groutedMultiplier, principalTensileStressFcThreshold;
   pSpecEntry->GetPrincipalTensileStressInWebsParameters(&method, &coefficient,&ductDiameterFactor,&ungroutedMultiplier,&groutedMultiplier, &principalTensileStressFcThreshold);

   *pPara << _T("Y = elevation in web where principal stress is computed, measured downwards from top centerline of non-composite girder.") << rptNewLine;
   *pPara << RPT_STRESS(_T("top")) << _T(" and ") << RPT_STRESS(_T("bot")) << _T(" Service III stress including effective prestress.") << rptNewLine;
   *pPara << rptRcImage(strImagePath + _T("fpcx.png")) << rptNewLine;
   if (method == pgsTypes::ptsmLRFD)
   {
      *pPara << rptRcImage(strImagePath + _T("ShearStress_LRFD.png")) << rptNewLine;
   }
   else
   {
      *pPara << rptRcImage(strImagePath + _T("ShearStress_NCHRP.png")) << rptNewLine;
   }
   *pPara << _T("Principal Tension Stress: ") << rptNewLine << rptRcImage(strImagePath + _T("PrincipalTensionStress.png")) << rptNewLine;

   // set up the table
   ColumnIndexType nCols = 14;
   if (method == pgsTypes::ptsmNCHRP)
   {
      nCols += 3;
   }
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols, _T("Principal Stress Details"));
   *pPara << pTable << rptNewLine;

   ColumnIndexType col = 0;

   (*pTable)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0, col++) << COLHDR(Sub2(_T("V"), _T("p")), rptForceUnitTag, pDisplayUnits->GetShearUnit());

   if (method == pgsTypes::ptsmLRFD)
   {
      (*pTable)(0, col++) << COLHDR(Sub2(_T("I"), _T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
   }
   else
   {
      (*pTable)(0, col++) << COLHDR(Sub2(_T("I"), _T("nc")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      (*pTable)(0, col++) << COLHDR(Sub2(_T("I"), _T("cmp")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
   }
   (*pTable)(0, col++) << COLHDR(Sub2(_T("H"), _T("g")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << _T("Web") << rptNewLine << _T("Location");
   (*pTable)(0, col++) << COLHDR(_T("Y"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(RPT_STRESS(_T("top")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(0, col++) << COLHDR(RPT_STRESS(_T("bot")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(0, col++) << COLHDR(RPT_STRESS(_T("pcx")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   if (method == pgsTypes::ptsmLRFD)
   {
      (*pTable)(0, col++) << COLHDR(Sub2(_T("V"), _T("u")), rptForceUnitTag, pDisplayUnits->GetShearUnit());
      (*pTable)(0, col++) << COLHDR(Sub2(_T("Q"), _T("g")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit());
   }
   else
   {
      (*pTable)(0, col++) << COLHDR(Sub2(_T("V"), _T("nc")), rptForceUnitTag, pDisplayUnits->GetShearUnit());
      (*pTable)(0, col++) << COLHDR(Sub2(_T("V"), _T("cmp")), rptForceUnitTag, pDisplayUnits->GetShearUnit());
      (*pTable)(0, col++) << COLHDR(Sub2(_T("Q"), _T("nc")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit());
      (*pTable)(0, col++) << COLHDR(Sub2(_T("Q"), _T("cmp")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit());
   }
   (*pTable)(0, col++) << COLHDR(Sub2(_T("b"), _T("w")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(symbol(tau), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(0, col++) << COLHDR(RPT_STRESS(_T("max")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

   // fill the table
   GET_IFACE2(pBroker, IPrincipalWebStress, pPrincipalWebStress);
   RowIndexType row = pTable->GetNumberOfHeaderRows();
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      nSegmentDucts += pSegmentTendonGeom->GetDuctCount(segmentKey); // just trying to figure out if there are segment ducts..doesn't matter of total is for all segments

      const auto* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const auto* pArtifact = pSegmentArtifact->GetPrincipalTensionStressArtifact();

      const auto* pvArtifacts = pArtifact->GetPrincipalTensionStressArtifacts();

      for (const auto& artifact : *pvArtifacts)
      {
         col = 0;

         const pgsPointOfInterest& poi(artifact.GetPointOfInterest());
         const auto* pDetails = pPrincipalWebStress->GetPrincipalWebStressDetails(poi);

         (*pTable)(row, col++) << location.SetValue(POI_SPAN, poi);
         (*pTable)(row, col++) << shear.SetValue(pDetails->Vp);
         if (method == pgsTypes::ptsmLRFD)
         {
            (*pTable)(row, col++) << l4.SetValue(pDetails->Ic);
         }
         else
         {
            (*pTable)(row, col++) << l4.SetValue(pDetails->Inc);
            (*pTable)(row, col++) << l4.SetValue(pDetails->Ic);
         }

         (*pTable)(row, col++) << l.SetValue(pDetails->Hg);

         const auto& webSections = pDetails->WebSections;
         for (const auto& webSection : webSections)
         {
            ColumnIndexType c = col;
            if (webSection.bIsShearWidthAdjustedForTendon)
            {
               (*pTable)(row, c++) << webSection.strLocation << _T(" *") << rptNewLine;
            }
            else
            {
               (*pTable)(row, c++) << webSection.strLocation << rptNewLine;
            }
            (*pTable)(row, c++) << l.SetValue(-webSection.YwebSection) << rptNewLine; // this is in girder section coordinates so negate to show positive value to user
            (*pTable)(row, c++) << stress.SetValue(webSection.fTop) << rptNewLine;
            (*pTable)(row, c++) << stress.SetValue(webSection.fBot) << rptNewLine;
            (*pTable)(row, c++) << stress.SetValue(webSection.fpcx) << rptNewLine;

            if (method == pgsTypes::ptsmLRFD)
            {
               (*pTable)(row, c++) << shear.SetValue(webSection.Vu) << rptNewLine;
               (*pTable)(row, c++) << l3.SetValue(webSection.Qc) << rptNewLine;
            }
            else
            {
               (*pTable)(row, c++) << shear.SetValue(pDetails->Vnc) << rptNewLine;
               (*pTable)(row, c++) << shear.SetValue(webSection.Vu) << rptNewLine;
               (*pTable)(row, c++) << l3.SetValue(webSection.Qnc) << rptNewLine;
               (*pTable)(row, c++) << l3.SetValue(webSection.Qc) << rptNewLine;
            }

            (*pTable)(row, c++) << l.SetValue(webSection.bw) << rptNewLine;

            (*pTable)(row, c++) << stress.SetValue(webSection.t) << rptNewLine;
            (*pTable)(row, c++) << stress.SetValue(webSection.fmax) << rptNewLine;
         }
         row++;
      }
   } // next segment

   if (0 < (nGirderDucts + nSegmentDucts))
   {
      // there is at least one duct so add the footnote
      pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << _T("* indicates web width has been reduced because an internal tendon crosses near the depth at which the maximum principal tension is being checked [LRFD 5.7.2.1, 5.9.2.3.3]");
   }
   return pChapter;
}

CChapterBuilder* CPrincipalTensionStressDetailsChapterBuilder::Clone() const
{
   return new CPrincipalTensionStressDetailsChapterBuilder();
}
