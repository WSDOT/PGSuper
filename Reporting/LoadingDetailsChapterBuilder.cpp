///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>

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
static bool IsLongitudinalJointLoadUniform(const std::vector<LongitudinalJointLoad>& loads);
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

rptChapter* CLoadingDetailsChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGdrRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pGdrLineRptSpec = std::dynamic_pointer_cast<const CGirderLineReportSpecification>(pRptSpec);
   auto pMultiGirderRptSpec = std::dynamic_pointer_cast<const CMultiGirderReportSpecification>(pRptSpec);

   CComPtr<IBroker> pBroker;
   std::vector<CGirderKey> girderKeys;
   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKeys.push_back(pGdrRptSpec->GetGirderKey());
   }
   else if ( pGdrLineRptSpec)
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      CGirderKey girderKey = pGdrLineRptSpec->GetGirderKey();

      GET_IFACE2(pBroker,IBridge,pBridge);
      pBridge->GetGirderline(girderKey, &girderKeys);
   }
   else if ( pMultiGirderRptSpec)
   {
      pMultiGirderRptSpec->GetBroker(&pBroker);
      girderKeys = pMultiGirderRptSpec->GetGirderKeys();
   }
   else
   {
      ATLASSERT(false); // not expecting a different kind of report spec
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   bool bDesign = m_bDesign;
   bool bRating = m_bRating;

   GET_IFACE2(pBroker,IDocumentType, pDocType);
   bool bIsSplicedGirder = (pDocType->IsPGSpliceDocument() ? true : false);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         loc,    pDisplayUnits->GetSpanLengthUnit(),     false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl,    pDisplayUnits->GetForcePerLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,         moment, pDisplayUnits->GetMomentUnit(),         false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,          force,  pDisplayUnits->GetGeneralForceUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         dim,    pDisplayUnits->GetComponentDimUnit(),   false );

   rptParagraph* pPara;
   rptRcTable* p_table;

   // See if any user defined loads
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserDefinedLoads);
   IndexType nPointLoads  = pUserDefinedLoads->GetPointLoadCount();
   IndexType nDistLoads   = pUserDefinedLoads->GetDistributedLoadCount();
   IndexType nMomentLoads = pUserDefinedLoads->GetMomentLoadCount();
   bool bHasUserLoads = (0 < nPointLoads + nDistLoads + nMomentLoads ? true : false);

   bool one_girder_has_shear_key = false;
            
   GET_IFACE2(pBroker,IProductLoads,pProdLoads);
   GET_IFACE2(pBroker,IBridge,pBridge);
   for (const auto& girderKey : girderKeys)
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;
      *pPara << pgsGirderLabel::GetGirderLabel(girderKey);

      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey thisSegmentKey(girderKey,segIdx);

         if ( 1 < nSegments /* && !m_bSimplifiedVersion*/ )
         {
            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            *pChapter << pPara;
            *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
         }

         // uniform loads
         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;
         *pPara << _T("Uniform Loads Applied Along the Entire Girder") << rptNewLine;

         pPara = new rptParagraph;
         *pChapter << pPara;

         p_table = rptStyleManager::CreateDefaultTable(2);
         *pPara << p_table << rptNewLine;

         p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
         p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

         (*p_table)(0,0) << _T("Load Type");
         (*p_table)(0,1) << COLHDR(_T("w"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

         RowIndexType row = p_table->GetNumberOfHeaderRows();

         std::vector<SegmentLoad> segLoad;
         std::vector<DiaphragmLoad> diaLoad;
         std::vector<ClosureJointLoad> cjLoad;
         pProdLoads->GetSegmentSelfWeightLoad(thisSegmentKey,&segLoad,&diaLoad,&cjLoad);
         bool bUniformGirderDeadLoad = (segLoad.size() == 1 && IsEqual(segLoad[0].Wstart,segLoad[0].Wend) ? true : false);

         bool bCJLoad = cjLoad.size() == 0 ? false : true;
         bool bUniformCJDeadLoad = bCJLoad ? (IsEqual(cjLoad[0].Wstart,cjLoad[0].Wend) && IsEqual(cjLoad[1].Wstart,cjLoad[1].Wend)) : true;;

         if ( bUniformGirderDeadLoad )
         {
            // girder load is uniform
            (*p_table)(row,0) << _T("Girder");
            (*p_table)(row++,1) << fpl.SetValue(-segLoad[0].Wstart);
         }

         if ( bCJLoad && bUniformCJDeadLoad )
         {
            (*p_table)(row,0) << _T("Closure Joint");
            (*p_table)(row++,1) << fpl.SetValue(-cjLoad[0].Wstart);
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
            p_table = rptStyleManager::CreateDefaultTable(4,_T("Girder Self-Weight"));
            *pPara << rptNewLine << p_table << rptNewLine;

            (*p_table)(0,0) << COLHDR(_T("Load Start,")<<rptNewLine<<_T("From Left End of Girder"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,1) << COLHDR(_T("Load End,")<<rptNewLine<<_T("From Left End of Girder"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,2) << COLHDR(_T("Start Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            (*p_table)(0,3) << COLHDR(_T("End Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

            row = p_table->GetNumberOfHeaderRows();
            std::vector<SegmentLoad>::iterator iter;
            for ( iter = segLoad.begin(); iter != segLoad.end(); iter++ )
            {
               SegmentLoad& load = *iter;

               (*p_table)(row,0) << loc.SetValue(load.Xstart);
               (*p_table)(row,1) << loc.SetValue(load.Xend);
               (*p_table)(row,2) << fpl.SetValue(-load.Wstart);
               (*p_table)(row,3) << fpl.SetValue(-load.Wend);
               row++;
            }
         }


         if ( bCJLoad && !bUniformCJDeadLoad )
         {
            p_table = rptStyleManager::CreateDefaultTable(4,_T("Closure Joint Self-Weight"));
            *pPara << rptNewLine << p_table << rptNewLine;

            (*p_table)(0,0) << COLHDR(_T("Load Start,")<<rptNewLine<<_T("From Left End of CJ"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,1) << COLHDR(_T("Load End,")<<rptNewLine<<_T("From Left End of CJ"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,2) << COLHDR(_T("Start Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            (*p_table)(0,3) << COLHDR(_T("End Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

            row = p_table->GetNumberOfHeaderRows();
            std::vector<ClosureJointLoad>::iterator iter;
            for ( iter = cjLoad.begin(); iter != cjLoad.end(); iter++ )
            {
               ClosureJointLoad& load = *iter;

               (*p_table)(row,0) << loc.SetValue(load.Xstart);
               (*p_table)(row,1) << loc.SetValue(load.Xend);
               (*p_table)(row,2) << fpl.SetValue(-load.Wstart);
               (*p_table)(row,3) << fpl.SetValue(-load.Wend);
               row++;
            }
         }

         ReportPrecastDiaphragmLoad(pChapter, pBridge, pProdLoads, pDisplayUnits, thisSegmentKey);
         ReportLongitudinalJointLoad(pChapter, pBridge, pProdLoads, pDisplayUnits, thisSegmentKey);
         ReportConstructionLoad(pChapter, pBridge, pProdLoads, pDisplayUnits, thisSegmentKey);
         ReportShearKeyLoad(pChapter, pBridge, pProdLoads, pDisplayUnits, thisSegmentKey, one_girder_has_shear_key);
         ReportSlabLoad(pBroker, pChapter,pBridge,pProdLoads,pDisplayUnits,thisSegmentKey);
         ReportOverlayLoad(     pChapter,pBridge,pProdLoads,pDisplayUnits,bRating,thisSegmentKey);
         ReportPedestrianLoad(pChapter, pBroker, pBridge, pProdLoads, pDisplayUnits, thisSegmentKey);
      } // segIdx

      // user defined loads are span based
      SpanIndexType startSpanIdx, endSpanIdx;
      pBridge->GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);
      for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
      {
         CSpanKey spanKey(spanIdx,girderKey.girderIndex);

         if (bIsSplicedGirder)
         {
            pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
            *pChapter << pPara;
            *pPara << _T("Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
         }

         ReportCastInPlaceDiaphragmLoad(pChapter,pBridge,pProdLoads,pDisplayUnits,spanKey);

         // User Defined Loads
         if ( bHasUserLoads )
         {
            pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
            *pChapter << pPara;
            *pPara<< _T("User Defined Loads")<<rptNewLine;
            pPara = CreatePointLoadTable(pBroker, spanKey, pDisplayUnits, level);
            *pChapter << pPara;
            pPara = CreateDistributedLoadTable(pBroker, spanKey, pDisplayUnits, level);
            *pChapter << pPara;
            pPara = CreateMomentLoadTable(pBroker, spanKey, pDisplayUnits, level);
            *pChapter << pPara;
         }
      }

      ReportEquivPretensionLoads(pChapter,bRating,pBridge,pDisplayUnits,girderKey);
      ReportEquivSegmentPostTensioningLoads(pChapter, bRating, pBridge, pDisplayUnits, girderKey);

   } // girderkey


   bool bPermit;
   ReportLiveLoad(pChapter,bDesign,bRating,pRatingSpec,bPermit);
   ReportLimitStates(pChapter,bDesign,bRating,bPermit,one_girder_has_shear_key,pRatingSpec);

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CLoadingDetailsChapterBuilder::Clone() const
{
   return std::make_unique<CLoadingDetailsChapterBuilder>(m_bSimplifiedVersion,m_bDesign,m_bRating,m_bSelect);
}


void CLoadingDetailsChapterBuilder::ReportPedestrianLoad(rptChapter* pChapter,IBroker* pBroker,IBridge* pBridge,IProductLoads* pProdLoads,IEAFDisplayUnits* pDisplayUnits,const CSegmentKey& thisSegmentKey) const
{
   GET_IFACE2(pBroker,IBarriers,pBarriers);
   rptParagraph* pPara = nullptr;

   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl,    pDisplayUnits->GetForcePerLengthUnit(), false );

   rptRcScalar scalar;
   scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   // Details for sidewalks and barriers
   pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Distribution of Uniform Barrier, Sidewalk, and Pedestrian Loads to Girder") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(4,_T(""));
   *pPara << p_table << rptNewLine;

   p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

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

void CLoadingDetailsChapterBuilder::ReportSlabLoad(IBroker* pBroker, rptChapter* pChapter,IBridge* pBridge,IProductLoads* pProdLoads,IEAFDisplayUnits* pDisplayUnits,const CSegmentKey& thisSegmentKey) const
{
   // slab loads between supports
   rptParagraph* pPara = nullptr;
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         loc,    pDisplayUnits->GetSpanLengthUnit(),     false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl,    pDisplayUnits->GetForcePerLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,         moment, pDisplayUnits->GetMomentUnit(),         false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,          force,  pDisplayUnits->GetGeneralForceUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         comp, pDisplayUnits->GetComponentDimUnit(),     false );
   INIT_UV_PROTOTYPE( rptDensityUnitValue,        density, pDisplayUnits->GetDensityUnit(),       true );

   Float64 end_size = pBridge->GetSegmentStartEndDistance(thisSegmentKey);

   std::_tstring strDeckName( IsNonstructuralDeck(pBridge->GetDeckType()) ? _T("Nonstructural Overlay") : _T("Slab") );
   
   if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
   {
      GET_IFACE2_NOCHECK(pBroker,IIntervals,pIntervals);
      GET_IFACE2_NOCHECK(pBroker,IMaterials,pMaterial);

      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;

      *pPara<< strDeckName << _T(" Load Applied Along Girder")<<rptNewLine;
      
      pgsTypes::SupportedDeckType deck_type = pBridge->GetDeckType();

      std::vector<SlabLoad> slab_loads;
      pProdLoads->GetMainSpanSlabLoad(thisSegmentKey, &slab_loads);

      bool is_uniform = IsSlabLoadUniform(slab_loads, deck_type);

      // Don't eject haunch note unless we have a haunch
      bool do_report_haunch = true;
      rptParagraph* pNotePara = new rptParagraph;
      *pChapter << pNotePara;

      *pNotePara << _T("Tributary width used to compute slab load is measured from top CL girder") << rptNewLine;

      if (is_uniform)
      {
         *pNotePara << strDeckName << _T(" load is uniform along entire girder length.") << rptNewLine;
      }
      else
      {
         *pNotePara << strDeckName << _T(" load is approximated with linear load segments applied along the length of the girder. Segments located outside of bearings are applied as point loads/moments at bearings.") << rptNewLine;
      }

      IndexType deckCastingRegionIdx = 0; // assume region zero to get properties that are common to all castings
      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(deckCastingRegionIdx);
      Float64 deck_density = pMaterial->GetDeckWeightDensity(deckCastingRegionIdx, castDeckIntervalIdx);
      Float64 deck_unit_weight = deck_density; // *WBFL::Units::System::GetGravitationalAcceleration();
      *pNotePara << strDeckName << _T(" unit weight with reinforcement = ") << density.SetValue(deck_unit_weight);

      pPara = new rptParagraph;
      *pChapter << pPara;

      if ( deck_type == pgsTypes::sdtCompositeSIP )
      {
         if (is_uniform)
         {
            rptRcTable* p_table = rptStyleManager::CreateDefaultTable(2,_T(""));
            *pPara << p_table << rptNewLine;
            p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
            p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
            (*p_table)(0,0) << _T("Load Type");
            (*p_table)(0,1) << COLHDR(_T("w"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            RowIndexType row = p_table->GetNumberOfHeaderRows();

            const SlabLoad& slab_load = *(slab_loads.begin());

            (*p_table)(row,0) << _T("Panel Weight");
            (*p_table)(row++,1) << fpl.SetValue(-slab_load.PanelLoad);
            (*p_table)(row,0) << _T("Cast ") << strDeckName << _T(" Weight");
            (*p_table)(row++,1) << fpl.SetValue(-slab_load.MainSlabLoad);
            (*p_table)(row,0) << _T("Haunch Weight");
            (*p_table)(row++,1) << fpl.SetValue(-slab_load.PadLoad);
            (*p_table)(row,0) << _T("Total ") << strDeckName << _T(" Weight");
            (*p_table)(row++,1) << fpl.SetValue(-slab_load.PanelLoad-slab_load.MainSlabLoad-slab_load.PadLoad);
         }
         else
         {
            rptRcTable* p_table = rptStyleManager::CreateDefaultTable(6,_T(""));
            *pPara << p_table;

            (*p_table)(0,0) << COLHDR(_T("Location")<<rptNewLine<<_T("From Left Bearing"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,1) << COLHDR(_T("Panel Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            (*p_table)(0,2) << _T("Cast ") << strDeckName << _T(" Weight");
            (*p_table)(0,3) << COLHDR(_T("Assumed")<<rptNewLine<<_T("Haunch Depth"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
            (*p_table)(0,4) << COLHDR(_T("Haunch Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            (*p_table)(0,5) << COLHDR(_T("Total ") << strDeckName << _T(" Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

            RowIndexType row = p_table->GetNumberOfHeaderRows();
            for( const auto& slab_load : slab_loads)
            {
               Float64 location  = slab_load.Loc - end_size;
               Float64 main_load  = slab_load.MainSlabLoad;
               Float64 panel_load = slab_load.PanelLoad;
               Float64 pad_load   = slab_load.PadLoad;
               (*p_table)(row,0) << loc.SetValue(location);
               (*p_table)(row,1) << fpl.SetValue(-panel_load);
               (*p_table)(row,2) << fpl.SetValue(-main_load);
               (*p_table)(row,3) << comp.SetValue(slab_load.HaunchDepth);
               (*p_table)(row,4) << fpl.SetValue(-pad_load);
               (*p_table)(row,5) << fpl.SetValue(-(panel_load+main_load+pad_load));

               row++;
            }
         }
      }
      else
      {
         if (is_uniform)
         {
            std::set<IndexType> castingRegions;
            for (const auto& slabLoad : slab_loads)
            {
               castingRegions.insert(slabLoad.DeckCastingRegionIdx);
            }
            *pPara << _T("Casting Regions ");
            auto begin = std::begin(castingRegions);
            auto iter = begin;
            auto end = std::end(castingRegions);
            for (; iter != end; iter++)
            {
               IndexType castingRegionIdx = *iter;
               if (iter != begin)
               {
                  *pPara << _T(", ");
               }
               *pPara << LABEL_INDEX(castingRegionIdx);
            }
            *pPara << rptNewLine;


            rptRcTable* p_table = rptStyleManager::CreateDefaultTable(2,_T(""));
            *pPara << p_table << rptNewLine;
            p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
            p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

            (*p_table)(0,0) << _T("Load Type");
            (*p_table)(0,1) << COLHDR(_T("w"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            RowIndexType row = p_table->GetNumberOfHeaderRows();

            const SlabLoad& slab_load = *(slab_loads.begin());

            // Don't report zero slab load in simplified version
            do_report_haunch = !(m_bSimplifiedVersion && IsZero(slab_load.PadLoad));

            if (do_report_haunch)
            {
               (*p_table)(row,0) << _T("Main ") << strDeckName << _T(" Weight");
               (*p_table)(row++,1) << fpl.SetValue(-slab_load.MainSlabLoad);
               (*p_table)(row,0) << _T("Haunch Weight");
               (*p_table)(row++,1) << fpl.SetValue(-slab_load.PadLoad);
            }
            (*p_table)(row,0) << _T("Total ") << strDeckName << _T(" Weight");
            (*p_table)(row++,1) << fpl.SetValue(-slab_load.MainSlabLoad-slab_load.PadLoad);
         }
         else
         {
            rptRcTable* p_table = rptStyleManager::CreateDefaultTable(6,_T(""));
            *pPara << p_table;

            ColumnIndexType col = 0;
            (*p_table)(0, col++) << COLHDR(_T("Location")<<rptNewLine<<_T("From Left Bearing"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0, col++) << _T("Casting") << rptNewLine << _T("Region");
            (*p_table)(0, col++) << COLHDR(_T("Main ") << strDeckName << _T(" Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            (*p_table)(0, col++) << COLHDR(_T("Assumed")<<rptNewLine<<_T("Haunch Depth"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
            (*p_table)(0, col++) << COLHDR(_T("Haunch Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
            (*p_table)(0, col++) << COLHDR(_T("Total ") << strDeckName << _T(" Weight"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

            RowIndexType row = p_table->GetNumberOfHeaderRows();
            for( const auto& slab_load : slab_loads)
            {
               col = 0;
               Float64 location = slab_load.Loc-end_size;
               Float64 main_load = slab_load.MainSlabLoad;
               Float64 pad_load  = slab_load.PadLoad;
               (*p_table)(row, col++) << loc.SetValue(location);
               (*p_table)(row, col++) << LABEL_INDEX(slab_load.DeckCastingRegionIdx);
               (*p_table)(row, col++) << fpl.SetValue(-main_load);
               (*p_table)(row, col++) << comp.SetValue(slab_load.HaunchDepth);
               (*p_table)(row, col++) << fpl.SetValue(-pad_load);
               (*p_table)(row, col++) << fpl.SetValue(-(main_load+pad_load));
               row++;
            }
         }
      } // end if ( pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP )

      if(do_report_haunch)     
      {
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2( pBroker, ISpecification, pSpec );
         if (pgsTypes::hlcAccountForCamber == pSpec->GetHaunchLoadComputationType())
         {
            *pNotePara <<rptNewLine<< _T("Haunch weight includes effects of roadway geometry and is measured along the centerline of the girder. Haunch depth used when computing haunch load is reduced for camber assuming that excess camber is a linear-piecewise parabola defined by the user-input assumed excess camber at mid-span.");
         }
         else
         {
            *pNotePara <<rptNewLine<< _T("Haunch weight includes effects of roadway geometry, and is measured along the centerline of the girder, but does not include a reduction for camber.");
         }
      }

      // the rest of the content is for the non-simplified version (full boat)
      if (!m_bSimplifiedVersion)
      {
         // slab cantilever loads
         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;
         *pPara<< strDeckName << _T(" Cantilever Loads")<<rptNewLine;
      
         pPara = new rptParagraph;
         *pChapter << pPara;

         rptRcTable* p_table = rptStyleManager::CreateDefaultTable(5,_T(""));
         *pPara << p_table;

         p_table->SetNumberOfHeaderRows(2);
         p_table->SetRowSpan(0,0,2);
         (*p_table)(0,0) << _T("Location");

         p_table->SetColumnSpan(0,1,2);
         (*p_table)(0,1) << _T("Main ") << strDeckName;
         (*p_table)(1,1) << COLHDR(_T("Point Load"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
         (*p_table)(1,2) << COLHDR(_T("Point Moment"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

         p_table->SetColumnSpan(0,3,2);
         (*p_table)(0,3) << _T("Haunch");
         (*p_table)(1,3) << COLHDR(_T("Point Load"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
         (*p_table)(1,4) << COLHDR(_T("Point Moment"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

         (*p_table)(2,0) << _T("Back Bearing");
         (*p_table)(3,0) << _T("Ahead Bearing");

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

         p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
         p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      }

      if (do_report_haunch && !is_uniform)
      {
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,ISpecification,pSpec);

         bool report_camber = pSpec->GetHaunchLoadComputationType() == pgsTypes::hlcAccountForCamber;

         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;
         *pPara << strDeckName << _T(" Haunch Load Details") << rptNewLine;

         rptRcTable* p_table = rptStyleManager::CreateDefaultTable(report_camber ? 11:10,_T(""));
         *pPara << p_table;

         ColumnIndexType col = 0;
         (*p_table)(0, col++) << COLHDR(_T("Location")<<rptNewLine<<_T("From Left Bearing"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*p_table)(0, col++) << _T("Station");
         (*p_table)(0, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
         (*p_table)(0, col++) << _T("Casting") << rptNewLine << _T("Region");
         (*p_table)(0, col++) << COLHDR(_T("Top") << rptNewLine << _T("Slab") << rptNewLine << _T("Elevation"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
         (*p_table)(0, col++) << COLHDR(_T("Girder") << rptNewLine << _T("Chord") << rptNewLine << _T("Elevation"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
         (*p_table)(0, col++) << COLHDR(_T("Top") << rptNewLine << _T("Girder") << rptNewLine << _T("Elevation"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
         (*p_table)(0, col++) << COLHDR(strDeckName<<rptNewLine<<_T("Thickness"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         if (report_camber)
         {
            (*p_table)(0, col++) << COLHDR(_T("*Assumed") << rptNewLine << _T("Excess") << rptNewLine << _T("Camber"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         (*p_table)(0,col++) << COLHDR(_T("Assumed")<<rptNewLine<<_T("Haunch")<<rptNewLine<<_T("Depth"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         (*p_table)(0,col++) << COLHDR(_T("Haunch")<<rptNewLine<<_T("Load"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

         RowIndexType row = p_table->GetNumberOfHeaderRows();
         for ( const auto& slab_load : slab_loads)
         {
            Float64 location = slab_load.Loc - end_size;

            col = 0;
            (*p_table)(row, col++) << loc.SetValue(location);
            (*p_table)(row, col++) << rptRcStation(slab_load.Station, &pDisplayUnits->GetStationFormat());
            (*p_table)(row, col++) << RPT_OFFSET(slab_load.Offset, loc);
            (*p_table)(row, col++) << LABEL_INDEX(slab_load.DeckCastingRegionIdx);
            (*p_table)(row, col++) << loc.SetValue(slab_load.TopSlabElevation);
            (*p_table)(row, col++) << loc.SetValue(slab_load.GirderChordElevation);
            (*p_table)(row, col++) << loc.SetValue(slab_load.TopGirderElevation);
            (*p_table)(row, col++) << comp.SetValue(slab_load.SlabDepth);
            if (report_camber)
            {
               (*p_table)(row, col++) << comp.SetValue(slab_load.AssumedExcessCamber);
            }
            (*p_table)(row, col++) << comp.SetValue(slab_load.HaunchDepth);
            (*p_table)(row, col++) << fpl.SetValue(-slab_load.PadLoad);
            row++;
         }

         if (report_camber)
         {
            Float64 factor = pSpec->GetHaunchLoadCamberFactor();

            pNotePara = new rptParagraph;
            *pChapter << pNotePara;
            *pNotePara << _T("Haunch load calculation based on haunch depth at CL girder.") << rptNewLine;
            *pNotePara << _T("* Factor of ") << factor*100.0 << _T("% applied to assumed excess camber per project criteria.");
         }
      }
   } // end if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
}

void CLoadingDetailsChapterBuilder::ReportOverlayLoad(rptChapter* pChapter,IBridge* pBridge,IProductLoads* pProdLoads,IEAFDisplayUnits* pDisplayUnits,bool bRating,const CSegmentKey& thisSegmentKey) const
{
   // slab loads between supports
   rptParagraph* pPara = nullptr;
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
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
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

         rptRcTable* p_table = rptStyleManager::CreateDefaultTable(3,_T(""));
         *pPara << p_table << rptNewLine;
         p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
         p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
         (*p_table)(0,0) << _T("Load Type");
         if (olayd==pgsTypes::olDistributeTributaryWidth)
         {
            (*p_table)(0,1) << COLHDR(Sub2(_T("W"),_T("trib")),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         }
         else
         {
            (*p_table)(0,1) << COLHDR(Sub2(_T("W"),_T("cc")),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         }
         (*p_table)(0,2) << COLHDR(_T("w"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
         RowIndexType row = p_table->GetNumberOfHeaderRows();

         const OverlayLoad& ovl_load = *(overlay_loads.begin());

         (*p_table)(row,0) << _T("Overlay Weight");
         (*p_table)(row,1) << loc.SetValue(ovl_load.StartWcc);
         (*p_table)(row++,2) << fpl.SetValue(-ovl_load.Wstart);
      }
      else
      {
         rptRcTable* p_table = rptStyleManager::CreateDefaultTable(6,_T(""));
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
            Float64 x1 = load.Xstart - end_size;
            Float64 x2 = load.Xend   - end_size;
            Float64 Wcc1 = load.StartWcc;
            Float64 Wcc2 = load.EndWcc;
            Float64 w1   = load.Wstart;
            Float64 w2   = load.Wend;

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

   rptParagraph* pPara = nullptr;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // construction load
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoads);
   if ( !IsZero(pUserLoads->GetConstructionLoad()) )
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
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

         rptRcTable* p_table = rptStyleManager::CreateDefaultTable(2,_T(""));
         *pPara << p_table << rptNewLine;
         p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
         p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
         (*p_table)(0,0) << _T("Load Type");
         (*p_table)(0,1) << COLHDR(_T("w"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
         RowIndexType row = p_table->GetNumberOfHeaderRows();

         const ConstructionLoad& cnst_load = *(construction_loads.begin());

         (*p_table)(row,0) << _T("Construction Weight");
         (*p_table)(row++,1) << fpl.SetValue(-cnst_load.Wstart);
      }
      else
      {
         rptRcTable* p_table = rptStyleManager::CreateDefaultTable(6,_T(""));
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
            Float64 x1 = load.Xstart - end_size;
            Float64 x2 = load.Xend   - end_size;
            Float64 Wcc1 = load.StartWcc;
            Float64 Wcc2 = load.EndWcc;
            Float64 w1   = load.Wstart;
            Float64 w2   = load.Wend;

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

void CLoadingDetailsChapterBuilder::ReportLongitudinalJointLoad(rptChapter* pChapter, IBridge* pBridge, IProductLoads* pProdLoads, IEAFDisplayUnits* pDisplayUnits, const CSegmentKey& thisSegmentKey) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, loc, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptForcePerLengthUnitValue, fpl, pDisplayUnits->GetForcePerLengthUnit(), false);

   Float64 end_size = pBridge->GetSegmentStartEndDistance(thisSegmentKey);

   rptParagraph* pPara = nullptr;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if (pBridgeDesc->HasStructuralLongitudinalJoints())
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;
      *pPara << _T("Longitudinal Joints") << rptNewLine;
      pPara = new rptParagraph;
      *pChapter << pPara;

      std::vector<LongitudinalJointLoad> loads;
      pProdLoads->GetLongitudinalJointLoad(thisSegmentKey, &loads);

      bool is_uniform = IsLongitudinalJointLoadUniform(loads);

      if (is_uniform)
      {
         *pPara << _T("Longitudinal Joint load is uniform along entire girder length.") << rptNewLine;

         rptRcTable* p_table = rptStyleManager::CreateDefaultTable(2, _T(""));
         *pPara << p_table << rptNewLine;
         p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
         p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
         (*p_table)(0, 0) << _T("Load Type");
         (*p_table)(0, 1) << COLHDR(_T("w"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());
         RowIndexType row = p_table->GetNumberOfHeaderRows();

         const LongitudinalJointLoad& load = loads.front();

         (*p_table)(row, 0) << _T("Weight");
         (*p_table)(row++, 1) << fpl.SetValue(-load.Wstart);
      }
      else
      {
         rptRcTable* p_table = rptStyleManager::CreateDefaultTable(4, _T(""));
         *pPara << p_table;

         (*p_table)(0, 0) << COLHDR(_T("Load Start,") << rptNewLine << _T("From Left Bearing"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
         (*p_table)(0, 1) << COLHDR(_T("Load End,") << rptNewLine << _T("From Left Bearing"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
         (*p_table)(0, 2) << COLHDR(_T("Start Weight"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());
         (*p_table)(0, 3) << COLHDR(_T("End Weight"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());


         RowIndexType row = p_table->GetNumberOfHeaderRows();
         for( const auto& load : loads)
         {
            Float64 x1 = load.Xstart - end_size;
            Float64 x2 = load.Xend - end_size;
            Float64 w1 = load.Wstart;
            Float64 w2 = load.Wend;

            (*p_table)(row, 0) << loc.SetValue(x1);
            (*p_table)(row, 1) << loc.SetValue(x2);
            (*p_table)(row, 2) << fpl.SetValue(-w1);
            (*p_table)(row, 3) << fpl.SetValue(-w2);

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

   rptParagraph* pPara = nullptr;

   // Shear key loads
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   bool has_shear_key = pGirder->HasShearKey(thisSegmentKey, spacingType);
   if ( has_shear_key )
   {
      one_girder_has_shear_key = true;
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
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

         rptRcTable* p_table = rptStyleManager::CreateDefaultTable(2,_T(""));
         *pPara << p_table << rptNewLine;
         p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
         p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
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

         rptRcTable* p_table = rptStyleManager::CreateDefaultTable(4,_T(""));
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
      rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<< _T("Precast Diaphragms Constructed as part of the Girder")<<rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      rptRcTable* p_table = rptStyleManager::CreateDefaultTable(5,_T(""));
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
   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc,    pDisplayUnits->GetSpanLengthUnit(),   false);
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetGeneralForceUnit(), false);
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(),       false);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dist,   pDisplayUnits->GetSpanLengthUnit(),   false);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,    pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE(rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), false);
   INIT_UV_PROTOTYPE(rptDensityUnitValue, density, pDisplayUnits->GetDensityUnit(), false);

   std::vector<DiaphragmLoad> diap_loads; // these are the actual loads that have been generated from the user input/diaphragm rules
   pProdLoads->GetIntermediateDiaphragmLoads(spanKey, &diap_loads);
   std::sort(diap_loads.begin(),diap_loads.end());

   if (0 < diap_loads.size() )
   {
      rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<< _T("Intermediate Diaphragms Constructed at Bridge Site")<<rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      rptRcTable* p_table = rptStyleManager::CreateDefaultTable(5,_T(""));
      *pPara << p_table;

      (*p_table)(0,0) << COLHDR(_T("Load Location,")<<rptNewLine<<_T("from Start of Span"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,1) << COLHDR(_T("H"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
      (*p_table)(0,2) << COLHDR(_T("W"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
      (*p_table)(0,3) << COLHDR(_T("T"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
      (*p_table)(0,4) << COLHDR(_T("Load"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );

      // this is the raw input from the bridge model
      std::vector<IntermedateDiaphragm> diaphragms = pBridge->GetCastInPlaceDiaphragms(spanKey);
      std::sort(diaphragms.begin(),diaphragms.end());
      std::vector<IntermedateDiaphragm>::iterator iter = diaphragms.begin();

      RowIndexType row = p_table->GetNumberOfHeaderRows();
      for (std::vector<DiaphragmLoad>::iterator id = diap_loads.begin(); id!=diap_loads.end(); id++, iter++)
      {
         DiaphragmLoad& rload = *id;
         IntermedateDiaphragm& dia = *iter;

         ATLASSERT(IsEqual(rload.Loc,dia.Location));

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
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<< _T("Pier Diaphragm Loads")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(10,_T(""));
   *pPara << p_table << rptNewLine;
   *pPara << _T("Diaphragm weight, P = (Unit Weight)(H)(W)(Trib Width)/cos(Skew)") << rptNewLine;

   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType colIdx = 0;
   (*p_table)(0, colIdx++) << _T("Pier");
   (*p_table)(0, colIdx++) << _T("Location");
   (*p_table)(0, colIdx++) << COLHDR(_T("Unit Weight"), rptDensityUnitTag, pDisplayUnits->GetDensityUnit());
   (*p_table)(0, colIdx++) << COLHDR(_T("H"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*p_table)(0, colIdx++) << COLHDR(_T("W"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*p_table)(0, colIdx++) << COLHDR(_T("Trib. Width"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*p_table)(0, colIdx++) << COLHDR(_T("Skew"), rptAngleUnitTag, pDisplayUnits->GetAngleUnit());
   (*p_table)(0, colIdx++) << COLHDR(_T("P"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*p_table)(0, colIdx++) << COLHDR(_T("Moment") << rptNewLine << _T("Arm"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*p_table)(0, colIdx++) << COLHDR(_T("M"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   SpanIndexType nSpans = pBridge->GetSpanCount();
   PierIndexType nPiers = (PierIndexType)(nSpans + 1);
   SpanIndexType startSpanIdx = (spanKey.spanIndex == ALL_SPANS ? 0 : spanKey.spanIndex);
   SpanIndexType endSpanIdx   = (spanKey.spanIndex == ALL_SPANS ? nSpans-1 : startSpanIdx);
   PierIndexType startPierIdx = (PierIndexType)startSpanIdx;
   PierIndexType endPierIdx   = (PierIndexType)(endSpanIdx+1);
   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      PIER_DIAPHRAGM_LOAD_DETAILS backSide, aheadSide;
      pProdLoads->GetPierDiaphragmLoads(pierIdx, spanKey.girderIndex, &backSide, &aheadSide);

      if (0 < pierIdx && pierIdx < nPiers - 1)
      {
         p_table->SetRowSpan(row, 0, 2);
      }

      colIdx = 0;
      (*p_table)(row, colIdx++) << LABEL_PIER(pierIdx);

      if (0 < pierIdx)
      {
         (*p_table)(row, colIdx++) << _T("Back Bearing");
         (*p_table)(row, colIdx++) << density.SetValue(backSide.Density);
         (*p_table)(row, colIdx++) << dist.SetValue(backSide.Height);
         (*p_table)(row, colIdx++) << dist.SetValue(backSide.Width);
         (*p_table)(row, colIdx++) << dist.SetValue(backSide.TribWidth);
         (*p_table)(row, colIdx++) << angle.SetValue(backSide.SkewAngle);
         (*p_table)(row, colIdx++) << force.SetValue(-backSide.P);
         (*p_table)(row, colIdx++) << dist.SetValue(backSide.MomentArm);
         (*p_table)(row, colIdx++) << moment.SetValue(backSide.M);

         row++;
      }


      if (pierIdx < nPiers - 1)
      {
         colIdx = 1;
         (*p_table)(row, colIdx++) << _T("Ahead Bearing");
         (*p_table)(row, colIdx++) << density.SetValue(aheadSide.Density);
         (*p_table)(row, colIdx++) << dist.SetValue(aheadSide.Height);
         (*p_table)(row, colIdx++) << dist.SetValue(aheadSide.Width);
         (*p_table)(row, colIdx++) << dist.SetValue(aheadSide.TribWidth);
         (*p_table)(row, colIdx++) << angle.SetValue(aheadSide.SkewAngle);
         (*p_table)(row, colIdx++) << force.SetValue(-aheadSide.P);
         (*p_table)(row, colIdx++) << dist.SetValue(aheadSide.MomentArm);
         (*p_table)(row, colIdx++) << moment.SetValue(aheadSide.M);

         row++;
      }
   }
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

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
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

      if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
      {
         std::vector<std::_tstring> legalEmergencyLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Emergency);
         if (legalEmergencyLiveLoads.empty())
         {
            *pPara << _T("No live loads were applied to the legal load rating, emergency vehicles (Service III and Strength I) limit states") << rptNewLine;
         }
         else
         {
            *pPara << _T("The following live loads were applied to the legal load rating, emergency vehicles (Service III and Strength I) limit states:") << rptNewLine;

            std::vector<std::_tstring>::iterator iter;
            for (iter = legalEmergencyLiveLoads.begin(); iter != legalEmergencyLiveLoads.end(); iter++)
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
   scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
   scalar.SetTolerance(1.0e-6);

   // LRFD Limit States
   GET_IFACE2(pBroker,ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

   GET_IFACE2(pBroker,ILossParameters,pLossParameters);
   pgsTypes::LossMethod loss_method = pLossParameters->GetLossMethod();

   // LRFD Limit States Load Factors
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<< _T("Limit State Load Factors")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   ColumnIndexType nColumns = 4;
   if ( loss_method == pgsTypes::TIME_STEP )
   {
      nColumns = 7;
   }

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nColumns);
   p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
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
   
   RowIndexType row = 1;

   if ( bDesign )
   {
      col = 0;
      (*p_table)(row,col++) << GetLimitStateString(pgsTypes::ServiceI);
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetDCMax(pgsTypes::ServiceI));
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetDWMax(pgsTypes::ServiceI));
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetLLIMMax(pgsTypes::ServiceI));
      if ( loss_method == pgsTypes::TIME_STEP )
      {
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetCRMax(pgsTypes::ServiceI));
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetSHMax(pgsTypes::ServiceI));
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetPSMax(pgsTypes::ServiceI));
      }
      row++;

      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         col = 0;
         (*p_table)(row,col++) << GetLimitStateString(pgsTypes::ServiceIA);
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetDCMax(pgsTypes::ServiceIA));
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetDWMax(pgsTypes::ServiceIA));
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetLLIMMax(pgsTypes::ServiceIA));
         if ( loss_method == pgsTypes::TIME_STEP )
         {
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetCRMax(pgsTypes::ServiceIA));
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetSHMax(pgsTypes::ServiceIA));
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetPSMax(pgsTypes::ServiceIA));
         }
         row++;
      }

      col = 0;
      (*p_table)(row,col++) << GetLimitStateString(pgsTypes::ServiceIII);
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetDCMax(pgsTypes::ServiceIII));
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetDWMax(pgsTypes::ServiceIII));
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetLLIMMax(pgsTypes::ServiceIII));
      if ( loss_method == pgsTypes::TIME_STEP )
      {
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetCRMax(pgsTypes::ServiceIII));
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetSHMax(pgsTypes::ServiceIII));
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetPSMax(pgsTypes::ServiceIII));
      }
      row++;

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         col = 0;
         (*p_table)(row,col++) << GetLimitStateString(pgsTypes::FatigueI);
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetDCMax(pgsTypes::FatigueI));
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetDWMax(pgsTypes::FatigueI));
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetLLIMMax(pgsTypes::FatigueI));
         if ( loss_method == pgsTypes::TIME_STEP )
         {
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetCRMax(pgsTypes::FatigueI));
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetSHMax(pgsTypes::FatigueI));
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetPSMax(pgsTypes::FatigueI));
         }
         row++;
      }

      col = 0;
      (*p_table)(row,col++) << GetLimitStateString(pgsTypes::StrengthI);
      (*p_table)(row,col  ) << scalar.SetValue(pLoadFactors->GetDCMax(pgsTypes::StrengthI)) << _T("/");
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetDCMin(pgsTypes::StrengthI));
      (*p_table)(row,col  ) << scalar.SetValue(pLoadFactors->GetDWMax(pgsTypes::StrengthI)) << _T("/");
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetDWMin(pgsTypes::StrengthI));
      (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetLLIMMax(pgsTypes::StrengthI));
      if ( loss_method == pgsTypes::TIME_STEP )
      {
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetCRMax(pgsTypes::StrengthI));
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetSHMax(pgsTypes::StrengthI));
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetPSMax(pgsTypes::StrengthI));
      }
      row++;

      if ( bPermit )
      {
         col = 0;
         (*p_table)(row,col++) << GetLimitStateString(pgsTypes::StrengthII);
         (*p_table)(row,col  ) << scalar.SetValue(pLoadFactors->GetDCMax(pgsTypes::StrengthII)) << _T("/");
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetDCMin(pgsTypes::StrengthII));
         (*p_table)(row,col  ) << scalar.SetValue(pLoadFactors->GetDWMax(pgsTypes::StrengthII)) << _T("/");
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetDWMin(pgsTypes::StrengthII));
         (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetLLIMMax(pgsTypes::StrengthII));
         if ( loss_method == pgsTypes::TIME_STEP )
         {
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetCRMax(pgsTypes::StrengthII));
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetSHMax(pgsTypes::StrengthII));
            (*p_table)(row,col++) << scalar.SetValue(pLoadFactors->GetPSMax(pgsTypes::StrengthII));
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
            (*p_table)(row,col++) << GetLimitStateString(ls);
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
         (*p_table)(row,col++) << GetLimitStateString(ls);
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
            (*p_table)(row,col++) << GetLimitStateString(ls);
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
         (*p_table)(row,col++) << GetLimitStateString(ls);
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
            (*p_table)(row,col++) << GetLimitStateString(ls);
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
         (*p_table)(row,col++) << GetLimitStateString(ls);
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
            (*p_table)(row,col++) << GetLimitStateString(ls);
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
         (*p_table)(row,col++) << GetLimitStateString(ls);
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

      if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
      {
         if (pRatingSpec->RateForStress(pgsTypes::lrLegal_Emergency))
         {
            col = 0;
            pgsTypes::LimitState ls = pgsTypes::ServiceIII_LegalEmergency;
            (*p_table)(row, col++) << GetLimitStateString(ls);
            (*p_table)(row, col++) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
            (*p_table)(row, col++) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
            Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls, true);
            if (gLL < 0)
            {
               (*p_table)(row, col++) << _T("*");
               bFootNote = true;
            }
            else
            {
               (*p_table)(row, col++) << scalar.SetValue(gLL);
            }

            if (loss_method == pgsTypes::TIME_STEP)
            {
               (*p_table)(row, col++) << scalar.SetValue(pRatingSpec->GetCreepFactor(ls));
               (*p_table)(row, col++) << scalar.SetValue(pRatingSpec->GetShrinkageFactor(ls));
               (*p_table)(row, col++) << scalar.SetValue(pRatingSpec->GetSecondaryEffectsFactor(ls));
            }

            row++;
         }

         pgsTypes::LimitState ls = pgsTypes::StrengthI_LegalEmergency;
         col = 0;
         (*p_table)(row, col++) << GetLimitStateString(ls);
         (*p_table)(row, col++) << scalar.SetValue(pRatingSpec->GetDeadLoadFactor(ls));
         (*p_table)(row, col++) << scalar.SetValue(pRatingSpec->GetWearingSurfaceFactor(ls));
         Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls, true);
         if (gLL < 0)
         {
            (*p_table)(row, col++) << _T("*");
            bFootNote = true;
         }
         else
         {
            (*p_table)(row, col++) << scalar.SetValue(gLL);
         }

         if (loss_method == pgsTypes::TIME_STEP)
         {
            (*p_table)(row, col++) << scalar.SetValue(pRatingSpec->GetCreepFactor(ls));
            (*p_table)(row, col++) << scalar.SetValue(pRatingSpec->GetShrinkageFactor(ls));
            (*p_table)(row, col++) << scalar.SetValue(pRatingSpec->GetSecondaryEffectsFactor(ls));
         }

         row++;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         if ( pRatingSpec->RateForStress(pgsTypes::lrPermit_Routine) )
         {
            col = 0;
            pgsTypes::LimitState ls = pgsTypes::ServiceI_PermitRoutine;
            (*p_table)(row,col++) << GetLimitStateString(ls);
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
         (*p_table)(row,col++) << GetLimitStateString(ls);
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
            (*p_table)(row,col++) << GetLimitStateString(ls);
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
         (*p_table)(row,col++) << GetLimitStateString(ls);
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
         pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
         *pChapter << pPara;
         *pPara << Super(_T("*")) << _T("Live Load Factor depends on the weight of the axles on the bridge") << rptNewLine;
      }
   }
}

std::_tstring GetImageName(LPCTSTR lpszBase, bool bAsymmetric, bool bPrecamber, bool bTopFlangeThickening)
{
   std::_tstring strName(lpszBase);
   strName += (bAsymmetric ? _T("_Asymmetric") : _T("_Symmetric"));
   if (bPrecamber)
   {
      strName += _T("_Precamber");
   }

   if (bTopFlangeThickening)
   {
      strName += _T("_TopFlangeThickening");
   }

   strName += _T(".png");
   return strName;
}

void CLoadingDetailsChapterBuilder::ReportEquivPretensionLoads(rptChapter* pChapter,bool bRating,IBridge* pBridge,IEAFDisplayUnits* pDisplayUnits,const CGirderKey& girderKey) const
{
   if ( m_bSimplifiedVersion || bRating )
   {
      return;
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2_NOCHECK(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2_NOCHECK(pBroker, IBridgeDescription, pIBridgeDesc);
   GET_IFACE2_NOCHECK(pBroker,IStrandGeometry, pStrandGeom);

   bool bHasAsymmetricGirders = pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing();

   INIT_UV_PROTOTYPE( rptLengthUnitValue,         loc,    pDisplayUnits->GetSpanLengthUnit(),     true );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,         moment, pDisplayUnits->GetMomentUnit(),         true );
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, ecc, pDisplayUnits->GetComponentDimUnit(), true);
   INIT_UV_PROTOTYPE(rptForcePerLengthUnitValue, distributed, pDisplayUnits->GetForcePerLengthUnit(), true);

   // Equivalent prestress loading for camber
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<< _T("Equivalent Pretension Loading")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);

   // First do a check to see if we are reporting anything
   bool is_strands = false;
   for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++)
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType firstGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
      GirderIndexType lastGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? nGirders - 1 : firstGirderIdx);
      for (GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++)
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx, gdrIdx));
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            CSegmentKey thisSegmentKey(grpIdx, gdrIdx, segIdx);
            StrandIndexType ns = pStrandGeom->GetStrandCount(thisSegmentKey, pgsTypes::Permanent) + pStrandGeom->GetStrandCount(thisSegmentKey, pgsTypes::Temporary);
            if (0 < ns)
            {
               is_strands = true;
               break;
            }
         }
      }
   }

   if (!is_strands)
   {
      *pPara << _T("No loads were applied becase there are no pretensioning strands.") << rptNewLine;
      return;
   }


   *pPara << _T("These loads are used to determine girder deflections due to pretension forces") << rptNewLine;
   *pPara << _T("Loads and eccentricities shown in positive directions") << rptNewLine;

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
               pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
               *pChapter << pPara;
               *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
            }

            pPara = new rptParagraph;
            *pChapter << pPara;

            std::vector<EquivPretensionLoad> vEquivLoad = pProductLoads->GetEquivPretensionLoads(thisSegmentKey,pgsTypes::Straight);
            if ( 0 < vEquivLoad.size() )
            {
               const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(thisSegmentKey);
               bool bPrecamber = !IsZero(pSegment->Precamber);
               bool bTopFlangeThickening = pSegment->TopFlangeThickeningType != pgsTypes::tftNone;
               bool bHasDebonding = pStrandGeom->HasDebonding(thisSegmentKey);
               std::_tstring strImage = GetImageName(_T("StraightStrandEquivPSLoading"),bHasAsymmetricGirders,bPrecamber,bTopFlangeThickening);

               *pPara << Bold(_T("Straight Strands")) << rptNewLine;
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + strImage) << rptNewLine;
               for ( const auto& equivLoad : vEquivLoad)
               {
                  ATLASSERT(IsZero(equivLoad.N));

                  *pPara << Sub2(_T("M"), _T("x")) << _T(" = ");
                  *pPara << _T("(") << force.SetValue(equivLoad.P) << _T(")(") << ecc.SetValue(equivLoad.eye) << _T(")");
                  *pPara << _T(" = ") << moment.SetValue(equivLoad.Mx) << _T(" at ") << loc.SetValue(equivLoad.Xs) << rptNewLine;

                  if (bHasAsymmetricGirders)
                  {
                     *pPara << Sub2(_T("M"), _T("y")) << _T(" = (") << force.SetValue(equivLoad.P) << _T(")(") << ecc.SetValue(equivLoad.ex) << _T(")");
                     *pPara << _T(" = ") << moment.SetValue(equivLoad.My) << _T(" at ") << loc.SetValue(equivLoad.Xs) << rptNewLine;
                  }
               }

               if (bHasDebonding)
               {
                  *pPara << _T("* Eccentricities at debond locations are that of only those strands debonded at the location.") << rptNewLine;
               }

               *pPara << rptNewLine;
            }
            else
            {
               *pPara << _T("No strands in this segment.") << rptNewLine;
            }

            vEquivLoad = pProductLoads->GetEquivPretensionLoads(thisSegmentKey,pgsTypes::Harped);
            if ( 0 < vEquivLoad.size() )
            {
               const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(thisSegmentKey);
               pgsTypes::AdjustableStrandType adj_type = pSegment->Strands.GetAdjustableStrandType();
               if (pgsTypes::asHarped == adj_type)
               {
                  *pPara << Bold(_T("Harped Strands")) << rptNewLine;
               }
               else
               {
                  *pPara << Bold(_T("Adjustable Straight Strands")) << rptNewLine;
               }

               bool bPrecamber = !IsZero(pSegment->Precamber);
               bool bTopFlangeThickening = pSegment->TopFlangeThickeningType != pgsTypes::tftNone;
               std::_tstring strImage = GetImageName(_T("HarpedStrandEquivPSLoading"), bHasAsymmetricGirders, bPrecamber, bTopFlangeThickening);
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + strImage) << rptNewLine;

               for(const auto& equivLoad : vEquivLoad)
               {
                  if ( IsZero(equivLoad.Mx) )
                  {
                     *pPara << Sub2(_T("e'"), _T("y")) << _T(" = ");
                     *pPara << _T("(") << ecc.SetValue(equivLoad.eyh) << _T(" - ");
                     *pPara << ecc.SetValue(equivLoad.eye);
                     if (bPrecamber)
                     {
                        *pPara << _T(" - ") << ecc.SetValue(equivLoad.PrecamberAtLoadPoint);
                     }
                     *pPara << _T(")");
                     *pPara << _T(" = ") << ecc.SetValue(equivLoad.eprime) << rptNewLine;
                     *pPara << _T("N = (") << force.SetValue(equivLoad.P) << _T(")(") << ecc.SetValue(equivLoad.eprime) << _T(")/(") << loc.SetValue(equivLoad.b) << _T(") = ");
                     *pPara << force.SetValue(equivLoad.N) << _T(" at ") << loc.SetValue(equivLoad.Xs) << rptNewLine;
                  }
                  else
                  {
                     *pPara << Sub2(_T("M"), _T("x")) << _T(" = ");
                     *pPara << _T("(") << force.SetValue(equivLoad.P) << _T(")(") << ecc.SetValue(equivLoad.eye) << _T(")");
                     *pPara << _T(" = ") << moment.SetValue(equivLoad.Mx) << _T(" at ") << loc.SetValue(equivLoad.Xs) << rptNewLine;
                  }

                  if (!IsZero(equivLoad.wy))
                  {
                     *pPara << Sub2(_T("w"), _T("y")) << _T(" = 8(") << force.SetValue(equivLoad.P) << _T(")(") << ecc.SetValue(equivLoad.Precamber) << _T(")/");
                     *pPara << _T("(") << loc.SetValue(equivLoad.Ls) << Super2(_T(")"), _T("2")) << _T(" = ");
                     *pPara << distributed.SetValue(equivLoad.wy) << _T(" from ") << loc.SetValue(equivLoad.Xs);
                     *pPara << _T(" to ") << loc.SetValue(equivLoad.Xe) << rptNewLine;
                  }

                  if (bHasAsymmetricGirders)
                  {
                     *pPara << Sub2(_T("M"), _T("y")) << _T(" = (") << force.SetValue(equivLoad.P) << _T(")(") << ecc.SetValue(equivLoad.ex) << _T(")");
                     *pPara << _T(" = ") << moment.SetValue(equivLoad.My) << _T(" at ") << loc.SetValue(equivLoad.Xs) << rptNewLine;
                  }
               }

               *pPara << rptNewLine;
            }
         
            vEquivLoad = pProductLoads->GetEquivPretensionLoads(thisSegmentKey,pgsTypes::Temporary);
            if ( 0 < vEquivLoad.size() )
            {
               const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(thisSegmentKey);
               bool bPrecamber = !IsZero(pSegment->Precamber);
               bool bTopFlangeThickening = pSegment->TopFlangeThickeningType != pgsTypes::tftNone;
               std::_tstring strImage = GetImageName(_T("TemporaryStrandEquivPSLoading"), bHasAsymmetricGirders, bPrecamber, bTopFlangeThickening);

               *pPara << Bold(_T("Temporary Strands")) << rptNewLine;
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + strImage) << rptNewLine;
               for (int i = 0; i < 2; i++)
               {
                  if (i == 0)
                  {
                     *pPara << _T("At installation") << rptNewLine;
                  }
                  else
                  {
                     *pPara << _T("At removal") << rptNewLine;
                     vEquivLoad = pProductLoads->GetEquivPretensionLoads(thisSegmentKey, pgsTypes::Temporary, false /*not at installation = at removal*/);
                  }

                  for (const auto& equivLoad : vEquivLoad)
                  {
                     ATLASSERT(IsZero(equivLoad.N));
                     *pPara << Sub2(_T("M"), _T("x")) << _T(" = ");
                     *pPara << _T("(") << force.SetValue(equivLoad.P) << _T(")(") << ecc.SetValue(equivLoad.eye) << _T(")");
                     *pPara << _T(" = ") << moment.SetValue(equivLoad.Mx) << _T(" at ") << loc.SetValue(equivLoad.Xs) << rptNewLine;

                     if (!IsZero(equivLoad.wy))
                     {
                        *pPara << Sub2(_T("w"), _T("y")) << _T(" = 8(") << force.SetValue(equivLoad.P) << _T(")(") << ecc.SetValue(equivLoad.Precamber) << _T(")/");
                        *pPara << _T("(") << loc.SetValue(equivLoad.Ls) << Super2(_T(")"), _T("2")) << _T(" = ");
                        *pPara << distributed.SetValue(equivLoad.wy) << _T(" from ") << loc.SetValue(equivLoad.Xs);
                        *pPara << _T(" to ") << loc.SetValue(equivLoad.Xe) << rptNewLine;
                     }

                     if (bHasAsymmetricGirders)
                     {
                        *pPara << Sub2(_T("M"), _T("y")) << _T(" = (") << force.SetValue(equivLoad.P) << _T(")(") << ecc.SetValue(equivLoad.ex) << _T(")");
                        *pPara << _T(" = ") << moment.SetValue(equivLoad.My) << _T(" at ") << loc.SetValue(equivLoad.Xs) << rptNewLine;
                     }
                  }
               }
            } // end if
         } // segmentIdx
      } // gdrIdx
   } // groupIdx
}

void CLoadingDetailsChapterBuilder::ReportEquivSegmentPostTensioningLoads(rptChapter* pChapter, bool bRating, IBridge* pBridge, IEAFDisplayUnits* pDisplayUnits, const CGirderKey& girderKey) const
{
   if (m_bSimplifiedVersion || bRating)
   {
      return;
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2_NOCHECK(pBroker, IProductLoads, pProductLoads);
   GET_IFACE2_NOCHECK(pBroker, IBridgeDescription, pIBridgeDesc);
   GET_IFACE2_NOCHECK(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);

   INIT_UV_PROTOTYPE(rptLengthUnitValue, loc, pDisplayUnits->GetSpanLengthUnit(), true);
   INIT_UV_PROTOTYPE(rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), true);
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, ecc, pDisplayUnits->GetComponentDimUnit(), true);
   INIT_UV_PROTOTYPE(rptForcePerLengthUnitValue, distributed, pDisplayUnits->GetForcePerLengthUnit(), true);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? nGroups - 1 : firstGroupIdx);

   // First do a check to see if we are reporting anything
   bool bReport = false;
   for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++)
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType firstGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
      GirderIndexType lastGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? nGirders - 1 : firstGirderIdx);
      for (GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++)
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx, gdrIdx));
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            CSegmentKey thisSegmentKey(grpIdx, gdrIdx, segIdx);
            DuctIndexType nDucts = pSegmentTendonGeometry->GetDuctCount(thisSegmentKey);
            if (0 < nDucts)
            {
               bReport = true;
               break;
            }
         }
      }
   }

   if (!bReport)
   {
      return;
   }


   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Equivalent Segment Post-Tension Loading") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T("These loads are used to determine girder deflections due to segment post-tension forces") << rptNewLine;
   *pPara << _T("Loads and eccentricities shown in positive directions") << rptNewLine;

   *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("EquivSegmentPTLoading.png")) << rptNewLine;

   for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++)
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType firstGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
      GirderIndexType lastGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? nGirders - 1 : firstGirderIdx);
      for (GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++)
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx, gdrIdx));
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            CSegmentKey thisSegmentKey(grpIdx, gdrIdx, segIdx);

            if (1 < nSegments)
            {
               pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
               *pChapter << pPara;
               *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
            }

            pPara = new rptParagraph;
            *pChapter << pPara;

            const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(thisSegmentKey);
            if (pSegment->Tendons.GetDuctCount() == 0)
            {
               *pPara << _T("No tendons in this segment.") << rptNewLine;
            }
            else
            {
               std::vector<EquivPretensionLoad> vEquivLoad = pProductLoads->GetEquivSegmentPostTensionLoads(thisSegmentKey);
               for (const auto& equivLoad : vEquivLoad)
               {
                  ATLASSERT(IsZero(equivLoad.N));

                  if (!IsZero(equivLoad.Mx))
                  {
                     *pPara << Sub2(_T("M"), _T("x")) << _T(" = ");
                     *pPara << _T("(") << force.SetValue(equivLoad.P) << _T(")(") << ecc.SetValue(equivLoad.eye) << _T(")");
                     *pPara << _T(" = ") << moment.SetValue(equivLoad.Mx) << _T(" at ") << loc.SetValue(equivLoad.Xs) << rptNewLine;
                  }

                  if (!IsZero(equivLoad.wy))
                  {
                     *pPara << Sub2(_T("w"), _T("y")) << _T(" = 8(") << force.SetValue(equivLoad.P) << _T(")(") << ecc.SetValue(equivLoad.eyh - equivLoad.eye) << _T(")/");
                     *pPara << _T("(") << loc.SetValue(equivLoad.Ls) << Super2(_T(")"), _T("2")) << _T(" = ");
                     *pPara << distributed.SetValue(equivLoad.wy) << _T(" from ") << loc.SetValue(equivLoad.Xs);
                     *pPara << _T(" to ") << loc.SetValue(equivLoad.Xe) << rptNewLine;
                  }
               }

               *pPara << rptNewLine;
            }
         } // segmentIdx
      } // gdrIdx
   } // groupIdx
}

rptParagraph* CLoadingDetailsChapterBuilder::CreatePointLoadTable(IBroker* pBroker, const CSpanKey& spanKey, IEAFDisplayUnits* pDisplayUnits, Uint16 level) const
{
   USES_CONVERSION;
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false);

   ASSERT_SPAN_KEY(spanKey);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanKey.spanIndex);
   CGirderKey girderKey(grpIdx, spanKey.girderIndex);
   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE2(pBroker, IIntervals, pIntervals);

   rptRcTable* table = rptStyleManager::CreateDefaultTable(5, _T("Point Loads"));

   table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   table->SetColumnStyle(4, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(4, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;

   (*table)(0, col++) << _T("Interval");
   (*table)(0, col++) << _T("Load") << rptNewLine << _T("Case");
   (*table)(0, col++) << COLHDR(_T("Location"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0, col++) << COLHDR(_T("Magnitude"), rptForceUnitTag, pDisplayUnits->GetShearUnit());
   (*table)(0, col++) << _T("Description");

   GET_IFACE2(pBroker, IUserDefinedLoads, pUdl);

   bool loads_exist = false;
   RowIndexType row = table->GetNumberOfHeaderRows();

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for (IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++)
   {
      CString strInterval;
      strInterval.Format(_T("Interval %d: %s"), LABEL_INTERVAL(intervalIdx), pIntervals->GetDescription(intervalIdx));

      const std::vector<IUserDefinedLoads::UserPointLoad>* ppl = pUdl->GetPointLoads(intervalIdx, spanKey);
      if (ppl != nullptr)
      {
         for ( const auto& upl : *ppl )
         {
            loads_exist = true;

            std::_tstring strLoadCaseName(UserLoads::GetLoadCaseName(upl.m_LoadCase));

            col = 0;

            (*table)(row, col++) << strInterval;
            (*table)(row, col++) << strLoadCaseName;
            (*table)(row, col++) << dim.SetValue(upl.m_Location);
            (*table)(row, col++) << shear.SetValue(upl.m_Magnitude);
            (*table)(row, col++) << upl.m_Description;

            row++;
         }
      }
   }

   if (loads_exist)
   {
      *pParagraph << table;
   }
   else
   {
      delete table;

      if (!m_bSimplifiedVersion)
      {
         *pParagraph << _T("Point loads were not defined for this girder") << rptNewLine;
      }
   }

   return pParagraph;
}

rptParagraph* CLoadingDetailsChapterBuilder::CreateDistributedLoadTable(IBroker* pBroker, const CSpanKey& spanKey, IEAFDisplayUnits* pDisplayUnits, Uint16 level) const
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptForcePerLengthUnitValue, fplu, pDisplayUnits->GetForcePerLengthUnit(), false);

   ASSERT_SPAN_KEY(spanKey);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanKey.spanIndex);
   CGirderKey girderKey(grpIdx, spanKey.girderIndex);
   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE2(pBroker, IIntervals, pIntervals);

   rptRcTable* table = rptStyleManager::CreateDefaultTable(7, _T("Distributed Loads"));

   table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   table->SetColumnStyle(6, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(6, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;

   (*table)(0, col++) << _T("Interval");
   (*table)(0, col++) << _T("Load") << rptNewLine << _T("Case");
   (*table)(0, col++) << COLHDR(_T("Start") << rptNewLine << _T("Location"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0, col++) << COLHDR(_T("End") << rptNewLine << _T("Location"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0, col++) << COLHDR(_T("Start") << rptNewLine << _T("Magnitude"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());
   (*table)(0, col++) << COLHDR(_T("End") << rptNewLine << _T("Magnitude"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());
   (*table)(0, col++) << _T("Description");

   GET_IFACE2(pBroker, IUserDefinedLoads, pUdl);

   bool loads_exist = false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for (IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++)
   {
      CString strInterval;
      strInterval.Format(_T("Interval %d: %s"), LABEL_INTERVAL(intervalIdx), pIntervals->GetDescription(intervalIdx));

      const std::vector<IUserDefinedLoads::UserDistributedLoad>* ppl = pUdl->GetDistributedLoads(intervalIdx, spanKey);
      if (ppl != nullptr)
      {
         for( const auto& upl : *ppl)
         {
            loads_exist = true;

            std::_tstring strLoadCaseName(UserLoads::GetLoadCaseName(upl.m_LoadCase));

            col = 0;

            (*table)(row, col++) << strInterval;
            (*table)(row, col++) << strLoadCaseName;
            (*table)(row, col++) << dim.SetValue(upl.m_StartLocation);
            (*table)(row, col++) << dim.SetValue(upl.m_EndLocation);
            (*table)(row, col++) << fplu.SetValue(upl.m_WStart);
            (*table)(row, col++) << fplu.SetValue(upl.m_WEnd);
            (*table)(row, col++) << upl.m_Description;

            row++;
         }
      }
   }

   if (loads_exist)
   {
      *pParagraph << table;
   }
   else
   {
      delete table;

      if (!m_bSimplifiedVersion)
      {
         *pParagraph << _T("Distributed loads were not defined for this girder") << rptNewLine;
      }
   }

   return pParagraph;
}

rptParagraph* CLoadingDetailsChapterBuilder::CreateMomentLoadTable(IBroker* pBroker, const CSpanKey& spanKey, IEAFDisplayUnits* pDisplayUnits, Uint16 level) const
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false);

   ASSERT_SPAN_KEY(spanKey);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanKey.spanIndex);
   CGirderKey girderKey(grpIdx, spanKey.girderIndex);
   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE2(pBroker, IIntervals, pIntervals);

   rptRcTable* table = rptStyleManager::CreateDefaultTable(5, _T("End Moments"));

   table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   table->SetColumnStyle(4, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(4, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;

   (*table)(0, col++) << _T("Event");
   (*table)(0, col++) << _T("Load") << rptNewLine << _T("Case");
   (*table)(0, col++) << _T("Location");
   (*table)(0, col++) << COLHDR(_T("Magnitude"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   (*table)(0, col++) << _T("Description");

   GET_IFACE2(pBroker, IUserDefinedLoads, pUdl);

   bool loads_exist = false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for (IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++)
   {
      CString strInterval;
      strInterval.Format(_T("Interval %d: %s"), LABEL_INTERVAL(intervalIdx), pIntervals->GetDescription(intervalIdx));

      const std::vector<IUserDefinedLoads::UserMomentLoad>* ppl = pUdl->GetMomentLoads(intervalIdx, spanKey);
      if (ppl != nullptr)
      {
         for ( const auto& upl : *ppl)
         {
            loads_exist = true;

            std::_tstring strLoadCaseName(UserLoads::GetLoadCaseName(upl.m_LoadCase));

            col = 0;

            (*table)(row, col++) << strInterval;
            (*table)(row, col++) << strLoadCaseName;

            if (IsZero(upl.m_Location))
            {
               (*table)(row, col++) << _T("Start of span");
            }
            else
            {
               (*table)(row, col++) << _T("End of span");
            }

            (*table)(row, col++) << moment.SetValue(upl.m_Magnitude);
            (*table)(row, col++) << upl.m_Description;

            row++;
         }
      }
   }

   if (loads_exist)
   {
      *pParagraph << table;
   }
   else
   {
      delete table;

      if (!m_bSimplifiedVersion)
      {
         *pParagraph << _T("Moment loads were not defined for this girder") << rptNewLine;
      }
   }

   return pParagraph;
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

   if (overlayLoads.size() == 1 && IsEqual(overlayLoads.front().Wstart, overlayLoads.front().Wend))
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
      if ( !IsEqual(load_1.Wstart,load_2.Wstart) )
      {
         return false;
      }

      if ( !IsEqual(load_1.Wend,load_2.Wend) )
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

static bool IsLongitudinalJointLoadUniform(const std::vector<LongitudinalJointLoad>& loads)
{
   if (loads.size() == 0)
   {
      return true;
   }

   if (loads.size() == 1 && IsEqual(loads.front().Wstart, loads.front().Wend))
   {
      return true;
   }

   std::vector<LongitudinalJointLoad>::const_iterator i1, i2;
   i1 = loads.begin();
   i2 = loads.begin() + 1;
   std::vector<LongitudinalJointLoad>::const_iterator end(loads.end());
   for (; i2 != end; i1++, i2++)
   {
      const LongitudinalJointLoad& load_1 = *i1;
      const LongitudinalJointLoad& load_2 = *i2;

      // only takes one difference to be nonuniform
      if (!IsEqual(load_1.Wstart, load_2.Wstart))
      {
         return false;
      }

      if (!IsEqual(load_1.Wend, load_2.Wend))
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

   if (loads.size() == 1 && IsEqual(loads.front().Wstart, loads.front().Wend))
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
      if ( !IsEqual(load_1.Wstart,load_2.Wstart) )
      {
         return false;
      }

      if ( !IsEqual(load_1.Wend,load_2.Wend) )
      {
         return false;
      }
   }

   return true;
}
