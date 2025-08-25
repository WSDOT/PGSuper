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
#include "HorizTieForceEng.h"
#include <AgentTools.h>
#include <IFace/Bridge.h>
#include <IFace/AnalysisResults.h>
#include <IFace/Intervals.h>
#include <IFace/PointOfInterest.h>
#include <IFace/Project.h>
#include <EAF/EAFDisplayUnits.h>

#include <PgsExt/GirderArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#include <PsgLib/GirderLabel.h>
#include <psgLib/SpecLibraryEntry.h>
#include <psgLib/EndZoneCriteria.h>
#include <PsgLib/BearingData2.h>

#include <Reporting\ReactionInterfaceAdapters.h>

#include <MathEx.h>

pgsHorizTieForceEng::pgsHorizTieForceEng(std::weak_ptr<WBFL::EAF::Broker> broker, StatusGroupIDType statusGroupID) :
   m_pBroker(broker), m_StatusGroupID(statusGroupID)
{
}

void pgsHorizTieForceEng::Check(const CGirderKey& girderKey, pgsGirderArtifact* pGdrArtifact) const
{
   GET_IFACE2(GetBroker(), IGirder, pGirder);
   auto nWebs = pGirder->GetWebCount(girderKey);

   bool bIsApplicable = true;
   if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::TenthEdition2024 || nWebs != 1)
   {
      // if needed, the applicability check can be made stronger by checking the beam family and beam types
      // For now, only I-beams have single webs, so checking the web count is sufficient
      bIsApplicable = false;
   }

   GET_IFACE2(GetBroker(), IIntervals, pIntervals);
   auto lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

   GET_IFACE2(GetBroker(), IProductForces, pProductForces);
   auto bat = pProductForces->GetBridgeAnalysisType(pgsTypes::OptimizationType::Maximize);

   Float64 phi = 0.9; // Phi for strut-and-tie... hard code for now

   std::vector<pgsTypes::LimitState> vLimitStates{ pgsTypes::StrengthI };
   GET_IFACE2(GetBroker(), ILimitStateForces, pLimitStateForces);
   if (pLimitStateForces->IsStrengthIIApplicable(girderKey))
   {
      vLimitStates.push_back(pgsTypes::StrengthII);
   }

   GET_IFACE2(GetBroker(), ILibrary, pLib);
   GET_IFACE2(GetBroker(), ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   auto n = pSpecEntry->GetEndZoneCriteria().SplittingZoneLengthFactor; // this is for h/4

   GET_IFACE2(GetBroker(), IBearingDesign, pBearingDesign);
   CmbLsBearingDesignReactionAdapter adapter(pBearingDesign, lastIntervalIdx, girderKey);
   GET_IFACE2(GetBroker(), IBridge, pBridge);
   GET_IFACE2(GetBroker(), IPointOfInterest, pPoi);
   auto iter = adapter.GetReactionLocations(pBridge, pPoi);

   // this interfaces are only used in the loop below
   GET_IFACE2_NOCHECK(GetBroker(), IStrandGeometry, pStrandGeometry);
   GET_IFACE2_NOCHECK(GetBroker(), IStirrupGeometry, pStirrupGeometry);
   GET_IFACE2_NOCHECK(GetBroker(), IMaterials, pMaterials);
   GET_IFACE2_NOCHECK(GetBroker(), IBridgeDescription, pIBridgeDesc);

   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      const ReactionLocation& reactionLocation(iter.CurrentItem());
      // Use IBearingDesign->GetBearingLimitStateReaction to get Vu
      // Use the reactionLocation.poi to get girder properties such as Hg and #webs

      if (!bIsApplicable)
      {
         for (auto& ls : vLimitStates)
         {
            pgsHorizontalTieForceArtifact artifact(reactionLocation, ls);
            if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::TenthEdition2024 || nWebs != 1)
            {
               // This check is only applicable starting with LRFD 10th Edition and it is only for
               // I-Beams with a single web
               artifact.IsApplicable(false);
               pGdrArtifact->AddHorizontalTensionTieArtifact(artifact);
               continue;
            }
         }
      }
      else
      {
         Float64 Hg = pGirder->GetHeight(reactionLocation.poi);
         Float64 hb = pGirder->GetBottomFlangeThickness(reactionLocation.poi, 0);

         const CBearingData2* pbd = pIBridgeDesc->GetBearingData(reactionLocation.PierIdx, (reactionLocation.Face == rftBack ? pgsTypes::Back : pgsTypes::Ahead), reactionLocation.poi.GetSegmentKey().girderIndex);
         Float64 bb = pbd->Width;

         std::vector<CComPtr<IRect2d>> rects = pStrandGeometry->GetWebWidthProjections(reactionLocation.poi);
         ASSERT(rects.size() == 1);

         Float64 x_left, x_right;
         rects[0]->get_Left(&x_left);
         rects[0]->get_Right(&x_right);

         StrandIndexType nLeft = 0;
         StrandIndexType nRight = 0;

         Float64 sum_x_left = 0.;
         Float64 sum_y_left = 0.;
         Float64 sum_x_right = 0.;
         Float64 sum_y_right = 0.;

         auto nStrands = pStrandGeometry->GetStrandCount(reactionLocation.poi.GetSegmentKey(), pgsTypes::Straight);
         StrandIndexType nDebondedStrands = 0;

         for (auto strandIdx = 0; strandIdx < nStrands; strandIdx++)
         {
            if (pStrandGeometry->IsStrandDebonded(reactionLocation.poi, strandIdx, pgsTypes::Straight))
            {
               nDebondedStrands++;
            }
            else
            {
               CComPtr<IPoint2d> p;
               pStrandGeometry->GetStrandPosition(reactionLocation.poi, strandIdx, pgsTypes::Straight, &p);
               Float64 x, y;
               p->Location(&x, &y);

               y += Hg; // y is in girder coordinates, we want y measured up from bottom of girder. Add girder height
               ASSERT(0.0 <= y); // y should always be a positive value

               if (x < x_left)
               {
                  // strand is left of web projection box
                  nLeft++;
                  sum_x_left += x;
                  sum_y_left += y;
               }
               else if (x_right < x)
               {
                  // strand is right of web projection box
                  nRight++;
                  sum_x_right += x;
                  sum_y_right += y;
               }
            }
         }

         // if there aren't any strands outside of the web area (nLeft, nRight are zero)
         // then use bb/2 for xp and 0 for yp.
         Float64 xp_left = nLeft == 0 ? bb / 2. : fabs(sum_x_left) / nLeft;
         Float64 yp_left = nLeft == 0 ? 0. : sum_y_left / nLeft;

         Float64 xp_right = nRight == 0 ? bb / 2. : fabs(sum_x_right) / nRight;
         Float64 yp_right = nRight == 0 ? 0. : sum_y_right / nRight;

         IndexType Nw = nStrands - nDebondedStrands; // total number of bonded strands at section

         // Resistance
         auto tp = reactionLocation.poi.IsTenthPoint(POI_SPAN);
         Float64 start_zl = 0.;
         Float64 end_zl = Hg / n;

         if (tp == 1)
         {
            // do nothing
         }
         else if (tp == 11)
         {
            // at end of span
            auto segment_length = pBridge->GetSegmentLength(reactionLocation.poi.GetSegmentKey());
            start_zl = segment_length - Hg / n;
            end_zl = segment_length;
         }
         else
         {
            // this is supposed to handle the case of the bearing being somewhere in the middle of a segment, such as
            // for a spliced girder pier segment. LRFD is not clear what to do. It makes sense to use the reinforcement on
            // either side of the bearing
            start_zl = reactionLocation.poi.GetDistFromStart() - 0.5 * (Hg / n);
            end_zl = reactionLocation.poi.GetDistFromStart() + 0.5 * (Hg / n);
         }
         auto Avs = pStirrupGeometry->GetSplittingAv(reactionLocation.poi.GetSegmentKey(), 0.0, Hg / n);

         Float64 Es, fy, fu;
         pMaterials->GetSegmentTransverseRebarProperties(reactionLocation.poi.GetSegmentKey(), &Es, &fy, &fu);

         for (auto& ls : vLimitStates)
         {
            pgsHorizontalTieForceArtifact artifact(reactionLocation, ls);
            artifact.IsApplicable(true);

            artifact.SetTieArea(Avs);
            artifact.SetTieYieldStrength(fy);

            Float64 Rmin, Rmax;
            pBearingDesign->GetBearingLimitStateReaction(lastIntervalIdx, reactionLocation, ls, bat, true, &Rmin, &Rmax);
            Float64 Vu = max(Rmin, Rmax);

            artifact.SetBearingWidth(bb);
            artifact.SetBottomBulbDepth(hb);
            artifact.SetTotalNumBondedStrands(Nw);

            artifact.SetNumBondedStrandsInFlange(pgsTypes::stLeft, nLeft);
            artifact.SetHorizDistance(pgsTypes::stLeft, xp_left);
            artifact.SetVertDistance(pgsTypes::stLeft, yp_left);

            artifact.SetNumBondedStrandsInFlange(pgsTypes::stRight, nRight);
            artifact.SetHorizDistance(pgsTypes::stRight, xp_right);
            artifact.SetVertDistance(pgsTypes::stRight, yp_right);

            artifact.SetShearForce(Vu);
            artifact.SetPhi(phi);

            pGdrArtifact->AddHorizontalTensionTieArtifact(artifact);
         }
      }
   }
}

void pgsHorizTieForceEng::ReportHorizontalTensionTieForceChecks(const pgsGirderArtifact* pGirderArtifact, rptChapter* pChapter) const
{
   GET_IFACE2(GetBroker(), IEAFDisplayUnits, pDisplayUnits);

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true);
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);
   rptCapacityToDemand cdRatio;

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   pPara->SetName(_T("Horizontal Transverse Tension Tie Reinforcement Check"));
   *pChapter << pPara;
   (*pPara) << pPara->GetName() << _T(" [5.9.4.4.3]") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   auto pTable = rptStyleManager::CreateDefaultTable(5);
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   *pPara << pTable << rptNewLine;

   ColumnIndexType col = 0;
   (*pTable)(0, col++) << _T("Location");
   (*pTable)(0, col++) << _T("Limit State");
   (*pTable)(0, col++) << COLHDR(RPT_AS << RPT_FY << rptNewLine << _T("(Demand)"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(0, col++) << COLHDR(RPT_AS << RPT_FY << rptNewLine << _T("(Capacity)"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(0, col++) << _T("Status") << rptNewLine << _T("(C/D)") << rptNewLine;

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   auto nArtifacts = pGirderArtifact->GetHorizontalTensionTieArtifactCount();
   for ( auto idx = 0; idx < nArtifacts; idx++, row++)
   {
      col = 0;
      const auto artifact = pGirderArtifact->GetHorizontalTensionTieArtifact(idx);

      (*pTable)(row, col++) << artifact->GetReactionLocation().PierLabel;
      (*pTable)(row, col++) << GetLimitStateString(artifact->GetLimitState());
      if (artifact->IsApplicable())
      {
         (*pTable)(row, col++) << force.SetValue(artifact->GetTieForce());
         (*pTable)(row, col++) << force.SetValue(artifact->GetTieResistance());
         bool bPassed = artifact->Passed();
         if (bPassed)
            (*pTable)(row, col) << RPT_PASS;
         else
            (*pTable)(row, col) << RPT_FAIL;

         (*pTable)(row, col) << rptNewLine << _T("(") << cdRatio.SetValue(artifact->GetTieResistance(), artifact->GetTieForce(), bPassed) << _T(")");
      }
      else
      {
         (*pTable)(row, col++) << _T("");
         (*pTable)(row, col++) << _T("");
         (*pTable)(row, col++) << _T("N/A");
      }
   } // next artifact
}

void pgsHorizTieForceEng::ReportHorizontalTensionTieForceCheckDetails(const pgsGirderArtifact* pGirderArtifact, rptChapter* pChapter) const
{
   GET_IFACE2(GetBroker(), IEAFDisplayUnits, pDisplayUnits);

   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), false);
   INIT_UV_PROTOTYPE(rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);

   auto pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("AASHTO LRFD BDS 5.9.4.4.3") << rptNewLine;
   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("horiz_tension_tie_force.png"), _T("A_s f_y=\\left(\\frac{n_f}{N_w}\\right)\\left[\\frac{x_p}{(h_b-y_p )}+\\frac{(x_p-c_b)}{y_p}\\right] \\frac{V_u}{\\phi}")) << rptNewLine;
   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("horiz_tension_tie_cb.png"), _T("c_b = \\left(\\frac{b_b}{2}\\right)\\left(1 - \\frac{n_f}{N_w}\\right) "));

   auto nArtifacts = pGirderArtifact->GetHorizontalTensionTieArtifactCount();

   bool bIsSymmetric = true;
   for (IndexType idx = 0; idx < nArtifacts; idx++)
   {
      auto* artifact = pGirderArtifact->GetHorizontalTensionTieArtifact(idx);
      bIsSymmetric &= artifact->IsSymmetric();
   }

   ColumnIndexType nColumns = bIsSymmetric ? 15 : 19;

   auto pTable = rptStyleManager::CreateDefaultTable(nColumns);
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));   
   *pPara << pTable << rptNewLine;

   if (!bIsSymmetric)
   {
      pTable->SetNumberOfHeaderRows(2);
      pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << _T("* AASHTO Equation 5.9.4.4.3-1 assumes strands are place symmetrically about the centerline of the web. The strands are unsymmetric for this girder. The ") << RPT_AS << RPT_FY << _T(" demand is computed using the left and right side values and the average value used.") << rptNewLine;
   }

   ColumnIndexType col = 0;
   if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << _T("Location");

   if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << _T("Limit") << rptNewLine << _T("State");

   if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << COLHDR(Sub2(_T("b"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << COLHDR(Sub2(_T("h"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << Sub2(_T("N"), _T("w"));

   if (bIsSymmetric)
   {
      if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
      (*pTable)(0, col++) << Sub2(_T("n"), _T("f"));

      if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
      (*pTable)(0, col++) << COLHDR(Sub2(_T("x"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
      (*pTable)(0, col++) << COLHDR(Sub2(_T("y"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
      (*pTable)(0, col++) << COLHDR(Sub2(_T("c"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }
   else
   {
      pTable->SetColumnSpan(0, col, 4);
      (*pTable)(0, col) << _T("Left Side");
      (*pTable)(1, col++) << Sub2(_T("n"), _T("f"));
      (*pTable)(1, col++) << COLHDR(Sub2(_T("x"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(1, col++) << COLHDR(Sub2(_T("y"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(1, col++) << COLHDR(Sub2(_T("c"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      pTable->SetColumnSpan(0, col, 4);
      (*pTable)(0, col) << _T("Right Side");
      (*pTable)(1, col++) << Sub2(_T("n"), _T("f"));
      (*pTable)(1, col++) << COLHDR(Sub2(_T("x"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(1, col++) << COLHDR(Sub2(_T("y"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(1, col++) << COLHDR(Sub2(_T("c"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }

   if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << symbol(phi);

   if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << COLHDR(RPT_Vu, rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

   if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << COLHDR(RPT_AS << RPT_FY << rptNewLine << _T("(Demand)"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

   if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << COLHDR(RPT_AS, rptAreaUnitTag, pDisplayUnits->GetAreaUnit());

   if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << COLHDR(RPT_FY, rptStressUnitTag, pDisplayUnits->GetStressUnit());

   if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << COLHDR(RPT_AS << RPT_FY << rptNewLine << _T("(Capacity)"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());


   auto row = pTable->GetNumberOfHeaderRows();
   for (IndexType idx = 0; idx < nArtifacts; idx++, row++)
   {
      col = 0;
      auto* artifact = pGirderArtifact->GetHorizontalTensionTieArtifact(idx);

      (*pTable)(row, col++) << artifact->GetReactionLocation().PierLabel;
      (*pTable)(row, col++) << GetLimitStateString(artifact->GetLimitState());
      if (artifact->IsApplicable())
      {
         (*pTable)(row, col++) << dim.SetValue(artifact->GetBearingWidth());
         (*pTable)(row, col++) << dim.SetValue(artifact->GetBottomBulbDepth());
         (*pTable)(row, col++) << artifact->GetTotalNumBondedStrands();

         (*pTable)(row, col++) << artifact->GetNumBondedStrandsInFlange(pgsTypes::stLeft);
         (*pTable)(row, col++) << dim.SetValue(artifact->GetHorizDistance(pgsTypes::stLeft));
         (*pTable)(row, col++) << dim.SetValue(artifact->GetVertDistance(pgsTypes::stLeft));
         (*pTable)(row, col++) << dim.SetValue(artifact->GetBearingReactionLocation(pgsTypes::stLeft));

         if (!bIsSymmetric)
         {
            (*pTable)(row, col++) << artifact->GetNumBondedStrandsInFlange(pgsTypes::stRight);
            (*pTable)(row, col++) << dim.SetValue(artifact->GetHorizDistance(pgsTypes::stRight));
            (*pTable)(row, col++) << dim.SetValue(artifact->GetVertDistance(pgsTypes::stRight));
            (*pTable)(row, col++) << dim.SetValue(artifact->GetBearingReactionLocation(pgsTypes::stRight));
         }

         (*pTable)(row, col++) << artifact->GetPhi();
         (*pTable)(row, col++) << force.SetValue(artifact->GetShearForce());
         (*pTable)(row, col++) << force.SetValue(artifact->GetTieForce());
         (*pTable)(row, col++) << area.SetValue(artifact->GetTieArea());
         (*pTable)(row, col++) << stress.SetValue(artifact->GetTieYieldStrength());
         (*pTable)(row, col++) << force.SetValue(artifact->GetTieResistance());
      }
      else
      {
         pTable->SetColumnSpan(row, col, pTable->GetNumberOfColumns() - 2);
         (*pTable)(row, col++) << _T("Not applicable");
      }
   }
}
