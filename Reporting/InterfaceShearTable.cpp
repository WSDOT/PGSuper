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
#include <Reporting\InterfaceShearTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Artifact.h>
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>

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

CInterfaceShearTable::CInterfaceShearTable()
{
}

CInterfaceShearTable::~CInterfaceShearTable()
{
}

void CInterfaceShearTable::Build( IBroker* pBroker, rptChapter* pChapter,
                                  const pgsGirderArtifact* pGirderArtifact,
                                  IEAFDisplayUnits* pDisplayUnits,
                                  IntervalIndexType intervalIdx,
                                  pgsTypes::LimitState ls) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   GET_IFACE2(pBroker,IBridge,pBridge);

   INIT_UV_PROTOTYPE( rptPointOfInterest,         location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, shear,    pDisplayUnits->GetForcePerLengthUnit(),        false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,         fy,       pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptAreaPerLengthValue,      AvS,      pDisplayUnits->GetAvOverSUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         dim,      pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,           area,     pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         dimu,      pDisplayUnits->GetComponentDimUnit(),  true);

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   location.IncludeSpanAndGirder(pDocType->IsPGSpliceDocument() || girderKey.groupIndex == ALL_GROUPS);

   rptCapacityToDemand cap_demand;

   rptParagraph* pPara;
   pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;

   (*pPara) << _T("Horizontal Interface Shear for ") << GetLimitStateString(ls) << _T(" Limit State") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   (*pPara) << _T("AASHTO LRFD BDS ") << LrfdCw8th(_T("5.8.4"), _T("5.7.4")) << rptNewLine;

   std::_tstring strSmaxArticle = LrfdCw8th(_T("5.8.4.2"), _T("5.7.4.5"));
   std::_tstring strAvfMinArticle = LrfdCw8th(_T("5.8.4.4"), _T("5.7.4.2"));;
   std::_tstring strVniArticle = LrfdCw8th(_T("5.8.4.1"), _T("5.7.4.1"));

   GET_IFACE2(pBroker, IMaterials, pMaterials);
   if (pMaterials->GetSegmentConcreteType(CSegmentKey(girderKey, 0)) == pgsTypes::PCI_UHPC)
   {
      (*pPara) << _T("PCI UHPC SDG E.7.4") << rptNewLine;
      strSmaxArticle = _T("SDG E.7.4.5");
      strAvfMinArticle = _T("SDG E.7.4.2");
      strVniArticle = _T("SDG E.7.4.1");
   }

   if (pMaterials->GetSegmentConcreteType(CSegmentKey(girderKey, 0)) == pgsTypes::UHPC)
   {
      (*pPara) << _T("UHPC GS 1.7.4") << rptNewLine;
      strSmaxArticle = _T("GS 1.7.4.5");
      strAvfMinArticle = _T("GS 1.7.4.2");
      strVniArticle = _T("GS 1.7.4.1");
   }

   pPara = new rptParagraph();
   *pChapter << pPara;

   rptRcTable* table = rptStyleManager::CreateDefaultTable(10);
   *pPara << table;

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   table->SetNumberOfHeaderRows(2);
   table->SetRowSpan(0,0,2);

   (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   table->SetColumnSpan(0,1,3);
   (*table)(0,1) << strSmaxArticle;
   (*table)(1,1) << COLHDR(_T("s"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*table)(1,2) << COLHDR(_T("s")<<Sub(_T("max")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*table)(1,3) << _T("Status");

   table->SetColumnSpan(0,4,3);
   (*table)(0,4) << strAvfMinArticle;
   (*table)(1,4) << COLHDR(_T("a")<<Sub(_T("vf")), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(1,5) << COLHDR(_T("a")<<Sub(_T("vf min")), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(1,6) << _T("Status");

   table->SetColumnSpan(0,7,3);
   (*table)(0,7) << strVniArticle;
   (*table)(1,7) << COLHDR(_T("|v") << Sub(_T("ui")) << _T("|"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   (*table)(1,8) << COLHDR(symbol(phi) << _T("v") << Sub(_T("ni")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   (*table)(1,9) << _T("Status") << rptNewLine << _T("(") << symbol(phi) << Sub2(_T("v"),_T("ni")) << _T("/") << _T("|") << Sub2(_T("v"),_T("ui")) << _T("|)");

   // Fill up the table
   Float64 bvmax = lrfdConcreteUtil::UpperLimitForBv();
   Float64 minlegs;
   bool do_note=false;
   bool bDidAvsDecreaseAtEnd = false;
   pgsPointOfInterest poiAtEnd;
   RowIndexType row = table->GetNumberOfHeaderRows();

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact* pStirrupArtifact = pSegmentArtifact->GetStirrupCheckArtifact();
      ATLASSERT(pStirrupArtifact != nullptr);

      CollectionIndexType nArtifacts = pStirrupArtifact->GetStirrupCheckAtPoisArtifactCount( intervalIdx,ls );
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pStirrupArtifact->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );
         if ( psArtifact == nullptr )
         {
            continue;
         }

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();

         const pgsHorizontalShearArtifact* pArtifact = psArtifact->GetHorizontalShearArtifact();

         // this check is always applicable and must come before IsApplicable() below
         if (pArtifact->DidAvsDecreaseAtEnd())
         {
            bDidAvsDecreaseAtEnd = true;
            poiAtEnd = poi;
         }

         if (!pArtifact->IsApplicable())
         {
            continue;
         }

         ColumnIndexType col = 0;

         (*table)(row,col++) << location.SetValue( POI_SPAN, poi );
         Float64 smax = pArtifact->GetSpacing();
         if (0.0 < smax)
         {
            (*table)(row,col++) << dim.SetValue( smax );
         }
         else
         {
            (*table)(row,col++) << symbol(infinity);
         }

         (*table)(row,col++) << dim.SetValue( pArtifact->GetSmax() );

         if ( pArtifact->SpacingPassed() )
         {
            (*table)(row,col++) << RPT_PASS;
         }
         else
         {
            (*table)(row,col++) << RPT_FAIL;
         }
         
         (*table)(row,col++) << AvS.SetValue( pArtifact->GetAvOverS());

         if (pArtifact->Is5_7_4_1_4Applicable())
         {
            (*table)(row,col++) << AvS.SetValue( pArtifact->GetAvOverSMin());
            if ( 0 < pArtifact->MinReinforcementPassed() )
            {
               (*table)(row,col++) << RPT_PASS;
            }
            else
            {
               (*table)(row,col++) << RPT_FAIL;
            }
         }
         else
         {
            (*table)(row,col++) << RPT_NA;
            (*table)(row,col++) << RPT_NA;
         }

         Float64 vu = pArtifact->GetDemand();
         Float64 vr = pArtifact->GetCapacity();
         (*table)(row,col++) << shear.SetValue( vu );
         (*table)(row,col++) << shear.SetValue( vr );

         if (bvmax <= pArtifact->GetBv())
         {
            if (pArtifact->GetNumLegs() < pArtifact->GetNumLegsReqd())
            {
               (*table)(row,col) << color(Blue)<< _T("* ") << color(Black);
               do_note = true;
               minlegs = pArtifact->GetNumLegsReqd();
            }
         }

         bool bPassed = pArtifact->StrengthPassed();
         if ( bPassed )
         {
            (*table)(row,col) << RPT_PASS;
         }
         else
         {
            (*table)(row,col) << RPT_FAIL;
         }

         (*table)(row,col++) << rptNewLine << _T("(") << cap_demand.SetValue(vr,vu,bPassed) << _T(")");

         row++;
      }
   } // next segment

   if (do_note)
   {
      INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

       pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
       *pChapter << pPara;
       *pPara<<color(Blue)<< _T("*") << color(Black)<<_T(" Note: b")<<Sub(_T("v"))<<_T(" exceeds ")<<dimu.SetValue(bvmax)<<_T(" and number of legs < ")<< scalar.SetValue(minlegs)<<rptNewLine;
   }

   pPara = new rptParagraph();
   *pChapter << pPara;

   // Check that avs at end pois are at least that at CSS
   if ( bDidAvsDecreaseAtEnd )
   {
      *pPara << RPT_FAIL << _T(" - Horizontal ") << Sub2(_T("a"),_T("vf")) << _T(" at ") << location.SetValue(POI_SPAN, poiAtEnd)
             << _T(" is less than at the design section (CS). Revise stirrup details to increase horizontal ") << Sub2(_T("a"),_T("vf"))
             << _T(" at this location.") << rptNewLine;
   }
}
