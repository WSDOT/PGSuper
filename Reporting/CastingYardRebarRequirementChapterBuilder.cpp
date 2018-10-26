///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <Reporting\CastingYardRebarRequirementChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Artifact.h>
#include <IFace\Bridge.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CCastingYardRebarRequirementChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCastingYardRebarRequirementChapterBuilder::CCastingYardRebarRequirementChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CCastingYardRebarRequirementChapterBuilder::GetName() const
{
   return TEXT("Casting Yard Tensile Reinforcement Requirements");
}

rptChapter* CCastingYardRebarRequirementChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );
   INIT_UV_PROTOTYPE( rptPointOfInterest, location,       pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(span == ALL_SPANS);
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,          pDisplayUnits->GetShearUnit(),         false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area,        pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Details for Tensile Reinforcement Requirement for Allowable Tension Stress in Casting Yard [5.9.4][C5.9.4.1.2]")<<rptNewLine;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( span, girder, pgsTypes::CastingYard, POI_FLEXURESTRESS | POI_TABULAR );
   CHECK(vPoi.size()>0);

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(12,_T("Rebar Requirements for Tensile Stress Limit [C5.9.4.1.2]"));
   *p << pTable << rptNewLine;

   pTable->SetNumberOfHeaderRows(2);

   if ( span == ALL_SPANS )
   {
      pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   ColumnIndexType col = 0;
   // build first heading row
   pTable->SetRowSpan(0,col,2);
   (*pTable)(0,col++) << COLHDR(RPT_GDR_END_LOCATION,  rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   pTable->SetRowSpan(0,col,2);
   (*pTable)(0,col++) << COLHDR(Sub2(_T("Y"),_T("na")),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   pTable->SetColumnSpan(0,col, 5);
   (*pTable)(0,col++) << _T("Girder Top");

   pTable->SetColumnSpan(0,col, 5);
   (*pTable)(0,col++) << _T("Girder Bottom");

   ColumnIndexType i;
   for ( i = col; i < pTable->GetNumberOfColumns(); i++ )
      pTable->SetColumnSpan(0,i,SKIP_CELL);

   // build second hearing row
   col = 0;
   pTable->SetRowSpan(1,col++,SKIP_CELL);
   pTable->SetRowSpan(1,col++,SKIP_CELL);
   (*pTable)(1,col++) << COLHDR(Sub2(_T("f"),_T("t")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(1,col++) << COLHDR(Sub2(_T("A"),_T("t")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(1,col++) << COLHDR(_T("T"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*pTable)(1,col++) << COLHDR(Sub2(_T("* A"),_T("s"))<< rptNewLine << _T("Provided"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(1,col++) << COLHDR(Sub2(_T("A"),_T("s"))<< rptNewLine << _T("Required"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(1,col++) << COLHDR(Sub2(_T("f"),_T("b")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(1,col++) << COLHDR(Sub2(_T("A"),_T("t")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(1,col++) << COLHDR(_T("T"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*pTable)(1,col++)<< COLHDR(Sub2(_T("* A"),_T("s"))<< rptNewLine << _T("Provided"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(1,col++)<< COLHDR(Sub2(_T("A"),_T("s"))<< rptNewLine << _T("Required"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );

   Int16 row=2;
   for (std::vector<pgsPointOfInterest>::iterator i = vPoi.begin(); i!= vPoi.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;
      (*pTable)(row,0) << location.SetValue( pgsTypes::CastingYard, poi );

      const pgsFlexuralStressArtifact* pArtifact;
      pArtifact = gdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Tension,poi.GetDistFromStart()) );

      ATLASSERT(pArtifact != NULL);
      if ( pArtifact == NULL )
         continue;

      Float64 Yna,At,T,AsProvided,AsRequired;
      pArtifact->GetAlternativeTensileStressParameters(&Yna,&At,&T,&AsProvided,&AsRequired);

      if (Yna < 0 )
      {
         // Entire section is in compression
         for ( ColumnIndexType ic = 1; ic < pTable->GetNumberOfColumns(); ic++ )
          (*pTable)(row,ic) << _T("-");
      }
      else
      {
         (*pTable)(row,1) << dim.SetValue(Yna);

         // We have a neutral axis. See which side is in tension
         Float64 fTop, fBot;
         pArtifact->GetDemand(&fTop, &fBot);

         // Half of the table is always n/a. Determine which half to fill
         ColumnIndexType dataStart, dataEnd;
         ColumnIndexType blankStart, blankEnd;
         if (fTop>0.0)
         {
            dataStart = 3;
            dataEnd = 7;
            blankStart = 8;
            blankEnd = 12;
         }
         else
         {
            dataStart = 8;
            dataEnd = 12;
            blankStart = 3;
            blankEnd = 7;
         }

         // Stresses
         (*pTable)(row,2) << stress.SetValue(fTop);
         (*pTable)(row,7) << stress.SetValue(fBot);

         // Fill in compression columns with n/a's first
         for (col=blankStart; col<blankEnd; col++)
         {
             (*pTable)(row,col) << RPT_NA;
         }

         // Now fill in tension side with data
         col = dataStart;
         (*pTable)(row,col++) << area.SetValue(At);
         (*pTable)(row,col++) << force.SetValue(T);
         (*pTable)(row,col++) << area.SetValue(AsProvided);
         (*pTable)(row,col++) << area.SetValue(AsRequired);
         ATLASSERT(col==dataEnd);
      }

      row++;
   }

   *p << _T("* Bars must be fully developed and lie within tension portion of section before they are considered.");

   return pChapter;
}


CChapterBuilder* CCastingYardRebarRequirementChapterBuilder::Clone() const
{
   return new CCastingYardRebarRequirementChapterBuilder;
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
