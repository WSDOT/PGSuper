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

#include <IFace\DisplayUnits.h>
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

/****************************************************************************
CLASS
   CLoadingDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CLoadingDetailsChapterBuilder::CLoadingDetailsChapterBuilder(bool bDesign,bool bRating):
m_bDesign(bDesign),
m_bRating(bRating),
m_bSimplifiedVersion(false)
{
}

CLoadingDetailsChapterBuilder::CLoadingDetailsChapterBuilder(bool SimplifiedVersion,bool bDesign,bool bRating):
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
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType gdr = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);

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
   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl, pDisplayUnits->GetForcePerLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,    pDisplayUnits->GetGeneralForceUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );

   // uniform loads
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(2,"Uniform Loads Applied Along the Entire Girder");
   *pPara << p_table << rptNewLine;

   p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   (*p_table)(0,0) << "Load Type";
   (*p_table)(0,1) << COLHDR("w",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

   GET_IFACE2(pBroker,IProductLoads,pProdLoads);

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   std::vector<GirderLoad> gdrLoad;
   std::vector<DiaphragmLoad> diaLoad;
   pProdLoads->GetGirderSelfWeightLoad(span,gdr,&gdrLoad,&diaLoad);
   bool bUniformGirderDeadLoad = (gdrLoad.size() == 1 && IsEqual(gdrLoad[0].wStart,gdrLoad[0].wEnd) ? true : false);
   if ( bUniformGirderDeadLoad )
   {
      // girder load is uniform
      (*p_table)(row,0) << "Girder";
      (*p_table)(row++,1) << fpl.SetValue(-gdrLoad[0].wStart);
   }

   if ( pProdLoads->HasSidewalkLoad(span,gdr) )
   {
      (*p_table)(row,0) << "Sidewalk";
      (*p_table)(row++,1) << fpl.SetValue(-pProdLoads->GetSidewalkLoad(span,gdr));
   }

   (*p_table)(row,0) << "Traffic Barrier";
   (*p_table)(row++,1) << fpl.SetValue(-pProdLoads->GetTrafficBarrierLoad(span,gdr));

   if ( pProdLoads->HasPedestrianLoad(span,gdr) )
   {
      (*p_table)(row,0) << "Pedestrian Live Load";
      (*p_table)(row++,1) << fpl.SetValue(pProdLoads->GetPedestrianLoad(span,gdr));
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
   GET_IFACE2(pBroker,IBridge,pBridge);

   Float64 end_size = pBridge->GetGirderStartConnectionLength( span, gdr );

   if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<< "Slab Load Applied Between Bearings"<<rptNewLine;
      
      pPara = new rptParagraph;
      *pChapter << pPara;

      *pPara << "Slab Load is approximated with Linear Load Segments applied along the length of the girder" << rptNewLine;
      *pPara << "Haunch weight includes effects of roadway geometry but does not include a reduction for camber" << rptNewLine;

      if ( pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP )
      {
         p_table = pgsReportStyleHolder::CreateDefaultTable(5,"");
         *pPara << p_table;

         (*p_table)(0,0) << COLHDR("Location"<<rptNewLine<<"From Left Bearing",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*p_table)(0,1) << COLHDR("Panel Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
         (*p_table)(0,2) << COLHDR("Cast Slab Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
         (*p_table)(0,3) << COLHDR("Haunch Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
         (*p_table)(0,4) << COLHDR("Total Slab Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

         std::vector<SlabLoad> slab_loads;
         pProdLoads->GetMainSpanSlabLoad(span, gdr, &slab_loads);

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
      else
      {
         p_table = pgsReportStyleHolder::CreateDefaultTable(4,"");
         *pPara << p_table;

         (*p_table)(0,0) << COLHDR("Location"<<rptNewLine<<"From Left Bearing",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*p_table)(0,1) << COLHDR("Main Slab Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
         (*p_table)(0,2) << COLHDR("Haunch Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
         (*p_table)(0,3) << COLHDR("Total Slab Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

         std::vector<SlabLoad> slab_loads;
         pProdLoads->GetMainSpanSlabLoad(span, gdr, &slab_loads);

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
         pProdLoads->GetCantileverSlabLoad(span, gdr, &P1, &M1, &P2, &M2);
         (*p_table)(1,0) << "Left Bearing";
         (*p_table)(1,1) << force.SetValue(-P1);
         (*p_table)(1,2) << moment.SetValue(M1);
      
         (*p_table)(2,0) << "Right Bearing";
         (*p_table)(2,1) << force.SetValue(-P2);
         (*p_table)(2,2) << moment.SetValue(M2);

         p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
         p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      }
   }


   // overlay laod
   if ( pBridge->HasOverlay() )
   {
      GET_IFACE2(pBroker, ISpecification, pSpec );
      pgsTypes::OverlayLoadDistributionType olayd = pSpec->GetOverlayLoadDistributionType();

      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<< "Overlay"<<rptNewLine;
   
      p_table = pgsReportStyleHolder::CreateDefaultTable(6,"");
      *pPara << p_table;

      (*p_table)(0,0) << COLHDR("Load Start,"<<rptNewLine<<"From Left Bearing",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,1) << COLHDR("Load End,"<<rptNewLine<<"From Left Bearing",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      if (olayd==pgsTypes::olDistributeTributaryWidth)
      {
         (*p_table)(0,2) << COLHDR("Start " << Sub2("W","trib"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*p_table)(0,3) << COLHDR("End " << Sub2("W","trib"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }
      else
      {
         (*p_table)(0,2) << COLHDR("Start " << Sub2("W","cc"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*p_table)(0,3) << COLHDR("End " << Sub2("W","cc"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }

      (*p_table)(0,4) << COLHDR("Start Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
      (*p_table)(0,5) << COLHDR("End Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

      std::vector<OverlayLoad> overlay_loads;
      pProdLoads->GetOverlayLoad(span, gdr, &overlay_loads);

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
   }

   // Shear key loads
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   bool has_shear_key = pGirder->HasShearKey(span, gdr, spacingType);
   if ( has_shear_key )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<< "Shear Key Load"<<rptNewLine;
      
      pPara = new rptParagraph;
      *pChapter << pPara;

      *pPara << "Shear Key Load is approximated with Linear Load Segments applied along the length of the girder" << rptNewLine;

      p_table = pgsReportStyleHolder::CreateDefaultTable(4,"");
      *pPara << p_table;

      (*p_table)(0,0) << COLHDR("Location"<<rptNewLine<<"From Left Bearing",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,1) << COLHDR("Load Within"<<rptNewLine<<"Girder Envelope",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
      (*p_table)(0,2) << COLHDR("Load Within"<<rptNewLine<<"Joint",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
      (*p_table)(0,3) << COLHDR("Total Shear"<<rptNewLine<<"Key Weight",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

      std::vector<ShearKeyLoad> loads;
      pProdLoads->GetShearKeyLoad(span, gdr, &loads);

      RowIndexType row = 1;
      for ( std::vector<ShearKeyLoad>::iterator i = loads.begin(); i != loads.end(); i++ )
      {
         ShearKeyLoad& sk_load = *i;

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
      pProdLoads->GetEndDiaphragmLoads(span, gdr, &P1, &M1, &P2, &M2);
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
      pProdLoads->GetIntermediateDiaphragmLoads(pgsTypes::CastingYard, span, gdr, &diap_loads);
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

         std::vector<IntermedateDiaphragm> diaphragms   = pBridge->GetIntermediateDiaphragms(pgsTypes::CastingYard,span,gdr);
         std::vector<IntermedateDiaphragm>::iterator iter = diaphragms.begin();

         for (std::vector<DiaphragmLoad>::iterator id = diap_loads.begin(); id!=diap_loads.end(); id++, iter++)
         {
            DiaphragmLoad& rload = *id;
            IntermedateDiaphragm& dia = *iter;

            (*p_table)(row,0) << loc.SetValue(rload.Loc);
            (*p_table)(row,1) << dim.SetValue(dia.H);
            (*p_table)(row,2) << dim.SetValue(dia.W);
            (*p_table)(row,3) << dim.SetValue(dia.T);
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
      pProdLoads->GetIntermediateDiaphragmLoads(pgsTypes::BridgeSite1, span, gdr, &diap_loads);
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

         std::vector<IntermedateDiaphragm> diaphragms   = pBridge->GetIntermediateDiaphragms(pgsTypes::BridgeSite1,span,gdr);
         std::vector<IntermedateDiaphragm>::iterator iter = diaphragms.begin();

         RowIndexType row = 1;
         for (std::vector<DiaphragmLoad>::iterator id = diap_loads.begin(); id!=diap_loads.end(); id++, iter++)
         {
            DiaphragmLoad& rload = *id;
            IntermedateDiaphragm& dia = *iter;

            (*p_table)(row,0) << loc.SetValue(rload.Loc);
            (*p_table)(row,1) << dim.SetValue(dia.H);
            (*p_table)(row,2) << dim.SetValue(dia.W);
            (*p_table)(row,3) << dim.SetValue(dia.T);
            (*p_table)(row,4) << force.SetValue(-rload.Load);
            row++;
         }
      }

      // live loads
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<< "Live Loads"<<rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
      bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);
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

      // User Defined Loads
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<< "User Defined Loads"<<rptNewLine;
      pPara = CUserDefinedLoadsChapterBuilder::CreatePointLoadTable(pBroker, span, gdr, pDisplayUnits, level);
      *pChapter << pPara;
      pPara = CUserDefinedLoadsChapterBuilder::CreateDistributedLoadTable(pBroker, span, gdr, pDisplayUnits, level);
      *pChapter << pPara;
      pPara = CUserDefinedLoadsChapterBuilder::CreateMomentLoadTable(pBroker, span, gdr, pDisplayUnits, level);
      *pChapter << pPara;


      // LRFD Limit States
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<< "LRFD Limit States"<<rptNewLine;
   
      pPara = new rptParagraph;
      *pChapter << pPara;

      p_table = pgsReportStyleHolder::CreateDefaultTable(2,"");
      *pPara << p_table;

      p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
      p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

      p_table->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
      p_table->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );


      (*p_table)(0,0) << "Stage";
      (*p_table)(0,1) << "Load Case";

      (*p_table)(1,0) << "Casting Yard";
      (*p_table)(1,1) << "DC = Girder";

      std::string strDC;
      if (has_shear_key)
      {
         strDC = "DC = Girder + Diaphragms + Shear Key + Slab";
      }
      else
      {
         strDC = "DC = Girder + Diaphragms + Slab";
      }

      (*p_table)(2,0) << "Deck and Diaphragm Placement (Bridge Site 1)";
      (*p_table)(2,1) << strDC;

      (*p_table)(3,0) << "Superimposed Dead Loads (Bridge Site 2)";
      (*p_table)(3,1) << strDC<<" + Traffic Barrier"<<rptNewLine
                      << "DW = Overlay";

      (*p_table)(4,0) << "Final with Live Load (Bridge Site 3)";
      (*p_table)(4,1) << strDC<<" + Traffic Barrier"<<rptNewLine
                      << "DW = Future Overlay"<< rptNewLine
                      << "LL+IM = Live Load + Impact" << rptNewLine
                      << "PL = Pedestrian Live Load" << rptNewLine;

   
      // LRFD Limit States Load Factors
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<< "LRFD Limit State Load Factors"<<rptNewLine;
   
      pPara = new rptParagraph;
      *pChapter << pPara;

      GET_IFACE2(pBroker,ILoadFactors,pLF);
      const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

      rptRcScalar scalar;
      scalar.SetFormat( sysNumericFormatTool::Fixed );
      scalar.SetWidth(6);
      scalar.SetPrecision(2);
      scalar.SetTolerance(1.0e-6);

      p_table = pgsReportStyleHolder::CreateDefaultTable(4,"");
      p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
      p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
      *pPara << p_table;


      (*p_table)(0,0) << "Limit State";
      (*p_table)(0,1) << Sub2(symbol(gamma),"DC");
      (*p_table)(0,2) << Sub2(symbol(gamma),"DW");
      (*p_table)(0,3) << Sub2(symbol(gamma),"LL");
      
      
      RowIndexType row = 1;

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
      std::vector<std::pair<Float64,Float64> > loads;
      pCamber->GetStraightStrandEquivLoading(span,gdr,&loads);
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
      pCamber->GetHarpedStrandEquivLoading(span,gdr,&Ml, &Mr, &Nl, &Nr, &Xl, &Xr);
      *pPara << Bold("Harped Strands") << rptNewLine;
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "HarpedStrandCamberLoading.gif") << rptNewLine;
      *pPara << "End moments, M = " << moment.SetValue(Ml);
      *pPara << " and " << moment.SetValue(Mr) << rptNewLine;
      *pPara << "Left Harp Point, N = " << force.SetValue(Nl) << " at = " << loc.SetValue(Xl) << rptNewLine;
      *pPara << "Right Harp Point, N = " << force.SetValue(Nr) << " at = " << loc.SetValue(Xr) << rptNewLine;

      *pPara << rptNewLine;
   
      GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
      if ( 0 < pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Temporary) )
      {
         double MxferL, MxferR, MremoveL, MremoveR;
         pCamber->GetTempStrandEquivLoading(span,gdr,&MxferL,&MxferR,&MremoveL,&MremoveR);
         *pPara << Bold("Temporary Strands") << rptNewLine;
         *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "TempStrandCamberLoading.gif") << rptNewLine;
         *pPara << "End moments at prestressing, M = " << moment.SetValue(MxferL) << " and ";
         *pPara << moment.SetValue(-MxferR) << rptNewLine;
         *pPara << "Moment at strand removal, M = " << moment.SetValue(MremoveL) << " and ";
         *pPara << moment.SetValue(-MremoveR) << rptNewLine;
      }
   } // end if
   return pChapter;
}

CChapterBuilder* CLoadingDetailsChapterBuilder::Clone() const
{
   return new CLoadingDetailsChapterBuilder(m_bSimplifiedVersion,m_bDesign,m_bRating);
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
