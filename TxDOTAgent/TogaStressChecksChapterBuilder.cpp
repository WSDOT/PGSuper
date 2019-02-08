///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include "TogaStressChecksChapterBuilder.h"
#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignUtilities.h"


#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\FlexuralStressCheckTable.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\Allowables.h>
#include <IFace\Intervals.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\CapacityToDemand.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CTogaStressChecksChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTogaStressChecksChapterBuilder::CTogaStressChecksChapterBuilder():
CPGSuperChapterBuilder(true)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTogaStressChecksChapterBuilder::GetName() const
{
   return TEXT("Stress Checks");
}

rptChapter* CTogaStressChecksChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   // This is a single segment report
   CSegmentKey segmentKey(girderKey,0);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,ISpecification,pSpec);
   *pPara << _T("Specification = ") << pSpec->GetSpecification() << rptNewLine;

   *pPara << Bold(_T("Notes: "))<< rptNewLine;
   *pPara <<symbol(DOT)<<_T(" Calculated total external load top and bottom stresses are multiplied by the appropriate (Top or Bottom) ratio of (Input Design Load Stress)/(Calculated Stress).");
   *pPara << _T(" This results in the Analysis Stress")<<rptNewLine;
   *pPara <<symbol(DOT)<<_T(" Stress Checks reflect the following sign convention: Compressive stress is negative. Tensile stress is positive.");

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetLastCompositeInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();


   BuildTableAndNotes(pChapter,pBroker,pDisplayUnits,releaseIntervalIdx,      pgsTypes::ServiceI);
   BuildTableAndNotes(pChapter,pBroker,pDisplayUnits,castDeckIntervalIdx,     pgsTypes::ServiceI);
   BuildTableAndNotes(pChapter,pBroker,pDisplayUnits,compositeDeckIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression);
   BuildTableAndNotes(pChapter,pBroker,pDisplayUnits,liveLoadIntervalIdx,     pgsTypes::ServiceI,pgsTypes::Compression);

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      BuildTableAndNotes(pChapter,pBroker,pDisplayUnits,liveLoadIntervalIdx,pgsTypes::ServiceIA,pgsTypes::Compression);
   }

   BuildTableAndNotes(pChapter,pBroker,pDisplayUnits,liveLoadIntervalIdx,pgsTypes::ServiceIII,pgsTypes::Tension);

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      BuildTableAndNotes(pChapter,pBroker,pDisplayUnits,liveLoadIntervalIdx,pgsTypes::FatigueI,pgsTypes::Compression);
   }

   return pChapter;
}

void CTogaStressChecksChapterBuilder::BuildTableAndNotes(rptChapter* pChapter, IBroker* pBroker,
                      IEAFDisplayUnits* pDisplayUnits,
                      IntervalIndexType intervalIdx,
                      pgsTypes::LimitState ls,
                      pgsTypes::StressType stress) const
{
   // We need the artifact that we've doctored for txdot reasons
   GET_IFACE2(pBroker,IGetTogaResults,pGetTogaResults);
   const pgsGirderArtifact* pFactoredGdrArtifact = pGetTogaResults->GetFabricatorDesignArtifact();
   const pgsSegmentArtifact* pSegmentArtifact = pFactoredGdrArtifact->GetSegmentArtifact(0);

   // Write notes from pgsuper default table, then our notes, then table
   CFlexuralStressCheckTable().BuildNotes(pChapter, pBroker, pFactoredGdrArtifact, 0, pDisplayUnits, intervalIdx, ls, true);

   CSegmentKey fabrSegmentKey(TOGA_SPAN,TOGA_FABR_GDR,0);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(fabrSegmentKey);

   if ( intervalIdx != releaseIntervalIdx)
   {
     // Toga Special notes
      rptParagraph* p = new rptParagraph;
      *pChapter << p;

      Float64 stress_val, stress_fac, stress_loc;
      pGetTogaResults-> GetControllingCompressiveStress(&stress_val, &stress_fac, &stress_loc);
      *p<<_T("Ratio applied to Top Stresses = ")<< stress_fac << rptNewLine;

      pGetTogaResults->GetControllingTensileStress(&stress_val, &stress_fac, &stress_loc);
      *p<<_T("Ratio applied to Bottom Stresses = ")<< stress_fac << rptNewLine;
   }


   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsSegmentArtifact* pUnfactoredGdrArtifact = pIArtifact->GetSegmentArtifact(fabrSegmentKey);

   BuildTable(pChapter, pBroker, pSegmentArtifact, pUnfactoredGdrArtifact, pDisplayUnits, intervalIdx, ls, stress);
}

void CTogaStressChecksChapterBuilder::BuildTable(rptChapter* pChapter, IBroker* pBroker,
                      const pgsSegmentArtifact* pFactoredGdrArtifact, const pgsSegmentArtifact* pUnfactoredGdrArtifact,
                      IEAFDisplayUnits* pDisplayUnits,
                      IntervalIndexType intervalIdx,
                      pgsTypes::LimitState limitState,
                      pgsTypes::StressType stressType) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress, pDisplayUnits->GetStressUnit(), false );

   rptCapacityToDemand cap_demand;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   CSegmentKey fabrSegmentKey(TOGA_SPAN, TOGA_FABR_GDR,0);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(fabrSegmentKey);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetLastCompositeInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   GET_IFACE2_NOCHECK(pBroker,IAllowableConcreteStress,pAllowable);

   // create and set up table
   const ColumnIndexType add_cols = 2;
   rptRcTable* p_table;
   if (intervalIdx == liveLoadIntervalIdx && limitState == pgsTypes::ServiceIII)
   {
      p_table = rptStyleManager::CreateDefaultTable(5+1,_T(""));
   }
   else if ( (intervalIdx == compositeDeckIntervalIdx && !pAllowable->CheckFinalDeadLoadTensionStress()) || intervalIdx == liveLoadIntervalIdx)
   {
      p_table = rptStyleManager::CreateDefaultTable(8+add_cols,_T(""));
   }
   else if (intervalIdx == releaseIntervalIdx )
   {
      p_table = rptStyleManager::CreateDefaultTable(10,_T(""));
   }
   else
   {
      p_table = rptStyleManager::CreateDefaultTable(9+add_cols,_T(""));
   }

   *p << p_table;


   ColumnIndexType col1=0;
   ColumnIndexType col2=0;

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,-1);
   if ( intervalIdx == releaseIntervalIdx )
   {
      (*p_table)(0,col1++) << COLHDR(RPT_GDR_END_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   }
   else
   {
      (*p_table)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   }

   std::_tstring strLimitState = GetLimitStateString(limitState);

   if ( limitState == pgsTypes::ServiceIII )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) << COLHDR(_T("Prestress") << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << _T("Prestress");
      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( limitState == pgsTypes::ServiceIII )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) << COLHDR(_T("Calculated") << rptNewLine << strLimitState << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) << COLHDR(_T("Analysis") << rptNewLine << strLimitState << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else if (intervalIdx == releaseIntervalIdx)
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << strLimitState;
      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << _T("Calculated") << rptNewLine << strLimitState;
      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << _T("Analysis") << rptNewLine << strLimitState;
      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( limitState == pgsTypes::ServiceIII )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) << COLHDR(_T("Demand") << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << _T("Demand");
      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   // get allowable stresses
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(fabrSegmentKey);

   const pgsFlexuralStressArtifact* pFactoredStressArtifact(nullptr);

   Float64 allowable_tension;
   Float64 allowable_tension_with_rebar;

   if (stressType==pgsTypes::Tension && ((intervalIdx != compositeDeckIntervalIdx) || (intervalIdx == compositeDeckIntervalIdx && pAllowable->CheckFinalDeadLoadTensionStress())) )
   {
      pFactoredStressArtifact = pFactoredGdrArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Tension,0 );
      allowable_tension = pFactoredStressArtifact->GetCapacity(pgsTypes::TopGirder);
      allowable_tension_with_rebar = pFactoredStressArtifact->GetAlternativeAllowableTensileStress(pgsTypes::TopGirder);
   }

   if (stressType==pgsTypes::Compression || intervalIdx != liveLoadIntervalIdx)
   {
      pFactoredStressArtifact = pFactoredGdrArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Compression,0 );
   }

   if (intervalIdx == compositeDeckIntervalIdx || intervalIdx == liveLoadIntervalIdx )
   {
      if ( intervalIdx == compositeDeckIntervalIdx || (intervalIdx == liveLoadIntervalIdx && limitState != pgsTypes::ServiceIII) )
      {
         p_table->SetRowSpan(0,col1,2);
         p_table->SetRowSpan(1,col2++,-1);
         (*p_table)(0,col1++) <<_T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");
      }

      if ( (intervalIdx == compositeDeckIntervalIdx && limitState == pgsTypes::ServiceI && pAllowable->CheckFinalDeadLoadTensionStress()) || (intervalIdx == liveLoadIntervalIdx && limitState == pgsTypes::ServiceIII) )
      {
         p_table->SetRowSpan(0,col1,2);
         p_table->SetRowSpan(1,col2++,-1);
         (*p_table)(0,col1) <<_T("Tension") << rptNewLine << _T("Status");
         if ( !IsZero(allowable_tension) )
         {
            (*p_table)(0,col1) << rptNewLine << _T("(C/D)");
         }

         col1++;
      }
   }
   else if ( intervalIdx == releaseIntervalIdx )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) << _T("Tension") << rptNewLine << _T("Status");
      if ( !IsZero(allowable_tension) )
      {
         (*p_table)(0,col1-1) << rptNewLine << _T("w/o rebar") << rptNewLine << _T("(C/D)");
      }

      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) << _T("Tension") << rptNewLine << _T("Status") << rptNewLine << _T("w/ rebar");
      if ( !IsZero(allowable_tension_with_rebar) )
      {
         (*p_table)(0,col1-1) << rptNewLine << _T("(C/D)");
      }

      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) << _T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");
   }
   else
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) <<_T("Tension")<<rptNewLine<<_T("Status");
      if ( !IsZero(allowable_tension) )
      {
         (*p_table)(0,col1-1) << rptNewLine << _T("(C/D)");
      }

      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) <<_T("Compression")<<rptNewLine<<_T("Status") << rptNewLine << _T("(C/D)");
   }

   p_table->SetNumberOfHeaderRows(2);
   for ( ColumnIndexType i = col1; i < p_table->GetNumberOfColumns(); i++ )
      p_table->SetColumnSpan(0,i,-1);

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();
   CollectionIndexType nArtifacts = pFactoredGdrArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,stressType);
   for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
   {
      ColumnIndexType col = 0;

      const pgsFlexuralStressArtifact* pFactoredOtherStressArtifact=0;
      const pgsFlexuralStressArtifact* pUnfactoredStressArtifact=0;

      if ( intervalIdx == compositeDeckIntervalIdx || intervalIdx == liveLoadIntervalIdx )
      {
         pFactoredStressArtifact   = pFactoredGdrArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,stressType,idx );
         pUnfactoredStressArtifact = pUnfactoredGdrArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,stressType,idx );
      }
      else
      {
         // get both tension and compression for other than bss3
         pFactoredStressArtifact      = pFactoredGdrArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Tension,idx );
         pFactoredOtherStressArtifact = pFactoredGdrArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Compression,idx );
         pUnfactoredStressArtifact    = pUnfactoredGdrArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Tension,idx );
      }

      const pgsPointOfInterest& poi(pFactoredStressArtifact->GetPointOfInterest());
      (*p_table)(row,col) << location.SetValue( (intervalIdx == releaseIntervalIdx ? POI_RELEASED_SEGMENT : POI_ERECTED_SEGMENT), poi);

      Float64 fTop, fBot;
      // prestress
      fTop = pFactoredStressArtifact->GetPretensionEffects(pgsTypes::TopGirder);
      fBot = pFactoredStressArtifact->GetPretensionEffects(pgsTypes::BottomGirder);
      if (limitState != pgsTypes::ServiceIII)
      {
         (*p_table)(row,++col) << stress.SetValue( fTop );
      }

      (*p_table)(row,++col) << stress.SetValue( fBot );

      // Calculated
      if (intervalIdx != releaseIntervalIdx)
      {
         fTop = pUnfactoredStressArtifact->GetExternalEffects(pgsTypes::TopGirder);
         fBot = pUnfactoredStressArtifact->GetExternalEffects(pgsTypes::BottomGirder);
         if (limitState != pgsTypes::ServiceIII)
         {
            (*p_table)(row,++col) << stress.SetValue( fTop );
         }

         (*p_table)(row,++col) << stress.SetValue( fBot );
      }

      // Analysis
      fTop = pFactoredStressArtifact->GetExternalEffects(pgsTypes::TopGirder);
      fBot = pFactoredStressArtifact->GetExternalEffects(pgsTypes::BottomGirder);
      if (limitState != pgsTypes::ServiceIII)
      {
         (*p_table)(row,++col) << stress.SetValue( fTop );
      }

      (*p_table)(row,++col) << stress.SetValue( fBot );

      // Demands
      fTop = pFactoredStressArtifact->GetDemand(pgsTypes::TopGirder);
      fBot = pFactoredStressArtifact->GetDemand(pgsTypes::BottomGirder);
      if (limitState != pgsTypes::ServiceIII)
      {
         (*p_table)(row,++col) << stress.SetValue( fTop );
      }

      (*p_table)(row,++col) << stress.SetValue( fBot );

      // Tension w/o rebar
      if ( intervalIdx == releaseIntervalIdx || 
           intervalIdx == tsRemovalIntervalIdx || 
           intervalIdx == castDeckIntervalIdx || 
          (intervalIdx == compositeDeckIntervalIdx && limitState == pgsTypes::ServiceI && pAllowable->CheckFinalDeadLoadTensionStress() ) ||
          (intervalIdx == liveLoadIntervalIdx && limitState == pgsTypes::ServiceIII)
         )
      {
         bool bPassed = (limitState == pgsTypes::ServiceIII ? pFactoredStressArtifact->Passed(pgsTypes::BottomGirder) : pFactoredStressArtifact->BeamPassed());
	      if ( bPassed )
         {
		     (*p_table)(row,++col) << RPT_PASS;
         }
	      else
         {
		     (*p_table)(row,++col) << RPT_FAIL;
         }


         Float64 fAllowTop,fAllowBot,allowable_tension;
         fAllowTop = pFactoredStressArtifact->GetCapacity(pgsTypes::TopGirder);
         fAllowBot = pFactoredStressArtifact->GetCapacity(pgsTypes::BottomGirder);
         if (fTop < fBot )
         {
            allowable_tension = fAllowBot;
         }
         else
         {
            allowable_tension = fAllowTop;
         }

         if ( !IsZero(allowable_tension) )
         {
            Float64 f = (limitState == pgsTypes::ServiceIII ? fBot : Max(fBot,fTop));
           (*p_table)(row,col) << rptNewLine <<_T("(")<< cap_demand.SetValue(allowable_tension,f,bPassed)<<_T(")");
         }
      }

      // Tension w/ rebar
      if ( intervalIdx == releaseIntervalIdx )
      {
         bool bPassed = ( fTop <= allowable_tension_with_rebar) && (fBot <= allowable_tension_with_rebar);
         if (bPassed)
         {
           (*p_table)(row,++col) << RPT_PASS;
         }
         else
         {
           (*p_table)(row,++col) << RPT_FAIL;
         }

         if ( !IsZero(allowable_tension_with_rebar) )
         {
            Float64 f = Max(fTop,fBot);
            (*p_table)(row,col) << rptNewLine <<_T("(")<< cap_demand.SetValue(allowable_tension_with_rebar,f,bPassed)<<_T(")");
          }
      }

      // Compression
      if ( intervalIdx == releaseIntervalIdx || 
           intervalIdx == tsRemovalIntervalIdx || 
           intervalIdx == castDeckIntervalIdx || 
           intervalIdx == compositeDeckIntervalIdx || 
          (intervalIdx == liveLoadIntervalIdx && (limitState == pgsTypes::ServiceI || limitState == pgsTypes::ServiceIA || limitState == pgsTypes::FatigueI))
         )
      {
         bool bPassed;
         if ( (intervalIdx == compositeDeckIntervalIdx && !pAllowable->CheckFinalDeadLoadTensionStress()) || intervalIdx == liveLoadIntervalIdx )
         {
            bPassed = pFactoredStressArtifact->BeamPassed();
         }
         else
         {
            bPassed = pFactoredOtherStressArtifact->BeamPassed();
            fTop = pFactoredOtherStressArtifact->GetDemand(pgsTypes::TopGirder);
            fBot = pFactoredOtherStressArtifact->GetDemand(pgsTypes::BottomGirder);
         }

         if ( bPassed )
         {
            (*p_table)(row, ++col) << RPT_PASS;
         }
	      else
         {
		      (*p_table)(row, ++col) << RPT_FAIL;
         }

         Float64 fAllowTop,fAllowBot,allowable_compression;
         fAllowTop = pFactoredStressArtifact->GetCapacity(pgsTypes::TopGirder);
         fAllowBot = pFactoredStressArtifact->GetCapacity(pgsTypes::BottomGirder);
         if (fTop < fBot )
         {
            allowable_compression = fAllowTop;
         }
         else
         {
            allowable_compression = fAllowBot;
         }

         Float64 f = Min(fTop,fBot);
         (*p_table)(row,col) << rptNewLine <<_T("(")<< cap_demand.SetValue(allowable_compression,f,bPassed)<<_T(")");
      }

      row++;
   }
}

CChapterBuilder* CTogaStressChecksChapterBuilder::Clone() const
{
   return new CTogaStressChecksChapterBuilder;
}
