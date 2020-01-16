///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
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

#include <Reporting\DesignOutcomeChapterBuilder.h>

#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Artifact.h>
#include <IFace\Bridge.h>
#include <IFace\GirderHandling.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\GirderHandlingSpecCriteria.h>

#include <Lrfd\RebarPool.h>

#include <PgsExt\GirderDesignArtifact.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\Helpers.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDesignOutcomeChapterBuilder
****************************************************************************/

void write_artifact_data(IBroker* pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsSegmentDesignArtifact* pArtifact);
void failed_design(IBroker* pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsSegmentDesignArtifact* pArtifact);
void successful_design(IBroker* pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsSegmentDesignArtifact* pArtifact);
void multiple_girder_table(ColumnIndexType startIdx, ColumnIndexType endIdx,IBroker* pBroker,const std::vector<CGirderKey>& girderKeys,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,IArtifact* pIArtifact);
void process_artifacts(IBroker* pBroker,ColumnIndexType startIdx, ColumnIndexType endIdx, const std::vector<CGirderKey>& girderKeys, IArtifact* pIArtifact,
                       const pgsGirderDesignArtifact** pArtifacts, bool& didFlexure, bool& didShear, bool& didLifting, bool& didHauling, bool& didSlabOffset, bool& didAssumedExcessCamber, bool& isHarped, bool& isTemporary);
void write_primary_shear_data(rptParagraph* pParagraph, IEAFDisplayUnits* pDisplayUnits,Float64 girderLength, ZoneIndexType nz,const CShearData2* pShearData);
void write_horiz_shear_data(rptParagraph* pParagraph, IEAFDisplayUnits* pDisplayUnits, Float64 girderLength, const CShearData2* pShearData);
void write_additional_shear_data(rptParagraph* pParagraph, IEAFDisplayUnits* pDisplayUnits, Float64 girderLength, const CShearData2* pShearData);
void write_design_notes(rptParagraph* pParagraph, const std::vector<pgsSegmentDesignArtifact::DesignNote>& notes);
void write_design_failures(rptParagraph* pParagraph, const pgsSegmentDesignArtifact* pArtifact);

// Function to compute columns in table that attempts to group all girders in a span per table
static const int MIN_TBL_COLS=3; // Minimum columns in multi-girder table
static const int MAX_TBL_COLS=8; // Maximum columns in multi-girder table

// Some fails can be successses (with caveats)
bool WasDesignSuccessful( pgsSegmentDesignArtifact::Outcome outcome)
{
      return outcome == pgsSegmentDesignArtifact::Success ||
             outcome == pgsSegmentDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionCy ||
             outcome == pgsSegmentDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionLifting ||
             outcome == pgsSegmentDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionHauling;
}

std::list<ColumnIndexType> ComputeTableCols(const std::vector<CGirderKey>& girderKeys)
{
   // Idea here is to break tables at spans. 
   // First build list of sizes of contiguous blocks of spans
   std::list<ColumnIndexType> contiguous_blocks1;
   SpanIndexType curr_span(INVALID_INDEX);
   bool first=false;
   std::vector<CGirderKey>::const_iterator it(girderKeys.begin());
   std::vector<CGirderKey>::const_iterator itEnd(girderKeys.end());
   for( ; it != itEnd; it++)
   {
      const CGirderKey& girderKey = *it;
      SpanIndexType new_span = girderKey.groupIndex;
      GirderIndexType new_gdr = girderKey.girderIndex;

      if (first || curr_span!=new_span)
      {
         first = false;
         curr_span = new_span;
         contiguous_blocks1.push_back(1);
      }
      else
      {
         contiguous_blocks1.back()++;
      }
   }

   // Next break blocks into list of table-sized chunks 
   std::list<ColumnIndexType> contiguous_blocks2;
   for(std::list<ColumnIndexType>::const_iterator it=contiguous_blocks1.begin(); it!=contiguous_blocks1.end(); it++)
   {
      ColumnIndexType ncols = *it;
      if (ncols > MAX_TBL_COLS)
      {
         ColumnIndexType num_big_chunks = ncols / MAX_TBL_COLS;
         ColumnIndexType rmdr = ncols % MAX_TBL_COLS;

         for (ColumnIndexType ich=0; ich<num_big_chunks; ich++)
         {
            contiguous_blocks2.push_back(MAX_TBL_COLS);
         }

         if(rmdr != 0)
         {
            contiguous_blocks2.push_back(rmdr);
         }
      }
      else
      {
         contiguous_blocks2.push_back(ncols);
      }
   }

   // Now we have a "right-sized" columns, but we could have a list of one-column tables, which
   // would be ugly. If all num colums are LE than min, combine into a wider, but not pretty table
   bool is_ugly = true;
   for(std::list<ColumnIndexType>::const_iterator it=contiguous_blocks2.begin(); it!=contiguous_blocks2.end(); it++)
   {
      ColumnIndexType ncols = *it;
      if (ncols > MIN_TBL_COLS)
      {
         is_ugly = false; // we have at least one table of minimum width - we're not ugly.
         break;
      }
   }

   std::list<ColumnIndexType> final_blocks;
   if (!is_ugly)
   {
      final_blocks = contiguous_blocks2;
   }
   else
   {
      // work to combine blocks
      std::list<ColumnIndexType>::const_iterator it=contiguous_blocks2.begin();
      while(it!=contiguous_blocks2.end())
      {
         ColumnIndexType ncols = *it;
         while (ncols<=MAX_TBL_COLS)
         {
            it++;
            if (it==contiguous_blocks2.end() || ncols+(*it) > MAX_TBL_COLS)
            {
               final_blocks.push_back(ncols);
               break;
            }
            else
            {
               ncols+= (*it);
            }
         }
      }
   }

   return final_blocks;
}

CDesignOutcomeChapterBuilder::CDesignOutcomeChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CDesignOutcomeChapterBuilder::GetName() const
{
   return TEXT("Design Outcome");
}

rptChapter* CDesignOutcomeChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CMultiGirderReportSpecification* pReportSpec = dynamic_cast<CMultiGirderReportSpecification*>(pRptSpec);
   ATLASSERT( pReportSpec != nullptr );

   std::vector<CGirderKey> girderKeys = pReportSpec->GetGirderKeys();

   CComPtr<IBroker> pBroker;
   pReportSpec->GetBroker(&pBroker);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2( pBroker, IEAFDisplayUnits, pDisplayUnits );
   GET_IFACE2( pBroker, IArtifact, pIArtifact );
   GET_IFACE2( pBroker, IBridge, pBridge);

   // Write multiple girder table only if we have more than one girder
   std::list<ColumnIndexType> table_cols = ComputeTableCols(girderKeys);

   if (!table_cols.empty() && !(table_cols.size()==1 && table_cols.front()==1) )
   {
      // List contains number of columns in each table
      bool first = true;
      ColumnIndexType start_idx, end_idx;
      for (std::list<ColumnIndexType>::iterator itcol = table_cols.begin(); itcol!=table_cols.end(); itcol++)
      {
         if (first)
         {
            start_idx = 0;
            end_idx = *itcol-1;
            first = false;
         }
         else
         {
            start_idx = end_idx+1;
            end_idx += *itcol;
            ATLASSERT(end_idx < girderKeys.size());
         }

         multiple_girder_table(start_idx, end_idx, pBroker, girderKeys, pChapter, pDisplayUnits, pIArtifact);
      }
   }

   if (1 < girderKeys.size())
   {
      rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      (*pChapter) << pPara;
      (*pPara) << rptNewLine <<_T("Design Outcomes for Individual Girders");
   }

   // Loop over all designed girders
   std::vector<CGirderKey>::iterator girderKeyIter(girderKeys.begin());
   std::vector<CGirderKey>::iterator girderKeyIterEnd(girderKeys.end());
   for ( ; girderKeyIter != girderKeyIterEnd; girderKeyIter++ )
   {
      CGirderKey& girderKey = *girderKeyIter;

      const pgsGirderDesignArtifact* pArtifact = pIArtifact->GetDesignArtifact(girderKey);

      if ( pArtifact == nullptr )
      {
         rptParagraph* pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << _T("This girder has not been designed") << rptNewLine;
         return pChapter;
      }

      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const pgsSegmentDesignArtifact* pSegmentDesignArtifact = pArtifact->GetSegmentDesignArtifact(segIdx);
         pgsSegmentDesignArtifact::Outcome outcome = pSegmentDesignArtifact->GetOutcome();
         if ( WasDesignSuccessful(outcome))
         {
            successful_design(pBroker,pChapter,pDisplayUnits,pSegmentDesignArtifact);
         }
         else
         {
            failed_design(pBroker,pChapter,pDisplayUnits,pSegmentDesignArtifact);
         }
      }
   }

   return pChapter;
}

CChapterBuilder* CDesignOutcomeChapterBuilder::Clone() const
{
   return new CDesignOutcomeChapterBuilder;
}

void write_artifact_data(IBroker* pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsSegmentDesignArtifact* pArtifact)
{
   const CSegmentKey& segmentKey = pArtifact->GetSegmentKey();

   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, distance, pDisplayUnits->GetXSectionDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       true );

   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_SPAN, &vPoi);
   ATLASSERT(vPoi.size()==1);
   const pgsPointOfInterest& poiMS = vPoi.front();
   arDesignOptions options = pArtifact->GetDesignOptions();

   // Start report with design notes. Notes will be written at end of this function
   bool design_success = WasDesignSuccessful( pArtifact->GetOutcome() );
   bool conc_strength_controlled_by_shear = design_success && pArtifact->GetDoDesignShear() &&
                                            pArtifact->GetFinalDesignState().GetAction()==pgsSegmentDesignArtifact::ConcreteStrengthDesignState::actShear;
   bool doNotes = (design_success && pArtifact->GetDoDesignFlexure()!=dtNoDesign) || 
                  conc_strength_controlled_by_shear || 
                  pArtifact->DoPreviouslyFailedDesignsExist() ||
                  pArtifact->DoDesignNotesExist();

   rptParagraph* pNotesParagraph = doNotes ? new rptParagraph() : nullptr;
   if ( doNotes )
   {
      rptParagraph* pParagraph = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      *pChapter << pParagraph;
      *pParagraph << _T("Design Notes:");
      *pChapter << pNotesParagraph;
   }

   if (pArtifact->GetDoDesignFlexure() != dtNoDesign)
   {
      GDRCONFIG config = pArtifact->GetSegmentConfiguration();

      GET_IFACE2(pBroker, ISegmentData,pSegmentData);
      const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);

      rptParagraph* pParagraph = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      *pChapter << pParagraph;
      *pParagraph << _T("Flexure Design:");

      pParagraph = new rptParagraph();
      *pChapter << pParagraph;

      // Design strategy
      *pParagraph << color(Blue)<< _T("A ")<<GetDesignTypeName(options.doDesignForFlexure)<< _T(" design strategy was used.") << color(Black);

      // Fill type
      if (options.doStrandFillType == ftDirectFill)
      {
         *pParagraph << color(Blue)<< _T("Strands for design will be filled using direct selection.") << color(Black);
      }
      else
      {
         // we asked design to fill using grid, but this may be a non-standard design - let's check
         StrandIndexType num_permanent = pArtifact->GetNumHarpedStrands() + pArtifact->GetNumStraightStrands();
         GET_IFACE2(pBroker,IStrandGeometry, pStrandGeometry );

         StrandIndexType ns, nh;
         if (pStrandGeometry->ComputeNumPermanentStrands(num_permanent, segmentKey, &ns, &nh))
         {
            if (ns != pArtifact->GetNumStraightStrands() )
            {
               *pParagraph << color(Blue)<<_T(" Strands for design were filled using Number of Straight and Number of Harped strands.") << color(Black);
            }
            else
            {
               *pParagraph << color(Blue)<<_T(" Strands for design were filled using the Permanent fill order defined in the girder library.") << color(Black);
            }
         }
      }

      pParagraph = new rptParagraph();
      *pChapter << pParagraph;

      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(3);
      *pParagraph << pTable;

      pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      RowIndexType row = 0;

      (*pTable)(row,0) << _T("Parameter");
      (*pTable)(row,1) << _T("Proposed Design");
      (*pTable)(row,2) << _T("Current Value");

      row++;

      GET_IFACE2(pBroker,IStrandGeometry, pStrandGeometry );

      // current offsets, measured in absolute
      Float64 abs_offset_end[2], abs_offset_hp[2];
      pStrandGeometry->GetHarpStrandOffsets(segmentKey,pgsTypes::metStart,&abs_offset_end[pgsTypes::metStart],&abs_offset_hp[pgsTypes::metStart]);
      pStrandGeometry->GetHarpStrandOffsets(segmentKey,pgsTypes::metEnd,  &abs_offset_end[pgsTypes::metEnd],  &abs_offset_hp[pgsTypes::metEnd]);

      (*pTable)(row,0) << _T("Number of Straight Strands");
      (*pTable)(row,1) << config.PrestressConfig.GetStrandCount(pgsTypes::Straight);
      (*pTable)(row,2) << pStrands->GetStrandCount(pgsTypes::Straight);

      // print straight debond information if exists
      CollectionIndexType ddb = config.PrestressConfig.Debond[pgsTypes::Straight].size();
      StrandIndexType pdb = pStrandGeometry->GetNumDebondedStrands(segmentKey,pgsTypes::Straight,pgsTypes::dbetEither);
      if (0 < ddb || 0 < pdb)
      {
         (*pTable)(row,1) << _T(" (")<<ddb<<_T(" debonded)");
         (*pTable)(row,2) << _T(" (")<<pdb<<_T(" debonded)");
      }

      row++;

      (*pTable)(row,0) << _T("Number of ") << LABEL_HARP_TYPE(options.doForceHarpedStrandsStraight) << _T(" Strands");
      (*pTable)(row,1) << config.PrestressConfig.GetStrandCount(pgsTypes::Harped);
      (*pTable)(row,2) << pStrands->GetStrandCount(pgsTypes::Harped);
      row++;

      if ( 0 < pStrandGeometry->GetMaxStrands(segmentKey,pgsTypes::Temporary) )
      {
         (*pTable)(row,0) << _T("Number of Temporary Strands");
         (*pTable)(row,1) << config.PrestressConfig.GetStrandCount(pgsTypes::Temporary);
         (*pTable)(row,2) << pStrands->GetStrandCount(pgsTypes::Temporary);
         row++;
      }

      (*pTable)(row,0) << _T("Straight Strand Jacking Force");
      (*pTable)(row,1) << force.SetValue(config.PrestressConfig.Pjack[pgsTypes::Straight]);
      (*pTable)(row,2) << force.SetValue(pStrands->GetPjack(pgsTypes::Straight));
      row++;

      (*pTable)(row,0) << LABEL_HARP_TYPE(options.doForceHarpedStrandsStraight) << _T(" Strand Jacking Force");
      (*pTable)(row,1) << force.SetValue(config.PrestressConfig.Pjack[pgsTypes::Harped]);
      (*pTable)(row,2) << force.SetValue(pStrands->GetPjack(pgsTypes::Harped));
      row++;

      if ( 0 < pStrandGeometry->GetMaxStrands(segmentKey,pgsTypes::Temporary) )
      {
         (*pTable)(row,0) << _T("Temporary Strand Jacking Force");
         (*pTable)(row,1) << force.SetValue(config.PrestressConfig.Pjack[pgsTypes::Temporary]);
         (*pTable)(row,2) << force.SetValue(pStrands->GetPjack(pgsTypes::Temporary));
         row++;
      }

      Float64 HgStart, HgHp1, HgHp2, HgEnd;
      pStrandGeometry->GetHarpedStrandControlHeights(segmentKey,&HgStart,&HgHp1,&HgHp2,&HgEnd);

      if (0 < config.PrestressConfig.GetStrandCount(pgsTypes::Harped) && 
          options.doDesignForFlexure == dtDesignForHarping &&
          !options.doForceHarpedStrandsStraight)
      {
         HarpedStrandOffsetType HsoEnd = pStrands->GetHarpStrandOffsetMeasurementAtEnd();

         GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
         const CSplicedGirderData* pGirder = pIBridgeDesc->GetGirder(segmentKey);
         std::_tstring gdrName = pGirder->GetGirderName();

         bool doAdjust = (0.0 <= pStrandGeometry->GetHarpedEndOffsetIncrement(gdrName.c_str(), pgsTypes::asHarped) ? true : false);
         if (doAdjust)
         {
            for ( int i = 0; i < 2; i++ )
            {
               pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
               std::_tstring strEnd(endType == pgsTypes::metStart ? _T("start") : _T("end"));

               switch( HsoEnd )
               {
               case hsoCGFROMTOP:
                  (*pTable)(row,0) << _T("Distance from top of girder to") << rptNewLine << _T("CG of harped strand group at ") << strEnd << _T(" of girder");
                  break;

               case hsoCGFROMBOTTOM:
                  (*pTable)(row,0) << _T("Distance from bottom of girder to") << rptNewLine << _T("CG of harped strand group at ") << strEnd << _T(" of girder");
                  break;

               case hsoLEGACY:
                  // convert legacy to display TOP 2 TOP

                  HsoEnd = hsoTOP2TOP;

               case hsoTOP2TOP:
                  (*pTable)(row,0) << _T("Distance from top of girder to") << rptNewLine << _T("top of harped strand group at ") << strEnd << _T(" of girder");
                  break;

               case hsoTOP2BOTTOM:
                  (*pTable)(row,0) << _T("Distance from bottom of girder") << rptNewLine << _T("to top of harped strand group at ") << strEnd << _T(" of girder");
                  break;

               case hsoBOTTOM2BOTTOM:
                  (*pTable)(row,0) << _T("Distance from bottom of girder") << rptNewLine << _T("to bottom of harped strand group at ") << strEnd << _T(" of girder");
                  break;

               case hsoECCENTRICITY:
                  (*pTable)(row,0) << _T("Eccentricity of harped strand") << rptNewLine << _T("group at ") << strEnd << _T(" of girder");
                  break;

               default:
                  ATLASSERT(false); // should never get here
               }

               const ConfigStrandFillVector& confvec_design = config.PrestressConfig.GetStrandFill(pgsTypes::Harped);

               Float64 offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(gdrName.c_str(), endType, pgsTypes::asHarped,
                                                                                   HgStart, HgHp1, HgHp2, HgEnd,
                                                                                   confvec_design, 
                                                                                   HsoEnd, 
                                                                                   pArtifact->GetHarpStrandOffsetEnd(endType));
               (*pTable)(row,1) << length.SetValue(offset);

               if (pStrands->GetAdjustableStrandType() == pgsTypes::asHarped)
               {
                  ConfigStrandFillVector confvec_current = pStrandGeometry->ComputeStrandFill(segmentKey, pgsTypes::Harped, pStrands->GetStrandCount(pgsTypes::Harped));

                  offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(segmentKey, endType, confvec_current, HsoEnd, abs_offset_end[endType]);

                  (*pTable)(row,2) << length.SetValue(offset);
               }
               else
               {
                  // old design was straight - offset not applicable
                  (*pTable)(row,2) << _T("N/A");
               }

               row++;
            }
         }

         // At harping points
         doAdjust = pStrandGeometry->GetHarpedHpOffsetIncrement(gdrName.c_str(), pgsTypes::asHarped) >= 0.0;

         if (doAdjust)
         {
            HarpedStrandOffsetType HsoHp = pStrands->GetHarpStrandOffsetMeasurementAtHarpPoint();
            for ( int i = 0; i < 2; i++ )
            {
               pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
               std::_tstring strHP(endType == pgsTypes::metStart ? _T("left") : _T("right"));

               switch( HsoHp )
               {
               case hsoCGFROMTOP:
                  (*pTable)(row,0) << _T("Distance from top of girder to") << rptNewLine << _T("CG of harped strand group at ") << strHP << _T(" harping point");
                  break;

               case hsoCGFROMBOTTOM:
                  (*pTable)(row,0) << _T("Distance from bottom of girder to") << rptNewLine << _T("CG of harped strand group at ") << strHP << _T(" harping point");
                  break;

               case hsoTOP2TOP:
                  (*pTable)(row,0) << _T("Distance from top of girder to") << rptNewLine << _T("top of harped strand group at ") << strHP << _T(" harping point");
                  break;

               case hsoTOP2BOTTOM:
                  (*pTable)(row,0) << _T("Distance from bottom of girder to") << rptNewLine << _T("top of harped strand group at ") << strHP << _T(" harping point");
                  break;

               case hsoLEGACY:
                  // convert legacy to display BOTTOM 2 BOTTOM
                  HsoHp = hsoBOTTOM2BOTTOM;

               case hsoBOTTOM2BOTTOM:
                  (*pTable)(row,0) << _T("Distance from bottom of girder to") << rptNewLine << _T("bottom of harped strand group at ") << strHP << _T(" harping point");
                  break;


               case hsoECCENTRICITY:
                  (*pTable)(row,0) << _T("Eccentricity of harped strand") << rptNewLine << _T("group at ") << strHP << _T(" harping point");
                  break;

               default:
                  ATLASSERT(false); // should never get here
               }

               const ConfigStrandFillVector& confvec_design = config.PrestressConfig.GetStrandFill(pgsTypes::Harped);

               Float64 offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(gdrName.c_str(), endType, pgsTypes::asHarped,
                                                                                   HgStart, HgHp1, HgHp2, HgEnd,
                                                                                   confvec_design, 
                                                                                   HsoHp, 
                                                                                   pArtifact->GetHarpStrandOffsetHp(endType));

               (*pTable)(row,1) << length.SetValue(offset);


               if (pStrands->GetAdjustableStrandType() == pgsTypes::asHarped)
               {
                  ConfigStrandFillVector confvec_current = pStrandGeometry->ComputeStrandFill(segmentKey, pgsTypes::Harped, pStrands->GetStrandCount(pgsTypes::Harped));

                  offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(gdrName.c_str(), endType, pgsTypes::asHarped,
                                                                              HgStart, HgHp1, HgHp2, HgEnd,
                                                                              confvec_current, 
                                                                              HsoHp, abs_offset_hp[endType]);
                  (*pTable)(row,2) << length.SetValue(offset);
               }
               else
               {
                  // old design was straight - offset not applicable
                  (*pTable)(row,2) << _T("N/A");
               }

               row++;
            }
         }
      }

      GET_IFACE2(pBroker,IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      Float64 neff;
      Float64 ecc_design  = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poiMS, false, &config, &neff);
      Float64 ecc_current = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poiMS, false, nullptr, &neff);

      (*pTable)(row,0) << _T("Eccentricity of Permanent Strands at Midspan");
      (*pTable)(row,1) << length.SetValue(ecc_design);
      (*pTable)(row,2) << length.SetValue(ecc_current);
      row++;

      (*pTable)(row,0) << RPT_FCI;
      (*pTable)(row,1) << stress.SetValue(pArtifact->GetReleaseStrength());
      (*pTable)(row,2) << stress.SetValue(pMaterial->Concrete.Fci);
      row++;

      (*pTable)(row,0) << RPT_FC;
      (*pTable)(row,1) << stress.SetValue(pArtifact->GetConcreteStrength());
      (*pTable)(row,2) << stress.SetValue(pMaterial->Concrete.Fc);
      row++;

      GET_IFACE2(pBroker,IBridge,pBridge);
      if ( (pBridge->GetDeckType()!=pgsTypes::sdtNone) && (options.doDesignSlabOffset != sodNoSlabOffsetDesign) )
      {
         GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
         GET_IFACE2(pBroker,ISpecification,pSpec);
         const CGirderGroupData* pGroup = pIBridgeDesc->GetBridgeDescription()->GetGirderGroup(segmentKey.groupIndex);
         const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

         // the computed slab offset will be applied according to the current slab offset mode
         if ( pIBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge )
         {
            // slab offset is for the entire bridge... the start value contains this parameter
            (*pTable)(row,0) << _T("Slab Offset (\"A\" Dimension)");
            (*pTable)(row,1) << length.SetValue( pArtifact->GetSlabOffset(pgsTypes::metStart) );
            (*pTable)(row,2) << length.SetValue( pSegment->GetSlabOffset(pgsTypes::metStart));
            row++;
         }
         else
         {
            (*pTable)(row,0) << _T("Slab Offset at Start (\"A\" Dimension)");
            (*pTable)(row,1) << length.SetValue( pArtifact->GetSlabOffset(pgsTypes::metStart) );
            (*pTable)(row,2) << length.SetValue( pSegment->GetSlabOffset(pgsTypes::metStart));
            row++;

            (*pTable)(row,0) << _T("Slab Offset at End (\"A\" Dimension)");
            (*pTable)(row,1) << length.SetValue( pArtifact->GetSlabOffset(pgsTypes::metEnd) );
            (*pTable)(row,2) << length.SetValue( pSegment->GetSlabOffset(pgsTypes::metEnd) );
            row++;
         }

         if ( options.doDesignSlabOffset==sodSlabOffsetandAssumedExcessCamberDesign && pSpec->IsAssumedExcessCamberForLoad() )
         {
            (*pTable)(row,0) << _T("Assumed Excess Camber");
            (*pTable)(row,1) << length.SetValue( pArtifact->GetAssumedExcessCamber() );
            (*pTable)(row,2) << length.SetValue( pIBridgeDesc->GetAssumedExcessCamber(segmentKey.groupIndex,segmentKey.girderIndex));
            row++;
         }
      }

      if (options.doDesignLifting)
      {
         GET_IFACE2(pBroker,ISegmentLifting,pSegmentLifting);

         (*pTable)(row,0) << _T("Lifting Point Location (Left)");
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetLeftLiftingLocation() );
         (*pTable)(row,2) << distance.SetValue( pSegmentLifting->GetLeftLiftingLoopLocation(segmentKey) );
         row++;

         (*pTable)(row,0) << _T("Lifting Point Location (Right)");
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetRightLiftingLocation() );
         (*pTable)(row,2) << distance.SetValue( pSegmentLifting->GetRightLiftingLoopLocation(segmentKey) );
         row++;
      }

      if (options.doDesignHauling)
      {
         GET_IFACE2(pBroker,ISegmentHauling,pSegmentHauling);

         (*pTable)(row,0) << _T("Truck Support Location (Leading)");
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetLeadingOverhang() );
         (*pTable)(row,2) << distance.SetValue( pSegmentHauling->GetLeadingOverhang(segmentKey) );
         row++;

         (*pTable)(row,0) << _T("Truck Support Location (Trailing)");
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetTrailingOverhang() );
         (*pTable)(row,2) << distance.SetValue( pSegmentHauling->GetTrailingOverhang(segmentKey) );
         row++;

         GET_IFACE2(pBroker,ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
         if ( pSegmentHaulingSpecCriteria->GetHaulingAnalysisMethod() == pgsTypes::hmWSDOT )
         {
            (*pTable)(row,0) << _T("Haul Truck");
            (*pTable)(row,1) << pArtifact->GetHaulTruck();
            (*pTable)(row,2) << pSegmentHauling->GetHaulTruck(segmentKey);
            row++;
         }
      }
   }
   else
   {
      rptParagraph* pParagraph = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      *pChapter << pParagraph;
      *pParagraph << _T("Flexure Design Not Requested")<<rptNewLine;
   }

   // Need design notes in shear output and at end of report
   std::vector<pgsSegmentDesignArtifact::DesignNote> design_notes = pArtifact->GetDesignNotes();

   if (pArtifact->GetDoDesignShear())
   {

      rptParagraph* pParagraph = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      *pChapter << pParagraph;
      *pParagraph << _T("Shear Design:");

      pParagraph = new rptParagraph();
      *pChapter << pParagraph;

      // Check if existing stirrup layout passed. Don't report design if it did
      std::vector<pgsSegmentDesignArtifact::DesignNote>::iterator itd = std::find(design_notes.begin(), design_notes.end(),
                                                                           pgsSegmentDesignArtifact::dnExistingShearDesignPassedSpecCheck);
      bool did_existing_pass = itd != design_notes.end();
      if (did_existing_pass)
      {
         *pParagraph << Italic( _T("The current transverse reinforcement design is adequate. No changes are required. Current values are reported below for convenience.") ) <<rptNewLine<<rptNewLine;
      }

      GET_IFACE2(pBroker, IBridge, pBridge);
      Float64 girder_length = pBridge->GetSegmentLength(segmentKey);

      *pParagraph << Bold(_T("Primary Bars")) << rptNewLine;

      const CShearData2* pShearData = pArtifact->GetShearData();

      if (!did_existing_pass)
      {
         *pParagraph << _T("Proposed Design:") << rptNewLine;

        // stirrup design results
         ZoneIndexType nZones = pArtifact->GetNumberOfStirrupZonesDesigned();

         if (0 < nZones)
         {
            write_primary_shear_data(pParagraph, pDisplayUnits, girder_length, nZones, pShearData);
         }
         else
         {
            *pParagraph << _T("No Zones Designed")<<rptNewLine;
         }

         // Current configuration
         *pParagraph << rptNewLine;
         *pParagraph << _T("Current Values:") << rptNewLine;
      }

      GET_IFACE2(pBroker,IShear,pShear);
      const CShearData2* p_shear_data = pShear->GetSegmentShearData(segmentKey);
      ZoneIndexType nZones = p_shear_data->ShearZones.size();

      write_primary_shear_data(pParagraph, pDisplayUnits, girder_length, nZones, p_shear_data);

      // Horiz interface bars
      *pParagraph <<rptNewLine<< Bold(_T("Additional Bars For Horizontal Interface Shear"))<<rptNewLine;
      if (!did_existing_pass)
      {
         *pParagraph << _T("Proposed Design:") << rptNewLine;
         write_horiz_shear_data(pParagraph, pDisplayUnits, girder_length, pShearData);

         *pParagraph << rptNewLine;
         *pParagraph << _T("Current Values:") << rptNewLine;
      }
      write_horiz_shear_data(pParagraph, pDisplayUnits, girder_length, p_shear_data);

      // Additional Shear Reinforcement at Girder Ends
      *pParagraph <<rptNewLine<< Bold(_T("Additional Shear Reinforcement at Girder Ends"))<<rptNewLine;
      if (!did_existing_pass)
      {
         *pParagraph << _T("Proposed Design:") << rptNewLine;
         write_additional_shear_data(pParagraph, pDisplayUnits, girder_length, pShearData);

         *pParagraph << rptNewLine;
         *pParagraph << _T("Current Values:") << rptNewLine;
      }

      write_additional_shear_data(pParagraph, pDisplayUnits, girder_length, p_shear_data);

      if (!did_existing_pass && pArtifact->GetWasLongitudinalRebarForShearDesigned())
      {
         // Always the last row
         const CLongitudinalRebarData& rlrebardata = pArtifact->GetLongitudinalRebarData();
         const CLongitudinalRebarData::RebarRow rrow = rlrebardata.RebarRows.back();

         *pParagraph <<rptNewLine<< Bold(_T("Additional Logitudinal Rebar Added to Girder Bottom For Longitudinal Reinforcement for Shear:"))<<rptNewLine;
         *pParagraph <<_T("- ")<< rrow.NumberOfBars <<_T(" bars of ")
                  <<lrfdRebarPool::GetBarSize(rrow.BarSize).c_str()<<_T(" at ")
                  <<length.SetValue(rrow.BarSpacing)<<rptNewLine;
      }
   }

   // End up with some notes about Design
   if ( doNotes )
   {
      if (pArtifact->DoPreviouslyFailedDesignsExist())
      {
         write_design_failures(pNotesParagraph, pArtifact);
      }

      // Explicit notes created during design
      if (pArtifact->DoDesignNotesExist())
      {
         write_design_notes(pNotesParagraph, design_notes);
      }

      if (pArtifact->GetDoDesignFlexure()!=dtNoDesign && design_success)
      {
         // Notes from a successful flexural design
         GET_IFACE2(pBroker,IMaterials,pMaterial);
         GET_IFACE2(pBroker,ILimits,pLimits);
         pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
         Float64 max_girder_fci = pLimits->GetMaxSegmentFci(concType);
         Float64 max_girder_fc = pLimits->GetMaxSegmentFc(concType);
         if (max_girder_fci < pArtifact->GetReleaseStrength())
         {
            *pNotesParagraph <<color(Red)<< _T("Warning: The designed girder release strength exceeds the normal value of ")<<stress.SetValue(max_girder_fci)<<color(Black)<< rptNewLine;
         }

         if (max_girder_fc < pArtifact->GetConcreteStrength())
         {
            *pNotesParagraph <<color(Red)<< _T("Warning: The designed girder final concrete strength exceeds the normal value of ")<<stress.SetValue(max_girder_fc)<<color(Black)<< rptNewLine;
         }

         // Negative camber is not technically a spec check, but a warning
         GDRCONFIG config = pArtifact->GetSegmentConfiguration();

         GET_IFACE2(pBroker,ICamber,pCamber);
         Float64 excess_camber = pCamber->GetExcessCamber(poiMS,CREEP_MAXTIME,&config);
         if ( excess_camber < 0 )
         {
            *pNotesParagraph<<color(Red)<< _T("Warning:  Excess camber is negative, indicating a potential sag in the beam.")<<color(Black)<< rptNewLine;
         }

         *pNotesParagraph << rptNewLine;
         *pNotesParagraph << _T("Concrete release strength was controlled by ") << pArtifact->GetReleaseDesignState().AsString() << rptNewLine;
         *pNotesParagraph << _T("Concrete final strength was controlled by ") << pArtifact->GetFinalDesignState().AsString() << rptNewLine;
         *pNotesParagraph << rptNewLine;
      }
   }
}

void successful_design(IBroker* pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsSegmentDesignArtifact* pArtifact)
{
   const CSegmentKey& segmentKey = pArtifact->GetSegmentKey();
   ATLASSERT(segmentKey.segmentIndex == 0); // design is only for precast girders which only have one segment

   rptParagraph* pParagraph = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pParagraph;

   pgsSegmentDesignArtifact::Outcome outcome = pArtifact->GetOutcome();

   if ( outcome == pgsSegmentDesignArtifact::Success)
   {
      *pParagraph << rptNewLine << color(Green)
                  << _T("The design for Span ") << LABEL_GROUP(segmentKey.groupIndex)
                  << _T(" Girder ") << LABEL_GIRDER(segmentKey.girderIndex)
                  << _T(" was successful.") 
                  << color(Black)
                  << rptNewLine;
   }
   else if (outcome == pgsSegmentDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionCy ||
            outcome == pgsSegmentDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionLifting ||
            outcome == pgsSegmentDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionHauling)
   {
      *pParagraph << rptNewLine << color(OrangeRed)
                  << _T("The design for Span ") << LABEL_GROUP(segmentKey.groupIndex)
                  << _T(" Girder ") << LABEL_GIRDER(segmentKey.girderIndex)
                  << _T(" failed.")
                  << color(Black);

      rptParagraph* pParagraph = new rptParagraph( );
      *pChapter << pParagraph;
      *pParagraph << color(OrangeRed) 
                  << _T("You may be able to create a successful design by adding longitudinal reinforcement to increase temporary tension stress limits");

      if (outcome == pgsSegmentDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionCy)
      {
         *pParagraph << _T(" for release in the Casting Yard.");
      }
      else if(outcome == pgsSegmentDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionLifting)
      {
         *pParagraph << _T(" for girder Lifting.");
      }
      else if(outcome == pgsSegmentDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionHauling)
      {
         *pParagraph << _T(" for girder Hauling.");
      }

      *pParagraph << color(Black);
   }
   else
   {
      ATLASSERT(false);
   }


   write_artifact_data(pBroker,pChapter,pDisplayUnits,pArtifact);
}

void failed_design(IBroker* pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsSegmentDesignArtifact* pArtifact)
{
   const CSegmentKey& segmentKey = pArtifact->GetSegmentKey();

   rptParagraph* pParagraph;
   pParagraph = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pParagraph;

   *pParagraph << rptNewLine << color(Red)
               << _T("The design attempt for Span ") << LABEL_GROUP(segmentKey.groupIndex)
               << _T(" Girder ") << LABEL_GIRDER(segmentKey.girderIndex)
               << _T(" failed.") 
               << color(Black)
               << rptNewLine;

   bool bContinue = true;
   pParagraph = new rptParagraph( rptStyleManager::GetSubheadingStyle() );
   *pChapter << pParagraph;
   switch( pArtifact->GetOutcome() )
   {
      case pgsSegmentDesignArtifact::Success:
         ATLASSERT(false); // Should never get here
         break;

      case pgsSegmentDesignArtifact::NoDesignRequested:
         *pParagraph << _T("No Design was requested.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::TooManyStrandsReqd:
         *pParagraph << _T("Too many strands are required to satisfy the stress criteria.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::OverReinforced:
         *pParagraph << _T("The section is over reinforced and the number of strands cannot be reduced.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::UnderReinforced:
         *pParagraph << _T("The section is under reinforced and the number of strands cannot be increased.") << rptNewLine;
         *pParagraph << _T("Increasing the compressive strength of the deck will improve section capacity.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::UltimateMomentCapacity:
         *pParagraph << _T("Too many strands are required to satisfy ultimate moment capacity criteria.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::MaxIterExceeded:
         *pParagraph << _T("After several iterations, a successful design could not be found.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::ReleaseStrength:
         *pParagraph << _T("An acceptable concrete release strength could not be found.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::ExceededMaxHoldDownForce:
         *pParagraph << _T("Design is such that maximum allowable hold down force in casting yard is exceeded.")<<rptNewLine;
         break;

      case pgsSegmentDesignArtifact::StrandSlopeOutOfRange:
         *pParagraph << _T("Design is such that maximum strand slope is exceeded.")<<rptNewLine;
         break;

      case pgsSegmentDesignArtifact::ShearExceedsMaxConcreteStrength:
         *pParagraph << _T("Section is too small to carry ultimate shear. Crushing capacity was exceeded") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::TooManyStirrupsReqd:
         *pParagraph << _T("Could not design stirrups - Minimum spacing requirements were violated.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::TooManyStirrupsReqdForHorizontalInterfaceShear:
         *pParagraph << _T("Could not design stirrups - Cannot add enough stirrups to resist horizontal interface shear requirements.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::TooManyStirrupsReqdForSplitting:
         *pParagraph << _T("Could not design stirrups for splitting demand - Minimum spacing requirements were violated") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::ConflictWithLongReinforcementShearSpec:
         *pParagraph << _T("Failed designing for longitudinal reinforcement for shear due to conflicting library information. Project criteria Shear Design tab says to use longitudinal rebar, while Shear Capacity tab disables use of mild steel rebar.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::StrandsReqdForLongReinfShearAndFlexureTurnedOff:
         *pParagraph << _T("Additional strands are required to meet longitudinal reinforcement for shear requirements. However, this can only be performed if flexural design is enabled.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::NoDevelopmentLengthForLongReinfShear:
         *pParagraph << _T("Additional longitudinal mild steel reinforcement bars are required to meet longitudinal reinforcement for shear requirements. However, the face of support is at the end of the girder, so there is no room for rebar development. Consider changing the connection to allow for development.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::NoStrandDevelopmentLengthForLongReinfShear:
         *pParagraph << _T("Additional strands are required to meet longitudinal reinforcement for shear requirements. However, the face of support is at the end of the girder, so there is no room for prestress development. Consider changing the connection to allow for development.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::TooManyBarsForLongReinfShear:
         *pParagraph << _T("Could not add enough mild steel reinforcement to meet longitudinal reinforcement for shear requirements while maintaining minimum spacing requirements per LRFD 5.10.3.1.2.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::TooMuchStrandsForLongReinfShear:
         *pParagraph << _T("Could not add enough strands to meet longitudinal reinforcement for shear requirements.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::GirderLiftingStability:
         *pParagraph << _T("Could not satisfy stability requirements for lifting") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::GirderLiftingConcreteStrength:
         *pParagraph << _T("Could not find a concrete strength to satisfy stress limits for lifting") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::GirderShippingStability:
         *pParagraph << _T("Could not satisfy stability requirements for shipping") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::GirderShippingConfiguration:
         *pParagraph << _T("Could not satisfy trucking configuration requirements for shipping") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::GirderShippingConcreteStrength:
         *pParagraph << _T("Could not find a concrete strength to satisfy stress limits for shipping") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::StressExceedsConcreteStrength:
         *pParagraph << _T("Could not find a concrete strength to satisfy stress limits") << rptNewLine;
         break;
      
      case pgsSegmentDesignArtifact::DebondDesignFailed:
         *pParagraph << _T("Unable to find an adequate debond design") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::LldfRangeOfApplicabilityError:
         *pParagraph << _T("The allowable range of applicability for computing live load distribution factors was exceeded during design. See the Status Center for more details.") << rptNewLine;
         break;

      case pgsSegmentDesignArtifact::DesignNotSupported_Losses:
         *pParagraph << _T("Design is not supported for time step loss analysis") << rptNewLine;
         bContinue = false;
         break;

      case pgsSegmentDesignArtifact::DesignNotSupported_Strands:
         *pParagraph << _T("Design is not supported for the current strand definition type. Use Number of Straight\\Harped Strands or Number of Permanent Strands") << rptNewLine;
         bContinue = false;
         break;

      default:
         ATLASSERT(false); // Should never get here
   }

   if (bContinue)
   {
      pParagraph = new rptParagraph();
      *pParagraph << Bold(_T("Results from last trial:")) << rptNewLine;
      *pChapter << pParagraph;
      write_artifact_data(pBroker, pChapter, pDisplayUnits, pArtifact);
   }
}

std::_tstring GetDesignNoteString(pgsSegmentDesignArtifact::DesignNote note)
{
   switch (note)
   {
   case pgsSegmentDesignArtifact::dnShearRequiresStrutAndTie:
      return std::_tstring(_T("WARNING: A strut and tie analysis is required in the girder end zones per LRFD ") + std::_tstring(LrfdCw8th(_T("5.7.3.2"),_T("5.8.3.2"))) + _T(". This design will fail a spec check."));
      break;

   case pgsSegmentDesignArtifact::dnExistingShearDesignPassedSpecCheck:
      return std::_tstring(_T("The existing stirrup input data passed the shear specification check. No design modificatons were made."));
      break;

   case pgsSegmentDesignArtifact::dnStrandsAddedForLongReinfShear:
      return std::_tstring(_T("The number of strands was controlled by longitudinal reinforcement for shear requirements."));
      break;

   case pgsSegmentDesignArtifact::dnStirrupsTightendedForLongReinfShear:
      return std::_tstring(_T("The stirrup layout spacing was tightened for longitudinal reinforcement for shear requirements."));
      break;

   case pgsSegmentDesignArtifact::dnLongitudinalBarsNeeded4FlexuralTensionCy:
      return std::_tstring(_T("Refer to the release \"Allowable Tension Reinforcement Requirements\" sections in the Details report for more information about required longitudinal reinforcement."));
      break;

   case pgsSegmentDesignArtifact::dnLongitudinalBarsNeeded4FlexuralTensionLifting:
      return std::_tstring(_T("Refer to the lifting \"Allowable Tension Reinforcement Requirements\" sections in the Details report for more information about required longitudinal reinforcement."));
      break;

   case pgsSegmentDesignArtifact::dnLongitudinalBarsNeeded4FlexuralTensionHauling:
      return std::_tstring(_T("Refer to the hauling \"Allowable Tension Reinforcement Requirements\" sections in the Details report for more information about required longitudinal reinforcement."));
      break;

   case pgsSegmentDesignArtifact::dnConcreteStrengthIncreasedForShearStress:
      return std::_tstring(_T("Final concrete strength was increased to alleviate shear stress requirements."));
      break;

   case pgsSegmentDesignArtifact::dnTransformedSectionsSetToGross:
      return std::_tstring(_T("The Project Criteria specifies an analysis based on Transformed Sections. However, this design was based on gross section properties. Design results may not be accurate - run a Spec Check report to insure correctness."));
      break;

   case pgsSegmentDesignArtifact::dnParabolicHaunchSetToConstant:
      return std::_tstring(_T("The Project Criteria specifies an analysis based on non-prismatic composite section properties accounting for Parabolic Haunch depth. This girder was designed using a constant depth haunch. Design results may not be accurate - run a Spec Check report to insure correctness."));
      break;

   default:
      ATLASSERT(false);
   }
   return std::_tstring();
}

void write_design_notes(rptParagraph* pParagraph, const std::vector<pgsSegmentDesignArtifact::DesignNote>& notes)
{
   for(std::vector<pgsSegmentDesignArtifact::DesignNote>::const_iterator it = notes.begin(); it!=notes.end(); it++)
   {
      if (*it != pgsSegmentDesignArtifact::dnExistingShearDesignPassedSpecCheck) // this is written elsewhere
      {
         *pParagraph <<_T(" -  ") << GetDesignNoteString( *it ) <<rptNewLine;
      }
   }
}

void write_design_failures(rptParagraph* pParagraph, const pgsSegmentDesignArtifact* pArtifact)
{
   std::vector<arFlexuralDesignType> failures = pArtifact->GetPreviouslyFailedFlexuralDesigns();
   for(std::vector<arFlexuralDesignType>::iterator itf=failures.begin(); itf!=failures.end(); itf++)
   {
      if ( *itf != dtNoDesign )
      {
         *pParagraph <<color(Blue)<<_T("Design attempt for a: ")<< GetDesignTypeName( *itf )<<_T(" design strategy - Failed") <<color(Black)<<rptNewLine;
      }
   }
}

void multiple_girder_table(ColumnIndexType startIdx, ColumnIndexType endIdx,
                     IBroker* pBroker,const std::vector<CGirderKey>& girderKeys,rptChapter* pChapter,
                     IEAFDisplayUnits* pDisplayUnits,IArtifact* pIArtifact)
{
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, distance, pDisplayUnits->GetXSectionDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       true );

   GET_IFACE2(pBroker,IStrandGeometry, pStrandGeometry );
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   // Since girder types, design info, etc can be different for each girder, process information for all
   // artifacts to get control data
   const pgsGirderDesignArtifact* pArtifacts[MAX_TBL_COLS]; // Might as well cache artifacts while processing
   bool did_flexure;
   bool did_shear;
   bool did_lifting;
   bool did_hauling;
   bool did_slaboffset;
   bool did_AssumedExcessCamber;
   bool is_harped;
   bool is_temporary;

   process_artifacts(pBroker,startIdx, endIdx, girderKeys, pIArtifact,
                     pArtifacts, did_flexure, did_shear, did_lifting, did_hauling, did_slaboffset, did_AssumedExcessCamber, is_harped, is_temporary);

   if (!did_flexure && !did_shear)
   {
      ATLASSERT(false); // probably shouldn't be here if no design was done
      return;
   }

   rptParagraph* pParagraph = new rptParagraph();
   *pChapter << pParagraph;

   // Our table has a column for each girder
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(endIdx-startIdx+2,_T("Multiple Girder Design Summary"));
   pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
   *pParagraph << pTable << rptNewLine;

   // Start by writing first column
   pTable->SetColumnWidth(0,2.25);
   RowIndexType row = 0;
   (*pTable)(row++,0) << _T("Parameter");

   if (did_flexure)
   {
      (*pTable)(row++,0) << _T("Flexural Design Outcome");
      (*pTable)(row++,0) << _T("Number of Straight Strands");

      if (is_harped)
      {
         (*pTable)(row++,0) << _T("Number of Adjustable Strands");
         (*pTable)(row++,0) << _T("Adjustable Strand Type");
      }

      if (is_temporary)
      {
         (*pTable)(row++, 0) << _T("Number of Temporary Strands");
      }

      (*pTable)(row++,0) << _T("Straight Strand Jacking Force");

      if (is_harped)
      {
         (*pTable)(row++, 0) << _T("Harped Strand Jacking Force");
      }

      if (is_temporary)
      {
         (*pTable)(row++, 0) << _T("Temporary Strand Jacking Force");
      }

      (*pTable)(row++,0) << _T("Eccentricity of Permanent Strands at Midspan");

      if (is_harped)
      {
         // We will have to convert any other measurements to this:
         (*pTable)(row++,0) << _T("Distance from Bottom of girder to") << rptNewLine << _T("top of adjustable strand group at ends of girder");
         (*pTable)(row++,0) << _T("Distance from bottom of girder to") << rptNewLine << _T("bottom of adjustable strand group at harping point");
      }

      (*pTable)(row++,0) << RPT_FCI;
      (*pTable)(row++,0) << RPT_FC;
   }

   if (did_lifting)
   {
      (*pTable)(row++,0) << _T("Lifting Point Location (Left)");
      (*pTable)(row++,0) << _T("Lifting Point Location (Right)");
   }

   if (did_hauling)
   {
      (*pTable)(row++,0) << _T("Truck Support Location (Leading)");
      (*pTable)(row++,0) << _T("Truck Support Location (Trailing)");
   }

   if (did_slaboffset)
   {
      (*pTable)(row++,0) << _T("Slab Offset (A Dimension)");
   }

   if (did_AssumedExcessCamber)
   {
      (*pTable)(row++,0) << _T("Assumed Excess Camber");
   }

   // Titles are now printed. Print results information
   ColumnIndexType idx = 0;
   ColumnIndexType col = 1;
   for (ColumnIndexType gdr_idx=startIdx; gdr_idx<=endIdx; gdr_idx++)
   {
      pTable->SetColumnWidth(col,0.75);

#pragma Reminder("UPDATE: assuming precast girder bridge") // assumes only one segment design artifact
      SegmentIndexType segIdx = 0;

      const CGirderKey& girderKey = girderKeys[gdr_idx];
      
      PoiList vPoi;
      pPoi->GetPointsOfInterest(CSegmentKey(girderKey, segIdx), POI_5L | POI_SPAN, &vPoi);
      ATLASSERT(vPoi.size()==1);
      const pgsPointOfInterest& poiMS = vPoi.front();

      row = 0;

      (*pTable)(row++,col) << _T("Span ") << LABEL_GROUP(girderKey.groupIndex) <<rptNewLine<<_T("Girder ")<<LABEL_GIRDER(girderKey.girderIndex);

      const pgsGirderDesignArtifact* pGirderDesignArtifact = pArtifacts[idx++];
      const pgsSegmentDesignArtifact* pArtifact = pGirderDesignArtifact->GetSegmentDesignArtifact(segIdx);
      const CSegmentKey& segmentKey(pArtifact->GetSegmentKey());

      pgsSegmentDesignArtifact::Outcome outcome = pArtifact->GetOutcome();
      if (outcome==pgsSegmentDesignArtifact::Success)
      {
         (*pTable)(row++,col) <<color(Green)<<_T("Success")<<color(Black);
      }
      else
      {
         (*pTable)(row++,col) <<color(Red)<<_T("Failed")<<color(Black);
      }

      if (did_flexure)
      {
         GDRCONFIG config = pArtifact->GetSegmentConfiguration();

         (*pTable)(row,col) << pArtifact->GetNumStraightStrands();
         // Debond into
         StrandIndexType ddb = config.PrestressConfig.Debond[pgsTypes::Straight].size();
         if(0 < ddb)
         {
            (*pTable)(row,col) << _T("(") << ddb << _T(")");
         }
         row++;

         pgsTypes::AdjustableStrandType adjType = pArtifact->GetAdjustableStrandType();
         if (is_harped)
         {
            (*pTable)(row++,col) << pArtifact->GetNumHarpedStrands();
            (*pTable)(row++,col) << ABR_LABEL_HARP_TYPE(adjType==pgsTypes::asStraight);
         }

         if (is_temporary)
         {
            (*pTable)(row++,col) << pArtifact->GetNumTempStrands();
         }

         // jacking forces
         (*pTable)(row++,col) << force.SetValue(config.PrestressConfig.Pjack[pgsTypes::Straight]);

         if (is_harped)
         {
            (*pTable)(row++,col) << force.SetValue(config.PrestressConfig.Pjack[pgsTypes::Harped]);
         }

         if (is_temporary)
         {
            (*pTable)(row++,col) << force.SetValue(config.PrestressConfig.Pjack[pgsTypes::Temporary]);
         }

         GET_IFACE2(pBroker,IIntervals,pIntervals);
         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
         Float64 neff;
         Float64 ecc_design  = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poiMS, false, &config, &neff);
         (*pTable)(row++,col) << length.SetValue(ecc_design);

         if (is_harped)
         {
            if (pArtifact->GetNumHarpedStrands()>0)
            {
               const CSplicedGirderData* pGirder = pIBridgeDesc->GetGirder(segmentKey);
               std::_tstring gdrName = pGirder->GetGirderName();

               const ConfigStrandFillVector& confvec_design = config.PrestressConfig.GetStrandFill(pgsTypes::Harped);

               Float64 offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(segmentKey, pgsTypes::metStart,
                                                                                   confvec_design, 
                                                                                   hsoTOP2BOTTOM, 
                                                                                   pArtifact->GetHarpStrandOffsetEnd(pgsTypes::metStart));
               (*pTable)(row++,col) << length.SetValue(offset);

               offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(segmentKey,pgsTypes::metStart,
                                                                           confvec_design, 
                                                                           hsoBOTTOM2BOTTOM, 
                                                                           pArtifact->GetHarpStrandOffsetHp(pgsTypes::metStart));
               (*pTable)(row++,col) << length.SetValue(offset);
            }
            else
            {
               (*pTable)(row++,col) << _T("-");
               (*pTable)(row++,col) << _T("-");
            }
         }

         (*pTable)(row++,col) << stress.SetValue(pArtifact->GetReleaseStrength());
         (*pTable)(row++,col) << stress.SetValue(pArtifact->GetConcreteStrength());
      }

      if (did_lifting)
      {
         (*pTable)(row++,col) << distance.SetValue( pArtifact->GetLeftLiftingLocation() );
         (*pTable)(row++,col) << distance.SetValue( pArtifact->GetRightLiftingLocation() );
      }

      if (did_hauling)
      {
         (*pTable)(row++,col) << distance.SetValue( pArtifact->GetLeadingOverhang() );
         (*pTable)(row++,col) << distance.SetValue( pArtifact->GetTrailingOverhang() );
      }

      if (did_slaboffset)
      {
#if defined _DEBUG
         ATLASSERT( pArtifact->GetSlabOffset(pgsTypes::metStart) ==  pArtifact->GetSlabOffset(pgsTypes::metEnd)); // designer learned a new trick?
#endif
         (*pTable)(row++,col) << length.SetValue( pArtifact->GetSlabOffset(pgsTypes::metStart) );
      }

      if (did_AssumedExcessCamber)
      {
         (*pTable)(row++,col) << length.SetValue( pArtifact->GetAssumedExcessCamber() );
      }

      col++;
   }
}

void process_artifacts(IBroker* pBroker,ColumnIndexType startIdx, ColumnIndexType endIdx, const std::vector<CGirderKey>& girderKeys, IArtifact* pIArtifact,
                       const pgsGirderDesignArtifact** pArtifacts, bool& didFlexure, bool& didShear, bool& didLifting, bool& didHauling, 
                       bool& didSlabOffset, bool& didAssumedExcessCamber, bool& isHarped, bool& isTemporary)
{
   // Set all outcomes to false
   didFlexure = false;
   didShear = false;
   didLifting = false;
   didHauling = false;
   didSlabOffset = false;
   didAssumedExcessCamber = false;
   isHarped = false;
   isTemporary = false;

   ColumnIndexType na = endIdx - startIdx + 1;
   ColumnIndexType idx = startIdx;
   for (ColumnIndexType ia=0; ia<na; ia++)
   {
      const CGirderKey& girderKey = girderKeys[idx];

      pArtifacts[ia] = pIArtifact->GetDesignArtifact(girderKey);

#pragma Reminder("UPDATE: assuming precast girder bridge")
      SegmentIndexType segIdx = 0; // the one and only segment is segIdx = 0
      const pgsSegmentDesignArtifact* pSegmentDesignArtifact = pArtifacts[ia]->GetSegmentDesignArtifact(segIdx);
      arDesignOptions options = pSegmentDesignArtifact->GetDesignOptions();

      if (options.doDesignForFlexure != dtNoDesign)
      {
         didFlexure = true;
      }

      if (options.doDesignForShear)
      {
         didShear = true;
      }

      if (options.doDesignLifting)
      {
         didLifting = true;
      }

      if (options.doDesignHauling)
      {
         didHauling = true;
      }

      if (options.doDesignSlabOffset != sodNoSlabOffsetDesign)
      {
         didSlabOffset = true;

         GET_IFACE2(pBroker,ISpecification,pSpec);

         if (options.doDesignSlabOffset == sodSlabOffsetandAssumedExcessCamberDesign && pSpec->IsAssumedExcessCamberInputEnabled())
         {
            didAssumedExcessCamber = true;
         }
      }

      // report harped information if we have any harped designs or, if we have harped strands
      if (options.doDesignForFlexure == dtDesignForHarping || 0 < pSegmentDesignArtifact->GetNumHarpedStrands() )
      {
         isHarped = true;
      }

      if (options.doDesignForFlexure != dtNoDesign && 0 < pSegmentDesignArtifact->GetNumTempStrands() )
      {
         isTemporary = true;
      }

      idx++;
   }
}

void write_primary_shear_data(rptParagraph* pParagraph, IEAFDisplayUnits* pDisplayUnits, Float64 girderLength, ZoneIndexType nz, const CShearData2* pShearData)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
   scalar.SetTolerance(1.0e-6);

   lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
   ATLASSERT(pool!=0);

   bool is_symm = pShearData->bAreZonesSymmetrical;

   // Primary bars
   bool is_stirrups(false);
   if (1 < nz)
   {
      is_stirrups = true;
   }
   else if (1 == nz)
   {
      if (pShearData->ShearZones[0].VertBarSize != matRebar::bsNone)
      {
         is_stirrups = true;
      }
   }

   ColumnIndexType col = 0;
   if (is_stirrups)
   {
      rptRcTable* pTables = rptStyleManager::CreateTableNoHeading(8);
      *pParagraph << pTables << rptNewLine;

      (*pTables)(0,col++) << _T("Zone #");
      (*pTables)(0,col++) << COLHDR(_T("Zone End"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTables)(0,col++) << COLHDR(_T("Zone Length"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTables)(0,col++) << _T("Bar Size");
      (*pTables)(0,col++) << COLHDR(_T("Spacing"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTables)(0,col++) << _T("# Legs")<<rptNewLine<<_T("Vertical");
      (*pTables)(0,col++) << _T("# Legs")<<rptNewLine<<_T("Into Deck");
      (*pTables)(0,col++) << _T("Confinement")<<rptNewLine<<_T("Bar Size");

      Float64 max_zoneloc = is_symm ? girderLength/2.0 : girderLength;

      Float64 zone_end = 0.0;
      RowIndexType i=0;
      bool bdone(false);
      while (!bdone)
      {
         col = 0;
         RowIndexType row = i+1;
         const CShearZoneData2& rszdata = pShearData->ShearZones[i];
         zone_end += rszdata.ZoneLength;
         (*pTables)(row,col++) << rszdata.ZoneNum;

         if ( nz <= i+1 || max_zoneloc <= zone_end)
         {
            bdone = true;
         }

         if (!bdone)
         {
            (*pTables)(row,col++) << location.SetValue(zone_end);
            (*pTables)(row,col++) << location.SetValue(rszdata.ZoneLength);
         }
         else
         {
            if (is_symm)
            {
               (*pTables)(row,col++) << _T("Mid-Girder");
            }
            else
            {
               (*pTables)(row,col++) << _T("End of Girder");
            }

            (*pTables)(row,col++) << _T("");
         }

         if (rszdata.VertBarSize!=matRebar::bsNone)
         {
            (*pTables)(row,col++) << lrfdRebarPool::GetBarSize(rszdata.VertBarSize).c_str();
            (*pTables)(row,col++) << length.SetValue(rszdata.BarSpacing);
            (*pTables)(row,col++) << scalar.SetValue(rszdata.nVertBars);
            (*pTables)(row,col++) << scalar.SetValue(rszdata.nHorzInterfaceBars);
            (*pTables)(row,col++) << lrfdRebarPool::GetBarSize(rszdata.ConfinementBarSize).c_str();
         }
         else
         {
            (*pTables)(row,col++) << _T("none");
            (*pTables)(row,col++) << _T("--");
            (*pTables)(row,col++) << _T("--");
            (*pTables)(row,col++) << _T("--");
            (*pTables)(row,col++) << _T("--");
         }

         i++;
      }
   }
   else
   {
      *pParagraph << _T("- No primary shear zones in girder")<<rptNewLine;
   }
}

void write_horiz_shear_data(rptParagraph* pParagraph, IEAFDisplayUnits* pDisplayUnits, Float64 girderLength,const CShearData2* pShearData)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), true );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
   scalar.SetTolerance(1.0e-6);

   bool is_symm = pShearData->bAreZonesSymmetrical;

   // Additional horizontal interface bars
   ZoneIndexType nhz = pShearData->HorizontalInterfaceZones.size();
   bool is_hstirrups(false);
   if (1 < nhz)
   {
      is_hstirrups = true;
   }
   else if (1 == nhz)
   {
      if (pShearData->HorizontalInterfaceZones[0].BarSize != matRebar::bsNone)
      {
         is_hstirrups = true;
      }
   }

   if(is_hstirrups)
   {
      rptRcTable* pTables = rptStyleManager::CreateTableNoHeading(6);
      *pParagraph << pTables << rptNewLine;

      ColumnIndexType col = 0;
      (*pTables)(0,col++) << _T("Zone #");
      (*pTables)(0,col++) << COLHDR(_T("Zone End"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTables)(0,col++) << COLHDR(_T("Zone Length"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTables)(0,col++) << _T("Bar Size");
      (*pTables)(0,col++) << COLHDR(_T("Spacing"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTables)(0,col++) << _T("# Legs");

      Float64 max_zoneloc = is_symm ? girderLength/2.0 : girderLength;

      Float64 zone_end = 0.0;
      RowIndexType i=0;
      bool bdone(false);
      while (!bdone)
      {
         col = 0;
         RowIndexType row = i+1;
         const CHorizontalInterfaceZoneData& rhzdata = pShearData->HorizontalInterfaceZones[i];
         zone_end += rhzdata.ZoneLength;
         (*pTables)(row,col++) << rhzdata.ZoneNum;

         if ( nhz <= i+1 || max_zoneloc <= zone_end )
         {
            bdone = true;
         }

         if (!bdone)
         {
            (*pTables)(row,col++) << location.SetValue(zone_end);
            (*pTables)(row,col++) << location.SetValue(rhzdata.ZoneLength);
         }
         else
         {
            if (is_symm)
            {
               (*pTables)(row,col++) << _T("Mid-Girder");
            }
            else
            {
               (*pTables)(row,col++) << _T("End of Girder");
            }

            (*pTables)(row,col++) << _T("");
         }

         if (rhzdata.BarSize!=matRebar::bsNone)
         {
            (*pTables)(row,col++) << lrfdRebarPool::GetBarSize(rhzdata.BarSize).c_str();
            (*pTables)(row,col++) << length.SetValue(rhzdata.BarSpacing);
            (*pTables)(row,col++) << scalar.SetValue(rhzdata.nBars);
         }
         else
         {
            (*pTables)(row,col++) << _T("none");
            (*pTables)(row,col++) << _T("--");
            (*pTables)(row,col++) << _T("--");
         }

         i++;
      }
   }
   else
   {
      *pParagraph << _T("- No additional horizontal interface shear zones")<<rptNewLine;
   }
}

void write_additional_shear_data(rptParagraph* pParagraph, IEAFDisplayUnits* pDisplayUnits, Float64 girderLength, const CShearData2* pShearData)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), true );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
   scalar.SetTolerance(1.0e-6);

   if (pShearData->SplittingBarSize != matRebar::bsNone)
   {
      *pParagraph <<_T("- Splitting Reinforcement: Zone Length =")<< location.SetValue(pShearData->SplittingZoneLength)
                  <<_T(";  ")<<scalar.SetValue(pShearData->nSplittingBars)<<_T(" legs of ")
                  <<lrfdRebarPool::GetBarSize(pShearData->SplittingBarSize).c_str()<<_T(" bars at ")
                  <<length.SetValue(pShearData->SplittingBarSpacing)<<rptNewLine;
   }
   else
   {
      *pParagraph <<_T("- Splitting Reinforcement: None")<<rptNewLine;
   }

   if (pShearData->ConfinementBarSize != matRebar::bsNone)
   {
      *pParagraph <<_T("- Confinement Reinforcement: Zone Length =")<< location.SetValue(pShearData->ConfinementZoneLength)<<_T("; ")
                  <<lrfdRebarPool::GetBarSize(pShearData->ConfinementBarSize).c_str()<<_T(" bars at ")
                  <<length.SetValue(pShearData->ConfinementBarSpacing)<<rptNewLine;
   }
   else
   {
      *pParagraph <<_T("- Confinement Reinforcement: None")<<rptNewLine;
   }
}