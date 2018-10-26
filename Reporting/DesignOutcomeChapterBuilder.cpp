///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include <PgsExt\GirderData.h>

#include <PgsExt\BridgeDescription.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDesignOutcomeChapterBuilder
****************************************************************************/

void write_artifact_data(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsDesignArtifact* pArtifact);
void failed_design(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsDesignArtifact* pArtifact);
void successful_design(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsDesignArtifact* pArtifact);
void multiple_girder_table(ColumnIndexType startIdx, ColumnIndexType endIdx,IBroker* pBroker,std::vector<SpanGirderHashType>& girderList,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,IArtifact* pIArtifact);
void process_artifacts(ColumnIndexType startIdx, ColumnIndexType endIdx, std::vector<SpanGirderHashType>& girderList, IArtifact* pIArtifact,
                       const pgsDesignArtifact** pArtifacts, bool& didFlexure, bool& didShear, bool& didLifting, bool& didHauling, bool& isHarped, bool& isTemporary);
void write_primary_shear_data(rptParagraph* pParagraph, IEAFDisplayUnits* pDisplayUnits,Float64 girderLength, ZoneIndexType nz,const CShearData& rsdata);
void write_horiz_shear_data(rptParagraph* pParagraph, IEAFDisplayUnits* pDisplayUnits, Float64 girderLength, const CShearData& rsdata);
void write_additional_shear_data(rptParagraph* pParagraph, IEAFDisplayUnits* pDisplayUnits, Float64 girderLength, const CShearData& rsdata);
void write_design_notes(rptChapter* pChapter, const std::vector<pgsDesignArtifact::DesignNote>& notes);

// Function to compute columns in table that attempts to group all girders in a span per table
static const int MIN_TBL_COLS=3; // Minimum columns in multi-girder table
static const int MAX_TBL_COLS=8; // Maximum columns in multi-girder table

inline std::list<ColumnIndexType> ComputeTableCols(const std::vector<SpanGirderHashType>& spanGirders)
{
   // Idea here is to break tables at spans. 
   // First build list of sizes of contiguous blocks of spans
   std::list<ColumnIndexType> contiguous_blocks1;
   SpanIndexType curr_span(-1);
   bool first=false;
   for(std::vector<SpanGirderHashType>::const_iterator it=spanGirders.begin(); it!=spanGirders.end(); it++)
   {
      SpanIndexType new_span;
      GirderIndexType new_gdr;
      UnhashSpanGirder(*it, &new_span, &new_gdr);

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

   std::vector<SpanGirderHashType> list = pReportSpec->GetGirderList();

   CComPtr<IBroker> pBroker;
   pReportSpec->GetBroker(&pBroker);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2( pBroker, IEAFDisplayUnits, pDisplayUnits );
   GET_IFACE2( pBroker, IArtifact, pIArtifact );

   // Write multiple girder table only if we have more than one girder
   std::list<ColumnIndexType> table_cols = ComputeTableCols(list);

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
            ATLASSERT(end_idx < list.size());
         }

         multiple_girder_table(start_idx, end_idx, pBroker, list, pChapter, pDisplayUnits, pIArtifact);
      }
   }

   if (!list.empty())
   {
      rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      (*pChapter) << pPara;
      (*pPara) << rptNewLine <<_T("Design Outcomes for Individual Girders");
   }

   // Loop over all designed girders
   for (std::vector<SpanGirderHashType>::iterator it=list.begin(); it!=list.end(); it++)
   {
      SpanIndexType span;
      GirderIndexType gdr;
      UnhashSpanGirder(*it,&span,&gdr);

      const pgsDesignArtifact* pArtifact = pIArtifact->GetDesignArtifact(span,gdr);

      if ( pArtifact == NULL )
      {
         rptParagraph* pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << _T("This girder has not been designed") << rptNewLine;
         return pChapter;
      }

      if ( pArtifact->GetOutcome() == pgsDesignArtifact::Success )
      {
         successful_design(pBroker,span,gdr,pChapter,pDisplayUnits,pArtifact);
      }
      else
      {
         failed_design(pBroker,span,gdr,pChapter,pDisplayUnits,pArtifact);
      }
   }

   return pChapter;
}

CChapterBuilder* CDesignOutcomeChapterBuilder::Clone() const
{
   return new CDesignOutcomeChapterBuilder;
}

void write_artifact_data(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsDesignArtifact* pArtifact)
{

   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, distance, pDisplayUnits->GetXSectionDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       true );

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   arDesignOptions options = pArtifact->GetDesignOptions();

   if (pArtifact->GetDoDesignFlexure()!=dtNoDesign)
   {
      rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pParagraph;

      // see if fill order type was changed
      if (options.doStrandFillType==ftGridOrder)
      {
         StrandIndexType num_permanent = pArtifact->GetNumHarpedStrands() + pArtifact->GetNumStraightStrands();
         // we asked design to fill using grid, but this may be a non-standard design - let's check
         GET_IFACE2(pBroker,IStrandGeometry, pStrandGeometry );

         StrandIndexType ns, nh;
         if (pStrandGeometry->ComputeNumPermanentStrands(num_permanent, span, gdr, &ns, &nh))
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

      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(3,_T(""));
      *pParagraph << pTable;

      pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      RowIndexType row = 0;

      (*pTable)(row,0) << _T("Parameter");
      (*pTable)(row,1) << _T("Proposed Design");
      (*pTable)(row,2) << _T("Current Value");

      row++;

      GDRCONFIG config = pArtifact->GetGirderConfiguration();

      GET_IFACE2(pBroker, IGirderData, pGirderData);
      CGirderData girderData = pGirderData->GetGirderData(span,gdr);

      GET_IFACE2(pBroker,IGirderLifting,pGirderLifting);
      GET_IFACE2(pBroker,IGirderHauling,pGirderHauling);
      GET_IFACE2(pBroker,IStrandGeometry, pStrandGeometry );

      // current offsets, measured in absolute
      Float64 abs_offset_end, abs_offset_hp;
      pStrandGeometry->GetHarpStrandOffsets(span,gdr,&abs_offset_end,&abs_offset_hp);

      (*pTable)(row,0) << _T("Number of Straight Strands");
      (*pTable)(row,1) << config.Nstrands[pgsTypes::Straight];
      (*pTable)(row,2) << girderData.Nstrands[pgsTypes::Straight];

      // print straight debond information if exists
      StrandIndexType ddb = config.Debond[pgsTypes::Straight].size();
      StrandIndexType pdb = pStrandGeometry->GetNumDebondedStrands(span,gdr,pgsTypes::Straight);
      if (ddb>0 || pdb>0)
      {
         (*pTable)(row,1) << _T(" (")<<ddb<<_T(" debonded)");
         (*pTable)(row,2) << _T(" (")<<pdb<<_T(" debonded)");
      }

      row++;

      (*pTable)(row,0) << _T("Number of ") << LABEL_HARP_TYPE(options.doForceHarpedStrandsStraight) << _T(" Strands");
      (*pTable)(row,1) << config.Nstrands[pgsTypes::Harped];
      (*pTable)(row,2) << girderData.Nstrands[pgsTypes::Harped];
      row++;

      if ( 0 < pStrandGeometry->GetMaxStrands(span,gdr,pgsTypes::Temporary) )
      {
         (*pTable)(row,0) << _T("Number of Temporary Strands");
         (*pTable)(row,1) << config.Nstrands[pgsTypes::Temporary];
         (*pTable)(row,2) << girderData.Nstrands[pgsTypes::Temporary];
         row++;
      }

      (*pTable)(row,0) << _T("Straight Strand Jacking Force");
      (*pTable)(row,1) << force.SetValue(config.Pjack[pgsTypes::Straight]);
      (*pTable)(row,2) << force.SetValue(girderData.Pjack[pgsTypes::Straight]);
      row++;

      (*pTable)(row,0) << LABEL_HARP_TYPE(options.doForceHarpedStrandsStraight) << _T(" Strand Jacking Force");
      (*pTable)(row,1) << force.SetValue(config.Pjack[pgsTypes::Harped]);
      (*pTable)(row,2) << force.SetValue(girderData.Pjack[pgsTypes::Harped]);
      row++;

      if ( 0 < pStrandGeometry->GetMaxStrands(span,gdr,pgsTypes::Temporary) )
      {
         (*pTable)(row,0) << _T("Temporary Strand Jacking Force");
         (*pTable)(row,1) << force.SetValue(config.Pjack[pgsTypes::Temporary]);
         (*pTable)(row,2) << force.SetValue(girderData.Pjack[pgsTypes::Temporary]);
         row++;
      }

      if (config.Nstrands[pgsTypes::Harped] > 0)
      {
         HarpedStrandOffsetType HsoEnd = girderData.HsoEndMeasurement;

         if(options.doForceHarpedStrandsStraight)
         {
            switch( HsoEnd )
            {
            case hsoCGFROMTOP:
               (*pTable)(row,0) << _T("Distance from top of girder to") << rptNewLine << _T("CG of Straight-Web strands");
               break;

            case hsoCGFROMBOTTOM:
               (*pTable)(row,0) << _T("Distance from bottom of girder to") << rptNewLine << _T("CG of Straight-Web strands");
               break;

            case hsoLEGACY:
               // convert legacy to display TOP 2 TOP
               HsoEnd = hsoTOP2TOP;

            case hsoTOP2TOP:
               (*pTable)(row,0) << _T("Distance from top of girder to") << rptNewLine << _T("top-most Straight-Web strand");
               break;

            case hsoTOP2BOTTOM:
               (*pTable)(row,0) << _T("Distance from bottom of girder") << rptNewLine << _T("to top-most Straight-Web strand");
               break;

            case hsoBOTTOM2BOTTOM:
               (*pTable)(row,0) << _T("Distance from bottom of girder") << rptNewLine << _T("to lowest Straight-Web strand");
               break;

            case hsoECCENTRICITY:
               (*pTable)(row,0) << _T("Eccentricity of Straight-Web strand group");
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

         double offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(span, gdr,
                                                                             pArtifact->GetNumHarpedStrands(), 
                                                                             HsoEnd, 
                                                                             pArtifact->GetHarpStrandOffsetEnd());
         (*pTable)(row,1) << length.SetValue(offset);

         offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(span, gdr, girderData.Nstrands[pgsTypes::Harped], 
                                                             HsoEnd, abs_offset_end);

         (*pTable)(row,2) << length.SetValue(offset);

         row++;

         if(!options.doForceHarpedStrandsStraight)
         {
            HarpedStrandOffsetType HsoHp = girderData.HsoHpMeasurement;
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


            offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(span, gdr,
                                                                        pArtifact->GetNumHarpedStrands(), 
                                                                        HsoHp, 
                                                                        pArtifact->GetHarpStrandOffsetHp());

            (*pTable)(row,1) << length.SetValue(offset);

            offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(span, gdr,
                                                                        girderData.Nstrands[pgsTypes::Harped], 
                                                                        HsoHp, abs_offset_hp);
            (*pTable)(row,2) << length.SetValue(offset);

            row++;
         }
      }

      (*pTable)(row,0) << RPT_FCI;
      (*pTable)(row,1) << stress.SetValue(pArtifact->GetReleaseStrength());
      (*pTable)(row,2) << stress.SetValue( girderData.Material.Fci );
      row++;

      (*pTable)(row,0) << RPT_FC;
      (*pTable)(row,1) << stress.SetValue(pArtifact->GetConcreteStrength());
      (*pTable)(row,2) << stress.SetValue(girderData.Material.Fc);
      row++;

      if ( options.doDesignSlabOffset && (pBridge->GetDeckType()!=pgsTypes::sdtNone) )
      {
         const CGirderTypes* pGirderTypes = pIBridgeDesc->GetBridgeDescription()->GetSpan(span)->GetGirderTypes();

         // the computed slab offset will be applied according to the current slab offset mode
         if ( pIBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge )
         {
            // slab offset is for the entire bridge... the start value contains this parameter
            (*pTable)(row,0) << _T("Slab Offset (\"A\" Dimension)");
            (*pTable)(row,1) << length.SetValue( pArtifact->GetSlabOffset(pgsTypes::metStart) );
            (*pTable)(row,2) << length.SetValue( pGirderTypes->GetSlabOffset(gdr,pgsTypes::metStart) );
            row++;
         }
         else
         {
            (*pTable)(row,0) << _T("Slab Offset at Start (\"A\" Dimension)");
            (*pTable)(row,1) << length.SetValue( pArtifact->GetSlabOffset(pgsTypes::metStart) );
            (*pTable)(row,2) << length.SetValue( pGirderTypes->GetSlabOffset(gdr,pgsTypes::metStart) );
            row++;

            (*pTable)(row,0) << _T("Slab Offset at End (\"A\" Dimension)");
            (*pTable)(row,1) << length.SetValue( pArtifact->GetSlabOffset(pgsTypes::metEnd) );
            (*pTable)(row,2) << length.SetValue( pGirderTypes->GetSlabOffset(gdr,pgsTypes::metEnd) );
            row++;
         }
      }

      if (options.doDesignLifting)
      {
         (*pTable)(row,0) << _T("Lifting Point Location (Left)");
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetLeftLiftingLocation() );
         (*pTable)(row,2) << distance.SetValue( pGirderLifting->GetLeftLiftingLoopLocation(span,gdr) );
         row++;

         (*pTable)(row,0) << _T("Lifting Point Location (Right)");
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetRightLiftingLocation() );
         (*pTable)(row,2) << distance.SetValue( pGirderLifting->GetRightLiftingLoopLocation(span,gdr) );
         row++;
      }

      if (options.doDesignHauling)
      {
         (*pTable)(row,0) << _T("Truck Support Location (Leading)");
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetLeadingOverhang() );
         (*pTable)(row,2) << distance.SetValue( pGirderHauling->GetLeadingOverhang(span,gdr) );
         row++;

         (*pTable)(row,0) << _T("Truck Support Location (Trailing)");
         (*pTable)(row,1) << distance.SetValue( pArtifact->GetTrailingOverhang() );
         (*pTable)(row,2) << distance.SetValue( pGirderHauling->GetTrailingOverhang(span,gdr) );
         row++;
      }
   }
   else
   {
      rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pParagraph;
      *pParagraph << _T("Flexure Design Not Requested")<<rptNewLine;
   }


   if (pArtifact->GetDoDesignShear())
   {

      rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pParagraph;
      *pParagraph << _T("Shear Design:");

      pParagraph = new rptParagraph();

      *pChapter << pParagraph;
      *pParagraph << Bold(_T("Primary Bars")) << rptNewLine;
      *pParagraph << _T("Proposed Design:") << rptNewLine;

      GET_IFACE2(pBroker, IBridge, pBridge);
      Float64 girder_length = pBridge->GetGirderLength(span,gdr);

     // stirrup design results
      ZoneIndexType nz = pArtifact->GetNumberOfStirrupZonesDesigned();
      const CShearData& rsdata = pArtifact->GetShearData();

      if (nz>0)
      {
         write_primary_shear_data(pParagraph, pDisplayUnits, girder_length, nz, rsdata);
      }
      else
      {
         *pParagraph << _T("No Zones Designed")<<rptNewLine;
      }

      // Current configuration
      *pParagraph << _T("Current Values:") << rptNewLine;

      GET_IFACE2(pBroker,IShear,pShear);
      CShearData shear_data = pShear->GetShearData(span, gdr);
      ZoneIndexType ncz = shear_data.ShearZones.size();

      write_primary_shear_data(pParagraph, pDisplayUnits, girder_length, ncz, shear_data);

      // Horiz interface bars
      *pParagraph <<rptNewLine<< Bold(_T("Additional Bars For Horizontal Interface Shear"))<<rptNewLine;
      *pParagraph << _T("Proposed Design:") << rptNewLine;
      write_horiz_shear_data(pParagraph, pDisplayUnits, girder_length, rsdata);

      *pParagraph << _T("Current Values:") << rptNewLine;
      write_horiz_shear_data(pParagraph, pDisplayUnits, girder_length, shear_data);

      // Additional Shear Reinforcement at Girder Ends
      *pParagraph <<rptNewLine<< Bold(_T("Additional Shear Reinforcement at Girder Ends"))<<rptNewLine;
      *pParagraph << _T("Proposed Design:") << rptNewLine;
      write_additional_shear_data(pParagraph, pDisplayUnits, girder_length, rsdata);

      *pParagraph << _T("Current Values:") << rptNewLine;
      write_additional_shear_data(pParagraph, pDisplayUnits, girder_length, shear_data);

      if(pArtifact->GetWasLongitudinalRebarForShearDesigned())
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

   // End up with some notes about Flexural Design
   if ( (pArtifact->GetDoDesignFlexure()!=dtNoDesign && pArtifact->GetOutcome()==pgsDesignArtifact::Success) ||
        pArtifact->DoDesignNotesExist())
   {
      rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pParagraph;
      *pParagraph << _T("Design Notes:") << rptNewLine;

      pParagraph = new rptParagraph();

      // Explicit notes created during design
      if (pArtifact->DoDesignNotesExist())
      {
         std::vector<pgsDesignArtifact::DesignNote> notes = pArtifact->GetDesignNotes();
         write_design_notes(pChapter, notes);
      }

      if (pArtifact->GetDoDesignFlexure()!=dtNoDesign && pArtifact->GetOutcome()==pgsDesignArtifact::Success)
      {
         // Notes from a successful flexural design
         GET_IFACE2(pBroker,IBridgeMaterialEx,pMaterial);
         GET_IFACE2(pBroker,ILimits2,pLimits);
         pgsTypes::ConcreteType concType = pMaterial->GetGdrConcreteType(span,gdr);
         Float64 max_girder_fci = pLimits->GetMaxGirderFci(concType);
         Float64 max_girder_fc = pLimits->GetMaxGirderFc(concType);
         if (pArtifact->GetReleaseStrength() > max_girder_fci)
         {
            *pParagraph <<color(Red)<< _T("Warning: The designed girder release strength exceeds the normal value of ")<<stress.SetValue(max_girder_fci)<<color(Black)<< rptNewLine;
         }

         if (pArtifact->GetConcreteStrength() > max_girder_fc)
         {
            *pParagraph <<color(Red)<< _T("Warning: The designed girder final concrete strength exceeds the normal value of ")<<stress.SetValue(max_girder_fc)<<color(Black)<< rptNewLine;
         }

         // Negative camber is not technically a spec check, but a warning
         GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
         std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_MIDSPAN);
         CHECK(vPoi.size()==1);
         pgsPointOfInterest poi = *vPoi.begin();

         GDRCONFIG config = pArtifact->GetGirderConfiguration();

         GET_IFACE2(pBroker,ICamber,pCamber);
         double excess_camber = pCamber->GetExcessCamber(poi,config,CREEP_MAXTIME);
         if ( excess_camber < 0 )
         {
            *pParagraph<<color(Red)<< _T("Warning:  Excess camber is negative, indicating a potential sag in the beam.")<<color(Black)<< rptNewLine;
         }

         *pParagraph << _T("Concrete release strength was controlled by ")<<pArtifact->GetReleaseDesignState().AsString() << rptNewLine;
         *pParagraph << _T("Concrete final strength was controlled by ")<<pArtifact->GetFinalDesignState().AsString() << rptNewLine;
         *pParagraph << rptNewLine;

         if ( options.doDesignSlabOffset && (pBridge->GetDeckType()!=pgsTypes::sdtNone) )
         {
            if ( pIBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge )
            {
               *pParagraph << _T("Slab Offset will be applied to the bridge") << rptNewLine;
            }
            else
            {
               *pParagraph << _T("Slab Offset will be applied to this girder") << rptNewLine;
            }
         }
      }

      *pChapter << pParagraph;
   }
}

void successful_design(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsDesignArtifact* pArtifact)
{
   rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pParagraph;

   *pParagraph << color(Green)
               << _T("The design for Span ") << LABEL_SPAN(pArtifact->GetSpan())
               << _T(" Girder ") << LABEL_GIRDER(pArtifact->GetGirder())
               << _T(" was successful.") 
               << color(Black)
               << rptNewLine;

   write_artifact_data(pBroker,span,gdr,pChapter,pDisplayUnits,pArtifact);
}

void failed_design(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const pgsDesignArtifact* pArtifact)
{
   rptParagraph* pParagraph;
   pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pParagraph;

   *pParagraph << color(Red)
               << _T("The design attempt for Span ") << LABEL_SPAN(pArtifact->GetSpan())
               << _T(" Girder ") << LABEL_GIRDER(pArtifact->GetGirder())
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
   write_artifact_data(pBroker,span,gdr,pChapter,pDisplayUnits,pArtifact);
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

   default:
      ATLASSERT(0);
   }
   return std::wstring();
}

void write_design_notes(rptChapter* pChapter, const std::vector<pgsDesignArtifact::DesignNote>& notes)
{
   rptParagraph* pParagraph;
   pParagraph = new rptParagraph();
   *pChapter << pParagraph;

   for(std::vector<pgsDesignArtifact::DesignNote>::const_iterator it = notes.begin(); it!=notes.end(); it++)
   {
      *pParagraph <<_T(" -  ") << GetDesignNoteString( *it ) <<rptNewLine;
   }
}


void multiple_girder_table(ColumnIndexType startIdx, ColumnIndexType endIdx,
                     IBroker* pBroker,std::vector<SpanGirderHashType>& girderList,rptChapter* pChapter,
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

   process_artifacts(startIdx, endIdx, girderList, pIArtifact,
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

      SpanGirderHashType hash = girderList[gdr_idx];
      SpanIndexType span;
      GirderIndexType gdr;
      UnhashSpanGirder(hash,&span,&gdr);

      row = 0;

      (*pTable)(row++,col) << _T("Span ") << span+1 <<rptNewLine<<_T("Girder ")<<LABEL_GIRDER(gdr);

      const pgsDesignArtifact* pArtifact = pArtifacts[idx++];

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

      GDRCONFIG config = pArtifact->GetGirderConfiguration();

      // jacking forces
      (*pTable)(row++,col) << force.SetValue(config.Pjack[pgsTypes::Straight]);

      if (is_harped)
         (*pTable)(row++,col) << force.SetValue(config.Pjack[pgsTypes::Harped]);

      if (is_temporary)
         (*pTable)(row++,col) << force.SetValue(config.Pjack[pgsTypes::Temporary]);

      if (is_harped)
      {
         if (pArtifact->GetNumHarpedStrands()>0)
         {
            double offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(span, gdr,
                                                                                pArtifact->GetNumHarpedStrands(), 
                                                                                hsoTOP2BOTTOM, 
                                                                                pArtifact->GetHarpStrandOffsetEnd());
            (*pTable)(row++,col) << length.SetValue(offset);

            offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(span, gdr,
                                                                        pArtifact->GetNumHarpedStrands(), 
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

void process_artifacts(ColumnIndexType startIdx, ColumnIndexType endIdx, std::vector<SpanGirderHashType>& girderList, IArtifact* pIArtifact,
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
      SpanGirderHashType hash = girderList[idx];
      SpanIndexType span;
      GirderIndexType gdr;
      UnhashSpanGirder(hash,&span,&gdr);

      pArtifacts[ia] = pIArtifact->GetDesignArtifact(span,gdr);

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

void write_primary_shear_data(rptParagraph* pParagraph, IEAFDisplayUnits* pDisplayUnits, Float64 girderLength, ZoneIndexType nz, const CShearData& rsdata)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), true );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
   scalar.SetTolerance(1.0e-6);

   lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
   CHECK(pool!=0);

   bool is_symm = rsdata.bAreZonesSymmetrical;

   // Primary bars
   bool is_stirrups(false);
   if (1 < nz)
   {
      is_stirrups = true;
   }
   else if (1 == nz)
   {
      if (rsdata.ShearZones[0].VertBarSize!=matRebar::bsNone)
      {
         is_stirrups = true;
      }
   }

   if (is_stirrups)
   {
      rptRcTable* pTables = pgsReportStyleHolder::CreateTableNoHeading(7,_T(""));
      *pParagraph << pTables;

      (*pTables)(0,0) << _T("Zone #");
      (*pTables)(0,1) << COLHDR(_T("Zone End"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTables)(0,2) << _T("Bar Size");
      (*pTables)(0,3) << COLHDR(_T("Spacing"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTables)(0,4) << _T("# Legs")<<rptNewLine<<_T("Vertical");
      (*pTables)(0,5) << _T("# Legs")<<rptNewLine<<_T("Into Deck");
      (*pTables)(0,6) << _T("Confinement")<<rptNewLine<<_T("Bar Size");

      Float64 max_zoneloc = is_symm ? girderLength/2.0 : girderLength;

      Float64 zone_end = 0.0;
      RowIndexType i=0;
      bool bdone(false);
      while (!bdone)
      {
         RowIndexType row = i+1;
         const CShearZoneData& rszdata = rsdata.ShearZones[i];
         zone_end += rszdata.ZoneLength;
         (*pTables)(row,0) << rszdata.ZoneNum;

         if ( nz <= i+1 || max_zoneloc <= zone_end)
         {
            bdone = true;
         }

         if (!bdone)
         {
            (*pTables)(row,1) << location.SetValue(zone_end);
         }
         else
         {
            if (is_symm)
               (*pTables)(row,1) << _T("Mid-Girder");
            else
               (*pTables)(row,1) << _T("End of Girder");
         }

         if (rszdata.VertBarSize!=matRebar::bsNone)
         {
            (*pTables)(row,2) << lrfdRebarPool::GetBarSize(rszdata.VertBarSize).c_str();
            (*pTables)(row,3) << length.SetValue(rszdata.BarSpacing);
            (*pTables)(row,4) << scalar.SetValue(rszdata.nVertBars);
            (*pTables)(row,5) << scalar.SetValue(rszdata.nHorzInterfaceBars);
            (*pTables)(row,6) << lrfdRebarPool::GetBarSize(rszdata.ConfinementBarSize).c_str();
         }
         else
         {
            (*pTables)(row,2) << _T("none");
            (*pTables)(row,3) << _T("--");
            (*pTables)(row,4) << _T("--");
            (*pTables)(row,5) << _T("--");
            (*pTables)(row,6) << _T("--");
         }

         i++;
      }
   }
   else
   {
      *pParagraph << _T("- No primary shear zones in girder")<<rptNewLine;
   }
}

void write_horiz_shear_data(rptParagraph* pParagraph, IEAFDisplayUnits* pDisplayUnits, Float64 girderLength,const CShearData& rsdata)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), true );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
   scalar.SetTolerance(1.0e-6);

   bool is_symm = rsdata.bAreZonesSymmetrical;

   // Additional horizontal interface bars
   ZoneIndexType nhz = rsdata.HorizontalInterfaceZones.size();
   bool is_hstirrups(false);
   if (1 < nhz)
   {
      is_hstirrups = true;
   }
   else if (1 == nhz)
   {
      if (rsdata.HorizontalInterfaceZones[0].BarSize!=matRebar::bsNone)
      {
         is_hstirrups = true;
      }
   }

   if(is_hstirrups)
   {
      rptRcTable* pTables = pgsReportStyleHolder::CreateTableNoHeading(5,_T(""));
      *pParagraph << pTables;

      (*pTables)(0,0) << _T("Zone #");
      (*pTables)(0,1) << COLHDR(_T("Zone End"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTables)(0,2) << _T("Bar Size");
      (*pTables)(0,3) << COLHDR(_T("Spacing"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTables)(0,4) << _T("# Legs");

      Float64 max_zoneloc = is_symm ? girderLength/2.0 : girderLength;

      Float64 zone_end = 0.0;
      RowIndexType i=0;
      bool bdone(false);
      while (!bdone)
      {
         RowIndexType row = i+1;
         const CHorizontalInterfaceZoneData& rhzdata = rsdata.HorizontalInterfaceZones[i];
         zone_end += rhzdata.ZoneLength;
         (*pTables)(row,0) << rhzdata.ZoneNum;

         if ( nhz <= i+1 || max_zoneloc <= zone_end )
         {
            bdone = true;
         }

         if (!bdone)
         {
            (*pTables)(row,1) << location.SetValue(zone_end);
         }
         else
         {
            if (is_symm)
               (*pTables)(row,1) << _T("Mid-Girder");
            else
               (*pTables)(row,1) << _T("End of Girder");
         }

         if (rhzdata.BarSize!=matRebar::bsNone)
         {
            (*pTables)(row,2) << lrfdRebarPool::GetBarSize(rhzdata.BarSize).c_str();
            (*pTables)(row,3) << length.SetValue(rhzdata.BarSpacing);
            (*pTables)(row,4) << scalar.SetValue(rhzdata.nBars);
         }
         else
         {
            (*pTables)(row,2) << _T("none");
            (*pTables)(row,3) << _T("--");
            (*pTables)(row,4) << _T("--");
         }

         i++;
      }
   }
   else
   {
      *pParagraph << _T("- No additional horizontal interface shear zones")<<rptNewLine;
   }
}

void write_additional_shear_data(rptParagraph* pParagraph, IEAFDisplayUnits* pDisplayUnits, Float64 girderLength, const CShearData& rsdata)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), true );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
   scalar.SetTolerance(1.0e-6);

   if (rsdata.SplittingBarSize!=matRebar::bsNone)
   {
      *pParagraph <<_T("- Splitting Reinforcement: Zone Length =")<< location.SetValue(rsdata.SplittingZoneLength)
                  <<_T(";  ")<<scalar.SetValue(rsdata.nSplittingBars)<<_T(" legs of ")
                  <<lrfdRebarPool::GetBarSize(rsdata.SplittingBarSize).c_str()<<_T(" bars at ")
                  <<length.SetValue(rsdata.SplittingBarSpacing)<<rptNewLine;
   }
   else
   {
      *pParagraph <<_T("- Splitting Reinforcement: None")<<rptNewLine;
   }

   if (rsdata.ConfinementBarSize!=matRebar::bsNone)
   {
      *pParagraph <<_T("- Confinement Reinforcement: Zone Length =")<< location.SetValue(rsdata.ConfinementZoneLength)<<_T("; ")
                  <<lrfdRebarPool::GetBarSize(rsdata.ConfinementBarSize).c_str()<<_T(" bars at ")
                  <<length.SetValue(rsdata.ConfinementBarSpacing)<<rptNewLine;
   }
   else
   {
      *pParagraph <<_T("- Confinement Reinforcement: None")<<rptNewLine;
   }
}