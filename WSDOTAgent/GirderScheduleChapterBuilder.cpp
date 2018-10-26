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
#include "GirderScheduleChapterBuilder.h"
#include <PGSuperTypes.h>
#include <Reporting\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <PgsExt\GirderPointOfInterest.h>
#include <PgsExt\StrandData.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#include <PgsExt\GirderArtifact.h>
#include <psgLib\SpecLibraryEntry.h>

#include <psgLib\ConnectionLibraryEntry.h>

#include <WBFLCogo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CGirderScheduleChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CGirderScheduleChapterBuilder::CGirderScheduleChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CGirderScheduleChapterBuilder::GetName() const
{
   return TEXT("Girder Schedule");
}

rptChapter* CGirderScheduleChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pGirderArtifact = pIArtifact->GetGirderArtifact(girderKey);

   GET_IFACE2( pBroker, IStrandGeometry, pStrandGeometry );
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, ISectionProperties, pSectProp );

   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc,            pDisplayUnits->GetSpanLengthUnit(),    true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,          pDisplayUnits->GetShearUnit(),         true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDisplayUnits->GetStressUnit(),        true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, mod_e,          pDisplayUnits->GetModEUnit(),          true );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment,         pDisplayUnits->GetMomentUnit(),        true );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,  angle,          pDisplayUnits->GetAngleUnit(),         true );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, wt_len, pDisplayUnits->GetForcePerLengthUnit(),true );
   INIT_UV_PROTOTYPE( rptMomentPerAngleUnitValue, spring, pDisplayUnits->GetMomentPerAngleUnit(),true );

   INIT_FRACTIONAL_LENGTH_PROTOTYPE( titledim1, IS_US_UNITS(pDisplayUnits), 4, pDisplayUnits->GetComponentDimUnit(), true, true );
   INIT_FRACTIONAL_LENGTH_PROTOTYPE( titledim2, IS_US_UNITS(pDisplayUnits), 8, pDisplayUnits->GetComponentDimUnit(), true, true );
   INIT_FRACTIONAL_LENGTH_PROTOTYPE( gdim,      IS_US_UNITS(pDisplayUnits), 8, pDisplayUnits->GetComponentDimUnit(), true, false );
   INIT_FRACTIONAL_LENGTH_PROTOTYPE( glength,   IS_US_UNITS(pDisplayUnits), 4, pDisplayUnits->GetSpanLengthUnit(),   true, false );

   SpanIndexType span = pBridge->GetGirderGroupStartSpan(girderKey.groupIndex);
   GirderIndexType girder = girderKey.girderIndex;

   PierIndexType prevPierIdx, nextPierIdx;
   pBridge->GetGirderGroupPiers(girderKey.groupIndex,&prevPierIdx,&nextPierIdx);

   bool bShowImage = true;
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);

      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsConstructabilityArtifact* pConstArtifact = pSegmentArtifact->GetConstructabilityArtifact();

      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      if ( 1 < nSegments )
      {
         rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }


      // WSDOT reports don't support Straight-Web strand option
      if (pStrandGeometry->GetAreHarpedStrandsForcedStraight(segmentKey))
      {
         rptParagraph* p;
         
         p = new rptParagraph;
         *pChapter << p;
         *p << color(Red) << Bold(_T("A WSDOT Girder Schedule could not be generated because this girder utilizes straight web strands. WSDOT Standard Girders utilize harped strands.")) << color(Black) << rptNewLine;
         bShowImage = false;
         continue;
      }


      const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
      if (pStrands->NumPermStrandsType == CStrandData::npsDirectSelection)
      {
         rptParagraph* p;
         
         p = new rptParagraph;
         *pChapter << p;
         *p << color(Red) << Bold(_T("A WSDOT Girder Schedule could not be generated because this girder utilizes Direct Strand Fill. WSDOT Standard Girders utilize sequentially filled strands.")) << color(Black) << rptNewLine;
         return pChapter;
      }

      if( pSegmentArtifact->Passed() )
      {
         rptParagraph* pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara << _T("The Specification Check was Successful") << rptNewLine;
      }
      else
      {
         rptParagraph* pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara << color(Red) << _T("The Specification Check Was Not Successful") << color(Black) << rptNewLine;
      }

      rptParagraph* p = new rptParagraph;
      *pChapter << p;


      // create pois at the start of girder and mid-span
      pgsPointOfInterest pois(segmentKey,0.0);

      GET_IFACE2( pBroker, ILibrary, pLib );
      GET_IFACE2( pBroker, ISpecification, pSpec );
      std::_tstring spec_name = pSpec->GetSpecification();
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
      Float64 min_days =  ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Min(), unitMeasure::Day);
      Float64 max_days =  ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day);

      GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );
      std::vector<pgsPointOfInterest> pmid( pPointOfInterest->GetPointsOfInterest(segmentKey,POI_MIDSPAN) );
      ATLASSERT(pmid.size()==1);

      if ( pBridge->GetDeckType() != pgsTypes::sdtNone && pConstArtifact->IsSlabOffsetApplicable() )
      {
         *p << _T("Dimension \"A\" at CL Bearing = ")<< titledim1.SetValue(pConstArtifact->GetProvidedSlabOffset())
            << _T(" based on a deflection (D at ") << max_days << _T(" days) of ")<< titledim2.SetValue(pCamber->GetDCamberForGirderSchedule( pmid[0], CREEP_MAXTIME )) << 
               _T(" at the time of slab casting")<<rptNewLine;
      }

      rptRcTable* p_table = pgsReportStyleHolder::CreateTableNoHeading(2,_T(""));
      *p << p_table;

      RowIndexType row = 0;
      (*p_table)(row,0) << _T("Span");
      (*p_table)(row,1) << LABEL_SPAN(span);

      (*p_table)(++row,0) << _T("Girder");
      (*p_table)(row  ,1) << LABEL_GIRDER(girder);

      //std::_tstring start_connection = pBridge->GetRightSidePierConnection(span);
      (*p_table)(++row,0) << _T("End 1 Type");
      (*p_table)(row  ,1) << color(Red) << _T("???") << color(Black); //start_connection;

      //std::_tstring end_connection = pBridge->GetLeftSidePierConnection(span+1);
      (*p_table)(++row,0) << _T("End 2 Type");
      (*p_table)(row  ,1) << color(Red) << _T("???") << color(Black); //end_connection;

      const pgsLiftingAnalysisArtifact* pLiftArtifact = pSegmentArtifact->GetLiftingAnalysisArtifact();
      if (pLiftArtifact!=NULL)
      {
         Float64 L = (pLiftArtifact->GetGirderLength() - pLiftArtifact->GetClearSpanBetweenPickPoints())/2.0;
         (*p_table)(++row,0) << _T("Location of Lifting Loops, L");
         (*p_table)(row  ,1) << loc.SetValue(L);
      }

      const pgsHaulingAnalysisArtifact* pHaulingArtifact = pSegmentArtifact->GetHaulingAnalysisArtifact();
      if ( pHaulingArtifact != NULL )
      {
         Float64 L = pHaulingArtifact->GetLeadingOverhang();
         (*p_table)(++row,0) << _T("Location of Lead Shipping Support, ") << Sub2(_T("L"),_T("L"));
         (*p_table)(row  ,1) << loc.SetValue(L);
         L = pHaulingArtifact->GetTrailingOverhang();
         (*p_table)(++row,0) << _T("Location of Trailing Shipping Support, ") << Sub2(_T("L"),_T("T"));
         (*p_table)(row  ,1) << loc.SetValue(L);
      }

      CComPtr<IDirection> objDirGirder;
      pBridge->GetSegmentBearing(segmentKey,&objDirGirder);
      Float64 dirGdr;
      objDirGirder->get_Value(&dirGdr);

      CComPtr<IDirection> objDir1;
      pBridge->GetPierDirection(prevPierIdx,&objDir1);
      Float64 dir1;
      objDir1->get_Value(&dir1);

      CComPtr<IDirection> objDir2;
      pBridge->GetPierDirection(nextPierIdx,&objDir2);
      Float64 dir2;
      objDir1->get_Value(&dir2);

      CComPtr<IAngle> objAngle1;
      objDirGirder->AngleBetween(objDir1,&objAngle1);
      Float64 t1;
      objAngle1->get_Value(&t1);

      CComPtr<IAngle> objAngle2;
      objDirGirder->AngleBetween(objDir2,&objAngle2);
      Float64 t2;
      objAngle2->get_Value(&t2);

      t1 -= M_PI;
      t2 -= M_PI;

      (*p_table)(++row,0) << Sub2(symbol(theta),_T("1"));
      (*p_table)(row  ,1) << angle.SetValue(t1);

      (*p_table)(++row,0) << Sub2(symbol(theta),_T("2"));
      (*p_table)(row  ,1) << angle.SetValue(t2);

      Float64 N1 = pBridge->GetSegmentStartEndDistance(segmentKey);
      (*p_table)(++row,0) << Sub2(_T("N"),_T("1"));
      (*p_table)(row  ,1) << gdim.SetValue(N1);

      Float64 N2 = pBridge->GetSegmentEndEndDistance(segmentKey);
      (*p_table)(++row,0) << Sub2(_T("N"),_T("2"));
      (*p_table)(row  ,1) << gdim.SetValue(N2);

      bool bContinuousLeft, bContinuousRight, bIntegralLeft, bIntegralRight;

      pBridge->IsContinuousAtPier(prevPierIdx,&bContinuousLeft,&bContinuousRight);
      pBridge->IsIntegralAtPier(prevPierIdx,&bIntegralLeft,&bIntegralRight);

      (*p_table)(++row,0) << Sub2(_T("P"),_T("1"));
      if ( bContinuousLeft || bIntegralLeft )
      {
         (*p_table)(row,1) << _T("-");
      }
      else
      {
         Float64 P1 = N1 / sin(t1);
         (*p_table)(row  ,1) << gdim.SetValue(P1);
      }

      pBridge->IsContinuousAtPier(nextPierIdx,&bContinuousLeft,&bContinuousRight);
      pBridge->IsIntegralAtPier(nextPierIdx,&bIntegralLeft,&bIntegralRight);
      (*p_table)(++row,0) << Sub2(_T("P"),_T("2"));
      if ( bContinuousRight || bIntegralRight )
      {
         (*p_table)(row,1) << _T("-");
      }
      else
      {
         Float64 P2 = N2 / sin(t2);
         (*p_table)(row  ,1) << gdim.SetValue(P2);
      }

      (*p_table)(++row,0) << _T("Plan Length (Along Girder Grade)");
      (*p_table)(row  ,1) << glength.SetValue(pBridge->GetSegmentPlanLength(segmentKey));

      GET_IFACE2(pBroker, IMaterials, pMaterial);
      (*p_table)(++row,0) << RPT_FC << _T(" (at 28 days)");
      (*p_table)(row  ,1) << stress.SetValue(pMaterial->GetSegmentFc(segmentKey,liveLoadIntervalIdx));

      (*p_table)(++row,0) << RPT_FCI << _T(" (at Release)");
      (*p_table)(row  ,1) << stress.SetValue(pMaterial->GetSegmentFc(segmentKey,releaseIntervalIdx));
     
      StrandIndexType nh = pStrandGeometry->GetNumStrands(segmentKey,pgsTypes::Harped);
      (*p_table)(++row,0) << _T("Number of Harped Strands");
      (*p_table)(row  ,1) << nh;

      Float64 hj = pStrandGeometry->GetPjack(segmentKey,pgsTypes::Harped);
      (*p_table)(++row,0) << _T("Jacking Force, Harped Strands");
      (*p_table)(row  ,1) << force.SetValue(hj);

      StrandIndexType ns = pStrandGeometry->GetNumStrands(segmentKey,pgsTypes::Straight);
      (*p_table)(++row,0) << _T("Number of Straight Strands");
      (*p_table)(row  ,1) << ns;
      StrandIndexType nDebonded = pStrandGeometry->GetNumDebondedStrands(segmentKey,pgsTypes::Straight);
      if ( nDebonded != 0 )
         (*p_table)(row,1) << _T(" (") << nDebonded << _T(" debonded)");

      Float64 sj = pStrandGeometry->GetPjack(segmentKey,pgsTypes::Straight);
      (*p_table)(++row,0) << _T("Jacking Force, Straight Strands");
      (*p_table)(row  ,1) << force.SetValue(sj);

      if ( 0 < pStrandGeometry->GetMaxStrands(segmentKey,pgsTypes::Temporary ) )
      {
         StrandIndexType nt = pStrandGeometry->GetNumStrands(segmentKey,pgsTypes::Temporary);
         (*p_table)(++row,0) << _T("Number of Temporary Strands");

         switch ( pStrands->TempStrandUsage )
         {
         case pgsTypes::ttsPTAfterLifting:
            (*p_table)(row,0) << rptNewLine << _T("Temporary strands post-tensioned immediately after lifting");
            break;

         case pgsTypes::ttsPTBeforeShipping:
            (*p_table)(row,0) << rptNewLine << _T("Temporary strands post-tensioned immediately before shipping");
            break;
         }

         (*p_table)(row  ,1) << nt;

         Float64 tj = pStrandGeometry->GetPjack(segmentKey,pgsTypes::Temporary);
         (*p_table)(++row,0) << _T("Jacking Force, Temporary Strands");
         (*p_table)(row  ,1) << force.SetValue(tj);
      }

      Float64 ybg = pSectProp->GetYb(releaseIntervalIdx,pmid[0]);
      Float64 nEff;
      Float64 sse = pStrandGeometry->GetSsEccentricity(releaseIntervalIdx, pmid[0], &nEff);
      (*p_table)(++row,0) << _T("E");
      if (0 < ns)
         (*p_table)(row,1) << gdim.SetValue(ybg-sse);
      else
         (*p_table)(row,1) << RPT_NA;

      Float64 hse = pStrandGeometry->GetHsEccentricity(releaseIntervalIdx, pmid[0], &nEff);
      (*p_table)(++row,0) << Sub2(_T("F"),_T("C.L."));
      if (0 < nh)
         (*p_table)(row,1) << gdim.SetValue(ybg-hse);
      else
         (*p_table)(row,1) << RPT_NA;

      // get location of first harped strand
      (*p_table)(++row,0) << Sub2(_T("F"),_T("b"));
      if (0 < nh)
      {
         GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);
         CComPtr<IPoint2d> pnt0, pnt1;
         pStrandGeometry->GetStrandPosition(pmid[0],0,pgsTypes::Harped,&pnt0);
         pStrandGeometry->GetStrandPosition(pmid[0],nh-1,pgsTypes::Harped,&pnt1);
         Float64 x,Fb0,Fb1;
         pnt0->Location(&x,&Fb0);
         pnt1->Location(&x,&Fb1);
         Float64 Fb = min(Fb0,Fb1);
         (*p_table)(row,1) << gdim.SetValue(Fb);
      }
      else
      {
         (*p_table)(row,1) << RPT_NA;
      }

      Float64 ytg = pSectProp->GetYtGirder(releaseIntervalIdx,pois);
      Float64 hss = pStrandGeometry->GetHsEccentricity(releaseIntervalIdx, pois, &nEff);
      (*p_table)(++row,0) << Sub2(_T("F"),_T("o"));
      if (0 < nh)
      {
         (*p_table)(row,1) << gdim.SetValue(ytg+hss);
      }
      else
      {
         (*p_table)(row,1) << RPT_NA;
      }

      (*p_table)(++row,0) << _T("Screed Camber, C");
      (*p_table)(row  ,1) << gdim.SetValue(pCamber->GetScreedCamber( pmid[0] ) );

      // get # of days for creep
      (*p_table)(++row,0) << _T("Lower bound camber at ")<< min_days<<_T(" days, 50% of D") <<Sub(min_days);
      (*p_table)(row  ,1) << gdim.SetValue(0.5*pCamber->GetDCamberForGirderSchedule( pmid[0], CREEP_MINTIME) );
      (*p_table)(++row,0) << _T("Upper bound camber at ")<< max_days<<_T(" days, D") << Sub(max_days);
      (*p_table)(row  ,1) << gdim.SetValue(pCamber->GetDCamberForGirderSchedule( pmid[0], CREEP_MAXTIME) );
   } // next segment

   // Figure
   if (bShowImage)
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("GirderSchedule.jpg")) << rptNewLine;
   }

   return pChapter;
}

CChapterBuilder* CGirderScheduleChapterBuilder::Clone() const
{
   return new CGirderScheduleChapterBuilder;
}
