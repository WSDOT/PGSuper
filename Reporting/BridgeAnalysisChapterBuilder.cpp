///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include <Reporting\BridgeAnalysisChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <Reporting\BridgeAnalysisReportSpecification.h>

#include <Reporting\CastingYardMomentsTable.h>
#include <Reporting\ProductAxialTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ProductShearTable.h>
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductDisplacementsTable.h>
#include <Reporting\ProductRotationTable.h>

#include <Reporting\CombinedAxialTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\CombinedShearTable.h>
#include <Reporting\CombinedReactionTable.h>

#include <Reporting\UserAxialTable.h>
#include <Reporting\UserMomentsTable.h>
#include <Reporting\UserShearTable.h>
#include <Reporting\UserReactionTable.h>
#include <Reporting\UserDisplacementsTable.h>
#include <Reporting\UserRotationTable.h>

#include <Reporting\LiveLoadDistributionFactorTable.h>

#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\VehicularLoadReactionTable.h>
#include <Reporting\CombinedReactionTable.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CBridgeAnalysisChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CBridgeAnalysisChapterBuilder::CBridgeAnalysisChapterBuilder(LPCTSTR strTitle,pgsTypes::AnalysisType analysisType,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_strTitle = strTitle;
   m_AnalysisType = analysisType;
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CBridgeAnalysisChapterBuilder::GetName() const
{
   return m_strTitle.c_str();
}

rptChapter* CBridgeAnalysisChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CBridgeAnalysisReportSpecification* pBridgeAnalysisRptSpec = dynamic_cast<CBridgeAnalysisReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pBridgeAnalysisRptSpec->GetBroker(&pBroker);

   CGirderKey girderKey(pBridgeAnalysisRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,IIntervals,pIntervals);

   // must use a specific girder key to get interval information
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   IntervalIndexType lastIntervalIdx     = pIntervals->GetIntervalCount()-1;

   std::vector<IntervalIndexType> vIntervals(pIntervals->GetSpecCheckIntervals(girderKey));

   GET_IFACE2(pBroker,ISpecification,pSpec);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* p = 0;

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   if ( (m_AnalysisType == pgsTypes::Simple     && analysisType == pgsTypes::Continuous ) || 
        (m_AnalysisType == pgsTypes::Continuous && analysisType == pgsTypes::Simple )     ||
        (m_AnalysisType == pgsTypes::Envelope   && analysisType != pgsTypes::Envelope )
      )
   {
      p = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << p;
      *p << _T("The structural analysis method had changed since this report was created. Analysis results are no longer available. Close this report and re-create it to get analysis results.") << rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(girderKey);

   bool bDesign = pBridgeAnalysisRptSpec->ReportDesignResults();
   bool bRating = pBridgeAnalysisRptSpec->ReportRatingResults();

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedestrian = pProductLoads->HasPedestrianLoad();
   bool bReportAxial = pProductLoads->ReportAxialResults();

   bool bIndicateControllingLoad = true;

   p = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *p << _T("Load Responses - Bridge Site")<<rptNewLine;
   p->SetName(_T("Bridge Site Results"));
   *pChapter << p;

   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
   bool bAreThereUserLoads = pUDL->DoUserLoadsExist(girderKey);

   // Product Axial
   if ( bReportAxial )
   {
      p = new rptParagraph;
      *pChapter << p;
      *p << CProductAxialTable().Build(pBroker,girderKey,m_AnalysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

      if ( bPedestrian )
      {
         *p << _T("$ Pedestrian values are per girder") << rptNewLine;
      }
    
      *p << LIVELOAD_PER_LANE << rptNewLine;
      LiveLoadTableFooter(pBroker,p,girderKey,bDesign,bRating);
       

      if (bAreThereUserLoads)
      {
         std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
         std::vector<IntervalIndexType>::iterator end(vIntervals.end());
         for ( ; iter != end; iter++ )
         {
            IntervalIndexType intervalIdx = *iter;
            if ( pUDL->DoUserLoadsExist(girderKey,intervalIdx) )
            {
               *p << CUserAxialTable().Build(pBroker,girderKey,m_AnalysisType,intervalIdx,pDisplayUnits) << rptNewLine;
            }
         }
      }
   }

   // Product Moments
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductMomentsTable().Build(pBroker,girderKey,m_AnalysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
   {
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;
   }
 
   *p << LIVELOAD_PER_LANE << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girderKey,bDesign,bRating);
    
   if (bAreThereUserLoads)
   {
      std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
      std::vector<IntervalIndexType>::iterator end(vIntervals.end());
      for ( ; iter != end; iter++ )
      {
         IntervalIndexType intervalIdx = *iter;
         if ( pUDL->DoUserLoadsExist(girderKey,intervalIdx) )
         {
            *p << CUserMomentsTable().Build(pBroker,girderKey,m_AnalysisType,intervalIdx,pDisplayUnits) << rptNewLine;
         }
      }
   }

   // Product Shears
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductShearTable().Build(pBroker,girderKey,m_AnalysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
   {
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;
   }

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girderKey,bDesign,bRating);

   if (bAreThereUserLoads)
   {
      std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
      std::vector<IntervalIndexType>::iterator end(vIntervals.end());
      for ( ; iter != end; iter++ )
      {
         IntervalIndexType intervalIdx = *iter;
         if ( pUDL->DoUserLoadsExist(girderKey,intervalIdx) )
         {
            *p << CUserShearTable().Build(pBroker,girderKey,m_AnalysisType,intervalIdx,pDisplayUnits) << rptNewLine;
         }
      }
   }

   // Product Reactions
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductReactionTable().Build(pBroker,girderKey,m_AnalysisType,PierReactionsTable,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
   {
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;
   }

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girderKey,bDesign,bRating);

   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   std::vector<PierIndexType> vPiers = pBearingDesign->GetBearingReactionPiers(lastIntervalIdx,girderKey);

   if ( 0 < vPiers.size() )
   {
      *p << CProductReactionTable().Build(pBroker,girderKey,m_AnalysisType,BearingReactionsTable,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

      if ( bPedestrian )
      {
         *p << _T("$ Pedestrian values are per girder") << rptNewLine;
      }

      *p << LIVELOAD_PER_LANE << rptNewLine;
      *p << rptNewLine;
      LiveLoadTableFooter(pBroker,p,girderKey,bDesign,bRating);
   }

   if (bAreThereUserLoads)
   {
      std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
      std::vector<IntervalIndexType>::iterator end(vIntervals.end());
      for ( ; iter != end; iter++ )
      {
         IntervalIndexType intervalIdx = *iter;
         if ( pUDL->DoUserLoadsExist(girderKey,intervalIdx) )
         {
            *p << CUserReactionTable().Build(pBroker,girderKey,m_AnalysisType,PierReactionsTable,intervalIdx,pDisplayUnits) << rptNewLine;
            *p << CUserReactionTable().Build(pBroker,girderKey,m_AnalysisType,BearingReactionsTable,intervalIdx,pDisplayUnits) << rptNewLine;
         }
      }
   }

   // Product Deflections
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductDeflectionsTable().Build(pBroker,girderKey,m_AnalysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
   {
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;
   }

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girderKey,bDesign,bRating);

   if (bAreThereUserLoads)
   {
      std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
      std::vector<IntervalIndexType>::iterator end(vIntervals.end());
      for ( ; iter != end; iter++ )
      {
         IntervalIndexType intervalIdx = *iter;
         if ( pUDL->DoUserLoadsExist(girderKey,intervalIdx) )
         {
            *p << CUserDeflectionsTable().Build(pBroker,girderKey,m_AnalysisType,intervalIdx,pDisplayUnits) << rptNewLine;
         }
      }
   }

   // Product Rotations
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductRotationTable().Build(pBroker,girderKey,m_AnalysisType,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
   {
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;
   }

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girderKey,bDesign,bRating);

   if (bAreThereUserLoads)
   {
      std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
      std::vector<IntervalIndexType>::iterator end(vIntervals.end());
      for ( ; iter != end; iter++ )
      {
         IntervalIndexType intervalIdx = *iter;
         if ( pUDL->DoUserLoadsExist(girderKey,intervalIdx) )
         {
            *p << CUserRotationTable().Build(pBroker,girderKey,m_AnalysisType,intervalIdx,pDisplayUnits) << rptNewLine;
         }
      }
   }

   // Responses from individual live load vehicules
   std::vector<pgsTypes::LiveLoadType> live_load_types;
   if ( bDesign )
   {
      live_load_types.push_back(pgsTypes::lltDesign);

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
        live_load_types.push_back(pgsTypes::lltFatigue);
      }

      GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
      bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);
      if ( bPermit )
      {
         live_load_types.push_back(pgsTypes::lltPermit);
      }
   }

   if ( bRating )
   {
      GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
      // if lltDesign isn't included because we aren't reporting design and if we are doing Design Inventory or Operating rating
      // then add the lltDesign
      if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
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

      if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
      {
         live_load_types.push_back(pgsTypes::lltLegalRating_Emergency);
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         live_load_types.push_back(pgsTypes::lltPermitRating_Routine);
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         live_load_types.push_back(pgsTypes::lltPermitRating_Special);
      }
   }

   std::vector<pgsTypes::LiveLoadType>::iterator iter;
   for ( iter = live_load_types.begin(); iter != live_load_types.end(); iter++ )
   {
      pgsTypes::LiveLoadType llType = *iter;

      GET_IFACE2( pBroker, IProductLoads, pProductLoads);
      std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,girderKey);

      // nothing to report if there are no loads
      if ( strLLNames.size() == 0 )
      {
         continue;
      }

      // if the only loading is the DUMMY load, then move on
      if ( strLLNames.size() == 1 && strLLNames[0] == std::_tstring(NO_LIVE_LOAD_DEFINED) )
      {
         continue;
      }

      p = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << p;

      if ( llType == pgsTypes::lltDesign )
      {
         p->SetName(_T("Design Live Load Individual Vehicle Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPermit )
      {
         p->SetName(_T("Permit Live Load Individual Vehicle Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltFatigue )
      {
         p->SetName(_T("Fatigue Live Load Individual Vehicle Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPedestrian )
      {
         p->SetName(_T("Pedestrian Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltLegalRating_Routine )
      {
         p->SetName(_T("AASHTO Legal Rating Routine Commercial Vehicle Individual Vehicle Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltLegalRating_Special )
      {
         p->SetName(_T("AASHTO Legal Rating Specialized Hauling Vehicle Individual Vehicle Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if (llType == pgsTypes::lltLegalRating_Emergency)
      {
         p->SetName(_T("Emergency Vehicle Rating Individual Vehicle Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPermitRating_Routine )
      {
         p->SetName(_T("Routine Permit Rating Individual Vehicle Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPermitRating_Special )
      {
         p->SetName(_T("Special Permit Rating Individual Vehicle Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else
      {
         ATLASSERT(false); // is there a new live load type???
      }

      std::vector<std::_tstring>::iterator iter;
      for ( iter = strLLNames.begin(); iter != strLLNames.end(); iter++ )
      {
         std::_tstring strLLName = *iter;

         VehicleIndexType index = iter - strLLNames.begin();

         p = new rptParagraph;
         *pChapter << p;
         p->SetName( strLLName.c_str() );
         *p << CVehicularLoadResultsTable().Build(pBroker,girderKey,llType,strLLName,index,m_AnalysisType,true,pDisplayUnits) << rptNewLine;
         *p << LIVELOAD_PER_LANE << rptNewLine;

         *p << rptNewLine;

         *p << CVehicularLoadReactionTable().Build(pBroker,girderKey,llType,strLLName,index,m_AnalysisType,true,true,pDisplayUnits) << rptNewLine;
         *p << LIVELOAD_PER_LANE << rptNewLine;
      }
   }

   // Load Combinations (DC, DW, etc) & Limit States
   std::vector<IntervalIndexType>::iterator intervalIter(vIntervals.begin());
   std::vector<IntervalIndexType>::iterator intervalIterEnd(vIntervals.end());
   for ( ; intervalIter != intervalIterEnd; intervalIter++ )
   {
      IntervalIndexType intervalIdx = *intervalIter;
      p = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << p;
      CString strName;
      strName.Format(_T("Combined Results - Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
      p->SetName(strName);
      *p << p->GetName();

      if ( liveLoadIntervalIdx <= intervalIdx )
      {
         CLiveLoadDistributionFactorTable().Build(pChapter,pBroker,girderKey,pDisplayUnits);
      }

      if ( bReportAxial )
      {
         CCombinedAxialTable().Build(pBroker,pChapter,girderKey,pDisplayUnits,intervalIdx, analysisType, bDesign, bRating);
      }

      CCombinedMomentsTable().Build(pBroker,pChapter,girderKey,pDisplayUnits,intervalIdx, analysisType, bDesign, bRating);
      CCombinedShearTable().Build(  pBroker,pChapter,girderKey,pDisplayUnits,intervalIdx, analysisType, bDesign, bRating);
      if ( castDeckIntervalIdx <= intervalIdx )
      {
         CCombinedReactionTable().Build(pBroker,pChapter,girderKey,pDisplayUnits,intervalIdx,analysisType,PierReactionsTable, bDesign, bRating);
         if( 0 < vPiers.size() )
         {
            CCombinedReactionTable().Build(pBroker,pChapter,girderKey,pDisplayUnits,intervalIdx,analysisType,BearingReactionsTable, bDesign, bRating);
         }

         if ( liveLoadIntervalIdx <= intervalIdx )
         {
            p = new rptParagraph(rptStyleManager::GetHeadingStyle());
            *pChapter << p;
            *p << _T("Live Load Reactions Without Impact") << rptNewLine;
            p->SetName(_T("Live Load Reactions Without Impact"));
            CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,girderKey,pDisplayUnits,analysisType,PierReactionsTable, false, true, false);
            if( 0 < vPiers.size() )
            {
               CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,girderKey,pDisplayUnits,analysisType,BearingReactionsTable, false, true, false);
            }
         }
      }
   } // next interval
   return pChapter;
}

CChapterBuilder* CBridgeAnalysisChapterBuilder::Clone() const
{
   return new CBridgeAnalysisChapterBuilder(m_strTitle.c_str(),m_AnalysisType);
}
