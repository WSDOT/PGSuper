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
#include <PgsExt\PointOfInterest.h>
#include <PgsExt\BridgeDescription.h>
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

#include <Plugins\BeamFamilyCLSID.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define STIRRUP_ERROR_NONE        0
#define STIRRUP_ERROR            -1
#define STIRRUP_ERROR_BARSIZE    -2
#define STIRRUP_ERROR_ZONES      -3
#define STIRRUP_ERROR_SYMMETRIC  -4
#define STIRRUP_ERROR_STARTZONE  -5
#define STIRRUP_ERROR_LASTZONE   -6
#define STIRRUP_ERROR_V6         -7

#define DEBOND_ERROR_NONE        0
#define DEBOND_ERROR_SYMMETRIC   -1

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


   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CSpanData* pSpan = pIBridgeDesc->GetSpan(span);
   const CGirderTypes* pGirderTypes = pSpan->GetGirderTypes();
   const GirderLibraryEntry* pGdrLibEntry = pGirderTypes->GetGirderLibraryEntry(girder);
   CComPtr<IBeamFactory> factory;
   pGdrLibEntry->GetBeamFactory(&factory);
   CLSID familyCLSID = factory->GetFamilyCLSID();
   if ( CLSID_WFBeamFamily != familyCLSID && CLSID_UBeamFamily != familyCLSID && CLSID_DeckBulbTeeBeamFamily != familyCLSID && CLSID_SlabBeamFamily != familyCLSID )
   {
      rptParagraph* pPara = new rptParagraph;
      *pPara << _T("WSDOT girder schedules can only be created for WF, U, Slab, and Deck Bulb Tee sections") << rptNewLine;
      *pChapter << pPara;
      return pChapter;
   }

   GET_IFACE2( pBroker, IStrandGeometry, pStrandGeometry );
   StrandIndexType NhMax = pStrandGeometry->GetMaxStrands(span,girder,pgsTypes::Harped);
   if ( CLSID_SlabBeamFamily == familyCLSID && 0 < NhMax )
   {
      rptParagraph* pPara = new rptParagraph;
      *pPara << _T("Cannot create girder schedule. WSDOT Slab Beams do not use harped strands and this girder has harped strand locations") << rptNewLine;
      *pChapter << pPara;
      return pChapter;
   }


   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pArtifact = pIArtifact->GetArtifact(span,girder);

   bool bCanReportPrestressInformation = true;

   // WsDOT reports don't support Straight-Web strand option
   if (pStrandGeometry->GetAreHarpedStrandsForcedStraight(span, girder))
   {
      bCanReportPrestressInformation = false;
   }

   GET_IFACE2(pBroker,IGirderData,pIGirderData);
   const CGirderData* pGirderData = pIGirderData->GetGirderData(span,girder);

   if (pGirderData->PrestressData.GetNumPermStrandsType() == NPS_DIRECT_SELECTION)
   {
      bCanReportPrestressInformation = false;
   }

   if( pArtifact->Passed() )
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

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDisplayUnits->GetStressUnit(),        true );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,  angle,          pDisplayUnits->GetAngleUnit(),         true );

   INIT_FRACTIONAL_LENGTH_PROTOTYPE( gdim,      IS_US_UNITS(pDisplayUnits), 8, pDisplayUnits->GetComponentDimUnit(), true, true );
   INIT_FRACTIONAL_LENGTH_PROTOTYPE( glength,   IS_US_UNITS(pDisplayUnits), 8, pDisplayUnits->GetSpanLengthUnit(),   true, true );

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pGdrArtifact = pArtifacts->GetArtifact(span,girder);
   const pgsConstructabilityArtifact* pConstArtifact = pGdrArtifact->GetConstructabilityArtifact();

   GET_IFACE2(pBroker,ICamber,pCamber);

   // create pois at the start of girder and mid-span
   pgsPointOfInterest poiStart(span,girder,0.0);

   GET_IFACE2( pBroker, ILibrary, pLib );
   GET_IFACE2( pBroker, ISpecification, pSpec );
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   Float64 min_days =  ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Min(), unitMeasure::Day);
   Float64 max_days =  ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day);

   GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );
   std::vector<pgsPointOfInterest> pmid = pPointOfInterest->GetPointsOfInterest(span, girder,pgsTypes::BridgeSite1, POI_MIDSPAN);
   ATLASSERT(pmid.size()==1);
   pgsPointOfInterest poiMidSpan(pmid.front());

   GET_IFACE2(pBroker,IBridge,pBridge);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   rptRcTable* p_table = pgsReportStyleHolder::CreateTableNoHeading(2,_T(""));
   *p << p_table;

   RowIndexType row = 0;
   (*p_table)(row,0) << _T("Span");
   (*p_table)(row,1) << LABEL_SPAN(span);

   (*p_table)(++row,0) << _T("Girder");
   (*p_table)(row  ,1) << LABEL_GIRDER(girder);

   if ( familyCLSID == CLSID_SlabBeamFamily )
   {
      GET_IFACE2(pBroker,ISectProp2,pSectProp);
      Float64 Hg = pSectProp->GetHg(pgsTypes::CastingYard,poiMidSpan);
      (*p_table)(++row,0) << _T("Girder Height");
      (*p_table)(row,1) << gdim.SetValue(Hg);
   }
   else
   {
      (*p_table)(++row,0) << _T("Girder Series");
      (*p_table)(row,  1) << pGirderTypes->GetGirderName(girder);
   }

   (*p_table)(++row,0) << _T("End 1 Type");
   (*p_table)(row  ,1) << _T("*");

   (*p_table)(++row,0) << _T("End 2 Type");
   (*p_table)(row  ,1) << _T("*");

   if ( familyCLSID == CLSID_DeckBulbTeeBeamFamily )
   {
      GET_IFACE2(pBroker,IGirder,pIGirder);
      Float64 B = pIGirder->GetTopWidth(poiMidSpan);
      (*p_table)(++row,0) << _T("B");
      (*p_table)(row,1) << glength.SetValue(B);
   }
   else
   {
      if ( pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge )
      {
         (*p_table)(++row,0) << _T("\"A\" Dimension at CL Bearings");
         (*p_table)(row,  1) << gdim.SetValue(pBridgeDesc->GetSlabOffset());
      }
      else
      {
         (*p_table)(++row,0) << _T("\"A\" Dimension at CL Bearing End 1");
         (*p_table)(row,  1) << gdim.SetValue(pGirderTypes->GetSlabOffset(girder,pgsTypes::metStart));

         (*p_table)(++row,0) << _T("\"A\" Dimension at CL Bearing End 2");
         (*p_table)(row,  1) << gdim.SetValue(pGirderTypes->GetSlabOffset(girder,pgsTypes::metEnd));
      }
   }

   if ( familyCLSID == CLSID_WFBeamFamily )
   {
      // only applies to WF sections
      (*p_table)(++row,0) << _T("Intermediate Diaphragm Type");
      (*p_table)(row,  1) << _T("**");
   }

   const pgsLiftingCheckArtifact* pLiftArtifact = pGdrArtifact->GetLiftingCheckArtifact();
   if (pLiftArtifact!=NULL)
   {
      Float64 L = (pLiftArtifact->GetGirderLength() - pLiftArtifact->GetClearSpanBetweenPickPoints())/2.0;
      (*p_table)(++row,0) << _T("Location of Lifting Loops, L");
      (*p_table)(row  ,1) << glength.SetValue(L);
   }

   (*p_table)(++row,0) << Sub2(_T("L"),_T("d"));
   (*p_table)(row,  1) << _T("***");

   const pgsHaulingCheckArtifact* pHaulingArtifact = pGdrArtifact->GetHaulingCheckArtifact();
   if ( pHaulingArtifact != NULL )
   {
      Float64 L = pHaulingArtifact->GetLeadingOverhang();
      (*p_table)(++row,0) << _T("Location of Lead Shipping Support, ") << Sub2(_T("L"),_T("L"));
      (*p_table)(row  ,1) << glength.SetValue(L);
      L = pHaulingArtifact->GetTrailingOverhang();
      (*p_table)(++row,0) << _T("Location of Trailing Shipping Support, ") << Sub2(_T("L"),_T("T"));
      (*p_table)(row  ,1) << glength.SetValue(L);
   }

   CComPtr<IDirection> objDirGirder;
   pBridge->GetGirderBearing(span,girder,&objDirGirder);
   Float64 dirGdr;
   objDirGirder->get_Value(&dirGdr);

   CComPtr<IDirection> objDir1;
   pBridge->GetPierDirection(span,&objDir1);
   Float64 dir1;
   objDir1->get_Value(&dir1);

   CComPtr<IDirection> objDir2;
   pBridge->GetPierDirection(span+1,&objDir2);
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


   PierIndexType prevPierIdx = (PierIndexType)span;
   PierIndexType nextPierIdx = prevPierIdx+1;
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
      Float64 P1 = pBridge->GetGirderStartConnectionLength(span,girder);
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
      Float64 P2 = pBridge->GetGirderEndConnectionLength(span,girder);
      (*p_table)(row  ,1) << gdim.SetValue(P2);
   }

   (*p_table)(++row,0) << _T("Plan Length (Along Girder Grade)");
   (*p_table)(row  ,1) << glength.SetValue(pBridge->GetGirderPlanLength(span,girder));

   GET_IFACE2(pBroker, IBridgeMaterial, pMaterial);
   (*p_table)(++row,0) << RPT_FC << _T(" (at 28 days)");
   (*p_table)(row  ,1) << stress.SetValue(pMaterial->GetFcGdr(span,girder));

   (*p_table)(++row,0) << RPT_FCI << _T(" (at Release)");
   (*p_table)(row  ,1) << stress.SetValue(pMaterial->GetFciGdr(span,girder));

   
   StrandIndexType Ns = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Straight);
   StrandIndexType Nh = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Harped);
   if ( bCanReportPrestressInformation )
   {
      (*p_table)(++row,0) << _T("Number of Straight Strands");
      (*p_table)(row  ,1) << Ns;

      if ( CLSID_SlabBeamFamily != familyCLSID )
      {
         StrandIndexType nDebonded = pStrandGeometry->GetNumDebondedStrands(span,girder,pgsTypes::Straight);
         if ( nDebonded != 0 )
            (*p_table)(row,1) << _T(" (") << nDebonded << _T(" debonded)");

         (*p_table)(++row,0) << _T("Number of Harped Strands");
         (*p_table)(row  ,1) << Nh;
      }

      if ( 0 < pStrandGeometry->GetMaxStrands(span,girder,pgsTypes::Temporary ) )
      {
         StrandIndexType Nt = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Temporary);
         (*p_table)(++row,0) << _T("Number of Temporary Strands");

         switch ( pGirderData->PrestressData.TempStrandUsage )
         {
         case pgsTypes::ttsPTAfterLifting:
            (*p_table)(row,0) << rptNewLine << _T("Temporary strands post-tensioned immediately after lifting");
            break;

         case pgsTypes::ttsPTBeforeShipping:
            (*p_table)(row,0) << rptNewLine << _T("Temporary strands post-tensioned immediately before shipping");
            break;
         }

         (*p_table)(row  ,1) << Nt;
      }
   }
   else
   {
      (*p_table)(++row,0) << _T("Number of Straight Strands");
      (*p_table)(row  ,1) << _T("#");

      if ( CLSID_SlabBeamFamily != familyCLSID )
      {
         (*p_table)(++row,0) << _T("Number of Harped Strands");
         (*p_table)(row  ,1) << _T("#");
      }

      (*p_table)(++row,0) << _T("Number of Temporary Strands");
      (*p_table)(row  ,1) << _T("#");
   }

   GET_IFACE2(pBroker, ISectProp2, pSectProp2 );
   Float64 ybg = pSectProp2->GetYb(pgsTypes::CastingYard,poiMidSpan);
   Float64 nEff;
   Float64 sse = pStrandGeometry->GetSsEccentricity(poiMidSpan, &nEff);
   (*p_table)(++row,0) << _T("E");
   if (0 < Ns)
      (*p_table)(row,1) << gdim.SetValue(ybg-sse);
   else
      (*p_table)(row,1) << RPT_NA;

   if ( CLSID_SlabBeamFamily != familyCLSID )
   {
      Float64 hse = pStrandGeometry->GetHsEccentricity(poiMidSpan, &nEff);
      (*p_table)(++row,0) << Sub2(_T("F"),_T("C.L."));
      if (0 < Nh)
         (*p_table)(row,1) << gdim.SetValue(ybg-hse);
      else
         (*p_table)(row,1) << RPT_NA;


      Float64 ytg = pSectProp2->GetYtGirder(pgsTypes::CastingYard,poiStart);
      Float64 hss = pStrandGeometry->GetHsEccentricity(poiStart, &nEff);
      (*p_table)(++row,0) << Sub2(_T("F"),_T("o"));
      if (0 < Nh)
      {
         (*p_table)(row,1) << gdim.SetValue(ytg+hss);
      }
      else
      {
         (*p_table)(row,1) << RPT_NA;
      }
   }

   // Strand Extensions
   (*p_table)(++row,0) << _T("Straight Strands to Extend, End 1");
   StrandIndexType nExtended = 0;
   for ( StrandIndexType strandIdx = 0; strandIdx < Ns; strandIdx++ )
   {
      if ( pStrandGeometry->IsExtendedStrand(span,girder,pgsTypes::metStart,strandIdx,pgsTypes::Straight) )
      {
         nExtended++;
         (*p_table)(row,1) << _T(" ") << (strandIdx+1);
      }
   }
   if ( nExtended == 0 )
   {
      (*p_table)(row,1) << _T("");
   }

   (*p_table)(++row,0) << _T("Straight Strands to Extend, End 2");
   nExtended = 0;
   for ( StrandIndexType strandIdx = 0; strandIdx < Ns; strandIdx++ )
   {
      if ( pStrandGeometry->IsExtendedStrand(span,girder,pgsTypes::metEnd,strandIdx,pgsTypes::Straight) )
      {
         nExtended++;
         (*p_table)(row,1) << _T(" ") << (strandIdx+1);
      }
   }
   if ( nExtended == 0 )
   {
      (*p_table)(row,1) << _T("");
   }

   int debondResults = DEBOND_ERROR_NONE;
   if ( CLSID_SlabBeamFamily == familyCLSID )
   {
      // Debonding for slab beams
      p_table->SetColumnSpan(++row,0,2);
      p_table->SetColumnSpan(row,1,SKIP_CELL);
      (*p_table)(row,0) << _T("Straight Strands to Debond");

      std::vector<DebondInformation> debondInfo;
      debondResults = GetDebondDetails(pBroker,span,girder,debondInfo);

      if ( debondResults < 0 )
      {
         for ( int i = 0; i < 3; i++ )
         {
            p_table->SetColumnSpan(++row,0,2);
            p_table->SetColumnSpan(row,1,SKIP_CELL);
            (*p_table)(row,0) << _T("Group ") << (i+1);

            (*p_table)(++row,0) << _T("Strands to Debond");
            (*p_table)(row,1) << _T("%");
            (*p_table)(++row,0) << _T("Sleeved Length at Ends to Prevent Bond");
            (*p_table)(row,1) << _T("%");
         }
      }
      else
      {
         int groupCount = 0;
         std::vector<DebondInformation>::iterator iter(debondInfo.begin());
         std::vector<DebondInformation>::iterator end(debondInfo.end());
         for ( ; iter != end; iter++, groupCount++ )
         {
            DebondInformation& dbInfo = *iter;

            p_table->SetColumnSpan(++row,0,2);
            p_table->SetColumnSpan(row,1,SKIP_CELL);
            (*p_table)(row,0) << _T("Group ") << (groupCount+1);

            (*p_table)(++row,0) << _T("Strands to Debond");
            std::vector<StrandIndexType>::iterator strandIter(dbInfo.Strands.begin());
            std::vector<StrandIndexType>::iterator strandIterEnd(dbInfo.Strands.end());
            for ( ; strandIter != strandIterEnd; strandIter++ )
            {
               StrandIndexType strandIdx = *strandIter;
               (*p_table)(row,1) << _T(" ") << (strandIdx+1);
            }

            (*p_table)(++row,0) << _T("Sleeved Length at Ends to Prevent Bond");
            (*p_table)(row,1) << glength.SetValue(dbInfo.Length);
         }

         for ( int i = groupCount; i < 3; i++ )
         {
            p_table->SetColumnSpan(++row,0,2);
            p_table->SetColumnSpan(row,1,SKIP_CELL);
            (*p_table)(row,0) << _T("Group ") << (i+1);

            (*p_table)(++row,0) << _T("Strands to Debond");
            (*p_table)(row,1) << _T("-");
            (*p_table)(++row,0) << _T("Sleeved Length at Ends to Prevent Bond");
            (*p_table)(row,1) << _T("-");
         }
      }
   }

   Float64 C = 0;
   if ( pBridgeDesc->GetDeckDescription()->DeckType != pgsTypes::sdtNone )
   {
      C = pCamber->GetScreedCamber( poiMidSpan ) ;
      (*p_table)(++row,0) << _T("Screed Camber, C");
      (*p_table)(row  ,1) << gdim.SetValue(C);
   }

   // get # of days for creep
   Float64 D = 999;
   if ( pBridgeDesc->GetDeckDescription()->DeckType == pgsTypes::sdtNone )
   {
      D = pCamber->GetDCamberForGirderSchedule( poiMidSpan, CREEP_MAXTIME);
      (*p_table)(++row,0) << _T("D @ ") << max_days << _T(" days (") << Sub2(_T("D"),max_days) << _T(")");
      if ( D < 0 )
         (*p_table)(row  ,1) << color(Red) << gdim.SetValue(D) << color(Black);
      else
         (*p_table)(row  ,1) << gdim.SetValue(D);
   }
   else
   {
      D = 0.5*pCamber->GetDCamberForGirderSchedule( poiMidSpan, CREEP_MINTIME);
      (*p_table)(++row,0) << _T("Lower bound camber at ")<< min_days<<_T(" days, 50% of D") <<Sub(min_days);
      if ( D < 0 )
         (*p_table)(row  ,1) << color(Red) << gdim.SetValue(D) << color(Black);
      else
         (*p_table)(row  ,1) << gdim.SetValue(D);

      (*p_table)(++row,0) << _T("Upper bound camber at ")<< max_days<<_T(" days, D") << Sub(max_days);
      Float64 D120 = pCamber->GetDCamberForGirderSchedule( poiMidSpan, CREEP_MAXTIME) ;
      if ( D120 < 0 )
         (*p_table)(row  ,1) << color(Red) << gdim.SetValue(D120) << color(Black);
      else
         (*p_table)(row  ,1) << gdim.SetValue(D120);
   }


   // Stirrups
   IndexType V1, V3, V5;
   Float64 V2, V4, V6;
   int reinfDetailsResult = GetReinforcementDetails(pBroker,span,girder,familyCLSID,&V1,&V2,&V3,&V4,&V5,&V6);
   if (reinfDetailsResult < 0)
   {
      (*p_table)(++row,0) << _T("V1");
      (*p_table)(row,1) << _T("###");
      (*p_table)(++row,0) << _T("V2");
      (*p_table)(row,1) << _T("###");
      (*p_table)(++row,0) << _T("V3");
      (*p_table)(row,1) << _T("###");
      (*p_table)(++row,0) << _T("V4");
      (*p_table)(row,1) << _T("###");
      (*p_table)(++row,0) << _T("V5");
      (*p_table)(row,1) << _T("###");
      (*p_table)(++row,0) << _T("V6");
      (*p_table)(row,1) << _T("###");
   }
   else
   {
      (*p_table)(++row,0) << _T("V1");
      (*p_table)(row,1) << V1;
      (*p_table)(++row,0) << _T("V2");
      (*p_table)(row,1) << gdim.SetValue(V2);
      (*p_table)(++row,0) << _T("V3");
      (*p_table)(row,1) << V3;
      (*p_table)(++row,0) << _T("V4");
      (*p_table)(row,1) << gdim.SetValue(V4);
      (*p_table)(++row,0) << _T("V5");
      (*p_table)(row,1) << V5;
      (*p_table)(++row,0) << _T("V6");
      (*p_table)(row,1) << gdim.SetValue(V6);
   }

   // Stirrup Height

   if ( familyCLSID == CLSID_WFBeamFamily || familyCLSID == CLSID_UBeamFamily )
   {
      // H1 (Hg + "A" + 3")
      if ( pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge )
      {
         (*p_table)(++row,0) << _T("H1 (##)");
         Float64 Hg = pSectProp2->GetHg(pgsTypes::CastingYard,poiStart);
         Float64 H1 = pBridgeDesc->GetSlabOffset() + Hg + ::ConvertToSysUnits(3.0,unitMeasure::Inch);
         (*p_table)(row,  1) << gdim.SetValue(H1);
      }
      else
      {
         (*p_table)(++row,0) << _T("H1 at End 1 (##)");
         Float64 Hg = pSectProp2->GetHg(pgsTypes::CastingYard,poiStart);
         Float64 H1 = pGirderTypes->GetSlabOffset(girder,pgsTypes::metStart) + Hg + ::ConvertToSysUnits(3.0,unitMeasure::Inch);
         (*p_table)(row,  1) << gdim.SetValue(H1);

         (*p_table)(++row,0) << _T("H1 at End 2 (##)");
         pgsPointOfInterest poiEnd(poiStart);
         poiEnd.SetDistFromStart(pBridge->GetGirderLength(span,girder));
         Hg = pSectProp2->GetHg(pgsTypes::CastingYard,poiEnd);
         H1 = pGirderTypes->GetSlabOffset(girder,pgsTypes::metEnd) + Hg + ::ConvertToSysUnits(3.0,unitMeasure::Inch);
         (*p_table)(row,  1) << gdim.SetValue(H1);
      }
   }

   if ( D < C )
   {
      p = new rptParagraph;
      *pChapter << p;

      *p << color(Red) << Bold(_T("WARNING: Screed Camber, C, is less than the camber at time of deck casting, D. The girder will end up with a sag.")) << color(Black) << rptNewLine;
   }
   else if ( IsEqual(C,D,::ConvertToSysUnits(0.25,unitMeasure::Inch)) )
   {
      p = new rptParagraph;
      *pChapter << p;

      *p << color(Red) << Bold(_T("WARNING: Screed Camber, C, is nearly equal to the camber at time of deck casting, D. The girder may end up with a sag.")) << color(Black) << rptNewLine;
   }


   p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << p;
   *p << _T("* End Types to be determined by the Designer.") << rptNewLine;
   *p << _T("** Intermediate Diaphragm Type to be determined by the Designer.") << rptNewLine;
   *p << _T("*** ") << Sub2(_T("L"),_T("d")) << _T(" to be determined by the Designer.") << rptNewLine;

   if ( debondResults == DEBOND_ERROR_SYMMETRIC )
   {
      *p << _T("% Debonding must be symmetric for this girder schedule.") << rptNewLine;
   }

   if ( !bCanReportPrestressInformation )
   {
      *p << _T("# Prestressing information could not be included in the girder schedule because strand input is not consistent with the standard WSDOT details.") << rptNewLine;
   }

   if ( familyCLSID == CLSID_WFBeamFamily || familyCLSID == CLSID_UBeamFamily )
   {
      *p << _T("## H1 is computed as the height of the girder H + 3\" + \"A\" Dimension. Designers shall check H1 for the effect of vertical curve and increase as necessary.") << rptNewLine;
   }

   if (reinfDetailsResult == STIRRUP_ERROR_ZONES)
   {
      *p << _T("### Reinforcement details could not be listed because the number of transverse reinforcement zones is not consistent with the girder schedule.") << rptNewLine;
   }
   else if ( reinfDetailsResult == STIRRUP_ERROR_SYMMETRIC )
   {
      *p << _T("### Reinforcement details could not be listed because the girder schedule is for symmetric transverse reinforcement layouts.") << rptNewLine;
   }
   else if ( reinfDetailsResult == STIRRUP_ERROR_STARTZONE )
   {
      *p << _T("### Reinforcement details could not be listed because first zone of transverse reinforcement is not consistent with standard WSDOT details.") << rptNewLine;
   }
   else if ( reinfDetailsResult == STIRRUP_ERROR_LASTZONE )
   {
      if ( familyCLSID == CLSID_SlabBeamFamily )
         *p << _T("### Reinforcement details could not be listed because spacing in the last zone is not 9\" per the standard WSDOT details.") << rptNewLine;
      else
         *p << _T("### Reinforcement details could not be listed because spacing in the last zone is not 1'-6\" per the standard WSDOT details.") << rptNewLine;
   }
   else if ( reinfDetailsResult == STIRRUP_ERROR_BARSIZE )
   {
      *p << _T("### Reinforcement details could not be listed because the bar size is not #5 in one or more zones.") << rptNewLine;
   }
   else if ( reinfDetailsResult == STIRRUP_ERROR_V6 )
   {
      *p << _T("### Reinforcement details could not be listed because two different values are needed for V6.") << rptNewLine;
   }
   else if ( reinfDetailsResult < 0 )
   {
      *p << _T("### Reinforcement details could not be listed.") << rptNewLine;
   }

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
int CGirderScheduleChapterBuilder::GetReinforcementDetails(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,CLSID& familyCLSID,IndexType* pV1,Float64 *pV2,IndexType *pV3,Float64* pV4,IndexType *pV5,Float64* pV6) const
{
   GET_IFACE2(pBroker,IStirrupGeometry,pStirrupGeometry);
   if ( !pStirrupGeometry->AreStirrupZonesSymmetrical(span,gdr) )
      return STIRRUP_ERROR_SYMMETRIC;

   // Check if the number of zones is consistent with the girder schedule
   ZoneIndexType nZones = pStirrupGeometry->GetNumPrimaryZones(span,gdr); // this is total number of zones
   nZones = nZones/2 + 1; // this is the input number of zones (and it must be symmetric)
   if ( nZones != 5 && nZones != 6 )
      return STIRRUP_ERROR_ZONES;

   // Check first zone... it must be 1-1/2" long with 1-1/2" spacing... one space
   matRebar::Size barSize;
   Float64 count, spacing;
   pStirrupGeometry->GetPrimaryVertStirrupBarInfo(span,gdr,0, &barSize, &count, &spacing);

   Float64 zoneStart,zoneEnd;
   pStirrupGeometry->GetPrimaryZoneBounds(span,gdr,0,&zoneStart,&zoneEnd);
   pStirrupGeometry->GetPrimaryVertStirrupBarInfo(span,gdr,0, &barSize, &count, &spacing);
   Float64 zoneLength = zoneEnd - zoneStart;
   Float64 v = zoneLength/spacing;

   if ( barSize != matRebar::bs5 )
      return STIRRUP_ERROR_BARSIZE;

   if ( !IsEqual(v,1.0) && !IsEqual(spacing,::ConvertToSysUnits(1.5,unitMeasure::Inch)) )
      return STIRRUP_ERROR_STARTZONE;

   // So far, so good... start figuring out the V values

   // Zone 1 (V1 & V2)
   pStirrupGeometry->GetPrimaryZoneBounds(span,gdr,1,&zoneStart,&zoneEnd);
   pStirrupGeometry->GetPrimaryVertStirrupBarInfo(span,gdr,1, &barSize, &count, &spacing);
   zoneLength = zoneEnd - zoneStart;
   v = zoneLength/spacing;
   if ( !IsEqual(v,Round(v)) )
      return STIRRUP_ERROR_ZONES;

   if ( barSize != matRebar::bs5 )
      return STIRRUP_ERROR_BARSIZE;

   *pV1 = IndexType(floor(v));
   *pV2 = spacing;

   // Zone 2 (V3 & V4)
   pStirrupGeometry->GetPrimaryZoneBounds(span,gdr,2,&zoneStart,&zoneEnd);
   pStirrupGeometry->GetPrimaryVertStirrupBarInfo(span,gdr,2, &barSize, &count, &spacing);
   zoneLength = zoneEnd - zoneStart;
   v = zoneLength/spacing;
   if ( !IsEqual(v,Round(v)) )
      return STIRRUP_ERROR_ZONES;

   if ( barSize != matRebar::bs5 )
      return STIRRUP_ERROR_BARSIZE;

   *pV3 = IndexType(Round(v));
   *pV4 = spacing;

   // Zone 3 (either V6 or V5 & V6);
   Float64 v6Spacing;
   if ( nZones == 6 )
   {
      // this small zone that is labeled V6 on the stirrup layout is modeled
      pStirrupGeometry->GetPrimaryZoneBounds(span,gdr,3,&zoneStart,&zoneEnd);
      pStirrupGeometry->GetPrimaryVertStirrupBarInfo(span,gdr,3, &barSize, &count, &spacing);
      zoneLength = zoneEnd - zoneStart;
      v = zoneLength/spacing;
      if ( !IsEqual(v,Round(v)) )
         return STIRRUP_ERROR_ZONES;

      if ( familyCLSID == CLSID_UBeamFamily )
      {
         if ( barSize != matRebar::bs4 )
            return STIRRUP_ERROR_BARSIZE;
      }
      else
      {
         if ( barSize != matRebar::bs5 )
            return STIRRUP_ERROR_BARSIZE;
      }

      if ( !IsEqual(zoneLength,spacing) )
      {
         return STIRRUP_ERROR_V6;
      }

      v6Spacing = spacing;
   }

   ZoneIndexType zoneIdx = (nZones == 5 ? 3 : 4);
   pStirrupGeometry->GetPrimaryZoneBounds(span,gdr,zoneIdx,&zoneStart,&zoneEnd);
   pStirrupGeometry->GetPrimaryVertStirrupBarInfo(span,gdr,zoneIdx, &barSize, &count, &spacing);
   zoneLength = zoneEnd - zoneStart;
   v = zoneLength/spacing;
   if ( !IsEqual(v,Round(v)) )
      return STIRRUP_ERROR_ZONES;

   if ( familyCLSID == CLSID_UBeamFamily )
   {
      if ( barSize != matRebar::bs4 )
         return STIRRUP_ERROR_BARSIZE;
   }
   else
   {
      if ( barSize != matRebar::bs5 )
         return STIRRUP_ERROR_BARSIZE;
   }

   *pV5 = IndexType(Round(v)) - (nZones == 5 ? 1 : 0);
   *pV6 = spacing;

   if ( nZones == 6 && !IsEqual(spacing,v6Spacing) )
   {
      return STIRRUP_ERROR_V6;
   }

   zoneIdx = (nZones == 5 ? 4 : 5);
   pStirrupGeometry->GetPrimaryVertStirrupBarInfo(span,gdr,zoneIdx, &barSize, &count, &spacing);

   if ( familyCLSID == CLSID_WFBeamFamily || familyCLSID == CLSID_UBeamFamily )
   {
      if ( !IsEqual(spacing,::ConvertToSysUnits(18.0,unitMeasure::Inch)) )
         return STIRRUP_ERROR_LASTZONE;
   }
   else
   {
      if ( !IsEqual(spacing,::ConvertToSysUnits(9.0,unitMeasure::Inch)) )
         return STIRRUP_ERROR_LASTZONE;
   }


   if ( familyCLSID == CLSID_UBeamFamily || familyCLSID == CLSID_SlabBeamFamily )
   {
      if ( barSize != matRebar::bs4 )
         return STIRRUP_ERROR_BARSIZE;
   }
   else
   {
      if ( barSize != matRebar::bs5 )
         return STIRRUP_ERROR_BARSIZE;
   }

   return STIRRUP_ERROR_NONE;
}

int CGirderScheduleChapterBuilder::GetDebondDetails(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,std::vector<DebondInformation>& debondInfo) const
{
   GET_IFACE2( pBroker, IStrandGeometry, pStrandGeometry );
   if ( !pStrandGeometry->IsDebondingSymmetric(span,gdr) )
      return DEBOND_ERROR_SYMMETRIC;

   // fail if not symmetric
   SectionIndexType nSections = pStrandGeometry->GetNumDebondSections(span,gdr,IStrandGeometry::geLeftEnd,pgsTypes::Straight);
   for ( SectionIndexType sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
   {
      DebondInformation dbInfo;
      dbInfo.Length = pStrandGeometry->GetDebondSection(span,gdr,IStrandGeometry::geLeftEnd,sectionIdx,pgsTypes::Straight);

      dbInfo.Strands = pStrandGeometry->GetDebondedStrandsAtSection(span,gdr,IStrandGeometry::geLeftEnd,sectionIdx,pgsTypes::Straight);
       
      debondInfo.push_back(dbInfo);
   }

   return DEBOND_ERROR_NONE;
}
