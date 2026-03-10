///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <Reporting\OptimizedFabricationChapterBuilder.h>

#include <IFace/Tools.h>
#include <EAF/EAFDisplayUnits.h>
#include <IFace\Constructability.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\GirderHandlingSpecCriteria.h>

#include <PsgLib\SplicedGirderData.h>


COptimizedFabricationChapterBuilder::COptimizedFabricationChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR COptimizedFabricationChapterBuilder::GetName() const
{
   return TEXT("Fabrication Options");
}

rptChapter* COptimizedFabricationChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pBroker = pGirderRptSpec->GetBroker();
   const CGirderKey& girderKey( pGirderRptSpec->GetGirderKey());

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara <<color(Red)<<_T("Fabrication optimization analysis not performed for time step loss method.")<<color(Black)<<rptNewLine;
      return pChapter;
   }


   // don't do report if shipping or lifting are disabled
   GET_IFACE2(pBroker,ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   if (!pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled())
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara <<color(Red)<<_T("Lifting analysis disabled in Project Criteria. Fabrication optimization analysis not performed.")<<color(Black)<<rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker,ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
   if (!pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara <<color(Red)<<_T("Hauling analysis disabled in Project Criteria. Fabrication optimization analysis not performed.")<<color(Black)<<rptNewLine;
      return pChapter;
   }

   if (pSegmentHaulingSpecCriteria->GetHaulingAnalysisMethod() != pgsTypes::HaulingAnalysisMethod::WSDOT)
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara <<color(Red)<<_T("Fabrication analysis not performed. Analysis can only be performed for WSDOT hauling analysis method.")<<color(Black)<<rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker, IBridgeDescription, pBridgeDesc);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   bool bUSUnits = IS_US_UNITS(pDisplayUnits);

   INIT_UV_PROTOTYPE( rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetAlignmentLengthUnit() , true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit() , true );

   GET_IFACE2_NOCHECK(pBroker,IFabricationOptimization,pFabOp);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      rptParagraph* pPara;

      if ( 1 < nSegments )
      {
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }

      CSegmentKey segmentKey(girderKey,segIdx);

      pPara = new rptParagraph;
      *pChapter << pPara;

      if ( pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Permanent) == 0 )
      {
         *pPara << _T("The girder must have strands to perform a fabrication optimization analysis") << rptNewLine;
         continue;
      }

      if (pBridgeDesc->GetPrecastSegmentData(segmentKey)->Strands.GetStrandDefinitionType() == pgsTypes::sdtDirectStrandInput)
      {
         *pPara << _T("Fabrication optimization analysis cannot be performed when strands are defined with the Individual Strand method.") << rptNewLine;
         continue;
      }

      FABRICATIONOPTIMIZATIONDETAILS details;
      pFabOp->GetFabricationOptimizationDetails(segmentKey,&details);


      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;
      *pPara << _T("Release Requirements") << rptNewLine;
      pPara = new rptParagraph;
      *pChapter << pPara;


      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      *pChapter << pPara;
      *pPara << _T("Form Stripping Strength") << rptNewLine;
      pPara = new rptParagraph;
      *pChapter << pPara;

      Float64 fci_form_stripping_without_tts = (bUSUnits ? CeilOffTol(details.Fci_FormStripping_WithoutTTS, WBFL::Units::ConvertToSysUnits(100,WBFL::Units::Measure::PSI)) 
                                                        : CeilOffTol(details.Fci_FormStripping_WithoutTTS, WBFL::Units::ConvertToSysUnits(6,WBFL::Units::Measure::MPa)) );
      
      if ( 0 <  pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary) )
      {
         if ( details.Fci_FormStripping_WithoutTTS < 0 )
         {
            *pPara << _T("There is no release strength that will work for form stripping without temporary strands.") << rptNewLine;
         }
         else
         {
            *pPara << _T("Minimum concrete strength for girder sitting in form without temporary strands: ") << RPT_FCI << _T(" = ") << stress.SetValue(details.Fci_FormStripping_WithoutTTS);
            *pPara << _T(" ") << symbol(RIGHT_DOUBLE_ARROW) << _T(" ") << stress.SetValue(fci_form_stripping_without_tts) << rptNewLine;
         }
      }
      else
      {
         if ( details.Fci_FormStripping_WithoutTTS < 0 )
         {
            *pPara << _T("There is no release strength that will work for form stripping.") << rptNewLine;
         }
         else
         {
            *pPara << _T("Minimum concrete strength for girder sitting in form: ") << RPT_FCI << _T(" = ") << stress.SetValue(details.Fci_FormStripping_WithoutTTS);
            *pPara << _T(" ") << symbol(RIGHT_DOUBLE_ARROW) << _T(" ") << stress.SetValue(fci_form_stripping_without_tts) << rptNewLine;
         }
      }

      *pPara << color(Red) << bold(ON) << _T("The forms and curing system should not be removed at this strength unless there is a high degree of confidence that the lifting and final strength targets can be attained.") << bold(OFF) << color(Black) << rptNewLine;

      
      if ( 0 < pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary) )
      {

         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;
         *pPara << _T("Lifting Requirements") << rptNewLine;
         pPara = new rptParagraph;
         *pChapter << pPara;

         if ( details.Nt == 0 )
         {
            *pPara << _T("Number of Temporary Strands = ") << details.Nt << rptNewLine;
         }
         else
         {
            *pPara << _T("Number of Temporary Strands = ") << details.Nt << rptNewLine;
            *pPara << _T("Jacking Force, ") << Sub2(_T("P"),_T("jack")) << _T(" = ") << force.SetValue(details.Pjack) << rptNewLine;

            Float64 fci[4];
            fci[NO_TTS]          = (bUSUnits ? CeilOffTol(details.Fci[NO_TTS],          WBFL::Units::ConvertToSysUnits(100,WBFL::Units::Measure::PSI)) 
                                             : CeilOffTol(details.Fci[NO_TTS],          WBFL::Units::ConvertToSysUnits(6,  WBFL::Units::Measure::MPa)));
            fci[PS_TTS]          = (bUSUnits ? CeilOffTol(details.Fci[PS_TTS],          WBFL::Units::ConvertToSysUnits(100,WBFL::Units::Measure::PSI)) 
                                             : CeilOffTol(details.Fci[PS_TTS],          WBFL::Units::ConvertToSysUnits(6,  WBFL::Units::Measure::MPa)));
            fci[PT_TTS_REQUIRED] = (bUSUnits ? CeilOffTol(details.Fci[PT_TTS_REQUIRED], WBFL::Units::ConvertToSysUnits(100,WBFL::Units::Measure::PSI)) 
                                             : CeilOffTol(details.Fci[PT_TTS_REQUIRED], WBFL::Units::ConvertToSysUnits(6,  WBFL::Units::Measure::MPa)));
            fci[PT_TTS_OPTIONAL] = (bUSUnits ? CeilOffTol(details.Fci[PT_TTS_OPTIONAL], WBFL::Units::ConvertToSysUnits(100,WBFL::Units::Measure::PSI)) 
                                             : CeilOffTol(details.Fci[PT_TTS_OPTIONAL], WBFL::Units::ConvertToSysUnits(6,  WBFL::Units::Measure::MPa)));
            

            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            *pChapter << pPara;
            *pPara << _T("Lifting without Temporary Strands") << rptNewLine;
            pPara = new rptParagraph;
            *pChapter << pPara;

            *pPara << _T("Lifting Location, L = ") << length.SetValue(details.L[NO_TTS]) << rptNewLine;
            if ( details.Fci[NO_TTS] < 0 )
            {
               *pPara << _T("There is no release strength that will work for lifting without temporary strands.") << rptNewLine;
            }
            else
            {
               *pPara << _T("Lifting Strength = ") << stress.SetValue(details.Fci[NO_TTS]);
               *pPara << _T(" ") << symbol(RIGHT_DOUBLE_ARROW) << _T(" ") << stress.SetValue(fci[NO_TTS]) << rptNewLine;
            }

            if ( 0 < details.Nt )
            {
               pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
               *pChapter << pPara;
               *pPara << _T("Lifting with Pretensioned Temporary Strands") << rptNewLine;
               pPara = new rptParagraph;
               *pChapter << pPara;
               *pPara << _T("Lifting Location, L = ") << length.SetValue(details.L[PS_TTS]) << rptNewLine;

               if ( details.Fci[PS_TTS] < 0 )
               {
                  *pPara << _T("There is no release strength that will work for lifting with Pretensioned Temporary Strands") << rptNewLine;
               }
               else
               {
                  *pPara << _T("Lifting Strength = ") << stress.SetValue(details.Fci[PS_TTS]);
                  *pPara << _T(" ") << symbol(RIGHT_DOUBLE_ARROW) << _T(" ") << stress.SetValue(fci[PS_TTS]) << rptNewLine;
               }


               
               pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
     
               *pChapter << pPara;
               *pPara << _T("Lifting with Post-Tensioned Temporary Strands") << rptNewLine;
               pPara = new rptParagraph;
               *pChapter << pPara;
               *pPara << _T("Lifting Location, L = ") << length.SetValue(details.L[PT_TTS_REQUIRED]) << rptNewLine;

               if ( details.Fci[PT_TTS_REQUIRED] < 0 )
               {
                  *pPara << _T("There is no release strength that will work for lifting with Post-Tensioned Temporary Strands") << rptNewLine;
               }
               else
               {
                  *pPara << _T("Lifting Strength = ") << stress.SetValue(details.Fci[PT_TTS_REQUIRED]);
                  *pPara << _T(" ") << symbol(RIGHT_DOUBLE_ARROW) << _T(" ") << stress.SetValue(fci[PT_TTS_REQUIRED]) << rptNewLine;
               }

               
               
               pPara = new rptParagraph;
               *pChapter << pPara;
               *pPara << _T("Lifting Location, L = ") << length.SetValue(details.L[PT_TTS_OPTIONAL]) << rptNewLine;

               if ( details.Fci[PT_TTS_OPTIONAL] < 0 )
               {
                  *pPara << _T("There is no release strength that will work for lifting with Post-Tensioned Temporary Strands") << rptNewLine;
               }
               else
               {
                  *pPara << _T("Lifting Strength = ") << stress.SetValue(details.Fci[PT_TTS_OPTIONAL]);
                  *pPara << _T(" ") << symbol(RIGHT_DOUBLE_ARROW) << _T(" ") << stress.SetValue(fci[PT_TTS_OPTIONAL]) << rptNewLine;
               }

               *pPara << rptNewLine;
               *pPara << Bold(_T("NOTE:")) << _T(" Post-tensioned temporary strands must be installed within 24 hours of prestress transfer.") << rptNewLine;
            }
         }
      }

      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;
      *pPara << _T("Shipping Requirements") << rptNewLine;
      pPara = new rptParagraph;
      *pChapter << pPara;

      if ( details.bTempStrandsRequiredForShipping )
      {
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << _T("Shipping") << rptNewLine;
         pPara = new rptParagraph;
         *pChapter << pPara;
         if ( 0 < details.Nt )
            *pPara << _T("Additional temporary strands are required for shipping") << rptNewLine;
         else
            *pPara << _T("Temporary strands are required for shipping") << rptNewLine;
      }
      else
      {
   //      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   //      *pChapter << pPara;
   //      *pPara << _T("Shipping Strength") << rptNewLine;
   //      pPara = new rptParagraph;
   //      *pChapter << pPara;
   //      Float64 fc = (bUSUnits ? CeilOff(details.Fc, WBFL::Units::ConvertToSysUnits(100,WBFL::Units::Measure::PSI)) 
   //                            : CeilOff(details.Fc, WBFL::Units::ConvertToSysUnits(6,  WBFL::Units::Measure::MPa)));
   //      *pPara << RPT_FC << _T(" = ") << stress.SetValue(details.Fc);
   //       *pPara << _T(" ") << symbol(RIGHT_DOUBLE_ARROW) << _T(" ") << stress.SetValue(fc) << rptNewLine;

         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << _T("Shipping with Equal Overhangs") << rptNewLine;
         pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara << Sub2(_T("L"),_T("min")) << _T(" = ") << length.SetValue(details.Lmin) << rptNewLine;
         *pPara << Sub2(_T("L"),_T("max")) << _T(" = ") << length.SetValue(details.Lmax) << rptNewLine;

         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << _T("Shipping with Unequal Overhangs") << rptNewLine;
         pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara << Sub2(_T("L"),_T("min")) << _T(" = ") << length.SetValue(details.LUmin) << rptNewLine;
         *pPara << Sub2(_T("L"),_T("max")) << _T(" = ") << length.SetValue(details.LUmax) << rptNewLine;
         *pPara << _T("Sum of cantilever length = ") << length.SetValue(details.LUsum) << rptNewLine;
      }
   } // next segment

   return pChapter;
}
