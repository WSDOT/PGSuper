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

#include <LRFD\VersionMgr.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSpecCheckSummaryChapterBuilder::CSpecCheckSummaryChapterBuilder(bool referToDetailsReport):
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
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType gdr = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pArtifact = pIArtifact->GetArtifact(span,gdr);

   if( pArtifact->Passed() )
   {
      *pPara << "The Specification Check was Successful" << rptNewLine;
   }
   else
   {
      *pPara << color(Red) << "The Specification Check Was Not Successful" << color(Black) << rptNewLine;
     
      GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
      bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

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
   std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(pgsTypes::BridgeSite3,span,gdr,POI_MIDSPAN);
   CHECK(vPoi.size()==1);
   pgsPointOfInterest poi = *vPoi.begin();

   GET_IFACE2(pBroker,ICamber,pCamber);
   double excess_camber = pCamber->GetExcessCamber(poi,CREEP_MAXTIME);
   if ( excess_camber < 0 )
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << "Warning:  Excess camber is negative, indicating a potential sag in the beam. Refer to the Details Report for more information." << rptNewLine;
   }

   return pChapter;
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
