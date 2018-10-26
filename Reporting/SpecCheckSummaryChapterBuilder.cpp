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

/****************************************************************************
CLASS
   CSpecCheckSummaryChapterBuilder
****************************************************************************/

#include <Reporting\SpecCheckSummaryChapterBuilder.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\GirderArtifactTool.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Artifact.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <EAF\EAFAutoProgress.h>

#include <LRFD\VersionMgr.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSpecCheckSummaryChapterBuilder::CSpecCheckSummaryChapterBuilder(bool referToDetailsReport,bool bSelect):
CPGSuperChapterBuilder(bSelect),
m_ReferToDetailsReport(referToDetailsReport)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CSpecCheckSummaryChapterBuilder::GetName() const
{
   return TEXT("Specification Check Summary");
}

rptChapter* CSpecCheckSummaryChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   // Report for a single girder
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   if (pSGRptSpec != NULL)
   {
      rptChapter* pChapter = CPGSuperChapterBuilder::Build(pSGRptSpec,level);

      CComPtr<IBroker> pBroker;
      pSGRptSpec->GetBroker(&pBroker);
      SpanIndexType span = pSGRptSpec->GetSpan();
      GirderIndexType gdr = pSGRptSpec->GetGirder();

      GET_IFACE2(pBroker,IArtifact,pIArtifact);
      const pgsGirderArtifact* pArtifact = pIArtifact->GetArtifact(span,gdr);

      CreateContent(pChapter, pBroker, span, gdr, pArtifact);

      return pChapter;
   }

   // Report multiple girders
   CMultiGirderReportSpecification* pMGRptSpec = dynamic_cast<CMultiGirderReportSpecification*>(pRptSpec);
   if (pMGRptSpec != NULL)
   {
      std::vector<SpanGirderHashType> list = pMGRptSpec->GetGirderList();

      // Give progress window a progress meter
      bool multi = list.size()>1;

      CComPtr<IBroker> pBroker;
      pMGRptSpec->GetBroker(&pBroker);
      GET_IFACE2(pBroker,IProgress,pProgress);
      DWORD mask = multi ? PW_ALL|PW_NOCANCEL : PW_ALL|PW_NOGAUGE|PW_NOCANCEL;

      CEAFAutoProgress ap(pProgress,0,mask); 

      if (multi)
         pProgress->Init(0,(short)list.size(),1);  // and for multi-girders, a gauge.

      // Build chapter and fill it
      rptChapter* pChapter = CPGSuperChapterBuilder::Build(pMGRptSpec,level);

      GET_IFACE2(pBroker,IArtifact,pIArtifact);

      for (std::vector<SpanGirderHashType>::iterator it=list.begin(); it!=list.end(); it++)
      {
         SpanIndexType span;
         GirderIndexType gdr;
         UnhashSpanGirder(*it,&span,&gdr);

         const pgsGirderArtifact* pArtifact = pIArtifact->GetArtifact(span,gdr);

         rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
         *pChapter << pParagraph;

         *pParagraph << _T("Results for Span ") << LABEL_SPAN(span) << _T(" Girder ") << LABEL_GIRDER(gdr);

         CreateContent(pChapter, pBroker, span, gdr, pArtifact);

         if (multi)
            pProgress->Increment();
      }

      return pChapter;
   }


   ATLASSERT(0);
   return NULL;
}

rptChapter* CSpecCheckSummaryChapterBuilder::BuildEx(CSpanGirderReportSpecification* pSGRptSpec,Uint16 level,
                    SpanIndexType span, GirderIndexType gdr, const pgsGirderArtifact* pArtifact) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pSGRptSpec,level);

   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);

   CreateContent(pChapter, pBroker, span, gdr, pArtifact);

   return pChapter;
}

void CSpecCheckSummaryChapterBuilder::CreateContent(rptChapter* pChapter, IBroker* pBroker,
                   SpanIndexType span, GirderIndexType gdr, const pgsGirderArtifact* pArtifact) const
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   if( pArtifact->Passed() )
   {
      *pPara << color(Green)<< _T("The Specification Check was Successful") << color(Black) << rptNewLine;
   }
   else
   {
      *pPara << color(Red) << _T("The Specification Check Was Not Successful") << color(Black) << rptNewLine;
     
      GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
      bool bPermit = pLimitStateForces->IsStrengthIIApplicable(span, gdr);

      // Build a list of our failures
      FailureList failures;

      // Allowable stress checks
      list_stress_failures(pBroker,failures,span,gdr,pArtifact,m_ReferToDetailsReport);

      // Moment Capacity Checks
      list_momcap_failures(pBroker,failures,span,gdr,pgsTypes::StrengthI,pArtifact);
      if ( bPermit )
         list_momcap_failures(pBroker,failures,span,gdr,pgsTypes::StrengthII,pArtifact);

      //Stirrup Checks
      list_vertical_shear_failures(pBroker,failures,span,gdr,pgsTypes::StrengthI,pArtifact);
      if ( bPermit )
         list_vertical_shear_failures(pBroker,failures,span,gdr,pgsTypes::StrengthII,pArtifact);

      list_horizontal_shear_failures(pBroker,failures,span,gdr,pgsTypes::StrengthI,pArtifact);
      if ( bPermit )
         list_horizontal_shear_failures(pBroker,failures,span,gdr,pgsTypes::StrengthII,pArtifact);

      list_stirrup_detailing_failures(pBroker,failures,span,gdr,pgsTypes::StrengthI,pArtifact);
      if ( bPermit )
         list_stirrup_detailing_failures(pBroker,failures,span,gdr,pgsTypes::StrengthII,pArtifact);

      list_debonding_failures(pBroker,failures,span,gdr,pArtifact);
      list_splitting_zone_failures(pBroker,failures,span,gdr,pArtifact);
      list_confinement_zone_failures(pBroker,failures,span,gdr,pArtifact);

      list_various_failures(pBroker,failures,span,gdr,pArtifact,m_ReferToDetailsReport);

      // Put failures into report
      for (FailureListIterator it=failures.begin(); it!=failures.end(); it++)
      {
         rptParagraph* pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara << *it << rptNewLine;
      }
   }

   // Negative camber is not technically a spec check, but a warning
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_MIDSPAN);
   CHECK(vPoi.size()==1);
   pgsPointOfInterest poi = *vPoi.begin();

   GET_IFACE2(pBroker,ICamber,pCamber);
   Float64 excess_camber = pCamber->GetExcessCamber(poi,CREEP_MAXTIME);
   if ( excess_camber < 0 )
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << _T("Warning:  Excess camber is negative, indicating a potential sag in the beam. Refer to the Details Report for more information.") << rptNewLine;
   }
}

CChapterBuilder* CSpecCheckSummaryChapterBuilder::Clone() const
{
   return new CSpecCheckSummaryChapterBuilder(m_ReferToDetailsReport);
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
