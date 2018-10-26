///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#include <Reporting\OptimizedFabricationChapterBuilder.h>

#include <IFace\DisplayUnits.h>
#include <IFace\Constructability.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\GirderHandlingSpecCriteria.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   COptimizedFabricationChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
COptimizedFabricationChapterBuilder::COptimizedFabricationChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR COptimizedFabricationChapterBuilder::GetName() const
{
   return TEXT("Fabrication Options");
}

rptChapter* COptimizedFabricationChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType gdr = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IDisplayUnits,pDispUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   if ( pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Permanent) == 0 )
   {
      *pPara << "The girder must have strands to perform a fabrication optimization analysis" << rptNewLine;
      return pChapter;
   }

   // don't do report if shipping or lifting are disabled
   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   if (!pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
   {
      *pPara <<color(Red)<<"Lifting analysis disabled in Project Criteria library entry. Fabrication analysis not performed."<<color(Black)<<rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if (!pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
   {
      *pPara <<color(Red)<<"Hauling analysis disabled in Project Criteria library entry. Fabrication analysis not performed."<<color(Black)<<rptNewLine;
      return pChapter;
   }


   bool bUSUnits = (pDispUnits->GetUnitDisplayMode() == pgsTypes::umUS ? true : false);

   INIT_UV_PROTOTYPE( rptForceUnitValue, force, pDispUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDispUnits->GetAlignmentLengthUnit() , true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDispUnits->GetStressUnit() , true );

   GET_IFACE2(pBroker,IFabricationOptimization,pFabOp);

   FABRICATIONOPTIMIZATIONDETAILS details;
   pFabOp->GetFabricationOptimizationDetails(span,gdr,&details);


   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << "Release Requirements" << rptNewLine;
   pPara = new rptParagraph;
   *pChapter << pPara;


   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   *pChapter << pPara;
   *pPara << "Form Stripping Strength" << rptNewLine;
   pPara = new rptParagraph;
   *pChapter << pPara;

   double fci_form_stripping_without_tts = (bUSUnits ? CeilOff(details.Fci_FormStripping_WithoutTTS, ::ConvertToSysUnits(100,unitMeasure::PSI)) 
                                                     : CeilOff(details.Fci_FormStripping_WithoutTTS, ::ConvertToSysUnits(6,unitMeasure::MPa)) );
   
   if ( 0 <  pStrandGeom->GetMaxStrands(span,gdr,pgsTypes::Temporary) )
   {
      if ( details.Fci_FormStripping_WithoutTTS < 0 )
      {
         *pPara << "There is no release strength that will work for form stripping without temporary strands." << rptNewLine;
      }
      else
      {
         *pPara << "Minimum concrete strength for girder sitting in form without temporary strands: " << RPT_FCI << " = " << stress.SetValue(details.Fci_FormStripping_WithoutTTS);
         *pPara << " " << symbol(RIGHT_DOUBLE_ARROW) << " " << stress.SetValue(fci_form_stripping_without_tts) << rptNewLine;
      }
   }
   else
   {
      if ( details.Fci_FormStripping_WithoutTTS < 0 )
      {
         *pPara << "There is no release strength that will work for form stripping." << rptNewLine;
      }
      else
      {
         *pPara << "Minimum concrete strength for girder sitting in form: " << RPT_FCI << " = " << stress.SetValue(details.Fci_FormStripping_WithoutTTS);
         *pPara << " " << symbol(RIGHT_DOUBLE_ARROW) << " " << stress.SetValue(fci_form_stripping_without_tts) << rptNewLine;
      }
   }

   *pPara << color(Red) << bold(ON) << "The forms and curing system should not be removed at this strength unless there is a high degree of confidence that the lifting and final strength targets can be attained." << bold(OFF) << color(Black) << rptNewLine;

   
   if ( 0 < pStrandGeom->GetMaxStrands(span,gdr,pgsTypes::Temporary) )
   {

      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara << "Lifting Requirements" << rptNewLine;
      pPara = new rptParagraph;
      *pChapter << pPara;

      if ( details.Nt == 0 )
      {
         *pPara << "Number of Temporary Strands = " << details.Nt << rptNewLine;
      }
      else
      {
         *pPara << "Number of Temporary Strands = " << details.Nt << rptNewLine;
         *pPara << "Jacking Force, " << Sub2("P","jack") << " = " << force.SetValue(details.Pjack) << rptNewLine;

         double fci[4];
         fci[NO_TTS]          = (bUSUnits ? CeilOff(details.Fci[NO_TTS],          ::ConvertToSysUnits(100,unitMeasure::PSI)) 
                                          : CeilOff(details.Fci[NO_TTS],          ::ConvertToSysUnits(6,  unitMeasure::MPa)));
         fci[PS_TTS]          = (bUSUnits ? CeilOff(details.Fci[PS_TTS],          ::ConvertToSysUnits(100,unitMeasure::PSI)) 
                                          : CeilOff(details.Fci[PS_TTS],          ::ConvertToSysUnits(6,  unitMeasure::MPa)));
         fci[PT_TTS_REQUIRED] = (bUSUnits ? CeilOff(details.Fci[PT_TTS_REQUIRED], ::ConvertToSysUnits(100,unitMeasure::PSI)) 
                                          : CeilOff(details.Fci[PT_TTS_REQUIRED], ::ConvertToSysUnits(6,  unitMeasure::MPa)));
         fci[PT_TTS_OPTIONAL] = (bUSUnits ? CeilOff(details.Fci[PT_TTS_OPTIONAL], ::ConvertToSysUnits(100,unitMeasure::PSI)) 
                                          : CeilOff(details.Fci[PT_TTS_OPTIONAL], ::ConvertToSysUnits(6,  unitMeasure::MPa)));
         

         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << "Lifting without Temporary Strands" << rptNewLine;
         pPara = new rptParagraph;
         *pChapter << pPara;

         *pPara << "Lifting Location, L = " << length.SetValue(details.L[NO_TTS]) << rptNewLine;
         if ( details.Fci[NO_TTS] < 0 )
         {
            *pPara << "There is no release strength that will work for lifting without temporary strands." << rptNewLine;
         }
         else
         {
            *pPara << "Lifting Strength = " << stress.SetValue(details.Fci[NO_TTS]);
            *pPara << " " << symbol(RIGHT_DOUBLE_ARROW) << " " << stress.SetValue(fci[NO_TTS]) << rptNewLine;
         }

         if ( 0 < details.Nt )
         {
            pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
            *pChapter << pPara;
            *pPara << "Lifting with Pretensioned Temporary Strands" << rptNewLine;
            pPara = new rptParagraph;
            *pChapter << pPara;
            *pPara << "Lifting Location, L = " << length.SetValue(details.L[PS_TTS]) << rptNewLine;

            if ( details.Fci[PS_TTS] < 0 )
            {
               *pPara << "There is no release strength that will work for lifting with Pretensioned Temporary Strands" << rptNewLine;
            }
            else
            {
               *pPara << "Lifting Strength = " << stress.SetValue(details.Fci[PS_TTS]);
               *pPara << " " << symbol(RIGHT_DOUBLE_ARROW) << " " << stress.SetValue(fci[PS_TTS]) << rptNewLine;
            }


            
            pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
  
            *pChapter << pPara;
            *pPara << "Lifting with Post-Tensioned Temporary Strands" << rptNewLine;
            pPara = new rptParagraph;
            *pChapter << pPara;
            *pPara << "Lifting Location, L = " << length.SetValue(details.L[PT_TTS_REQUIRED]) << rptNewLine;

            if ( details.Fci[PT_TTS_REQUIRED] < 0 )
            {
               *pPara << "There is no release strength that will work for lifting with Post-Tensioned Temporary Strands" << rptNewLine;
            }
            else
            {
               *pPara << "Lifting Strength = " << stress.SetValue(details.Fci[PT_TTS_REQUIRED]);
               *pPara << " " << symbol(RIGHT_DOUBLE_ARROW) << " " << stress.SetValue(fci[PT_TTS_REQUIRED]) << rptNewLine;
            }

            
            
            pPara = new rptParagraph;
            *pChapter << pPara;
            *pPara << "Lifting Location, L = " << length.SetValue(details.L[PT_TTS_OPTIONAL]) << rptNewLine;

            if ( details.Fci[PT_TTS_OPTIONAL] < 0 )
            {
               *pPara << "There is no release strength that will work for lifting with Post-Tensioned Temporary Strands" << rptNewLine;
            }
            else
            {
               *pPara << "Lifting Strength = " << stress.SetValue(details.Fci[PT_TTS_OPTIONAL]);
               *pPara << " " << symbol(RIGHT_DOUBLE_ARROW) << " " << stress.SetValue(fci[PT_TTS_OPTIONAL]) << rptNewLine;
            }

            *pPara << rptNewLine;
            *pPara << Bold("NOTE:") << " Post-tensioned temporary strands must be installed within 24 hours of prestress transfer." << rptNewLine;
         }
      }
   }

   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << "Shipping Requirements" << rptNewLine;
   pPara = new rptParagraph;
   *pChapter << pPara;

   if ( details.bTempStrandsRequiredForShipping )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      *pChapter << pPara;
      *pPara << "Shipping" << rptNewLine;
      pPara = new rptParagraph;
      *pChapter << pPara;
      if ( 0 < details.Nt )
         *pPara << "Additional temporary strands are required for shipping" << rptNewLine;
      else
         *pPara << "Temporary strands are required for shipping" << rptNewLine;
   }
   else
   {
//      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
//      *pChapter << pPara;
//      *pPara << "Shipping Strength" << rptNewLine;
//      pPara = new rptParagraph;
//      *pChapter << pPara;
//      double fc = (bUSUnits ? CeilOff(details.Fc, ::ConvertToSysUnits(100,unitMeasure::PSI)) 
//                            : CeilOff(details.Fc, ::ConvertToSysUnits(6,  unitMeasure::MPa)));
//      *pPara << RPT_FC << " = " << stress.SetValue(details.Fc);
//       *pPara << " " << symbol(RIGHT_DOUBLE_ARROW) << " " << stress.SetValue(fc) << rptNewLine;

      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      *pChapter << pPara;
      *pPara << "Shipping with Equal Overhangs" << rptNewLine;
      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << Sub2("L","min") << " = " << length.SetValue(details.Lmin) << rptNewLine;
      *pPara << Sub2("L","max") << " = " << length.SetValue(details.Lmax) << rptNewLine;

      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      *pChapter << pPara;
      *pPara << "Shipping with Unequal Overhangs" << rptNewLine;
      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << Sub2("L","min") << " = " << length.SetValue(details.LUmin) << rptNewLine;
      *pPara << Sub2("L","max") << " = " << length.SetValue(details.LUmax) << rptNewLine;
      *pPara << "Sum of cantilever length = " << length.SetValue(details.LUsum) << rptNewLine;
   }

   return pChapter;
}

CChapterBuilder* COptimizedFabricationChapterBuilder::Clone() const
{
   return new COptimizedFabricationChapterBuilder;
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
