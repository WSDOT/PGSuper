///////////////////////////////////////////////////////////////////////
// PGSplice - Precast Post-tensioned Spliced Girder Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <Reporting\DebondCheckTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\ReportPointOfInterest.h>

#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Allowables.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/****************************************************************************
CLASS
   CDebondCheckTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CDebondCheckTable::CDebondCheckTable()
{
}


CDebondCheckTable::~CDebondCheckTable()
{
}

//======================== OPERATORS  =======================================

//======================== OPERATIONS =======================================
void CDebondCheckTable::Build(rptChapter* pChapter, IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry);

   pgsTypes::StrandType strandType(pgsTypes::Straight); // we only debond straight strands

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   // are there any debonded strands in this girder?
   bool bDebondedStrands = false;
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      if (0 < pStrandGeometry->GetNumDebondedStrands(CSegmentKey(girderKey, segIdx), strandType, pgsTypes::dbetEither))
      {
         bDebondedStrands = true;
         break;
      }
   } // next segment

   if (!bDebondedStrands)
      return; // no debonded strands... nothing to report

   bool bAfter8thEdition = lrfdVersionMgr::NinthEdition2020 <= lrfdVersionMgr::GetVersion() ? true : false;

   // report debonding checks
   INIT_UV_PROTOTYPE(rptLengthUnitValue, loc, pDisplayUnits->GetSpanLengthUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, loc2, pDisplayUnits->GetSpanLengthUnit(), true);

   rptParagraph* p = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << p;
   *p <<_T("Debonded Strands [5.9.4.3.3]");

   GET_IFACE2(pBroker,IDebondLimits,pDebondLimits);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const CSegmentKey& segmentKey(pSegmentArtifact->GetSegmentKey());
      const pgsDebondArtifact* pDebondArtifact = pSegmentArtifact->GetDebondArtifact();

      if ( 1 < nSegments )
      {
         p = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         *pChapter << p;
         *p << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }

      p = new rptParagraph;
      *pChapter << p;

      if (pDebondArtifact->GetNumDebondedStrands() == 0)
      {
         *p << _T("This segment does not have debonded strands") << rptNewLine;
         continue;
      }

      // Max fraction for debonded strands.... not an LRFD requirement in 9th Edition
      if (pDebondArtifact->CheckMaxFraDebondedStrands())
      {
         Float64 fraction = 100.0*pDebondArtifact->GetFraDebondedStrands();
         Float64 limit = 100.0*pDebondArtifact->GetMaxFraDebondedStrands();

         StrandIndexType ndb = pDebondArtifact->GetNumDebondedStrands();
         StrandIndexType ns = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Permanent);

         if (limit < fraction)
         {
            *p << Bold(_T("Warning: "));
         }

         *p << fraction << _T("% (") << ndb << _T(" of ") << ns << _T(") strands are debonded. The number of debonded strands should not exceed ") << limit << _T("%.") << rptNewLine;
      }
      *p << rptNewLine;


      CComPtr<IIndexArray> arrayPermStrandIndex;
      pStrandGeometry->ComputePermanentStrandIndices(segmentKey, pgsTypes::Straight, &arrayPermStrandIndex);

      // Requirement A
      if (bAfter8thEdition)
      {
         switch (pDebondArtifact->GetSection())
         {
         case pgsDebondArtifact::I:
            *p << Bold(_T("Requirement A and Requirement I, 3rd bullet")) << rptNewLine;
            break;

         case pgsDebondArtifact::J:
            *p << Bold(_T("Requirement A and Requirement J, 3rd bullet")) << rptNewLine;
            break;

         case pgsDebondArtifact::K:
            *p << Bold(_T("Requirement A and Requirement K, 2nd bullet")) << rptNewLine;
            break;

         default:
            ATLASSERT(false); // should never get here... is there a new requirement type?
         }
      }
      *p << Build1(pDebondArtifact, bAfter8thEdition, pDisplayUnits);
      *p << Super(_T("*")) << _T(" ");
      if (bAfter8thEdition)
      {
         switch (pDebondArtifact->GetSection())
         {
         case pgsDebondArtifact::I:
            *p << _T("Bond the outer-most strands in all rows located within the full-width section of the flange.") << rptNewLine;
            break;

         case pgsDebondArtifact::J:
            *p << _T("Bond the outer-most strands within the section.") << rptNewLine;
            break;

         case pgsDebondArtifact::K:
            *p << _T("Bond the outer-most strands within the section, stem, or web.") << rptNewLine;
            break;

         default:
            ATLASSERT(false); // should never get here... is there a new requirement type?
         }
      }
      else
      {
         *p << _T("Exterior strands in each horizontal row shall be bonded.") << rptNewLine;
      }
      *p << rptNewLine;

      // Requirement B
      if (bAfter8thEdition)
      {
         *p << Bold(_T("Requirement B")) << rptNewLine;
      }
      StrandIndexType nDebonded10orLess, nDebonded;
      bool bCheckMax;
      Float64 fraMax;
      pDebondLimits->GetMaxDebondedStrandsPerSection(segmentKey, &nDebonded10orLess, &nDebonded, &bCheckMax, &fraMax);
      *p << _T("Debonding shall not be terminated for more than ") << nDebonded << _T(" strands in any given section. When a total of ten or fewer strands are debonded, debonding shall not be terminated for more than ") << nDebonded10orLess << _T(" strands in any given section");
      if (bCheckMax)
      {
         *p << _T(", but not more than ") << fraMax * 100 << _T("% of all strands may be debonded");
      }
      *p << _T(".") << rptNewLine;      
      *p << Build2(pDebondArtifact, pDisplayUnits) << rptNewLine;

      // Requirement C
      if (bAfter8thEdition)
      {
         *p << Bold(_T("Requirement C")) << rptNewLine;
      }
      Float64 ndb, minDistance;
      bool bUseDist;
      pDebondLimits->GetMinDistanceBetweenDebondSections(segmentKey, &ndb, &bUseDist, &minDistance);

      *p << _T("Longitudinal spacing of debonding termination locations shall be at least ") << ndb << Sub2(_T("d"), _T("b")) << _T(" apart");
      if (bUseDist)
      {
         *p << _T(" but not less than ") << loc.SetValue(minDistance);
      }
      *p << _T(".") << rptNewLine;
      Float64 mndbs = pDebondArtifact->GetMinDebondSectionSpacing();
      Float64 mndbsl = pDebondArtifact->GetDebondSectionSpacingLimit();
      *p << _T("The least distance between debond termination sections is ") << loc.SetValue(mndbs) << _T(" and the minimum distance is ") << loc2.SetValue(mndbsl) << rptNewLine;
      *p << (pDebondArtifact->PassedDebondTerminationSectionLocation() ? RPT_PASS : RPT_FAIL) << rptNewLine;
      *p << rptNewLine;

      bool bReqD = true;
      Uint16 symmetrical_debonding = pDebondArtifact->IsDebondingSymmetrical();
      if (pDebondArtifact->CheckDebondingSymmetry() && symmetrical_debonding != DEBOND_SYMMETRY_NA)
      {
         if (bAfter8thEdition)
         {
            *p << Bold(_T("Requirement D")) << rptNewLine;
         }
         *p << _T("Debonded strands shall be symmetrically distributed about the vertical centerline of the member. Debonding shall be terminated symmetrically at the same longitudinal location.") << rptNewLine;
         ATLASSERT(pDebondArtifact->PassedDebondingSymmetry() ? symmetrical_debonding == DEBOND_SYMMETRY_TRUE : symmetrical_debonding == DEBOND_SYMMETRY_FALSE);
         *p << (pDebondArtifact->PassedDebondingSymmetry() ? RPT_PASS : RPT_FAIL) << rptNewLine;
         *p << rptNewLine;
      }
      else
      {
         bReqD = false;
      }

      bool bReqE = true;
      if (pDebondArtifact->CheckAdjacentDebonding())
      {
         if (bAfter8thEdition)
         {
            *p << Bold(_T("Requirement E")) << rptNewLine;
         }
         *p << _T("Alternate bonded and debonded strand locations both horizontally and vertically.") << rptNewLine;
         if (pDebondArtifact->PassedAdjacentDebondedStrands())
         {
            *p << _T(" ") << RPT_PASS << rptNewLine;
         }
         else
         {
            *p << _T(" ") << RPT_FAIL << rptNewLine;
            for (int i = 0; i < 2; i++)
            {
               pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
               IndexType nAdj = pDebondArtifact->GetAdjacentDebondedStrandsCount(endType);
               if (0 < nAdj)
               {
                  if (endType == pgsTypes::metStart)
                  {
                     *p << _T("The following debonded strands at the start of girder are adjacent to each other:") << rptNewLine;
                  }
                  else
                  {
                     *p << _T("The following debonded strands at the end of girder are adjacent to each other:") << rptNewLine;
                  }
               }

               for (IndexType idx = 0; idx < nAdj; idx++)
               {
                  StrandIndexType strandIdx1, strandIdx2;
                  pDebondArtifact->GetAdjacentDebondedStrands(pgsTypes::metStart, idx, &strandIdx1, &strandIdx2);

                  StrandIndexType permStrandIdx1, permStrandIdx2;
                  arrayPermStrandIndex->get_Item(strandIdx1, &permStrandIdx1);
                  arrayPermStrandIndex->get_Item(strandIdx2, &permStrandIdx2);

                  *p << _T("(") << LABEL_INDEX(permStrandIdx1) << _T(", ") << LABEL_INDEX(permStrandIdx2) << _T(")") << rptNewLine;
               }
               *p << rptNewLine;
            }
         }
         *p << rptNewLine;
      }
      else
      {
         bReqE = false;
      }

      if (bAfter8thEdition)
      {
         *p << Bold(_T("Requirement F")) << rptNewLine;
      }
      *p << _T("Development lengths from the end of the debonded zone are determined using LRFD Eq. 5.9.4.3.2-1 with ") << symbol(kappa) << _T(" = 2.0.") << rptNewLine;
      *p << rptNewLine;

      if (bAfter8thEdition)
      {
         *p << Bold(_T("Requirement G")) << rptNewLine;
      }
      Float64 dbl_limit;
      pgsTypes::DebondLengthControl control;
      pDebondArtifact->GetDebondLengthLimit(&dbl_limit, &control);
      Float64 maxdbl = pDebondArtifact->GetMaxDebondLength();

      *p << _T("The longest debond length from the end of the member is ") << loc.SetValue(maxdbl) << _T(" and the permissible length limit is ") << loc2.SetValue(dbl_limit) << _T(" which is controlled by ");
      if (control == pgsTypes::mdbDefault)
      {
         *p << _T("development length from mid-girder.");
      }
      else if (control == pgsTypes::mbdFractional)
      {
         *p << _T("fraction of girder length.");
      }
      else
      {
         *p << _T("minimum length.");
      }
      *p << rptNewLine;

      *p << (pDebondArtifact->PassedDebondLength() ? RPT_PASS : RPT_FAIL) << rptNewLine;
      *p << rptNewLine;

      bool bReqIJK = true;
      if (pDebondArtifact->CheckDebondingInWebWidthProjection())
      {
         if (bAfter8thEdition)
         {
            switch (pDebondArtifact->GetSection())
            {
            case pgsDebondArtifact::I:
               *p << Bold(_T("Requirement I")) << rptNewLine;
               break;

            case pgsDebondArtifact::J:
               *p << Bold(_T("Requirement J")) << rptNewLine;
               break;

            case pgsDebondArtifact::K:
               *p << Bold(_T("Requirement K")) << rptNewLine;
               break;

            default:
               ATLASSERT(false); // should never get here... is there a new requirement type?
            }
         }

         if (pDebondArtifact->GetSection() != pgsDebondArtifact::K)
         {
            // bonded strands in the web width project is not part of requirement K
            if (pDebondArtifact->GetSection() == pgsDebondArtifact::I)
            {
               *p << _T("Bond all strands within the horizontal limits of the web when the total number of debonded strands exceeds 25 percent or when the bottom flange to web width ratio, ") << Sub2(_T("b"),_T("f")) << _T("/") << Sub2(_T("b"),_T("w")) << _T(", exceeds 4.") << rptNewLine;
               Float64 fraction = 100.0*pDebondArtifact->GetFraDebondedStrands();
               Float64 ratio_start = pDebondArtifact->GetBottomFlangeToWebWidthRatio(pgsTypes::metStart);
               Float64 ratio_end   = pDebondArtifact->GetBottomFlangeToWebWidthRatio(pgsTypes::metEnd);
               *p << _T("Debonding = ") << fraction << _T("%, ") << Sub2(_T("b"), _T("f")) << _T("/") << Sub2(_T("b"), _T("w")) << _T(" = ") << ratio_start <<_T(" at start, and = ") << ratio_end << _T(" at end of member.") << rptNewLine;
            }
            else
            {
               ATLASSERT(pDebondArtifact->GetSection() == pgsDebondArtifact::J);
               *p << _T("Strands shall be bonded within 1.0 times the web width projection.") << rptNewLine;
            }

            if (pDebondArtifact->PassedBondedStrandsInWebWidthProjection())
            {
               *p << _T(" ") << RPT_PASS << rptNewLine;
            }
            else
            {
               *p << _T(" ") << RPT_FAIL << rptNewLine;
               for (int i = 0; i < 2; i++)
               {
                  pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;

                  const auto& vStrands = pDebondArtifact->GetDebondedStrandsInWebWidthProjection(endType);
                  if (0 < vStrands.size())
                  {
                     if (endType == pgsTypes::metStart)
                     {
                        *p << _T("The following strands at the start of girder are debonded within the");
                     }
                     else
                     {
                        *p << _T("The following strands at the end of girder are debonded within the");
                     }

                     if (pDebondArtifact->GetSection() == pgsDebondArtifact::I)
                     {
                        *p << _T(" horizontal limits of the web:") << rptNewLine;
                     }
                     else
                     {
                        *p << _T(" web width projection:") << rptNewLine;
                     }

                     // the following two lines write a comma separated list
                     std::for_each(std::cbegin(vStrands), std::prev(std::cend(vStrands)), [&p,&arrayPermStrandIndex](const auto& strandIdx) 
                     { 
                        StrandIndexType permStrandIdx;
                        arrayPermStrandIndex->get_Item(strandIdx, &permStrandIdx);
                        *p << LABEL_INDEX(permStrandIdx) << _T(", "); 
                     });
                     StrandIndexType permStrandIdx;
                     arrayPermStrandIndex->get_Item(vStrands.back(), &permStrandIdx);
                     *p << LABEL_INDEX(permStrandIdx) << rptNewLine;
                  } // if strands
               } // member end (i)
            } // if passed
         } // if section not K
      } // if checking
      else
      {
         bReqIJK = false;
      } // if checking

      *p << rptNewLine;
      if (bAfter8thEdition || !bReqD || !bReqE)
      {
         *p << Bold(_T("The following requirements are not evaluated:")) << rptNewLine;
      }

      if (!bReqD)
      {
         if (bAfter8thEdition)
         {
            *p << _T("Requirement D - ");
         }
         *p << _T("Debonded strands shall be symmetrically distributed about the vertical centerline of the cross section of the member. Debonding shall be terminated symmetrically at the same longitudinal location.") << rptNewLine;
      }

      if (!bReqE)
      {
         if (bAfter8thEdition)
         {
            *p << _T("Requirement E - ");
         }
         *p << _T("Alternate bonded and debonded strands both horizontally and vertically") << rptNewLine;
      }

      if (bAfter8thEdition)
      {
         *p << _T("Requirement H - Not evaluated: LRFD 5.12.3.3.9a does not permit debonded strands to be used for positive moment connections at continuity diaphragms") << rptNewLine;
      }

      if (bAfter8thEdition)
      {
         switch (pDebondArtifact->GetSection())
         {
         case pgsDebondArtifact::I:
            if (!bReqIJK)
            {
               *p << _T("Requirement I, first and second bullets - Bond all strands within the horizontal limits of the web.") << rptNewLine;
            }
            *p << _T("Requirement I, fourth bullet - Position of debonded strands furthest from the vertical centerline") << rptNewLine;
            break;

         case pgsDebondArtifact::J:
            *p << _T("Requirement J, first bullet - Uniformly distributed debonded strands between webs.") << rptNewLine;
            if (!bReqIJK)
            {
               *p << _T("Requirement J, second bullet - Strands shall be bonded within 1.0 times the web width projection.") << rptNewLine;
            }
            break;

         case pgsDebondArtifact::K:
            *p << _T("Requirement K, first bullet - Debond uniformly across the width of the section.") << rptNewLine;
            break;

         default:
            ATLASSERT(false); // should never get here... is there a new requirement type?
         }
      }
   } // next Segment
}

rptRcTable* CDebondCheckTable::Build1(const pgsDebondArtifact* pDebondArtifact, bool bAfter8thEdition, IEAFDisplayUnits* pDisplayUnits) const
{
   ColumnIndexType nCols = 7;
   if (bAfter8thEdition && pDebondArtifact->GetSection() == pgsDebondArtifact::I)
   {
      nCols++;
   }
   rptRcTable* table = rptStyleManager::CreateDefaultTable(nCols, _T(""));

   ColumnIndexType col = 0;
   (*table)(0, col++) << _T("Row");
   (*table)(0, col++) << _T("Number") << rptNewLine << _T("Strands");
   (*table)(0, col++) << _T("Number") << rptNewLine << _T("Debonded") << rptNewLine << _T("Strands");
   (*table)(0, col++) << _T("% Debonded");
   (*table)(0, col++) << _T("% Debonded") << rptNewLine << _T("Limit");
   (*table)(0, col++) << _T("Outer-most") << rptNewLine << _T("Strand") << rptNewLine << _T("Bonded") << Super(_T("*"));
   if (bAfter8thEdition && pDebondArtifact->GetSection() == pgsDebondArtifact::I)
   {
      (*table)(0, col++) << _T("Row in full-width section of web");
   }
   (*table)(0, col++) << _T("Status");

   // Fill up the table
   table->TableLabel().SetStyleName(rptStyleManager::GetFootnoteStyle());
   RowIndexType table_row = table->GetNumberOfHeaderRows();


   std::vector<StrandIndexType> nStrandsInRow = pDebondArtifact->GetStrandCountInRow();
   std::vector<StrandIndexType> nDebondedStrandsInRow = pDebondArtifact->GetNumDebondedStrandsInRow();
   std::vector<Float64> vFra = pDebondArtifact->GetFraDebondedStrandsInRow();
   std::vector<Float64> vMaxFra = pDebondArtifact->GetMaxFraDebondedStrandsInRow();
   const std::set<std::tuple<RowIndexType, pgsDebondArtifact::State, WebIndexType>>& exteriorStrandDebondState = pDebondArtifact->GetExteriorStrandBondState();

   std::array<std::_tstring, 2> strYesNo{ _T("Yes"), _T("No") };

   auto iter = vFra.begin();
   auto end = vFra.end();
   RowIndexType rowIdx = 0;
   for (; iter != end; iter++, rowIdx++)
   {
      col = 0;
      (*table)(table_row, col++) << LABEL_INDEX(rowIdx);
      (*table)(table_row, col++) << nStrandsInRow[rowIdx];
      (*table)(table_row, col++) << nDebondedStrandsInRow[rowIdx];
      (*table)(table_row, col++) << vFra[rowIdx] * 100. << _T("%");
      (*table)(table_row, col++) << vMaxFra[rowIdx] * 100. << _T("%");

      bool bInFullWidthSectionOfFlange = true;
      auto nDebondStatesForThisRow = std::count_if(std::cbegin(exteriorStrandDebondState), std::cend(exteriorStrandDebondState), [rowIdx](const auto& item) {return std::get<0>(item) == rowIdx; });
      if (1 < nDebondStatesForThisRow)
      {
         ATLASSERT(bAfter8thEdition); // the per stem/web for type K sections is for 9th edition and later only
         ATLASSERT(pDebondArtifact->GetSection() == pgsDebondArtifact::K);

         // there is more than one entry for this row, meaning that there are multiple webs
         for (const auto& item : exteriorStrandDebondState)
         {
            if (std::get<0>(item) == rowIdx)
            {
               WebIndexType webIdx = std::get<2>(item);
               ATLASSERT(webIdx != INVALID_INDEX);
               (*table)(table_row, col) << _T("Web ") << LABEL_INDEX(webIdx) << _T(", ") << strYesNo.at(std::get<1>(item)) << rptNewLine;

               if (std::get<1>(item) == pgsDebondArtifact::None)
               {
                  ATLASSERT(false); // should never get here for multi-web sections
                  bInFullWidthSectionOfFlange = false;
               }
            }
         }
         ATLASSERT(bInFullWidthSectionOfFlange == true); // this should never be false for a multi-web section because it is not a requirement of that type of section
         col++;
      }
      else
      {
         auto found = std::find_if(std::cbegin(exteriorStrandDebondState), std::cend(exteriorStrandDebondState), [rowIdx](const auto& item) {return std::get<0>(item) == rowIdx; });
         ATLASSERT(found != std::cend(exteriorStrandDebondState));

         if (std::get<1>(*found) == pgsDebondArtifact::None)
         {
            ATLASSERT(pDebondArtifact->GetSection() != pgsDebondArtifact::K); // this is only applicable to type/requirement I and J, not K section
            bInFullWidthSectionOfFlange = false;
            (*table)(table_row, col++) << RPT_NA;
         }
         else
         {
            (*table)(table_row, col++) << strYesNo.at(std::get<1>(*found));
         }
         ATLASSERT(std::get<2>(*found) == INVALID_INDEX);
      }

      if (bAfter8thEdition && pDebondArtifact->GetSection() == pgsDebondArtifact::I)
      {
         (*table)(table_row, col++) << strYesNo.at(bInFullWidthSectionOfFlange ? 0 : 1);
      }

      (*table)(table_row, col++) << (pDebondArtifact->RowPassed(rowIdx) ? RPT_PASS : RPT_FAIL);

      table_row++;
   }

   return table;
}

rptRcTable* CDebondCheckTable::Build2(const pgsDebondArtifact* pDebondArtifact, IEAFDisplayUnits* pDisplayUnits) const
{
   rptRcTable* table = rptStyleManager::CreateDefaultTable(5, _T("Number of strands terminating debonding at each section"));

   ColumnIndexType col = 0;
   (*table)(0, col++) << _T("Debond") << rptNewLine << _T("Termination") << rptNewLine << _T("Section");
   (*table)(0, col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0, col++) << _T("Number") << rptNewLine << _T("Strands") << rptNewLine << _T("Terminating") << rptNewLine << _T("Debonding");
   (*table)(0, col++) << _T("Strand") << rptNewLine << _T("Debond") << rptNewLine << _T("Termination") << rptNewLine << _T("Limit");
   (*table)(0, col++) << _T("Status");

   INIT_UV_PROTOTYPE(rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false);

   StrandIndexType nMaxStrands1;
   bool bCheck;
   Float64 fraMaxStrands;
   pDebondArtifact->GetMaxDebondStrandsAtSection(&nMaxStrands1, &bCheck, &fraMaxStrands);

   StrandIndexType nDebondedStrands = pDebondArtifact->GetNumDebondedStrands();

   // allow int to floor
   StrandIndexType nMaxStrands2 = StrandIndexType(floor(fraMaxStrands * nDebondedStrands));
   StrandIndexType nMaxStrands = bCheck ? Max(nMaxStrands1, nMaxStrands2) : nMaxStrands1;

   RowIndexType row = table->GetNumberOfHeaderRows();
   SectionIndexType nSections = pDebondArtifact->GetNumDebondSections();
   for (SectionIndexType sectionIdx = 0; sectionIdx < nSections; sectionIdx++, row++)
   {
      col = 0;

      Float64 loc;
      StrandIndexType nStrands;
      Float64 fraStrands;

      pDebondArtifact->GetDebondSection(sectionIdx, &loc, &nStrands, &fraStrands);

      (*table)(row, col++) << LABEL_INDEX(sectionIdx);
      (*table)(row, col++) << location.SetValue(loc);
      (*table)(row, col++) << nStrands;

      (*table)(row, col++) << nMaxStrands;

      (*table)(row, col++) << (pDebondArtifact->SectionPassed(sectionIdx) ? RPT_PASS : RPT_FAIL);
   }

   return table;
}
