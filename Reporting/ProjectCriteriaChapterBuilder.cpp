///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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


#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Allowables.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Intervals.h>
#include <IFace\PrestressForce.h>

#include <Lrfd\VersionMgr.h>

#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\PrecastSegmentData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void write_load_modifiers(rptParagraph* pPara,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits);
void write_environmental_conditions(rptParagraph* pPara,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits);
void write_structural_analysis(rptParagraph* pPara,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits);

void write_casting_yard(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey);
void write_lifting(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey);
void write_wsdot_hauling(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey);
void write_kdot_hauling(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey);
void write_temp_strand_removal(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey);
void write_bridge_site1(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey);
void write_bridge_site2(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey);
void write_bridge_site3(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey);
void write_moment_capacity(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey);
void write_shear_capacity(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey);
void write_creep(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry);
void write_losses(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry);
void write_strand_stress(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry);
void write_deflections(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry);
void write_rating_criteria(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const RatingLibraryEntry* pRatingEntry);
void write_load_factors(rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,LPCTSTR lpszName,const CLiveLoadFactorModel& model);

// this information needs to be reported
#pragma Reminder("UPDATE: need to include time-dependent material model, section property method, and time-step loss method")

CProjectCriteriaChapterBuilder::CProjectCriteriaChapterBuilder(bool bRating,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bRating = bRating;
}

LPCTSTR CProjectCriteriaChapterBuilder::GetName() const
{
   return TEXT("Project Criteria");
}

rptChapter* CProjectCriteriaChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      girderKey = pGdrLineRptSpec->GetGirderKey();
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
      *pPara <<Bold(_T("Based on:  ")) << lrfrVersionMgr::GetCodeString() << _T(", ") << lrfrVersionMgr::GetVersionString();

      // Load rating criteria includes the design criteria... write the name here
      *pPara << _T("Load Rating Criteria includes ") << spec_name << rptNewLine;
   }


   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara <<Bold(_T("Name: "))<< spec_name << rptNewLine;
   *pPara <<Bold(_T("Description: "))<<pSpecEntry->GetDescription()<<rptNewLine;
   *pPara <<Bold(_T("Based on: ")) << lrfdVersionMgr::GetCodeString() << _T(", ") << lrfdVersionMgr::GetVersionString();
   
   lrfdVersionMgr::Units units = pSpecEntry->GetSpecificationUnits();
   if (units==lrfdVersionMgr::SI)
   {
      *pPara<<_T(" - SI Units")<<rptNewLine;
   }
   else
   {
      *pPara<<_T(" - US Units")<<rptNewLine;
   }


   pPara = new rptParagraph;
   *pChapter << pPara;
   if ( pSpecEntry->GetSectionPropertyMode() == pgsTypes::spmGross )
   {
      *pPara << Bold(_T("Section Properties: ")) << _T("Gross") << rptNewLine;
   }
   else
   {
      *pPara << Bold(_T("Section Properties: ")) << _T("Transformed") << rptNewLine;
   }

   pPara = new rptParagraph;
   *pChapter << pPara;
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmLRFD )
   {
      *pPara << Bold(_T("Effective Flange Width computed in accordance with LRFD 4.6.2.6")) << rptNewLine;
   }
   else
   {
      *pPara << Bold(_T("Effective Flange Width computed using tributary width")) << rptNewLine;
   }


   pPara = new rptParagraph;
   *pChapter << pPara;
   rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable(3);
   *pPara << pLayoutTable << rptNewLine;

   write_load_modifiers(          &(*pLayoutTable)(0,0), pBroker, pDisplayUnits);
   write_environmental_conditions(&(*pLayoutTable)(0,1), pBroker, pDisplayUnits);
   write_structural_analysis(     &(*pLayoutTable)(0,2), pBroker, pDisplayUnits);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);
   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);

      SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         if ( 1 < nSegments )
         {
            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            *pChapter << pPara;
            *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
         }

         pPara = new rptParagraph;
         *pChapter << pPara;

         CSegmentKey segmentKey(thisGirderKey,segIdx);
         if ( !bRating )
         {
            write_casting_yard(pChapter, pBroker, pDisplayUnits, pSpecEntry, segmentKey);
            
            GET_IFACE2(pBroker,ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
            if (pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled())
            {
               write_lifting(pChapter, pBroker, pDisplayUnits, pSpecEntry, segmentKey);
            }

            GET_IFACE2(pBroker,ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
            if (pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled())
            {
               if( pSegmentHaulingSpecCriteria->GetHaulingAnalysisMethod() == pgsTypes::hmWSDOT )
               {
                  write_wsdot_hauling(pChapter, pBroker, pDisplayUnits, pSpecEntry, segmentKey);
               }
               else
               {
                  write_kdot_hauling(pChapter, pBroker, pDisplayUnits, pSpecEntry, segmentKey);
               }
            }

            write_temp_strand_removal(pChapter, pBroker, pDisplayUnits, pSpecEntry, segmentKey);
            write_bridge_site1(pChapter, pBroker, pDisplayUnits, pSpecEntry, segmentKey);
            write_bridge_site2(pChapter, pBroker, pDisplayUnits, pSpecEntry, segmentKey);
         }

         write_bridge_site3(pChapter, pBroker, pDisplayUnits, pSpecEntry, segmentKey);
         write_moment_capacity(pChapter, pBroker, pDisplayUnits, pSpecEntry, segmentKey);
         write_shear_capacity(pChapter, pBroker, pDisplayUnits, pSpecEntry, segmentKey);
      } // next segment

      write_creep(pChapter, pBroker, pDisplayUnits, pSpecEntry);
      write_losses(pChapter, pBroker, pDisplayUnits, pSpecEntry);
      write_strand_stress(pChapter, pBroker, pDisplayUnits, pSpecEntry);
      write_deflections(pChapter, pBroker, pDisplayUnits, pSpecEntry);

      if ( bRating )
      {
         write_rating_criteria(pChapter,pBroker,pDisplayUnits,pRatingEntry);
      }
   } // next group

   return pChapter;
}


CChapterBuilder* CProjectCriteriaChapterBuilder::Clone() const
{
   return new CProjectCriteriaChapterBuilder(m_bRating);
}

void write_load_modifiers(rptParagraph* pPara,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits)
{
   rptRcTable* p_table = rptStyleManager::CreateTableNoHeading(2,_T("Load Modifiers"));
   *pPara << p_table;

   GET_IFACE2(pBroker,ILoadModifiers,pLoadModifiers);

   (*p_table)(0,0) << _T("Ductility  - ")<< Sub2(symbol(eta),_T("D"));
   (*p_table)(0,1) <<  pLoadModifiers->GetDuctilityFactor();

   (*p_table)(1,0) << _T("Importance - ")<< Sub2(symbol(eta),_T("I"));
   (*p_table)(1,1) <<  pLoadModifiers->GetImportanceFactor();

   (*p_table)(2,0) << _T("Redundancy - ")<< Sub2(symbol(eta),_T("R"));
   (*p_table)(2,1) <<  pLoadModifiers->GetRedundancyFactor();
}

void write_environmental_conditions(rptParagraph* pPara,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits)
{
   rptRcTable* p_table = rptStyleManager::CreateTableNoHeading(2,_T("Environmental"));
   *pPara << p_table;

   GET_IFACE2(pBroker,IEnvironment,pEnvironment);

   (*p_table)(0,0) << _T("Exposure Condition");
   enumExposureCondition cond = pEnvironment->GetExposureCondition();
   if (cond==expNormal)
   {
      (*p_table)(0,1) << _T("Normal");
   }
   else
   {
      (*p_table)(0,1) << _T("Severe");
   }

   (*p_table)(1,0) << _T("Relative Humidity");
   (*p_table)(1,1) <<  pEnvironment->GetRelHumidity()<<_T("%");
}

void write_structural_analysis(rptParagraph* pPara,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits)
{
   rptRcTable* p_table = rptStyleManager::CreateTableNoHeading(1,_T("Structural Analysis"));
   *pPara << p_table;

   GET_IFACE2( pBroker, ISpecification, pSpec );

   switch( pSpec->GetAnalysisType() )
   {
   case pgsTypes::Simple:
      (*p_table)(0,0) << _T("Simple Span");
      break;

   case pgsTypes::Continuous:
      (*p_table)(0,0) << _T("Simple Spans Made Continuous");
      break;

   case pgsTypes::Envelope:
      (*p_table)(0,0) << _T("Envelope of Simple Span and Simple Spans Made Continuous");
      break;
   }
}

void write_casting_yard(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
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
      *pPara << _T("Max. slope for 0.5\" ") << symbol(phi) << _T(" strands = 1:") << slope05 << rptNewLine;
      *pPara << _T("Max. slope for 0.6\" ") << symbol(phi) << _T(" strands = 1:") << slope06 << rptNewLine;
      *pPara << _T("Max. slope for 0.7\" ") << symbol(phi) << _T(" strands = 1:") << slope07 << rptNewLine;
   }
   else
   {
      *pPara << _T("Max. Strand slope is not checked") << rptNewLine;
   }

   Float64 f;
   pSpecEntry->GetHoldDownForce(&do_check, &do_design, &f);
   if (do_check)
   {
      *pPara << _T("Max. hold down force in casting yard = ") << force.SetValue(f) << rptNewLine;
   }
   else
   {
      *pPara << _T("Max. hold down force in casting yard  is not checked") << rptNewLine;
   }

   int method = pSpecEntry->GetCuringMethod();
   if (method == CURING_NORMAL)
   {
      *pPara << _T("Girder was cured using Normal method") << rptNewLine;
   }
   else if (method == CURING_ACCELERATED)
   {
      *pPara << _T("Girder was cured using Accelerated method") << rptNewLine;
   }
   else
   {
      ATLASSERT(false); // is there a new curing method
   }
   
   *pPara << _T("1 day of steam or radiant heat curing is equal to ") << pSpecEntry->GetCuringMethodTimeAdjustmentFactor() << _T(" days of normal curing") << rptNewLine;
   *pPara << rptNewLine;

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowableConcreteStress);

   pgsPointOfInterest poi(segmentKey,0.0);
   // can use TopGirder or BottomGirder to get allowable stress... 
   // we just need to designate that we want the allowable stress for the girder and
   // not the deck
   Float64 fccy = pAllowableConcreteStress->GetSegmentAllowableCompressionStress(poi, releaseIntervalIdx, pgsTypes::ServiceI);
   Float64 ftcy = pAllowableConcreteStress->GetSegmentAllowableTensionStress(poi, releaseIntervalIdx, pgsTypes::ServiceI, false);
   Float64 ft   = pAllowableConcreteStress->GetSegmentAllowableTensionStress(poi, releaseIntervalIdx, pgsTypes::ServiceI, true /*with bonded rebar*/); 
   *pPara << _T("Allowable Concrete Stresses - Service I (5.8.4.1.1)") << rptNewLine;
   *pPara << _T("- Compressive Stress = ") << stress.SetValue(fccy) << rptNewLine;
   *pPara << _T("- Tensile Stress (w/o mild rebar) = ") << stress.SetValue(ftcy) << rptNewLine;
   *pPara << _T("- Tensile Stress (w/  mild rebar) = ") << stress.SetValue(ft) << rptNewLine;

#pragma Reminder("UPDATE: if this is a spliced girder, report the deck allowables too")

   if (pSpecEntry->IsSplittingCheckEnabled())
   {
      if ( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() )
      {
         *pPara << _T("Splitting zone length: h/") << pSpecEntry->GetSplittingZoneLengthFactor() << rptNewLine;
      }
      else
      {
         *pPara << _T("Bursting zone length: h/") << pSpecEntry->GetSplittingZoneLengthFactor() << rptNewLine;
      }
   }
   else
   {
      if ( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() )
      {
         *pPara << _T("Splitting checks (5.10.10.1) are disabled.") << rptNewLine;
      }
      else
      {
         *pPara << _T("Bursting checks (5.10.10.1) are disabled.") << rptNewLine;
      }
   }

   if (pSpecEntry->IsConfinementCheckEnabled())
   {
      *pPara << _T("Confinement checks (5.10.10.2) are enabled.") << rptNewLine;
   }
   else
   {
      *pPara << _T("Confinement checks (5.10.10.2) are disabled.") << rptNewLine;
   }

}

void write_lifting(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey)
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Lifting Criteria") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), true );
   INIT_UV_PROTOTYPE( rptVelocityUnitValue, speed, pDisplayUnits->GetVelocityUnit(), true );
   INIT_UV_PROTOTYPE( rptPressureUnitValue, pressure, pDisplayUnits->GetWindPressureUnit(), true );
   INIT_SCALAR_PROTOTYPE( rptRcPercentage, percentage, pDisplayUnits->GetPercentageFormat());

   *pPara << _T("Factors of Safety") << rptNewLine;
   *pPara << _T("- Cracking F.S. = ") << pSpecEntry->GetCrackingFOSLifting() << rptNewLine;
   *pPara << _T("- Failure F.S. = ") << pSpecEntry->GetLiftingFailureFOS() << rptNewLine;

   *pPara << _T("Impact Factors") << rptNewLine;
   *pPara << _T("- Upward   = ") << percentage.SetValue(pSpecEntry->GetLiftingUpwardImpactFactor())   << rptNewLine;
   *pPara << _T("- Downward = ") << percentage.SetValue(pSpecEntry->GetLiftingDownwardImpactFactor()) << rptNewLine;

   *pPara << _T("Height of pick point above top of girder = ") << dim.SetValue(pSpecEntry->GetPickPointHeight()) << rptNewLine;
   *pPara << _T("Lifting loop placement tolerance = ") << dim.SetValue(pSpecEntry->GetLiftingLoopTolerance()) << rptNewLine;

   if ( pDisplayUnits->GetUnitMode() == eafTypes::umSI )
   {
      *pPara<<_T("Sweep tolerance = ") << pSpecEntry->GetLiftingMaximumGirderSweepTolerance() << _T("m/m") << rptNewLine;
   }
   else
   {
      INT x = (INT)::RoundOff((1.0/(pSpecEntry->GetLiftingMaximumGirderSweepTolerance()*120.0)),1.0);
      *pPara << _T("Sweel tolerance = ") << x << _T("in/10 ft") << rptNewLine;
   }

   *pPara << _T("Min. angle of inclination of lifting cables = ") << angle.SetValue(pSpecEntry->GetMinCableInclination()) << rptNewLine;

   GET_IFACE2(pBroker,ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   if ( pSegmentLiftingSpecCriteria->GetLiftingCamberMethod() == pgsTypes::cmApproximate )
   {
      *pPara << _T("Increase girder CG for camber by ") << percentage.SetValue(pSegmentLiftingSpecCriteria->GetLiftingIncreaseInCgForCamber()) << rptNewLine;
   }
   else
   {
      *pPara << _T("Computed camber is used when locating girder CG") << rptNewLine;
   }

   *pPara << _T("Wind Loading") << rptNewLine;
   if ( pSegmentLiftingSpecCriteria->GetLiftingWindType() == pgsTypes::Speed )
   {
      *pPara << _T("- Wind Speed = ") << speed.SetValue(pSegmentLiftingSpecCriteria->GetLiftingWindLoad()) << rptNewLine;
   }
   else
   {
      *pPara << _T("- Wind Pressure = ") << pressure.SetValue(pSegmentLiftingSpecCriteria->GetLiftingWindLoad()) << rptNewLine;
   }


   Float64 fr = pSegmentLiftingSpecCriteria->GetLiftingModulusOfRupture(segmentKey);
   *pPara << _T("Modulus of rupture = ") << stress.SetValue(fr) << rptNewLine;
   
   Float64 fccy = pSegmentLiftingSpecCriteria->GetLiftingAllowableCompressiveConcreteStress(segmentKey);
   Float64 ftcy = pSegmentLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStress(segmentKey);
   Float64 ft   = pSegmentLiftingSpecCriteria->GetLiftingWithMildRebarAllowableStress(segmentKey);
   *pPara << _T("Allowable Concrete Stresses - Lifting (5.9.4.1.1)") << rptNewLine;
   if ( pSegmentLiftingSpecCriteria->EvaluateLiftingStressesPlumbGirder() )
   {
      *pPara << _T("Stresses are evaluated for a plumb girder") << rptNewLine;
   }
   else
   {
      *pPara << _T("Stresses are evaluated for a tilted girder at equilibrium") << rptNewLine;
   }
   *pPara << _T("- Compressive Stress = ") << stress.SetValue(fccy) << rptNewLine;
   *pPara << _T("- Tensile Stress (w/o mild rebar) = ") << stress.SetValue(ftcy) << rptNewLine;
   *pPara << _T("- Tensile Stress (w/  mild rebar) = ") << stress.SetValue(ft) << rptNewLine;
}

void write_wsdot_hauling(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey)
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Hauling Criteria - WSDOT Method") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim2, pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force, pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptMomentPerAngleUnitValue, spring, pDisplayUnits->GetMomentPerAngleUnit(),true );
   INIT_UV_PROTOTYPE( rptVelocityUnitValue, speed, pDisplayUnits->GetVelocityUnit(), true );
   INIT_UV_PROTOTYPE( rptPressureUnitValue, pressure, pDisplayUnits->GetWindPressureUnit(), true );
   INIT_SCALAR_PROTOTYPE( rptRcPercentage, percentage, pDisplayUnits->GetPercentageFormat());

   *pPara << _T("Factors of Safety") << rptNewLine;
   *pPara << _T("- Cracking F.S. = ") << pSpecEntry->GetHaulingCrackingFOS() << rptNewLine;
   *pPara << _T("- Failure & Roll over F.S. = ") << pSpecEntry->GetHaulingFailureFOS() << rptNewLine;

   *pPara << _T("Impact Factors") << rptNewLine;
   *pPara << _T("- Upward   = ") << percentage.SetValue(pSpecEntry->GetHaulingUpwardImpactFactor()) << rptNewLine;
   *pPara << _T("- Downward = ") << percentage.SetValue(pSpecEntry->GetHaulingDownwardImpactFactor()) << rptNewLine;

   GET_IFACE2(pBroker,ISegmentHaulingSpecCriteria,pHauling);


   if ( pDisplayUnits->GetUnitMode() == eafTypes::umSI )
   {
      *pPara<<_T("Normal Crown Slope = ") << pHauling->GetNormalCrownSlope() << _T("m/m") << rptNewLine;
      *pPara<<_T("Max. Superelevation = ") << pHauling->GetMaxSuperelevation() << _T("m/m") << rptNewLine;
      *pPara<<_T("Sweep tolerance = ") << pHauling->GetHaulingSweepTolerance() << _T("m/m") << rptNewLine;
   }
   else
   {
      *pPara<<_T("Normal Crown Slope = ") << pHauling->GetNormalCrownSlope() << _T("ft/ft") << rptNewLine;
      *pPara<<_T("Max. Superelevation = ") << pHauling->GetMaxSuperelevation() << _T("ft/ft") << rptNewLine;
      INT x = (INT)::RoundOff((1.0/(pHauling->GetHaulingSweepTolerance()*120.0)),1.0);
      *pPara << _T("Sweel tolerance = ") << x << _T("in/10 ft") << rptNewLine;
   }

   *pPara << _T("Support placement lateral tolerance = ") << dim.SetValue(pHauling->GetHaulingSupportPlacementTolerance()) << rptNewLine;

   if ( pHauling->GetHaulingCamberMethod() == pgsTypes::cmApproximate )
   {
      *pPara << _T("Increase girder CG for camber by ") << percentage.SetValue(pHauling->GetHaulingIncreaseInCgForCamber()) << rptNewLine;
   }
   else
   {
      *pPara << _T("Computed camber is used when locating girder CG") << rptNewLine;
   }

   *pPara << _T("Wind Loading") << rptNewLine;
   if ( pHauling->GetHaulingWindType() == pgsTypes::Speed )
   {
      *pPara << _T("- Wind Speed = ") << speed.SetValue(pHauling->GetHaulingWindLoad()) << rptNewLine;
   }
   else
   {
      *pPara << _T("- Wind Pressure = ") << pressure.SetValue(pHauling->GetHaulingWindLoad()) << rptNewLine;
   }

   *pPara << _T("Centrifugal Force") << rptNewLine;
   if ( pHauling->GetCentrifugalForceType() == pgsTypes::Adverse )
   {
      *pPara << _T("- Force is adverse") << rptNewLine;
   }
   else
   {
      *pPara << _T("- Force is favorable") << rptNewLine;
   }
   *pPara << _T("- Hauling Speed = ") << speed.SetValue(pHauling->GetHaulingSpeed()) << rptNewLine;
   *pPara << _T("- Turning Radius = ") << dim2.SetValue(pHauling->GetTurningRadius()) << rptNewLine;


   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   const HaulTruckLibraryEntry* pHaulTruck = pSegment->HandlingData.pHaulTruckLibraryEntry;
   *pPara << _T("Haul Truck") << rptNewLine;
   *pPara << _T("- Name = ") << pHaulTruck->GetName() << rptNewLine;
   *pPara << _T("- Roll Stiffness, ") << Sub2(_T("K"),symbol(theta)) << _T(" = ") << spring.SetValue(pHaulTruck->GetRollStiffness()) << rptNewLine;

   *pPara << _T("- Height of girder bottom above roadway, ") << Sub2(_T("H"),_T("bg")) << _T(" = ") << dim.SetValue(pHaulTruck->GetBottomOfGirderHeight()) << rptNewLine;
   *pPara << _T("- Height of truck roll center above roadway, ") << Sub2(_T("H"),_T("rc")) << _T(" = ") << dim.SetValue(pHaulTruck->GetRollCenterHeight()) << rptNewLine;
   *pPara << _T("- Center-to-Center distance between truck tires, ") << Sub2(_T("W"),_T("cc")) << _T(" = ") << dim.SetValue(pHaulTruck->GetAxleWidth()) << rptNewLine;
   *pPara << _T("- Max. distance between bunk points, ") << Sub2(_T("L"),_T("s")) << _T(" = ") << dim2.SetValue(pHaulTruck->GetMaxDistanceBetweenBunkPoints()) << rptNewLine;
   *pPara << _T("- Max. leading overhang (nearest truck cab), ") << Sub2(_T("L"),_T("oh")) << _T(" = ") << dim2.SetValue(pHaulTruck->GetMaximumLeadingOverhang()) << rptNewLine;
   *pPara << _T("- Max. girder weight, ") << Sub2(_T("W"),_T("max")) << _T(" = ") << force.SetValue(pHaulTruck->GetMaxGirderWeight()) << rptNewLine;

   Float64 fr = pHauling->GetHaulingModulusOfRupture(segmentKey);
   *pPara << _T("Modulus of rupture = ") << stress.SetValue(fr) << rptNewLine;

   Float64 fccy = pHauling->GetHaulingAllowableCompressiveConcreteStress(segmentKey);
   Float64 ftcy = pHauling->GetHaulingAllowableTensileConcreteStressNormalCrown(segmentKey);
   Float64 ft   = pHauling->GetHaulingWithMildRebarAllowableStressNormalCrown(segmentKey);
   *pPara << _T("Allowable Concrete Stresses - Hauling (5.9.4.2.1) - Normal Crown Slope") << rptNewLine;
   *pPara << _T("- Compressive Stress = ")<<stress.SetValue(fccy) << rptNewLine;
   *pPara << _T("- Tensile Stress (w/o mild rebar) = ") << stress.SetValue(ftcy) << rptNewLine;
   *pPara << _T("- Tensile Stress (w/  mild rebar) = ") << stress.SetValue(ft) << rptNewLine;

   ftcy = pHauling->GetHaulingAllowableTensileConcreteStressMaxSuper(segmentKey);
   ft   = pHauling->GetHaulingWithMildRebarAllowableStressMaxSuper(segmentKey);
   *pPara << _T("Allowable Concrete Stresses - Hauling (5.9.4.2.1) - Maximum Superelevation") << rptNewLine;
   *pPara << _T("- Compressive Stress = ")<<stress.SetValue(fccy) << rptNewLine;
   *pPara << _T("- Tensile Stress (w/o mild rebar) = ") << stress.SetValue(ftcy) << rptNewLine;
   *pPara << _T("- Tensile Stress (w/  mild rebar) = ") << stress.SetValue(ft) << rptNewLine;
}

void write_kdot_hauling(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry, const CSegmentKey& segmentKey)
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Hauling Criteria - KDOT Method")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim2, pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force, pDisplayUnits->GetGeneralForceUnit(), true );

   *pPara<<_T("Dynamic 'G' Factors")<<rptNewLine;
   *pPara<<_T("- In Cantilever Region   = ")<< pSpecEntry->GetOverhangGFactor()<<rptNewLine;
   *pPara<<_T("- Between Bunk Points = ")<< pSpecEntry->GetInteriorGFactor()<<rptNewLine;

   GET_IFACE2(pBroker,IKdotGirderHaulingSpecCriteria,pHauling);
   GET_IFACE2(pBroker,IBridge,pBridge);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (segmentKey.groupIndex == ALL_GROUPS ? 0 : segmentKey.groupIndex);
   GroupIndexType lastGroupIdx  = (segmentKey.groupIndex == ALL_GROUPS ? nGroups-1: firstGroupIdx);
   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType firstGirderIdx = Min(nGirders-2,(segmentKey.girderIndex == ALL_GIRDERS ? 0 : segmentKey.girderIndex));
      GirderIndexType lastGirderIdx  = Min(nGirders-1,(segmentKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx));
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         CSegmentKey thisSegmentKey(grpIdx,gdrIdx,segmentKey.segmentIndex);

         *pPara << _T("Span ") << LABEL_SPAN(grpIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;

         Float64 segment_length = pBridge->GetSegmentLength(thisSegmentKey);

         *pPara<<_T("Minimum support location = ")<<dim2.SetValue(pSpecEntry->GetMininumTruckSupportLocation());
         if (pSpecEntry->GetUseMinTruckSupportLocationFactor())
         {
            Float64 ml = segment_length * pSpecEntry->GetMinTruckSupportLocationFactor();
            *pPara<<_T(", or ")<<dim2.SetValue(ml)<<_T(". Whichever is greater.")<<rptNewLine;
         }
         else
         {
            *pPara<<_T(".")<<rptNewLine;
         }

         *pPara<<_T("Support location design accuracy = ")<<dim2.SetValue(pSpecEntry->GetTruckSupportLocationAccuracy())<<rptNewLine;

         Float64 fccy = pHauling->GetKdotHaulingAllowableCompressiveConcreteStress(thisSegmentKey);
         Float64 ftcy = pHauling->GetKdotHaulingAllowableTensileConcreteStress(thisSegmentKey);
         Float64 ft   = pHauling->GetKdotHaulingWithMildRebarAllowableStress(thisSegmentKey);
         *pPara<<_T("Allowable Concrete Stresses - Hauling (5.9.4.2.1)")<<rptNewLine;
         *pPara<<_T("- Compressive Stress = ")<<stress.SetValue(fccy)<<rptNewLine;
         *pPara<<_T("- Tensile Stress (w/o mild rebar) = ")<<stress.SetValue(ftcy) << rptNewLine;
         *pPara<<_T("- Tensile Stress (w/  mild rebar) = ")<<stress.SetValue(ft) << rptNewLine;
      }
   }
}

void write_temp_strand_removal(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowableConcreteStress);

   if ( !pAllowableConcreteStress->CheckTemporaryStresses() )
   {
      // not checking stresses here so nothing to report
      return;
   }

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   if ( tsRemovalIntervalIdx != INVALID_INDEX )
   {
      rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<<_T("Temporary Strand Removal Criteria")<<rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );

      pgsPointOfInterest poi(segmentKey,0.0);
      Float64 fcsp = pAllowableConcreteStress->GetSegmentAllowableCompressionStress(poi, tsRemovalIntervalIdx,pgsTypes::ServiceI);
      *pPara<<_T("Allowable Compressive Concrete Stresses")<<rptNewLine;
      *pPara<<_T("- Service I = ")<<stress.SetValue(fcsp)<<rptNewLine;

      Float64 fts = pAllowableConcreteStress->GetSegmentAllowableTensionStress(poi, tsRemovalIntervalIdx,pgsTypes::ServiceI, false);
      Float64 ft  = pAllowableConcreteStress->GetSegmentAllowableTensionStress(poi, tsRemovalIntervalIdx,pgsTypes::ServiceI, true);
      *pPara<<_T("Allowable Tensile Concrete Stresses")<<rptNewLine;
      *pPara<<_T("- Service I (w/o mild rebar) = ")<<stress.SetValue(fts) << rptNewLine;
      *pPara<<_T("- Service I (w/  mild rebar) = ")<<stress.SetValue(ft) << rptNewLine;
    }
}

void write_bridge_site1(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowableConcreteStress);

   if ( !pAllowableConcreteStress->CheckTemporaryStresses() )
   {
      // not checking stresses here so nothing to report
      return;
   }

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Deck and Diaphragm Placement Stage (Bridge Site 1) Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );

   pgsPointOfInterest poi(segmentKey,0.0);
   Float64 fcsp = pAllowableConcreteStress->GetSegmentAllowableCompressionStress(poi, castDeckIntervalIdx, pgsTypes::ServiceI);
   *pPara << _T("Allowable Compressive Concrete Stresses") << rptNewLine;
   *pPara << _T("- Service I = ") << stress.SetValue(fcsp) << rptNewLine;

   Float64 fts = pAllowableConcreteStress->GetSegmentAllowableTensionStress(poi, castDeckIntervalIdx, pgsTypes::ServiceI, false);
   *pPara << _T("Allowable Tensile Concrete Stresses") << rptNewLine;
   *pPara << _T("- Service I = ") << stress.SetValue(fts) << rptNewLine;
}

void write_bridge_site2(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey)
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Final without Live Load (Bridge Site 2) Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );

   switch ( pSpecEntry->GetTrafficBarrierDistributionType() )
   {
   case pgsTypes::tbdGirder:
      *pPara << _T("Railing system weight is distributed to ") << pSpecEntry->GetMaxGirdersDistTrafficBarrier() << _T(" nearest girders") << rptNewLine;
      break;

   case pgsTypes::tbdMatingSurface:
      *pPara << _T("Railing system weight is distributed to ") << pSpecEntry->GetMaxGirdersDistTrafficBarrier() << _T(" nearest mating surfaces") << rptNewLine;
      break;

   case pgsTypes::tbdWebs:
      *pPara << _T("Railing system weight is distributed to ") << pSpecEntry->GetMaxGirdersDistTrafficBarrier() << _T(" nearest webs") << rptNewLine;
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowableConcreteStress);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

   pgsPointOfInterest poi(segmentKey,0.0);
   Float64 fcsp = pAllowableConcreteStress->GetSegmentAllowableCompressionStress(poi, compositeDeckIntervalIdx,pgsTypes::ServiceI);
   *pPara<<_T("Allowable Compressive Concrete Stresses (5.9.4.2.1)")<<rptNewLine;
   *pPara<<_T("- Service I = ")<<stress.SetValue(fcsp)<<rptNewLine;

   if ( pAllowableConcreteStress->CheckFinalDeadLoadTensionStress() )
   {
      Float64 fts = pAllowableConcreteStress->GetSegmentAllowableTensionStress(poi, compositeDeckIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);
      *pPara<<_T("Allowable Tensile Concrete Stresses")<<rptNewLine;
      *pPara<<_T("- Service I = ")<<stress.SetValue(fts)<<rptNewLine;
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

void write_bridge_site3(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Final with Live Load Stage (Bridge Site 3) Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptMomentPerAngleUnitValue, spring, pDisplayUnits->GetMomentPerAngleUnit(),true );

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowableConcreteStress);

   pgsPointOfInterest poi(segmentKey,0.0);

   Float64 fcsl = pAllowableConcreteStress->GetSegmentAllowableCompressionStress(poi, liveLoadIntervalIdx,pgsTypes::ServiceI);
   *pPara<<_T("Allowable Compressive Concrete Stresses (5.9.4.2.1)")<<rptNewLine;
   *pPara<<_T("- Service I (permanent + live load) = ")<<stress.SetValue(fcsl)<<rptNewLine;

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      Float64 fcsa = pAllowableConcreteStress->GetSegmentAllowableCompressionStress(poi, liveLoadIntervalIdx,pgsTypes::ServiceIA);
      *pPara<<_T("- Service IA (one-half of permanent + live load) = ")<<stress.SetValue(fcsa)<<rptNewLine;
   }

   Float64 fts = pAllowableConcreteStress->GetSegmentAllowableTensionStress(poi, liveLoadIntervalIdx,pgsTypes::ServiceIII, false,false);
   *pPara<<_T("Allowable Tensile Concrete Stresses (5.9.4.2.2)")<<rptNewLine;
   *pPara<<_T("- Service III = ")<<stress.SetValue(fts)<<rptNewLine;

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      Float64 ftf = pAllowableConcreteStress->GetSegmentAllowableCompressionStress(poi, liveLoadIntervalIdx,pgsTypes::FatigueI);
      *pPara<<_T("Allowable Compressive Concrete Stresses (5.5.3.1)")<<rptNewLine;
      *pPara<<_T("- Fatigue I = ")<<stress.SetValue(ftf)<<rptNewLine;
   }

   Int16 method = pSpecEntry->GetLiveLoadDistributionMethod();
   if (method==LLDF_LRFD)
   {
      *pPara<<_T("LL Distribution factors are calculated in accordance with LRFD 4.6.2.2")<<rptNewLine;
   }
   else if (method==LLDF_WSDOT)
   {
      *pPara<<_T("LL Distribution factors are calculated in accordance with WSDOT Bridge Design Manual")<<rptNewLine;
   }
   else if (method==LLDF_TXDOT)
   {
      *pPara<<_T("LL Distribution factors are calculated in accordance with TxDOT LRFD Bridge Design Manual")<<rptNewLine;
   }
   else
   {
      ATLASSERT(false); // new method?
   }

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   std::_tstring straction = pLiveLoads->GetLLDFSpecialActionText();
   if ( !straction.empty() )
   {
      *pPara << straction << rptNewLine;
   }
}

void write_moment_capacity(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Moment Capacity Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      if (pSpecEntry->GetLRFDOverreinforcedMomentCapacity())
      {
         *pPara << _T("Capacity of over reinforced sections computed in accordance with LRFD C5.7.3.3.1") << rptNewLine;
      }
      else
      {
         *pPara << _T("Capacity of over reinforced sections computed in accordance with WSDOT Bridge Design Manual") << rptNewLine;
      }
   }

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );
   GET_IFACE2(pBroker,IMaterials,pMaterial);
   Float64 fr = pMaterial->GetSegmentFlexureFr(segmentKey,liveLoadIntervalIdx);
   *pPara << _T("Modulus of rupture = ") << stress.SetValue(fr) << rptNewLine;
}

void write_shear_capacity(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry,const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
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
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(),    true );

   GET_IFACE2(pBroker,IMaterials,pMaterials);
   Float64 fr = pMaterials->GetSegmentShearFr(segmentKey,liveLoadIntervalIdx);
   *pPara << _T("Modulus of rupture = ") << stress.SetValue(fr) << rptNewLine;


   bool bAfter1999 = lrfdVersionMgr::SecondEditionWith2000Interims <= pSpecEntry->GetSpecificationType() ? true : false;
   std::_tstring strFcCoefficient(bAfter1999 ? _T("0.125") : _T("0.1"));
   Float64 k1,k2,s1,s2;
   pSpecEntry->GetMaxStirrupSpacing(&k1,&s1,&k2,&s2);
   *pPara << _T("Maximum Spacing of Transverse Reinforcement (LRFD 5.8.2.7)") << rptNewLine;
   if ( bAfter1999 )
   {
      *pPara << _T("Eqn 5.8.2.7-1: If ") << italic(ON) << Sub2(_T("v"),_T("u")) << italic(OFF) << _T(" < ") << strFcCoefficient << RPT_FC << _T(" then ") << Sub2(_T("S"),_T("max")) << _T(" = ") << k1 << Sub2(_T("d"),_T("v")) << symbol(LTE) << dim.SetValue(s1) << rptNewLine;
      *pPara << _T("Eqn 5.8.2.7-2: If ") << italic(ON) << Sub2(_T("v"),_T("u")) << italic(OFF) << _T(" ") << symbol(GTE) << _T(" ") << strFcCoefficient << RPT_FC << _T(" then ") << Sub2(_T("S"),_T("max")) << _T(" = ") << k2 << Sub2(_T("d"),_T("v")) << symbol(LTE) <<  dim.SetValue(s2) << rptNewLine;
   }
   else
   {
      *pPara << _T("Eqn 5.8.2.7-1: If ") << italic(ON) << Sub2(_T("V"),_T("u")) << italic(OFF) << _T(" < ") << strFcCoefficient << RPT_FC << Sub2(_T("b"),_T("v")) << Sub2(_T("d"),_T("v")) << _T(" then ") << Sub2(_T("S"),_T("max")) << _T(" = ") << k1 << Sub2(_T("d"),_T("v")) << symbol(LTE) << dim.SetValue(s1) << rptNewLine;
      *pPara << _T("Eqn 5.8.2.7-2: If ") << italic(ON) << Sub2(_T("V"),_T("u")) << italic(OFF) << _T(" ") << symbol(GTE) << _T(" ") << strFcCoefficient << RPT_FC << Sub2(_T("b"),_T("v")) << Sub2(_T("d"),_T("v")) << _T(" then ") << Sub2(_T("S"),_T("max")) << _T(" = ") << k2 << Sub2(_T("d"),_T("v")) << symbol(LTE) <<  dim.SetValue(s2) << rptNewLine;
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

   switch ( pSpecEntry->GetShearFlowMethod() )
   {
   case sfmLRFD:
      *pPara << _T("Shear stress at girder/deck interface computed using the LRFD simplified method: ") << Sub2(_T("V"),_T("ui")) << _T(" = ") << _T("V/bd") << rptNewLine;
      break;

   case sfmClassical:
      *pPara << _T("Shear stress at girder/deck interface computed using the classical shear flow formula: ") << Sub2(_T("V"),_T("ui")) << _T(" = (") << Sub2(_T("V"),_T("u")) << _T("Q)") << _T("/") << _T("(Ib)") << rptNewLine;
      break;
   }

   *pPara << _T("Maximum spacing of interface shear connectors (LRFD 5.8.4.2): ") << dim.SetValue(pSpecEntry->GetMaxInterfaceShearConnectorSpacing());
   if ( lrfdVersionMgr::SeventhEdition2014 <= lrfdVersionMgr::GetVersion() )
   {
      *pPara << _T(", or the depth of the member.");
   }
   *pPara << rptNewLine;
}

void write_creep(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry)
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Creep Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   Int16 method = pSpecEntry->GetCreepMethod();
   if (method==CREEP_LRFD)
   {
      *pPara<<_T("Creep is calculated in accordance with LRFD (5.4.2.3.2)")<<rptNewLine;
   }
   else if (method==CREEP_WSDOT)
   {
      *pPara<<_T("Creep is calculated in accordance with WSDOT Bridge Design Manual (6.1.2c.2)")<<rptNewLine;
      *pPara<<_T("Creep factor = ")<<pSpecEntry->GetCreepFactor();
   }
   else
   {
      ATLASSERT(false); // new method?
   }

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time, pDisplayUnits->GetWholeDaysUnit(), true );

   *pPara<< _T("# of hours from stressing to prestress transfer : ")<<::ConvertFromSysUnits(pSpecEntry->GetXferTime(),unitMeasure::Hour)<<rptNewLine;
   *pPara<< _T("# of days from prestress transfer until removal of temporary strands / diaphram casting : ")<<::ConvertFromSysUnits(pSpecEntry->GetCreepDuration1Min(),unitMeasure::Day) << _T(" Min");
   *pPara<< _T(", ") << ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration1Max(),unitMeasure::Day) << _T(" Max") <<rptNewLine;
   *pPara<< _T("# of days from prestress transfer until slab-girder continuity is achieved : ")<<::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Min(),unitMeasure::Day) << _T(" Min");
   *pPara<< _T(", ") << ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Max(),unitMeasure::Day) << _T(" Max") << rptNewLine;

   *pPara << rptNewLine << rptNewLine;

   Float64 Cfactor = pSpecEntry->GetCamberVariability();
   *pPara << _T("Variability between upper and lower bound camber : ") << 100*Cfactor << rptNewLine;
}

void write_losses(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry)
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Losses Criteria")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),    true );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time, pDisplayUnits->GetWholeDaysUnit(), true );

   GET_IFACE2(pBroker,ILossParameters,pLossParameters);
   pgsTypes::LossMethod loss_method = pLossParameters->GetLossMethod();

   if ( loss_method == pgsTypes::GENERAL_LUMPSUM )
   {
      *pPara<<_T("Losses calculated per General Lump Sum")<<rptNewLine;
      *pPara<<_T("- Before Prestress Transfer = ")<<stress.SetValue(pLossParameters->GetBeforeXferLosses())<<rptNewLine;
      *pPara<<_T("- After Prestress Transfer = ")<<stress.SetValue(pLossParameters->GetAfterXferLosses())<<rptNewLine;
      *pPara<<_T("- At Lifting Losses = ")<<stress.SetValue(pLossParameters->GetLiftingLosses())<<rptNewLine;
      *pPara<<_T("- At Shipping Losses = ")<<stress.SetValue(pLossParameters->GetShippingLosses()) << _T(", but not to exceed final losses") << rptNewLine;
      *pPara<<_T("- Before Temp. Strand Removal = ")<<stress.SetValue(pLossParameters->GetBeforeTempStrandRemovalLosses())<<rptNewLine;
      *pPara<<_T("- After Temp. Strand Removal = ")<<stress.SetValue(pLossParameters->GetAfterTempStrandRemovalLosses())<<rptNewLine;
      *pPara<<_T("- After Deck Placement = ")<<stress.SetValue(pLossParameters->GetAfterDeckPlacementLosses())<<rptNewLine;
      *pPara<<_T("- After After Superimposed Dead Loads = ")<<stress.SetValue(pLossParameters->GetAfterSIDLLosses())<<rptNewLine;
      *pPara<<_T("- Final Losses = ")<<stress.SetValue(pLossParameters->GetFinalLosses())<<rptNewLine;
   }
   else
   {
      bool bReportElasticGainParameters = false;
      std::_tstring relaxation_method[3] = {
         _T("LRFD Equation 5.9.5.4.2c-1"),
         _T("LRFD Equation C5.9.5.4.2c-1"),
         _T("1.2 ksi per LRFD 5.9.5.4.2c")
      };
      switch( loss_method )
      {
      case pgsTypes::AASHTO_REFINED:
         *pPara<<_T("Losses calculated per Refined Estimate Method in accordance with AASHTO LRFD 5.9.5.4")<<rptNewLine;
         *pPara<<_T("Relaxation Loss Method = ") << relaxation_method[pSpecEntry->GetRelaxationLossMethod()] << rptNewLine;
         bReportElasticGainParameters = (lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() ? true : false);
         break;
      case pgsTypes::WSDOT_REFINED:
         *pPara<<_T("Losses calculated per Refined Estimate Method in accordance with AASHTO LRFD 5.9.5.4 and WSDOT Bridge Design")<<rptNewLine;
         *pPara<<_T("Relaxation Loss Method = ") << relaxation_method[pSpecEntry->GetRelaxationLossMethod()] << rptNewLine;
         bReportElasticGainParameters = (lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() ? true : false);
         break;
      case pgsTypes::TXDOT_REFINED_2004:
         *pPara<<_T("Losses calculated per Refined Estimate Method in accordance with AASHTO LRFD 5.9.5.4 and TxDOT Bridge Design")<<rptNewLine;
         break;
      case pgsTypes::TXDOT_REFINED_2013:
         *pPara<<_T("Losses calculated per Refined Estimate Method in accordance with TxDOT Bridge Research Report 0-6374-2, June, 2013")<<rptNewLine;
         break;
      case pgsTypes::AASHTO_LUMPSUM:
         bReportElasticGainParameters = (lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() ? true : false);
      case pgsTypes::AASHTO_LUMPSUM_2005:
         *pPara<<_T("Losses calculated per Approximate Lump Sum Method in accordnace with AASHTO LRFD 5.9.5.3")<<rptNewLine;
         break;
      case pgsTypes::WSDOT_LUMPSUM:
         *pPara<<_T("Losses calculated per Approximate Lump Sum Method in accordnace with AASHTO LRFD 5.9.5.3 and WSDOT Bridge Design Manual") << rptNewLine;
         break;
      case pgsTypes::TIME_STEP:
         *pPara<<_T("Losses calculated with a time-step method") << rptNewLine;
         if ( pSpecEntry->GetTimeDependentModel() == TDM_AASHTO )
         {
            *pPara<<_T("Time-dependent concrete properties based on AASHTO LRFD") << rptNewLine;
         }
         else if ( pSpecEntry->GetTimeDependentModel() == TDM_ACI209)
         {
            *pPara<<_T("Time-dependent concrete properties based on ACI 209R-92") << rptNewLine;
         }
         else if ( pSpecEntry->GetTimeDependentModel() == TDM_CEBFIP)
         {
            *pPara<<_T("Time-dependent concrete properties based on CEB-FIP Model Code 1990") << rptNewLine;
         }
         else
         {
            ATLASSERT(false);
         }
         break;
      default:
         ATLASSERT(false); // Should never get here
      }

      if ( loss_method != pgsTypes::TXDOT_REFINED_2013 && lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
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

      if ( bReportElasticGainParameters )
      {
         *pPara << _T("Contribution to Elastic Gains") << rptNewLine;
         *pPara << _T("Slab = ") << pSpecEntry->GetSlabElasticGain()*100.0 << _T("%") << rptNewLine;
         *pPara << _T("Haunch = ") << pSpecEntry->GetSlabPadElasticGain()*100.0 << _T("%") << rptNewLine;
         *pPara << _T("Diaphragms = ") << pSpecEntry->GetDiaphragmElasticGain()*100.0 << _T("%") << rptNewLine;
         *pPara << _T("User DC (Before Deck Placement) = ") << pSpecEntry->GetUserLoadBeforeDeckDCElasticGain()*100.0 << _T("%") << rptNewLine;
         *pPara << _T("User DW (Before Deck Placement) = ") << pSpecEntry->GetUserLoadBeforeDeckDWElasticGain()*100.0 << _T("%") << rptNewLine;
         *pPara << _T("Railing System = ") << pSpecEntry->GetRailingSystemElasticGain()*100.0 << _T("%") << rptNewLine;
         *pPara << _T("Overlay = ") << pSpecEntry->GetOverlayElasticGain()*100.0 << _T("%") << rptNewLine;
         *pPara << _T("User DC (After Deck Placement) = ") << pSpecEntry->GetUserLoadAfterDeckDCElasticGain()*100.0 << _T("%") << rptNewLine;
         *pPara << _T("User DW (After Deck Placement) = ") << pSpecEntry->GetUserLoadAfterDeckDWElasticGain()*100.0 << _T("%") << rptNewLine;

         GET_IFACE2(pBroker,ILosses, pLosses);
         if( pLosses->IsDeckShrinkageApplicable() )
         {
            *pPara << _T("Deck Shrinkage = ") << pSpecEntry->GetDeckShrinkageElasticGain()*100.0 << _T("%") << rptNewLine;
         }
      }
   }
}

void write_strand_stress(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry)
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Stress Limits for Prestressing Tendons")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pSpecEntry->CheckStrandStress(CSS_AT_JACKING) )
   {
      *pPara << rptNewLine;
      *pPara << _T("Stress Limit at Jacking") << rptNewLine;
      *pPara << _T("- Stress Relieved Strand = ") << pSpecEntry->GetStrandStressCoefficient(CSS_AT_JACKING,STRESS_REL) << RPT_STRESS(_T("pu")) << rptNewLine;
      *pPara << _T("- Low Relaxation Strand = ") << pSpecEntry->GetStrandStressCoefficient(CSS_AT_JACKING,LOW_RELAX) << RPT_STRESS(_T("pu")) << rptNewLine;
   }

   if ( pSpecEntry->CheckStrandStress(CSS_BEFORE_TRANSFER) )
   {
      *pPara << rptNewLine;
      *pPara << _T("Stress Limit Immediately Prior to Transfer") << rptNewLine;
      *pPara << _T("- Stress Relieved Strand = ") << pSpecEntry->GetStrandStressCoefficient(CSS_BEFORE_TRANSFER,STRESS_REL) << RPT_STRESS(_T("pu")) << rptNewLine;
      *pPara << _T("- Low Relaxation Strand = ") << pSpecEntry->GetStrandStressCoefficient(CSS_BEFORE_TRANSFER,LOW_RELAX) << RPT_STRESS(_T("pu")) << rptNewLine;
   }

   if ( pSpecEntry->CheckStrandStress(CSS_AFTER_TRANSFER) )
   {
      *pPara << rptNewLine;
      *pPara << _T("Stress Limit Immediately After Transfer") << rptNewLine;
      *pPara << _T("- Stress Relieved Strand = ") << pSpecEntry->GetStrandStressCoefficient(CSS_AFTER_TRANSFER,STRESS_REL) << RPT_STRESS(_T("pu")) << rptNewLine;
      *pPara << _T("- Low Relaxation Strand = ") << pSpecEntry->GetStrandStressCoefficient(CSS_AFTER_TRANSFER,LOW_RELAX) << RPT_STRESS(_T("pu")) << rptNewLine;
   }

   if ( pSpecEntry->CheckStrandStress(CSS_AFTER_ALL_LOSSES) )
   {
      *pPara << rptNewLine;
      *pPara << _T("Stress Limit at service limit state after all losses") << rptNewLine;
      *pPara << _T("- Stress Relieved Strand = ") << pSpecEntry->GetStrandStressCoefficient(CSS_AFTER_ALL_LOSSES,STRESS_REL) << RPT_STRESS(_T("py")) << rptNewLine;
      *pPara << _T("- Low Relaxation Strand = ") << pSpecEntry->GetStrandStressCoefficient(CSS_AFTER_ALL_LOSSES,LOW_RELAX) << RPT_STRESS(_T("py")) << rptNewLine;
   }
}

void write_deflections(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, const SpecLibraryEntry* pSpecEntry)
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
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
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
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

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
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
