///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include <Reporting\HorizontalInterfaceShearCapacityDetailsChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\InterfaceShearDetails.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PsgLib\ShearData.h>
#include <PgsExt\BridgeDescription2.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\ShearCapacity.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\RatingSpecification.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\BeamFactory.h>

#include <Reporter\ReportingUtils.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CHorizontalInterfaceShearCapacityDetailsChapterBuilder
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CHorizontalInterfaceShearCapacityDetailsChapterBuilder::CHorizontalInterfaceShearCapacityDetailsChapterBuilder(bool bDesign,bool bRating,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bDesign = bDesign;
   m_bRating = bRating;
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CHorizontalInterfaceShearCapacityDetailsChapterBuilder::GetName() const
{
   return TEXT("Horizontal Interface Shear Capacity Details");
}

rptChapter* CHorizontalInterfaceShearCapacityDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);

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

   GET_IFACE2(pBroker, IBridge, pBridge);
   if (!pBridge->IsCompositeDeck())
   {
      return nullptr;
   }


   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);

   bool bDesign = m_bDesign;
   bool bRating = m_bRating;

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph();
   *pChapter << pPara;

   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount() - 1;
   std::_tstring stage_name(pIntervals->GetDescription(intervalIdx));

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);
   for(const auto& thisGirderKey : vGirderKeys)
   {
      rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;

      bool bPermit = pLimitStateForces->IsStrengthIIApplicable(thisGirderKey);

      // vBasicPoi does not contain CS for shear POI
      PoiList vBasicPoi;
      pPoi->GetPointsOfInterest(CSegmentKey(thisGirderKey, ALL_SEGMENTS), &vBasicPoi);;
      pPoi->RemovePointsOfInterest(vBasicPoi,POI_BOUNDARY_PIER);

      std::vector<pgsTypes::LimitState> vLimitStates;
      if ( bDesign )
      {
         vLimitStates.push_back(pgsTypes::StrengthI);
         if ( bPermit )
         {
            vLimitStates.push_back(pgsTypes::StrengthII);
         }
      }

      if ( bRating )
      {
         GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) && pRatingSpec->RateForShear(pgsTypes::lrDesign_Inventory) )
         {
            vLimitStates.push_back(pgsTypes::StrengthI_Inventory);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) && pRatingSpec->RateForShear(pgsTypes::lrDesign_Operating) )
         {
            vLimitStates.push_back(pgsTypes::StrengthI_Operating);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) && pRatingSpec->RateForShear(pgsTypes::lrLegal_Routine) )
         {
            vLimitStates.push_back(pgsTypes::StrengthI_LegalRoutine);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) && pRatingSpec->RateForShear(pgsTypes::lrLegal_Special) )
         {
            vLimitStates.push_back(pgsTypes::StrengthI_LegalSpecial);
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency) && pRatingSpec->RateForShear(pgsTypes::lrLegal_Emergency))
         {
            vLimitStates.push_back(pgsTypes::StrengthI_LegalEmergency);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) && pRatingSpec->RateForShear(pgsTypes::lrPermit_Routine) )
         {
            vLimitStates.push_back(pgsTypes::StrengthII_PermitRoutine);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) && pRatingSpec->RateForShear(pgsTypes::lrPermit_Special) )
         {
            vLimitStates.push_back(pgsTypes::StrengthII_PermitSpecial);
         }
      }

      for (const auto& limitState : vLimitStates)
      {
         PoiList vCSPoi;
         pPoi->GetCriticalSections(limitState, thisGirderKey,&vCSPoi);
         PoiList vPoi;
         pPoi->MergePoiLists(vBasicPoi, vCSPoi,&vPoi); // merge, sort, and remove duplicates

         if (1 < vLimitStates.size())
         {
            pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
            pPara->SetName(GetLimitStateString(limitState));
            *pPara << pPara->GetName() << rptNewLine;
            *pChapter << pPara;
         }
         CInterfaceShearDetails(pDisplayUnits).Build(pBroker, pChapter, thisGirderKey, pDisplayUnits, intervalIdx, limitState);
      }
   } // next group

   return pChapter;
}

CChapterBuilder* CHorizontalInterfaceShearCapacityDetailsChapterBuilder::Clone() const
{
   return new CHorizontalInterfaceShearCapacityDetailsChapterBuilder(m_bDesign,m_bRating);
}
