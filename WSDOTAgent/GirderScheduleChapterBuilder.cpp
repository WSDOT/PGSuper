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
#include "GirderScheduleChapterBuilder.h"
#include <PGSuperTypes.h>
#include <Reporting\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderData.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
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
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pArtifact = pIArtifact->GetArtifact(span,girder);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   if( pArtifact->Passed() )
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << "The Specification Check was Successful" << rptNewLine;
   }
   else
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << color(Red) << "The Specification Check Was Not Successful" << color(Black) << rptNewLine;
   }

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

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pGdrArtifact = pArtifacts->GetArtifact(span,girder);
   const pgsConstructabilityArtifact* pConstArtifact = pGdrArtifact->GetConstructabilityArtifact();

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,girder);

   // create pois at the start of girder and mid-span
   pgsPointOfInterest pois(span,girder,0.0);

   GET_IFACE2( pBroker, ILibrary, pLib );
   GET_IFACE2( pBroker, ISpecification, pSpec );
   std::string spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   Float64 min_days =  ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Min(), unitMeasure::Day);
   Float64 max_days =  ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day);

   GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );
   std::vector<pgsPointOfInterest> pmid = pPointOfInterest->GetPointsOfInterest(span, girder,pgsTypes::BridgeSite1, POI_MIDSPAN);
   ATLASSERT(pmid.size()==1);

   GET_IFACE2(pBroker,IBridge,pBridge);
   if ( pBridge->GetDeckType() != pgsTypes::sdtNone && pConstArtifact->IsSlabOffsetApplicable() )
   {
      *p << "Dimension \"A\" at CL Bearing = "<< titledim1.SetValue(pConstArtifact->GetProvidedSlabOffset())
         << " based on a deflection (D at " << max_days << " days) of "<< titledim2.SetValue(pCamber->GetDCamberForGirderSchedule( pmid[0], CREEP_MAXTIME )) << 
            " at the time of slab casting"<<rptNewLine;
   }

   rptRcTable* p_table = pgsReportStyleHolder::CreateTableNoHeading(2,"");
   *p << p_table;

   RowIndexType row = 0;
   (*p_table)(row,0) << "Span";
   (*p_table)(row,1) << LABEL_SPAN(span);

   (*p_table)(++row,0) << "Girder";
   (*p_table)(row  ,1) << LABEL_GIRDER(girder);

   std::string start_connection = pBridge->GetRightSidePierConnection(span);
   (*p_table)(++row,0) << "End 1 Type";
   (*p_table)(row  ,1) << start_connection;

   std::string end_connection = pBridge->GetLeftSidePierConnection(span+1);
   (*p_table)(++row,0) << "End 2 Type";
   (*p_table)(row  ,1) << end_connection;

   const pgsLiftingCheckArtifact* pLiftArtifact = pGdrArtifact->GetLiftingCheckArtifact();
   if (pLiftArtifact!=NULL)
   {
      Float64 L = (pLiftArtifact->GetGirderLength() - pLiftArtifact->GetClearSpanBetweenPickPoints())/2.0;
      (*p_table)(++row,0) << "Location of Lifting Loops, L";
      (*p_table)(row  ,1) << loc.SetValue(L);
   }

   const pgsHaulingCheckArtifact* pHaulingArtifact = pGdrArtifact->GetHaulingCheckArtifact();
   if ( pHaulingArtifact != NULL )
   {
      Float64 L = pHaulingArtifact->GetLeadingOverhang();
      (*p_table)(++row,0) << "Location of Lead Shipping Support, " << Sub2("L","L");
      (*p_table)(row  ,1) << loc.SetValue(L);
      L = pHaulingArtifact->GetTrailingOverhang();
      (*p_table)(++row,0) << "Location of Trailing Shipping Support, " << Sub2("L","T");
      (*p_table)(row  ,1) << loc.SetValue(L);
   }

   CComPtr<IDirection> objDirGirder;
   pBridge->GetGirderBearing(span,girder,&objDirGirder);
   double dirGdr;
   objDirGirder->get_Value(&dirGdr);

   CComPtr<IDirection> objDir1;
   pBridge->GetPierDirection(span,&objDir1);
   double dir1;
   objDir1->get_Value(&dir1);

   CComPtr<IDirection> objDir2;
   pBridge->GetPierDirection(span+1,&objDir2);
   double dir2;
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

   (*p_table)(++row,0) << Sub2(symbol(theta),"1");
   (*p_table)(row  ,1) << angle.SetValue(t1);

   (*p_table)(++row,0) << Sub2(symbol(theta),"2");
   (*p_table)(row  ,1) << angle.SetValue(t2);

   const ConnectionLibraryEntry* pConnEntry = pLib->GetConnectionEntry( start_connection.c_str() );
   Float64 N1 = pConnEntry->GetGirderEndDistance();
   (*p_table)(++row,0) << Sub2("N","1");
   (*p_table)(row  ,1) << gdim.SetValue(N1);

   pConnEntry = pLib->GetConnectionEntry( end_connection.c_str() );
   Float64 N2 = pConnEntry->GetGirderEndDistance();
   (*p_table)(++row,0) << Sub2("N","2");
   (*p_table)(row  ,1) << gdim.SetValue(N2);

   PierIndexType prevPierIdx = (PierIndexType)span;
   PierIndexType nextPierIdx = prevPierIdx+1;
   bool bContinuousLeft, bContinuousRight, bIntegralLeft, bIntegralRight;

   pBridge->IsContinuousAtPier(prevPierIdx,&bContinuousLeft,&bContinuousRight);
   pBridge->IsIntegralAtPier(prevPierIdx,&bIntegralLeft,&bIntegralRight);

   (*p_table)(++row,0) << Sub2("P","1");
   if ( bContinuousLeft || bIntegralLeft )
   {
      (*p_table)(row,1) << "-";
   }
   else
   {
      Float64 P1 = N1 / sin(t1);
      (*p_table)(row  ,1) << gdim.SetValue(P1);
   }

   pBridge->IsContinuousAtPier(nextPierIdx,&bContinuousLeft,&bContinuousRight);
   pBridge->IsIntegralAtPier(nextPierIdx,&bIntegralLeft,&bIntegralRight);
   (*p_table)(++row,0) << Sub2("P","2");
   if ( bContinuousRight || bIntegralRight )
   {
      (*p_table)(row,1) << "-";
   }
   else
   {
      Float64 P2 = N2 / sin(t2);
      (*p_table)(row  ,1) << gdim.SetValue(P2);
   }

   (*p_table)(++row,0) << "Plan Length (Along Girder Grade)";
   (*p_table)(row  ,1) << glength.SetValue(pBridge->GetGirderPlanLength(span,girder));

   GET_IFACE2(pBroker, IBridgeMaterial, pMaterial);
   (*p_table)(++row,0) << RPT_FC << " (at 28 days)";
   (*p_table)(row  ,1) << stress.SetValue(pMaterial->GetFcGdr(span,girder));

   (*p_table)(++row,0) << RPT_FCI << " (at Release)";
   (*p_table)(row  ,1) << stress.SetValue(pMaterial->GetFciGdr(span,girder));

   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );
   
   StrandIndexType nh = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Harped);
   (*p_table)(++row,0) << "Number of Harped Strands";
   (*p_table)(row  ,1) << nh;

   Float64 hj = pStrandGeometry->GetPjack(span,girder,pgsTypes::Harped);
   (*p_table)(++row,0) << "Jacking Force, Harped Strands";
   (*p_table)(row  ,1) << force.SetValue(hj);

   StrandIndexType ns = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Straight);
   (*p_table)(++row,0) << "Number of Straight Strands";
   (*p_table)(row  ,1) << ns;
   StrandIndexType nDebonded = pStrandGeometry->GetNumDebondedStrands(span,girder,pgsTypes::Straight);
   if ( nDebonded != 0 )
      (*p_table)(row,1) << " (" << nDebonded << " debonded)";

   Float64 sj = pStrandGeometry->GetPjack(span,girder,pgsTypes::Straight);
   (*p_table)(++row,0) << "Jacking Force, Straight Strands";
   (*p_table)(row  ,1) << force.SetValue(sj);

   if ( 0 < pStrandGeometry->GetMaxStrands(span,girder,pgsTypes::Temporary ) )
   {
      StrandIndexType nt = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Temporary);
      (*p_table)(++row,0) << "Number of Temporary Strands";

      switch ( girderData.TempStrandUsage )
      {
      case pgsTypes::ttsPTAfterLifting:
         (*p_table)(row,0) << rptNewLine << "Temporary strands post-tensioned immediately after lifting";
         break;

      case pgsTypes::ttsPTBeforeShipping:
         (*p_table)(row,0) << rptNewLine << "Temporary strands post-tensioned immediately before shipping";
         break;
      }

      (*p_table)(row  ,1) << nt;

      Float64 tj = pStrandGeometry->GetPjack(span,girder,pgsTypes::Temporary);
      (*p_table)(++row,0) << "Jacking Force, Temporary Strands";
      (*p_table)(row  ,1) << force.SetValue(tj);
   }

   GET_IFACE2(pBroker, ISectProp2, pSectProp2 );
   Float64 ybg = pSectProp2->GetYb(pgsTypes::CastingYard,pmid[0]);
   Float64 nEff;
   Float64 sse = pStrandGeometry->GetSsEccentricity(pmid[0], &nEff);
   (*p_table)(++row,0) << "E";
   if (0 < ns)
      (*p_table)(row,1) << gdim.SetValue(ybg-sse);
   else
      (*p_table)(row,1) << RPT_NA;

   Float64 hse = pStrandGeometry->GetHsEccentricity(pmid[0], &nEff);
   (*p_table)(++row,0) << Sub2("F","C.L.");
   if (0 < nh)
      (*p_table)(row,1) << gdim.SetValue(ybg-hse);
   else
      (*p_table)(row,1) << RPT_NA;

   // get location of first harped strand
   (*p_table)(++row,0) << Sub2("F","b");
   if (0 < nh)
   {
      GDRCONFIG config = pBridge->GetGirderConfiguration(span,girder);
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

   Float64 ytg = pSectProp2->GetYtGirder(pgsTypes::CastingYard,pois);
   Float64 hss = pStrandGeometry->GetHsEccentricity(pois, &nEff);
   (*p_table)(++row,0) << Sub2("F","o");
   if (0 < nh)
   {
      (*p_table)(row,1) << gdim.SetValue(ytg+hss);
   }
   else
   {
      (*p_table)(row,1) << RPT_NA;
   }

   (*p_table)(++row,0) << "Screed Camber, C";
   (*p_table)(row  ,1) << gdim.SetValue(pCamber->GetScreedCamber( pmid[0] ) );

   // get # of days for creep
   (*p_table)(++row,0) << "Estimated camber at "<< min_days<<" days, D";
   (*p_table)(row  ,1) << gdim.SetValue(pCamber->GetDCamberForGirderSchedule( pmid[0], CREEP_MINTIME) );
   (*p_table)(++row,0) << "Estimated camber at "<< max_days<<" days, D";
   (*p_table)(row  ,1) << gdim.SetValue(pCamber->GetDCamberForGirderSchedule( pmid[0], CREEP_MAXTIME) );

   // Figure
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + "GirderSchedule.jpg") << rptNewLine;

   return pChapter;
}

CChapterBuilder* CGirderScheduleChapterBuilder::Clone() const
{
   return new CGirderScheduleChapterBuilder;
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
