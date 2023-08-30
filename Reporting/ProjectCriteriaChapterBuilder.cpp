///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include <Lrfd/BDSManager.h>

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

CProjectCriteriaChapterBuilder::CProjectCriteriaChapterBuilder(bool bRating,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bRating = bRating;
}

LPCTSTR CProjectCriteriaChapterBuilder::GetName() const
{
   return TEXT("Project Criteria");
}

rptChapter* CProjectCriteriaChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGdrRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pGdrLineRptSpec = std::dynamic_pointer_cast<const CGirderLineReportSpecification>(pRptSpec);

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

   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable(3);
   *pPara << pLayoutTable << rptNewLine;

   write_load_modifiers(&(*pLayoutTable)(0, 0), pBroker, pDisplayUnits);
   write_environmental_conditions(&(*pLayoutTable)(0, 1), pBroker, pDisplayUnits);
   write_structural_analysis(&(*pLayoutTable)(0, 2), pBroker, pDisplayUnits);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   GET_IFACE2( pBroker, ISpecification, pSpec );
   std::_tstring spec_name = pSpec->GetSpecification();
   std::_tstring rating_name = pRatingSpec->GetRatingSpecification();

   GET_IFACE2( pBroker, ILibrary, pLib );
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   const RatingLibraryEntry* pRatingEntry = pLib->GetRatingEntry( rating_name.c_str() );


   pSpecEntry->Report(pChapter, pDisplayUnits);

   if (m_bRating)
   {
      pPara = new rptParagraph;
      *pChapter << pPara;
      pRatingEntry->Report(pChapter, pDisplayUnits);
   }

   return pChapter;
}


std::unique_ptr<WBFL::Reporting::ChapterBuilder> CProjectCriteriaChapterBuilder::Clone() const
{
   return std::make_unique<CProjectCriteriaChapterBuilder>(m_bRating);
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
   if (pEnvironment->GetExposureCondition() == pgsTypes::ExposureCondition::Normal)
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
