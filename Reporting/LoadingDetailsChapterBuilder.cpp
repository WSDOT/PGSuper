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
#include <Reporting\LoadingDetailsChapterBuilder.h>
#include <Reporting\UserDefinedLoadsChapterBuilder.h>

#include <PgsExt\LoadFactors.h>
#include <PgsExt\BridgeDescription2.h>


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
#pragma Reminder("REVIEW: use chapter levels")
   // The reporting framework has the concept of building a chapter at a specified level. This
   // is the same thing as using the m_bSimplifiedVersion flag... consider re-working this
   // to use the levels feature.
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

   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else if ( pGdrLineRptSpec)
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      girderKey = pGdrLineRptSpec->GetGirderKey();
   }
   else
   {
      ATLASSERT(false); // not expecting a different kind of report spec
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


      // if none of the rating types are enabled, skip the rating
      if ( !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) 
         )
         bRating = false;
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
            
   GET_IFACE2(pBroker,IProductLoads,pProdLoads);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);
   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType firstGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
      GirderIndexType lastGirderIdx  = (girderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx);
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,gdrIdx));
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey thisSegmentKey(grpIdx,gdrIdx,segIdx);

            if ( 1 < nSegments /* && !m_bSimplifiedVersion*/ )
            {
               pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
               *pChapter << pPara;
               *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
            }

            // uniform loads
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;
            *pPara << _T("Uniform Loads Applied Along the Entire Girder") << rptNewLine;

            pPara = new rptParagraph;
            *pChapter << pPara;

            p_table = pgsReportStyleHolder::CreateDefaultTable(2);
            *pPara << p_table << rptNewLine;

            p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
            p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

            (*p_table)(0,0) << _T("Load Type");
            (*p_table)(0,1) << COLHDR(_T("w"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

            RowIndexType row = p_table->GetNumberOfHeaderRows();

            std::vector<GirderLoad> gdrLoad;
            std::vector<DiaphragmLoad> diaLoad;
            pProdLoads->GetGirderSelfWeightLoad(thisSegmentKey,&gdrLoad,&diaLoad);
            bool bUniformGirderDeadLoad = (gdrLoad.size() == 1 && IsEqual(gdrLoad[0].wStart,gdrLoad[0].wEnd) ? true : false);
            if ( bUniformGirderDeadLoad )
            {
               // girder load is uniform
               (*p_table)(row,0) << _T("Girder");
               (*p_table)(row++,1) << fpl.SetValue(-gdrLoad[0].wStart);
            }

            // Sum of railing system loads not shown in simplified version
            if ( !m_bSimplifiedVersion )
            {
               if ( pProdLoads->HasSidewalkLoad(thisSegmentKey) )
               {
                  (*p_table)(row,0) << _T("Sidewalk");
                  (*p_table)(row++,1) << fpl.SetValue(-pProdLoads->GetSidewalkLoad(thisSegmentKey));
               }

               Float64 tb_load = pProdLoads->GetTrafficBarrierLoad(thisSegmentKey);
               if (tb_load!=0.0)
               {
                  (*p_table)(row,0) << _T("Traffic Barrier");
                  (*p_table)(row++,1) << fpl.SetValue(-tb_load);
               }

               if ( pProdLoads->HasPedestrianLoad(thisSegmentKey) )
               {
                  (*p_table)(row,0) << _T("Pedestrian Live Load");
                  (*p_table)(row++,1) << fpl.SetValue(pProdLoads->GetPedestrianLoad(thisSegmentKey));
               }
            }

            if ( !bUniformGirderDeadLoad )
            {
               p_table = pgsReportStyleHolder::CreateDefaultTable(4,_T("Girder Self-Weight"));
               *pPara << rptNewLine << p_table << rptNewLine;

               (*p_table)(0,0) << COLHDR(_T("Load Start,")<<rptNewLine<<_T("From Left End of Girder"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
               (*p_table)(0,1) << COLHDR(_T("Load End,")<<rptNewLine<<_T("From Left End of Girder"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
               (*p_table)(0,2) << COLHDR(_T("Start Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
               (*p_table)(0,3) << COLHDR(_T("End Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

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

            ReportPedestrianLoad(  pChapter,pBroker,pBridge,pProdLoads,pDisplayUnits,thisSegmentKey);
            ReportSlabLoad(        pChapter,pBridge,pProdLoads,pDisplayUnits,thisSegmentKey);
            ReportOverlayLoad(     pChapter,pBridge,pProdLoads,pDisplayUnits,bRating,thisSegmentKey);
            ReportConstructionLoad(pChapter,pBridge,pProdLoads,pDisplayUnits,thisSegmentKey);
            ReportShearKeyLoad(    pChapter,pBridge,pProdLoads,pDisplayUnits,thisSegmentKey,one_girder_has_shear_key);
            ReportPrecastDiaphragmLoad(pChapter,pBridge,pProdLoads,pDisplayUnits,thisSegmentKey);
         } // segIdx
      } // gdrIdx
   } // grpIdx

   // User defined loads.... these loads are span/girder based
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserDefinedLoads);
   IndexType nPointLoads  = pUserDefinedLoads->GetPointLoadCount();
   IndexType nDistLoads   = pUserDefinedLoads->GetDistributedLoadCount();
   IndexType nMomentLoads = pUserDefinedLoads->GetMomentLoadCount();
   bool bHasUserLoads = (0 < nPointLoads + nDistLoads + nMomentLoads ? true : false);

   SpanIndexType startSpanIdx, endSpanIdx;
   pBridge->GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType firstGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
      GirderIndexType lastGirderIdx  = (girderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx);
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         CSpanKey spanKey(spanIdx,gdrIdx);

         ReportCastInPlaceDiaphragmLoad(pChapter,pBridge,pProdLoads,pDisplayUnits,spanKey);

         // User Defined Loads
         if ( bHasUserLoads )
         {
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;
            *pPara<< _T("User Defined Loads")<<rptNewLine;
            pPara = CUserDefinedLoadsChapterBuilder::CreatePointLoadTable(pBroker, spanKey, pDisplayUnits, level, m_bSimplifiedVersion);
            *pChapter << pPara;
            pPara = CUserDefinedLoadsChapterBuilder::CreateDistributedLoadTable(pBroker, spanKey, pDisplayUnits, level, m_bSimplifiedVersion);
            *pChapter << pPara;
            pPara = CUserDefinedLoadsChapterBuilder::CreateMomentLoadTable(pBroker, spanKey, pDisplayUnits, level, m_bSimplifiedVersion);
            *pChapter << pPara;
         }
      }
   }


   ReportEquivPostTensionLoads(pChapter,pDisplayUnits,girderKey);

   bool bPermit;
   ReportLiveLoad(pChapter,bDesign,bRating,pRatingSpec,bPermit);
   ReportLimitStates(pChapter,bDesign,bRating,bPermit,one_girder_has_shear_key,pRatingSpec);

   ReportEquivPretensionLoads(pChapter,bRating,pBridge,pDisplayUnits,girderKey);

   return pChapter;
}

CChapterBuilder* CLoadingDetailsChapterBuilder::Clone() const
{
   return new CLoadingDetailsChapterBuilder(m_bSimplifiedVersion,m_bDesign,m_bRating,m_bSelect);
}


void CLoadingDetailsChapterBuilder::ReportPedestrianLoad(rptChapter* pChapter,IBroker* pBroker,IBridge* pBridge,IProductLoads* pProdLoads,IEAFDisplayUnits* pDisplayUnits,const CSegmentKey& thisSegmentKey) const
{
   GET_IFACE2(pBroker,IBarriers,pBarriers);
   rptParagraph* pPara = NULL;

   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl,    pDisplayUnits->GetForcePerLengthUnit(), false );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   // Details for sidewalks and barriers
   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Distribution of Uniform Barrier, Sidewalk, and Pedestrian Loads to Girder") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(4,_T(""));
   *pPara << p_table << rptNewLine;

   p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   (*p_table)(0,0) << _T("Load Type");
   (*p_table)(0,1) << COLHDR(_T("Total Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   (*p_table)(0,2) << _T("Fraction")<<rptNewLine<<_T("to Girder");
   (*p_table)(0,3) << COLHDR(_T("Girder Load"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   Float64 Wtb_per_girder, fraExtLeft, fraExtRight, fraIntLeft, fraIntRight;
   pProdLoads->GetTrafficBarrierLoadFraction(thisSegmentKey,&Wtb_per_girder,&fraExtLeft,&fraIntLeft,&fraExtRight,&fraIntRight);
   Float64 Wsw_per_girder, fraSwLeft, fraSwRight;
   pProdLoads->GetSidewalkLoadFraction(thisSegmentKey,&Wsw_per_girder,&fraSwLeft,&fraSwRight);

   Float64 barwt = pBarriers->GetExteriorBarrierWeight(pgsTypes::tboLeft);
   (*p_table)(row,0) << _T("Left Ext. Barrier");
   (*p_table)(row,1) <<  fpl.SetValue(barwt);
   (*p_table)(row,2) << scalar.SetValue(fraExtLeft);
   (*p_table)(row++,3) <<  fpl.SetValue(barwt*fraExtLeft);

   Float64 swwt = pBarriers->GetSidewalkWeight(pgsTypes::tboLeft);
   if (0.0 < swwt)
   {
      (*p_table)(row,0) << _T("Left Sidewalk");
      (*p_table)(row,1) <<  fpl.SetValue(swwt);
      (*p_table)(row,2) << scalar.SetValue(fraSwLeft);
      (*p_table)(row++,3) <<  fpl.SetValue(swwt*fraSwLeft);
   }

   if ( pProdLoads->HasPedestrianLoad() )
   {
      swwt = pProdLoads->GetPedestrianLoadPerSidewalk(pgsTypes::tboLeft);
      if (0.0 < swwt)
      {
         (*p_table)(row,0) << _T("Left Pedestrian Live");
         (*p_table)(row,1) <<  fpl.SetValue(swwt);
         (*p_table)(row,2) << scalar.SetValue(fraSwLeft);
         (*p_table)(row++,3) <<  fpl.SetValue(swwt*fraSwLeft);
      }
   }

   barwt = pBarriers->GetInteriorBarrierWeight(pgsTypes::tboLeft);
   if (0.0 < barwt)
   {
      (*p_table)(row,0) << _T("Left Int. Barrier");
      (*p_table)(row,1) <<  fpl.SetValue(barwt);
      (*p_table)(row,2) << scalar.SetValue(fraIntLeft);
      (*p_table)(row++,3) <<  fpl.SetValue(barwt*fraIntLeft);
   }

   barwt = pBarriers->GetInteriorBarrierWeight(pgsTypes::tboRight);
   if (0.0 < barwt)
   {
      (*p_table)(row,0) << _T("Right Int. Barrier");
      (*p_table)(row,1) <<  fpl.SetValue(barwt);
      (*p_table)(row,2) << scalar.SetValue(fraIntRight);
      (*p_table)(row++,3) <<  fpl.SetValue(barwt*fraIntRight);
   }

   swwt = pBarriers->GetSidewalkWeight(pgsTypes::tboRight);
   if (0.0 < swwt)
   {
      (*p_table)(row,0) << _T("Right Sidewalk");
      (*p_table)(row,1) <<  fpl.SetValue(swwt);
      (*p_table)(row,2) << scalar.SetValue(fraSwRight);
      (*p_table)(row++,3) <<  fpl.SetValue(swwt*fraSwRight);
   }

   barwt = pBarriers->GetExteriorBarrierWeight(pgsTypes::tboRight);
   (*p_table)(row,0) << _T("Right Ext. Barrier");
   (*p_table)(row,1) <<  fpl.SetValue(barwt);
   (*p_table)(row,2) << scalar.SetValue(fraExtRight);
   (*p_table)(row++,3) <<  fpl.SetValue(barwt*fraExtRight);

   if ( pProdLoads->HasPedestrianLoad() )
   {
      swwt = pProdLoads->GetPedestrianLoadPerSidewalk(pgsTypes::tboRight);
      if (0.0 < swwt)
      {
         (*p_table)(row,0) << _T("Right Pedestrian Live");
         (*p_table)(row,1) <<  fpl.SetValue(swwt);
         (*p_table)(row,2) << scalar.SetValue(fraSwRight);
         (*p_table)(row++,3) <<  fpl.SetValue(swwt*fraSwRight);
      }
   }
}

void CLoadingDetailsChapterBuilder::ReportSlabLoad(rptChapter* pChapter,IBridge* pBridge,IProductLoads* pProdLoads,IEAFDisplayUnits* pDisplayUnits,const CSegmentKey& thisSegmentKey) const
{
   // slab loads between supports
   rptParagraph* pPara = NULL;
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         loc,    pDisplayUnits->GetSpanLengthUnit(),     false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl,    pDisplayUnits->GetForcePerLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,         moment, pDisplayUnits->GetMomentUnit(),         false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,          force,  pDisplayUnits->GetGeneralForceUnit(),   false );

   Float64 end_size = pBridge->GetSegmentStartEndDistance(thisSegmentKey);

   if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<< _T("Slab Load Applied Between Bearings")<<rptNewLine;
      
      pgsTypes::SupportedDeckType deck_type = pBridge->GetDeckType();

      std::vector<SlabLoad> slab_loads;
      pProdLoads->GetMainSpanSlabLoad(thisSegmentKey, &slab_loads);

      bool is_uniform = IsSlabLoadUniform(slab_loads, deck_type);

      // Don't eject haunch note unless we have a haunch
      bool do_report_haunch = true;
      rptParagraph* pNotePara = new rptParagraph;
      *pChapter << pNotePara;

      if (is_uniform)
      {
         *pNotePara << _T("Slab Load is uniform along entire girder length.");
      }
      else
      {
         *pNotePara << _T("Slab Load is approximated with Linear Load Segments applied along the length of the girder");
      }

      pPara = new rptParagraph;
      *pChapter << pPara;

      if ( deck_type == pgsTypes::sdtCompositeSIP )
      {
         if (is_uniform)
         {
            rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(2,_T(""));
            *pPara << p_table << rptNewLine;
            p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
            p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
            (*p_table)(0,0) << _T("Load Type");
            (*p_table)(0,1) << COLHDR(_T("w"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            RowIndexType row = p_table->GetNumberOfHeaderRows();

            const SlabLoad& slab_load = *(slab_loads.begin());

            (*p_table)(row,0) << _T("Panel Weight");
            (*p_table)(row++,1) << fpl.SetValue(-slab_load.PanelLoad);
            (*p_table)(row,0) << _T("Cast Slab Weight");
            (*p_table)(row++,1) << fpl.SetValue(-slab_load.MainSlabLoad);
            (*p_table)(row,0) << _T("Haunch Weight");
            (*p_table)(row++,1) << fpl.SetValue(-slab_load.PadLoad);
            (*p_table)(row,0) << _T("Total Slab Weight");
            (*p_table)(row++,1) << fpl.SetValue(-slab_load.PanelLoad-slab_load.MainSlabLoad-slab_load.PadLoad);
         }
         else
         {
            rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(5,_T(""));
            *pPara << p_table;

            (*p_table)(0,0) << COLHDR(_T("Location")<<rptNewLine<<_T("From Left Bearing"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,1) << COLHDR(_T("Panel Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            (*p_table)(0,2) << COLHDR(_T("Cast Slab Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            (*p_table)(0,3) << COLHDR(_T("Haunch Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            (*p_table)(0,4) << COLHDR(_T("Total Slab Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

            RowIndexType row = p_table->GetNumberOfHeaderRows();
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
               (*p_table)(row,4) << fpl.SetValue(-(panel_load+main_load+pad_load));

               row++;
            }
         }
      }
      else
      {
         if (is_uniform)
         {
            rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(2,_T(""));
            *pPara << p_table << rptNewLine;
            p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
            p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
            (*p_table)(0,0) << _T("Load Type");
            (*p_table)(0,1) << COLHDR(_T("w"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            RowIndexType row = p_table->GetNumberOfHeaderRows();

            const SlabLoad& slab_load = *(slab_loads.begin());

            // Don't report zero slab load in simplified version
            do_report_haunch = !(m_bSimplifiedVersion && IsZero(slab_load.PadLoad));

            if (do_report_haunch)
            {
               (*p_table)(row,0) << _T("Main Slab Weight");
               (*p_table)(row++,1) << fpl.SetValue(-slab_load.MainSlabLoad);
               (*p_table)(row,0) << _T("Haunch Weight");
               (*p_table)(row++,1) << fpl.SetValue(-slab_load.PadLoad);
            }
            (*p_table)(row,0) << _T("Total Slab Weight");
            (*p_table)(row++,1) << fpl.SetValue(-slab_load.MainSlabLoad-slab_load.PadLoad);
         }
         else
         {
            rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(4,_T(""));
            *pPara << p_table;

            (*p_table)(0,0) << COLHDR(_T("Location")<<rptNewLine<<_T("From Left Bearing"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,1) << COLHDR(_T("Main Slab Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            (*p_table)(0,2) << COLHDR(_T("Haunch Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            (*p_table)(0,3) << COLHDR(_T("Total Slab Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

            RowIndexType row = p_table->GetNumberOfHeaderRows();
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
      {
         *pNotePara <<rptNewLine<< _T("Haunch weight includes effects of roadway geometry but does not include a reduction for camber");
      }

      // the rest of the content is for the non-simplified version (full boat)
      if (!m_bSimplifiedVersion)
      {
         // slab cantilever loads
         pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter << pPara;
         *pPara<< _T("Slab Cantilever Loads")<<rptNewLine;
      
         pPara = new rptParagraph;
         *pChapter << pPara;

         rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(5,_T(""));
         *pPara << p_table;

         p_table->SetNumberOfHeaderRows(2);
         p_table->SetRowSpan(0,0,2);
         p_table->SetRowSpan(1,0,SKIP_CELL);
         (*p_table)(0,0) << _T("Location");

         p_table->SetColumnSpan(0,1,2);
         p_table->SetColumnSpan(0,2,SKIP_CELL);
         (*p_table)(0,1) << _T("Main Slab");
         (*p_table)(1,1) << COLHDR(_T("Point Load"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
         (*p_table)(1,2) << COLHDR(_T("Point Moment"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

         p_table->SetColumnSpan(0,3,2);
         p_table->SetColumnSpan(0,4,SKIP_CELL);
         (*p_table)(0,3) << _T("Haunch");
         (*p_table)(1,3) << COLHDR(_T("Point Load"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
         (*p_table)(1,4) << COLHDR(_T("Point Moment"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

         (*p_table)(2,0) << _T("Left Bearing");
         (*p_table)(3,0) << _T("Right Bearing");

         Float64 P1, P2, M1, M2;
         pProdLoads->GetCantileverSlabLoad(thisSegmentKey, &P1, &M1, &P2, &M2);
         (*p_table)(2,1) << force.SetValue(-P1);
         (*p_table)(2,2) << moment.SetValue(M1);
      
         (*p_table)(3,1) << force.SetValue(-P2);
         (*p_table)(3,2) << moment.SetValue(M2);

         pProdLoads->GetCantileverSlabPadLoad(thisSegmentKey, &P1, &M1, &P2, &M2);
         (*p_table)(2,3) << force.SetValue(-P1);
         (*p_table)(2,4) << moment.SetValue(M1);
      
         (*p_table)(3,3) << force.SetValue(-P2);
         (*p_table)(3,4) << moment.SetValue(M2);

         p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
         p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      }
   } // end if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
}

void CLoadingDetailsChapterBuilder::ReportOverlayLoad(rptChapter* pChapter,IBridge* pBridge,IProductLoads* pProdLoads,IEAFDisplayUnits* pDisplayUnits,bool bRating,const CSegmentKey& thisSegmentKey) const
{
   // slab loads between supports
   rptParagraph* pPara = NULL;
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         loc,    pDisplayUnits->GetSpanLengthUnit(),     false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl,    pDisplayUnits->GetForcePerLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,         moment, pDisplayUnits->GetMomentUnit(),         false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,          force,  pDisplayUnits->GetGeneralForceUnit(),   false );

   Float64 end_size = pBridge->GetSegmentStartEndDistance(thisSegmentKey);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // overlay laod
   bool bReportOverlay = pBridge->HasOverlay();
   if ( bRating && pBridge->IsFutureOverlay() ) // don't report future overlay load for ratings
   {
      bReportOverlay = false;
   }

   if ( bReportOverlay )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara << _T("Overlay") << rptNewLine;
      pPara = new rptParagraph;
      *pChapter << pPara;

      GET_IFACE2(pBroker, ISpecification, pSpec );
      pgsTypes::OverlayLoadDistributionType olayd = pSpec->GetOverlayLoadDistributionType();

      std::vector<OverlayLoad> overlay_loads;
      pProdLoads->GetOverlayLoad(thisSegmentKey, &overlay_loads);

      bool is_uniform = IsOverlayLoadUniform(overlay_loads);

      if (is_uniform)
      {
         *pPara<<_T("Overlay load is uniform along entire girder length.")<<rptNewLine;

         rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(3,_T(""));
         *pPara << p_table << rptNewLine;
         p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
         p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
         (*p_table)(0,0) << _T("Load Type");
         (*p_table)(0,1) << COLHDR(Sub2(_T("W"),_T("trib")),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*p_table)(0,2) << COLHDR(_T("w"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
         RowIndexType row = p_table->GetNumberOfHeaderRows();

         const OverlayLoad& ovl_load = *(overlay_loads.begin());

         (*p_table)(row,0) << _T("Overlay Weight");
         (*p_table)(row,1) << loc.SetValue(ovl_load.StartWcc);
         (*p_table)(row++,2) << fpl.SetValue(-ovl_load.wStart);
      }
      else
      {
         rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(6,_T(""));
         *pPara << p_table;

         (*p_table)(0,0) << COLHDR(_T("Load Start,")<<rptNewLine<<_T("From Left Bearing"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*p_table)(0,1) << COLHDR(_T("Load End,")  <<rptNewLine<<_T("From Left Bearing"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

         if (olayd==pgsTypes::olDistributeTributaryWidth)
         {
            (*p_table)(0,2) << COLHDR(_T("Start ") << Sub2(_T("W"),_T("trib")),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,3) << COLHDR(_T("End ")   << Sub2(_T("W"),_T("trib")),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         }
         else
         {
            (*p_table)(0,2) << COLHDR(_T("Start ") << Sub2(_T("W"),_T("cc")),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,3) << COLHDR(_T("End ")   << Sub2(_T("W"),_T("cc")),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         }

         (*p_table)(0,4) << COLHDR(_T("Start Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
         (*p_table)(0,5) << COLHDR(_T("End Weight"),  rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

         RowIndexType row = p_table->GetNumberOfHeaderRows();
         for (std::vector<OverlayLoad>::iterator i = overlay_loads.begin(); i != overlay_loads.end(); i++)
         {
            OverlayLoad& load = *i;
            Float64 x1 = load.StartLoc - end_size;
            Float64 x2 = load.EndLoc   - end_size;
            Float64 Wcc1 = load.StartWcc;
            Float64 Wcc2 = load.EndWcc;
            Float64 w1   = load.wStart;
            Float64 w2   = load.wEnd;

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
         *pPara << _T("Overlay load is distributed using tributary width.")<< rptNewLine;
      }
      else
      {
         *pPara << _T("Overlay load is distributed uniformly among all girders per LRFD 4.6.2.2.1")<< rptNewLine;
         *pPara << Sub2(_T("W"),_T("cc")) << _T(" is the curb to curb width")<< rptNewLine;
      }
   } // end if overlay
}

void CLoadingDetailsChapterBuilder::ReportConstructionLoad(rptChapter* pChapter,IBridge* pBridge,IProductLoads* pProdLoads,IEAFDisplayUnits* pDisplayUnits,const CSegmentKey& thisSegmentKey) const
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         loc,    pDisplayUnits->GetSpanLengthUnit(),     false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl,    pDisplayUnits->GetForcePerLengthUnit(), false );

   Float64 end_size = pBridge->GetSegmentStartEndDistance(thisSegmentKey);

   rptParagraph* pPara = NULL;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // construction load
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoads);
   if ( !IsZero(pUserLoads->GetConstructionLoad()) )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara << _T("Construction") << rptNewLine;
      pPara = new rptParagraph;
      *pChapter << pPara;

      std::vector<ConstructionLoad> construction_loads;
      pProdLoads->GetConstructionLoad(thisSegmentKey, &construction_loads);

      bool is_uniform = IsConstructionLoadUniform(construction_loads);

      if (is_uniform)
      {
         *pPara<<_T("Construction load is uniform along entire girder length.")<<rptNewLine;

         rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(2,_T(""));
         *pPara << p_table << rptNewLine;
         p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
         p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
         (*p_table)(0,0) << _T("Load Type");
         (*p_table)(0,1) << COLHDR(_T("w"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
         RowIndexType row = p_table->GetNumberOfHeaderRows();

         const ConstructionLoad& cnst_load = *(construction_loads.begin());

         (*p_table)(row,0) << _T("Construction Weight");
         (*p_table)(row++,1) << fpl.SetValue(-cnst_load.wStart);
      }
      else
      {
         rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(6,_T(""));
         *pPara << p_table;

         (*p_table)(0,0) << COLHDR(_T("Load Start,")<<rptNewLine<<_T("From Left Bearing"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*p_table)(0,1) << COLHDR(_T("Load End,")<<rptNewLine<<_T("From Left Bearing"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

         (*p_table)(0,2) << COLHDR(_T("Start ") << Sub2(_T("W"),_T("cc")),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*p_table)(0,3) << COLHDR(_T("End ") << Sub2(_T("W"),_T("cc")),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

         (*p_table)(0,4) << COLHDR(_T("Start Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
         (*p_table)(0,5) << COLHDR(_T("End Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );


         RowIndexType row = p_table->GetNumberOfHeaderRows();
         for (std::vector<ConstructionLoad>::iterator i = construction_loads.begin(); i != construction_loads.end(); i++)
         {
            ConstructionLoad& load = *i;
            Float64 x1 = load.StartLoc - end_size;
            Float64 x2 = load.EndLoc   - end_size;
            Float64 Wcc1 = load.StartWcc;
            Float64 Wcc2 = load.EndWcc;
            Float64 w1   = load.wStart;
            Float64 w2   = load.wEnd;

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
}

void CLoadingDetailsChapterBuilder::ReportShearKeyLoad(rptChapter* pChapter,IBridge* pBridge,IProductLoads* pProdLoads,IEAFDisplayUnits* pDisplayUnits,const CSegmentKey& thisSegmentKey,bool& one_girder_has_shear_key) const
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         loc,    pDisplayUnits->GetSpanLengthUnit(),     false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl,    pDisplayUnits->GetForcePerLengthUnit(), false );

   Float64 end_size = pBridge->GetSegmentStartEndDistance(thisSegmentKey);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   rptParagraph* pPara = NULL;

   // Shear key loads
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   bool has_shear_key = pGirder->HasShearKey(thisSegmentKey, spacingType);
   if ( has_shear_key )
   {
      one_girder_has_shear_key = true;
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<< _T("Shear Key Load")<<rptNewLine;
      
      pPara = new rptParagraph;
      *pChapter << pPara;

      std::vector<ShearKeyLoad> loads;
      pProdLoads->GetShearKeyLoad(thisSegmentKey, &loads);

      bool is_uniform = IsShearKeyLoadUniform(loads);
      if (is_uniform)
      {
         *pPara << _T("Shear Key Load is uniform along entire girder length") << rptNewLine;

         rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(2,_T(""));
         *pPara << p_table << rptNewLine;
         p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
         p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
         (*p_table)(0,0) << _T("Load Type");
         (*p_table)(0,1) << COLHDR(_T("w"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

         RowIndexType row = p_table->GetNumberOfHeaderRows();

         const ShearKeyLoad& load = *(loads.begin());

         (*p_table)(row,0) << _T("Load Within Girder Envelope");
         (*p_table)(row++,1) << fpl.SetValue(-load.UniformLoad);

         (*p_table)(row,0) << _T("Load Within Joint");
         (*p_table)(row++,1) << fpl.SetValue(-load.StartJointLoad);

         (*p_table)(row,0) << _T("Total Shear Key Weight");
         (*p_table)(row++,1) << fpl.SetValue(-load.StartJointLoad - load.UniformLoad);
      }
      else
      {
         *pPara << _T("Shear Key Load is approximated with Linear Load Segments applied along the length of the girder") << rptNewLine;

         rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(4,_T(""));
         *pPara << p_table;

         (*p_table)(0,0) << COLHDR(_T("Location")<<rptNewLine<<_T("From Left Bearing"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*p_table)(0,1) << COLHDR(_T("Load Within")<<rptNewLine<<_T("Girder Envelope"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
         (*p_table)(0,2) << COLHDR(_T("Load Within")<<rptNewLine<<_T("Joint"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
         (*p_table)(0,3) << COLHDR(_T("Total Shear")<<rptNewLine<<_T("Key Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

         RowIndexType row = p_table->GetNumberOfHeaderRows();
         for ( std::vector<ShearKeyLoad>::iterator i = loads.begin(); i != loads.end(); i++ )
         {
           const ShearKeyLoad& sk_load = *i;

            if (row == p_table->GetNumberOfHeaderRows())
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
}

void CLoadingDetailsChapterBuilder::ReportPrecastDiaphragmLoad(rptChapter* pChapter,IBridge* pBridge,IProductLoads* pProdLoads,IEAFDisplayUnits* pDisplayUnits,const CSegmentKey& thisSegmentKey) const
{
   if ( m_bSimplifiedVersion )
   {
      return;
   }

   INIT_UV_PROTOTYPE( rptLengthUnitValue,         loc,    pDisplayUnits->GetSpanLengthUnit(),     false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl,    pDisplayUnits->GetForcePerLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,          force,  pDisplayUnits->GetGeneralForceUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         dim,    pDisplayUnits->GetComponentDimUnit(),   false );

   Float64 end_size = pBridge->GetSegmentStartEndDistance(thisSegmentKey);

   std::vector<DiaphragmLoad> diaphragm_loads;
   pProdLoads->GetPrecastDiaphragmLoads(thisSegmentKey,&diaphragm_loads);

   if ( 0 < diaphragm_loads.size() )
   {
      rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<< _T("Precast Diaphragms Constructed as part of the Girder")<<rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(5,_T(""));
      *pPara << p_table;

#pragma Reminder("UPDATE: girder/segment label") // should be segment for spliced girders
      (*p_table)(0,0) << COLHDR(_T("Load Location,")<<rptNewLine<<_T("From Left End of Girder"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,1) << COLHDR(_T("H"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
      (*p_table)(0,2) << COLHDR(_T("W"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
      (*p_table)(0,3) << COLHDR(_T("T"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
      (*p_table)(0,4) << COLHDR(_T("Load"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );

      RowIndexType row = p_table->GetNumberOfHeaderRows();

      std::vector<IntermedateDiaphragm> diaphragms(pBridge->GetPrecastDiaphragms(thisSegmentKey));
      ATLASSERT(diaphragms.size() == diaphragm_loads.size());
      std::vector<IntermedateDiaphragm>::iterator iter( diaphragms.begin() );
      std::vector<IntermedateDiaphragm>::iterator end( diaphragms.end() );
      std::vector<DiaphragmLoad>::iterator dia_load_iter(diaphragm_loads.begin());
      for ( ; iter != end; iter++,dia_load_iter++ )
      {
         DiaphragmLoad& rLoad = *dia_load_iter;
         IntermedateDiaphragm& dia = *iter;

         (*p_table)(row,0) << loc.SetValue(rLoad.Loc);
         if ( dia.m_bCompute )
         {
            (*p_table)(row,1) << dim.SetValue(dia.H);
            (*p_table)(row,2) << dim.SetValue(dia.W);
            (*p_table)(row,3) << dim.SetValue(dia.T);
         }
         else
         {
            (*p_table)(row,1) << _T("");
            (*p_table)(row,2) << _T("");
            (*p_table)(row,3) << _T("");
         }
         (*p_table)(row,4) << force.SetValue(-rLoad.Load);
         row++;
      }
   }
}

void CLoadingDetailsChapterBuilder::ReportCastInPlaceDiaphragmLoad(rptChapter* pChapter,IBridge* pBridge,IProductLoads* pProdLoads,IEAFDisplayUnits* pDisplayUnits,const CSpanKey& spanKey) const
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  loc,    pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,   force,  pDisplayUnits->GetGeneralForceUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,  moment, pDisplayUnits->GetMomentUnit(),       false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  dim,    pDisplayUnits->GetComponentDimUnit(), false );

   std::vector<DiaphragmLoad> diap_loads;
   pProdLoads->GetIntermediateDiaphragmLoads(spanKey, &diap_loads);

   if (0 < diap_loads.size() )
   {
      rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<< _T("Intermediate Diaphragms Constructed at Bridge Site")<<rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(5,_T(""));
      *pPara << p_table;

      (*p_table)(0,0) << COLHDR(_T("Load Location,")<<rptNewLine<<_T("from Start of Span"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,1) << COLHDR(_T("H"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
      (*p_table)(0,2) << COLHDR(_T("W"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
      (*p_table)(0,3) << COLHDR(_T("T"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
      (*p_table)(0,4) << COLHDR(_T("Load"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );

      std::vector<IntermedateDiaphragm> diaphragms   = pBridge->GetCastInPlaceDiaphragms(spanKey);
      std::vector<IntermedateDiaphragm>::iterator iter = diaphragms.begin();

      RowIndexType row = p_table->GetNumberOfHeaderRows();
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
            (*p_table)(row,1) << _T("");
            (*p_table)(row,2) << _T("");
            (*p_table)(row,3) << _T("");
         }

         (*p_table)(row,4) << force.SetValue(-rload.Load);
         row++;
      }
   }

   // pier diaphragm loads
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<< _T("Pier Diaphragm Loads")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(4,_T(""));
   *pPara << p_table;

   p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   (*p_table)(0,0) << _T("Pier");
   (*p_table)(0,1) << _T("Location");
   (*p_table)(0,2) << COLHDR(_T("P"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*p_table)(0,3) << COLHDR(_T("M"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   SpanIndexType nSpans = pBridge->GetSpanCount();
   SpanIndexType startSpanIdx = (spanKey.spanIndex == ALL_SPANS ? 0 : spanKey.spanIndex);
   SpanIndexType endSpanIdx   = (spanKey.spanIndex == ALL_SPANS ? nSpans-1 : startSpanIdx);
   PierIndexType startPierIdx = (PierIndexType)startSpanIdx;
   PierIndexType endPierIdx   = (PierIndexType)(endSpanIdx+1);
   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      Float64 Pback, Mback, Pahead, Mahead;
      pProdLoads->GetPierDiaphragmLoads( pierIdx, spanKey.girderIndex, &Pback, &Mback, &Pahead, &Mahead);

      p_table->SetRowSpan(row,0,2);
      (*p_table)(row,0) << LABEL_PIER(pierIdx);

      (*p_table)(row,1) << _T("Left Bearing");
      (*p_table)(row,2) << force.SetValue(-Pback);
      (*p_table)(row,3) << moment.SetValue(Mback);

      row++;

      p_table->SetRowSpan(row,0,SKIP_CELL);
      (*p_table)(row,1) << _T("Right Bearing");
      (*p_table)(row,2) << force.SetValue(-Pahead);
      (*p_table)(row,3) << moment.SetValue(Mahead);

      row++;
   }
}

void CLoadingDetailsChapterBuilder::ReportEquivPostTensionLoads(rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const CGirderKey& girderKey) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,ITendonGeometry,pTendons);
   GET_IFACE2_NOCHECK(pBroker,IProductLoads,pProductLoads); // only used if there are tendons in the model

   std::vector<EquivPostTensionPointLoad> ptLoads;
   std::vector<EquivPostTensionDistributedLoad> distLoads;
   std::vector<EquivPostTensionMomentLoad> momentLoads;

   INIT_UV_PROTOTYPE( rptLengthUnitValue,         loc,    pDisplayUnits->GetSpanLengthUnit(),     false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl,    pDisplayUnits->GetForcePerLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,         moment, pDisplayUnits->GetMomentUnit(),         false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,          force,  pDisplayUnits->GetGeneralForceUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         ecc,    pDisplayUnits->GetComponentDimUnit(),     false );

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<< _T("Equivalent Loading for Post-tensioning")<<rptNewLine;

   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = firstGroupIdx;
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
      lastGroupIdx = pBridgeDesc->GetGirderGroupCount()-1;
   }

   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);

      if ( girderKey.groupIndex == ALL_GROUPS )
      {
         pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara<< _T("Group ") << LABEL_GROUP(grpIdx) << rptNewLine;
      }


      DuctIndexType nDucts = pTendons->GetDuctCount(thisGirderKey);
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter << pPara;

         (*pPara) << _T("Duct ") << LABEL_DUCT(ductIdx) << rptNewLine;

         pProductLoads->GetEquivPostTensionLoads(thisGirderKey,ductIdx,ptLoads,distLoads,momentLoads);

         ColumnIndexType nLayoutColumns = 0;
         nLayoutColumns += (0 < ptLoads.size()     ? 1 : 0);
         nLayoutColumns += (0 < distLoads.size()   ? 1 : 0);
         nLayoutColumns += (0 < momentLoads.size() ? 1 : 0);

         rptRcTable* pLayoutTable = pgsReportStyleHolder::CreateLayoutTable(nLayoutColumns);
         *pPara << pLayoutTable << rptNewLine;

         ColumnIndexType layoutCol = 0;

         rptRcTable* p_table = 0;
         RowIndexType row = 0;

         // Point Loads
         if ( 0 < ptLoads.size() )
         {
            rptParagraph* p = &(*pLayoutTable)(0,layoutCol++);

            p_table = pgsReportStyleHolder::CreateDefaultTable(3,_T("Point"));
            *p << p_table;

            (*p_table)(0,0) << _T("Span");
            (*p_table)(0,1) << COLHDR(_T("X"),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
            (*p_table)(0,2) << COLHDR(_T("P"),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());

            row = p_table->GetNumberOfHeaderRows();
            std::vector<EquivPostTensionPointLoad>::const_iterator ptIter(ptLoads.begin());
            std::vector<EquivPostTensionPointLoad>::const_iterator ptIterEnd(ptLoads.end());
            for ( ; ptIter != ptIterEnd; ptIter++ )
            {
               const EquivPostTensionPointLoad& ptLoad = *ptIter;
               (*p_table)(row,0) << LABEL_SPAN(ptLoad.spanIdx);
               (*p_table)(row,1) << loc.SetValue(ptLoad.X);
               (*p_table)(row,2) << force.SetValue(ptLoad.P);
               row++;
            }
         }


         // Distributed Loads
         if ( 0 < distLoads.size() )
         {
            rptParagraph* p = &(*pLayoutTable)(0,layoutCol++);

            ColumnIndexType nCol = 10;

            p_table = pgsReportStyleHolder::CreateDefaultTable(nCol,_T("Linear"));
            *p << p_table;

            (*p_table)(0,0) << _T("Span");
            (*p_table)(0,1) << COLHDR(Sub2(_T("X"),_T("start")),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
            (*p_table)(0,2) << COLHDR(Sub2(_T("X"),_T("end")),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
            (*p_table)(0,3) << COLHDR(Sub2(_T("W"),_T("start")),rptForcePerLengthUnitTag,pDisplayUnits->GetForcePerLengthUnit());
            (*p_table)(0,4) << COLHDR(Sub2(_T("W"),_T("end")),rptForcePerLengthUnitTag,pDisplayUnits->GetForcePerLengthUnit());
            (*p_table)(0,5) << COLHDR(Sub2(_T("Y"),_T("start")),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
            (*p_table)(0,6) << COLHDR(Sub2(_T("Y"),_T("end")),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
            (*p_table)(0,7) << COLHDR(_T("e'"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
            (*p_table)(0,8) << COLHDR(_T("X"),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
            (*p_table)(0,9) << COLHDR(Sub2(_T("P"),_T("e")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());

            row = p_table->GetNumberOfHeaderRows();
            std::vector<EquivPostTensionDistributedLoad>::const_iterator distIter(distLoads.begin());
            std::vector<EquivPostTensionDistributedLoad>::const_iterator distIterEnd(distLoads.end());
            for ( ; distIter != distIterEnd; distIter++ )
            {
               const EquivPostTensionDistributedLoad& distLoad = *distIter;
               (*p_table)(row,0) << LABEL_SPAN(distLoad.spanIdx);
               (*p_table)(row,1) << loc.SetValue(distLoad.Xstart);
               (*p_table)(row,2) << loc.SetValue(distLoad.Xend);
               (*p_table)(row,3) << fpl.SetValue(distLoad.Wstart);
               (*p_table)(row,4) << fpl.SetValue(distLoad.Wend);
               (*p_table)(row,5) << ecc.SetValue(distLoad.Ystart);
               (*p_table)(row,6) << ecc.SetValue(distLoad.Yend);
               (*p_table)(row,7) << ecc.SetValue(distLoad.e_prime);
               (*p_table)(row,8) << loc.SetValue(distLoad.x);
               (*p_table)(row,9) << force.SetValue(distLoad.P);

               row++;
            }
         }


         // Moment Loads
         if ( 0 < momentLoads.size() )
         {
            rptParagraph* p = &(*pLayoutTable)(0,layoutCol++);

            p_table = pgsReportStyleHolder::CreateDefaultTable(3,_T("Moment"));
            *p << p_table;

            (*p_table)(0,0) << _T("Span");
            (*p_table)(0,1) << COLHDR(_T("X"),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
            (*p_table)(0,2) << COLHDR(_T("M"),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());

            row = p_table->GetNumberOfHeaderRows();
            std::vector<EquivPostTensionMomentLoad>::const_iterator momIter(momentLoads.begin());
            std::vector<EquivPostTensionMomentLoad>::const_iterator momIterEnd(momentLoads.end());
            for ( ; momIter != momIterEnd; momIter++ )
            {
               const EquivPostTensionMomentLoad& momLoad = *momIter;
               (*p_table)(row,0) << LABEL_SPAN(momLoad.spanIdx);
               (*p_table)(row,1) << loc.SetValue(momLoad.X);
               (*p_table)(row,2) << moment.SetValue(momLoad.M);
               row++;
            }
         }
      } // ductIdx
   } // groupIndex
}

void CLoadingDetailsChapterBuilder::ReportLiveLoad(rptChapter* pChapter,bool bDesign,bool bRating,IRatingSpecification* pRatingSpec,bool& bPermit) const
{
   if ( m_bSimplifiedVersion )
   {
      return;
   }

   // live loads
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);

   bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<< _T("Live Loads")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   if ( bDesign )
   {
      std::vector<std::_tstring> designLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltDesign);
      if ( designLiveLoads.empty() )
      {
         *pPara<<_T("No live loads were applied to the design (Service and Strength I) limit states")<< rptNewLine;
      }
      else
      {
         *pPara<<_T("The following live loads were applied to the design (Service and Strength I) limit states:")<< rptNewLine;

         std::vector<std::_tstring>::iterator iter;
         for (iter = designLiveLoads.begin(); iter != designLiveLoads.end(); iter++)
         {
            std::_tstring& load_name = *iter;
            *pPara << load_name << rptNewLine;
         }
      }
      *pPara << rptNewLine;

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         std::vector<std::_tstring> fatigueLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltFatigue);
         if ( fatigueLiveLoads.empty() )
         {
            *pPara<<_T("No live loads were applied to the fatigue (Fatigue I) limit states")<< rptNewLine;
         }
         else
         {
            *pPara<<_T("The following live loads were applied to the fatigue (Fatigue I) limit states:")<< rptNewLine;

            std::vector<std::_tstring>::iterator iter;
            for (iter = fatigueLiveLoads.begin(); iter != fatigueLiveLoads.end(); iter++)
            {
               std::_tstring& load_name = *iter;
               *pPara << load_name << rptNewLine;
            }
         }

         *pPara << rptNewLine;
      }

      if ( bPermit )
      {
         std::vector<std::_tstring> permitLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermit);
         if ( permitLiveLoads.empty() )
         {
            *pPara<<_T("No live loads were applied to the permit (Strength II) limit states")<< rptNewLine;
         }
         else
         {
            *pPara<<_T("The following live loads were applied to the permit (Strength II) limit states:")<< rptNewLine;

            std::vector<std::_tstring>::iterator iter;
            for (iter = permitLiveLoads.begin(); iter != permitLiveLoads.end(); iter++)
            {
               std::_tstring& load_name = *iter;
               *pPara << load_name << rptNewLine;
            }
         }
      }
   }

   if ( bRating )
   {
      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
      {
         std::vector<std::_tstring> designLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltDesign);
         if ( designLiveLoads.empty() )
         {
            *pPara<<_T("No live loads were applied to the design load rating (Service III and Strength I) limit states")<< rptNewLine;
         }
         else
         {
            *pPara<<_T("The following live loads were applied to the design load rating (Service III and Strength I) limit states:")<< rptNewLine;

            std::vector<std::_tstring>::iterator iter;
            for (iter = designLiveLoads.begin(); iter != designLiveLoads.end(); iter++)
            {
               std::_tstring& load_name = *iter;
               *pPara << load_name << rptNewLine;
            }
         }
         *pPara << rptNewLine;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         std::vector<std::_tstring> legalRoutineLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Routine);
         if ( legalRoutineLiveLoads.empty() )
         {
            *pPara<<_T("No live loads were applied to the legal load rating, routine commercial traffic (Service III and Strength I) limit states")<< rptNewLine;
         }
         else
         {
            *pPara<<_T("The following live loads were applied to the legal load rating, routine commercial traffic (Service III and Strength I) limit states:")<< rptNewLine;

            std::vector<std::_tstring>::iterator iter;
            for (iter = legalRoutineLiveLoads.begin(); iter != legalRoutineLiveLoads.end(); iter++)
            {
               std::_tstring& load_name = *iter;
               *pPara << load_name << rptNewLine;
            }
         }
         *pPara << rptNewLine;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         std::vector<std::_tstring> legalSpecialLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Special);
         if ( legalSpecialLiveLoads.empty() )
         {
            *pPara<<_T("No live loads were applied to the legal load rating, specialized hauling vehicles (Service III and Strength I) limit states")<< rptNewLine;
         }
         else
         {
            *pPara<<_T("The following live loads were applied to the legal load rating, specialized hauling vehicles (Service III and Strength I) limit states:")<< rptNewLine;

            std::vector<std::_tstring>::iterator iter;
            for (iter = legalSpecialLiveLoads.begin(); iter != legalSpecialLiveLoads.end(); iter++)
            {
               std::_tstring& load_name = *iter;
               *pPara << load_name << rptNewLine;
            }
         }
         *pPara << rptNewLine;
      }


      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         std::vector<std::_tstring> permitRoutineLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Routine);
         if ( permitRoutineLiveLoads.empty() )
         {
            *pPara<<_T("No live loads were applied to the permit load rating, routine/annual permit (Service I and Strength II) limit states")<< rptNewLine;
         }
         else
         {
            *pPara<<_T("The following live loads were applied to the permit load rating, routine/annual permit (Service I and Strength II) limit states:")<< rptNewLine;

            std::vector<std::_tstring>::iterator iter;
            for (iter = permitRoutineLiveLoads.begin(); iter != permitRoutineLiveLoads.end(); iter++)
            {
               std::_tstring& load_name = *iter;
               *pPara << load_name << rptNewLine;
            }
         }
         *pPara << rptNewLine;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         std::vector<std::_tstring> permitSpecialLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Special);
         if ( permitSpecialLiveLoads.empty() )
         {
            *pPara<<_T("No live loads were applied to the permit load rating, special/limited crossing permit (Service I and Strength II) limit states")<< rptNewLine;
         }
         else
         {
            *pPara<<_T("The following live loads were applied to the permit load rating, special/limited crossing permit (Service I and Strength II) limit states:")<< rptNewLine;

            std::vector<std::_tstring>::iterator iter;
            for (iter = permitSpecialLiveLoads.begin(); iter != permitSpecialLiveLoads.end(); iter++)
            {
               std::_tstring& load_name = *iter;
               *pPara << load_name << rptNewLine;
            }
         }
         *pPara << rptNewLine;
      }
   }
}

void CLoadingDetailsChapterBuilder::ReportLimitStates(rptChapter* pChapter,bool bDesign,bool bRating,bool bPermit,bool one_girder_has_shear_key,IRatingSpecification* pRatingSpec) const
{
   if ( m_bSimplifiedVersion)
   {
      return;
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
   scalar.SetTolerance(1.0e-6);

   // LRFD Limit States
   GET_IFACE2(pBroker,ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

   GET_IFACE2(pBroker,ILossParameters,pLossParameters);
   pgsTypes::LossMethod loss_method = pLossParameters->GetLossMethod();

   RowIndexType row = 0;

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<< _T("Limit States")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(2);
   *pPara << p_table;

   p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   p_table->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   p_table->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   (*p_table)(row,0) << _T("Stage");
   (*p_table)(row,1) << _T("Load Case");
   row++;

   std::_tstring strDC;
   if (one_girder_has_shear_key)
   {
      strDC = _T("DC = Girder + Diaphragms + Shear Key + Construction + Slab");
   }
   else
   {
      strDC = _T("DC = Girder + Diaphragms + Construction + Slab");
   }

   if ( bDesign )
   {
      (*p_table)(row,0) << _T("Casting Yard");
      (*p_table)(row,1) << _T("DC = Girder");
      row++;


      (*p_table)(row,0) << _T("Deck and Diaphragm Placement (Bridge Site 1)");
      (*p_table)(row,1) << strDC;
      row++;

      (*p_table)(row,0) << _T("Superimposed Dead Loads (Bridge Site 2)");
      (*p_table)(row,1) << strDC<<_T(" + Traffic Barrier")<<rptNewLine
                      << _T("DW = Overlay");
      row++;
   }

   (*p_table)(row,0) << _T("Final with Live Load (Bridge Site 3)");
   (*p_table)(row,1) << strDC<<_T(" + Traffic Barrier")<<rptNewLine
                   << _T("DW = Future Overlay")<< rptNewLine
                   << _T("LL+IM = Live Load + Impact") << rptNewLine
                   << _T("PL = Pedestrian Live Load") << rptNewLine;
   row++;


   // LRFD Limit States Load Factors
   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<< _T("Limit State Load Factors")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   ColumnIndexType nColumns = 4;
   if ( loss_method == pgsTypes::TIME_STEP )
   {
      nColumns = 7;
   }

   p_table = pgsReportStyleHolder::CreateDefaultTable(nColumns);
   p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
   *pPara << p_table;


   ColumnIndexType col = 0;
   (*p_table)(0,col++) << _T("Limit State");
   (*p_table)(0,col++) << Sub2(symbol(gamma),_T("DC"));
   (*p_table)(0,col++) << Sub2(symbol(gamma),_T("DW"));
   (*p_table)(0,col++) << Sub2(symbol(gamma),_T("LL"));
   if ( loss_method == pgsTypes::TIME_STEP )
   {
      (*p_table)(0,col++) << Sub2(symbol(gamma),_T("CR"));
      (*p_table)(0,col++) << Sub2(symbol(gamma),_T("SH"));
      (*p_table)(0,col++) << Sub2(symbol(gamma),_T("PS"));
   }
   
   row = 1;

   GET_IFACE2(pBroker,IEventMap,pEventMap);

   if ( bDesign )
   {
      col = 0;
      (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(pgsTypes::ServiceI));
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->DCmax[pgsTypes::ServiceI]);
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->DWmax[pgsTypes::ServiceI]);
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->LLIMmax[pgsTypes::ServiceI]);
      if ( loss_method == pgsTypes::TIME_STEP )
      {
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->CRmax[pgsTypes::ServiceI]);
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->SHmax[pgsTypes::ServiceI]);
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->PSmax[pgsTypes::ServiceI]);
      }
      row++;

      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         col = 0;
         (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(pgsTypes::ServiceIA));
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->DCmax[pgsTypes::ServiceIA]);
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->DWmax[pgsTypes::ServiceIA]);
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->LLIMmax[pgsTypes::ServiceIA]);
         if ( loss_method == pgsTypes::TIME_STEP )
         {
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->CRmax[pgsTypes::ServiceIA]);
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->SHmax[pgsTypes::ServiceIA]);
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->PSmax[pgsTypes::ServiceIA]);
         }
         row++;
      }

      col = 0;
      (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(pgsTypes::ServiceIII));
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->DCmax[pgsTypes::ServiceIII]);
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->DWmax[pgsTypes::ServiceIII]);
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->LLIMmax[pgsTypes::ServiceIII]);
      if ( loss_method == pgsTypes::TIME_STEP )
      {
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->CRmax[pgsTypes::ServiceIII]);
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->SHmax[pgsTypes::ServiceIII]);
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->PSmax[pgsTypes::ServiceIII]);
      }
      row++;

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         col = 0;
         (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(pgsTypes::FatigueI));
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->DCmax[pgsTypes::FatigueI]);
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->DWmax[pgsTypes::FatigueI]);
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->LLIMmax[pgsTypes::FatigueI]);
         if ( loss_method == pgsTypes::TIME_STEP )
         {
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->CRmax[pgsTypes::FatigueI]);
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->SHmax[pgsTypes::FatigueI]);
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->PSmax[pgsTypes::FatigueI]);
         }
         row++;
      }

      col = 0;
      (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(pgsTypes::StrengthI));
      (*p_table)(row,col  ) << scalar.SetValue(pLoadFactors->DCmax[pgsTypes::StrengthI]) << _T("/");
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->DCmin[pgsTypes::StrengthI]);
      (*p_table)(row,col  ) << scalar.SetValue(pLoadFactors->DWmax[pgsTypes::StrengthI]) << _T("/");
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->DWmin[pgsTypes::StrengthI]);
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->LLIMmax[pgsTypes::StrengthI]);
      if ( loss_method == pgsTypes::TIME_STEP )
      {
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->CRmax[pgsTypes::StrengthI]);
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->SHmax[pgsTypes::StrengthI]);
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->PSmax[pgsTypes::StrengthI]);
      }
      row++;

      if ( bPermit )
      {
         col = 0;
         (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(pgsTypes::StrengthII));
         (*p_table)(row,col  ) << scalar.SetValue(pLoadFactors->DCmax[pgsTypes::StrengthII]) << _T("/");
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->DCmin[pgsTypes::StrengthII]);
         (*p_table)(row,col  ) << scalar.SetValue(pLoadFactors->DWmax[pgsTypes::StrengthII]) << _T("/");
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->DWmin[pgsTypes::StrengthII]);
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->LLIMmax[pgsTypes::StrengthII]);
         if ( loss_method == pgsTypes::TIME_STEP )
         {
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->CRmax[pgsTypes::StrengthII]);
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->SHmax[pgsTypes::StrengthII]);
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->PSmax[pgsTypes::StrengthII]);
         }
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
            col = 0;
            pgsTypes::LimitState ls = pgsTypes::ServiceIII_Inventory;
            (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
            Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
            if ( gLL < 0 )
            {
               (*p_table)(row,col++) << _T("*");
               bFootNote = true;
            }
            else
            {
               (*p_table)(row,col++) << scalar.SetValue(gLL);
            }

            if ( loss_method == pgsTypes::TIME_STEP )
            {
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetCreepFactor(ls));
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetShrinkageFactor(ls));
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetSecondaryEffectsFactor(ls));
            }
            row++;
         }

         pgsTypes::LimitState ls = pgsTypes::StrengthI_Inventory;
         col = 0;
         (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(ls));
         (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
         (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
         Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
         if ( gLL < 0 )
         {
            (*p_table)(row,col++) << _T("*");
            bFootNote = true;
         }
         else
         {
            (*p_table)(row,col++) << scalar.SetValue(gLL);
         }

         if ( loss_method == pgsTypes::TIME_STEP )
         {
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetCreepFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetShrinkageFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetSecondaryEffectsFactor(ls));
         }

         row++;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
      {
         if ( pRatingSpec->RateForStress(pgsTypes::lrDesign_Operating) )
         {
            col = 0;
            pgsTypes::LimitState ls = pgsTypes::ServiceIII_Operating;
            (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
            Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
            if ( gLL < 0 )
            {
               (*p_table)(row,col++) << _T("*");
               bFootNote = true;
            }
            else
            {
               (*p_table)(row,col++) << scalar.SetValue(gLL);
            }

            if ( loss_method == pgsTypes::TIME_STEP )
            {
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetCreepFactor(ls));
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetShrinkageFactor(ls));
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetSecondaryEffectsFactor(ls));
            }

            row++;
         }

         pgsTypes::LimitState ls = pgsTypes::StrengthI_Operating;
         col = 0;
         (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(ls));
         (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
         (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
         Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
         if ( gLL < 0 )
         {
            (*p_table)(row,col++) << _T("*");
            bFootNote = true;
         }
         else
         {
            (*p_table)(row,col++) << scalar.SetValue(gLL);
         }

         if ( loss_method == pgsTypes::TIME_STEP )
         {
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetCreepFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetShrinkageFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetSecondaryEffectsFactor(ls));
         }

         row++;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         if ( pRatingSpec->RateForStress(pgsTypes::lrLegal_Routine) )
         {
            col = 0;
            pgsTypes::LimitState ls = pgsTypes::ServiceIII_LegalRoutine;
            (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
            Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
            if ( gLL < 0 )
            {
               (*p_table)(row,col++) << _T("*");
               bFootNote = true;
            }
            else
            {
               (*p_table)(row,col++) << scalar.SetValue(gLL);
            }

            if ( loss_method == pgsTypes::TIME_STEP )
            {
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetCreepFactor(ls));
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetShrinkageFactor(ls));
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetSecondaryEffectsFactor(ls));
            }

            row++;
         }

         pgsTypes::LimitState ls = pgsTypes::StrengthI_LegalRoutine;
         col = 0;
         (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(ls));
         (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
         (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
         Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
         if ( gLL < 0 )
         {
            (*p_table)(row,col++) << _T("*");
            bFootNote = true;
         }
         else
         {
            (*p_table)(row,col++) << scalar.SetValue(gLL);
         }

         if ( loss_method == pgsTypes::TIME_STEP )
         {
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetCreepFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetShrinkageFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetSecondaryEffectsFactor(ls));
         }

         row++;
      }


      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         if ( pRatingSpec->RateForStress(pgsTypes::lrLegal_Special) )
         {
            col = 0;
            pgsTypes::LimitState ls = pgsTypes::ServiceIII_LegalSpecial;
            (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
            Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
            if ( gLL < 0 )
            {
               (*p_table)(row,col++) << _T("*");
               bFootNote = true;
            }
            else
            {
               (*p_table)(row,col++) << scalar.SetValue(gLL);
            }

            if ( loss_method == pgsTypes::TIME_STEP )
            {
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetCreepFactor(ls));
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetShrinkageFactor(ls));
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetSecondaryEffectsFactor(ls));
            }

            row++;
         }

         pgsTypes::LimitState ls = pgsTypes::StrengthI_LegalSpecial;
         col = 0;
         (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(ls));
         (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
         (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
         Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
         if ( gLL < 0 )
         {
            (*p_table)(row,col++) << _T("*");
            bFootNote = true;
         }
         else
         {
            (*p_table)(row,col++) << scalar.SetValue(gLL);
         }

         if ( loss_method == pgsTypes::TIME_STEP )
         {
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetCreepFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetShrinkageFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetSecondaryEffectsFactor(ls));
         }

         row++;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         if ( pRatingSpec->RateForStress(pgsTypes::lrPermit_Routine) )
         {
            col = 0;
            pgsTypes::LimitState ls = pgsTypes::ServiceI_PermitRoutine;
            (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
            Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
            if ( gLL < 0 )
            {
               (*p_table)(row,col++) << _T("*");
               bFootNote = true;
            }
            else
            {
               (*p_table)(row,col++) << scalar.SetValue(gLL);
            }

            if ( loss_method == pgsTypes::TIME_STEP )
            {
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetCreepFactor(ls));
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetShrinkageFactor(ls));
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetSecondaryEffectsFactor(ls));
            }

            row++;
         }

         pgsTypes::LimitState ls = pgsTypes::StrengthII_PermitRoutine;
         col = 0;
         (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(ls));
         (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
         (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
         Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
         if ( gLL < 0 )
         {
            (*p_table)(row,col++) << _T("*");
            bFootNote = true;
         }
         else
         {
            (*p_table)(row,col++) << scalar.SetValue(gLL);
         }

         if ( loss_method == pgsTypes::TIME_STEP )
         {
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetCreepFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetShrinkageFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetSecondaryEffectsFactor(ls));
         }

         row++;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         if ( pRatingSpec->RateForStress(pgsTypes::lrPermit_Special) )
         {
            pgsTypes::LimitState ls = pgsTypes::ServiceI_PermitSpecial;
            col = 0;
            (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
            Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
            if ( gLL < 0 )
            {
               (*p_table)(row,col++) << _T("*");
               bFootNote = true;
            }
            else
            {
               (*p_table)(row,col++) << scalar.SetValue(gLL);
            }

            if ( loss_method == pgsTypes::TIME_STEP )
            {
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetCreepFactor(ls));
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetShrinkageFactor(ls));
               (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetSecondaryEffectsFactor(ls));
            }

            row++;
         }

         pgsTypes::LimitState ls = pgsTypes::StrengthII_PermitSpecial;
         col = 0;
         (*p_table)(row,col++) << OLE2T(pEventMap->GetLimitStateName(ls));
         (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
         (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
         Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
         if ( gLL < 0 )
         {
            (*p_table)(row,col++) << _T("*");
            bFootNote = true;
         }
         else
         {
            (*p_table)(row,col++) << scalar.SetValue(gLL);
         }

         if ( loss_method == pgsTypes::TIME_STEP )
         {
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetCreepFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetShrinkageFactor(ls));
            (*p_table)(row,col++) << scalar.SetValue(pRatingSpec->GetSecondaryEffectsFactor(ls));
         }

         row++;
      }

      if ( bFootNote )
      {
         pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
         *pChapter << pPara;
         *pPara << Super(_T("*")) << _T("Live Load Factor depends on the weight of the axles on the bridge") << rptNewLine;
      }
   }
}

void CLoadingDetailsChapterBuilder::ReportEquivPretensionLoads(rptChapter* pChapter,bool bRating,IBridge* pBridge,IEAFDisplayUnits* pDisplayUnits,const CGirderKey& girderKey) const
{
   if ( m_bSimplifiedVersion || bRating )
   {
      return;
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   INIT_UV_PROTOTYPE( rptLengthUnitValue,         loc,    pDisplayUnits->GetSpanLengthUnit(),     true );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,         moment, pDisplayUnits->GetMomentUnit(),         true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,          force,  pDisplayUnits->GetGeneralForceUnit(),   true );

   // Equivalent prestress loading for camber
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<< _T("Equivalent Pretension Loading")<<rptNewLine<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T("Loads shown in positive directions") << rptNewLine;

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);
   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType firstGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
      GirderIndexType lastGirderIdx  = (girderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx);
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,gdrIdx));
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey thisSegmentKey(grpIdx,gdrIdx,segIdx);

            if ( 1 < nSegments )
            {
               pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
               *pChapter << pPara;
               *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
            }

            pPara = new rptParagraph;
            *pChapter << pPara;

            std::vector<EquivPretensionLoad> vEquivLoad = pProductLoads->GetEquivPretensionLoads(thisSegmentKey,pgsTypes::Straight);
            std::vector<EquivPretensionLoad>::iterator loadIter;
            std::vector<EquivPretensionLoad>::iterator loadIterEnd;
            if ( 0 < vEquivLoad.size() )
            {
               *pPara << Bold(_T("Straight Strands")) << rptNewLine;
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("StraightStrandCamberLoading.gif")) << rptNewLine;
               loadIter    = vEquivLoad.begin();
               loadIterEnd = vEquivLoad.end();
               for ( ; loadIter != loadIterEnd; loadIter++ )
               {
                  EquivPretensionLoad& equivLoad = *loadIter;
                  Float64 X = equivLoad.Xs;
                  Float64 M = equivLoad.M;
                  ATLASSERT(IsZero(equivLoad.P));

                  *pPara << _T("M = ") << moment.SetValue(M) << _T(" at ") << loc.SetValue(X) << rptNewLine;
               }

               *pPara << rptNewLine;
            }

            vEquivLoad = pProductLoads->GetEquivPretensionLoads(thisSegmentKey,pgsTypes::Harped);
            if ( 0 < vEquivLoad.size() )
            {
               *pPara << Bold(_T("Harped Strands")) << rptNewLine;
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("HarpedStrandCamberLoading.gif")) << rptNewLine;
               loadIter    = vEquivLoad.begin();
               loadIterEnd = vEquivLoad.end();
               for ( ; loadIter != loadIterEnd; loadIter++ )
               {
                  EquivPretensionLoad& equivLoad = *loadIter;
                  if ( IsZero(equivLoad.M) )
                  {
                     *pPara << _T("P = ") << force.SetValue(equivLoad.P) << _T(" at ") << loc.SetValue(equivLoad.Xs) << rptNewLine;
                  }
                  else
                  {
                     *pPara << _T("M = ") << moment.SetValue(equivLoad.M) << _T(" at ") << loc.SetValue(equivLoad.Xs) << rptNewLine;
                  }
               }

               *pPara << rptNewLine;
            }
         
            vEquivLoad = pProductLoads->GetEquivPretensionLoads(thisSegmentKey,pgsTypes::Temporary);
            if ( 0 < vEquivLoad.size() )
            {
               *pPara << Bold(_T("Temporary Strands")) << rptNewLine;
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("TempStrandCamberLoading.gif")) << rptNewLine;
               loadIter    = vEquivLoad.begin();
               loadIterEnd = vEquivLoad.end();
               for ( ; loadIter != loadIterEnd; loadIter++ )
               {
                  EquivPretensionLoad& equivLoad = *loadIter;
                  Float64 X = equivLoad.Xs;
                  Float64 M = equivLoad.M;
                  ATLASSERT(IsZero(equivLoad.P));

                  *pPara << _T("M = ") << moment.SetValue(M) << _T(" at ") << loc.SetValue(X) << rptNewLine;
               }
            } // end if
         } // segmentIdx
      } // gdrIdx
   } // groupIdx
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
   if ( slabLoads.size() == 0 )
   {
      return true;
   }

   bool is_panel = deckType==pgsTypes::sdtCompositeSIP ? true : false;

   std::vector<SlabLoad>::const_iterator i1,i2;
   i1 = slabLoads.begin();
   i2 = slabLoads.begin()+1;
   std::vector<SlabLoad>::const_iterator end(slabLoads.end());
   for ( ; i2 != end; i1++, i2++ )
   {
      const SlabLoad& slab_load_1 = *i1;
      const SlabLoad& slab_load_2 = *i2;

      // only takes one difference to be nonuniform
      if ( !IsEqual(slab_load_1.MainSlabLoad,slab_load_2.MainSlabLoad) )
      {
         return false;
      }

      if ( is_panel && !IsEqual(slab_load_1.PanelLoad,slab_load_2.PanelLoad) )
      {
         return false;
      }

      if ( !IsEqual(slab_load_1.PadLoad,slab_load_2.PadLoad) )
      {
         return false;
      }
   }

   return true;
}

static bool IsOverlayLoadUniform(const std::vector<OverlayLoad>& overlayLoads)
{
   if ( overlayLoads.size() == 0 )
   {
      return true;
   }

   std::vector<OverlayLoad>::const_iterator i1,i2;
   i1 = overlayLoads.begin();
   i2 = overlayLoads.begin()+1;
   std::vector<OverlayLoad>::const_iterator end(overlayLoads.end());
   for ( ; i2 != end; i1++, i2++ )
   {
      const OverlayLoad& load_1 = *i1;
      const OverlayLoad& load_2 = *i2;

      // only takes one difference to be nonuniform
      if ( !IsEqual(load_1.wStart,load_2.wStart) )
      {
         return false;
      }

      if ( !IsEqual(load_1.wEnd,load_2.wEnd) )
      {
         return false;
      }
   }

   return true;
}

static bool IsShearKeyLoadUniform(const std::vector<ShearKeyLoad>& loads)
{
   if ( loads.size() == 0 )
   {
      return true;
   }

   std::vector<ShearKeyLoad>::const_iterator i1,i2;
   i1 = loads.begin();
   i2 = loads.begin()+1;
   std::vector<ShearKeyLoad>::const_iterator end(loads.end());
   for ( ; i2 != end; i1++, i2++ )
   {
      const ShearKeyLoad& load_1 = *i1;
      const ShearKeyLoad& load_2 = *i2;

      // only takes one difference to be nonuniform
      if ( !IsEqual(load_1.UniformLoad,load_2.UniformLoad) )
      {
         return false;
      }

      if ( !IsEqual(load_1.StartJointLoad,load_2.StartJointLoad) )
      {
         return false;
      }

      if ( !IsEqual(load_1.EndJointLoad,load_2.EndJointLoad) )
      {
         return false;
      }
   }

   return true;
}

static bool IsConstructionLoadUniform(const std::vector<ConstructionLoad>& loads)
{
   if ( loads.size() == 0 )
   {
      return true;
   }

   std::vector<ConstructionLoad>::const_iterator i1,i2;
   i1 = loads.begin();
   i2 = loads.begin()+1;
   std::vector<ConstructionLoad>::const_iterator end(loads.end());
   for ( ; i2 != end; i1++, i2++ )
   {
      const ConstructionLoad& load_1 = *i1;
      const ConstructionLoad& load_2 = *i2;

      // only takes one difference to be nonuniform
      if ( !IsEqual(load_1.wStart,load_2.wStart) )
      {
         return false;
      }

      if ( !IsEqual(load_1.wEnd,load_2.wEnd) )
      {
         return false;
      }
   }

   return true;
}


