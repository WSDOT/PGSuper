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
#include <Reporting\CritSectionChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>

#include <IFace\DisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\ShearCapacity.h>
#include <IFace\Bridge.h>

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
CCritSectionChapterBuilder::CCritSectionChapterBuilder()
{
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
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType gdr = pSGRptSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);
   Build(pChapter,pgsTypes::StrengthI,pBroker,span,gdr,pDisplayUnits,level);

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   if ( pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit) )
      Build(pChapter,pgsTypes::StrengthII,pBroker,span,gdr,pDisplayUnits,level);

   return pChapter;
}

void CCritSectionChapterBuilder::Build(rptChapter* pChapter,pgsTypes::LimitState limitState,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDisplayUnits,Uint16 level) const
{

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bThirdEdition = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::ThirdEdition2004 ? true : false );


   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   if ( limitState == pgsTypes::StrengthI )
      *pPara << "Strength I Limit State" << rptNewLine;
   else
      *pPara << "Strength II Limit State" << rptNewLine;

   *pChapter << pPara;

   pPara = new rptParagraph;
   *pChapter << pPara;

   ColumnIndexType nColumns;
   if ( bThirdEdition )
   {
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Critical Section Picture 2004.jpg") << rptNewLine;
      *pPara << "LRFD 5.8.3.2"<<rptNewLine;
      *pPara << "Critical Section = d" << Sub("v") << " measured at d" << Sub("v") << " from the face of support" << rptNewLine;
      nColumns = 4;
   }
   else
   {
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Critical Section Picture.jpg") << rptNewLine;
      *pPara << "LRFD 5.8.3.2"<<rptNewLine;
      *pPara << "Critical Section = max(CS1, CS2)" << rptNewLine;
      *pPara << "CS1 = d"<<Sub("v") << rptNewLine;
      *pPara << "CS2 = 0.5 cot("<<symbol(theta)<<") d"<<Sub("v") << rptNewLine;
      nColumns = 7;
   }

   rptRcTable* ptable = pgsReportStyleHolder::CreateDefaultTable(nColumns," ");
   *pPara << ptable;
   ptable->TableLabel() << "Critical Section Calculation";
  
   (*ptable)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*ptable)(0,1)  << COLHDR("Assumed C.S."<<rptNewLine<<"Location", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*ptable)(0,2)  << COLHDR("d"<<Sub("v") , rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   if ( bThirdEdition )
   {
      (*ptable)(0,3)  << "CS"<<rptNewLine<<"Intersects?";
   }
   else
   {
      (*ptable)(0,3)  << "CS1"<<rptNewLine<<"Intersects?";
      (*ptable)(0,4)  << COLHDR(symbol(theta),  rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );
      (*ptable)(0,5)  << COLHDR("0.5 cot("<<symbol(theta)<<") d"<<Sub("v"),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*ptable)(0,6)  << "CS2"<<rptNewLine<<"Intersects?";
   }

   INIT_UV_PROTOTYPE( rptPointOfInterest,         locationp, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue,      location,  pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue,      dim,       pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptAngleSectionValue,       ang,       pDisplayUnits->GetAngleUnit(),  false );

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

   if ( !bThirdEdition )
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
      (*ptable)(row,0) << locationp.SetValue( poi, end_size );

      (*ptable)(row,1) << dim.SetValue(new_loc);
      (*ptable)(row,2) << dim.SetValue(pdetp->Dv);

      if (lp.Wart==LocPair::DvIntersection)
         (*ptable)(row,3) << "*Yes";
      else
         (*ptable)(row,3) << "No";

      if ( !bThirdEdition )
      {
         if (pdetp->InRange)
         {
            (*ptable)(row,4) << ang.SetValue(pdetp->Theta);
            (*ptable)(row,5) << dim.SetValue(pdetp->CotanThetaDv05);

            if (lp.Wart==LocPair::ThetaIntersection)
               (*ptable)(row,6) << "*Yes";
            else
               (*ptable)(row,6) << "No";
         }
         else
         {
            all_in_range=false;
            (*ptable)(row,4) <<"**";
            (*ptable)(row,5) <<"**";
            (*ptable)(row,6) <<"**";
         }
      }

      row++;
   }

   *pPara << "* - Intersection values are linearly interpolated" <<rptNewLine;
   if (!all_in_range)
      *pPara << "** - Theta could not be calculated because shear stress exceeded max."<<rptNewLine<<rptNewLine;

   Float64 left_cs, right_cs;
   pShearCapacity->GetCriticalSection(limitState,span,gdr,&left_cs,&right_cs);
   *pPara << "Location of Left  C.S. is "<<location.SetValue(left_cs-end_size)<<" "<<location.GetUnitTag()<<" from left support"<<rptNewLine;
   *pPara << "Location of Right C.S. is "<<location.SetValue(right_cs-end_size)<<" "<<location.GetUnitTag()<<" from left support"<<rptNewLine;
}

CChapterBuilder* CCritSectionChapterBuilder::Clone() const
{
   return new CCritSectionChapterBuilder;
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
