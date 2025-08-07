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
#include <Reporting\MinDeckReinforcementCheck.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <IFace/Tools.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Artifact.h>
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>
#include <IFace\ReportOptions.h>

/****************************************************************************
CLASS
   CMinDeckReinforcementCheck
****************************************************************************/

CMinDeckReinforcementCheck::CMinDeckReinforcementCheck() 
{
}

LPCTSTR CMinDeckReinforcementCheck::GetName() const
{
   return TEXT("Minimum Deck Reinforcement Check");
}

void CMinDeckReinforcementCheck::Build(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
   GET_IFACE2(pBroker, ISpecification, pSpec);
   GET_IFACE2(pBroker, IBridge, pBridge);

   bool doCheck = pSpec->GetSpecificationType() >= WBFL::LRFD::BDSManager::Edition::TenthEdition2024 && pBridge->IsCompositeDeck();
   if (!doCheck)
   {
      return;
   }

   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stressWLabel, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), false);
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   SegmentIndexType nSegments = pBridge->GetSegmentCount(pGirderArtifact->GetGirderKey());

   location.IncludeSpanAndGirder(nSegments > 1);

   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName(_T("Details for Minimum Deck Reinforcement in Negative Moment Region"));
   *pParagraph << pParagraph->GetName() << _T(" [9.7.1.6]") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << _T("Where the longitudinal tensile stress in the concrete deck due to Load Combination Service I exceeds ") << symbol(phi) << Sub2(_T("f"), _T("r")) << _T(", the total cross-sectional area of the longitudinal reinforcement shall not be less than 1% of the total cross-sectional area of the concrete deck. ");

   auto* pSegmentArtifact0 = pGirderArtifact->GetSegmentArtifact(0);
   auto* pDeckReinfArtifact0 = pSegmentArtifact0->GetDeckReinforcementCheckArtifact();

   Float64 phi = pDeckReinfArtifact0->GetPhiFactor();

   IndexType numCastingRegions = pBridge->GetDeckCastingRegionCount();
   if (numCastingRegions < 2)
   {
      Float64 modRupt = pDeckReinfArtifact0->GetDeckReinforcementCheckAtPoisArtifact(0)->GetDeckModulusRupture();
      *pParagraph << _T("Deck modulus of rupture ") << Sub2(_T("f"), _T("r")) << _T(" = ") << stressWLabel.SetValue(modRupt) << _T(", ") << symbol(phi) << _T(" = ") << rptRcScalar(phi)
         << _T(", Stress Threshold where min reinforcement is required = ") << stressWLabel.SetValue(modRupt * phi) << rptNewLine;
   }
   else
   {
      *pParagraph << symbol(phi) << _T(" = ") << rptRcScalar(phi) << rptNewLine;
   }

   IndexType numColumns = numCastingRegions > 1 ? 7 : 6; // need to put rupture modulus in table if multiple regions

   rptRcTable* table = rptStyleManager::CreateDefaultTable(numColumns);
   *pParagraph << table << rptNewLine;

   IndexType col = 0;
   (*table)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0, col++) << COLHDR(_T("Stress at") << rptNewLine <<_T("Top of Deck"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*table)(0, col++) << COLHDR(_T("Area of")<<rptNewLine<<_T("CIP Deck"), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
   (*table)(0, col++) << COLHDR(_T("Area of Longitudinal")<<rptNewLine<<_T("Reinforcement"), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
   (*table)(0, col++) << _T("Percentage of") << rptNewLine << _T("Reinforcement");
   if (numCastingRegions > 1)
   {
      (*table)(0, col++) << COLHDR(_T("Modulus of Rupture") << rptNewLine << _T("of Slab Concrete, ")<<Sub2(_T("f"), _T("r")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   }
   (*table)(0, col++) << _T("Status");

   // Fill up the table
   RowIndexType row = table->GetNumberOfHeaderRows();

   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      auto* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      auto* pDeckReinfArtifact = pSegmentArtifact->GetDeckReinforcementCheckArtifact();
      IndexType nArtifacts = pDeckReinfArtifact->GetDeckReinforcementCheckAtPoisArtifactCount();

      for (IndexType Idx = 0; Idx < nArtifacts; Idx++)
      {
         const auto* pArtifact = pDeckReinfArtifact->GetDeckReinforcementCheckAtPoisArtifact(Idx);
         const pgsPointOfInterest& poi = pArtifact->GetPointOfInterest();

         col = 0;
         (*table)(row, col++) << location.SetValue(POI_SPAN, poi);
         (*table)(row, col++) << stress.SetValue(pArtifact->GetDeckTensileStress());
         (*table)(row, col++) << area.SetValue(pArtifact->GetAreaCIPDeck());
         (*table)(row, col++) << area.SetValue(pArtifact->GetAreaReinforcement());
         (*table)(row, col++) << scalar.SetValue(100.0 * pArtifact->GetAreaReinforcement() / pArtifact->GetAreaCIPDeck());
         if (numCastingRegions > 1)
         {
            (*table)(row, col++) << stress.SetValue(pArtifact->GetDeckModulusRupture());
         }

         if (!pArtifact->IsApplicable())
         {
            (*table)(row, col) << RPT_NA;
         }
         else
         {
            if (pArtifact->Passed())
            {
               (*table)(row, col) << RPT_PASS;
            }
            else
            {
               (*table)(row, col) << RPT_FAIL;
            }
         }

         row++;
      } // next artifact
   } // next segment
}
