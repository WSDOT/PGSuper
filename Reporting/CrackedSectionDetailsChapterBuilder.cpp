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

#include <Reporting\CrackedSectionDetailsChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\PointOfInterest.h>

#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
#include <IFace\CrackedSection.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CCrackedSectionDetailsChapterBuilder
****************************************************************************/

void write_cracked_section_table(IBroker* pBroker,
                             IDisplayUnits* pDisplayUnits,
                             SpanIndexType span,
                             GirderIndexType gdr,
                             const std::vector<pgsPointOfInterest>& pois,
                             rptChapter* pChapter,
                             bool bPositiveMoment);

CCrackedSectionDetailsChapterBuilder::CCrackedSectionDetailsChapterBuilder()
{
}

LPCTSTR CCrackedSectionDetailsChapterBuilder::GetName() const
{
   return TEXT("Cracked Section Analysis Details");
}

rptChapter* CCrackedSectionDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   bool bRateRoutine = pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine);
   bool bRateSpecial = pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special);

   if ( !bRateRoutine && !bRateSpecial )
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << "Cracked section analysis not performed" << rptNewLine;
      return pChapter;
   }
   else
   {
      if ( !pRatingSpec->RateForStress(pgsTypes::lrPermit_Routine) && !pRatingSpec->RateForStress(pgsTypes::lrPermit_Special) )
      {
         rptParagraph* pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara << "Cracked section analysis not performed" << rptNewLine;
         return pChapter;
      }
   }

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   *pChapter << pPara;
   *pPara << "Positive Moment Cracked Section Analysis Details" << rptNewLine;


   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPoi;

   vPoi = pIPOI->GetPointsOfInterest(pgsTypes::BridgeSite3, span, girder, POI_FLEXURECAPACITY | POI_SHEAR, POIFIND_OR );
   write_cracked_section_table(pBroker,pDisplayUnits,span,girder, vPoi, pChapter,true);

   if ( pBridge->ProcessNegativeMoments(span) )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      *pChapter << pPara;
      *pPara << "Negative Moment Cracked Section Analysis Details" << rptNewLine;

      write_cracked_section_table(pBroker,pDisplayUnits,span,girder, vPoi, pChapter, false);
   }

   return pChapter;
}

CChapterBuilder* CCrackedSectionDetailsChapterBuilder::Clone() const
{
   return new CCrackedSectionDetailsChapterBuilder;
}

void write_cracked_section_table(IBroker* pBroker,
                             IDisplayUnits* pDisplayUnits,
                             SpanIndexType span,
                             GirderIndexType gdr,
                             const std::vector<pgsPointOfInterest>& pois,
                             rptChapter* pChapter,
                             bool bPositiveMoment)
{
   rptParagraph* pPara = new rptParagraph();
   *pChapter << pPara;

   GET_IFACE2(pBroker, IBridge,            pBridge);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   // Setup the table
   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(4,"");
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pPara << table << rptNewLine;

   ColumnIndexType col = 0;

   (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << COLHDR(Sub2("Y","t"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(Sub2("Y","b"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(Sub2("I","cr"), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest,  location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, mom_i,    pDisplayUnits->GetMomentOfInertiaUnit(),       false );

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,ICrackedSection,pCrackedSection);
   GET_IFACE2(pBroker,ISectProp2,pSectProp);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      CRACKEDSECTIONDETAILS csd;
      pCrackedSection->GetCrackedSectionDetails(poi,bPositiveMoment,&csd);

      Float64 h = pSectProp->GetHg(pgsTypes::BridgeSite3,poi);

      Float64 Yt = csd.c;
      Float64 Yb = h - Yt;

      col = 0;

      (*table)(row,col++) << location.SetValue( poi, end_size );
      (*table)(row,col++) << dim.SetValue( Yt );
      (*table)(row,col++) << dim.SetValue( Yb );
      (*table)(row,col++) << mom_i.SetValue( csd.Icr );

      row++;
   }
}
