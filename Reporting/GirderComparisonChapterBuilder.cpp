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
#include <Reporting\GirderComparisonChapterBuilder.h>
#include <Reporting\StirrupTable.h>

#include <IFace\DisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\GirderHandling.h>

#include <Material\PsStrand.h>

#include <PgsExt\BridgeDescription.h>
#include <PgsExt\GirderData.h>

#include <Lrfd\RebarPool.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CGirderComparisonChapterBuilder
****************************************************************************/


void girders(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDispUnit,SpanIndexType span);
void prestressing(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDispUnits,SpanIndexType span);
void material(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDispUnits,SpanIndexType span);
void stirrups(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDispUnits,SpanIndexType span);
void handling(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDispUnits,SpanIndexType span);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CGirderComparisonChapterBuilder::CGirderComparisonChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CGirderComparisonChapterBuilder::GetName() const
{
   return TEXT("Girder Comparison");
}

                                               
rptChapter* CGirderComparisonChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanReportSpecification* pSpec = dynamic_cast<CSpanReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);
   SpanIndexType span = pSpec->GetSpan();

   GET_IFACE2(pBroker,IDisplayUnits,pDispUnit);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   girders(pChapter,pBroker,pDispUnit,span);
   prestressing(pChapter,pBroker,pDispUnit,span);
   material(pChapter,pBroker,pDispUnit,span);
   stirrups(pChapter,pBroker,pDispUnit,span);
   handling(pChapter,pBroker,pDispUnit,span);

   return pChapter;
}

CChapterBuilder* CGirderComparisonChapterBuilder::Clone() const
{
   return new CGirderComparisonChapterBuilder;
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

void girders(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDispUnit,SpanIndexType span)
{
   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<"Girder Types for Span "<< LABEL_SPAN(span) << rptNewLine;

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(2,"");
   *pPara << p_table<<rptNewLine;

   int col = 0;
   (*p_table)(0,col++) << "Girder";
   (*p_table)(0,col++) << "Type";


   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   GirderIndexType nGirders = pSpan->GetGirderCount();
   int row=1;
   for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
   {
      std::string strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdrIdx);

      col = 0;
      (*p_table)(row,col++) << LABEL_GIRDER(gdrIdx);
      (*p_table)(row,col++) << strGirderName;
      row++;
   }
}

void prestressing(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDispUnit,SpanIndexType span)
{
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   SpanIndexType nspans = pBridge->GetSpanCount();
   CHECK(span<nspans);

   bool bTempStrands = (0 < pStrandGeometry->GetMaxStrands(span,0,pgsTypes::Temporary) ? true : false);


   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<"Prestressing Strands for Span "<< LABEL_SPAN(span) <<rptNewLine;


   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,          pDispUnit->GetShearUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDispUnit->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDispUnit->GetStressUnit(),        false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(bTempStrands ? 10 : 8,"");
   *pPara << p_table<<rptNewLine;

   p_table->SetNumberOfHeaderRows(2);

   ColumnIndexType col1 = 0;
   ColumnIndexType col2 = 0;
   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++) << "Girder";
   p_table->SetRowSpan(1,col2++,-1);

   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++) << "Material";
   p_table->SetRowSpan(1,col2++,-1);

   ColumnIndexType nSkipCols = 1;
   p_table->SetColumnSpan(0,col1,2);
   (*p_table)(0,col1++) << "Straight Strands";
   (*p_table)(1,col2++) << "#";
   (*p_table)(1,col2++) << COLHDR(Sub2("P","jack"),rptForceUnitTag,pDispUnit->GetGeneralForceUnit());

   p_table->SetColumnSpan(0,col1,2);
   (*p_table)(0,col1++) << "Harped Strands";
   (*p_table)(1,col2++) << "#";
   (*p_table)(1,col2++) << COLHDR(Sub2("P","jack"),rptForceUnitTag,pDispUnit->GetGeneralForceUnit());
   nSkipCols++;

   if ( bTempStrands )
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << "Temporary Strands";
      (*p_table)(1,col2++) << "#";
      (*p_table)(1,col2++) << COLHDR(Sub2("P","jack"),rptForceUnitTag,pDispUnit->GetGeneralForceUnit());
      nSkipCols++;
   }

   for ( ColumnIndexType i = 0; i < nSkipCols; i++ )
   {
      p_table->SetColumnSpan(0,col1+i,-1);
   }
   col1 += nSkipCols;


   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++) << COLHDR("Girder End"<<rptNewLine<<"Offset",rptLengthUnitTag, pDispUnit->GetComponentDimUnit() );
   p_table->SetRowSpan(1,col2++,-1);

   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++)<< COLHDR("Harping Pt"<<rptNewLine<<"Offset",rptLengthUnitTag, pDispUnit->GetComponentDimUnit() );
   p_table->SetRowSpan(1,col2++,-1);


   GirderIndexType ngirds = pBridge->GetGirderCount(span);
   int row=p_table->GetNumberOfHeaderRows();
   for (GirderIndexType ig=0; ig<ngirds; ig++)
   {
      CGirderData girderData = pGirderData->GetGirderData(span,ig);

      ColumnIndexType col = 0;
      (*p_table)(row,col++) << LABEL_GIRDER(ig);
      (*p_table)(row,col++) << girderData.Material.pStrandMaterial->GetName();
      (*p_table)(row,col++) << girderData.Nstrands[pgsTypes::Straight];
      (*p_table)(row,col++) << force.SetValue(girderData.Pjack[pgsTypes::Straight]);
      (*p_table)(row,col++) << girderData.Nstrands[pgsTypes::Harped];
      (*p_table)(row,col++) << force.SetValue(girderData.Pjack[pgsTypes::Harped]);
      if ( bTempStrands )
      {
         (*p_table)(row,col++) << girderData.Nstrands[pgsTypes::Temporary];
         (*p_table)(row,col++) << force.SetValue(girderData.Pjack[pgsTypes::Temporary]);
      }

      // convert to absolute adjustment
      double adjustment = pStrandGeometry->ComputeAbsoluteHarpedOffsetEnd(span, ig, girderData.Nstrands[pgsTypes::Harped], girderData.HsoEndMeasurement, girderData.HpOffsetAtEnd);
      (*p_table)(row,col++) << dim.SetValue(adjustment);

      adjustment = pStrandGeometry->ComputeAbsoluteHarpedOffsetHp(span, ig, girderData.Nstrands[pgsTypes::Harped], girderData.HsoHpMeasurement, girderData.HpOffsetAtHp);
      (*p_table)(row,col++) << dim.SetValue(adjustment);
      row++;
   }

   *pPara<<"Girder End Offset - Distance the HS bundle at the girder ends is adjusted vertically from original library location."<<rptNewLine;
   *pPara<<"Harping Point Offset - Distance the HS bundle at the harping point is adjusted  vertically from original library location."<<rptNewLine;
}

void material(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDispUnit,SpanIndexType span)
{
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType nspans = pBridge->GetSpanCount();
   CHECK(span<nspans);

   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<"Girder Concrete for Span " << LABEL_SPAN(span) << rptNewLine;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDispUnit->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDispUnit->GetStressUnit(),        false );
   INIT_UV_PROTOTYPE( rptDensityUnitValue, density, pDispUnit->GetDensityUnit(),      false );

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(6,"");
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << p_table<<rptNewLine;

   (*p_table)(0,0) << "Girder";
   (*p_table)(0,1) << COLHDR(RPT_FC,rptStressUnitTag, pDispUnit->GetStressUnit() );
   (*p_table)(0,2) << COLHDR(RPT_FCI,rptStressUnitTag, pDispUnit->GetStressUnit() );
   (*p_table)(0,3) << COLHDR("Density" << rptNewLine << "for" << rptNewLine << "Strength",rptDensityUnitTag, pDispUnit->GetDensityUnit() );
   (*p_table)(0,4) << COLHDR("Density" << rptNewLine << "for" << rptNewLine << "Weight",rptDensityUnitTag, pDispUnit->GetDensityUnit() );
   (*p_table)(0,5) << COLHDR("Max" << rptNewLine << "Aggregate" << rptNewLine << "Size",rptLengthUnitTag, pDispUnit->GetComponentDimUnit() );

   GirderIndexType ngirds = pBridge->GetGirderCount(span);
   int row=1;
   for (GirderIndexType ig=0; ig<ngirds; ig++)
   {
      const CGirderMaterial* pGirderMaterial = pGirderData->GetGirderMaterial(span,ig);
      CHECK(pGirderMaterial!=0);

      (*p_table)(row,0) << LABEL_GIRDER(ig);
      (*p_table)(row,1) << stress.SetValue(pGirderMaterial->Fc);
      (*p_table)(row,2) << stress.SetValue(pGirderMaterial->Fci);
      (*p_table)(row,3) << density.SetValue(pGirderMaterial->StrengthDensity);
      (*p_table)(row,4) << density.SetValue(pGirderMaterial->WeightDensity);
      (*p_table)(row,5) << dim.SetValue(pGirderMaterial->MaxAggregateSize);
      row++;
   }

}

void stirrups(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDispUnits,SpanIndexType span)
{
   GET_IFACE2(pBroker,IBridge,pBridge);

   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<"Transverse Reinforcement Stirrup Zones"<<rptNewLine;

   CStirrupTable stirr_table;

   GirderIndexType ngirds = pBridge->GetGirderCount(span);
   int row=1;
   for (GirderIndexType ig=0; ig<ngirds; ig++)
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara <<bold(ON)<<"Span " << LABEL_SPAN(span) << ", Girder "<<LABEL_GIRDER(ig)<<bold(OFF);
      stirr_table.Build(pChapter,pBroker,span,ig,pDispUnits);
   }
}

void handling(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDispUnit,SpanIndexType span)
{
   GET_IFACE2(pBroker,IGirderLifting,pGirderLifting);
   GET_IFACE2(pBroker,IGirderHauling,pGirderHauling);
   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType nspans = pBridge->GetSpanCount();
   CHECK(span<nspans);

   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<"Lifting and Shipping Locations for Span "<< LABEL_SPAN(span)<<rptNewLine;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc, pDispUnit->GetSpanLengthUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(5,"");
   *pPara << p_table<<rptNewLine;

   (*p_table)(0,0) << "Girder";
   (*p_table)(0,1) << COLHDR("Left Lifting" << rptNewLine << "Loop Location",rptLengthUnitTag, pDispUnit->GetSpanLengthUnit() );
   (*p_table)(0,2) << COLHDR("Right Lifting" << rptNewLine << "Loop Location",rptLengthUnitTag, pDispUnit->GetSpanLengthUnit() );
   (*p_table)(0,3) << COLHDR("Leading Truck" << rptNewLine << "Support Location",rptLengthUnitTag, pDispUnit->GetSpanLengthUnit() );
   (*p_table)(0,4) << COLHDR("Trailing Truck" << rptNewLine << "Support Location",rptLengthUnitTag, pDispUnit->GetSpanLengthUnit() );


   GirderIndexType ngirds = pBridge->GetGirderCount(span);
   int row=1;
   for (GirderIndexType ig=0; ig<ngirds; ig++)
   {
      (*p_table)(row,0) << LABEL_GIRDER(ig);
      (*p_table)(row,1) << loc.SetValue(pGirderLifting->GetLeftLiftingLoopLocation(span,ig));
      (*p_table)(row,2) << loc.SetValue(pGirderLifting->GetRightLiftingLoopLocation(span,ig));
      (*p_table)(row,3) << loc.SetValue(pGirderHauling->GetLeadingOverhang(span,ig));
      (*p_table)(row,4) << loc.SetValue(pGirderHauling->GetTrailingOverhang(span,ig));
      row++;
   }
}