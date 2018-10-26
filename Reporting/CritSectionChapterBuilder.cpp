///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#include <PgsExt\PointOfInterest.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\ShearCapacity.h>
#include <IFace\Bridge.h>
#include <IFace\RatingSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CCritSectionChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////
// struct for sorting results
struct LocPair
{
   enum LocWart {DvIntersection, ThetaIntersection, NoIntersection};
   Float64 Loc;
   LocWart Wart;
   bool operator<(const LocPair& x)const{return Loc<x.Loc;}
};

//======================== LIFECYCLE  =======================================
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
   return TEXT("Critical Section For Shear Details");
}

rptChapter* CCritSectionChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CGirderReportSpecification* pGdrRptSpec    = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   SpanIndexType span;
   GirderIndexType gdr;

   if ( pSGRptSpec )
   {
      pSGRptSpec->GetBroker(&pBroker);
      span = pSGRptSpec->GetSpan();
      gdr = pSGRptSpec->GetGirder();
   }
   else if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      span = ALL_SPANS;
      gdr = pGdrRptSpec->GetGirder();
   }
   else
   {
      span = ALL_SPANS;
      gdr  = ALL_GIRDERS;
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   bool bDesign = m_bDesign;
   bool bRating;
   
   if ( m_bRating )
   {
      bRating = true;
   }
   else
   {
      // include load rating results if we are always load rating
      bRating = pRatingSpec->AlwaysLoadRate();


      // if none of the rating types are enabled, skip the rating
      if ( !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) 
         )
         bRating = false;
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfterThirdEdition = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::ThirdEdition2004 ? true : false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   SpanIndexType firstSpanIdx = (span == ALL_SPANS ? 0 : span);
   SpanIndexType lastSpanIdx  = (span == ALL_SPANS ? nSpans : firstSpanIdx+1);
   for ( SpanIndexType spanIdx = firstSpanIdx; spanIdx < lastSpanIdx; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType firstGirderIdx = min(nGirders-1,(gdr == ALL_GIRDERS ? 0 : gdr));
      GirderIndexType lastGirderIdx  = min(nGirders,  (gdr == ALL_GIRDERS ? nGirders : firstGirderIdx + 1));
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx < lastGirderIdx; gdrIdx++ )
      {
         rptParagraph* pPara;
         if ( span == ALL_SPANS || gdr == ALL_GIRDERS )
         {
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;
            std::_tostringstream os;
            os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx);
            pPara->SetName( os.str().c_str() );
            (*pPara) << pPara->GetName() << rptNewLine;
         }

         if ( bDesign )
         {
            Build(pChapter,pgsTypes::StrengthI,pBroker,spanIdx,gdrIdx,pDisplayUnits,level);

            if ( pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit) && !bAfterThirdEdition )
               Build(pChapter,pgsTypes::StrengthII,pBroker,spanIdx,gdrIdx,pDisplayUnits,level);
         }

         if ( bRating )
         {
            if ( bAfterThirdEdition )
            {
               Build(pChapter,pgsTypes::StrengthI,pBroker,spanIdx,gdrIdx,pDisplayUnits,level);
            }
            else
            {
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
                  Build(pChapter,pgsTypes::StrengthI_Inventory,pBroker,spanIdx,gdrIdx,pDisplayUnits,level);

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
                  Build(pChapter,pgsTypes::StrengthI_Operating,pBroker,spanIdx,gdrIdx,pDisplayUnits,level);

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
                  Build(pChapter,pgsTypes::StrengthI_LegalRoutine,pBroker,spanIdx,gdrIdx,pDisplayUnits,level);

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
                  Build(pChapter,pgsTypes::StrengthI_LegalSpecial,pBroker,spanIdx,gdrIdx,pDisplayUnits,level);

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
                  Build(pChapter,pgsTypes::StrengthII_PermitRoutine,pBroker,spanIdx,gdrIdx,pDisplayUnits,level);

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
                  Build(pChapter,pgsTypes::StrengthII_PermitSpecial,pBroker,spanIdx,gdrIdx,pDisplayUnits,level);
            }
         }
      }
   }

   return pChapter;
}

void CCritSectionChapterBuilder::Build(rptChapter* pChapter,pgsTypes::LimitState limitState,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const
{
   USES_CONVERSION;
   GET_IFACE2(pBroker,IStageMap,pStageMap);
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfterThirdEdition = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::ThirdEdition2004 ? true : false );


   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   if ( !bAfterThirdEdition )
   {
      *pPara << OLE2T(pStageMap->GetLimitStateName(limitState));
   }

   *pChapter << pPara;

   pPara = new rptParagraph;
   *pChapter << pPara;

   ColumnIndexType nColumns;
   if ( bAfterThirdEdition )
   {
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Critical Section Picture 2004.jpg")) << rptNewLine;
      *pPara << _T("LRFD 5.8.3.2")<<rptNewLine;
      *pPara << _T("Critical Section = d") << Sub(_T("v")) << _T(" measured at d") << Sub(_T("v")) << _T(" from the face of support") << rptNewLine;
      nColumns = 4;
   }
   else
   {
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Critical Section Picture.jpg")) << rptNewLine;
      *pPara << _T("LRFD 5.8.3.2")<<rptNewLine;
      *pPara << _T("Critical Section = max(CS1, CS2)") << rptNewLine;
      *pPara << _T("CS1 = d")<<Sub(_T("v")) << rptNewLine;
      *pPara << _T("CS2 = 0.5 cot(")<<symbol(theta)<<_T(") d")<<Sub(_T("v")) << rptNewLine;
      nColumns = 7;
   }

   rptRcTable* ptable = pgsReportStyleHolder::CreateDefaultTable(nColumns,_T(" "));
   *pPara << ptable;
   ptable->TableLabel() << _T("Critical Section Calculation");
  
   if ( span == ALL_SPANS )
   {
      ptable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      ptable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   (*ptable)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*ptable)(0,1)  << COLHDR(_T("Assumed C.S.")<<rptNewLine<<_T("Location"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
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

   INIT_UV_PROTOTYPE( rptPointOfInterest,         locationp, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue,      location,  pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue,      dim,       pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptAngleSectionValue,       ang,       pDisplayUnits->GetAngleUnit(),  false );

   locationp.IncludeSpanAndGirder(span == ALL_SPANS);

   // Fill up the table
   GET_IFACE2(pBroker,IShearCapacity,pShearCapacity);
   CRITSECTDETAILS det;
   pShearCapacity->GetCriticalSectionDetails(limitState,span,gdr,&det);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   Float64 end_size2 = pBridge->GetGirderEndConnectionLength(span,gdr);

   Float64 start_support_width = pBridge->GetGirderStartSupportWidth(span,gdr);
   Float64 end_support_width   = pBridge->GetGirderEndSupportWidth(span,gdr);

   Float64 girder_length = pBridge->GetGirderLength(span,gdr);

   // use a map to sort intersection values
   // use a wart to determine the type of poi for the report
   std::multimap<LocPair, const CRITSECTIONDETAILSATPOI*> sorted_list;

   // first put in poi values
   LocPair mylp;
   mylp.Wart = LocPair::NoIntersection;
   std::list<CRITSECTIONDETAILSATPOI>::iterator i;
   for ( i = det.PoiData.begin(); i != det.PoiData.end(); i++ )
   {
      const CRITSECTIONDETAILSATPOI& detp = (*i);
      const pgsPointOfInterest& poi = detp.Poi;
      mylp.Loc = poi.GetDistFromStart();

      sorted_list.insert(std::make_pair(mylp, &detp));
   }

   if ( !bAfterThirdEdition )
   {
      // next put in intersection values - give them a wart so we can catch them
      // on the way out.
      // 0.5cot(theta)dv intersections
      if (!det.bAtLeftFaceOfSupport && det.LeftCsDvt.InRange)
      {
         mylp.Wart = LocPair::ThetaIntersection;
         mylp.Loc  = det.LeftCsDvt.Poi.GetDistFromStart();
         sorted_list.insert(std::make_pair(mylp, &(det.LeftCsDvt)));
      }

      if (!det.bAtRightFaceOfSupport && det.RightCsDvt.InRange)
      {
         mylp.Loc  = det.RightCsDvt.Poi.GetDistFromStart();
         sorted_list.insert(std::make_pair(mylp, &(det.RightCsDvt)));
      }
   }

   // dv intersections
   if ( !det.bAtLeftFaceOfSupport )
   {
      mylp.Wart = LocPair::DvIntersection;
      mylp.Loc  = det.LeftCsDv.Poi.GetDistFromStart();
      sorted_list.insert(std::make_pair(mylp, &(det.LeftCsDv)));
   }

   if ( !det.bAtRightFaceOfSupport )
   {
      mylp.Wart = LocPair::DvIntersection;
      mylp.Loc  = det.RightCsDv.Poi.GetDistFromStart();
      sorted_list.insert(std::make_pair(mylp, &(det.RightCsDv)));
   }

   bool all_in_range=true;
   RowIndexType row = ptable->GetNumberOfHeaderRows();

   std::multimap<LocPair, const CRITSECTIONDETAILSATPOI*>::const_iterator sli;
   for ( sli = sorted_list.begin(); sli != sorted_list.end(); sli++ )
   {
      const CRITSECTIONDETAILSATPOI* pdetp = (*sli).second;
      const pgsPointOfInterest& poi = pdetp->Poi;

      Float64 new_loc;
      if ( poi.GetDistFromStart() < girder_length/2 )
         new_loc = poi.GetDistFromStart()-end_size-start_support_width/2;
      else
         new_loc = (girder_length - poi.GetDistFromStart())-end_size2-end_support_width/2;

      const LocPair& lp = (*sli).first;

      // insert intersection values into
      (*ptable)(row,0) << locationp.SetValue( pgsTypes::BridgeSite3, poi, end_size );

      (*ptable)(row,1) << dim.SetValue(new_loc);
      (*ptable)(row,2) << dim.SetValue(pdetp->Dv);

      if (lp.Wart==LocPair::DvIntersection)
         (*ptable)(row,3) << _T("*Yes");
      else
         (*ptable)(row,3) << _T("No");

      if ( !bAfterThirdEdition )
      {
         if (pdetp->InRange)
         {
            (*ptable)(row,4) << ang.SetValue(pdetp->Theta);
            (*ptable)(row,5) << dim.SetValue(pdetp->CotanThetaDv05);

            if (lp.Wart==LocPair::ThetaIntersection)
               (*ptable)(row,6) << _T("*Yes");
            else
               (*ptable)(row,6) << _T("No");
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
   }

   *pPara << _T("* - Intersection values are linearly interpolated") <<rptNewLine;
   if (!all_in_range)
      *pPara << _T("** - Theta could not be calculated because shear stress exceeded max.")<<rptNewLine<<rptNewLine;

   Float64 left_cs, right_cs;
   pShearCapacity->GetCriticalSection(limitState,span,gdr,&left_cs,&right_cs);
   *pPara << _T("Location of Left  C.S. is ")<<location.SetValue(left_cs-end_size)<<_T(" ")<<location.GetUnitTag()<<_T(" from left support")<<rptNewLine;
   *pPara << _T("Location of Right C.S. is ")<<location.SetValue(right_cs-end_size)<<_T(" ")<<location.GetUnitTag()<<_T(" from left support")<<rptNewLine;
}

CChapterBuilder* CCritSectionChapterBuilder::Clone() const
{
   return new CCritSectionChapterBuilder(m_bDesign,m_bRating);
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
