///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <Reporting\LoadingDetailsChapterBuilder.h>
#include <Reporting\UserDefinedLoadsChapterBuilder.h>

#include <PgsExt\LoadFactors.h>
#include <PgsExt\BridgeDescription.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// free functions
static bool IsSlabLoadUniform(const std::vector<SlabLoad>& slabLoads, pgsTypes::SupportedDeckType deckType);
static bool IsOverlayLoadUniform(const std::vector<OverlayLoad>& overlayLoads);
static bool IsShearKeyLoadUniform(const std::vector<ShearKeyLoad>& loads);
static bool IsConstructionLoadUniform(const std::vector<ConstructionLoad>& loads);

/****************************************************************************
CLASS
   CLoadingDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CLoadingDetailsChapterBuilder::CLoadingDetailsChapterBuilder(bool bDesign,bool bRating,bool bSelect):
CPGSuperChapterBuilder(bSelect),
m_bDesign(bDesign),
m_bRating(bRating),
m_bSimplifiedVersion(false)
{
}

CLoadingDetailsChapterBuilder::CLoadingDetailsChapterBuilder(bool SimplifiedVersion,bool bDesign,bool bRating,bool bSelect):
CPGSuperChapterBuilder(bSelect),
m_bDesign(bDesign),
m_bRating(bRating),
m_bSimplifiedVersion(SimplifiedVersion)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CLoadingDetailsChapterBuilder::GetName() const
{
   return TEXT("Loading Details");
}

rptChapter* CLoadingDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   USES_CONVERSION;

   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CGirderReportSpecification* pGdrRptSpec    = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   SpanIndexType span;
   GirderIndexType gdr;

   if ( pSGRptSpec )
   {
      pSGRptSpec->GetBroker(&pBroker);
      span = pSGRptSpec->GetSpan();
      gdr = pSGRptSpec->GetGirder();
   }
   else if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      span = ALL_SPANS;
      gdr = pGdrRptSpec->GetGirder();
   }
   else
   {
      span = ALL_SPANS;
      gdr  = ALL_GIRDERS;
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   bool bDesign = m_bDesign;
   bool bRating;
   
   if ( m_bRating )
   {
      bRating = true;
   }
   else
   {
      // include load rating results if we are always load rating
      bRating = pRatingSpec->AlwaysLoadRate();
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         loc,    pDisplayUnits->GetSpanLengthUnit(),     false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl,    pDisplayUnits->GetForcePerLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,         moment, pDisplayUnits->GetMomentUnit(),         false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,          force,  pDisplayUnits->GetGeneralForceUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         dim,    pDisplayUnits->GetComponentDimUnit(),   false );

   rptParagraph* pPara;
   rptRcTable* p_table;

   bool one_girder_has_shear_key = false;

   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   SpanIndexType firstSpanIdx = (span == ALL_SPANS ? 0 : span);
   SpanIndexType lastSpanIdx  = (span == ALL_SPANS ? nSpans : firstSpanIdx+1);
   for ( SpanIndexType spanIdx = firstSpanIdx; spanIdx < lastSpanIdx; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType firstGirderIdx = min(nGirders-1,(gdr == ALL_GIRDERS ? 0 : gdr));
      GirderIndexType lastGirderIdx  = min(nGirders,  (gdr == ALL_GIRDERS ? nGirders : firstGirderIdx + 1));
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx < lastGirderIdx; gdrIdx++ )
      {
         if ( (span == ALL_SPANS || gdr == ALL_GIRDERS) && !m_bSimplifiedVersion )
         {
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;
            std::ostringstream os;
            os << "Span " << LABEL_SPAN(spanIdx) << " Girder " << LABEL_GIRDER(gdrIdx);
            pPara->SetName( os.str().c_str() );
            (*pPara) << pPara->GetName() << rptNewLine;
         }

         // uniform loads
         pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter << pPara;
         *pPara << "Uniform Loads Applied Along the Entire Girder" << rptNewLine;

         pPara = new rptParagraph;
         *pChapter << pPara;

         p_table = pgsReportStyleHolder::CreateDefaultTable(2,"");
         *pPara << p_table << rptNewLine;

         p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
         p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

         (*p_table)(0,0) << "Load Type";
         (*p_table)(0,1) << COLHDR("w",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

         GET_IFACE2(pBroker,IProductLoads,pProdLoads);

         RowIndexType row = p_table->GetNumberOfHeaderRows();

         std::vector<GirderLoad> gdrLoad;
         std::vector<DiaphragmLoad> diaLoad;
         pProdLoads->GetGirderSelfWeightLoad(spanIdx,gdrIdx,&gdrLoad,&diaLoad);
         bool bUniformGirderDeadLoad = (gdrLoad.size() == 1 && IsEqual(gdrLoad[0].wStart,gdrLoad[0].wEnd) ? true : false);
         if ( bUniformGirderDeadLoad )
         {
            // girder load is uniform
            (*p_table)(row,0) << "Girder";
            (*p_table)(row++,1) << fpl.SetValue(-gdrLoad[0].wStart);
         }

         if ( pProdLoads->HasSidewalkLoad(spanIdx,gdrIdx) )
         {
            (*p_table)(row,0) << "Sidewalk";
            (*p_table)(row++,1) << fpl.SetValue(-pProdLoads->GetSidewalkLoad(spanIdx,gdrIdx));
         }

         Float64 tb_load = pProdLoads->GetTrafficBarrierLoad(spanIdx,gdrIdx);
         if (tb_load!=0.0)
         {
            (*p_table)(row,0) << "Traffic Barrier";
            (*p_table)(row++,1) << fpl.SetValue(-tb_load);
         }

         if ( pProdLoads->HasPedestrianLoad(spanIdx,gdrIdx) )
         {
            (*p_table)(row,0) << "Pedestrian Live Load";
            (*p_table)(row++,1) << fpl.SetValue(pProdLoads->GetPedestrianLoad(spanIdx,gdrIdx));
         }

         if ( !bUniformGirderDeadLoad )
         {
            p_table = pgsReportStyleHolder::CreateDefaultTable(4,"Girder Self-Weight");
            *pPara << rptNewLine << p_table << rptNewLine;

            (*p_table)(0,0) << COLHDR("Load Start,"<<rptNewLine<<"From Left End of Girder",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,1) << COLHDR("Load End,"<<rptNewLine<<"From Left End of Girder",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,2) << COLHDR("Start Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            (*p_table)(0,3) << COLHDR("End Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

            row = p_table->GetNumberOfHeaderRows();
            std::vector<GirderLoad>::iterator iter;
            for ( iter = gdrLoad.begin(); iter != gdrLoad.end(); iter++ )
            {
               GirderLoad& load = *iter;

               (*p_table)(row,0) << loc.SetValue(load.StartLoc);
               (*p_table)(row,1) << loc.SetValue(load.EndLoc);
               (*p_table)(row,2) << fpl.SetValue(-load.wStart);
               (*p_table)(row,3) << fpl.SetValue(-load.wEnd);
               row++;
            }
         }

         // slab loads between supports

         Float64 end_size = pBridge->GetGirderStartConnectionLength( spanIdx, gdrIdx );

         if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
         {
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;
            *pPara<< "Slab Load Applied Between Bearings"<<rptNewLine;
            
            pgsTypes::SupportedDeckType deck_type = pBridge->GetDeckType();

            std::vector<SlabLoad> slab_loads;
            pProdLoads->GetMainSpanSlabLoad(spanIdx, gdrIdx, &slab_loads);

            bool is_uniform = IsSlabLoadUniform(slab_loads, deck_type);

            // Don't eject haunch note unless we have a haunch
            bool do_report_haunch = true;
            rptParagraph* pNotePara = new rptParagraph;
            *pChapter << pNotePara;

            if (is_uniform)
            {
               *pNotePara << "Slab Load is uniform along entire girder length.";
            }
            else
            {
               *pNotePara << "Slab Load is approximated with Linear Load Segments applied along the length of the girder";
            }

            pPara = new rptParagraph;
            *pChapter << pPara;

            if ( deck_type == pgsTypes::sdtCompositeSIP )
            {
               if (is_uniform)
               {
                  p_table = pgsReportStyleHolder::CreateDefaultTable(2,"");
                  *pPara << p_table << rptNewLine;
                  p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
                  p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
                  (*p_table)(0,0) << "Load Type";
                  (*p_table)(0,1) << COLHDR("w",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
                  RowIndexType row = p_table->GetNumberOfHeaderRows();

                  const SlabLoad& slab_load = *(slab_loads.begin());

                  (*p_table)(row,0) << "Panel Weight";
                  (*p_table)(row++,1) << fpl.SetValue(-slab_load.PanelLoad);
                  (*p_table)(row,0) << "Cast Slab Weight";
                  (*p_table)(row++,1) << fpl.SetValue(-slab_load.MainSlabLoad);
                  (*p_table)(row,0) << "Haunch Weight";
                  (*p_table)(row++,1) << fpl.SetValue(-slab_load.PadLoad);
                  (*p_table)(row,0) << "Total Slab Weight";
                  (*p_table)(row++,1) << fpl.SetValue(-slab_load.MainSlabLoad-slab_load.PadLoad);
               }
               else
               {
                  p_table = pgsReportStyleHolder::CreateDefaultTable(5,"");
                  *pPara << p_table;

                  (*p_table)(0,0) << COLHDR("Location"<<rptNewLine<<"From Left Bearing",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
                  (*p_table)(0,1) << COLHDR("Panel Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
                  (*p_table)(0,2) << COLHDR("Cast Slab Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
                  (*p_table)(0,3) << COLHDR("Haunch Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
                  (*p_table)(0,4) << COLHDR("Total Slab Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

                  RowIndexType row = 1;
                  for ( std::vector<SlabLoad>::iterator i = slab_loads.begin(); i != slab_loads.end(); i++ )
                  {
                     SlabLoad& slab_load = *i;

                     Float64 location  = slab_load.Loc - end_size;
                     Float64 main_load  = slab_load.MainSlabLoad;
                     Float64 panel_load = slab_load.PanelLoad;
                     Float64 pad_load   = slab_load.PadLoad;
                     (*p_table)(row,0) << loc.SetValue(location);
                     (*p_table)(row,1) << fpl.SetValue(-panel_load);
                     (*p_table)(row,2) << fpl.SetValue(-main_load);
                     (*p_table)(row,3) << fpl.SetValue(-pad_load);
                     (*p_table)(row,4) << fpl.SetValue(-(main_load+pad_load));

                     row++;
                  }
               }
            }
            else
            {
               if (is_uniform)
               {
                  p_table = pgsReportStyleHolder::CreateDefaultTable(2,"");
                  *pPara << p_table << rptNewLine;
                  p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
                  p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
                  (*p_table)(0,0) << "Load Type";
                  (*p_table)(0,1) << COLHDR("w",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
                  RowIndexType row = p_table->GetNumberOfHeaderRows();

                  const SlabLoad& slab_load = *(slab_loads.begin());

                  // Don't report zero slab load in simplified version
                  do_report_haunch = !(m_bSimplifiedVersion && IsZero(slab_load.PadLoad));

                  if (do_report_haunch)
                  {
                     (*p_table)(row,0) << "Main Slab Weight";
                     (*p_table)(row++,1) << fpl.SetValue(-slab_load.MainSlabLoad);
                     (*p_table)(row,0) << "Haunch Weight";
                     (*p_table)(row++,1) << fpl.SetValue(-slab_load.PadLoad);
                  }
                  (*p_table)(row,0) << "Total Slab Weight";
                  (*p_table)(row++,1) << fpl.SetValue(-slab_load.MainSlabLoad-slab_load.PadLoad);
               }
               else
               {
                  p_table = pgsReportStyleHolder::CreateDefaultTable(4,"");
                  *pPara << p_table;

                  (*p_table)(0,0) << COLHDR("Location"<<rptNewLine<<"From Left Bearing",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
                  (*p_table)(0,1) << COLHDR("Main Slab Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
                  (*p_table)(0,2) << COLHDR("Haunch Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
                  (*p_table)(0,3) << COLHDR("Total Slab Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

                  RowIndexType row = 1;
                  for (std::vector<SlabLoad>::iterator i = slab_loads.begin(); i!=slab_loads.end(); i++)
                  {
                     SlabLoad& slab_load = *i;
                     Float64 location = slab_load.Loc-end_size;
                     Float64 main_load = slab_load.MainSlabLoad;
                     Float64 pad_load  = slab_load.PadLoad;
                     (*p_table)(row,0) << loc.SetValue(location);
                     (*p_table)(row,1) << fpl.SetValue(-main_load);
                     (*p_table)(row,2) << fpl.SetValue(-pad_load);
                     (*p_table)(row,3) << fpl.SetValue(-(main_load+pad_load));
                     row++;
                  }
               }
            } // end if ( pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP )

            if(do_report_haunch)            
               *pNotePara <<rptNewLine<< "Haunch weight includes effects of roadway geometry but does not include a reduction for camber";

            // the rest of the content is for the non-simplified version (full boat)
            if (!m_bSimplifiedVersion)
            {
               // slab cantilever loads
               pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
               *pChapter << pPara;
               *pPara<< "Slab Cantilever Loads"<<rptNewLine;
            
               pPara = new rptParagraph;
               *pChapter << pPara;

               p_table = pgsReportStyleHolder::CreateDefaultTable(3,"");
               *pPara << p_table;

               (*p_table)(0,0) << "Location";
               (*p_table)(0,1) << COLHDR("Point Load",rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
               (*p_table)(0,2) << COLHDR("Point Moment",rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

               Float64 P1, P2, M1, M2;
               pProdLoads->GetCantileverSlabLoad(spanIdx, gdrIdx, &P1, &M1, &P2, &M2);
               (*p_table)(1,0) << "Left Bearing";
               (*p_table)(1,1) << force.SetValue(-P1);
               (*p_table)(1,2) << moment.SetValue(M1);
            
               (*p_table)(2,0) << "Right Bearing";
               (*p_table)(2,1) << force.SetValue(-P2);
               (*p_table)(2,2) << moment.SetValue(M2);

               p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
               p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
            }
         } // end if ( pBridge->GetDeckType() != pgsTypes::sdtNone )

         // overlay laod
         bool bReportOverlay = pBridge->HasOverlay();
         if ( bRating && pBridge->IsFutureOverlay() ) // don't report future overlay load for ratings
            bReportOverlay = false;

         if ( bReportOverlay )
         {
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;
            *pPara << "Overlay" << rptNewLine;
            pPara = new rptParagraph;
            *pChapter << pPara;

            GET_IFACE2(pBroker, ISpecification, pSpec );
            pgsTypes::OverlayLoadDistributionType olayd = pSpec->GetOverlayLoadDistributionType();

            std::vector<OverlayLoad> overlay_loads;
            pProdLoads->GetOverlayLoad(spanIdx, gdrIdx, &overlay_loads);

            bool is_uniform = IsOverlayLoadUniform(overlay_loads);

            if (is_uniform)
            {
               *pPara<<"Overlay load is uniform along entire girder length."<<rptNewLine;

               p_table = pgsReportStyleHolder::CreateDefaultTable(2,"");
               *pPara << p_table << rptNewLine;
               p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
               p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
               (*p_table)(0,0) << "Load Type";
               (*p_table)(0,1) << COLHDR("w",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
               RowIndexType row = p_table->GetNumberOfHeaderRows();

               const OverlayLoad& ovl_load = *(overlay_loads.begin());

               (*p_table)(row,0) << "Overlay Weight";
               (*p_table)(row++,1) << fpl.SetValue(-ovl_load.StartLoad);
            }
            else
            {
               p_table = pgsReportStyleHolder::CreateDefaultTable(6,"");
               *pPara << p_table;

               (*p_table)(0,0) << COLHDR("Load Start,"<<rptNewLine<<"From Left Bearing",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
               (*p_table)(0,1) << COLHDR("Load End,"  <<rptNewLine<<"From Left Bearing",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

               if (olayd==pgsTypes::olDistributeTributaryWidth)
               {
                  (*p_table)(0,2) << COLHDR("Start " << Sub2("W","trib"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
                  (*p_table)(0,3) << COLHDR("End "   << Sub2("W","trib"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
               }
               else
               {
                  (*p_table)(0,2) << COLHDR("Start " << Sub2("W","cc"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
                  (*p_table)(0,3) << COLHDR("End "   << Sub2("W","cc"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
               }

               (*p_table)(0,4) << COLHDR("Start Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
               (*p_table)(0,5) << COLHDR("End Weight",  rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

               RowIndexType row = 1;
               for (std::vector<OverlayLoad>::iterator i = overlay_loads.begin(); i != overlay_loads.end(); i++)
               {
                  OverlayLoad& load = *i;
                  Float64 x1 = load.StartLoc - end_size;
                  Float64 x2 = load.EndLoc   - end_size;
                  Float64 Wcc1 = load.StartWcc;
                  Float64 Wcc2 = load.EndWcc;
                  Float64 w1   = load.StartLoad;
                  Float64 w2   = load.EndLoad;

                  (*p_table)(row,0) << loc.SetValue(x1);
                  (*p_table)(row,1) << loc.SetValue(x2);
                  (*p_table)(row,2) << loc.SetValue(Wcc1);
                  (*p_table)(row,3) << loc.SetValue(Wcc2);
                  (*p_table)(row,4) << fpl.SetValue(-w1);
                  (*p_table)(row,5) << fpl.SetValue(-w2);

                  row++;
               }
            }

            pPara = new rptParagraph;
            *pChapter << pPara;
            if (olayd==pgsTypes::olDistributeTributaryWidth)
            {
               *pPara << "Overlay load is distributed using tributary width."<< rptNewLine;
            }
            else
            {
               *pPara << "Overlay load is distributed uniformly among all girders per LRFD 4.6.2.2.1"<< rptNewLine;
               *pPara << Sub2("W","cc") << " is the curb to curb width"<< rptNewLine;
            }
         } // end if overlay

         // construction load
         GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoads);
         if ( !IsZero(pUserLoads->GetConstructionLoad()) )
         {
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;
            *pPara << "Construction" << rptNewLine;
            pPara = new rptParagraph;
            *pChapter << pPara;

            std::vector<ConstructionLoad> construction_loads;
            pProdLoads->GetConstructionLoad(spanIdx, gdrIdx, &construction_loads);

            bool is_uniform = IsConstructionLoadUniform(construction_loads);

            if (is_uniform)
            {
               *pPara<<"Construction load is uniform along entire girder length."<<rptNewLine;

               p_table = pgsReportStyleHolder::CreateDefaultTable(2,"");
               *pPara << p_table << rptNewLine;
               p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
               p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
               (*p_table)(0,0) << "Load Type";
               (*p_table)(0,1) << COLHDR("w",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
               RowIndexType row = p_table->GetNumberOfHeaderRows();
         
               const ConstructionLoad& cnst_load = *(construction_loads.begin());

               (*p_table)(row,0) << "Construction Weight";
               (*p_table)(row++,1) << fpl.SetValue(-cnst_load.StartLoad);
            }
            else
            {
            p_table = pgsReportStyleHolder::CreateDefaultTable(6,"");
            *pPara << p_table;

            (*p_table)(0,0) << COLHDR("Load Start,"<<rptNewLine<<"From Left Bearing",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,1) << COLHDR("Load End,"<<rptNewLine<<"From Left Bearing",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

            (*p_table)(0,2) << COLHDR("Start " << Sub2("W","cc"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,3) << COLHDR("End " << Sub2("W","cc"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

            (*p_table)(0,4) << COLHDR("Start Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            (*p_table)(0,5) << COLHDR("End Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );


            RowIndexType row = 1;
            for (std::vector<ConstructionLoad>::iterator i = construction_loads.begin(); i != construction_loads.end(); i++)
            {
               ConstructionLoad& load = *i;
               Float64 x1 = load.StartLoc - end_size;
               Float64 x2 = load.EndLoc   - end_size;
               Float64 Wcc1 = load.StartWcc;
               Float64 Wcc2 = load.EndWcc;
               Float64 w1   = load.StartLoad;
               Float64 w2   = load.EndLoad;

               (*p_table)(row,0) << loc.SetValue(x1);
               (*p_table)(row,1) << loc.SetValue(x2);
               (*p_table)(row,2) << loc.SetValue(Wcc1);
               (*p_table)(row,3) << loc.SetValue(Wcc2);
               (*p_table)(row,4) << fpl.SetValue(-w1);
               (*p_table)(row,5) << fpl.SetValue(-w2);

               row++;
            }
         }
      }

         // Shear key loads
         GET_IFACE2(pBroker,IGirder,pGirder);
         GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
         const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
         pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

         bool has_shear_key = pGirder->HasShearKey(spanIdx, gdrIdx, spacingType);
         if ( has_shear_key )
         {
            one_girder_has_shear_key = true;
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;
            *pPara<< "Shear Key Load"<<rptNewLine;
            
            pPara = new rptParagraph;
            *pChapter << pPara;

            std::vector<ShearKeyLoad> loads;
            pProdLoads->GetShearKeyLoad(spanIdx, gdrIdx, &loads);

            bool is_uniform = IsShearKeyLoadUniform(loads);
            if (is_uniform)
            {
               *pPara << "Shear Key Load is uniform along entire girder length" << rptNewLine;

               p_table = pgsReportStyleHolder::CreateDefaultTable(2,"");
               *pPara << p_table << rptNewLine;
               p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
               p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
               (*p_table)(0,0) << "Load Type";
               (*p_table)(0,1) << COLHDR("w",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
               RowIndexType row = p_table->GetNumberOfHeaderRows();

               const ShearKeyLoad& load = *(loads.begin());

               (*p_table)(row,0) << "Load Within Girder Envelope";
               (*p_table)(row++,1) << fpl.SetValue(-load.UniformLoad);

               (*p_table)(row,0) << "Load Within Joint";
               (*p_table)(row++,1) << fpl.SetValue(-load.StartJointLoad);

               (*p_table)(row,0) << "Total Shear Key Weight";
               (*p_table)(row++,1) << fpl.SetValue(-load.StartJointLoad - load.UniformLoad);
            }
            else
            {
               *pPara << "Shear Key Load is approximated with Linear Load Segments applied along the length of the girder" << rptNewLine;

               p_table = pgsReportStyleHolder::CreateDefaultTable(4,"");
               *pPara << p_table;

               (*p_table)(0,0) << COLHDR("Location"<<rptNewLine<<"From Left Bearing",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
               (*p_table)(0,1) << COLHDR("Load Within"<<rptNewLine<<"Girder Envelope",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
               (*p_table)(0,2) << COLHDR("Load Within"<<rptNewLine<<"Joint",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
               (*p_table)(0,3) << COLHDR("Total Shear"<<rptNewLine<<"Key Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

               RowIndexType row = 1;
               for ( std::vector<ShearKeyLoad>::iterator i = loads.begin(); i != loads.end(); i++ )
               {
                 const ShearKeyLoad& sk_load = *i;

                  if (row==1)
                  {
                     Float64 location  = sk_load.StartLoc - end_size;
                     Float64 unif_load  = sk_load.UniformLoad;
                     Float64 joint_load = sk_load.StartJointLoad;
                     Float64 total_load = unif_load + joint_load;
                     (*p_table)(row,0) << loc.SetValue(location);
                     (*p_table)(row,1) << fpl.SetValue(-unif_load);
                     (*p_table)(row,2) << fpl.SetValue(-joint_load);
                     (*p_table)(row,3) << fpl.SetValue(-total_load);
                     row++;
                  }

                  Float64 location  = sk_load.EndLoc - end_size;
                  Float64 unif_load  = sk_load.UniformLoad;
                  Float64 joint_load = sk_load.EndJointLoad;
                  Float64 total_load = unif_load + joint_load;
                  (*p_table)(row,0) << loc.SetValue(location);
                  (*p_table)(row,1) << fpl.SetValue(-unif_load);
                  (*p_table)(row,2) << fpl.SetValue(-joint_load);
                  (*p_table)(row,3) << fpl.SetValue(-total_load);

                  row++;
               }
            }
         }

         if ( !m_bSimplifiedVersion )
         {
            // end diaphragm loads
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;
            *pPara<< "End Diaphragm Loads"<<rptNewLine;
         
            pPara = new rptParagraph;
            *pChapter << pPara;

            p_table = pgsReportStyleHolder::CreateDefaultTable(3,"");
            *pPara << p_table;

            (*p_table)(0,0) << "Location";
            (*p_table)(0,1) << COLHDR("Point Load",rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
            (*p_table)(0,2) << COLHDR("Point Moment",rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

            Float64 P1, P2, M1, M2;
            pProdLoads->GetEndDiaphragmLoads(spanIdx, gdrIdx, &P1, &M1, &P2, &M2);
            (*p_table)(1,0) << "Left Bearing";
            (*p_table)(1,1) << force.SetValue(-P1);
            (*p_table)(1,2) << moment.SetValue(M1);
         
            (*p_table)(2,0) << "Right Bearing";
            (*p_table)(2,1) << force.SetValue(-P2);
            (*p_table)(2,2) << moment.SetValue(M2);

            p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
            p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

            // diaphragm loads between supports
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;
            *pPara<< "Intermediate Diaphragm Loads Constructed in Casting Yard"<<rptNewLine;
         
            pPara = new rptParagraph;
            *pChapter << pPara;

            std::vector<DiaphragmLoad> diap_loads;
            pProdLoads->GetIntermediateDiaphragmLoads(pgsTypes::CastingYard, spanIdx, gdrIdx, &diap_loads);
            std::sort(diap_loads.begin(),diap_loads.end());

            if (diap_loads.size() == 0)
            {
               *pPara<<"No Intermediate Diaphragms Present"<<rptNewLine;
            }
            else
            {
               p_table = pgsReportStyleHolder::CreateDefaultTable(5,"");
               *pPara << p_table;

               (*p_table)(0,0) << COLHDR("Load Location,"<<rptNewLine<<"From Left End of Girder",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
               (*p_table)(0,1) << COLHDR("H",rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
               (*p_table)(0,2) << COLHDR("W",rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
               (*p_table)(0,3) << COLHDR("T",rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
               (*p_table)(0,4) << COLHDR("Load",rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );

               int row=1;

               std::vector<IntermedateDiaphragm> diaphragms   = pBridge->GetIntermediateDiaphragms(pgsTypes::CastingYard,spanIdx,gdrIdx);
               std::vector<IntermedateDiaphragm>::iterator iter = diaphragms.begin();

               for (std::vector<DiaphragmLoad>::iterator id = diap_loads.begin(); id!=diap_loads.end(); id++, iter++)
               {
                  DiaphragmLoad& rload = *id;
                  IntermedateDiaphragm& dia = *iter;

                  (*p_table)(row,0) << loc.SetValue(rload.Loc);
                  if ( dia.m_bCompute )
                  {
                     (*p_table)(row,1) << dim.SetValue(dia.H);
                     (*p_table)(row,2) << dim.SetValue(dia.W);
                     (*p_table)(row,3) << dim.SetValue(dia.T);
                  }
                  else
                  {
                     (*p_table)(row,1) << "";
                     (*p_table)(row,2) << "";
                     (*p_table)(row,3) << "";
                  }
                  (*p_table)(row,4) << force.SetValue(-rload.Load);
                  row++;
               }
            }

            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;
            *pPara<< "Intermediate Diaphragm Loads Constructed at Bridge Site"<<rptNewLine;
         
            pPara = new rptParagraph;
            *pChapter << pPara;

            diap_loads.clear();
            pProdLoads->GetIntermediateDiaphragmLoads(pgsTypes::BridgeSite1, spanIdx, gdrIdx, &diap_loads);
            std::sort(diap_loads.begin(),diap_loads.end());

            if (diap_loads.size() == 0)
            {
               *pPara<<"No Intermediate Diaphragms Present"<<rptNewLine;
            }
            else
            {
               p_table = pgsReportStyleHolder::CreateDefaultTable(5,"");
               *pPara << p_table;

               (*p_table)(0,0) << COLHDR("Load Location,"<<rptNewLine<<"From Left Bearing",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
               (*p_table)(0,1) << COLHDR("H",rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
               (*p_table)(0,2) << COLHDR("W",rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
               (*p_table)(0,3) << COLHDR("T",rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
               (*p_table)(0,4) << COLHDR("Load",rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );

               std::vector<IntermedateDiaphragm> diaphragms   = pBridge->GetIntermediateDiaphragms(pgsTypes::BridgeSite1,spanIdx,gdrIdx);
               std::vector<IntermedateDiaphragm>::iterator iter = diaphragms.begin();

               RowIndexType row = 1;
               for (std::vector<DiaphragmLoad>::iterator id = diap_loads.begin(); id!=diap_loads.end(); id++, iter++)
               {
                  DiaphragmLoad& rload = *id;
                  IntermedateDiaphragm& dia = *iter;

                  (*p_table)(row,0) << loc.SetValue(rload.Loc);

                  if ( dia.m_bCompute )
                  {
                     (*p_table)(row,1) << dim.SetValue(dia.H);
                     (*p_table)(row,2) << dim.SetValue(dia.W);
                     (*p_table)(row,3) << dim.SetValue(dia.T);
                  }
                  else
                  {
                     (*p_table)(row,1) << "";
                     (*p_table)(row,2) << "";
                     (*p_table)(row,3) << "";
                  }

                  (*p_table)(row,4) << force.SetValue(-rload.Load);
                  row++;
               }
            }

            // User Defined Loads
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;
            *pPara<< "User Defined Loads"<<rptNewLine;
            pPara = CUserDefinedLoadsChapterBuilder::CreatePointLoadTable(pBroker, spanIdx, gdrIdx, pDisplayUnits, level, m_bSimplifiedVersion);
            *pChapter << pPara;
            pPara = CUserDefinedLoadsChapterBuilder::CreateDistributedLoadTable(pBroker, spanIdx, gdrIdx, pDisplayUnits, level, m_bSimplifiedVersion);
            *pChapter << pPara;
            pPara = CUserDefinedLoadsChapterBuilder::CreateMomentLoadTable(pBroker, spanIdx, gdrIdx, pDisplayUnits, level, m_bSimplifiedVersion);
            *pChapter << pPara;
         } // !simplified version
      } // gdrIdx
   } // spanIdx


   // live loads
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   if (!m_bSimplifiedVersion)
   {
   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<< "Live Loads"<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   if ( bDesign )
   {
      std::vector<std::string> designLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltDesign);
      if ( designLiveLoads.empty() )
      {
         *pPara<<"No live loads were applied to the design (Service and Strength I) limit states"<< rptNewLine;
      }
      else
      {
         *pPara<<"The following live loads were applied to the design (Service and Strength I) limit states:"<< rptNewLine;

         std::vector<std::string>::iterator iter;
         for (iter = designLiveLoads.begin(); iter != designLiveLoads.end(); iter++)
         {
            std::string& load_name = *iter;
            *pPara << load_name << rptNewLine;
         }
      }
      *pPara << rptNewLine;

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         std::vector<std::string> fatigueLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltFatigue);
         if ( fatigueLiveLoads.empty() )
         {
            *pPara<<"No live loads were applied to the fatigue (Fatigue I) limit states"<< rptNewLine;
         }
         else
         {
            *pPara<<"The following live loads were applied to the fatigue (Fatigue I) limit states:"<< rptNewLine;

            std::vector<std::string>::iterator iter;
            for (iter = fatigueLiveLoads.begin(); iter != fatigueLiveLoads.end(); iter++)
            {
               std::string& load_name = *iter;
               *pPara << load_name << rptNewLine;
            }
         }

         *pPara << rptNewLine;
      }

      if ( bPermit )
      {
         std::vector<std::string> permitLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermit);
         if ( permitLiveLoads.empty() )
         {
            *pPara<<"No live loads were applied to the permit (Strength II) limit states"<< rptNewLine;
         }
         else
         {
            *pPara<<"The following live loads were applied to the permit (Strength II) limit states:"<< rptNewLine;

            std::vector<std::string>::iterator iter;
            for (iter = permitLiveLoads.begin(); iter != permitLiveLoads.end(); iter++)
            {
               std::string& load_name = *iter;
               *pPara << load_name << rptNewLine;
            }
         }
      }
   }

   if ( bRating )
   {
      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
      {
         std::vector<std::string> designLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltDesign);
         if ( designLiveLoads.empty() )
         {
            *pPara<<"No live loads were applied to the design load rating (Service III and Strength I) limit states"<< rptNewLine;
         }
         else
         {
            *pPara<<"The following live loads were applied to the design load rating (Service III and Strength I) limit states:"<< rptNewLine;

            std::vector<std::string>::iterator iter;
            for (iter = designLiveLoads.begin(); iter != designLiveLoads.end(); iter++)
            {
               std::string& load_name = *iter;
               *pPara << load_name << rptNewLine;
            }
         }
         *pPara << rptNewLine;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         std::vector<std::string> legalRoutineLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Routine);
         if ( legalRoutineLiveLoads.empty() )
         {
            *pPara<<"No live loads were applied to the legal load rating, routine commercial traffic (Service III and Strength I) limit states"<< rptNewLine;
         }
         else
         {
            *pPara<<"The following live loads were applied to the legal load rating, routine commercial traffic (Service III and Strength I) limit states:"<< rptNewLine;

            std::vector<std::string>::iterator iter;
            for (iter = legalRoutineLiveLoads.begin(); iter != legalRoutineLiveLoads.end(); iter++)
            {
               std::string& load_name = *iter;
               *pPara << load_name << rptNewLine;
            }
         }
         *pPara << rptNewLine;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         std::vector<std::string> legalSpecialLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Special);
         if ( legalSpecialLiveLoads.empty() )
         {
            *pPara<<"No live loads were applied to the legal load rating, specialized hauling vehicles (Service III and Strength I) limit states"<< rptNewLine;
         }
         else
         {
            *pPara<<"The following live loads were applied to the legal load rating, specialized hauling vehicles (Service III and Strength I) limit states:"<< rptNewLine;

            std::vector<std::string>::iterator iter;
            for (iter = legalSpecialLiveLoads.begin(); iter != legalSpecialLiveLoads.end(); iter++)
            {
               std::string& load_name = *iter;
               *pPara << load_name << rptNewLine;
            }
         }
         *pPara << rptNewLine;
      }


      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         std::vector<std::string> permitRoutineLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Routine);
         if ( permitRoutineLiveLoads.empty() )
         {
            *pPara<<"No live loads were applied to the permit load rating, routine/annual permit (Service I and Strength II) limit states"<< rptNewLine;
         }
         else
         {
            *pPara<<"The following live loads were applied to the permit load rating, routine/annual permit (Service I and Strength II) limit states:"<< rptNewLine;

            std::vector<std::string>::iterator iter;
            for (iter = permitRoutineLiveLoads.begin(); iter != permitRoutineLiveLoads.end(); iter++)
            {
               std::string& load_name = *iter;
               *pPara << load_name << rptNewLine;
            }
         }
         *pPara << rptNewLine;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         std::vector<std::string> permitSpecialLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Special);
         if ( permitSpecialLiveLoads.empty() )
         {
            *pPara<<"No live loads were applied to the permit load rating, special/limited crossing permit (Service I and Strength II) limit states"<< rptNewLine;
         }
         else
         {
            *pPara<<"The following live loads were applied to the permit load rating, special/limited crossing permit (Service I and Strength II) limit states:"<< rptNewLine;

            std::vector<std::string>::iterator iter;
            for (iter = permitSpecialLiveLoads.begin(); iter != permitSpecialLiveLoads.end(); iter++)
            {
               std::string& load_name = *iter;
               *pPara << load_name << rptNewLine;
            }
         }
         *pPara << rptNewLine;
      }
   }
   }

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
   scalar.SetTolerance(1.0e-6);

   // LRFD Limit States
   GET_IFACE2(pBroker,ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();
   RowIndexType row = 0;

   if (!m_bSimplifiedVersion)
   {
   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<< "Limit States"<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   p_table = pgsReportStyleHolder::CreateDefaultTable(2,"");
   *pPara << p_table;

   p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   p_table->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   p_table->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   (*p_table)(row,0) << "Stage";
   (*p_table)(row,1) << "Load Case";
   row++;

   std::string strDC;
   if (one_girder_has_shear_key)
   {
      strDC = "DC = Girder + Diaphragms + Shear Key + Construction + Slab";
   }
   else
   {
      strDC = "DC = Girder + Diaphragms + Construction + Slab";
   }

   if ( bDesign )
   {
      (*p_table)(row,0) << "Casting Yard";
      (*p_table)(row,1) << "DC = Girder";
      row++;


      (*p_table)(row,0) << "Deck and Diaphragm Placement (Bridge Site 1)";
      (*p_table)(row,1) << strDC;
      row++;

      (*p_table)(row,0) << "Superimposed Dead Loads (Bridge Site 2)";
      (*p_table)(row,1) << strDC<<" + Traffic Barrier"<<rptNewLine
                      << "DW = Overlay";
      row++;
   }

   (*p_table)(row,0) << "Final with Live Load (Bridge Site 3)";
   (*p_table)(row,1) << strDC<<" + Traffic Barrier"<<rptNewLine
                   << "DW = Future Overlay"<< rptNewLine
                   << "LL+IM = Live Load + Impact" << rptNewLine
                   << "PL = Pedestrian Live Load" << rptNewLine;
   row++;


   // LRFD Limit States Load Factors
   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<< "Limit State Load Factors"<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   p_table = pgsReportStyleHolder::CreateDefaultTable(4,"");
   p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
   *pPara << p_table;


   (*p_table)(0,0) << "Limit State";
   (*p_table)(0,1) << Sub2(symbol(gamma),"DC");
   (*p_table)(0,2) << Sub2(symbol(gamma),"DW");
   (*p_table)(0,3) << Sub2(symbol(gamma),"LL");
   
   row = 1;

   GET_IFACE2(pBroker,IStageMap,pStageMap);

   if ( bDesign )
   {
      (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::ServiceI));
      (*p_table)(row,1) << scalar.SetValue(pLoadFactors->DCmax[pgsTypes::ServiceI]);
      (*p_table)(row,2) << scalar.SetValue(pLoadFactors->DWmax[pgsTypes::ServiceI]);
      (*p_table)(row,3) << scalar.SetValue(pLoadFactors->LLIMmax[pgsTypes::ServiceI]);
      row++;

      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::ServiceIA));
         (*p_table)(row,1) << scalar.SetValue(pLoadFactors->DCmax[pgsTypes::ServiceIA]);
         (*p_table)(row,2) << scalar.SetValue(pLoadFactors->DWmax[pgsTypes::ServiceIA]);
         (*p_table)(row,3) << scalar.SetValue(pLoadFactors->LLIMmax[pgsTypes::ServiceIA]);
         row++;
      }

      (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::ServiceIII));
      (*p_table)(row,1) << scalar.SetValue(pLoadFactors->DCmax[pgsTypes::ServiceIII]);
      (*p_table)(row,2) << scalar.SetValue(pLoadFactors->DWmax[pgsTypes::ServiceIII]);
      (*p_table)(row,3) << scalar.SetValue(pLoadFactors->LLIMmax[pgsTypes::ServiceIII]);
      row++;

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::FatigueI));
         (*p_table)(row,1) << scalar.SetValue(pLoadFactors->DCmax[pgsTypes::FatigueI]);
         (*p_table)(row,2) << scalar.SetValue(pLoadFactors->DWmax[pgsTypes::FatigueI]);
         (*p_table)(row,3) << scalar.SetValue(pLoadFactors->LLIMmax[pgsTypes::FatigueI]);
         row++;
      }

      (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthI));
      (*p_table)(row,1) << scalar.SetValue(pLoadFactors->DCmax[pgsTypes::StrengthI]) << "/";
      (*p_table)(row,1) << scalar.SetValue(pLoadFactors->DCmin[pgsTypes::StrengthI]);
      (*p_table)(row,2) << scalar.SetValue(pLoadFactors->DWmax[pgsTypes::StrengthI]) << "/";
      (*p_table)(row,2) << scalar.SetValue(pLoadFactors->DWmin[pgsTypes::StrengthI]);
      (*p_table)(row,3) << scalar.SetValue(pLoadFactors->LLIMmax[pgsTypes::StrengthI]);
      row++;

      if ( bPermit )
      {
         (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthII));
         (*p_table)(row,1) << scalar.SetValue(pLoadFactors->DCmax[pgsTypes::StrengthII]) << "/";
         (*p_table)(row,1) << scalar.SetValue(pLoadFactors->DCmin[pgsTypes::StrengthII]);
         (*p_table)(row,2) << scalar.SetValue(pLoadFactors->DWmax[pgsTypes::StrengthII]) << "/";
         (*p_table)(row,2) << scalar.SetValue(pLoadFactors->DWmin[pgsTypes::StrengthII]);
         (*p_table)(row,3) << scalar.SetValue(pLoadFactors->LLIMmax[pgsTypes::StrengthII]);
         row++;
      }
   }

   if ( bRating )
   {
      bool bFootNote = false;
      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
      {
         if ( pRatingSpec->RateForStress(pgsTypes::lrDesign_Inventory) )
         {
            pgsTypes::LimitState ls = pgsTypes::ServiceIII_Inventory;
            (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(ls));
            (*p_table)(row,1) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
            (*p_table)(row,2) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
            Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
            if ( gLL < 0 )
            {
               (*p_table)(row,3) << "*";
               bFootNote = true;
            }
            else
            {
               (*p_table)(row,3) << scalar.SetValue(gLL);
            }

            row++;
         }

         pgsTypes::LimitState ls = pgsTypes::StrengthI_Inventory;
         (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(ls));
         (*p_table)(row,1) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
         (*p_table)(row,2) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
         Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
         if ( gLL < 0 )
         {
            (*p_table)(row,3) << "*";
            bFootNote = true;
         }
         else
         {
            (*p_table)(row,3) << scalar.SetValue(gLL);
         }

         row++;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
      {
         if ( pRatingSpec->RateForStress(pgsTypes::lrDesign_Operating) )
         {
            pgsTypes::LimitState ls = pgsTypes::ServiceIII_Operating;
            (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(ls));
            (*p_table)(row,1) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
            (*p_table)(row,2) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
            Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
            if ( gLL < 0 )
            {
               (*p_table)(row,3) << "*";
               bFootNote = true;
            }
            else
            {
               (*p_table)(row,3) << scalar.SetValue(gLL);
            }

            row++;
         }

         pgsTypes::LimitState ls = pgsTypes::StrengthI_Operating;
         (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(ls));
         (*p_table)(row,1) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
         (*p_table)(row,2) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
         Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
         if ( gLL < 0 )
         {
            (*p_table)(row,3) << "*";
            bFootNote = true;
         }
         else
         {
            (*p_table)(row,3) << scalar.SetValue(gLL);
         }

         row++;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         if ( pRatingSpec->RateForStress(pgsTypes::lrLegal_Routine) )
         {
            pgsTypes::LimitState ls = pgsTypes::ServiceIII_LegalRoutine;
            (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(ls));
            (*p_table)(row,1) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
            (*p_table)(row,2) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
            Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
            if ( gLL < 0 )
            {
               (*p_table)(row,3) << "*";
               bFootNote = true;
            }
            else
            {
               (*p_table)(row,3) << scalar.SetValue(gLL);
            }

            row++;
         }

         pgsTypes::LimitState ls = pgsTypes::StrengthI_LegalRoutine;
         (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(ls));
         (*p_table)(row,1) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
         (*p_table)(row,2) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
         Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
         if ( gLL < 0 )
         {
            (*p_table)(row,3) << "*";
            bFootNote = true;
         }
         else
         {
            (*p_table)(row,3) << scalar.SetValue(gLL);
         }

         row++;
      }


      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         if ( pRatingSpec->RateForStress(pgsTypes::lrLegal_Special) )
         {
            pgsTypes::LimitState ls = pgsTypes::ServiceIII_LegalSpecial;
            (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(ls));
            (*p_table)(row,1) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
            (*p_table)(row,2) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
            Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
            if ( gLL < 0 )
            {
               (*p_table)(row,3) << "*";
               bFootNote = true;
            }
            else
            {
               (*p_table)(row,3) << scalar.SetValue(gLL);
            }

            row++;
         }

         pgsTypes::LimitState ls = pgsTypes::StrengthI_LegalSpecial;
         (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(ls));
         (*p_table)(row,1) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
         (*p_table)(row,2) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
         Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
         if ( gLL < 0 )
         {
            (*p_table)(row,3) << "*";
            bFootNote = true;
         }
         else
         {
            (*p_table)(row,3) << scalar.SetValue(gLL);
         }

         row++;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         if ( pRatingSpec->RateForStress(pgsTypes::lrPermit_Routine) )
         {
            pgsTypes::LimitState ls = pgsTypes::ServiceI_PermitRoutine;
            (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(ls));
            (*p_table)(row,1) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
            (*p_table)(row,2) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
            Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
            if ( gLL < 0 )
            {
               (*p_table)(row,3) << "*";
               bFootNote = true;
            }
            else
            {
               (*p_table)(row,3) << scalar.SetValue(gLL);
            }

            row++;
         }

         pgsTypes::LimitState ls = pgsTypes::StrengthII_PermitRoutine;
         (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(ls));
         (*p_table)(row,1) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
         (*p_table)(row,2) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
         Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
         if ( gLL < 0 )
         {
            (*p_table)(row,3) << "*";
            bFootNote = true;
         }
         else
         {
            (*p_table)(row,3) << scalar.SetValue(gLL);
         }

         row++;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         if ( pRatingSpec->RateForStress(pgsTypes::lrPermit_Special) )
         {
            pgsTypes::LimitState ls = pgsTypes::ServiceI_PermitSpecial;
            (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(ls));
            (*p_table)(row,1) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
            (*p_table)(row,2) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
            Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
            if ( gLL < 0 )
            {
               (*p_table)(row,3) << "*";
               bFootNote = true;
            }
            else
            {
               (*p_table)(row,3) << scalar.SetValue(gLL);
            }

            row++;
         }

         pgsTypes::LimitState ls = pgsTypes::StrengthII_PermitSpecial;
         (*p_table)(row,0) << OLE2A(pStageMap->GetLimitStateName(ls));
         (*p_table)(row,1) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
         (*p_table)(row,2) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
         Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
         if ( gLL < 0 )
         {
            (*p_table)(row,3) << "*";
            bFootNote = true;
         }
         else
         {
            (*p_table)(row,3) << scalar.SetValue(gLL);
         }

         row++;
      }

      if ( bFootNote )
      {
         pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
         *pChapter << pPara;
         *pPara << Super("*") << "Live Load Factor depends on the weight of the axles on the bridge" << rptNewLine;
      }
   }
   }


   if (!m_bSimplifiedVersion && !bRating)
   {
   // Equivalent prestress loading for camber
   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<< "Equivalent Prestress Loading for Camber"<<rptNewLine<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << "Loads shown in positive directions" << rptNewLine;

   moment.ShowUnitTag(true);
   force.ShowUnitTag(true);
   loc.ShowUnitTag(true);

   GET_IFACE2(pBroker,ICamber,pCamber);
   for ( SpanIndexType spanIdx = firstSpanIdx; spanIdx < lastSpanIdx; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType firstGirderIdx = min(nGirders-1,(gdr == ALL_GIRDERS ? 0 : gdr));
      GirderIndexType lastGirderIdx  = min(nGirders,  (gdr == ALL_GIRDERS ? nGirders : firstGirderIdx + 1));
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx < lastGirderIdx; gdrIdx++ )
      {
         if ( span == ALL_SPANS || gdr == ALL_GIRDERS )
         {
            std::ostringstream os;
            os << "Span " << LABEL_SPAN(spanIdx) << " Girder " << LABEL_GIRDER(gdrIdx);
            pPara->SetName( os.str().c_str() );
            (*pPara) << pPara->GetName() << rptNewLine;
         }

         pPara = new rptParagraph;
         *pChapter << pPara;

         std::vector<std::pair<Float64,Float64> > loads;
         pCamber->GetStraightStrandEquivLoading(spanIdx,gdrIdx,&loads);
         std::vector<std::pair<Float64,Float64> >::iterator iter;
         *pPara << Bold("Straight Strands") << rptNewLine;
         *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "StraightStrandCamberLoading.gif") << rptNewLine;
         for ( iter = loads.begin(); iter != loads.end(); iter++ )
         {
            double M = iter->first;
            double X = iter->second;

            *pPara << "M = " << moment.SetValue(M) << " at " << loc.SetValue(X) << rptNewLine;
         }

         *pPara << rptNewLine;

         double Ml,Mr,Nl,Nr,Xl,Xr;
         pCamber->GetHarpedStrandEquivLoading(spanIdx,gdrIdx,&Ml, &Mr, &Nl, &Nr, &Xl, &Xr);
         *pPara << Bold("Harped Strands") << rptNewLine;
         *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "HarpedStrandCamberLoading.gif") << rptNewLine;
         *pPara << "End moments, M = " << moment.SetValue(Ml);
         *pPara << " and " << moment.SetValue(Mr) << rptNewLine;
         *pPara << "Left Harp Point, N = " << force.SetValue(Nl) << " at = " << loc.SetValue(Xl) << rptNewLine;
         *pPara << "Right Harp Point, N = " << force.SetValue(Nr) << " at = " << loc.SetValue(Xr) << rptNewLine;

         *pPara << rptNewLine;
      
         GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
         if ( 0 < pStrandGeom->GetNumStrands(spanIdx,gdrIdx,pgsTypes::Temporary) )
         {
            double MxferL, MxferR, MremoveL, MremoveR;
            pCamber->GetTempStrandEquivLoading(spanIdx,gdrIdx,&MxferL,&MxferR,&MremoveL,&MremoveR);
            *pPara << Bold("Temporary Strands") << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "TempStrandCamberLoading.gif") << rptNewLine;
            *pPara << "End moments at prestressing, M = " << moment.SetValue(MxferL) << " and ";
            *pPara << moment.SetValue(-MxferR) << rptNewLine;
            *pPara << "Moment at strand removal, M = " << moment.SetValue(MremoveL) << " and ";
            *pPara << moment.SetValue(-MremoveR) << rptNewLine;
         } // end if
      } // gdrIdx
   } // spanIdx
   }

   return pChapter;
}

CChapterBuilder* CLoadingDetailsChapterBuilder::Clone() const
{
   return new CLoadingDetailsChapterBuilder(m_bSimplifiedVersion,m_bDesign,m_bRating,m_bSelect);
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

static bool IsSlabLoadUniform(const std::vector<SlabLoad>& slabLoads, pgsTypes::SupportedDeckType deckType)
{
   bool is_pad = deckType==pgsTypes::sdtCompositeSIP;

   Float64 main_load;
   Float64 panel_load;
   Float64 pad_load;

   bool first=true;
   for ( std::vector<SlabLoad>::const_iterator i = slabLoads.begin(); i != slabLoads.end(); i++ )
   {
      const SlabLoad& slab_load = *i;

      if (first)
      {
         main_load  = slab_load.MainSlabLoad;
         panel_load = slab_load.PanelLoad;
         pad_load   = slab_load.PadLoad;
         first = false;
      }
      else
      {
         // only takes one difference to be nonuniform
         if (!IsEqual(main_load, slab_load.MainSlabLoad))
            return false;

         if (!IsEqual(panel_load, slab_load.PanelLoad))
            return false;

         if (is_pad && !IsEqual(pad_load, slab_load.PadLoad))
            return false;
      }
   }

   return true;
}

static bool IsOverlayLoadUniform(const std::vector<OverlayLoad>& overlayLoads)
{
   Float64 loadval;

   bool first=true;
   for ( std::vector<OverlayLoad>::const_iterator i = overlayLoads.begin(); i != overlayLoads.end(); i++ )
   {
      const OverlayLoad& load = *i;

      if (first)
      {
         loadval = load.StartLoad;
         first = false;
      }
      else
      {
         // only takes one difference to be nonuniform
         if (!IsEqual(loadval, load.StartLoad))
            return false;
      }

      if (!IsEqual(loadval, load.EndLoad))
         return false;
   }


   return true;
}

static bool IsShearKeyLoadUniform(const std::vector<ShearKeyLoad>& loads)
{
   Float64 loadval, unifval;

   bool first=true;
   for ( std::vector<ShearKeyLoad>::const_iterator i = loads.begin(); i != loads.end(); i++ )
   {
      const ShearKeyLoad& load = *i;

      if (first)
      {
         loadval = load.StartJointLoad;
         unifval = load.UniformLoad;
         first = false;
      }
      else
      {
         // only takes one difference to be nonuniform
         if (!IsEqual(loadval, load.StartJointLoad))
            return false;

         if (!IsEqual(unifval, load.UniformLoad))
            return false;
      }

      if (!IsEqual(loadval, load.EndJointLoad))
         return false;
   }

   return true;
}

static bool IsConstructionLoadUniform(const std::vector<ConstructionLoad>& loads)
{
   Float64 loadval;

   bool first=true;
   for ( std::vector<ConstructionLoad>::const_iterator i = loads.begin(); i != loads.end(); i++ )
   {
      const ConstructionLoad& load = *i;

      if (first)
      {
         loadval = load.StartLoad;
         first = false;
      }
      else
      {
         // only takes one difference to be nonuniform
         if (!IsEqual(loadval, load.StartLoad))
            return false;

      }

      if (!IsEqual(loadval, load.EndLoad))
         return false;
   }

   return true;
}
