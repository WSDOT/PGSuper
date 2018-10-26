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

   const pgsFlexuralStressArtifact* pArtifact;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,_T("Rebar Requirements for Tensile Stress Limit [C5.9.4.1.2]"));
   *p << pTable << rptNewLine;

   if ( span == ALL_SPANS )
   {
      pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   (*pTable)(0,0) << COLHDR(RPT_GDR_END_LOCATION,  rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*pTable)(0,1) << COLHDR(Sub2(_T("Y"),_T("na")),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,2) << COLHDR(Sub2(_T("A"),_T("t")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(0,3) << COLHDR(_T("T"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*pTable)(0,4) << COLHDR(Sub2(_T("A"),_T("s")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );

   Int16 row=1;
   for (std::vector<pgsPointOfInterest>::iterator i = vPoi.begin(); i!= vPoi.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;
      (*pTable)(row,0) << location.SetValue( pgsTypes::CastingYard, poi );

      pArtifact = gdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Tension,poi.GetDistFromStart()) );

      ATLASSERT(pArtifact != NULL);
      if ( pArtifact == NULL )
         continue;

      ATLASSERT(pArtifact->IsAlternativeTensileStressApplicable());
 
      Float64 Yna,At,T,As;
      pArtifact->GetAlternativeTensileStressParameters(&Yna,&At,&T,&As);

      if (Yna < 0 )
          (*pTable)(row,1) << _T("-");
      else
         (*pTable)(row,1) << dim.SetValue(Yna);

      (*pTable)(row,2) << area.SetValue(At);
      (*pTable)(row,3) << force.SetValue(T);
      (*pTable)(row,4) << area.SetValue(As);

      row++;
   }

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
