///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include <Lrfd\RebarPool.h>

#include <PgsExt\DesignArtifact.h>

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDesignOutcomeChapterBuilder
****************************************************************************/

void write_artifact_data(IBroker* pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsDesignArtifact* pArtifact);
void failed_design(IBroker* pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsDesignArtifact* pArtifact);
void successful_design(IBroker* pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsDesignArtifact* pArtifact);
void multiple_girder_table(ColumnIndexType startIdx, ColumnIndexType endIdx,IBroker* pBroker,const std::vector<CGirderKey>& girderKeys,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,IArtifact* pIArtifact);
void process_artifacts(ColumnIndexType startIdx, ColumnIndexType endIdx, const std::vector<CGirderKey>& girderKeys, IArtifact* pIArtifact,
                       const pgsDesignArtifact** pArtifacts, bool& didFlexure, bool& didShear, bool& didLifting, bool& didHauling, bool& isHarped, bool& isTemporary);
void write_primary_shear_data(rptParagraph* pParagraph, IEAFDisplayUnits* pDisplayUnits,Float64 girderLength, ZoneIndexType nz,const CShearData2* pShearData);
void write_horiz_shear_data(rptParagraph* pParagraph, IEAFDisplayUnits* pDisplayUnits, Float64 girderLength, const CShearData2* pShearData);
void write_additional_shear_data(rptParagraph* pParagraph, IEAFDisplayUnits* pDisplayUnits, Float64 girderLength, const CShearData2* pShearData);
void write_design_notes(rptParagraph* pParagraph, const std::vector<pgsDesignArtifact::DesignNote>& notes);

// Function to compute columns in table that attempts to group all girders in a span per table
static const int MIN_TBL_COLS=3; // Minimum columns in multi-girder table
static const int MAX_TBL_COLS=8; // Maximum columns in multi-girder table

// Some fails can be successses (with caveats)
bool WasDesignSuccessful( pgsDesignArtifact::Outcome outcome)
{
      return outcome == pgsDesignArtifact::Success ||
             outcome == pgsDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionCy ||
             outcome == pgsDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionLifting ||
             outcome == pgsDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionHauling;
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
   ATLASSERT( pReportSpec != NULL );

   std::vector<CGirderKey> girderKeys = pReportSpec->GetGirderKeys();

   CComPtr<IBroker> pBroker;
   pReportSpec->GetBroker(&pBroker);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2( pBroker, IEAFDisplayUnits, pDisplayUnits );
   GET_IFACE2( pBroker, IArtifact, pIArtifact );

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
      rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      (*pChapter) << pPara;
      (*pPara) << rptNewLine <<_T("Design Outcomes for Individual Girders");
   }

   // Loop over all designed girders
   std::vector<CGirderKey>::iterator girderKeyIter(girderKeys.begin());
   std::vector<CGirderKey>::iterator girderKeyIterEnd(girderKeys.end());
   for ( ; girderKeyIter != girderKeyIterEnd; girderKeyIter++ )
   {
      CGirderKey& girderKey = *girderKeyIter;

      const pgsDesignArtifact* pArtifact = pIArtifact->GetDesignArtifact(girderKey);

      if ( pArtifact == NULL )
      {
         rptParagraph* pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << _T("This girder has not been designed") << rptNewLine;
         return pChapter;
      }

      pgsDesignArtifact::Outcome outcome = pArtifact->GetOutcome();
      if ( WasDesignSuccessful(outcome))
      {
         successful_design(pBroker,pChapter,pDisplayUnits,pArtifact);
      }
      else
      {
         failed_design(pBroker,pChapter,pDisplayUnits,pArtifact);
      }
   }

   return pChapter;
}

CChapterBuilder* CDesignOutcomeChapterBuilder::Clone() const
{
   return new CDesignOutcomeChapterBuilder;
}

void write_artifact_data(IBroker* pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsDesignArtifact* pArtifact)
{
   const CSegmentKey& segmentKey = pArtifact->GetSegmentKey();

   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, distance, pDisplayUnits->GetXSectionDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       true );

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   arDesignOptions options = pArtifact->GetDesignOptions();

   // Start report with design notes. Notes will be written at end of this function
   bool design_success = WasDesignSuccessful( pArtifact->GetOutcome() );
   bool doNotes = (pArtifact->GetDoDesignFlexure()!=dtNoDesign && design_success) || 
                   pArtifact->DoDesignNotesExist();

   rptParagraph* pNotesParagraph = doNotes ? new rptParagraph() : NULL;
   if ( doNotes )
   {
      rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pParagraph;
      *pParagraph << _T("Design Notes:");
      *pChapter << pNotesParagraph;
   }

   if (pArtifact->GetDoDesignFlexure()!=dtNoDesign)
   {
      GDRCONFIG config = pArtifact->GetSegmentConfiguration();

      GET_IFACE2(pBroker, ISegmentData,pSegmentData);
      const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);

      rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pParagraph;
      *pParagraph << _T("Flexure Design:");

      // Make note if original strands were filled using direct input
      if (pStrands->NumPermStrandsType == CStrandData::npsDirectSelection)
      {
         pParagraph = new rptParagraph();
         *pChapter << pParagraph;
         *pParagraph << color(Blue)<<_T("Note that current strands are filled using direct selection.") << color(Black);
      }

      // see if fill order type was changed
      if (options.doStrandFillType==ftGridOrder)
      {
         StrandIndexType num_permanent = pArtifact->GetNumHarpedStrands() + pArtifact->GetNumStraightStrands();
         // we asked design to fill using grid, but this may be a non-standard design - let's check
         GET_IFACE2(pBroker,IStrandGeometry, pStrandGeometry );

         StrandIndexType ns, nh;
         if (pStrandGeometry->ComputeNumPermanentStrands(num_permanent, segmentKey, &ns, &nh))
         {
            if (ns!=pArtifact->GetNumStraightStrands() )
            {
               pParagraph = new rptParagraph();
               *pChapter << pParagraph;
               *pParagraph << color(Blue)<<_T("Note that strand fill order has been changed from ")<<Bold(_T("Number of Permanent"))<<_T(" to ")<< Bold(_T("Number of Straight and Number of Harped")) << _T(" strands.") << color(Black);
            }
         }
      }

      pParagraph = new rptParagraph();
      *pChapter << pParagraph;

      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(3);
      *pParagraph << pTable;

      pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      RowIndexType row = 0;

      (*pTable)(row,0) << _T("Parameter");
      (*pTable)(row,1) << _T("Proposed Design");
      (*pTable)(row,2) << _T("Current Value");

      row++;
      GET_IFACE2(pBroker,IGirderLifting,pGirderLifting);
      GET_IFACE2(pBroker,IGirderHauling,pGirderHauling);
      GET_IFACE2(pBroker,IStrandGeometry, pStrandGeometry );

      // current offsets, measured in absolute
      Float64 abs_offset_end, abs_offset_hp;
      pStrandGeometry->GetHarpStrandOffsets(segmentKey,&abs_offset_end,&abs_offset_hp);

      (*pTable)(row,0) << _T("Number of Straight Strands");
      (*pTable)(row,1) << config.PrestressConfig.GetNStrands(pgsTypes::Straight);
      (*pTable)(row,2) << pStrands->Nstrands[pgsTypes::Straight];

      // print straight debond information if exists
      CollectionIndexType ddb = config.PrestressConfig.Debond[pgsTypes::Straight].size();
      StrandIndexType pdb = pStrandGeometry->GetNumDebondedStrands(segmentKey,pgsTypes::Straight);
      if (ddb>0 || pdb>0)
      {
         (*pTable)(row,1) << _T(" (")<<ddb<<_T(" debonded)");
         (*pTable)(row,2) << _T(" (")<<pdb<<_T(" debonded)");
      }

      row++;

      (*pTable)(row,0) << _T("Number of ") << LABEL_HARP_TYPE(options.doForceHarpedStrandsStraight) << _T(" Strands");
      (*pTable)(row,1) << config.PrestressConfig.GetNStrands(pgsTypes::Harped);
      (*pTable)(row,2) << pStrands->Nstrands[pgsTypes::Harped];
      row++;

      if ( 0 < pStrandGeometry->GetMaxStrands(segmentKey,pgsTypes::Temporary) )
      {
         (*pTable)(row,0) << _T("Number of Temporary Strands");
         (*pTable)(row,1) << config.PrestressConfig.GetNStrands(pgsTypes::Temporary);
         (*pTable)(row,2) << pStrands->Nstrands[pgsTypes::Temporary];
         row++;
      }

      (*pTable)(row,0) << _T("Straight Strand Jacking Force");
      (*pTable)(row,1) << force.SetValue(config.PrestressConfig.Pjack[pgsTypes::Straight]);
      (*pTable)(row,2) << force.SetValue(pStrands->Pjack[pgsTypes::Straight]);
      row++;

      (*pTable)(row,0) << LABEL_HARP_TYPE(options.doForceHarpedStrandsStraight) << _T(" Strand Jacking Force");
      (*pTable)(row,1) << force.SetValue(config.PrestressConfig.Pjack[pgsTypes::Harped]);
      (*pTable)(row,2) << force.SetValue(pStrands->Pjack[pgsTypes::Harped]);
      row++;

      if ( 0 < pStrandGeometry->GetMaxStrands(segmentKey,pgsTypes::Temporary) )
      {
         (*pTable)(row,0) << _T("Temporary Strand Jacking Force");
         (*pTable)(row,1) << force.SetValue(config.PrestressConfig.Pjack[pgsTypes::Temporary]);
         (*pTable)(row,2) << force.SetValue(pStrands->Pjack[pgsTypes::Temporary]);
         row++;
      }

      if (0 < config.PrestressConfig.GetNStrands(pgsTypes::Harped))
      {
         HarpedStrandOffsetType HsoEnd = pStrands->HsoEndMeasurement;
         if(options.doForceHarpedStrandsStraight)
         {
         switch( HsoEnd )
         {
         case hsoCGFROMTOP:
            (*pTable)(row,0) << _T("Distance from top of girder to") << rptNewLine << _T("CG of harped strand group at ends of girder");
            break;

         case hsoCGFROMBOTTOM:
            (*pTable)(row,0) << _T("Distance from bottom of girder to") << rptNewLine << _T("CG of harped strand group at ends of girder");
            break;

         case hsoLEGACY:
            // convert legacy to display TOP 2 TOP

            HsoEnd = hsoTOP2TOP;

         case hsoTOP2TOP:
            (*pTable)(row,0) << _T("Distance from top of girder to") << rptNewLine << _T("top of harped strand group at ends of girder");
            break;

         case hsoTOP2BOTTOM:
            (*pTable)(row,0) << _T("Distance from bottom of girder") << rptNewLine << _T("to top of harped strand group at ends of girder");
            break;

         case hsoBOTTOM2BOTTOM:
            (*pTable)(row,0) << _T("Distance from bottom of girder") << rptNewLine << _T("to bottom of harped strand group at ends of girder");
            break;

         case hsoECCENTRICITY:
            (*pTable)(row,0) << _T("Eccentricity of harped strand") << rptNewLine << _T("group at ends of girder");
            break;

         default:
            ATLASSERT(false); // should never get here
         }
         }
         else
         {
            switch( HsoEnd )
            {
            case hsoCGFROMTOP:
               (*pTable)(row,0) << _T("Distance from top of girder to") << rptNewLine << _T("CG of harped strand group at ends of girder");
               break;

            case hsoCGFROMBOTTOM:
               (*pTable)(row,0) << _T("Distance from bottom of girder to") << rptNewLine << _T("CG of harped strand group at ends of girder");
               break;

            case hsoLEGACY:
               // convert legacy to display TOP 2 TOP

               HsoEnd = hsoTOP2TOP;

            case hsoTOP2TOP:
               (*pTable)(row,0) << _T("Distance from top of girder to") << rptNewLine << _T("top of harped strand group at ends of girder");
               break;

            case hsoTOP2BOTTOM:
               (*pTable)(row,0) << _T("Distance from bottom of girder") << rptNewLine << _T("to top of harped strand group at ends of girder");
               break;

            case hsoBOTTOM2BOTTOM:
               (*pTable)(row,0) << _T("Distance from bottom of girder") << rptNewLine << _T("to bottom of harped strand group at ends of girder");
               break;

            case hsoECCENTRICITY:
               (*pTable)(row,0) << _T("Eccentricity of harped strand") << rptNewLine << _T("group at ends of girder");
               break;

            default:
               ATLASSERT(false); // should never get here
            }
         }

         const ConfigStrandFillVector& confvec_design = config.PrestressConfig.GetStrandFill(pgsTypes::Harped);

         Float64 offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(segmentKey,
                                                                             confvec_design, 
                                                                             HsoEnd, 
                                                                             pArtifact->GetHarpStrandOffsetEnd());
         (*pTable)(row,1) << length.SetValue(offset);

         ConfigStrandFillVector confvec_current = pStrandGeometry->ComputeStrandFill(segmentKey, pgsTypes::Harped, pStrands->GetNstrands(pgsTypes::Harped));

         offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(segmentKey, confvec_current, 
                                                                      HsoEnd, abs_offset_end);

         (*pTable)(row,2) << length.SetValue(offset);

         row++;

         if(!options.doForceHarpedStrandsStraight)
         {
         HarpedStrandOffsetType HsoHp = pStrands->HsoHpMeasurement;
         switch( HsoHp )
         {
         case hsoCGFROMTOP:
            (*pTable)(row,0) << _T("Distance from top of girder to") << rptNewLine << _T("CG of harped strand group at harping point");
            break;

         case hsoCGFROMBOTTOM:
            (*pTable)(row,0) << _T("Distance from bottom of girder to") << rptNewLine << _T("CG of harped strand group at harping point");
            break;

         case hsoTOP2TOP:
            (*pTable)(row,0) << _T("Distance from top of girder to") << rptNewLine << _T("top of harped strand group at harping point");
            break;

         case hsoTOP2BOTTOM:
            (*pTable)(row,0) << _T("Distance from bottom of girder to") << rptNewLine << _T("top of harped strand group at harping point");
            break;

         case hsoLEGACY:
            // convert legacy to display BOTTOM 2 BOTTOM
            HsoHp = hsoBOTTOM2BOTTOM;

         case hsoBOTTOM2BOTTOM:
            (*pTable)(row,0) << _T("Distance from bottom of girder to") << rptNewLine << _T("bottom of harped strand group at harping point");
            break;


         case hsoECCENTRICITY:
            (*pTable)(row,0) << _T("Eccentricity of harped strand") << rptNewLine << _T("group at harping point");
            break;

         default:
            ATLASSERT(false); // should never get here
         }


         offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(segmentKey,
                                                                     confvec_design, 
                                                                     HsoHp, 
                                                                     pArtifact->GetHarpStrandOffsetHp());

         (*pTable)(row,1) << length.SetValue(offset);

         offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(segmentKey,
                                                                     confvec_current, 
                                                                     HsoHp, abs_offset_hp);

         (*pTable)(row,2) << length.SetValue(offset);

         row++;
      }
      }

      (*pTable)(row,0) << RPT_FCI;
      (*pTable)(row,1) << stress.SetValue(pArtifact->GetReleaseStrength());
      (*pTable)(row,2) << stress.SetValue(pMaterial->Concrete.Fci);
      row++;

      (*pTable)(row,0) << RPT_FC;
      (*pTable)(row,1) << stress.SetValue(pArtifact->GetConcreteStrength());
      (*pTable)(row,2) << stress.SetValue(pMaterial->Concrete.Fc);
      row++;

      if ( options.doDesignSlabOffset && (pBridge->GetDeckType()!=pgsTypes::sdtNone) )
      {
         const CGirderGroupData* pGroup = pIBridgeDesc->GetBridgeDescription()->GetGirderGroup(segmentKey.groupIndex);
         const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

         // the computed slab offset will be applied according to the current slab offset mode
         if ( pIBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge )
         {
            // slab offset is for the entire bridge... the start value contains this parameter
            (*pTable)(row,0) << _T("Slab Offset (\"A\" Dimension)");
            (*pTable)(row,1) << length.SetValue( pArtifact->GetSlabOffset(pgsTypes::metStart) );
            (*pTable)(row,2) << length.SetValue( pGroup->GetSlabOffset(pGroup->GetPierIndex(pgsTypes::metStart),segmentKey.girderIndex));
            row++;
         }
         else
         {
            (*pTable)(row,0) << _T("Slab Offset at Start (\"A\" Dimension)");
            (*pTable)(row,1) << length.SetValue( pArtifact->GetSlabOffset(pgsTypes::metStart) );
            (*pTable)(row,2) << length.SetValue( pGroup->GetSlabOffset(pGroup->GetPierIndex(pgsTypes::metStart),segmentKey.girderIndex));
            row++;

            (*pTable)(row,0) << _T("Slab Offset at End (\"A\" Dimension)");
            (*pTable)(row,1) << length.SetValue( pArtifact->GetSlabOffset(pgsTypes::metEnd) );
            (*pTable)(row,2) << length.SetValue( pGroup->GetSlabOffset(pGroup->GetPierIndex(pgsTypes::metEnd),segmentKey.girderIndex));
            row++;
         }
      }

      if (options.doDesignLifting)
      {
         (*pTable)(row,0) << _T("Lifting Point Location (Left)");
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetLeftLiftingLocation() );
         (*pTable)(row,2) << distance.SetValue( pGirderLifting->GetLeftLiftingLoopLocation(segmentKey) );
         row++;

         (*pTable)(row,0) << _T("Lifting Point Location (Right)");
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetRightLiftingLocation() );
         (*pTable)(row,2) << distance.SetValue( pGirderLifting->GetRightLiftingLoopLocation(segmentKey) );
         row++;
      }

      if (options.doDesignHauling)
      {
         (*pTable)(row,0) << _T("Truck Support Location (Leading)");
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetLeadingOverhang() );
         (*pTable)(row,2) << distance.SetValue( pGirderHauling->GetLeadingOverhang(segmentKey) );
         row++;

         (*pTable)(row,0) << _T("Truck Support Location (Trailing)");
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetTrailingOverhang() );
         (*pTable)(row,2) << distance.SetValue( pGirderHauling->GetTrailingOverhang(segmentKey) );
         row++;
      }
   }
   else
   {
      rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pParagraph;
      *pParagraph << _T("Flexure Design Not Requested")<<rptNewLine;
   }

   // Need design notes in shear output and at end of report
   std::vector<pgsDesignArtifact::DesignNote> design_notes = pArtifact->GetDesignNotes();

   if (pArtifact->GetDoDesignShear())
   {

      rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pParagraph;
      *pParagraph << _T("Shear Design:");

      pParagraph = new rptParagraph();
      *pChapter << pParagraph;

      // Check if existing stirrup layout passed. Don't report design if it did
      std::vector<pgsDesignArtifact::DesignNote>::iterator itd = std::find(design_notes.begin(), design_notes.end(),
                                                                           pgsDesignArtifact::dnExistingShearDesignPassedSpecCheck);
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
         ZoneIndexType nz = pArtifact->GetNumberOfStirrupZonesDesigned();

         if (nz>0)
         {
            write_primary_shear_data(pParagraph, pDisplayUnits, girder_length, nz, pShearData);
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
      ZoneIndexType ncz = p_shear_data->ShearZones.size();

      write_primary_shear_data(pParagraph, pDisplayUnits, girder_length, ncz, p_shear_data);

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
         GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
         std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_MIDSPAN,POIFIND_AND);
         CHECK(vPoi.size()==1);
         pgsPointOfInterest poi = *vPoi.begin();

         GDRCONFIG config = pArtifact->GetSegmentConfiguration();

         GET_IFACE2(pBroker,ICamber,pCamber);
         Float64 excess_camber = pCamber->GetExcessCamber(poi,config,CREEP_MAXTIME);
         if ( excess_camber < 0 )
         {
            *pNotesParagraph<<color(Red)<< _T("Warning:  Excess camber is negative, indicating a potential sag in the beam.")<<color(Black)<< rptNewLine;
         }

         *pNotesParagraph << _T("Concrete release strength was controlled by ") << pArtifact->GetReleaseDesignState().AsString() << rptNewLine;
         *pNotesParagraph << _T("Concrete final strength was controlled by ") << pArtifact->GetFinalDesignState().AsString() << rptNewLine;
         *pNotesParagraph << rptNewLine;

         if ( options.doDesignSlabOffset && (pBridge->GetDeckType()!=pgsTypes::sdtNone) )
         {
            if ( pIBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge )
            {
               *pNotesParagraph << _T("Slab Offset will be applied to the bridge") << rptNewLine;
            }
            else
            {
               *pNotesParagraph << _T("Slab Offset will be applied to this girder") << rptNewLine;
            }
         }
      }
   }
}

void successful_design(IBroker* pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsDesignArtifact* pArtifact)
{
   const CSegmentKey& segmentKey = pArtifact->GetSegmentKey();
   ATLASSERT(segmentKey.segmentIndex == 0); // design is only for precast girders which only have one segment

   rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pParagraph;

   pgsDesignArtifact::Outcome outcome = pArtifact->GetOutcome();

   if ( outcome == pgsDesignArtifact::Success)
   {
      *pParagraph << color(Green)
                  << _T("The design for Span ") << LABEL_GROUP(segmentKey.groupIndex)
                  << _T(" Girder ") << LABEL_GIRDER(segmentKey.girderIndex)
                  << _T(" was successful.") 
                  << color(Black)
                  << rptNewLine;
   }
   else if (outcome == pgsDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionCy ||
            outcome == pgsDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionLifting ||
            outcome == pgsDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionHauling)
   {
      *pParagraph << color(OrangeRed)
                  << _T("The design for Span ") << LABEL_GROUP(segmentKey.groupIndex)
                  << _T(" Girder ") << LABEL_GIRDER(segmentKey.girderIndex)
                  << _T(" failed.")
                  << color(Black);

      rptParagraph* pParagraph = new rptParagraph( );
      *pChapter << pParagraph;
      *pParagraph << color(OrangeRed) 
                  << _T("But, you may be able to create a successful design by adding longitudinal rebar to increase temporary tensile stress limits");

      if (outcome == pgsDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionCy)
      {
         *pParagraph << _T(" for release in the Casting Yard.");
      }
      else if(outcome == pgsDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionLifting)
      {
         *pParagraph << _T(" for girder Lifting.");
      }
      else if(outcome == pgsDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionHauling)
      {
         *pParagraph << _T(" for girder Hauling.");
      }

      *pParagraph << color(Black);
   }
   else
   {
      ATLASSERT(0);
   }


   write_artifact_data(pBroker,pChapter,pDisplayUnits,pArtifact);
}

void failed_design(IBroker* pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsDesignArtifact* pArtifact)
{
   const CSegmentKey& segmentKey = pArtifact->GetSegmentKey();

   rptParagraph* pParagraph;
   pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pParagraph;

   *pParagraph << color(Red)
               << _T("The design attempt for Span ") << LABEL_GROUP(segmentKey.groupIndex)
               << _T(" Girder ") << LABEL_GIRDER(segmentKey.girderIndex)
               << _T(" failed.") 
               << color(Black)
               << rptNewLine;

   pParagraph = new rptParagraph( pgsReportStyleHolder::GetSubheadingStyle() );
   *pChapter << pParagraph;
   switch( pArtifact->GetOutcome() )
   {
      case pgsDesignArtifact::Success:
         CHECK(false); // Should never get here
         break;

      case pgsDesignArtifact::NoDesignRequested:
         *pParagraph << _T("No Design was requested.") << rptNewLine;
         break;

      case pgsDesignArtifact::TooManyStrandsReqd:
         *pParagraph << _T("Too many strands are required to satisfy the stress criteria.") << rptNewLine;
         break;

      case pgsDesignArtifact::OverReinforced:
         *pParagraph << _T("The section is over reinforced and the number of strands cannot be reduced.") << rptNewLine;
         break;

      case pgsDesignArtifact::UnderReinforced:
         *pParagraph << _T("The section is under reinforced and the number of strands cannot be increased.") << rptNewLine;
         *pParagraph << _T("Increasing the compressive strength of the deck will improve section capacity.") << rptNewLine;
         break;

      case pgsDesignArtifact::UltimateMomentCapacity:
         *pParagraph << _T("Too many strands are required to satisfy ultimate moment capacity criteria.") << rptNewLine;
         break;

      case pgsDesignArtifact::MaxIterExceeded:
         *pParagraph << _T("After several iterations, a successful design could not be found.") << rptNewLine;
         break;

      case pgsDesignArtifact::ReleaseStrength:
         *pParagraph << _T("An acceptable concrete release strength could not be found.") << rptNewLine;
         break;

      case pgsDesignArtifact::ExceededMaxHoldDownForce:
         *pParagraph << _T("Design is such that maximum allowable hold down force in casting yard is exceeded.")<<rptNewLine;
         break;

      case pgsDesignArtifact::StrandSlopeOutOfRange:
         *pParagraph << _T("Design is such that maximum strand slope is exceeded.")<<rptNewLine;
         break;

      case pgsDesignArtifact::ShearExceedsMaxConcreteStrength:
         *pParagraph << _T("Section is too small to carry ultimate shear. Crushing capacity was exceeded") << rptNewLine;
         break;

      case pgsDesignArtifact::TooManyStirrupsReqd:
         *pParagraph << _T("Could not design stirrups - Minimum spacing requirements were violated.") << rptNewLine;
         break;

      case pgsDesignArtifact::TooManyStirrupsReqdForHorizontalInterfaceShear:
         *pParagraph << _T("Could not design stirrups - Cannot add enough stirrups to resist horizontal interface shear requirements.") << rptNewLine;
         break;

      case pgsDesignArtifact::TooManyStirrupsReqdForSplitting:
         *pParagraph << _T("Could not design stirrups for splitting demand - Minimum spacing requirements were violated") << rptNewLine;
         break;

      case pgsDesignArtifact::ConflictWithLongReinforcementShearSpec:
         *pParagraph << _T("Failed designing for longitudinal reinforcement for shear due to conflicting library information. Project criteria Shear Design tab says to use longitudinal rebar, while Shear Capacity tab disables use of mild steel rebar.") << rptNewLine;
         break;

      case pgsDesignArtifact::StrandsReqdForLongReinfShearAndFlexureTurnedOff:
         *pParagraph << _T("Additional strands are required to meet longitudinal reinforcement for shear requirements. However, this can only be performed if flexural design is enabled.") << rptNewLine;
         break;

      case pgsDesignArtifact::NoDevelopmentLengthForLongReinfShear:
         *pParagraph << _T("Additional longitudinal mild steel reinforcement bars are required to meet longitudinal reinforcement for shear requirements. However, the face of support is at the end of the girder, so there is no room for rebar development. Consider changing the connection to allow for development.") << rptNewLine;
         break;

      case pgsDesignArtifact::NoStrandDevelopmentLengthForLongReinfShear:
         *pParagraph << _T("Additional strands are required to meet longitudinal reinforcement for shear requirements. However, the face of support is at the end of the girder, so there is no room for prestress development. Consider changing the connection to allow for development.") << rptNewLine;
         break;

      case pgsDesignArtifact::TooManyBarsForLongReinfShear:
         *pParagraph << _T("Could not add enough mild steel reinforcement to meet longitudinal reinforcement for shear requirements while maintaining minimum spacing requirements per LRFD 5.10.3.1.2.") << rptNewLine;
         break;

      case pgsDesignArtifact::TooMuchStrandsForLongReinfShear:
         *pParagraph << _T("Could not add enough strands to meet longitudinal reinforcement for shear requirements.") << rptNewLine;
         break;

      case pgsDesignArtifact::GirderLiftingStability:
         *pParagraph << _T("Could not satisfy stability requirements for lifting") << rptNewLine;
         break;

      case pgsDesignArtifact::GirderLiftingConcreteStrength:
         *pParagraph << _T("Could not find a concrete strength to satisfy stress limits for lifting") << rptNewLine;
         break;

      case pgsDesignArtifact::GirderShippingStability:
         *pParagraph << _T("Could not satisfy stability requirements for shipping") << rptNewLine;
         break;

      case pgsDesignArtifact::GirderShippingConfiguration:
         *pParagraph << _T("Could not satisfy trucking configuration requirements for shipping") << rptNewLine;
         break;

      case pgsDesignArtifact::GirderShippingConcreteStrength:
         *pParagraph << _T("Could not find a concrete strength to satisfy stress limits for shipping") << rptNewLine;
         break;

      case pgsDesignArtifact::StressExceedsConcreteStrength:
         *pParagraph << _T("Could not find a concrete strength to satisfy stress limits") << rptNewLine;
         break;
      
      case pgsDesignArtifact::DebondDesignFailed:
         *pParagraph << _T("Unable to find an adequate debond design") << rptNewLine;
         break;

      default:
         CHECK(false); // Should never get here
   }

   pParagraph = new rptParagraph();
   *pParagraph << Bold(_T("Results from last trial:")) << rptNewLine;
   *pChapter << pParagraph;
   write_artifact_data(pBroker,pChapter,pDisplayUnits,pArtifact);
}

std::wstring GetDesignNoteString(pgsDesignArtifact::DesignNote note)
{
   switch (note)
   {
   case pgsDesignArtifact::dnShearRequiresStrutAndTie:
      return std::wstring(_T("WARNING: A strut and tie analysis is required in the girder end zones per LRFD 5.8.3.2. This design will fail a spec check."));
      break;

   case pgsDesignArtifact::dnExistingShearDesignPassedSpecCheck:
      return std::wstring(_T("The existing stirrup input data passed the shear specification check. No design modificatons were made."));
      break;

   case pgsDesignArtifact::dnStrandsAddedForLongReinfShear:
      return std::wstring(_T("The number of strands was controlled by longitudinal reinforcement for shear requirements."));
      break;

   case pgsDesignArtifact::dnLongitudinalBarsNeeded4FlexuralTensionCy:
      return std::wstring(_T("Refer to the casting yard \"Rebar Requirements for Tensile Stress Limit\" sections in the Details report for more information about required longitudinal reinforcement."));
      break;

   case pgsDesignArtifact::dnLongitudinalBarsNeeded4FlexuralTensionLifting:
      return std::wstring(_T("Refer to the lifting \"Rebar Requirements for Tensile Stress Limit\" sections in the Details report for more information about required longitudinal reinforcement."));
      break;

   case pgsDesignArtifact::dnLongitudinalBarsNeeded4FlexuralTensionHauling:
      return std::wstring(_T("Refer to the hauling \"Rebar Requirements for Tensile Stress Limit\" sections in the Details report for more information about required longitudinal reinforcement."));
      break;

   default:
      ATLASSERT(0);
   }
   return std::wstring();
}

void write_design_notes(rptParagraph* pParagraph, const std::vector<pgsDesignArtifact::DesignNote>& notes)
{
   for(std::vector<pgsDesignArtifact::DesignNote>::const_iterator it = notes.begin(); it!=notes.end(); it++)
   {
      if (*it != pgsDesignArtifact::dnExistingShearDesignPassedSpecCheck) // this is written elsewhere
      {
         *pParagraph <<_T(" -  ") << GetDesignNoteString( *it ) <<rptNewLine;
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

   // Since girder types, design info, etc can be different for each girder, process information for all
   // artifacts to get control data
   const pgsDesignArtifact* pArtifacts[MAX_TBL_COLS]; // Might as well cache artifacts while processing
   bool did_flexure;
   bool did_shear;
   bool did_lifting;
   bool did_hauling;
   bool is_harped;
   bool is_temporary;

   process_artifacts(startIdx, endIdx, girderKeys, pIArtifact,
                     pArtifacts, did_flexure, did_shear, did_lifting, did_hauling, is_harped, is_temporary);

   if (!did_flexure && !did_shear)
   {
      ATLASSERT(0); // probably shouldn't be here if no design was done
      return;
   }

   rptParagraph* pParagraph = new rptParagraph();
   *pChapter << pParagraph;

   // Our table has a column for each girder
   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(endIdx-startIdx+2,_T("Multiple Girder Design Summary"));
   pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
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
         (*pTable)(row++,0) << _T("Number of Harped Strands");

      if (is_temporary)
         (*pTable)(row++,0) << _T("Number of Temporary Strands");

      (*pTable)(row++,0) << _T("Straight Strand Jacking Force");

      if (is_harped)
         (*pTable)(row++,0) << _T("Harped Strand Jacking Force");

      if (is_temporary)
         (*pTable)(row++,0) << _T("Temporary Strand Jacking Force");

      if (is_harped)
      {
         // We will have to convert any other measurements to this:
         (*pTable)(row++,0) << _T("Distance from Bottom of girder to") << rptNewLine << _T("top of harped strand group at ends of girder");
         (*pTable)(row++,0) << _T("Distance from bottom of girder to") << rptNewLine << _T("bottom of harped strand group at harping point");
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


   // Titles are now printed. Print results information
   ColumnIndexType idx = 0;
   ColumnIndexType col = 1;
   for (ColumnIndexType gdr_idx=startIdx; gdr_idx<=endIdx; gdr_idx++)
   {
      pTable->SetColumnWidth(col,0.75);

      const CGirderKey& girderKey = girderKeys[gdr_idx];

      row = 0;

      (*pTable)(row++,col) << _T("Span ") << LABEL_GROUP(girderKey.groupIndex) <<rptNewLine<<_T("Girder ")<<LABEL_GIRDER(girderKey.girderIndex);

      const pgsDesignArtifact* pArtifact = pArtifacts[idx++];
      const CSegmentKey& segmentKey(pArtifact->GetSegmentKey());

      pgsDesignArtifact::Outcome outcome = pArtifact->GetOutcome();
      if (outcome==pgsDesignArtifact::Success)
      {
         (*pTable)(row++,col) <<color(Green)<<_T("Success")<<color(Black);
      }
      else
      {
         (*pTable)(row++,col) <<color(Red)<<_T("Failed")<<color(Black);
      }

      (*pTable)(row++,col) << pArtifact->GetNumStraightStrands();

      if (is_harped)
         (*pTable)(row++,col) << pArtifact->GetNumHarpedStrands();

      if (is_temporary)
         (*pTable)(row++,col) << pArtifact->GetNumTempStrands();

      GDRCONFIG config = pArtifact->GetSegmentConfiguration();

      // jacking forces
      (*pTable)(row++,col) << force.SetValue(config.PrestressConfig.Pjack[pgsTypes::Straight]);

      if (is_harped)
         (*pTable)(row++,col) << force.SetValue(config.PrestressConfig.Pjack[pgsTypes::Harped]);

      if (is_temporary)
         (*pTable)(row++,col) << force.SetValue(config.PrestressConfig.Pjack[pgsTypes::Temporary]);

      if (is_harped)
      {
         if (pArtifact->GetNumHarpedStrands()>0)
         {
            const ConfigStrandFillVector& confvec_design = config.PrestressConfig.GetStrandFill(pgsTypes::Harped);


            Float64 offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(segmentKey,
                                                                                confvec_design, 
                                                                                hsoTOP2BOTTOM, 
                                                                                pArtifact->GetHarpStrandOffsetEnd());
            (*pTable)(row++,col) << length.SetValue(offset);

            offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(segmentKey,
                                                                        confvec_design, 
                                                                        hsoBOTTOM2BOTTOM, 
                                                                        pArtifact->GetHarpStrandOffsetHp());
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

      col++;
   }
}

void process_artifacts(ColumnIndexType startIdx, ColumnIndexType endIdx, const std::vector<CGirderKey>& girderKeys, IArtifact* pIArtifact,
                       const pgsDesignArtifact** pArtifacts, bool& didFlexure, bool& didShear, bool& didLifting, bool& didHauling, bool& isHarped, bool& isTemporary)
{
   // Set all outcomes to false
   didFlexure = false;
   didShear = false;
   didLifting = false;
   didHauling = false;
   isHarped = false;
   isTemporary = false;

   ColumnIndexType na = endIdx - startIdx + 1;
   ColumnIndexType idx = startIdx;
   for (ColumnIndexType ia=0; ia<na; ia++)
   {
      const CGirderKey& girderKey = girderKeys[idx];

      pArtifacts[ia] = pIArtifact->GetDesignArtifact(girderKey);

      arDesignOptions options = pArtifacts[ia]->GetDesignOptions();

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

      // report harped information if we have any harped designs or, if we have harped strands
      if (options.doDesignForFlexure == dtDesignForHarping || pArtifacts[ia]->GetNumHarpedStrands() > 0)
      {
         isHarped = true;
      }

      if (options.doDesignForFlexure != dtNoDesign && pArtifacts[ia]->GetNumTempStrands() > 0)
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
   CHECK(pool!=0);

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
      rptRcTable* pTables = pgsReportStyleHolder::CreateTableNoHeading(8);
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
               (*pTables)(row,col++) << _T("Mid-Girder");
            else
               (*pTables)(row,col++) << _T("End of Girder");

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
      rptRcTable* pTables = pgsReportStyleHolder::CreateTableNoHeading(6);
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
               (*pTables)(row,col++) << _T("Mid-Girder");
            else
               (*pTables)(row,col++) << _T("End of Girder");

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