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
#include <Reporting\LoadRatingReactionsChapterBuilder.h>
#include <Reporting\BridgeAnalysisReportSpecification.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\RatingSpecification.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <Reporting\VehicularLoadReactionTable.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLoadRatingReactionsChapterBuilder
****************************************************************************/

CLoadRatingReactionsChapterBuilder::CLoadRatingReactionsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CLoadRatingReactionsChapterBuilder::GetName() const
{
   return TEXT("Reactions");
}

rptChapter* CLoadRatingReactionsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   SpanIndexType spanIdx = ALL_SPANS;
   GirderIndexType gdrIdx = pGirderRptSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   // Responses from individual live load vehicules
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   std::vector<pgsTypes::LiveLoadType> live_load_types;
   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
   {
      live_load_types.push_back(pgsTypes::lltDesign);
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
   {
      live_load_types.push_back(pgsTypes::lltLegalRating_Routine);
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
   {
      live_load_types.push_back(pgsTypes::lltLegalRating_Special);
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
   {
      live_load_types.push_back(pgsTypes::lltPermitRating_Routine);
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
   {
      live_load_types.push_back(pgsTypes::lltPermitRating_Special);
   }

   GET_IFACE2(pBroker,ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   std::vector<pgsTypes::LiveLoadType>::iterator llTypeIter(live_load_types.begin());
   std::vector<pgsTypes::LiveLoadType>::iterator llTypeIterEnd(live_load_types.end());
   for ( ; llTypeIter != llTypeIterEnd; llTypeIter++ )
   {
      pgsTypes::LiveLoadType llType = *llTypeIter;

      GET_IFACE2( pBroker, IProductLoads, pProductLoads);
      std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,gdrIdx);

      // nothing to report if there are no loads
      if ( strLLNames.size() == 0 )
      {
         continue;
      }

      // if the only loading is the DUMMY load, then move on
      if ( strLLNames.size() == 1 && strLLNames[0] == std::_tstring(_T("No Live Load Defined")) )
      {
         continue;
      }

      rptParagraph* p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;

      if ( llType == pgsTypes::lltDesign )
      {
         *p << _T("Design Live Load Individual Vehicle Response") << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPermit )
      {
         *p << _T("Permit Live Load Individual Vehicle Response") << rptNewLine;
      }
      else if ( llType == pgsTypes::lltFatigue )
      {
         *p << _T("Fatigue Live Load Individual Vehicle Response") << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPedestrian )
      {
         *p << _T("Pedestrian Live Load Response") << rptNewLine;
      }
      else if ( llType == pgsTypes::lltLegalRating_Routine )
      {
         *p << _T("AASHTO Legal Rating Routine Commercial Vehicle Individual Vehicle Live Load Response")  << rptNewLine;
      }
      else if ( llType == pgsTypes::lltLegalRating_Special )
      {
         *p << _T("AASHTO Legal Rating Specialized Hauling Vehicle Individual Vehicle Live Load Response") << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPermitRating_Routine )
      {
         *p << _T("Routine Permit Rating Individual Vehicle Live Load Response") << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPermitRating_Special )
      {
         *p << _T("Special Permit Rating Individual Vehicle Live Load Response") << rptNewLine;
      }
      else
      {
         ATLASSERT(false); // is there a new live load type???
      }

      std::vector<std::_tstring>::iterator llNameIter(strLLNames.begin());
      std::vector<std::_tstring>::iterator llNameIterEnd(strLLNames.end());
      VehicleIndexType vehIdx = 0;
      for ( ; llNameIter != llNameIterEnd; llNameIter++, vehIdx++ )
      {
         std::_tstring strLLName = *llNameIter;

         p = new rptParagraph;
         *pChapter << p;
         p->SetName( strLLName.c_str() );
         *p << CVehicularLoadReactionTable().Build(pBroker,spanIdx,gdrIdx,llType,strLLName,vehIdx,analysisType,false,false,pDisplayUnits) << rptNewLine;
         *p << LIVELOAD_PER_LANE << rptNewLine;
      }
   }

   return pChapter;
}


CChapterBuilder* CLoadRatingReactionsChapterBuilder::Clone() const
{
   return new CLoadRatingReactionsChapterBuilder;
}
