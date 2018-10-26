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
#include <Reporting\InterfaceShearTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
#include <IFace\Artifact.h>

#include <Lrfd\ConcreteUtil.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CInterfaceShearTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CInterfaceShearTable::CInterfaceShearTable()
{
}

CInterfaceShearTable::~CInterfaceShearTable()
{
}

//======================== OPERATORS  =======================================

//======================== OPERATIONS =======================================
void CInterfaceShearTable::Build( IBroker* pBroker, rptChapter* pChapter,
                                  SpanIndexType span,GirderIndexType girder,
                                  IDisplayUnits* pDisplayUnits,
                                  pgsTypes::Stage stage,
                                  pgsTypes::LimitState ls) const
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   INIT_UV_PROTOTYPE( rptPointOfInterest,         location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, shear,    pDisplayUnits->GetForcePerLengthUnit(),        false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,         fy,       pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptAreaPerLengthValue,      AvS,      pDisplayUnits->GetAvOverSUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         dim,      pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,           area,     pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         dimu,      pDisplayUnits->GetComponentDimUnit(),  true);

   rptCapacityToDemand cap_demand;

   rptParagraph* pPara;
   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;

   if (ls==pgsTypes::StrengthI)
      *pPara << "Horizontal Interface Shears/Length for Strength I Limit State [5.8.4]"<<rptNewLine;
   else if (ls==pgsTypes::StrengthII)
      *pPara << "Horizontal Interface Shears/Length for Strength II Limit State [5.8.4]"<<rptNewLine;
   else
      ATLASSERT(false);

   pPara = new rptParagraph();
   *pChapter << pPara;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(10,"");
   *pPara << table;

   table->SetNumberOfHeaderRows(2);
   table->SetRowSpan(0,0,2);
   table->SetRowSpan(1,0,-1);
   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   table->SetColumnSpan(0,1,3);
   (*table)(0,1) << "5.8.4.2";
   (*table)(1,1)  << COLHDR("s", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*table)(1,2)  << COLHDR("s"<<Sub("max"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*table)(1,3) << "Status";

   table->SetColumnSpan(0,2,3);
   (*table)(0,2) << "5.8.4.4";
   (*table)(1,4)  << COLHDR("a"<<Sub("vf"), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(1,5)  << COLHDR("a"<<Sub("vf min"), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(1,6) << "Status";

   table->SetColumnSpan(0,3,3);
   (*table)(0,3) << "5.8.4.1";
   (*table)(1,7)  << COLHDR("|v" << Sub("ui") << "|", rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   (*table)(1,8)  << COLHDR(symbol(phi) << "v" << Sub("ni"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   (*table)(1,9) << "Status" << rptNewLine << "(" << symbol(phi) << Sub2("v","ni") << "/" << "|" << Sub2("v","ui") << "|)";

   table->SetColumnSpan(0,4,-1);
   table->SetColumnSpan(0,5,-1);
   table->SetColumnSpan(0,6,-1);
   table->SetColumnSpan(0,7,-1);
   table->SetColumnSpan(0,8,-1);
   table->SetColumnSpan(0,9,-1);

   // Fill up the table
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsStirrupCheckArtifact* pstirrup_artifact= gdrArtifact->GetStirrupCheckArtifact();
   CHECK(pstirrup_artifact);

   Float64 bvmax = lrfdConcreteUtil::UpperLimitForBv();
   Uint32 minlegs;
   bool do_note=false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( stage, span, girder, POI_TABULAR|POI_SHEAR );
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      const pgsHorizontalShearArtifact* pArtifact = psArtifact->GetHorizontalShearArtifact();

      ColumnIndexType col = 0;

      (*table)(row,col++) << location.SetValue( poi, end_size );
      (*table)(row,col++) << dim.SetValue( pArtifact->GetSMax() );
      (*table)(row,col++) << dim.SetValue( pArtifact->GetSall() );

      if ( pArtifact->SpacingPassed() )
         (*table)(row,col++) << RPT_PASS;
      else
         (*table)(row,col++) << RPT_FAIL;
      
      (*table)(row,col++) << AvS.SetValue( pArtifact->GetAvOverS());

      if (pArtifact->Is5_8_4_1_4Applicable())
      {
         (*table)(row,col++) << AvS.SetValue( pArtifact->GetAvOverSMin());
         if ( 0 < pArtifact->MinReinforcementPassed() )
            (*table)(row,col++) << RPT_PASS;
         else
            (*table)(row,col++) << RPT_FAIL;
      }
      else
      {
         (*table)(row,col++) << RPT_NA;
         (*table)(row,col++) << RPT_NA;
      }

      double vu = pArtifact->GetDemand();
      double vr = pArtifact->GetCapacity();
      (*table)(row,col++) << shear.SetValue( vu );
      (*table)(row,col++) << shear.SetValue( vr );

      if (pArtifact->GetBv()>=bvmax)
      {
         if (pArtifact->GetNumLegs()<pArtifact->GetNumLegsReqd())
         {
            (*table)(row,col++) << color(Blue)<< "*" << color(Black);
            do_note = true;
            minlegs = pArtifact->GetNumLegsReqd();
         }
      }

      bool bPassed = pArtifact->Passed();
      if ( bPassed )
         (*table)(row,col) << RPT_PASS;
      else
         (*table)(row,col) << RPT_FAIL;

      (*table)(row,col++) << rptNewLine << "(" << cap_demand.SetValue(vr,vu,bPassed) << ")";

      row++;
   }

   if (do_note)
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara<<color(Blue)<< "*" << color(Black)<<" Note: b"<<Sub("v")<<" exceeds "<<dimu.SetValue(bvmax)<<" and number of legs < "<< minlegs<<rptNewLine;
   }
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

//======================== DEBUG      =======================================
#if defined _DEBUG
bool CInterfaceShearTable::AssertValid() const
{
   return true;
}

void CInterfaceShearTable::Dump(dbgDumpContext& os) const
{
   os << "Dump for CInterfaceShearTable" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CInterfaceShearTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CInterfaceShearTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CInterfaceShearTable");

   TESTME_EPILOG("CInterfaceShearTable");
}
#endif // _UNITTEST
