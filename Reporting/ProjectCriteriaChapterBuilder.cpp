///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

/****************************************************************************
CLASS
   CProjectCriteriaChapterBuilder
****************************************************************************/

#include <Reporting\ProjectCriteriaChapterBuilder.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Allowables.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\RatingSpecification.h>

#include <Lrfd\VersionMgr.h>

#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void write_load_modifiers(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits);
void write_environmental_conditions(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits);
void write_casting_yard(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr);
void write_lifting(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr);
void write_hauling(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr);
void write_temp_strand_removal(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr);
void write_bridge_site1(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr);
void write_bridge_site2(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr);
void write_bridge_site3(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr);
void write_moment_capacity(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr);
void write_shear_capacity(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr);
void write_creep(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry);
void write_losses(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry);
void write_strand_stress(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry);
void write_structural_analysis(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits);
void write_deflections(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry);
void write_rating_criteria(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const RatingLibraryEntry* pRatingEntry);
void write_load_factors(rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,LPCTSTR lpszName,const CLiveLoadFactorModel& model);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProjectCriteriaChapterBuilder::CProjectCriteriaChapterBuilder(bool bRating,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bRating = bRating;
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CProjectCriteriaChapterBuilder::GetName() const
{
   return TEXT("Project Criteria");
}

rptChapter* CProjectCriteriaChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
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

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

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

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2( pBroker, ISpecification, pSpec );
   std::_tstring spec_name = pSpec->GetSpecification();
   std::_tstring rating_name = pRatingSpec->GetRatingSpecification();

   GET_IFACE2( pBroker, ILibrary, pLib );
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   const RatingLibraryEntry* pRatingEntry = pLib->GetRatingEntry( rating_name.c_str() );

   rptParagraph* pPara;

   if ( bRating )
   {
      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << _T("Load Rating Criteria") << rptNewLine;
      *pPara << Bold(_T("Name: ")) << rating_name << rptNewLine;
      *pPara << Bold(_T("Description: ")) << pRatingEntry->GetDescription() << rptNewLine;
      *pPara <<Bold(_T("Based on:  "));
      switch( pRatingEntry->GetSpecificationVersion() )
      {
      case lrfrVersionMgr::FirstEdition2008:
         *pPara << _T("AASHTO Manual for Bridge Evaluation, 1st Edition, 2008") << rptNewLine;
         break;

      case lrfrVersionMgr::FirstEditionWith2010Interims:
         *pPara << _T("AASHTO Manual for Bridge Evaluation, 1st Edition, 2008 with 2010 interim provisions") << rptNewLine;
         break;

      default:
         ATLASSERT(false);
         *pPara <<_T("Unknown") << rptNewLine;
         break;
      }

      // write load rating criteria here

      *pPara << _T("Load Rating Criteria includes ") << spec_name << rptNewLine;
   }


   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara <<Bold(_T("Name: "))<< spec_name << rptNewLine;
   *pPara <<Bold(_T("Description: "))<<pSpecEntry->GetDescription()<<rptNewLine;
   *pPara <<Bold(_T("Based on: ")) << _T("AASHTO LRFD Bridge Design Specifications, ") << lrfdVersionMgr::GetVersionString();
   
   lrfdVersionMgr::Units units = pSpecEntry->GetSpecificationUnits();
   if (units==lrfdVersionMgr::SI)
      *pPara<<_T(" - SI Units")<<rptNewLine;
   else
      *pPara<<_T(" - US Units")<<rptNewLine;

   write_load_modifiers(          pChapter, pBroker, pDisplayUnits);
   write_environmental_conditions(pChapter, pBroker, pDisplayUnits);
   write_structural_analysis(     pChapter, pBroker, pDisplayUnits);

   if ( !bRating )
   {
      write_casting_yard(pChapter, pBroker, pDisplayUnits, pSpecEntry,span,gdr);
      
      GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
      if (pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
      {
         write_lifting(pChapter, pBroker, pDisplayUnits, pSpecEntry,span,gdr);
      }

      GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
      if (pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
      {
         write_hauling(pChapter, pBroker, pDisplayUnits, pSpecEntry,span,gdr);
      }

      write_temp_strand_removal(pChapter, pBroker, pDisplayUnits, pSpecEntry,span,gdr);
      write_bridge_site1(pChapter, pBroker, pDisplayUnits, pSpecEntry,span,gdr);
      write_bridge_site2(pChapter, pBroker, pDisplayUnits, pSpecEntry,span,gdr);
   }

   write_bridge_site3(pChapter, pBroker, pDisplayUnits, pSpecEntry,span,gdr);
   write_moment_capacity(pChapter, pBroker, pDisplayUnits, pSpecEntry,span,gdr);
   write_shear_capacity(pChapter, pBroker, pDisplayUnits, pSpecEntry,span,gdr);
   write_creep(pChapter, pBroker, pDisplayUnits, pSpecEntry);
   write_losses(pChapter, pBroker, pDisplayUnits, pSpecEntry);
   write_strand_stress(pChapter, pBroker, pDisplayUnits, pSpecEntry);
   write_deflections(pChapter, pBroker, pDisplayUnits, pSpecEntry);

   if ( bRating )
      write_rating_criteria(pChapter,pBroker,pDisplayUnits,pRatingEntry);

   return pChapter;
}


CChapterBuilder* CProjectCriteriaChapterBuilder::Clone() const
{
   return new CProjectCriteriaChapterBuilder(m_bRating);
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

void write_load_modifiers(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = pgsReportStyleHolder::CreateTableNoHeading(2,_T("Load Modifiers"));
   *pPara << p_table;

   GET_IFACE2(pBroker,ILoadModifiers,pLoadModifiers);

   (*p_table)(0,0) << _T("Ductility  - ")<< Sub2(symbol(eta),_T("D"));
   (*p_table)(0,1) <<  pLoadModifiers->GetDuctilityFactor();

   (*p_table)(1,0) << _T("Importance - ")<< Sub2(symbol(eta),_T("I"));
   (*p_table)(1,1) <<  pLoadModifiers->GetImportanceFactor();

   (*p_table)(2,0) << _T("Redundancy - ")<< Sub2(symbol(eta),_T("R"));
   (*p_table)(2,1) <<  pLoadModifiers->GetRedundancyFactor();
}

void write_environmental_conditions(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = pgsReportStyleHolder::CreateTableNoHeading(2,_T("Environmental Conditions"));
   *pPara << p_table;

   GET_IFACE2(pBroker,IEnvironment,pEnvironment);

   (*p_table)(0,0) << _T("Exposure Condition");
   enumExposureCondition cond = pEnvironment->GetExposureCondition();
   if (cond==expNormal)
      (*p_table)(0,1) << _T("Normal");
   else
      (*p_table)(0,1) << _T("Severe");

   (*p_table)(1,0) << _T("Relative Humidity");
   (*p_table)(1,1) <<  pEnvironment->GetRelHumidity()<<_T("%");
}

void write_structural_analysis(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;

   *pPara << _T("Structural Analysis Method") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2( pBroker, ISpecification, pSpec );

   switch( pSpec->GetAnalysisType() )
   {
   case pgsTypes::Simple:
      *pPara << _T("Simple Span Analysis") << rptNewLine;
      break;

   case pgsTypes::Continuous:
      *pPara << _T("Continuous Span Analysis") << rptNewLine;
      break;

   case pgsTypes::Envelope:
      *pPara << _T("Envelope of Simple and Continuous Span Analyses") << rptNewLine;
      break;
   }
}

void write_casting_yard(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Casting Yard Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true );

   bool do_check, do_design;
   Float64 slope05, slope06, slope07;
   pSpecEntry->GetMaxStrandSlope(&do_check, &do_design, &slope05, &slope06, &slope07);
   if (do_check)
   {
      *pPara<<_T("Max. slope for 0.5\" ") << symbol(phi) << _T(" strands = 1:")<<slope05<<rptNewLine;
      *pPara<<_T("Max. slope for 0.6\" ") << symbol(phi) << _T(" strands = 1:")<<slope06<<rptNewLine;
      *pPara<<_T("Max. slope for 0.7\" ") << symbol(phi) << _T(" strands = 1:")<<slope07<<rptNewLine;
   }
   else
      *pPara <<_T("Max. Strand slope is not checked")<<rptNewLine;

   Float64 f;
   pSpecEntry->GetHoldDownForce(&do_check, &do_design, &f);
   if (do_check)
      *pPara<<_T("Max. hold down force in casting yard = ")<<force.SetValue(f)<<rptNewLine;
   else
      *pPara <<_T("Max. hold down force in casting yard  is not checked")<<rptNewLine;

   int method = pSpecEntry->GetCuringMethod();
   if (method==CURING_NORMAL)
      *pPara <<_T("Girder was cured using Normal method")<<rptNewLine;
   else if (method==CURING_ACCELERATED)
      *pPara <<_T("Girder was cured using Accelerated method")<<rptNewLine;
   else
      CHECK(0);

   *pPara<<_T("Max stirrup spacing = ")<< dim.SetValue(pSpecEntry->GetMaxStirrupSpacing())<<rptNewLine;

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowableConcreteStress);

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
         pgsPointOfInterest poi(spanIdx,gdrIdx,0.0);
         Float64 fccy = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::CastingYard,pgsTypes::ServiceI, pgsTypes::Compression);
         Float64 ftcy = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::CastingYard,pgsTypes::ServiceI, pgsTypes::Tension);
         Float64 ft   = pAllowableConcreteStress->GetCastingYardWithMildRebarAllowableStress(poi.GetSpan(),poi.GetGirder());
         *pPara<<_T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
         *pPara<<_T("Allowable Concrete Stresses - Service I (5.8.4.1.1)")<<rptNewLine;
         *pPara<<_T("- Compressive Stress = ")<<stress.SetValue(fccy)<<rptNewLine;
         *pPara<<_T("- Tensile Stress (w/o mild rebar) = ")<<stress.SetValue(ftcy) << rptNewLine;
         *pPara<<_T("- Tensile Stress (w/  mild rebar) = ")<<stress.SetValue(ft) << rptNewLine;
      } // gdrIdx
   } // spanIdx

   if (pSpecEntry->IsAnchorageCheckEnabled())
   {
      if ( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() )
         *pPara<<_T("Splitting zone length: h/") << pSpecEntry->GetSplittingZoneLengthFactor() << rptNewLine;
      else
         *pPara<<_T("Bursting zone length: h/") << pSpecEntry->GetSplittingZoneLengthFactor() << rptNewLine;
   }
   else
   {
      if ( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() )
         *pPara<<_T("Splitting and confinement checks (5.10.10) are disabled.") << rptNewLine;
      else
         *pPara<<_T("Bursting and confinement checks (5.10.10) are disabled.") << rptNewLine;
   }
}

void write_lifting(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Lifting Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), true );

   *pPara<<_T("Factors of Safety")<<rptNewLine;
   *pPara<<_T("- Cracking F.S. = ")<< pSpecEntry->GetCyLiftingCrackFs()<<rptNewLine;
   *pPara<<_T("- Failure F.S. = ")<< pSpecEntry->GetCyLiftingFailFs()<<rptNewLine;

   *pPara<<_T("Impact Factors")<<rptNewLine;
   *pPara<<_T("- Upward   = ")<< pSpecEntry->GetCyLiftingUpwardImpact()<<rptNewLine;
   *pPara<<_T("- Downward = ")<< pSpecEntry->GetCyLiftingDownwardImpact()<<rptNewLine;

   *pPara<<_T("Height of pick point above top of girder = ")<<dim.SetValue(pSpecEntry->GetPickPointHeight())<<rptNewLine;
   *pPara<<_T("Lifting loop placement tolerance = ")<<dim.SetValue(pSpecEntry->GetLiftingLoopTolerance())<<rptNewLine;
   *pPara<<_T("Max. girder sweep tolerance = ")<<pSpecEntry->GetMaxGirderSweepLifting()<<rptNewLine;
   *pPara<<_T("Min. angle of inclination of lifting cables = ")<<angle.SetValue(pSpecEntry->GetMinCableInclination())<<rptNewLine;

   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);


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
         *pPara << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;

         Float64 fr = pGirderLiftingSpecCriteria->GetLiftingModulusOfRupture(spanIdx,gdrIdx);
         *pPara << _T("Modulus of rupture = ") << stress.SetValue(fr) << rptNewLine;
         
         Float64 fccy = pGirderLiftingSpecCriteria->GetLiftingAllowableCompressiveConcreteStress(spanIdx,gdrIdx);
         Float64 ftcy = pGirderLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStress(spanIdx,gdrIdx);
         Float64 ft   = pGirderLiftingSpecCriteria->GetLiftingWithMildRebarAllowableStress(spanIdx,gdrIdx);
         *pPara<<_T("Allowable Concrete Stresses - Lifting (5.9.4.1.1)")<<rptNewLine;
         *pPara<<_T("- Compressive Stress = ")<<stress.SetValue(fccy)<<rptNewLine;
         *pPara<<_T("- Tensile Stress (w/o mild rebar) = ")<<stress.SetValue(ftcy) << rptNewLine;
         *pPara<<_T("- Tensile Stress (w/  mild rebar) = ")<<stress.SetValue(ft) << rptNewLine;
      } // gdrIdx
   } // spanIdx
}

void write_hauling(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Hauling Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim2, pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force, pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptMomentPerAngleUnitValue, spring, pDisplayUnits->GetMomentPerAngleUnit(),true );

   *pPara<<_T("Factors of Safety")<<rptNewLine;
   *pPara<<_T("- Cracking F.S. = ")<< pSpecEntry->GetHaulingCrackFs()<<rptNewLine;
   *pPara<<_T("- Roll Over F.S. = ")<< pSpecEntry->GetHaulingFailFs()<<rptNewLine;

   *pPara<<_T("Impact Factors")<<rptNewLine;
   *pPara<<_T("- Upward   = ")<< pSpecEntry->GetHaulingUpwardImpact()<<rptNewLine;
   *pPara<<_T("- Downward = ")<< pSpecEntry->GetHaulingDownwardImpact()<<rptNewLine;

   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pHauling);

   if ( pHauling->GetRollStiffnessMethod() == IGirderHaulingSpecCriteria::LumpSum )
   {
      *pPara<<_T("Roll stiffness of trailer = ")<<spring.SetValue(pHauling->GetLumpSumRollStiffness())<<rptNewLine;
   }
   else
   {
      *pPara<<_T("Maximum Weight Per Axle = ")<< force.SetValue(pHauling->GetAxleWeightLimit())<<rptNewLine;
      *pPara<<_T("Stiffness Per Axle = ")<< spring.SetValue(pHauling->GetAxleStiffness())<<rptNewLine;
      *pPara<<_T("Minimum Roll Stiffness = ")<< spring.SetValue(pHauling->GetMinimumRollStiffness())<<rptNewLine;
   }
   *pPara<<_T("Height of girder bottom above roadway = ")<<dim.SetValue(pHauling->GetHeightOfGirderBottomAboveRoadway())<<rptNewLine;
   *pPara<<_T("Height of truck roll center above roadway = ")<<dim.SetValue(pHauling->GetHeightOfTruckRollCenterAboveRoadway())<<rptNewLine;
   *pPara<<_T("C-C distance between truck tires = ")<<dim.SetValue(pHauling->GetAxleWidth())<<rptNewLine;
   *pPara<<_T("Max. distance between girder supports = ")<<dim2.SetValue(pHauling->GetAllowableDistanceBetweenSupports())<<rptNewLine;
   *pPara<<_T("Max. leading overhang (nearest truck cab) = ")<<dim2.SetValue(pHauling->GetAllowableLeadingOverhang() )<<rptNewLine;
   *pPara<<_T("Max. superelevation = ")<<pHauling->GetMaxSuperelevation()<<rptNewLine;
   *pPara<<_T("Girder sweep tolerance = ")<<pHauling->GetHaulingSweepTolerance()<<rptNewLine;
   *pPara<<_T("Support placement lateral tolerance = ")<<dim.SetValue(pHauling->GetHaulingSupportPlacementTolerance())<<rptNewLine;
   *pPara<<_T("Increase of girder camber for CG = ")<<pHauling->GetIncreaseInCgForCamber()<<rptNewLine;
   *pPara<<_T("Max. girder weight = ")<< force.SetValue(pHauling->GetMaxGirderWgt())<<rptNewLine;

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
         *pPara << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;

         Float64 fr = pHauling->GetHaulingModulusOfRupture(spanIdx,gdrIdx);
         *pPara << _T("Modulus of rupture = ") << stress.SetValue(fr) << rptNewLine;

         Float64 fccy = pHauling->GetHaulingAllowableCompressiveConcreteStress(spanIdx,gdrIdx);
         Float64 ftcy = pHauling->GetHaulingAllowableTensileConcreteStress(spanIdx,gdrIdx);
         Float64 ft   = pHauling->GetHaulingWithMildRebarAllowableStress(spanIdx,gdrIdx);
         *pPara<<_T("Allowable Concrete Stresses - Hauling (5.9.4.2.1)")<<rptNewLine;
         *pPara<<_T("- Compressive Stress = ")<<stress.SetValue(fccy)<<rptNewLine;
         *pPara<<_T("- Tensile Stress (w/o mild rebar) = ")<<stress.SetValue(ftcy) << rptNewLine;
         *pPara<<_T("- Tensile Stress (w/  mild rebar) = ")<<stress.SetValue(ft) << rptNewLine;
      }
   }
}

void write_temp_strand_removal(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Temporary Strand Removal Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowableConcreteStress);

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
         *pPara << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
         
         pgsPointOfInterest poi(spanIdx,gdrIdx,0.0);
         Float64 fcsp = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::TemporaryStrandRemoval,pgsTypes::ServiceI, pgsTypes::Compression);
         *pPara<<_T("Allowable Compressive Concrete Stresses (5.9.4.2.1)")<<rptNewLine;
         *pPara<<_T("- Service I = ")<<stress.SetValue(fcsp)<<rptNewLine;

         Float64 fts = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::TemporaryStrandRemoval,pgsTypes::ServiceI, pgsTypes::Tension);
         *pPara<<_T("Allowable Tensile Concrete Stresses (5.9.4.2.2)")<<rptNewLine;
         *pPara<<_T("- Service I = ")<<stress.SetValue(fts)<<rptNewLine;
      }
   }
}

void write_bridge_site1(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Deck and Diaphragm Placement Stage (Bridge Site 1) Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowableConcreteStress);

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
         *pPara << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
         
         pgsPointOfInterest poi(spanIdx,gdrIdx,0.0);
         Float64 fcsp = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::BridgeSite1,pgsTypes::ServiceI, pgsTypes::Compression);
         *pPara<<_T("Allowable Compressive Concrete Stresses (5.9.4.2.1)")<<rptNewLine;
         *pPara<<_T("- Service I = ")<<stress.SetValue(fcsp)<<rptNewLine;

         Float64 fts = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::BridgeSite1,pgsTypes::ServiceI, pgsTypes::Tension);
         *pPara<<_T("Allowable Tensile Concrete Stresses (5.9.4.2.2)")<<rptNewLine;
         *pPara<<_T("- Service I = ")<<stress.SetValue(fts)<<rptNewLine;
      }
   }
}

void write_bridge_site2(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Superimposed Dead Load Stage (Bridge Site 2) Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );

   switch ( pSpecEntry->GetTrafficBarrierDistributionType() )
   {
   case pgsTypes::tbdGirder:
      *pPara << _T("Railing system weight is distributed to ") << pSpecEntry->GetMaxGirdersDistTrafficBarrier() << _T(" girders") << rptNewLine;
      break;

   case pgsTypes::tbdMatingSurface:
      *pPara << _T("Railing system weight is distributed to ") << pSpecEntry->GetMaxGirdersDistTrafficBarrier() << _T(" mating surfaces") << rptNewLine;
      break;

   case pgsTypes::tbdWebs:
      *pPara << _T("Railing system weight is distributed to ") << pSpecEntry->GetMaxGirdersDistTrafficBarrier() << _T(" webs") << rptNewLine;
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowableConcreteStress);

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
         *pPara << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
         
         pgsPointOfInterest poi(spanIdx,gdrIdx,0.0);
         Float64 fcsp = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::BridgeSite2,pgsTypes::ServiceI, pgsTypes::Compression);
         *pPara<<_T("Allowable Compressive Concrete Stresses (5.9.4.2.1)")<<rptNewLine;
         *pPara<<_T("- Service I = ")<<stress.SetValue(fcsp)<<rptNewLine;
      }
   }

   *pPara << rptNewLine;
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth )
   {
      *pPara << _T("Effective Flange Width taken to be the tributary width") << rptNewLine;
   }
   else
   {
      *pPara << _T("Effective Flange Width computed in accordance with LRFD 4.6.2.6") << rptNewLine;
   }

   *pPara << rptNewLine;
   if( pSpecEntry->GetOverlayLoadDistributionType()==pgsTypes::olDistributeTributaryWidth)
   {
      *pPara << _T("Overlay load is distributed using tributary width.")<< rptNewLine;
   }
   else
   {
      *pPara << _T("Overlay load is distributed uniformly among all girders per LRFD 4.6.2.2.1")<< rptNewLine;
   }
}

void write_bridge_site3(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Final with Live Load Stage (Bridge Site 3) Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptMomentPerAngleUnitValue, spring, pDisplayUnits->GetMomentPerAngleUnit(),true );

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowableConcreteStress);

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
         *pPara << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
         
         pgsPointOfInterest poi(spanIdx,gdrIdx,0.0);

         Float64 fcsl = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::BridgeSite3,pgsTypes::ServiceI, pgsTypes::Compression);
         *pPara<<_T("Allowable Compressive Concrete Stresses (5.9.4.2.1)")<<rptNewLine;
         *pPara<<_T("- Service I (permanent + live load) = ")<<stress.SetValue(fcsl)<<rptNewLine;

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            Float64 fcsa = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::BridgeSite3,pgsTypes::ServiceIA, pgsTypes::Compression);
            *pPara<<_T("- Service IA (one-half of permanent + live load) = ")<<stress.SetValue(fcsa)<<rptNewLine;
         }

         Float64 fts = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::BridgeSite3,pgsTypes::ServiceIII, pgsTypes::Tension);
         *pPara<<_T("Allowable Tensile Concrete Stresses (5.9.4.2.2)")<<rptNewLine;
         *pPara<<_T("- Service III = ")<<stress.SetValue(fts)<<rptNewLine;

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            Float64 ftf = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::BridgeSite3,pgsTypes::FatigueI, pgsTypes::Compression);
            *pPara<<_T("Allowable Compressive Concrete Stresses (5.5.3.1)")<<rptNewLine;
            *pPara<<_T("- Fatigue I = ")<<stress.SetValue(ftf)<<rptNewLine;
         }
      }
   }


   Int16 method = pSpecEntry->GetLiveLoadDistributionMethod();
   if (method==LLDF_LRFD)
      *pPara<<_T("LL Distribution factors are calculated in accordance with LRFD 4.6.2.2")<<rptNewLine;
   else if (method==LLDF_WSDOT)
      *pPara<<_T("LL Distribution factors are calculated in accordance with WSDOT Bridge Design Manual")<<rptNewLine;
   else if (method==LLDF_TXDOT)
      *pPara<<_T("LL Distribution factors are calculated in accordance with TxDOT LRFD Bridge Design Manual")<<rptNewLine;
   else
      CHECK(0); // new method?

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   std::_tstring straction = pLiveLoads->GetLLDFSpecialActionText();
   if ( !straction.empty() )
   {
      *pPara << straction << rptNewLine;
   }
}

void write_moment_capacity(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Moment Capacity Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      if (pSpecEntry->GetBs3LRFDOverreinforcedMomentCapacity())
      {
         *pPara << _T("Capacity of over reinforced sections computed in accordance with LRFD C5.7.3.3.1") << rptNewLine;
      }
      else
      {
         *pPara << _T("Capacity of over reinforced sections computed in accordance with WSDOT Bridge Design Manual") << rptNewLine;
      }
   }

   if ( pSpecEntry->IncludeRebarForMoment() )
      *pPara << _T("Longitudinal reinforcing bars included in moment capacity calculations") << rptNewLine;
   else
      *pPara << _T("Longitudinal reinforcing bars ignored in moment capacity calculations") << rptNewLine;


   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );
   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);
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
         *pPara << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
         
         Float64 fr = pMaterial->GetFlexureFrGdr(spanIdx,gdrIdx);
         *pPara << _T("Modulus of rupture = ") << stress.SetValue(fr) << rptNewLine;
      }
   }
}

void write_shear_capacity(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Shear Capacity Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   switch( pSpecEntry->GetShearCapacityMethod() )
   {
   case scmBTEquations:
      *pPara << _T("Shear capacity computed in accordance with LRFD 5.8.3.4.2 (General method)") << rptNewLine;
      break;

   case scmVciVcw:
      *pPara << _T("Shear capacity computed in accordance with LRFD 5.8.3.4.3 (Vci, Vcw method)") << rptNewLine;
      break;

   case scmBTTables:
      *pPara << _T("Shear capacity computed in accordance with LRFD B5.1 (Beta-Theta Tables)") << rptNewLine;
      break;

   case scmWSDOT2001:
      *pPara << _T("Shear capacity computed in accordance with WSDOT Bridge Design Manual (June 2001)") << rptNewLine;
      break;

   case scmWSDOT2007:
      *pPara << _T("Shear capacity computed in accordance with WSDOT Bridge Design Manual (August 2007)") << rptNewLine;
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );
   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);
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
         *pPara << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
         Float64 fr = pMaterial->GetShearFrGdr(spanIdx,gdrIdx);
         *pPara << _T("Modulus of rupture = ") << stress.SetValue(fr) << rptNewLine;
      }
   }

   Int16 method = pSpecEntry->GetLongReinfShearMethod();
   if ( method != WSDOT_METHOD )
   {
      *pPara << _T("Longitudinal reinforcement requirements computed in accordance with LRFD 5.8.3.5") << rptNewLine;
   }
   else
   {
      *pPara << _T("Longitudinal reinforcement requirements computed in accordance with WSDOT Bridge Design Manual") << rptNewLine;
   }


   if ( pSpecEntry->IncludeRebarForShear() )
   {
      *pPara << _T("Longitudinal reinforcing bars included in shear induced tension capacity calculations") << rptNewLine;
   }
   else
   {
      *pPara << _T("Longitudinal reinforcing bars ignored in shear induced tension capacity calculations") << rptNewLine;
   }

   switch ( pSpecEntry->GetShearFlowMethod() )
   {
   case sfmLRFD:
      *pPara << _T("Shear stress at girder/deck interface computed using the LRFD simplified method: ") << Sub2(_T("V"),_T("ui")) << _T(" = ") << _T("V/bd") << rptNewLine;
      break;

   case sfmClassical:
      *pPara << _T("Shear stress at girder/deck interface computed using the classical shear flow formula: ") << Sub2(_T("V"),_T("ui")) << _T(" = (") << Sub2(_T("V"),_T("u")) << _T("Q)") << _T("/") << _T("(Ib)") << rptNewLine;
      break;
   }
}

void write_creep(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Creep Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   Int16 method = pSpecEntry->GetCreepMethod();
   if (method==CREEP_LRFD)
      *pPara<<_T("Creep is calculated in accordance with LRFD (5.4.2.3.2)")<<rptNewLine;
   else if (method==CREEP_WSDOT)
   {
      *pPara<<_T("Creep is calculated in accordance with WSDOT Bridge Design Manual (6.1.2c.2)")<<rptNewLine;
      *pPara<<_T("Creep factor = ")<<pSpecEntry->GetCreepFactor();
   }
   else
      CHECK(0); // new method?

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time, pDisplayUnits->GetLongTimeUnit(), true );

   *pPara<< _T("# of hours from stressing to prestress transfer : ")<<::ConvertFromSysUnits(pSpecEntry->GetXferTime(),unitMeasure::Hour)<<rptNewLine;
   *pPara<< _T("# of days from prestress transfer until removal of temporary strands / diaphram casting : ")<<::ConvertFromSysUnits(pSpecEntry->GetCreepDuration1Min(),unitMeasure::Day) << _T(" Min");
   *pPara<< _T(", ") << ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration1Max(),unitMeasure::Day) << _T(" Max") <<rptNewLine;
   *pPara<< _T("# of days from prestress transfer until slab-girder continuity is achieved : ")<<::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Min(),unitMeasure::Day) << _T(" Min");
   *pPara<< _T(", ") << ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Max(),unitMeasure::Day) << _T(" Max") << rptNewLine;

   *pPara << rptNewLine << rptNewLine;

   *pPara << _T("1 day of steam or radiant heat curing is equal to ") << pSpecEntry->GetCuringMethodTimeAdjustmentFactor() << _T(" days of normal curing") << rptNewLine;
}

void write_losses(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Losses Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time, pDisplayUnits->GetLongTimeUnit(), true );

   int method = pSpecEntry->GetLossMethod();

   if ( method == LOSSES_GENERAL_LUMPSUM )
   {
      *pPara<<_T("Losses calculated per General Lump Sum")<<rptNewLine;
      *pPara<<_T("- Before Prestress Transfer = ")<<stress.SetValue(pSpecEntry->GetBeforeXferLosses())<<rptNewLine;
      *pPara<<_T("- After Prestress Transfer = ")<<stress.SetValue(pSpecEntry->GetAfterXferLosses())<<rptNewLine;
      *pPara<<_T("- At Lifting Losses = ")<<stress.SetValue(pSpecEntry->GetLiftingLosses())<<rptNewLine;
      *pPara<<_T("- At Shipping Losses = ")<<stress.SetValue(pSpecEntry->GetShippingLosses()) << _T(", but not to exceed final losses") << rptNewLine;
      *pPara<<_T("- Before Temp. Strand Removal = ")<<stress.SetValue(pSpecEntry->GetBeforeTempStrandRemovalLosses())<<rptNewLine;
      *pPara<<_T("- After Temp. Strand Removal = ")<<stress.SetValue(pSpecEntry->GetAfterTempStrandRemovalLosses())<<rptNewLine;
      *pPara<<_T("- After Deck Placement = ")<<stress.SetValue(pSpecEntry->GetAfterDeckPlacementLosses())<<rptNewLine;
      *pPara<<_T("- After After Superimposed Dead Loads = ")<<stress.SetValue(pSpecEntry->GetAfterSIDLLosses())<<rptNewLine;
      *pPara<<_T("- Final Losses = ")<<stress.SetValue(pSpecEntry->GetFinalLosses())<<rptNewLine;
   }
   else
   {
      switch( method )
      {
      case LOSSES_AASHTO_REFINED:
         *pPara<<_T("Losses calculated per Refined Estimate Method in accordance with AASHTO LRFD 5.9.5.4")<<rptNewLine;
         break;
      case LOSSES_WSDOT_REFINED:
         *pPara<<_T("Losses calculated per Refined Estimate Method in accordance with AASHTO LRFD 5.9.5.4 and WSDOT Bridge Design")<<rptNewLine;
         break;
      case LOSSES_TXDOT_REFINED_2004:
         *pPara<<_T("Losses calculated per Refined Estimate Method in accordance with AASHTO LRFD 5.9.5.4 and TxDOT Bridge Design")<<rptNewLine;
         break;
      case LOSSES_AASHTO_LUMPSUM:
      case LOSSES_AASHTO_LUMPSUM_2005:
         *pPara<<_T("Losses calculated per Approximate Lump Sum Method in accordnace with AASHTO LRFD 5.9.5.3")<<rptNewLine;
         break;
      case LOSSES_WSDOT_LUMPSUM:
         *pPara<<_T("Losses calculated per Approximate Lump Sum Method in accordnace with AASHTO LRFD 5.9.5.3 and WSDOT Bridge Design Manual") << rptNewLine;
         break;
      default:
         CHECK(false); // Should never get here
      }

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         *pPara << _T("Assumed time at shipping = ") << time.SetValue(pSpecEntry->GetShippingTime()) << rptNewLine;
      }
      else
      {
         Float64 shipping = pSpecEntry->GetShippingLosses();
         if ( shipping < 0 )
         {
            *pPara << _T("- Shipping Losses = ") << (-100.0*shipping) << _T("% of final losses, but not less than losses immediately after prestress transfer") << rptNewLine;
         }
         else
         {
            *pPara<<_T("- Shipping Losses = ")<< stress.SetValue(shipping) << _T(", but not to exceed final losses") << rptNewLine;
         }
      }
   }
}

void write_strand_stress(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Stress Limits for Prestressing Tendons")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pSpecEntry->CheckStrandStress(AT_JACKING) )
   {
      *pPara << rptNewLine;
      *pPara << _T("Stress Limit at Jacking") << rptNewLine;
      *pPara << _T("- Stress Relieved Strand = ") << pSpecEntry->GetStrandStressCoefficient(AT_JACKING,STRESS_REL) << RPT_STRESS(_T("pu")) << rptNewLine;
      *pPara << _T("- Low Relaxation Strand = ") << pSpecEntry->GetStrandStressCoefficient(AT_JACKING,LOW_RELAX) << RPT_STRESS(_T("pu")) << rptNewLine;
   }

   if ( pSpecEntry->CheckStrandStress(BEFORE_TRANSFER) )
   {
      *pPara << rptNewLine;
      *pPara << _T("Stress Limit Immediately Prior to Transfer") << rptNewLine;
      *pPara << _T("- Stress Relieved Strand = ") << pSpecEntry->GetStrandStressCoefficient(BEFORE_TRANSFER,STRESS_REL) << RPT_STRESS(_T("pu")) << rptNewLine;
      *pPara << _T("- Low Relaxation Strand = ") << pSpecEntry->GetStrandStressCoefficient(BEFORE_TRANSFER,LOW_RELAX) << RPT_STRESS(_T("pu")) << rptNewLine;
   }

   if ( pSpecEntry->CheckStrandStress(AFTER_TRANSFER) )
   {
      *pPara << rptNewLine;
      *pPara << _T("Stress Limit Immediately After Transfer") << rptNewLine;
      *pPara << _T("- Stress Relieved Strand = ") << pSpecEntry->GetStrandStressCoefficient(AFTER_TRANSFER,STRESS_REL) << RPT_STRESS(_T("pu")) << rptNewLine;
      *pPara << _T("- Low Relaxation Strand = ") << pSpecEntry->GetStrandStressCoefficient(AFTER_TRANSFER,LOW_RELAX) << RPT_STRESS(_T("pu")) << rptNewLine;
   }

   if ( pSpecEntry->CheckStrandStress(AFTER_ALL_LOSSES) )
   {
      *pPara << rptNewLine;
      *pPara << _T("Stress Limit at service limit state after all losses") << rptNewLine;
      *pPara << _T("- Stress Relieved Strand = ") << pSpecEntry->GetStrandStressCoefficient(AFTER_ALL_LOSSES,STRESS_REL) << RPT_STRESS(_T("py")) << rptNewLine;
      *pPara << _T("- Low Relaxation Strand = ") << pSpecEntry->GetStrandStressCoefficient(AFTER_ALL_LOSSES,LOW_RELAX) << RPT_STRESS(_T("py")) << rptNewLine;
   }
}

void write_deflections(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Deflection Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pSpecEntry->GetDoEvaluateLLDeflection() )
   {
      *pPara << _T("Live Load Deflection Limit: ") << Sub2(_T("L"),_T("span")) << _T("/") << pSpecEntry->GetLLDeflectionLimit() << rptNewLine;
   }
   else
   {
      *pPara << _T("Live Load Deflection Limit not evaluated") << rptNewLine;
   }
}

void write_rating_criteria(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const RatingLibraryEntry* pRatingEntry)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Live Load Factors for Load Rating")<<rptNewLine;

   write_load_factors(pChapter,pDisplayUnits,_T("Design - Inventory"),pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrDesign_Inventory));
   write_load_factors(pChapter,pDisplayUnits,_T("Design - Operating"),pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrDesign_Operating));
   write_load_factors(pChapter,pDisplayUnits,_T("Legal - Routine"),pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrLegal_Routine));
   write_load_factors(pChapter,pDisplayUnits,_T("Legal - Special"),pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrLegal_Special));
   write_load_factors(pChapter,pDisplayUnits,_T("Permit - Routine"),pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrPermit_Routine));
   write_load_factors(pChapter,pDisplayUnits,_T("Permit - Special - Single Trip, escorted"),pRatingEntry->GetLiveLoadFactorModel(pgsTypes::ptSingleTripWithEscort));
   write_load_factors(pChapter,pDisplayUnits,_T("Permit - Special - Single Trip, mixed with traffic"),pRatingEntry->GetLiveLoadFactorModel(pgsTypes::ptSingleTripWithTraffic));
   write_load_factors(pChapter,pDisplayUnits,_T("Permit - Special - Multiple Trip, mixed with traffic"),pRatingEntry->GetLiveLoadFactorModel(pgsTypes::ptMultipleTripWithTraffic));
}

void write_load_factors(rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,LPCTSTR lpszName,const CLiveLoadFactorModel& model)
{
   INIT_UV_PROTOTYPE( rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(),    true );

   rptRcScalar scalar;
   scalar.SetFormat(sysNumericFormatTool::Fixed);
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
   scalar.SetTolerance(1.0e-6);

   std::_tstring strModel;
   pgsTypes::LiveLoadFactorType llfType = model.GetLiveLoadFactorType();
   switch( llfType )
   {
   case pgsTypes::gllSingleValue:
      strModel = _T("Single Value");
      break;

   case pgsTypes::gllStepped:
      strModel = _T("Stepped");
      break;

   case pgsTypes::gllLinear:
      strModel = _T("Linear");
      break;

   case pgsTypes::gllBilinear:
      strModel = _T("Bilinear");
      break;

   case pgsTypes::gllBilinearWithWeight:
      strModel = _T("Bilinear with Vehicle Weight");
      break;

   }

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   *pChapter << pPara;
   *pPara << lpszName << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("Live Load Factor Model: ") << strModel << rptNewLine;

   Int16 adtt1, adtt2, adtt3, adtt4;
   model.GetADTT(&adtt1,&adtt2,&adtt3,&adtt4);

   Float64 g1,g2,g3,g4;
   model.GetLowerLiveLoadFactor(&g1,&g2,&g3,&g4);

   if ( llfType == pgsTypes::gllSingleValue )
   {
      *pPara << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(g1) << rptNewLine;
   }
   else if ( llfType == pgsTypes::gllStepped )
   {
      *pPara << _T("ADTT < ") << adtt1 << _T(" ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(g1) << rptNewLine;
      *pPara << _T("Otherwise ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(g2) << rptNewLine;
      *pPara << _T("ADTT = Unknown ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(g4) << rptNewLine;
   }
   else if ( llfType == pgsTypes::gllLinear )
   {
      *pPara << _T("ADTT < ") << adtt1 << _T(" ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(g1) << rptNewLine;
      *pPara << _T("ADTT > ") << adtt2 << _T(" ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(g2) << rptNewLine;
      *pPara << _T("ADTT = Unknown ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(g4) << rptNewLine;

      *pPara << rptNewLine;
      if ( model.GetLiveLoadFactorModifier() == pgsTypes::gllmRoundUp )
      {
         *pPara << _T("Load factors are rounded up") << rptNewLine;
      }
      else
      {
         *pPara << _T("Load factors are linearly interpolated") << rptNewLine;
      }
   }
   else if ( llfType == pgsTypes::gllBilinear )
   {
      *pPara << _T("ADTT < ") << adtt1 << _T(" ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(g1) << rptNewLine;
      *pPara << _T("ADTT = ") << adtt2 << _T(" ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(g2) << rptNewLine;
      *pPara << _T("ADTT > ") << adtt3 << _T(" ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(g3) << rptNewLine;
      *pPara << _T("ADTT = Unknown ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(g4) << rptNewLine;

      *pPara << rptNewLine;
      if ( model.GetLiveLoadFactorModifier() == pgsTypes::gllmRoundUp )
      {
         *pPara << _T("Load factors are rounded up") << rptNewLine;
      }
      else
      {
         *pPara << _T("Load factors are linearly interpolated") << rptNewLine;
      }
   }
   else if ( llfType == pgsTypes::gllBilinearWithWeight )
   {
      Float64 Wlower, Wupper;
      model.GetVehicleWeight(&Wlower,&Wupper);
      *pPara << _T("For vehicle weight up to ") << force.SetValue(Wlower) << rptNewLine;
      *pPara << _T("ADTT < ") << adtt1 << _T(" ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(g1) << rptNewLine;
      *pPara << _T("ADTT = ") << adtt2 << _T(" ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(g2) << rptNewLine;
      *pPara << _T("ADTT > ") << adtt3 << _T(" ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(g3) << rptNewLine;
      *pPara << _T("ADTT = Unknown ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(g4) << rptNewLine;

      *pPara << rptNewLine;

      Float64 ga,gb,gc,gd;
      model.GetUpperLiveLoadFactor(&ga,&gb,&gc,&gd);
      *pPara << _T("For vehicle weight of ") << force.SetValue(Wupper) << _T(" or more") << rptNewLine;
      *pPara << _T("ADTT < ") << adtt1 << _T(" ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(ga) << rptNewLine;
      *pPara << _T("ADTT = ") << adtt2 << _T(" ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(gb) << rptNewLine;
      *pPara << _T("ADTT > ") << adtt3 << _T(" ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(gc) << rptNewLine;
      *pPara << _T("ADTT = Unknown ") << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(gd) << rptNewLine;

      *pPara << rptNewLine;
      if ( model.GetLiveLoadFactorModifier() == pgsTypes::gllmRoundUp )
      {
         *pPara << _T("Load factors are rounded up") << rptNewLine;
      }
      else
      {
         *pPara << _T("Load factors are linearly interpolated") << rptNewLine;
      }
   }
   else
   {
      ATLASSERT(false); // is there a new model???
   }
}
