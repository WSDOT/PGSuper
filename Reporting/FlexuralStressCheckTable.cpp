///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <Reporting\FlexuralStressCheckTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\TimelineEvent.h>
#include <PgsExt\GirderPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\FlexuralStressArtifact.h>
#include <PgsExt\CapacityToDemand.h>
#include <PgsExt\ClosurePourData.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CFlexuralStressCheckTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CFlexuralStressCheckTable::CFlexuralStressCheckTable()
{
}

CFlexuralStressCheckTable::CFlexuralStressCheckTable(const CFlexuralStressCheckTable& rOther)
{
   MakeCopy(rOther);
}

CFlexuralStressCheckTable::~CFlexuralStressCheckTable()
{
}

//======================== OPERATORS  =======================================
CFlexuralStressCheckTable& CFlexuralStressCheckTable::operator= (const CFlexuralStressCheckTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CFlexuralStressCheckTable::Build(rptChapter* pChapter,
                                      IBroker* pBroker,
                                      const pgsGirderArtifact* pGirderArtifact,
                                      IEAFDisplayUnits* pDisplayUnits,
                                      IntervalIndexType intervalIdx,
                                      pgsTypes::LimitState limitState,
                                      pgsTypes::StressType stressType
                                      ) const
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

   if ( intervalIdx < compositeDeckIntervalIdx )
   {
      // segments are not yet connected and act independently...
      // report segments individually
      GET_IFACE2(pBroker,IBridge,pBridge);
      SegmentIndexType nSegments = pBridge->GetSegmentCount(pGirderArtifact->GetGirderKey());
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
         if ( 1 < nSegments )
         {
            // Write out one section header for all segments
            if ( segIdx == 0 )
            {
               BuildSectionHeading(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, intervalIdx, limitState, stressType);
            }

            rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
            *pChapter << pPara;
            *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
            BuildAllowStressInformation(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, intervalIdx, limitState, stressType);
            BuildTable(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, intervalIdx, limitState, stressType);
         }
         else
         {
            Build(pChapter,pBroker,pGirderArtifact,segIdx,pDisplayUnits,intervalIdx,limitState,stressType);
         }

   #pragma Reminder("UPDATE: report stresses in closure pour")
         //if ( 1 < nSegments && segIdx != nSegments-1 )
         //{
         //   const pgsClosurePourArtifact* pClosurePourArtifact = pGirderArtifact->GetClosurePourArtifact(segIdx);
         //   Build(pChapter,pBroker,pClosurePourArtifact,pDisplayUnits,intervalIdx,limitState);
         //}
      }
   }
   else
   {
      // all girder segments are connected... report as a single girder
      Build(pChapter,pBroker,pGirderArtifact,ALL_SEGMENTS,pDisplayUnits,intervalIdx,limitState,stressType);
   }
}

void CFlexuralStressCheckTable::Build(rptChapter* pChapter,IBroker* pBroker,const pgsSegmentArtifact* pSegmentArtifact,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState,
                                           pgsTypes::StressType stressType) const
{
#pragma Reminder("OBSOLETE: remove obsolete code") // this method should not be called... remove it
#pragma Reminder("UPDATE: some things still call this method (TxDOT Agent)")
   ATLASSERT(false); // should never get here
   //// Write notes, then table
   //BuildNotes(pChapter, pBroker, pSegmentArtifact, pDisplayUnits, intervalIdx, limitState, stressType);
   //BuildTable(pChapter, pBroker, pSegmentArtifact, pDisplayUnits, intervalIdx, limitState, stressType);
}
   
void CFlexuralStressCheckTable::BuildNotes(rptChapter* pChapter, 
                   IBroker* pBroker,
                   const pgsSegmentArtifact* pSegmentArtifact,
                   IEAFDisplayUnits* pDisplayUnits,
                   IntervalIndexType intervalIdx,
                   pgsTypes::LimitState ls,
                   pgsTypes::StressType stress) const
{
#pragma Reminder("OBSOLETE: remove obsolete code") // this method should not be called... remove it
#pragma Reminder("UPDATE: some things still call this method (TxDOT Agent)")
   ATLASSERT(false); // should never get here
}


void CFlexuralStressCheckTable::Build(rptChapter* pChapter,IBroker* pBroker,
                                      const pgsGirderArtifact* pGirderArtifact,
                                      SegmentIndexType segIdx,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState,
                                           pgsTypes::StressType stressType) const
{
   // Write notes, then table
   BuildNotes(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, intervalIdx, limitState, stressType);
   BuildTable(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, intervalIdx, limitState, stressType);
}

void CFlexuralStressCheckTable::BuildNotes(rptChapter* pChapter, 
                                           IBroker* pBroker,
                                           const pgsGirderArtifact* pGirderArtifact,
                                           SegmentIndexType segIdx,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState,
                                           pgsTypes::StressType stressType) const
{
   BuildSectionHeading(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, intervalIdx, limitState, stressType);
   BuildAllowStressInformation(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, intervalIdx, limitState, stressType);
}

void CFlexuralStressCheckTable::BuildSectionHeading(rptChapter* pChapter, 
                                           IBroker* pBroker,
                                           const pgsGirderArtifact* pGirderArtifact,
                                           SegmentIndexType segIdx,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState,
                                           pgsTypes::StressType stressType) const
{
   USES_CONVERSION;

   // Build table
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress,   pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(CSegmentKey(pGirderArtifact->GetGirderKey(),segIdx == ALL_SEGMENTS ? 0 : segIdx));
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   EventIndexType eventIdx = pIntervals->GetStartEvent(intervalIdx);
   std::_tstring strEvent( pIBridgeDesc->GetEventByIndex(eventIdx)->GetDescription() );
   std::_tstring aux_msg1( releaseIntervalIdx == intervalIdx ? _T("For temporary stresses before losses ") : _T("For stresses at service limit state after losses ") );

   GET_IFACE2(pBroker, IEventMap, pEventMap );
   std::_tstring strLimitState = OLE2T(pEventMap->GetLimitStateName(limitState));

   std::_tstring aux_msg2;
   switch(limitState)
   {
   case pgsTypes::ServiceI:
      if (intervalIdx == releaseIntervalIdx)
      {
         if (stressType == pgsTypes::Compression)
            aux_msg2 = _T("in areas other than the precompressed tensile zones and without bonded auxiliary reinforcement.");
         else
            aux_msg2 = _T("in pretensioned components");
      }
      else if ( liveLoadIntervalIdx <= intervalIdx )
      {
         if (stressType == pgsTypes::Compression)
            aux_msg2 = _T("in other than segmentally constructed bridges due to permanent and transient loads");
         else
            ATLASSERT(0); // shouldn't happen
      }
      else 
      {
         if (stressType == pgsTypes::Compression)
            aux_msg2 = _T("in other than segmentally constructed bridges due to permanent loads");
         else
            aux_msg2 = _T("for components with bonded prestressing tendons other than piles");
      }

      break;

   case pgsTypes::ServiceIA:
      if (liveLoadIntervalIdx <= intervalIdx)
      {
         if (stressType == pgsTypes::Compression)
            aux_msg2 = _T("in other than segmentally constructed bridges due to live load plus one-half of the permanent loads");
         else
            CHECK(0); // shouldn't happen
      }
      else
         CHECK(0);

      break;

   case pgsTypes::ServiceIII:
      if (liveLoadIntervalIdx <= intervalIdx)
      {
         if (stressType == pgsTypes::Tension)
            aux_msg2 = _T("which involve traffic loading in members with bonded prestressing tendons other than piles");
         else
            CHECK(0); // shouldn't happen
      }
      else
         CHECK(0);

      break;

   case pgsTypes::FatigueI:
      if (liveLoadIntervalIdx <= intervalIdx)
      {
         if (stressType == pgsTypes::Compression)
            aux_msg2 = _T("in other than segmentally constructed bridges due to the Fatigue I load combination and one-half the sum of effective prestress and permanent loads");
         else
            CHECK(0); // shouldn't happen
      }
      else
         CHECK(0);

      break;
   }

   std::_tstring article; // LRFD article number
   if ( intervalIdx == releaseIntervalIdx && stressType == pgsTypes::Compression )
      article = _T("[5.9.4.1.1]");
   else if ( intervalIdx == releaseIntervalIdx && stressType == pgsTypes::Tension )
      article = _T("[5.9.4.1.2]");
   else if ( liveLoadIntervalIdx <= intervalIdx && stressType == pgsTypes::Compression )
      article = (limitState == pgsTypes::ServiceIA ? _T("[5.9.4.2.1]") : _T("[5.5.3.1]"));
   else if ( liveLoadIntervalIdx <= intervalIdx && stressType == pgsTypes::Tension )
      article = _T("[5.9.4.2.2]");
   else if ( stressType == pgsTypes::Compression )
      article = _T("[5.9.4.2.1]");
   else if ( stressType == pgsTypes::Tension )
      article = _T("[5.9.4.2.2]");
   else
      ATLASSERT(false); // should never get here

   std::_tostringstream os;
   if ( liveLoadIntervalIdx <= intervalIdx )
   {
      os << _T("Stress Check for ") << (stressType == pgsTypes::Compression ? _T("Compressive") : _T("Tensile") ) 
         << _T(" Stresses for ") << strLimitState << _T(" for ") << strEvent << std::endl;
   }
   else
   {
      os << _T("Stress Check for ") << strLimitState << _T(" for ") << strEvent << std::endl;
   }

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << os.str()  << _T(" ") << article;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;
   *p << aux_msg1 << aux_msg2 <<rptNewLine;
}

void CFlexuralStressCheckTable::BuildAllowStressInformation(rptChapter* pChapter, 
                                           IBroker* pBroker,
                                           const pgsGirderArtifact* pGirderArtifact,
                                           SegmentIndexType segIdx,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState,
                                           pgsTypes::StressType stressType) const
{
   USES_CONVERSION;

   // Build table
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress,   pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(CSegmentKey(pGirderArtifact->GetGirderKey(),segIdx == ALL_SEGMENTS ? 0 : segIdx));
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE2(pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker, ILibrary,       pLib );
   GET_IFACE2(pBroker, IEnvironment,   pEnvironment);

#pragma Reminder("UPDATE: update this code")
   // This code is basically a copy of what is in the SpecAgent
   // Update the SpecAgent so these coefficients are returned through a function
   // that way the logic is encapsulated in a single location
   std::_tstring specName = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( specName.c_str() );
   Float64 c; // compression coefficient
   Float64 t; // tension coefficient
   Float64 t_max; // maximum allowable tension
   Float64 t_with_rebar; // allowable tension when sufficient rebar is used
   bool b_t_max; // true if max allowable tension is applicable

   if ( intervalIdx == releaseIntervalIdx )
   {
      c = pSpecEntry->GetCyCompStressService();
      t = pSpecEntry->GetCyMaxConcreteTens();
      t_with_rebar = pSpecEntry->GetCyMaxConcreteTensWithRebar();
      pSpecEntry->GetCyAbsMaxConcreteTens(&b_t_max,&t_max);
   }
#pragma Reminder("UPDATE: review this... why is it commented out?") // fix if needed, delete if obsolete
   //else if ( stage == pIBridgeDesc->GetSegmentLiftingStageIndex(segmentKey) )
   //{
   //   ATLASSERT( ls == pgsTypes::ServiceI );
   //   x = pSpec->GetCyCompStressLifting();
   //}
   //else if ( stage == pIBridgeDesc->GetSegmentHaulingStageIndex(segmentKey) )
   //{
   //   ATLASSERT( ls == pgsTypes::ServiceI );
   //   x = pSpec->GetHaulingCompStress();
   //}
   else if ( intervalIdx == pIntervals->GetCastDeckInterval() )
   {
      c = pSpecEntry->GetBs1CompStress();
      t = pSpecEntry->GetBs1MaxConcreteTens();
      pSpecEntry->GetBs1AbsMaxConcreteTens(&b_t_max,&t_max);
   }
   else if ( intervalIdx == pIntervals->GetCompositeDeckInterval() )
   {
      c = pSpecEntry->GetBs2CompStress();
   }
   else if ( liveLoadIntervalIdx <= intervalIdx )
   {
      c = (limitState == pgsTypes::ServiceIA || limitState == pgsTypes::FatigueI ? pSpecEntry->GetBs3CompStressService1A() : pSpecEntry->GetBs3CompStressService() );
      t = (pEnvironment->GetExposureCondition() == expNormal ? pSpecEntry->GetBs3MaxConcreteTensNc() : pSpecEntry->GetBs3MaxConcreteTensSc() );
      pEnvironment->GetExposureCondition() == expNormal ? pSpecEntry->GetBs3AbsMaxConcreteTensNc(&b_t_max,&t_max) : pSpecEntry->GetBs3AbsMaxConcreteTensSc(&b_t_max,&t_max);
   }
   else
   {
      c = pSpecEntry->GetTempStrandRemovalCompStress();
      t = pSpecEntry->GetTempStrandRemovalMaxConcreteTens();
      pSpecEntry->GetTempStrandRemovalAbsMaxConcreteTens(&b_t_max,&t_max);
   }

   // get allowable stresses
   const pgsFlexuralStressArtifact* pArtifact;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   Float64 allowable_tension;
   Float64 allowable_tension_with_rebar;
   Float64 allowable_compression;

   GET_IFACE2(pBroker,IBridge,pBridge);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(pGirderArtifact->GetGirderKey());
   SegmentIndexType firstSegIdx = (segIdx == ALL_SEGMENTS ? 0 : segIdx);
   SegmentIndexType lastSegIdx  = (segIdx == ALL_SEGMENTS ? nSegments-1 : firstSegIdx);

   for ( SegmentIndexType sIdx = firstSegIdx; sIdx <= lastSegIdx; sIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(sIdx);      

      if ( firstSegIdx != lastSegIdx )
      {
         *p << _T("Segment ") << LABEL_SEGMENT(sIdx) << rptNewLine;
      }

      if (stressType == pgsTypes::Tension && (intervalIdx != compositeDeckIntervalIdx))
      {
         ATLASSERT( 0 < pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Tension));
         pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Tension,0 );

         allowable_tension = pArtifact->GetCapacity();
         allowable_tension_with_rebar = pSegmentArtifact->GetCastingYardCapacityWithMildRebar();
         *p << _T("Allowable tensile stress");

         if ( liveLoadIntervalIdx <= intervalIdx )
             *p << _T(" in the precompressed tensile zone");

         *p << _T(" = ") << tension_coeff.SetValue(t) << symbol(ROOT);

         if ( intervalIdx == releaseIntervalIdx )
            *p << RPT_FCI;
         else
            *p << RPT_FC;

         if ( b_t_max )
            *p << _T(" but not more than ") << stress_u.SetValue(t_max);

         *p  << _T(" = ") << stress_u.SetValue(allowable_tension)<<rptNewLine;

         if ( intervalIdx == releaseIntervalIdx )
         {
          *p << _T("Allowable tensile stress = ") << tension_coeff.SetValue(t_with_rebar) << symbol(ROOT) << RPT_FCI;
          *p << _T(" = ") << stress_u.SetValue(allowable_tension_with_rebar);
          *p << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.") << rptNewLine;
         }
      }

      if (stressType == pgsTypes::Compression || intervalIdx < liveLoadIntervalIdx)
      {
         ATLASSERT( 0 < pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Compression));
         pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Compression,0 );

         allowable_compression = pArtifact->GetCapacity();
         *p << _T("Allowable compressive stress = -") << c;
         if (intervalIdx == releaseIntervalIdx)
            *p << RPT_FCI;
         else
            *p << RPT_FC;

         *p << _T(" = ") <<stress_u.SetValue(allowable_compression)<<rptNewLine;
      }

      Float64 fc_reqd = pSegmentArtifact->GetRequiredConcreteStrength(intervalIdx,limitState);
      if ( 0 < fc_reqd )
      {
         if ( intervalIdx == releaseIntervalIdx )
            *p << RPT_FCI << _T(" required to satisfy this stress check = ") << stress_u.SetValue( fc_reqd ) << rptNewLine;
         else
            *p << RPT_FC  << _T(" required to satisfy this stress check = ") << stress_u.SetValue( fc_reqd ) << rptNewLine;
      }
      else if ( IsZero(fc_reqd) )
      {
         // do nothing if exactly zero
      }
      else
      {
         *p << _T("Regardless of the concrete strength, the stress requirements will not be satisfied.") << rptNewLine;
      }
   }
}

void CFlexuralStressCheckTable::BuildTable(rptChapter* pChapter, 
                                           IBroker* pBroker,
                                           const pgsGirderArtifact* pGirderArtifact,
                                           SegmentIndexType segIdx,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState,
                                           pgsTypes::StressType stressType) const
{
   USES_CONVERSION;

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(CSegmentKey(girderKey,segIdx == ALL_SEGMENTS ? 0 : segIdx));
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue,   stress,   pDisplayUnits->GetStressUnit(),     false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);
   rptCapacityToDemand cap_demand;

   bool bStressTendonInterval = false;
   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      if ( pIntervals->GetStressTendonInterval(girderKey,ductIdx) == intervalIdx )
      {
         bStressTendonInterval = true;
         break;
      }
   }


   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   // create and set up table
   rptRcTable* p_table;
   ColumnIndexType nColumns;
   if ( intervalIdx == releaseIntervalIdx || bStressTendonInterval )
   {
      nColumns = 10;
   }
   else if ( intervalIdx <= castDeckIntervalIdx )
   {
      nColumns = 9;
   }
   else if ( intervalIdx == railingSystemIntervalIdx )
   {
      nColumns = 8;
   }
   else if ( liveLoadIntervalIdx <= intervalIdx )
   {
      if ( limitState == pgsTypes::ServiceIII )
      {
         nColumns = 5;
      }
      else if ( limitState == pgsTypes::ServiceI || limitState == pgsTypes::ServiceIA || limitState == pgsTypes::FatigueI )
      {
         nColumns = 8;
      }
      else
      {
         ATLASSERT(false); // is there another limit state for live load?
      }
   }
   else
   {
      ATLASSERT(false);
      return;
   }

   if ( 0 < nDucts )
   {
      if ( limitState == pgsTypes::ServiceIII )
         nColumns++;
      else
         nColumns += 2;
   }

   p_table = pgsReportStyleHolder::CreateDefaultTable(nColumns);
   *p << p_table << rptNewLine;

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   ColumnIndexType col1 = 0;
   ColumnIndexType col2 = 0;

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);
   if ( intervalIdx == releaseIntervalIdx )
      (*p_table)(0,col1++) << COLHDR(RPT_GDR_END_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   else
      (*p_table)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );


   GET_IFACE2(pBroker, IEventMap, pEventMap );
   std::_tstring strLimitState = OLE2T(pEventMap->GetLimitStateName(limitState));

   if ( limitState == pgsTypes::ServiceIII )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1++) << COLHDR(_T("Pre-tension") << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      if ( 0 < nDucts )
      {
         p_table->SetRowSpan(0,col1,2);
         p_table->SetRowSpan(1,col2++,SKIP_CELL);
         (*p_table)(0,col1++) << COLHDR(_T("Post-tension") << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
   }
   else
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << _T("Pre-tension");
      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      if ( 0 < nDucts )
      {
         p_table->SetColumnSpan(0,col1,2);
         (*p_table)(0,col1++) << _T("Post-tension");
         (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
   }

   if ( limitState == pgsTypes::ServiceIII )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1++) << COLHDR(strLimitState << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << strLimitState;
      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( limitState == pgsTypes::ServiceIII )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
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
   Float64 allowable_tension = 0;
   Float64 allowable_tension_with_rebar = 0;
   Float64 allowable_compression = 0;

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   SegmentIndexType firstSegIdx = (segIdx == ALL_SEGMENTS ? 0 : segIdx);
   SegmentIndexType lastSegIdx  = (segIdx == ALL_SEGMENTS ? nSegments-1 : firstSegIdx);

   for ( SegmentIndexType sIdx = firstSegIdx; sIdx <= lastSegIdx; sIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(sIdx);
      if ( liveLoadIntervalIdx <= intervalIdx && limitState == pgsTypes::ServiceIII )
      {
         // tension only for this limit state
         ATLASSERT( 0 < pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Tension) );
         const pgsFlexuralStressArtifact* pArtifact;
         pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Tension,0 );
         allowable_tension = max(allowable_tension,pArtifact->GetCapacity());
      }
      else if ( liveLoadIntervalIdx <= intervalIdx && (limitState == pgsTypes::ServiceI || limitState == pgsTypes::FatigueI || limitState == pgsTypes::ServiceIA) )
      {
         // compression only for this limit state
         ATLASSERT(0 < pSegmentArtifact->GetFlexuralStressArtifactCount( intervalIdx,limitState,pgsTypes::Compression ) );
         const pgsFlexuralStressArtifact* pArtifact;
         pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Compression,0 );
         allowable_compression = min(allowable_compression,pArtifact->GetCapacity());
      }
      else
      {
         ATLASSERT(0 < pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Tension) );
         const pgsFlexuralStressArtifact* pArtifact;
         pArtifact = pSegmentArtifact->GetFlexuralStressArtifact(intervalIdx,limitState,pgsTypes::Tension,0);
         allowable_tension = max(allowable_tension,pArtifact->GetCapacity());
         allowable_tension_with_rebar = max(allowable_tension_with_rebar,pSegmentArtifact->GetCastingYardCapacityWithMildRebar());

         ATLASSERT(0 < pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Compression) );
         pArtifact = pSegmentArtifact->GetFlexuralStressArtifact(intervalIdx,limitState,pgsTypes::Compression,0);
         allowable_compression = min(allowable_compression,pArtifact->GetCapacity());
      }
   }

   if ( intervalIdx == releaseIntervalIdx || bStressTendonInterval )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1++) << _T("Tension") << rptNewLine << _T("Status");
      if ( !IsZero(allowable_tension) )
         (*p_table)(0,col1-1) << rptNewLine << _T("w/o rebar") << rptNewLine << _T("(C/D)");

      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1++) << _T("Tension") << rptNewLine << _T("Status") << rptNewLine << _T("w/ rebar");
      if ( !IsZero(allowable_tension_with_rebar) )
         (*p_table)(0,col1-1) << rptNewLine << _T("(C/D)");

      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1++) << _T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");
   }
   else if ( intervalIdx <= castDeckIntervalIdx )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1) <<_T("Tension") << rptNewLine << _T("Status");
      if ( !IsZero(allowable_tension) )
         (*p_table)(0,col1) << rptNewLine << _T("(C/D)");

      col1++;

      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1) <<_T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");

      col1++;
   }
   else if ( intervalIdx == railingSystemIntervalIdx )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1) <<_T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");

      col1++;
   }
   else if ( liveLoadIntervalIdx <= intervalIdx )
   {
      if ( limitState == pgsTypes::ServiceIII )
      {
         p_table->SetRowSpan(0,col1,2);
         p_table->SetRowSpan(1,col2++,SKIP_CELL);
         (*p_table)(0,col1) <<_T("Tension") << rptNewLine << _T("Status");
         if ( !IsZero(allowable_tension) )
            (*p_table)(0,col1) << rptNewLine << _T("(C/D)");

         col1++;
      }
      else if ( limitState == pgsTypes::ServiceI || limitState == pgsTypes::ServiceIA || limitState == pgsTypes::FatigueI )
      {
         p_table->SetRowSpan(0,col1,2);
         p_table->SetRowSpan(1,col2++,SKIP_CELL);
         (*p_table)(0,col1) <<_T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");

         col1++;
      }
      else
      {
         ATLASSERT(false); // is there another limit state for live load?
      }
   }
   else
   {
      ATLASSERT(false); // this table can't report results for the specified interval
                        // this is probably going to be an issue for PGSplice files
   }

   p_table->SetNumberOfHeaderRows(2);
   for ( ColumnIndexType i = col1; i < p_table->GetNumberOfColumns(); i++ )
      p_table->SetColumnSpan(0,i,SKIP_CELL);

   // Fill up the table
   Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));
   if ( intervalIdx == releaseIntervalIdx )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   for ( SegmentIndexType sIdx = firstSegIdx; sIdx <= lastSegIdx; sIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(sIdx);

      CollectionIndexType nArtifacts = pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,stressType);
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         ColumnIndexType col = 0;

         const pgsFlexuralStressArtifact* pTensionArtifact = 0;
         const pgsFlexuralStressArtifact* pCompressionArtifact = 0;

         if ( (intervalIdx == releaseIntervalIdx || bStressTendonInterval) || intervalIdx <= castDeckIntervalIdx )
         {
            pTensionArtifact     = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Tension,     idx );
            pCompressionArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Compression, idx );
         }
         else if ( intervalIdx == railingSystemIntervalIdx )
         {
            pCompressionArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Compression, idx );
         }
         else if ( liveLoadIntervalIdx <= intervalIdx )
         {
            if ( limitState == pgsTypes::ServiceIII )
            {
               pTensionArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Tension,idx );
            }
            else if ( limitState == pgsTypes::ServiceI || limitState == pgsTypes::ServiceIA || limitState == pgsTypes::FatigueI )
            {
               pCompressionArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Compression,idx );
            }
            else
            {
               ATLASSERT(false); // is there another limit state for live load?
            }
         }
         else
         {
            ATLASSERT(false); // this table can't report results for the specified interval
                              // this is probably going to be an issue for PGSplice files
         }

         if ( pTensionArtifact == NULL && pCompressionArtifact == NULL )
            continue;


         const pgsPointOfInterest& poi( pTensionArtifact ? pTensionArtifact->GetPointOfInterest() : pCompressionArtifact->GetPointOfInterest());

         (*p_table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

         if ( pTensionArtifact )
         {
            Float64 fTop, fBot;
            pTensionArtifact->GetPretensionEffects( &fTop, &fBot );

            // only checking bottom stress (precompressed tensile zone) for ServiceIII
            if (limitState != pgsTypes::ServiceIII)
            {
               (*p_table)(row,col++) << stress.SetValue( fTop );
            }

            (*p_table)(row,col++) << stress.SetValue( fBot );

            if ( 0 < nDucts )
            {
               pTensionArtifact->GetPosttensionEffects( &fTop, &fBot );

               // only checking bottom stress (precompressed tensile zone) for ServiceIII
               if (limitState != pgsTypes::ServiceIII)
               {
                  (*p_table)(row,col++) << stress.SetValue( fTop );
               }

               (*p_table)(row,col++) << stress.SetValue( fBot );
            }

            pTensionArtifact->GetExternalEffects( &fTop, &fBot );
            if (limitState != pgsTypes::ServiceIII)
            {
               (*p_table)(row,col++) << stress.SetValue( fTop );
            }

            (*p_table)(row,col++) << stress.SetValue( fBot );

            pTensionArtifact->GetDemand( &fTop, &fBot );
            if (limitState != pgsTypes::ServiceIII)
            {
               (*p_table)(row,col++) << stress.SetValue( fTop );
            }

            (*p_table)(row,col++) << stress.SetValue( fBot );

            // Tension w/o rebar
            bool bPassed;
            if ( limitState == pgsTypes::ServiceIII )
            {
               // Only checking tension on the bottom (precompressed tension zone)
               bPassed = pTensionArtifact->BottomPassed();
            }
            else
            {
               // checking tension everywhere
               bPassed = pTensionArtifact->Passed();
            }

            if ( bPassed )
              (*p_table)(row,col++) << RPT_PASS;
            else
              (*p_table)(row,col++) << RPT_FAIL;

            if ( !IsZero(allowable_tension) )
            {
               Float64 f = (limitState == pgsTypes::ServiceIII ? fBot : max(fBot,fTop));
              (*p_table)(row,col-1) << rptNewLine <<_T("(")<< cap_demand.SetValue(allowable_tension,f,bPassed)<<_T(")");
            }

            // Tension w/ rebar
            if ( intervalIdx == releaseIntervalIdx || bStressTendonInterval )
            {
               // tension w/ rebar is only applicable to the at release case

               bool bPassed = ( fTop <= allowable_tension_with_rebar) && (fBot <= allowable_tension_with_rebar);
               if (bPassed)
               {
                 (*p_table)(row,col++) << RPT_PASS;
               }
               else
               {
                 (*p_table)(row,col++) << RPT_FAIL;
               }

               if ( !IsZero(allowable_tension_with_rebar) )
               {
                  Float64 f = max(fTop,fBot);
                  (*p_table)(row,col-1) << rptNewLine <<_T("(")<< cap_demand.SetValue(allowable_tension_with_rebar,f,bPassed)<<_T(")");
               }
            }
         }

         // Compression
         if ( pCompressionArtifact )
         {
            Float64 fTop, fBot;
            if ( !pTensionArtifact )
            {
               // if there is a tension artifact, then this stuff is already reported
               // if not, report it here
               pCompressionArtifact->GetPretensionEffects( &fTop, &fBot );
               (*p_table)(row,col++) << stress.SetValue( fTop );
               (*p_table)(row,col++) << stress.SetValue( fBot );

               if ( 0 < nDucts )
               {
                  pCompressionArtifact->GetPosttensionEffects( &fTop, &fBot );
                  (*p_table)(row,col++) << stress.SetValue( fTop );
                  (*p_table)(row,col++) << stress.SetValue( fBot );
               }

               pCompressionArtifact->GetExternalEffects( &fTop, &fBot );
               (*p_table)(row,col++) << stress.SetValue( fTop );
               (*p_table)(row,col++) << stress.SetValue( fBot );

               pCompressionArtifact->GetDemand( &fTop, &fBot );
               (*p_table)(row,col++) << stress.SetValue( fTop );
               (*p_table)(row,col++) << stress.SetValue( fBot );
            }
            else
            {
               pCompressionArtifact->GetDemand( &fTop, &fBot );
            }

            bool bPassed = pCompressionArtifact->Passed();

            if ( bPassed )
               (*p_table)(row, col++) << RPT_PASS;
	         else
		         (*p_table)(row, col++) << RPT_FAIL;

            Float64 f = min(fTop,fBot);
            (*p_table)(row,col-1) << rptNewLine <<_T("(")<< cap_demand.SetValue(allowable_compression,f,bPassed)<<_T(")");
         }

         row++;
      } // next artifact
   } // next segment
}

void CFlexuralStressCheckTable::Build(rptChapter* pChapter,
                                      IBroker* pBroker,
                                      const pgsClosurePourArtifact* pClosureArtifact,
                                      IEAFDisplayUnits* pDisplayUnits,
                                      IntervalIndexType intervalIdx,
                                      pgsTypes::LimitState limitState) const
{
   BuildNotes(pChapter, pBroker, pClosureArtifact, pDisplayUnits, intervalIdx, limitState);
   BuildTable(pChapter, pBroker, pClosureArtifact, pDisplayUnits, intervalIdx, limitState);
}

void CFlexuralStressCheckTable::BuildNotes(rptChapter* pChapter, 
                                           IBroker* pBroker,
                                           const pgsClosurePourArtifact* pClosureArtifact,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState,
                                           pgsTypes::StressType stressType) const
{
   USES_CONVERSION;

   const CSegmentKey& closureKey = pClosureArtifact->GetClosurePourKey();

   // Build table
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress,   pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);


   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(closureKey);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   EventIndexType eventIdx = pIntervals->GetStartEvent(intervalIdx);
   std::_tstring strEvent( pIBridgeDesc->GetEventByIndex(eventIdx)->GetDescription() );
   std::_tstring aux_msg1( intervalIdx == releaseIntervalIdx ? _T("For temporary stresses before losses ") : _T("For stresses at service limit state after losses ") );

   GET_IFACE2(pBroker, IEventMap, pEventMap );
   std::_tstring strLimitState = OLE2T(pEventMap->GetLimitStateName(limitState));

   std::_tstring aux_msg2;
   switch(limitState)
   {
   case pgsTypes::ServiceI:
      if (intervalIdx == releaseIntervalIdx/*pgsTypes::CastingYard*/)
      {
         if (stressType == pgsTypes::Compression)
            aux_msg2 = _T("in areas other than the precompressed tensile zones and without bonded auxiliary reinforcement.");
         else
            aux_msg2 = _T("in pretensioned components");
      }
      else if ( intervalIdx == liveLoadIntervalIdx )
      {
         if (stressType == pgsTypes::Compression)
            aux_msg2 = _T("in other than segmentally constructed bridges due to permanent and transient loads");
         else
            ATLASSERT(0); // shouldn't happen
      }
      else //if (/*stage == pgsTypes::GirderPlacement ||*/ stage == pgsTypes::TemporaryStrandRemoval || stage==pgsTypes::BridgeSite1 || stage==pgsTypes::BridgeSite2)
      {
         if (stressType == pgsTypes::Compression)
            aux_msg2 = _T("in other than segmentally constructed bridges due to permanent loads");
         else
            aux_msg2 = _T("for components with bonded prestressing tendons other than piles");
      }
#pragma Reminder("OBSOLETE: remove if obsolete")
      ////else if (stage==pgsTypes::BridgeSite3)
      ////{
      ////   if (stressType == pgsTypes::Compression)
      ////      aux_msg2 = _T("in other than segmentally constructed bridges due to permanent and transient loads");
      ////   else
      ////      ATLASSERT(0); // shouldn't happen
      ////}
      //else
      //   ATLASSERT(0);

      break;

   case pgsTypes::ServiceIA:
      if (intervalIdx == liveLoadIntervalIdx /*pgsTypes::BridgeSite3*/)
      {
         if (stressType == pgsTypes::Compression)
            aux_msg2 = _T("in other than segmentally constructed bridges due to live load plus one-half of the permanent loads");
         else
            CHECK(0); // shouldn't happen
      }
      else
         CHECK(0);

      break;

   case pgsTypes::ServiceIII:
      if (liveLoadIntervalIdx <= intervalIdx/*pgsTypes::BridgeSite3*/)
      {
         if (stressType == pgsTypes::Tension)
            aux_msg2 = _T("which involve traffic loading in members with bonded prestressing tendons other than piles");
         else
            CHECK(0); // shouldn't happen
      }
      else
         CHECK(0);

      break;

   case pgsTypes::FatigueI:
      if (liveLoadIntervalIdx <= intervalIdx/*pgsTypes::BridgeSite3*/)
      {
         if (stressType == pgsTypes::Compression)
            aux_msg2 = _T("in other than segmentally constructed bridges due to the Fatigue I load combination and one-half the sum of effective prestress and permanent loads");
         else
            CHECK(0); // shouldn't happen
      }
      else
         CHECK(0);

      break;
   }

   GET_IFACE2(pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker, ILibrary,       pLib );
   GET_IFACE2(pBroker, IEnvironment,   pEnvironment);

#pragma Reminder("UPDATE: update this code")
   // This code is basically a copy of what is in the SpecAgent
   // Update the SpecAgent so these coefficients are returned through a function
   // that way the logic is encapsulated in a single location
   std::_tstring specName = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( specName.c_str() );
   Float64 c; // compression coefficient
   Float64 t; // tension coefficient
   Float64 t_max; // maximum allowable tension
   Float64 t_with_rebar; // allowable tension when sufficient rebar is used
   bool b_t_max; // true if max allowable tension is applicable

   if ( intervalIdx == releaseIntervalIdx )
   {
      c = pSpecEntry->GetCyCompStressService();
      t = pSpecEntry->GetCyMaxConcreteTens();
      t_with_rebar = pSpecEntry->GetCyMaxConcreteTensWithRebar();
      pSpecEntry->GetCyAbsMaxConcreteTens(&b_t_max,&t_max);
   }
   //else if ( stage == pIBridgeDesc->GetSegmentLiftingStageIndex(segmentKey) )
   //{
   //   ATLASSERT( ls == pgsTypes::ServiceI );
   //   x = pSpec->GetCyCompStressLifting();
   //}
   //else if ( stage == pIBridgeDesc->GetSegmentHaulingStageIndex(segmentKey) )
   //{
   //   ATLASSERT( ls == pgsTypes::ServiceI );
   //   x = pSpec->GetHaulingCompStress();
   //}
   else if ( intervalIdx == castDeckIntervalIdx )
   {
      c = pSpecEntry->GetBs1CompStress();
      t = pSpecEntry->GetBs1MaxConcreteTens();
      pSpecEntry->GetBs1AbsMaxConcreteTens(&b_t_max,&t_max);
   }
   else if ( liveLoadIntervalIdx <= intervalIdx )
   {
      c = (limitState == pgsTypes::ServiceIA || limitState == pgsTypes::FatigueI ? pSpecEntry->GetBs3CompStressService1A() : pSpecEntry->GetBs3CompStressService() );
      t = (pEnvironment->GetExposureCondition() == expNormal ? pSpecEntry->GetBs3MaxConcreteTensNc() : pSpecEntry->GetBs3MaxConcreteTensSc() );
      pEnvironment->GetExposureCondition() == expNormal ? pSpecEntry->GetBs3AbsMaxConcreteTensNc(&b_t_max,&t_max) : pSpecEntry->GetBs3AbsMaxConcreteTensSc(&b_t_max,&t_max);
   }
   else
   {
      c = pSpecEntry->GetTempStrandRemovalCompStress();
      t = pSpecEntry->GetTempStrandRemovalMaxConcreteTens();
      pSpecEntry->GetTempStrandRemovalAbsMaxConcreteTens(&b_t_max,&t_max);
   }

   std::_tstring article; // LRFD article number
   if ( intervalIdx == releaseIntervalIdx && stressType == pgsTypes::Compression )
      article = _T("[5.9.4.1.1]");
   else if ( intervalIdx == releaseIntervalIdx && stressType == pgsTypes::Tension )
      article = _T("[5.9.4.1.2]");
   else if ( liveLoadIntervalIdx <= intervalIdx && stressType == pgsTypes::Compression )
      article = (limitState == pgsTypes::ServiceIA ? _T("[5.9.4.2.1]") : _T("[5.5.3.1]"));
   else if ( liveLoadIntervalIdx <= intervalIdx && stressType == pgsTypes::Tension )
      article = _T("[5.9.4.2.2]");
   else if ( stressType == pgsTypes::Compression )
      article = _T("[5.9.4.2.1]");
   else if ( stressType == pgsTypes::Tension )
      article = _T("[5.9.4.2.2]");
   else
      ATLASSERT(false); // should never get here

   std::_tostringstream os;
   if ( liveLoadIntervalIdx <= intervalIdx )
   {
      os << _T("Stress Check for ") << (stressType == pgsTypes::Compression ? _T("Compressive") : _T("Tensile") ) 
         << _T(" Stresses for ") << strLimitState << _T(" for ") << strEvent << std::endl;
   }
   else
   {
      os << _T("Stress Check for ") << strLimitState << _T(" for ") << strEvent << std::endl;
   }

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << os.str()  << _T(" ") << article;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;
   *p << aux_msg1 << aux_msg2 <<rptNewLine;

   // get allowable stresses
   const pgsFlexuralStressArtifact* pArtifact;

   Float64 allowable_tension;
   Float64 allowable_compression;

   if (stressType == pgsTypes::Tension && (intervalIdx != compositeDeckIntervalIdx /*pgsTypes::BridgeSite2*/))
   {
      pArtifact = pClosureArtifact->GetFlexuralStressArtifact(pgsTypes::Tension);

      if ( !pArtifact )
      {
#pragma Reminder("OBSOLETE: this is temporary code")
         *p << _T("No flexural stress artifact for this stage") << rptNewLine;
         return;
      }

      allowable_tension = pArtifact->GetCapacity();
      *p << _T("Allowable tensile stress");

      if ( liveLoadIntervalIdx <= intervalIdx )
          *p << _T(" in the precompressed tensile zone");

      *p << _T(" = ") << tension_coeff.SetValue(t) << symbol(ROOT);

      if ( intervalIdx == releaseIntervalIdx )
         *p << RPT_FCI;
      else
         *p << RPT_FC;

      if ( b_t_max )
         *p << _T(" but not more than ") << stress_u.SetValue(t_max);

      *p  << _T(" = ") << stress_u.SetValue(allowable_tension)<<rptNewLine;
   }

   if (stressType==pgsTypes::Compression || intervalIdx < liveLoadIntervalIdx /*!= pgsTypes::BridgeSite3*/)
   {
      pArtifact = pClosureArtifact->GetFlexuralStressArtifact( pgsTypes::Compression );

      if ( !pArtifact )
      {
#pragma Reminder("OBSOLETE: this is temporary code")
         *p << _T("No flexural stress artifact for this stage") << rptNewLine;
         return;
      }

      allowable_compression = pArtifact->GetCapacity();
      *p << _T("Allowable compressive stress = -") << c;
      if (intervalIdx == releaseIntervalIdx)
         *p << RPT_FCI;
      else
         *p << RPT_FC;
      *p << _T(" = ") <<stress_u.SetValue(allowable_compression)<<rptNewLine;
   }

   Float64 fc_reqd = pClosureArtifact->GetRequiredConcreteStrength(intervalIdx,limitState);
   if ( 0 < fc_reqd )
   {
      if ( intervalIdx == releaseIntervalIdx )
         *p << RPT_FCI << _T(" required to satisfy this stress check = ") << stress_u.SetValue( fc_reqd ) << rptNewLine;
      else
         *p << RPT_FC  << _T(" required to satisfy this stress check = ") << stress_u.SetValue( fc_reqd ) << rptNewLine;
   }
   else if ( IsZero(fc_reqd) )
   {
      // do nothing if exactly zero
   }
   else
   {
      *p << _T("Regardless of the concrete strength, the stress requirements will not be satisfied.") << rptNewLine;
   }
}

void CFlexuralStressCheckTable::BuildTable(rptChapter* pChapter, 
                                           IBroker* pBroker,
                                           const pgsClosurePourArtifact* pClosureArtifact,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState) const
{
#pragma Reminder("REVIEW: commented out from RDP merge of allowable tension")
   ATLASSERT(false);

//   USES_CONVERSION;
//
//   const CSegmentKey& closureKey = pClosureArtifact->GetClosurePourKey();
//
//   // Build table
//   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
//   INIT_UV_PROTOTYPE( rptPressureSectionValue,   stress,   pDisplayUnits->GetStressUnit(),     false );
//
//   location.IncludeSpanAndGirder(closureKey.groupIndex == ALL_GROUPS);
//   rptCapacityToDemand cap_demand;
//
//
//   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
//
//   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
//   DuctIndexType nDucts = pTendonGeom->GetDuctCount(closureKey);
//
//   rptParagraph* p = new rptParagraph;
//   *pChapter << p;
//
//   //
//   // create and set up table
//   //
//   rptRcTable* p_table;
//   ColumnIndexType nColumns;
//   if ( limitState == pgsTypes::ServiceIII )
//      nColumns = 5;
//   else if ( limitState == pgsTypes::FatigueI || limitState == pgsTypes::ServiceIA )
//      nColumns = 8;
//   else
//      nColumns = 11;
//
//   if ( nDucts == 0 )
//      nColumns -= 2;
//
//   p_table = pgsReportStyleHolder::CreateDefaultTable(nColumns,_T(""));
//   *p << p_table;
//
//   if ( closureKey.groupIndex == ALL_GROUPS )
//   {
//      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
//      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
//   }
//
//   //
//   // Label Columns
//   //
//   ColumnIndexType col1 = 0;
//   ColumnIndexType col2 = 0;
//
//   p_table->SetRowSpan(0,col1,2);
//   p_table->SetRowSpan(1,col2++,SKIP_CELL);
//   (*p_table)(0,col1++) << _T("Location");
//
//
//   GET_IFACE2(pBroker, IEventMap, pEventMap );
//   std::_tstring strLimitState = OLE2T(pEventMap->GetLimitStateName(limitState));
//
//   if ( 0 < nDucts )
//   {
//      if ( limitState == pgsTypes::ServiceIII )
//      {
//         p_table->SetRowSpan(0,col1,2);
//         p_table->SetRowSpan(1,col2++,SKIP_CELL);
//         (*p_table)(0,col1++) << COLHDR(_T("Post-tension") << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
//      }
//      else
//      {
//         p_table->SetColumnSpan(0,col1,2);
//         (*p_table)(0,col1++) << _T("Post-tension");
//         (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
//         (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
//      }
//   }
//
//   if ( limitState == pgsTypes::ServiceIII )
//   {
//      p_table->SetRowSpan(0,col1,2);
//      p_table->SetRowSpan(1,col2++,SKIP_CELL);
//      (*p_table)(0,col1++) << COLHDR(strLimitState << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
//   }
//   else
//   {
//      p_table->SetColumnSpan(0,col1,2);
//      (*p_table)(0,col1++) << strLimitState;
//      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
//      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
//   }
//
//   if ( limitState == pgsTypes::ServiceIII )
//   {
//      p_table->SetRowSpan(0,col1,2);
//      p_table->SetRowSpan(1,col2++,SKIP_CELL);
//      (*p_table)(0,col1++) << COLHDR(_T("Demand") << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
//   }
//   else
//   {
//      p_table->SetColumnSpan(0,col1,2);
//      (*p_table)(0,col1++) << _T("Demand");
//      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
//      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
//   }
//
//   if ( intervalIdx == releaseIntervalIdx ) 
//   {
//      p_table->SetColumnSpan(0,col1,2);
//      (*p_table)(0,col1++) << _T("Tensile Capacity");
//      (*p_table)(1,col2++) << COLHDR(_T("Top"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
//      (*p_table)(1,col2++) << COLHDR(_T("Bottom"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
//   }
//
//   // get allowable stresses
//   Float64 allowable_tension, allowable_compression;
//   if ( limitState == pgsTypes::ServiceIII )
//   {
//      // tension only for this limit state
//      const pgsFlexuralStressArtifact* pTensionArtifact = pClosureArtifact->GetFlexuralStressArtifact(pgsTypes::Tension);
//      allowable_tension = pTensionArtifact->GetCapacity();
//   }
//   else if ( limitState == pgsTypes::FatigueI || limitState == pgsTypes::ServiceIA )
//   {
//      // compression only for this limit state
//      const pgsFlexuralStressArtifact* pCompressionArtifact = pClosureArtifact->GetFlexuralStressArtifact(pgsTypes::Compression);
//      allowable_compression = pCompressionArtifact->GetCapacity();
//   }
//   else
//   {
//      const pgsFlexuralStressArtifact* pTensionArtifact = pClosureArtifact->GetFlexuralStressArtifact(pgsTypes::Tension);
//      allowable_tension = pTensionArtifact->GetCapacity();
//      const pgsFlexuralStressArtifact* pCompressionArtifact = pClosureArtifact->GetFlexuralStressArtifact(pgsTypes::Compression);
//      allowable_compression = pCompressionArtifact->GetCapacity();
//   }
//
//   if ( limitState == pgsTypes::ServiceIII )
//   {
//      p_table->SetRowSpan(0,col1,2);
//      p_table->SetRowSpan(1,col2++,SKIP_CELL);
//      (*p_table)(0,col1) <<_T("Tension") << rptNewLine << _T("Status");
//      if ( !IsZero(allowable_tension) )
//         (*p_table)(0,col1) << rptNewLine << _T("(C/D)");
//
//      col1++;
//   }
//   else if ( limitState == pgsTypes::FatigueI || limitState == pgsTypes::ServiceIA )
//   {
//      p_table->SetRowSpan(0,col1,2);
//      p_table->SetRowSpan(1,col2++,SKIP_CELL);
//      (*p_table)(0,col1) <<_T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");
//
//      col1++;
//   }
//   else
//   {
//      p_table->SetRowSpan(0,col1,2);
//      p_table->SetRowSpan(1,col2++,SKIP_CELL);
//      (*p_table)(0,col1++) << _T("Tension") << rptNewLine << _T("Status");
//      if ( !IsZero(allowable_tension) )
//         (*p_table)(0,col1-1) << rptNewLine << _T("(C/D)");
//
//#pragma Reminder("OBSOLETE: remove if obsolete")
//      // I don't think we need to report this for closure pours
//      // Commented out during development
//      //p_table->SetRowSpan(0,col1,2);
//      //p_table->SetRowSpan(1,col2++,SKIP_CELL);
//      //(*p_table)(0,col1++) << _T("Tension") << rptNewLine << _T("Status") << rptNewLine << _T("w/ rebar");
//      //if ( !IsZero(allowable_tension_with_rebar) )
//      //   (*p_table)(0,col1-1) << rptNewLine << _T("(C/D)");
//
//      p_table->SetRowSpan(0,col1,2);
//      p_table->SetRowSpan(1,col2++,SKIP_CELL);
//      (*p_table)(0,col1++) << _T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");
//   }
//
//   p_table->SetNumberOfHeaderRows(2);
//   for ( ColumnIndexType i = col1; i < p_table->GetNumberOfColumns(); i++ )
//   {
//      p_table->SetColumnSpan(0,i,SKIP_CELL);
//   }
//
//   //
//   // Fill up the table
//   //
//   RowIndexType row = p_table->GetNumberOfHeaderRows();
//
//   ColumnIndexType col = 0;
//
//   const CClosurePourData* pClosure = pIBridgeDesc->GetClosurePourData(closureKey);
//   if ( pClosure->GetTemporarySupport() )
//   {
//      (*p_table)(row,col++) << _T("TS ") << LABEL_TEMPORARY_SUPPORT(pClosure->GetTemporarySupport()->GetIndex());
//   }
//   else
//   {
//      (*p_table)(row,col++) << _T("Pier ") << LABEL_PIER(pClosure->GetPier()->GetIndex());
//   }
//
//   const pgsFlexuralStressArtifact* pTensionArtifact = 0;
//   const pgsFlexuralStressArtifact* pCompressionArtifact = 0;
//
//   if ( limitState == pgsTypes::ServiceIII )
//   {
//      pTensionArtifact = pClosureArtifact->GetFlexuralStressArtifact(pgsTypes::Tension);
//   }
//   else if ( limitState == pgsTypes::FatigueI || limitState == pgsTypes::ServiceIA )
//   {
//      pCompressionArtifact = pClosureArtifact->GetFlexuralStressArtifact(pgsTypes::Compression);
//   }
//   else
//   {
//      pTensionArtifact     = pClosureArtifact->GetFlexuralStressArtifact(pgsTypes::Tension);
//      pCompressionArtifact = pClosureArtifact->GetFlexuralStressArtifact(pgsTypes::Compression);
//   }
//
//   if ( pTensionArtifact )
//   {
//      Float64 fTop, fBot;
//      if ( 0 < nDucts )
//      {
//         pTensionArtifact->GetPosttensionEffects( &fTop, &fBot );
//
//         // only checking bottom stress (precompressed tensile zone) for ServiceIII
//         if (limitState != pgsTypes::ServiceIII)
//         {
//            (*p_table)(row,col++) << stress.SetValue( fTop );
//         }
//
//         (*p_table)(row,col++) << stress.SetValue( fBot );
//      }
//
//      pTensionArtifact->GetExternalEffects( &fTop, &fBot );
//      if (limitState != pgsTypes::ServiceIII)
//      {
//         (*p_table)(row,col++) << stress.SetValue( fTop );
//      }
//
//      (*p_table)(row,col++) << stress.SetValue( fBot );
//
//      pTensionArtifact->GetDemand( &fTop, &fBot );
//      if (limitState != pgsTypes::ServiceIII)
//      {
//         (*p_table)(row,++col) << stress.SetValue( fTop );
//      }
//
//      (*p_table)(row,++col) << stress.SetValue( fBot );
//
//
//      if ( stage == pgsTypes::CastingYard )
//      {
//         // Casting yard tension varies along length of the girder
//         allowable_tension = pArtifact->GetCapacity();
//
//         // Only print capacity if demand is in tension
//         if (fTop>0.0)
//         {
//            (*p_table)(row,++col) << stress.SetValue( allowable_tension );
//         }
//         else
//         {
//            (*p_table)(row,++col) << _T("-");
//         }
//
//         if (fBot>0.0)
//         {
//            (*p_table)(row,++col) << stress.SetValue( allowable_tension );
//         }
//         else
//         {
//            (*p_table)(row,++col) << _T("-");
//         }
//      }
//
//      // Tension 
//      if ( stage == pgsTypes::CastingYard || 
////           stage == pgsTypes::GirderPlacement || 
//           stage == pgsTypes::TemporaryStrandRemoval || 
//           stage == pgsTypes::BridgeSite1 || 
//          (stage == pgsTypes::BridgeSite3 && limitState == pgsTypes::ServiceIII)
//         )
//      {
//         bool bPassed = (limitState == pgsTypes::ServiceIII ? pArtifact->BottomPassed() : pArtifact->Passed());
//	      if ( bPassed )
//		     (*p_table)(row,++col) << RPT_PASS;
//	      else
//		     (*p_table)(row,++col) << RPT_FAIL;
//
//         if ( !IsZero(allowable_tension) )
//         {
//            Float64 f = (limitState == pgsTypes::ServiceIII ? fBot : max(fBot,fTop));
//           (*p_table)(row,col) << rptNewLine <<_T("(")<< cap_demand.SetValue(allowable_tension,f,bPassed)<<_T(")");
//         }
//      }
//
//      // Compression
//      if (stage == pgsTypes::CastingYard || 
////          stage == pgsTypes::GirderPlacement ||
//          stage == pgsTypes::TemporaryStrandRemoval ||
//          stage == pgsTypes::BridgeSite1 ||
//          stage == pgsTypes::BridgeSite2 ||
//         (stage == pgsTypes::BridgeSite3 && (limitState == pgsTypes::ServiceI || limitState == pgsTypes::ServiceIA || limitState == pgsTypes::FatigueI))
//         )
//      {
//         bool bPassed;
//         if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3 )
//         {
//            bPassed = pArtifact->Passed();
//         }
//         else
//         {
//            bPassed = pOtherArtifact->Passed();
//            pOtherArtifact->GetDemand( &fTop, &fBot );
//         }
//
//         if ( bPassed )
//            (*p_table)(row, ++col) << RPT_PASS;
//	      else
//		      (*p_table)(row, ++col) << RPT_FAIL;
//
//         Float64 f = min(fTop,fBot);
//         (*p_table)(row,col) << rptNewLine <<_T("(")<< cap_demand.SetValue(allowable_compression,f,bPassed)<<_T(")");
//      }
//
//      row++;
//   }

/*
      if (limitState != pgsTypes::ServiceIII)
      {
         (*p_table)(row,col++) << stress.SetValue( fTop );
      }

      (*p_table)(row,col++) << stress.SetValue( fBot );

      // Tension w/o rebar
      bool bPassed;
      if ( limitState == pgsTypes::ServiceIII )
      {
         // Only checking tension on the bottom (precompressed tension zone)
         bPassed = pTensionArtifact->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar);
      }
      else
      {
         // checking tension everywhere
         bPassed = pTensionArtifact->Passed(pgsFlexuralStressArtifact::WithoutRebar);
      }

      if ( bPassed )
        (*p_table)(row,col++) << RPT_PASS;
      else
        (*p_table)(row,col++) << RPT_FAIL;

      if ( !IsZero(allowable_tension) )
      {
         Float64 f = (limitState == pgsTypes::ServiceIII ? fBot : max(fBot,fTop));
        (*p_table)(row,col-1) << rptNewLine <<_T("(")<< cap_demand.SetValue(allowable_tension,f,bPassed)<<_T(")");
      }

#pragma Reminder("OBSOLETE: remove if obsolete")
      // I don't think we need to report this for closure pours
      // Commented out during development

      //// Tension w/ rebar
      //if ( limitState != pgsTypes::ServiceIII )
      //{
      //   // tension w/ rebar not applicable to Service III limit state

      //   bool bPassed = ( fTop <= allowable_tension_with_rebar) && (fBot <= allowable_tension_with_rebar);
      //   if (bPassed)
      //   {
      //     (*p_table)(row,col++) << RPT_PASS;
      //   }
      //   else
      //   {
      //     (*p_table)(row,col++) << RPT_FAIL;
      //   }

      //   if ( !IsZero(allowable_tension_with_rebar) )
      //   {
      //      Float64 f = max(fTop,fBot);
      //      (*p_table)(row,col-1) << rptNewLine <<_T("(")<< cap_demand.SetValue(allowable_tension_with_rebar,f,bPassed)<<_T(")");
      //   }
      //}
   }

   // Compression
   if ( pCompressionArtifact )
   {
      Float64 fTop, fBot;
      if ( limitState == pgsTypes::FatigueI || limitState == pgsTypes::ServiceIA )
      {
         if ( 0 < nDucts )
         {
            pCompressionArtifact->GetPosttensionEffects( &fTop, &fBot );
            (*p_table)(row,col++) << stress.SetValue( fTop );
            (*p_table)(row,col++) << stress.SetValue( fBot );
         }

         pCompressionArtifact->GetExternalEffects( &fTop, &fBot );
         (*p_table)(row,col++) << stress.SetValue( fTop );
         (*p_table)(row,col++) << stress.SetValue( fBot );

         pCompressionArtifact->GetDemand( &fTop, &fBot );
         (*p_table)(row,col++) << stress.SetValue( fTop );
         (*p_table)(row,col++) << stress.SetValue( fBot );
      }
      else
      {
         pCompressionArtifact->GetDemand( &fTop, &fBot );
      }

      bool bPassed = pCompressionArtifact->Passed(pgsFlexuralStressArtifact::WithoutRebar);

      if ( bPassed )
         (*p_table)(row, col++) << RPT_PASS;
      else
	      (*p_table)(row, col++) << RPT_FAIL;

      Float64 f = min(fTop,fBot);
      (*p_table)(row,col-1) << rptNewLine <<_T("(")<< cap_demand.SetValue(allowable_compression,f,bPassed)<<_T(")");
   }

   row++;
*/
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CFlexuralStressCheckTable::MakeCopy(const CFlexuralStressCheckTable& rOther)
{
   // Add copy code here...
}

void CFlexuralStressCheckTable::MakeAssignment(const CFlexuralStressCheckTable& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool CFlexuralStressCheckTable::AssertValid() const
{
   return true;
}

void CFlexuralStressCheckTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CCombinedMomentsTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CFlexuralStressCheckTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CFlexuralStressCheckTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CFlexuralStressCheckTable");

   TESTME_EPILOG("CFlexuralStressCheckTable");
}
#endif // _UNITTEST
