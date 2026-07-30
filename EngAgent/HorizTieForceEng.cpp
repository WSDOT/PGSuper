///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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
   auto nBottomFlanges = pGirder->GetBottomFlangeCount(girderKey);

   bool bIsApplicable = false;
   if (WBFL::LRFD::BDSManager::Edition::TenthEdition2024 <= WBFL::LRFD::BDSManager::GetEdition() && nWebs == 1 && nBottomFlanges == 1)
   {
      bIsApplicable = true;
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

   auto n = pSpecEntry->GetEndZoneCriteria().HorizTensionTieZoneLengthFactor;

   bool bIsChecked = pSpecEntry->GetEndZoneCriteria().bCheckHorizTensionTie;

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

      // This check is only intended for the simple ends of beams, not interior pier locations of spliced girders
      bool bIsBoundaryPier = pBridge->IsBoundaryPier(reactionLocation.PierIdx);

      bool bAnalyze = bIsApplicable && bIsBoundaryPier;
      if (bAnalyze)
      {
         Float64 Hg = pGirder->GetHeight(reactionLocation.poi);
         Float64 hb = pGirder->GetBottomFlangeThickness(reactionLocation.poi, 0);
         Float64 bw = pGirder->GetBottomFlangeWidth(reactionLocation.poi);

         const CBearingData2* pbd = pIBridgeDesc->GetBearingData(reactionLocation.PierIdx, (reactionLocation.Face == rftBack ? pgsTypes::Back : pgsTypes::Ahead), reactionLocation.poi.GetSegmentKey().girderIndex);
         Float64 bb = (pbd->Width <= bw? pbd->Width : bw);

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

               if (y <= hb)
               {
                  // only look at strands in the bottom flange - for spliced girders, straight strands could be in the top
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
         }

         // if there aren't any strands outside of the web area (nLeft, nRight are zero)
         // then use bb/2 for xp and 0 for yp.
         Float64 xp_left = nLeft == 0 ? bb / 2. : fabs(sum_x_left) / nLeft;
         Float64 yp_left = nLeft == 0 ? 0. : sum_y_left / nLeft;

         Float64 xp_right = nRight == 0 ? bb / 2. : fabs(sum_x_right) / nRight;
         Float64 yp_right = nRight == 0 ? 0. : sum_y_right / nRight;

         IndexType Nw = nStrands - nDebondedStrands; // total number of bonded strands at section

         //
         // Resistance
         //

         // Need to figure out the relative position on the segment where the reaction is occuring.
         // There is no direct way to get this information so two strategies will be used:
         // 1) Check if the poi is at a 10th Point POI. If the 10th Point is 1 or 11, then the reaction is at the start or end of the segment. 
         //    Otherwise, the reaction is somewhere in the middle of the segment.
         // 2) If the poi is not at a 10th Point, then use the distance from the start of the segment to determine if the reaction is at the start or end of the segment.
         //    If the poi is in the 1st 1/3 or the last 1/3 of the segment, then the reaction is at the start or end of the segment. Otherwise, the reaction is somewhere in the middle of the segment.
         auto tp = reactionLocation.poi.IsTenthPoint(POI_SPAN);

         if (tp == 0)
         {
            auto segment_length = pBridge->GetSegmentLength(reactionLocation.poi.GetSegmentKey());
            if (reactionLocation.poi.GetDistFromStart() <= segment_length / 3.)
            {
               tp = 1; // start of segment
            }
            else if (2. * segment_length / 3. <= reactionLocation.poi.GetDistFromStart())
            {
               tp = 11; // end of segment
            }
         }

         Float64 start_zl = 0.;
         Float64 end_zl = 0;

         // From LRFD 5.9.4.4.3, "The horizontal transverse tie reinforcement shall be uniformly distributed over a length of h/4 beyond the bearing"
         // We don't have a good way to assess if the reinforcement is uniformly distributed, so we will just use the average area of reinforcement over the tie reinforcement zone length.
         // Checking uniform distribution of reinforcement is a future enhancement.

         if (tp == 1)
         {
            start_zl = 0.;
            auto support_length = pBridge->GetSegmentStartSupportLength(reactionLocation.poi.GetSegmentKey());
            end_zl = reactionLocation.poi.GetDistFromStart() + support_length / 2. + Hg / n;
         }
         else if (tp == 11)
         {
            // at end of span
            auto support_length = pBridge->GetSegmentEndSupportLength(reactionLocation.poi.GetSegmentKey());
            auto segment_length = pBridge->GetSegmentLength(reactionLocation.poi.GetSegmentKey());
            start_zl = reactionLocation.poi.GetDistFromStart() - support_length / 2. - Hg / n;
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

         auto Avc = pStirrupGeometry->GetConfinementAv(reactionLocation.poi.GetSegmentKey(), start_zl, end_zl);

         Float64 Es, fy, fu;
         pMaterials->GetSegmentTransverseRebarProperties(reactionLocation.poi.GetSegmentKey(), &Es, &fy, &fu);

         for (auto& ls : vLimitStates)
         {
            pgsHorizontalTieForceArtifact artifact(reactionLocation, ls);
            artifact.IsApplicable(true);
            artifact.IsChecked(bIsChecked);

            artifact.SetTieReinforcementZoneRange(reactionLocation.poi.GetSegmentKey().segmentIndex,start_zl, end_zl);
            artifact.SetTieArea(Avc);
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
         } // next limit state
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

   GET_IFACE2(GetBroker(), ILibrary, pLib);
   GET_IFACE2(GetBroker(), ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   bool bIsChecked = pSpecEntry->GetEndZoneCriteria().bCheckHorizTensionTie;
   if (!bIsChecked)
   {
      *pPara << _T("Checking of the horizontal transverse tension tie reinforcement is disabled in the Project Criteria") << rptNewLine;
   }

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

         if (artifact->IsChecked())
         {
            bool bPassed = artifact->Passed();
            if (bPassed)
               (*pTable)(row, col) << RPT_PASS;
            else
               (*pTable)(row, col) << RPT_FAIL;

            (*pTable)(row, col) << rptNewLine << _T("(") << cdRatio.SetValue(artifact->GetTieResistance(), artifact->GetTieForce(), bPassed) << _T(")");
         }
         else
         {
            (*pTable)(row, col++) << _T("N/A");
         }
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
   INIT_UV_PROTOTYPE(rptLengthUnitValue, span, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), false);
   INIT_UV_PROTOTYPE(rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);

   auto pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("AASHTO LRFD BDS 5.9.4.4.3") << rptNewLine;
   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("horiz_tension_tie_force.png"), _T("A_s f_y=\\left(\\frac{n_f}{N_w}\\right)\\left[\\frac{x_p}{(h_b-y_p )}+\\frac{(x_p-c_b)}{y_p}\\right] \\frac{V_u}{\\phi}")) << rptNewLine;
   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("horiz_tension_tie_cb.png"), _T("c_b = \\left(\\frac{b_b}{2}\\right)\\left(1 - \\frac{n_f}{N_w}\\right) "));

   auto nArtifacts = pGirderArtifact->GetHorizontalTensionTieArtifactCount();

   GET_IFACE2(GetBroker(), IBridge, pBridge);
   auto nSegments = pBridge->GetSegmentCount(pGirderArtifact->GetGirderKey());

   bool bIsSymmetric = true;
   for (IndexType idx = 0; idx < nArtifacts; idx++)
   {
      auto* artifact = pGirderArtifact->GetHorizontalTensionTieArtifact(idx);
      bIsSymmetric &= artifact->IsSymmetric();
   }

   ColumnIndexType nColumns = bIsSymmetric ? 17 : 21;

   if (1 < nSegments)
      nColumns++; // add a column for segment number

   auto pTable = rptStyleManager::CreateDefaultTable(nColumns);
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));   
   *pPara << pTable << rptNewLine;

   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   auto strGirder = 1 < nSegments ? _T("segment") : _T("girder");
   *pPara << _T("*") << _T(" = Distance from the left end of the ") << strGirder << _T(" to the start and end of the transverse tie reinforcement") << rptNewLine;

   if (!bIsSymmetric)
   {
      pTable->SetNumberOfHeaderRows(2);
      pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << _T("** AASHTO Equation 5.9.4.4.3-1 assumes strands are place symmetrically about the centerline of the web. The strands are unsymmetric for this girder. The ") << RPT_AS << RPT_FY << _T(" demand is computed using the left and right side values and the average value used.") << rptNewLine;
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

   if (1 < nSegments)
   {
      if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
      (*pTable)(0, col++) << _T("Segment");
   }

   if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << COLHDR(Super2(_T("Start"),_T("*")), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   if (!bIsSymmetric) pTable->SetRowSpan(0, col, 2);
   (*pTable)(0, col++) << COLHDR(Super2(_T("End"),_T("*")), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

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
         auto [segIdx,xs, xe] = artifact->GetTieReinforcementZoneRange();
         if(1 < nSegments)
         {
            (*pTable)(row, col++) << LABEL_INDEX(segIdx);
         }
         (*pTable)(row, col++) << span.SetValue(xs);
         (*pTable)(row, col++) << span.SetValue(xe);
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
