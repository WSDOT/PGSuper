///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#include <Reporting\MVRChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\CastingYardMomentsTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ProductShearTable.h>
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductDisplacementsTable.h>
#include <Reporting\ProductRotationTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\CombinedShearTable.h>
#include <Reporting\CombinedReactionTable.h>
#include <Reporting\ConcurrentShearTable.h>
#include <Reporting\UserMomentsTable.h>
#include <Reporting\UserShearTable.h>
#include <Reporting\UserReactionTable.h>
#include <Reporting\UserDisplacementsTable.h>
#include <Reporting\UserRotationTable.h>
#include <Reporting\LiveLoadDistributionFactorTable.h>
#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\VehicularLoadReactionTable.h>
#include <Reporting\LiveLoadReactionTable.h>

#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>

#include <psgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CMVRChapterBuilder
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CMVRChapterBuilder::CMVRChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CMVRChapterBuilder::GetName() const
{
   return TEXT("Moments, Shears, and Reactions");
}

rptChapter* CMVRChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IDisplayUnits,pDispUnit);

   rptParagraph* p = 0;

   GET_IFACE2(pBroker,ISpecification,pSpec);

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   // Product Moments
   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *p << "Load Responses - Casting Yard"<<rptNewLine;
   p->SetName("Casting Yard Results");
   *pChapter << p;

   p = new rptParagraph;
   *pChapter << p;
   *p << CCastingYardMomentsTable().Build(pBroker,span,girder,pDispUnit) << rptNewLine;

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *p << "Load Responses - Bridge Site"<<rptNewLine;
   p->SetName("Bridge Site Results");
   *pChapter << p;
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductMomentsTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),true,pDispUnit) << rptNewLine;
   *p << LIVELOAD_PER_LANE << rptNewLine;

    
   GET_IFACE2(pBroker,IProductForces,pProductForces);
   std::vector<std::string> strLLNames = pProductForces->GetVehicleNames(pgsTypes::lltDesign,girder);
   std::vector<std::string>::iterator iter;
   long j = 0;
   for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
   {
      *p << "(D" << j << ") " << *iter << rptNewLine;
   }

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      strLLNames = pProductForces->GetVehicleNames(pgsTypes::lltFatigue,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << "(F" << j << ") " << *iter << rptNewLine;
      }
   }

   if ( bPermit )
   {
      strLLNames = pProductForces->GetVehicleNames(pgsTypes::lltPermit,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << "(P" << j << ") " << *iter << rptNewLine;
      }
   }

   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
   bool are_user_loads = pUDL->DoUserLoadsExist(span,girder);
   if (are_user_loads)
   {
      *p << CUserMomentsTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),pDispUnit) << rptNewLine;
   }

   // Product Shears
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductShearTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),true,pDispUnit) << rptNewLine;
   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;

   strLLNames = pProductForces->GetVehicleNames(pgsTypes::lltDesign,girder);
   j = 0;
   for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
   {
      *p << "(D" << j << ") " << *iter << rptNewLine;
   }

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      strLLNames = pProductForces->GetVehicleNames(pgsTypes::lltFatigue,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << "(F" << j << ") " << *iter << rptNewLine;
      }
   }

   if ( bPermit )
   {
      strLLNames = pProductForces->GetVehicleNames(pgsTypes::lltPermit,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << "(P" << j << ") " << *iter << rptNewLine;
      }
   }

   if (are_user_loads)
   {
      *p << CUserShearTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),pDispUnit) << rptNewLine;
   }

   // Product Reactions
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductReactionTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),true,false,true,pDispUnit) << rptNewLine;
   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;

   strLLNames = pProductForces->GetVehicleNames(pgsTypes::lltDesign,girder);
   j = 0;
   for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
   {
      *p << "(D" << j << ") " << *iter << rptNewLine;
   }

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      strLLNames = pProductForces->GetVehicleNames(pgsTypes::lltFatigue,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << "(F" << j << ") " << *iter << rptNewLine;
      }
   }

   if ( bPermit )
   {
      strLLNames = pProductForces->GetVehicleNames(pgsTypes::lltPermit,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << "(P" << j << ") " << *iter << rptNewLine;
      }
   }

   if (are_user_loads)
   {
      *p << CUserReactionTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),pDispUnit) << rptNewLine;
   }

   // Product Displacements
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductDisplacementsTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),true,pDispUnit) << rptNewLine;
   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;

   strLLNames = pProductForces->GetVehicleNames(pgsTypes::lltDesign,girder);
   j = 0;
   for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
   {
      *p << "(D" << j << ") " << *iter << rptNewLine;
   }

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      strLLNames = pProductForces->GetVehicleNames(pgsTypes::lltFatigue,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << "(F" << j << ") " << *iter << rptNewLine;
      }
   }

   if ( bPermit )
   {
      strLLNames = pProductForces->GetVehicleNames(pgsTypes::lltPermit,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << "(P" << j << ") " << *iter << rptNewLine;
      }
   }

   if (are_user_loads)
   {
      *p << CUserDisplacementsTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),pDispUnit) << rptNewLine;
   }

   // Product Rotations
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductRotationTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),true,false,true,pDispUnit) << rptNewLine;
   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;

   strLLNames = pProductForces->GetVehicleNames(pgsTypes::lltDesign,girder);
   j = 0;
   for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
   {
      *p << "(D" << j << ") " << *iter << rptNewLine;
   }

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      strLLNames = pProductForces->GetVehicleNames(pgsTypes::lltFatigue,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << "(F" << j << ") " << *iter << rptNewLine;
      }
   }

   if ( bPermit )
   {
      strLLNames = pProductForces->GetVehicleNames(pgsTypes::lltPermit,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << "(P" << j << ") " << *iter << rptNewLine;
      }
   }

   if (are_user_loads)
   {
      *p << CUserRotationTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),pDispUnit) << rptNewLine;
   }

   GET_IFACE2( pBroker, ILibrary, pLib );
   std::string spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if (pSpecEntry->GetDoEvaluateLLDeflection())
   {
      // Optional Live Load Displacements
      p = new rptParagraph;
      p->SetName("Live Load Displacements");
      *pChapter << p;
      *p << CProductDisplacementsTable().BuildLiveLoadTable(pBroker,span,girder,pDispUnit) << rptNewLine;
      *p << "D1 = LRFD Design truck without lane load"<< rptNewLine;
      *p << "D2 = 0.25*(Design truck) + lane load"<< rptNewLine;
      *p << "D(Controlling) = Max(D1, D2)"<< rptNewLine;
      *p << rptNewLine;
   }

//   // Responses from individual live load vehicules
//   for ( int i = 0; i < 3; i++ )
//   {
//      pgsTypes::LiveLoadType llType = (pgsTypes::LiveLoadType)i;
//
//      GET_IFACE2( pBroker, IProductForces, pProductForces);
//      std::vector<std::string> strLLNames = pProductForces->GetVehicleNames(llType,girder);
//
//      // nothing to report if there are no loads
//      if ( strLLNames.size() == 0 )
//         continue;
//
//      // if the only loading is the DUMMY load, then move on
//      if ( strLLNames.size() == 1 && strLLNames[0] == std::string("No Live Load Defined") )
//         continue;
//
//      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
//      *pChapter << p;
//
//      if ( llType == pgsTypes::lltDesign )
//      {
//         p->SetName("Design Live Load Individual Vehicular Response");
//         *p << p->GetName() << rptNewLine;
//      }
//      else if ( llType == pgsTypes::lltPermit )
//      {
//         p->SetName("Permit Live Load Individual Vehicular Response");
//         *p << p->GetName() << rptNewLine;
//      }
//      else
//      {
//         p->SetName("Pedestrian Live Load Response");
//         *p << p->GetName() << rptNewLine;
//      }
//
//      std::vector<std::string>::iterator iter;
//      for ( iter = strLLNames.begin(); iter != strLLNames.end(); iter++ )
//      {
//         std::string strLLName = *iter;
//
//         long index = iter - strLLNames.begin();
//
//         p = new rptParagraph;
//         *pChapter << p;
//         p->SetName( strLLName.c_str() );
//         *p << CVehicularLoadResultsTable().Build(pBroker,span,girder,llType,strLLName,index,pSpec->GetAnalysisType(),false,pDispUnit) << rptNewLine;
//         *p << LIVELOAD_PER_LANE << rptNewLine;
//
//         *p << rptNewLine;
//
//         *p << CVehicularLoadReactionTable().Build(pBroker,span,girder,llType,strLLName,index,pSpec->GetAnalysisType(),false,pDispUnit) << rptNewLine;
//         *p << LIVELOAD_PER_LANE << rptNewLine;
//      }
//   }

   // Load Combinations (DC, DW, etc) & Limit States
   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << "Responses - Casting Yard Stage" << rptNewLine;
   p->SetName("Combined Results - Casting Yard");
   CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::CastingYard, pSpec->GetAnalysisType());
//   CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::CastingYard);
//   CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::CastingYard);

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << "Responses - Girder Placement" << rptNewLine;
   p->SetName("Combined Results - Girder Placement");
   CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::GirderPlacement, pSpec->GetAnalysisType());
   CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::GirderPlacement, pSpec->GetAnalysisType());
   CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::GirderPlacement, pSpec->GetAnalysisType());

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   if ( 0 < pStrandGeom->GetMaxStrands(span,girder,pgsTypes::Temporary) )
   {
      // if there can be temporary strands, report the loads at the temporary strand removal stage
      // because this is when the girder load is applied
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << "Responses - Temporary Strand Removal Stage" << rptNewLine;
      p->SetName("Combined Results - Temporary Strand Removal");
      CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::TemporaryStrandRemoval, pSpec->GetAnalysisType());
      CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::TemporaryStrandRemoval, pSpec->GetAnalysisType());
      CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::TemporaryStrandRemoval, pSpec->GetAnalysisType());
   }

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << "Responses - Deck and Diaphragm Placement (Bridge Site 1)" << rptNewLine;
   p->SetName("Combined Results - Deck and Diaphragm Placement (Bridge Site 1)");
   CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::BridgeSite1, pSpec->GetAnalysisType());
   CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::BridgeSite1, pSpec->GetAnalysisType());
   CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::BridgeSite1, pSpec->GetAnalysisType());

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << "Responses - Superimposed Dead Loads (Bridge Site 2)" << rptNewLine;
   p->SetName("Combined Results - Superimposed Dead Loads (Bridge Site 2)");
   CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::BridgeSite2, pSpec->GetAnalysisType());
   CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::BridgeSite2, pSpec->GetAnalysisType());
   CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::BridgeSite2, pSpec->GetAnalysisType());

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << "Responses - Final with Live Load (Bridge Site 3)" << rptNewLine;
   p->SetName("Combined Results - Final with Live Load (Bridge Site 3)");
   CLiveLoadDistributionFactorTable().Build(pChapter,pBroker,span,girder,pDispUnit);
   CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::BridgeSite3, pSpec->GetAnalysisType());
   CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::BridgeSite3, pSpec->GetAnalysisType());
   CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::BridgeSite3, pSpec->GetAnalysisType());

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << "Live Load Reactions without Impact" << rptNewLine;
   p->SetName("Live Load Reactions without Impact");
   CLiveLoadReactionTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::BridgeSite3, pSpec->GetAnalysisType());

   if ( pSpecEntry->GetShearCapacityMethod() == scmVciVcw )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << "Concurrent Shears" << rptNewLine;
      p->SetName("Concurrent Shears");
      CConcurrentShearTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::BridgeSite3, pSpec->GetAnalysisType());
   }

   return pChapter;
}

CChapterBuilder* CMVRChapterBuilder::Clone() const
{
   return new CMVRChapterBuilder;
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
