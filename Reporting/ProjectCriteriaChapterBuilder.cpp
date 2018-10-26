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
void write_load_factors(rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const char* lpszName,const CLiveLoadFactorModel& model);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProjectCriteriaChapterBuilder::CProjectCriteriaChapterBuilder(bool bRating)
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
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2( pBroker, ISpecification, pSpec );
   std::string spec_name = pSpec->GetSpecification();
   std::string rating_name = pRatingSpec->GetRatingSpecification();

   GET_IFACE2( pBroker, ILibrary, pLib );
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   const RatingLibraryEntry* pRatingEntry = pLib->GetRatingEntry( rating_name.c_str() );

   rptParagraph* pPara;

   if ( bRating )
   {
      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << "Load Rating Criteria" << rptNewLine;
      *pPara << Bold("Name: ") << rating_name << rptNewLine;
      *pPara << Bold("Description: ") << pRatingEntry->GetDescription() << rptNewLine;
      *pPara <<Bold("Based on:  ");
      switch( pRatingEntry->GetSpecificationVersion() )
      {
      case lrfrVersionMgr::FirstEdition2008:
         *pPara << "AASHTO Manual for Bridge Evaluation, 1st Edition, 2008" << rptNewLine;
         break;

      default:
         ATLASSERT(false);
         *pPara <<"Unknown" << rptNewLine;
         break;
      }

      // write load rating criteria here

      *pPara << "Load Rating Criteria includes " << spec_name << rptNewLine;
   }


   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara <<Bold("Name: ")<< spec_name << rptNewLine;
   *pPara <<Bold("Description: ")<<pSpecEntry->GetDescription()<<rptNewLine;
   *pPara <<Bold("Based on:  ");

   lrfdVersionMgr::Version vers = pSpecEntry->GetSpecificationType();
   switch(vers)
   {
      case lrfdVersionMgr::FirstEdition1994:
         *pPara <<"AASHTO LRFD Bridge Design Specifications, 1st Edition, 1994";
         break;
      case lrfdVersionMgr::FirstEditionWith1996Interims:
         *pPara <<"AASHTO LRFD Bridge Design Specifications, 1st Edition, 1994 with 1996 interim provisions";
         break;
      case lrfdVersionMgr::FirstEditionWith1997Interims:
         *pPara <<"AASHTO LRFD Bridge Design Specifications, 1st Edition, 1994 with 1996 and 1997 interim provisions";
         break;
      case lrfdVersionMgr::SecondEdition1998:
         *pPara <<"AASHTO LRFD Bridge Design Specifications, 2nd Edition, 1998";
         break;
      case lrfdVersionMgr::SecondEditionWith1999Interims:
         *pPara <<"AASHTO LRFD Bridge Design Specifications, 2nd Edition, 1998 with 1999 interim provisions";
         break;
      case lrfdVersionMgr::SecondEditionWith2000Interims:
         *pPara <<"AASHTO LRFD Bridge Design Specifications, 2nd Edition, 1998 with 1999 and 2000 interim provisions";
         break;
      case lrfdVersionMgr::SecondEditionWith2001Interims:
         *pPara <<"AASHTO LRFD Bridge Design Specifications, 2nd Edition, 1998 with 1999 - 2001 interim provisions";
         break;
      case lrfdVersionMgr::SecondEditionWith2002Interims:
         *pPara <<"AASHTO LRFD Bridge Design Specifications, 2nd Edition, 1998 with 1999 - 2002 interim provisions";
         break;
      case lrfdVersionMgr::SecondEditionWith2003Interims:
         *pPara <<"AASHTO LRFD Bridge Design Specifications, 2nd Edition, 1998 with 1999 - 2003 interim provisions";
         break;
      case lrfdVersionMgr::ThirdEdition2004:
         *pPara <<"AASHTO LRFD Bridge Design Specifications, 3rd Edition, 2004";
         break;
      case lrfdVersionMgr::ThirdEditionWith2005Interims:
         *pPara <<"AASHTO LRFD Bridge Design Specifications, 3rd Edition, 2004 with 2005 interim provisions";
         break;
      case lrfdVersionMgr::ThirdEditionWith2006Interims:
         *pPara <<"AASHTO LRFD Bridge Design Specifications, 3rd Edition, 2004 with 2005 - 2006 interim provisions";
         break;
      case lrfdVersionMgr::FourthEdition2007:
         *pPara <<"AASHTO LRFD Bridge Design Specifications, 4th Edition, 2007";
         break;
      case lrfdVersionMgr::FourthEditionWith2008Interims:
         *pPara <<"AASHTO LRFD Bridge Design Specifications, 4th Edition, 2007 with 2008 interim provisions";
         break;
      case lrfdVersionMgr::FourthEditionWith2009Interims:
         *pPara <<"AASHTO LRFD Bridge Design Specifications, 4th Edition, 2007 with 2008 - 2009 interim provisions";
         break;
      default:
         ATLASSERT(false);
         *pPara <<"Unknown";
         break;
   }
   
   lrfdVersionMgr::Units units = pSpecEntry->GetSpecificationUnits();
   if (units==lrfdVersionMgr::SI)
      *pPara<<" - SI Units"<<rptNewLine;
   else
      *pPara<<" - US Units"<<rptNewLine;


   write_load_modifiers(pChapter, pBroker, pDisplayUnits);
   write_environmental_conditions(pChapter, pBroker, pDisplayUnits);
   write_structural_analysis(pChapter, pBroker, pDisplayUnits);
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

   rptRcTable* p_table = pgsReportStyleHolder::CreateTableNoHeading(2,"Load Modifiers");
   *pPara << p_table;

   GET_IFACE2(pBroker,ILoadModifiers,pLoadModifiers);

   (*p_table)(0,0) << "Ductility  - "<< Sub2(symbol(eta),"D");
   (*p_table)(0,1) <<  pLoadModifiers->GetDuctilityFactor();

   (*p_table)(1,0) << "Importance - "<< Sub2(symbol(eta),"I");
   (*p_table)(1,1) <<  pLoadModifiers->GetImportanceFactor();

   (*p_table)(2,0) << "Redundancy - "<< Sub2(symbol(eta),"R");
   (*p_table)(2,1) <<  pLoadModifiers->GetRedundancyFactor();
}

void write_environmental_conditions(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = pgsReportStyleHolder::CreateTableNoHeading(2,"Environmental Conditions");
   *pPara << p_table;

   GET_IFACE2(pBroker,IEnvironment,pEnvironment);

   (*p_table)(0,0) << "Exposure Condition";
   enumExposureCondition cond = pEnvironment->GetExposureCondition();
   if (cond==expNormal)
      (*p_table)(0,1) << "Normal";
   else
      (*p_table)(0,1) << "Severe";

   (*p_table)(1,0) << "Relative Humidity";
   (*p_table)(1,1) <<  pEnvironment->GetRelHumidity()<<"%";
}

void write_structural_analysis(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;

   *pPara << "Structural Analysis Method" << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2( pBroker, ISpecification, pSpec );

   switch( pSpec->GetAnalysisType() )
   {
   case pgsTypes::Simple:
      *pPara << "Simple Span Analysis" << rptNewLine;
      break;

   case pgsTypes::Continuous:
      *pPara << "Continuous Span Analysis" << rptNewLine;
      break;

   case pgsTypes::Envelope:
      *pPara << "Envelope of Simple and Continuous Span Analyses" << rptNewLine;
      break;
   }
}

void write_casting_yard(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<"Casting Yard Criteria"<<rptNewLine;

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
      *pPara<<"Max. slope for 0.5\" " << symbol(phi) << " strands = 1:"<<slope05<<rptNewLine;
      *pPara<<"Max. slope for 0.6\" " << symbol(phi) << " strands = 1:"<<slope06<<rptNewLine;
      *pPara<<"Max. slope for 0.7\" " << symbol(phi) << " strands = 1:"<<slope07<<rptNewLine;
   }
   else
      *pPara <<"Max. Strand slope is not checked"<<rptNewLine;

   Float64 f;
   pSpecEntry->GetHoldDownForce(&do_check, &do_design, &f);
   if (do_check)
      *pPara<<"Max. hold down force in casting yard = "<<force.SetValue(f)<<rptNewLine;
   else
      *pPara <<"Max. hold down force in casting yard  is not checked"<<rptNewLine;

   int method = pSpecEntry->GetCuringMethod();
   if (method==CURING_NORMAL)
      *pPara <<"Girder was cured using Normal method"<<rptNewLine;
   else if (method==CURING_ACCELERATED)
      *pPara <<"Girder was cured using Accelerated method"<<rptNewLine;
   else
      CHECK(0);

   *pPara<<"Max stirrup spacing = "<< dim.SetValue(pSpecEntry->GetMaxStirrupSpacing())<<rptNewLine;

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
         *pPara<<"Span " << LABEL_SPAN(spanIdx) << " Girder " << LABEL_GIRDER(gdrIdx) << rptNewLine;
         *pPara<<"Allowable Concrete Stresses - Service I (5.8.4.1.1)"<<rptNewLine;
         *pPara<<"- Compressive Stress = "<<stress.SetValue(fccy)<<rptNewLine;
         *pPara<<"- Tensile Stress (w/o mild rebar) = "<<stress.SetValue(ftcy) << rptNewLine;
         *pPara<<"- Tensile Stress (w/  mild rebar) = "<<stress.SetValue(ft) << rptNewLine;
      } // gdrIdx
   } // spanIdx

   if (pSpecEntry->IsAnchorageCheckEnabled())
   {
      if ( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() )
         *pPara<<"Splitting zone length: h/" << pSpecEntry->GetSplittingZoneLengthFactor() << rptNewLine;
      else
         *pPara<<"Bursting zone length: h/" << pSpecEntry->GetSplittingZoneLengthFactor() << rptNewLine;
   }
   else
   {
      if ( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() )
         *pPara<<"Splitting and confinement checks (5.10.10) are disabled." << rptNewLine;
      else
         *pPara<<"Bursting and confinement checks (5.10.10) are disabled." << rptNewLine;
   }
}

void write_lifting(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<"Lifting Criteria"<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), true );

   *pPara<<"Factors of Safety"<<rptNewLine;
   *pPara<<"- Cracking F.S. = "<< pSpecEntry->GetCyLiftingCrackFs()<<rptNewLine;
   *pPara<<"- Failure F.S. = "<< pSpecEntry->GetCyLiftingFailFs()<<rptNewLine;

   *pPara<<"Impact Factors"<<rptNewLine;
   *pPara<<"- Upward   = "<< pSpecEntry->GetCyLiftingUpwardImpact()<<rptNewLine;
   *pPara<<"- Downward = "<< pSpecEntry->GetCyLiftingDownwardImpact()<<rptNewLine;

   *pPara<<"Height of pick point above top of girder = "<<dim.SetValue(pSpecEntry->GetPickPointHeight())<<rptNewLine;
   *pPara<<"Lifting loop placement tolerance = "<<dim.SetValue(pSpecEntry->GetLiftingLoopTolerance())<<rptNewLine;
   *pPara<<"Max. girder sweep tolerance = "<<pSpecEntry->GetMaxGirderSweepLifting()<<rptNewLine;
   *pPara<<"Min. angle of inclination of lifting cables = "<<angle.SetValue(pSpecEntry->GetMinCableInclination())<<rptNewLine;

   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
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
         *pPara << "Span " << LABEL_SPAN(spanIdx) << " Girder " << LABEL_GIRDER(gdrIdx) << rptNewLine;

         Float64 fr = pGirderLiftingSpecCriteria->GetLiftingModulusOfRupture(pMaterial->GetFcGdr(spanIdx,gdrIdx));
         *pPara << "Modulus of rupture = " << stress.SetValue(fr) << rptNewLine;
         
         Float64 fccy = pGirderLiftingSpecCriteria->GetLiftingAllowableCompressiveConcreteStress(spanIdx,gdrIdx);
         Float64 ftcy = pGirderLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStress(spanIdx,gdrIdx);
         Float64 ft   = pGirderLiftingSpecCriteria->GetLiftingWithMildRebarAllowableStress(spanIdx,gdrIdx);
         *pPara<<"Allowable Concrete Stresses - Lifting (5.9.4.1.1)"<<rptNewLine;
         *pPara<<"- Compressive Stress = "<<stress.SetValue(fccy)<<rptNewLine;
         *pPara<<"- Tensile Stress (w/o mild rebar) = "<<stress.SetValue(ftcy) << rptNewLine;
         *pPara<<"- Tensile Stress (w/  mild rebar) = "<<stress.SetValue(ft) << rptNewLine;
      } // gdrIdx
   } // spanIdx
}

void write_hauling(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<"Hauling Criteria"<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim2, pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force, pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptMomentPerAngleUnitValue, spring, pDisplayUnits->GetMomentPerAngleUnit(),true );

   *pPara<<"Factors of Safety"<<rptNewLine;
   *pPara<<"- Cracking F.S. = "<< pSpecEntry->GetHaulingCrackFs()<<rptNewLine;
   *pPara<<"- Roll Over F.S. = "<< pSpecEntry->GetHaulingFailFs()<<rptNewLine;

   *pPara<<"Impact Factors"<<rptNewLine;
   *pPara<<"- Upward   = "<< pSpecEntry->GetHaulingUpwardImpact()<<rptNewLine;
   *pPara<<"- Downward = "<< pSpecEntry->GetHaulingDownwardImpact()<<rptNewLine;

   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pHauling);

   if ( pHauling->GetRollStiffnessMethod() == IGirderHaulingSpecCriteria::LumpSum )
   {
      *pPara<<"Roll stiffness of trailer = "<<spring.SetValue(pHauling->GetLumpSumRollStiffness())<<rptNewLine;
   }
   else
   {
      *pPara<<"Maximum Weight Per Axle = "<< force.SetValue(pHauling->GetAxleWeightLimit())<<rptNewLine;
      *pPara<<"Stiffness Per Axle = "<< spring.SetValue(pHauling->GetAxleStiffness())<<rptNewLine;
      *pPara<<"Minimum Roll Stiffness = "<< spring.SetValue(pHauling->GetMinimumRollStiffness())<<rptNewLine;
   }
   *pPara<<"Height of girder bottom above roadway = "<<dim.SetValue(pHauling->GetHeightOfGirderBottomAboveRoadway())<<rptNewLine;
   *pPara<<"Height of truck roll center above roadway = "<<dim.SetValue(pHauling->GetHeightOfTruckRollCenterAboveRoadway())<<rptNewLine;
   *pPara<<"C-C distance between truck tires = "<<dim.SetValue(pHauling->GetAxleWidth())<<rptNewLine;
   *pPara<<"Max. distance between girder supports = "<<dim2.SetValue(pHauling->GetAllowableDistanceBetweenSupports())<<rptNewLine;
   *pPara<<"Max. leading overhang (nearest truck cab) = "<<dim2.SetValue(pHauling->GetAllowableLeadingOverhang() )<<rptNewLine;
   *pPara<<"Max. superelevation = "<<pHauling->GetMaxSuperelevation()<<rptNewLine;
   *pPara<<"Girder sweep tolerance = "<<pHauling->GetHaulingSweepTolerance()<<rptNewLine;
   *pPara<<"Support placement lateral tolerance = "<<dim.SetValue(pHauling->GetHaulingSupportPlacementTolerance())<<rptNewLine;
   *pPara<<"Increase of girder camber for CG = "<<pHauling->GetIncreaseInCgForCamber()<<rptNewLine;
   *pPara<<"Max. girder weight = "<< force.SetValue(pHauling->GetMaxGirderWgt())<<rptNewLine;

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
         *pPara << "Span " << LABEL_SPAN(spanIdx) << " Girder " << LABEL_GIRDER(gdrIdx) << rptNewLine;

         Float64 fr = pHauling->GetHaulingModulusOfRupture(pMaterial->GetFciGdr(spanIdx,gdrIdx));
         *pPara << "Modulus of rupture = " << stress.SetValue(fr) << rptNewLine;

         Float64 fccy = pHauling->GetHaulingAllowableCompressiveConcreteStress(spanIdx,gdrIdx);
         Float64 ftcy = pHauling->GetHaulingAllowableTensileConcreteStress(spanIdx,gdrIdx);
         Float64 ft   = pHauling->GetHaulingWithMildRebarAllowableStress(spanIdx,gdrIdx);
         *pPara<<"Allowable Concrete Stresses - Hauling (5.9.4.2.1)"<<rptNewLine;
         *pPara<<"- Compressive Stress = "<<stress.SetValue(fccy)<<rptNewLine;
         *pPara<<"- Tensile Stress (w/o mild rebar) = "<<stress.SetValue(ftcy) << rptNewLine;
         *pPara<<"- Tensile Stress (w/  mild rebar) = "<<stress.SetValue(ft) << rptNewLine;
      }
   }
}

void write_temp_strand_removal(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<"Temporary Strand Removal Criteria"<<rptNewLine;

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
         *pPara << "Span " << LABEL_SPAN(spanIdx) << " Girder " << LABEL_GIRDER(gdrIdx) << rptNewLine;
         
         pgsPointOfInterest poi(spanIdx,gdrIdx,0.0);
         Float64 fcsp = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::TemporaryStrandRemoval,pgsTypes::ServiceI, pgsTypes::Compression);
         *pPara<<"Allowable Compressive Concrete Stresses (5.9.4.2.1)"<<rptNewLine;
         *pPara<<"- Service I = "<<stress.SetValue(fcsp)<<rptNewLine;

         Float64 fts = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::TemporaryStrandRemoval,pgsTypes::ServiceI, pgsTypes::Tension);
         *pPara<<"Allowable Tensile Concrete Stresses (5.9.4.2.2)"<<rptNewLine;
         *pPara<<"- Service I = "<<stress.SetValue(fts)<<rptNewLine;
      }
   }
}

void write_bridge_site1(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<"Deck and Diaphragm Placement Stage (Bridge Site 1) Criteria"<<rptNewLine;

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
         *pPara << "Span " << LABEL_SPAN(spanIdx) << " Girder " << LABEL_GIRDER(gdrIdx) << rptNewLine;
         
         pgsPointOfInterest poi(spanIdx,gdrIdx,0.0);
         Float64 fcsp = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::BridgeSite1,pgsTypes::ServiceI, pgsTypes::Compression);
         *pPara<<"Allowable Compressive Concrete Stresses (5.9.4.2.1)"<<rptNewLine;
         *pPara<<"- Service I = "<<stress.SetValue(fcsp)<<rptNewLine;

         Float64 fts = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::BridgeSite1,pgsTypes::ServiceI, pgsTypes::Tension);
         *pPara<<"Allowable Tensile Concrete Stresses (5.9.4.2.2)"<<rptNewLine;
         *pPara<<"- Service I = "<<stress.SetValue(fts)<<rptNewLine;
      }
   }
}

void write_bridge_site2(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<"Superimposed Dead Load Stage (Bridge Site 2) Criteria"<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );

   switch ( pSpecEntry->GetTrafficBarrierDistributionType() )
   {
   case pgsTypes::tbdGirder:
      *pPara << "Railing system weight is distributed to " << pSpecEntry->GetMaxGirdersDistTrafficBarrier() << " girders" << rptNewLine;
      break;

   case pgsTypes::tbdMatingSurface:
      *pPara << "Railing system weight is distributed to " << pSpecEntry->GetMaxGirdersDistTrafficBarrier() << " mating surfaces" << rptNewLine;
      break;

   case pgsTypes::tbdWebs:
      *pPara << "Railing system weight is distributed to " << pSpecEntry->GetMaxGirdersDistTrafficBarrier() << " webs" << rptNewLine;
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
         *pPara << "Span " << LABEL_SPAN(spanIdx) << " Girder " << LABEL_GIRDER(gdrIdx) << rptNewLine;
         
         pgsPointOfInterest poi(spanIdx,gdrIdx,0.0);
         Float64 fcsp = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::BridgeSite2,pgsTypes::ServiceI, pgsTypes::Compression);
         *pPara<<"Allowable Compressive Concrete Stresses (5.9.4.2.1)"<<rptNewLine;
         *pPara<<"- Service I = "<<stress.SetValue(fcsp)<<rptNewLine;
      }
   }

   *pPara << rptNewLine;
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth )
   {
      *pPara << "Effective Flange Width taken to be the tributary width" << rptNewLine;
   }
   else
   {
      *pPara << "Effective Flange Width computed in accordance with LRFD 4.6.2.6" << rptNewLine;
   }

   *pPara << rptNewLine;
   if( pSpecEntry->GetOverlayLoadDistributionType()==pgsTypes::olDistributeTributaryWidth)
   {
      *pPara << "Overlay load is distributed using tributary width."<< rptNewLine;
   }
   else
   {
      *pPara << "Overlay load is distributed uniformly among all girders per LRFD 4.6.2.2.1"<< rptNewLine;
   }
}

void write_bridge_site3(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<"Final with Live Load Stage (Bridge Site 3) Criteria"<<rptNewLine;

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
         *pPara << "Span " << LABEL_SPAN(spanIdx) << " Girder " << LABEL_GIRDER(gdrIdx) << rptNewLine;
         
         pgsPointOfInterest poi(spanIdx,gdrIdx,0.0);

         Float64 fcsl = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::BridgeSite3,pgsTypes::ServiceI, pgsTypes::Compression);
         *pPara<<"Allowable Compressive Concrete Stresses (5.9.4.2.1)"<<rptNewLine;
         *pPara<<"- Service I (permanent + live load) = "<<stress.SetValue(fcsl)<<rptNewLine;

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            Float64 fcsa = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::BridgeSite3,pgsTypes::ServiceIA, pgsTypes::Compression);
            *pPara<<"- Service IA (one-half of permanent + live load) = "<<stress.SetValue(fcsa)<<rptNewLine;
         }

         Float64 fts = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::BridgeSite3,pgsTypes::ServiceIII, pgsTypes::Tension);
         *pPara<<"Allowable Tensile Concrete Stresses (5.9.4.2.2)"<<rptNewLine;
         *pPara<<"- Service III = "<<stress.SetValue(fts)<<rptNewLine;

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            Float64 ftf = pAllowableConcreteStress->GetAllowableStress(poi, pgsTypes::BridgeSite3,pgsTypes::FatigueI, pgsTypes::Compression);
            *pPara<<"Allowable Compressive Concrete Stresses (5.5.3.1)"<<rptNewLine;
            *pPara<<"- Fatigue I = "<<stress.SetValue(ftf)<<rptNewLine;
         }
      }
   }


   Int16 method = pSpecEntry->GetLiveLoadDistributionMethod();
   if (method==LLDF_LRFD)
      *pPara<<"LL Distribution factors are calculated in accordance with LRFD 4.6.2.2"<<rptNewLine;
   else if (method==LLDF_WSDOT)
      *pPara<<"LL Distribution factors are calculated in accordance with WSDOT Bridge Design Manual"<<rptNewLine;
   else if (method==LLDF_TXDOT)
      *pPara<<"LL Distribution factors are calculated in accordance with TxDOT LRFD Bridge Design Manual"<<rptNewLine;
   else
      CHECK(0); // new method?

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   std::string straction = pLiveLoads->GetLLDFSpecialActionText();
   if ( !straction.empty() )
   {
      *pPara << straction << rptNewLine;
   }
}

void write_moment_capacity(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<"Moment Capacity Criteria"<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      if (pSpecEntry->GetBs3LRFDOverreinforcedMomentCapacity())
      {
         *pPara << "Capacity of over reinforced sections computed in accordance with LRFD C5.7.3.3.1" << rptNewLine;
      }
      else
      {
         *pPara << "Capacity of over reinforced sections computed in accordance with WSDOT Bridge Design Manual" << rptNewLine;
      }
   }

   if ( pSpecEntry->IncludeRebarForMoment() )
      *pPara << "Longitudinal reinforcing bars included in moment capacity calculations" << rptNewLine;
   else
      *pPara << "Longitudinal reinforcing bars ignored in moment capacity calculations" << rptNewLine;


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
         *pPara << "Span " << LABEL_SPAN(spanIdx) << " Girder " << LABEL_GIRDER(gdrIdx) << rptNewLine;
         
         Float64 fr = pMaterial->GetFlexureFrGdr(spanIdx,gdrIdx);
         *pPara << "Modulus of rupture = " << stress.SetValue(fr) << rptNewLine;
      }
   }
}

void write_shear_capacity(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,SpanIndexType span,GirderIndexType gdr)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<"Shear Capacity Criteria"<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   switch( pSpecEntry->GetShearCapacityMethod() )
   {
   case scmBTEquations:
      *pPara << "Shear capacity computed in accordance with LRFD 5.8.3.4.2 (General method)" << rptNewLine;
      break;

   case scmVciVcw:
      *pPara << "Shear capacity computed in accordance with LRFD 5.8.3.4.3 (Vci, Vcw method)" << rptNewLine;
      break;

   case scmBTTables:
      *pPara << "Shear capacity computed in accordance with LRFD B5.1 (Beta-Theta Tables)" << rptNewLine;
      break;

   case scmWSDOT2001:
      *pPara << "Shear capacity computed in accordance with WSDOT Bridge Design Manual (June 2001)" << rptNewLine;
      break;

   case scmWSDOT2007:
      *pPara << "Shear capacity computed in accordance with WSDOT Bridge Design Manual (August 2007)" << rptNewLine;
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
         *pPara << "Span " << LABEL_SPAN(spanIdx) << " Girder " << LABEL_GIRDER(gdrIdx) << rptNewLine;
         Float64 fr = pMaterial->GetShearFrGdr(spanIdx,gdrIdx);
         *pPara << "Modulus of rupture = " << stress.SetValue(fr) << rptNewLine;
      }
   }

   Int16 method = pSpecEntry->GetLongReinfShearMethod();
   if ( method != WSDOT_METHOD )
   {
      *pPara << "Longitudinal reinforcement requirements computed in accordance with LRFD 5.8.3.5" << rptNewLine;
   }
   else
   {
      *pPara << "Longitudinal reinforcement requirements computed in accordance with WSDOT Bridge Design Manual" << rptNewLine;
   }


   if ( pSpecEntry->IncludeRebarForShear() )
   {
      *pPara << "Longitudinal reinforcing bars included in shear induced tension capacity calculations" << rptNewLine;
   }
   else
   {
      *pPara << "Longitudinal reinforcing bars ignored in shear induced tension capacity calculations" << rptNewLine;
   }

   switch ( pSpecEntry->GetShearFlowMethod() )
   {
   case sfmLRFD:
      *pPara << "Shear stress at girder/deck interface computed using the LRFD simplified method: " << Sub2("V","ui") << " = " << "V/bd" << rptNewLine;
      break;

   case sfmClassical:
      *pPara << "Shear stress at girder/deck interface computed using the classical shear flow formula: " << Sub2("V","ui") << " = (" << Sub2("V","u") << "Q)" << "/" << "(Ib)" << rptNewLine;
      break;
   }
}

void write_creep(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<"Creep Criteria"<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   Int16 method = pSpecEntry->GetCreepMethod();
   if (method==CREEP_LRFD)
      *pPara<<"Creep is calculated in accordance with LRFD (5.4.2.3.2)"<<rptNewLine;
   else if (method==CREEP_WSDOT)
   {
      *pPara<<"Creep is calculated in accordance with WSDOT Bridge Design Manual (6.1.2c.2)"<<rptNewLine;
      *pPara<<"Creep factor = "<<pSpecEntry->GetCreepFactor();
   }
   else
      CHECK(0); // new method?

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time, pDisplayUnits->GetLongTimeUnit(), true );

   *pPara<< "# of hours from stressing to prestress transfer : "<<::ConvertFromSysUnits(pSpecEntry->GetXferTime(),unitMeasure::Hour)<<rptNewLine;
   *pPara<< "# of days from prestress transfer until removal of temporary strands / diaphram casting : "<<::ConvertFromSysUnits(pSpecEntry->GetCreepDuration1Min(),unitMeasure::Day) << " Min";
   *pPara<< ", " << ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration1Max(),unitMeasure::Day) << " Max" <<rptNewLine;
   *pPara<< "# of days from prestress transfer until slab-girder continuity is achieved : "<<::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Min(),unitMeasure::Day) << " Min";
   *pPara<< ", " << ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Max(),unitMeasure::Day) << " Max" << rptNewLine;

   *pPara << rptNewLine << rptNewLine;

   *pPara << "1 day of steam or radiant heat curing is equal to " << pSpecEntry->GetCuringMethodTimeAdjustmentFactor() << " days of normal curing" << rptNewLine;
}

void write_losses(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<"Losses Criteria"<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );

   int method = pSpecEntry->GetLossMethod();

   if ( method == LOSSES_GENERAL_LUMPSUM )
   {
      *pPara<<"Losses calculated per General Lump Sum"<<rptNewLine;
      *pPara<<"- Befor Transfer Losses = "<<stress.SetValue(pSpecEntry->GetBeforeXferLosses())<<rptNewLine;
      *pPara<<"- After Transfer Losses = "<<stress.SetValue(pSpecEntry->GetAfterXferLosses())<<rptNewLine;
      *pPara<<"- Shipping Losses = "<<stress.SetValue(pSpecEntry->GetShippingLosses()) << ", but not to exceed final losses" << rptNewLine;
      *pPara<<"- Final Losses = "<<stress.SetValue(pSpecEntry->GetFinalLosses())<<rptNewLine;
   }
   else
   {
      switch( method )
      {
      case LOSSES_AASHTO_REFINED:
         *pPara<<"Losses calculated per Refined Estimate Method in accordance with AASHTO LRFD 5.9.5.4"<<rptNewLine;
         break;
      case LOSSES_WSDOT_REFINED:
         *pPara<<"Losses calculated per Refined Estimate Method in accordance with AASHTO LRFD 5.9.5.4 and WSDOT Bridge Design"<<rptNewLine;
         break;
      case LOSSES_TXDOT_REFINED_2004:
         *pPara<<"Losses calculated per Refined Estimate Method in accordance with AASHTO LRFD 5.9.5.4 and TxDOT Bridge Design"<<rptNewLine;
         break;
      case LOSSES_AASHTO_LUMPSUM:
      case LOSSES_AASHTO_LUMPSUM_2005:
         *pPara<<"Losses calculated per Approximate Lump Sum Method in accordnace with AASHTO LRFD 5.9.5.3"<<rptNewLine;
         break;
      case LOSSES_WSDOT_LUMPSUM:
         *pPara<<"Losses calculated per Approximate Lump Sum Method in accordnace with AASHTO LRFD 5.9.5.3 and WSDOT Bridge Design Manual" << rptNewLine;
         break;
      default:
         CHECK(false); // Should never get here
      }

      Float64 shipping = pSpecEntry->GetShippingLosses();
      if ( shipping < 0 )
      {
         *pPara << "- Shipping Losses = " << (-100.0*shipping) << "% of final losses, but not less than losses immediately after prestress transfer" << rptNewLine;
      }
      else
      {
         *pPara<<"- Shipping Losses = "<< stress.SetValue(shipping) << ", but not to exceed final losses" << rptNewLine;
      }
   }
}

void write_strand_stress(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<"Stress Limits for Prestressing Tendons"<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pSpecEntry->CheckStrandStress(AT_JACKING) )
   {
      *pPara << rptNewLine;
      *pPara << "Stress Limit at Jacking" << rptNewLine;
      *pPara << "- Stress Relieved Strand = " << pSpecEntry->GetStrandStressCoefficient(AT_JACKING,STRESS_REL) << Sub2("f","pu") << rptNewLine;
      *pPara << "- Low Relaxation Strand = " << pSpecEntry->GetStrandStressCoefficient(AT_JACKING,LOW_RELAX) << Sub2("f","pu") << rptNewLine;
   }

   if ( pSpecEntry->CheckStrandStress(BEFORE_TRANSFER) )
   {
      *pPara << rptNewLine;
      *pPara << "Stress Limit Immediately Prior to Transfer" << rptNewLine;
      *pPara << "- Stress Relieved Strand = " << pSpecEntry->GetStrandStressCoefficient(BEFORE_TRANSFER,STRESS_REL) << Sub2("f","pu") << rptNewLine;
      *pPara << "- Low Relaxation Strand = " << pSpecEntry->GetStrandStressCoefficient(BEFORE_TRANSFER,LOW_RELAX) << Sub2("f","pu") << rptNewLine;
   }

   if ( pSpecEntry->CheckStrandStress(AFTER_TRANSFER) )
   {
      *pPara << rptNewLine;
      *pPara << "Stress Limit Immediately After Transfer" << rptNewLine;
      *pPara << "- Stress Relieved Strand = " << pSpecEntry->GetStrandStressCoefficient(AFTER_TRANSFER,STRESS_REL) << Sub2("f","pu") << rptNewLine;
      *pPara << "- Low Relaxation Strand = " << pSpecEntry->GetStrandStressCoefficient(AFTER_TRANSFER,LOW_RELAX) << Sub2("f","pu") << rptNewLine;
   }

   if ( pSpecEntry->CheckStrandStress(AFTER_ALL_LOSSES) )
   {
      *pPara << rptNewLine;
      *pPara << "Stress Limit at service limit state after all losses" << rptNewLine;
      *pPara << "- Stress Relieved Strand = " << pSpecEntry->GetStrandStressCoefficient(AFTER_ALL_LOSSES,STRESS_REL) << Sub2("f","py") << rptNewLine;
      *pPara << "- Low Relaxation Strand = " << pSpecEntry->GetStrandStressCoefficient(AFTER_ALL_LOSSES,LOW_RELAX) << Sub2("f","py") << rptNewLine;
   }
}

void write_deflections(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<"Deflection Criteria"<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pSpecEntry->GetDoEvaluateLLDeflection() )
   {
      *pPara << "Live Load Deflection Limit: " << Sub2("L","span") << "/" << pSpecEntry->GetLLDeflectionLimit() << rptNewLine;
   }
   else
   {
      *pPara << "Live Load Deflection Limit not evaluated" << rptNewLine;
   }
}

void write_rating_criteria(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const RatingLibraryEntry* pRatingEntry)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<"Live Load Factors for Load Rating"<<rptNewLine;

   write_load_factors(pChapter,pDisplayUnits,"Design - Inventory",pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrDesign_Inventory));
   write_load_factors(pChapter,pDisplayUnits,"Design - Operating",pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrDesign_Operating));
   write_load_factors(pChapter,pDisplayUnits,"Legal - Routine",pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrLegal_Routine));
   write_load_factors(pChapter,pDisplayUnits,"Legal - Special",pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrLegal_Special));
   write_load_factors(pChapter,pDisplayUnits,"Permit - Routine",pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrPermit_Routine));
   write_load_factors(pChapter,pDisplayUnits,"Permit - Special - Single Trip, escorted",pRatingEntry->GetLiveLoadFactorModel(pgsTypes::ptSingleTripWithEscort));
   write_load_factors(pChapter,pDisplayUnits,"Permit - Special - Single Trip, mixed with traffic",pRatingEntry->GetLiveLoadFactorModel(pgsTypes::ptSingleTripWithTraffic));
   write_load_factors(pChapter,pDisplayUnits,"Permit - Special - Multiple Trip, mixed with traffic",pRatingEntry->GetLiveLoadFactorModel(pgsTypes::ptMultipleTripWithTraffic));
}

void write_load_factors(rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const char* lpszName,const CLiveLoadFactorModel& model)
{
   INIT_UV_PROTOTYPE( rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(),    true );

   rptRcScalar scalar;
   scalar.SetFormat(sysNumericFormatTool::Fixed);
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
   scalar.SetTolerance(1.0e-6);

   std::string strModel;
   pgsTypes::LiveLoadFactorType llfType = model.GetLiveLoadFactorType();
   switch( llfType )
   {
   case pgsTypes::gllSingleValue:
      strModel = "Single Value";
      break;

   case pgsTypes::gllStepped:
      strModel = "Stepped";
      break;

   case pgsTypes::gllLinear:
      strModel = "Linear";
      break;

   case pgsTypes::gllBilinear:
      strModel = "Bilinear";
      break;

   case pgsTypes::gllBilinearWithWeight:
      strModel = "Bilinear with Vehicle Weight";
      break;

   }

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   *pChapter << pPara;
   *pPara << lpszName << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << "Live Load Factor Model: " << strModel << rptNewLine;

   Int16 adtt1, adtt2, adtt3, adtt4;
   model.GetADTT(&adtt1,&adtt2,&adtt3,&adtt4);

   Float64 g1,g2,g3,g4;
   model.GetLowerLiveLoadFactor(&g1,&g2,&g3,&g4);

   if ( llfType == pgsTypes::gllSingleValue )
   {
      *pPara << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(g1) << rptNewLine;
   }
   else if ( llfType == pgsTypes::gllStepped )
   {
      *pPara << "ADTT < " << adtt1 << " " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(g1) << rptNewLine;
      *pPara << "Otherwise " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(g2) << rptNewLine;
      *pPara << "ADTT = Unknown " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(g4) << rptNewLine;
   }
   else if ( llfType == pgsTypes::gllLinear )
   {
      *pPara << "ADTT < " << adtt1 << " " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(g1) << rptNewLine;
      *pPara << "ADTT > " << adtt2 << " " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(g2) << rptNewLine;
      *pPara << "ADTT = Unknown " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(g4) << rptNewLine;

      *pPara << rptNewLine;
      if ( model.GetLiveLoadFactorModifier() == pgsTypes::gllmRoundUp )
      {
         *pPara << "Load factors are rounded up" << rptNewLine;
      }
      else
      {
         *pPara << "Load factors are linearly interpolated" << rptNewLine;
      }
   }
   else if ( llfType == pgsTypes::gllBilinear )
   {
      *pPara << "ADTT < " << adtt1 << " " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(g1) << rptNewLine;
      *pPara << "ADTT = " << adtt2 << " " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(g2) << rptNewLine;
      *pPara << "ADTT > " << adtt3 << " " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(g3) << rptNewLine;
      *pPara << "ADTT = Unknown " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(g4) << rptNewLine;

      *pPara << rptNewLine;
      if ( model.GetLiveLoadFactorModifier() == pgsTypes::gllmRoundUp )
      {
         *pPara << "Load factors are rounded up" << rptNewLine;
      }
      else
      {
         *pPara << "Load factors are linearly interpolated" << rptNewLine;
      }
   }
   else if ( llfType == pgsTypes::gllBilinearWithWeight )
   {
      Float64 Wlower, Wupper;
      model.GetVehicleWeight(&Wlower,&Wupper);
      *pPara << "For vehicle weight up to " << force.SetValue(Wlower) << rptNewLine;
      *pPara << "ADTT < " << adtt1 << " " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(g1) << rptNewLine;
      *pPara << "ADTT = " << adtt2 << " " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(g2) << rptNewLine;
      *pPara << "ADTT > " << adtt3 << " " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(g3) << rptNewLine;
      *pPara << "ADTT = Unknown " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(g4) << rptNewLine;

      *pPara << rptNewLine;

      Float64 ga,gb,gc,gd;
      model.GetUpperLiveLoadFactor(&ga,&gb,&gc,&gd);
      *pPara << "For vehicle weight of " << force.SetValue(Wupper) << " or more" << rptNewLine;
      *pPara << "ADTT < " << adtt1 << " " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(ga) << rptNewLine;
      *pPara << "ADTT = " << adtt2 << " " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(gb) << rptNewLine;
      *pPara << "ADTT > " << adtt3 << " " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(gc) << rptNewLine;
      *pPara << "ADTT = Unknown " << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(gd) << rptNewLine;

      *pPara << rptNewLine;
      if ( model.GetLiveLoadFactorModifier() == pgsTypes::gllmRoundUp )
      {
         *pPara << "Load factors are rounded up" << rptNewLine;
      }
      else
      {
         *pPara << "Load factors are linearly interpolated" << rptNewLine;
      }
   }
   else
   {
      ATLASSERT(false); // is there a new model???
   }
}
