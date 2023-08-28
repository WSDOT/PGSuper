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
#include <Reporting\CritSectionChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\ShearCapacity.h>
#include <IFace\Bridge.h>
#include <IFace\RatingSpecification.h>
#include <IFace\AnalysisResults.h>
#include <IFace\DocumentType.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CCritSectionChapterBuilder
****************************************************************************/

CCritSectionChapterBuilder::CCritSectionChapterBuilder(bool bDesign,bool bRating,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bDesign = bDesign;
   m_bRating = bRating;
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CCritSectionChapterBuilder::GetName() const
{
   return TEXT("Critical Section for Shear Details");
}

rptChapter* CCritSectionChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
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

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2_NOCHECK(pBroker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(pBroker, IDocumentType, pDocType);
   bool isPGSuper = pDocType->IsPGSuperDocument();

   bool bDesign = m_bDesign;
   bool bRating = m_bRating;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfterThirdEdition = ( pSpecEntry->GetSpecificationType() >= WBFL::LRFD::LRFDVersionMgr::Version::ThirdEdition2004 ? true : false );

   GET_IFACE2_NOCHECK(pBroker,ILimitStateForces,pLimitStateForces); // not used if bDesign = false
   GET_IFACE2(pBroker,IBridge,pBridge);
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);
   for(const auto& thisGirderKey : vGirderKeys)
   {
      rptParagraph* pPara;
      if ( girderKey.groupIndex == ALL_GROUPS )
      {
         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;
         std::_tostringstream os;
         if (isPGSuper)
         {
            os << _T("Span ") << LABEL_SPAN(thisGirderKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(thisGirderKey.girderIndex);
         }
         else
         {
            os << _T("Group ") << LABEL_GROUP(thisGirderKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(thisGirderKey.girderIndex);
         }

         pPara->SetName( os.str().c_str() );
         (*pPara) << pPara->GetName() << rptNewLine;
      }

      if ( bDesign )
      {
         Build(pChapter,pgsTypes::StrengthI,pBroker,thisGirderKey,pDisplayUnits,level);

         if ( pLimitStateForces->IsStrengthIIApplicable(thisGirderKey) && !bAfterThirdEdition )
         {
            Build(pChapter,pgsTypes::StrengthII,pBroker,thisGirderKey,pDisplayUnits,level);
         }
      }

      if ( bRating )
      {
         if ( bAfterThirdEdition )
         {
            Build(pChapter,pgsTypes::StrengthI,pBroker,thisGirderKey,pDisplayUnits,level);
         }
         else
         {
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            {
               Build(pChapter,pgsTypes::StrengthI_Inventory,pBroker,thisGirderKey,pDisplayUnits,level);
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
            {
               Build(pChapter,pgsTypes::StrengthI_Operating,pBroker,thisGirderKey,pDisplayUnits,level);
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               Build(pChapter,pgsTypes::StrengthI_LegalRoutine,pBroker,thisGirderKey,pDisplayUnits,level);
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               Build(pChapter,pgsTypes::StrengthI_LegalSpecial,pBroker,thisGirderKey,pDisplayUnits,level);
            }

            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
            {
               Build(pChapter, pgsTypes::StrengthI_LegalEmergency, pBroker, thisGirderKey, pDisplayUnits, level);
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               Build(pChapter,pgsTypes::StrengthII_PermitRoutine,pBroker,thisGirderKey,pDisplayUnits,level);
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               Build(pChapter,pgsTypes::StrengthII_PermitSpecial,pBroker,thisGirderKey,pDisplayUnits,level);
            }
         }
      }
   }

   return pChapter;
}

void CCritSectionChapterBuilder::Build(rptChapter* pChapter,pgsTypes::LimitState limitState,IBroker* pBroker,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const
{
   USES_CONVERSION;

   INIT_UV_PROTOTYPE( rptPointOfInterest,         locationp, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue,      location,  pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue,      dim,       pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptAngleSectionValue,       ang,       pDisplayUnits->GetAngleUnit(),  false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   locationp.IncludeSpanAndGirder(nSegments > 1);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfterThirdEdition = ( WBFL::LRFD::LRFDVersionMgr::Version::ThirdEdition2004 <= pSpecEntry->GetSpecificationType() ? true : false );

   GET_IFACE2(pBroker,IShearCapacity,pShearCapacity);
   const std::vector<CRITSECTDETAILS>& vcsDetails(pShearCapacity->GetCriticalSectionDetails(limitState,girderKey));

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T("Limit State: ") << GetLimitStateString(limitState) << rptNewLine;

   ColumnIndexType nColumns;
   if ( bAfterThirdEdition )
   {
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Critical Section Picture 2004.jpg")) << rptNewLine;
      *pPara << _T("LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.7.3.2"),_T("5.8.3.2"))<<rptNewLine;
      *pPara << _T("Critical Section = d") << Sub(_T("v")) << _T(" measured at d") << Sub(_T("v")) << _T(" from the face of support") << rptNewLine;
      nColumns = 4;
   }
   else
   {
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Critical Section Picture.jpg")) << rptNewLine;
      *pPara << _T("LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.7.3.2"),_T("5.8.3.2"))<<rptNewLine;
      *pPara << _T("Critical Section = max(CS1, CS2)") << rptNewLine;
      *pPara << _T("CS1 = d")<<Sub(_T("v")) << rptNewLine;
      *pPara << _T("CS2 = 0.5 cot(")<<symbol(theta)<<_T(") d")<<Sub(_T("v")) << rptNewLine;
      nColumns = 7;
   }

   std::vector<CRITSECTDETAILS>::const_iterator csIter(vcsDetails.begin());
   std::vector<CRITSECTDETAILS>::const_iterator csIterEnd(vcsDetails.end());
   for ( ; csIter != csIterEnd; csIter++ )
   {
      const CRITSECTDETAILS& csDetails(*csIter);


      rptRcTable* ptable = rptStyleManager::CreateDefaultTable(nColumns);
      *pPara << ptable;
      ptable->TableLabel() << _T("Critical Section Calculation for ") << (csDetails.PierFace == pgsTypes::Back ? _T("Back") : _T("Ahead")) 
                           << _T(" side of Pier ") << LABEL_PIER(csDetails.PierIdx);
     
      if ( girderKey.groupIndex == ALL_GROUPS )
      {
         ptable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
         ptable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      }

      (*ptable)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*ptable)(0,1)  << COLHDR(_T("Distance from")<<rptNewLine<<_T("Face of Support"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*ptable)(0,2)  << COLHDR(_T("d")<<Sub(_T("v")) , rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      if ( bAfterThirdEdition )
      {
         (*ptable)(0,3)  << _T("CS")<<rptNewLine<<_T("Intersects?");
      }
      else
      {
         (*ptable)(0,3)  << _T("CS1")<<rptNewLine<<_T("Intersects?");
         (*ptable)(0,4)  << COLHDR(symbol(theta),  rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );
         (*ptable)(0,5)  << COLHDR(_T("0.5 cot(")<<symbol(theta)<<_T(") d")<<Sub(_T("v")),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         (*ptable)(0,6)  << _T("CS2")<<rptNewLine<<_T("Intersects?");
      }

      RowIndexType row = ptable->GetNumberOfHeaderRows();
      bool all_in_range = true;

      std::vector<CRITSECTIONDETAILSATPOI>::const_iterator iter(csDetails.PoiData.begin());
      std::vector<CRITSECTIONDETAILSATPOI>::const_iterator iterEnd(csDetails.PoiData.end());
      for ( ; iter != iterEnd; iter++ )
      {
         const CRITSECTIONDETAILSATPOI& csDetailsAtPoi = *iter;

         (*ptable)(row,0) << locationp.SetValue( POI_ERECTED_SEGMENT, csDetailsAtPoi.Poi );

         (*ptable)(row,1) << dim.SetValue(csDetailsAtPoi.DistFromFOS);
         (*ptable)(row,2) << dim.SetValue(csDetailsAtPoi.Dv);

         if (csDetailsAtPoi.Intersection == CRITSECTIONDETAILSATPOI::DvIntersection)
         {
            (*ptable)(row,3) << _T("*Yes");
         }
         else
         {
            (*ptable)(row,3) << _T("No");
         }

         if ( !bAfterThirdEdition )
         {
            if (csDetailsAtPoi.InRange)
            {
               (*ptable)(row,4) << ang.SetValue(csDetailsAtPoi.Theta);
               (*ptable)(row,5) << dim.SetValue(csDetailsAtPoi.CotanThetaDv05);

               if (csDetailsAtPoi.Intersection == CRITSECTIONDETAILSATPOI::ThetaIntersection)
               {
                  (*ptable)(row,6) << _T("*Yes");
               }
               else
               {
                  (*ptable)(row,6) << _T("No");
               }
            }
            else
            {
               all_in_range=false;
               (*ptable)(row,4) <<_T("**");
               (*ptable)(row,5) <<_T("**");
               (*ptable)(row,6) <<_T("**");
            }
         }

         row++;
      } // next CSDetailsAtPoi

      *pPara << _T("* - Intersection values are linearly interpolated") <<rptNewLine;
      if (!all_in_range)
      {
         *pPara << _T("** - Theta could not be calculated because shear stress exceeded max.")<<rptNewLine<<rptNewLine;
      }

      if ( csDetails.bAtFaceOfSupport )
      {
         *pPara << _T("Critical Section is at ") << location.SetValue(0.0) << _T(" ") << location.GetUnitTag()
                << _T(" (") << dim.SetValue(0.0) << _T(" ") << dim.GetUnitTag() << _T(")")
                << _T(" from the face of support on the ") << (csDetails.PierFace == pgsTypes::Back ? _T("Back") : _T("Ahead")) 
                << _T(" side of Pier ") << LABEL_PIER(csDetails.PierIdx) << rptNewLine;
      }
      else
      {
         *pPara << _T("Critical Section is at ") << location.SetValue(csDetails.pCriticalSection->DistFromFOS) << _T(" ") << location.GetUnitTag()
                << _T(" (") << dim.SetValue(csDetails.pCriticalSection->DistFromFOS) << _T(" ") << dim.GetUnitTag() << _T(")")
                << _T(" from the face of support on the ") << (csDetails.PierFace == pgsTypes::Back ? _T("Back") : _T("Ahead")) 
                << _T(" side of Pier ") << LABEL_PIER(csDetails.PierIdx) << rptNewLine;
      }

      *pPara << rptNewLine;
   } // next CS
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CCritSectionChapterBuilder::Clone() const
{
   return std::make_unique<CCritSectionChapterBuilder>(m_bDesign,m_bRating);
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
