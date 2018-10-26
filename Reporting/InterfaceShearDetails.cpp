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
#include <Reporting\InterfaceShearDetails.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\GirderPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <Lrfd\ConcreteUtil.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CInterfaceShearDetails
****************************************************************************/

CInterfaceShearDetails::CInterfaceShearDetails()
{
}

CInterfaceShearDetails::~CInterfaceShearDetails()
{
}

void CInterfaceShearDetails::Build( IBroker* pBroker, rptChapter* pChapter,
                                  const CGirderKey& girderKey,
                                  IEAFDisplayUnits* pDisplayUnits,
                                  IntervalIndexType intervalIdx,
                                  pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IStirrupGeometry, pStirrupGeometry);
   GET_IFACE2(pBroker,IMaterials,pMaterial);

   const pgsGirderArtifact* pGirderArtifact = pIArtifact->GetGirderArtifact(girderKey);

   INIT_UV_PROTOTYPE( rptPointOfInterest,  location,         pDisplayUnits->GetSpanLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,          shear,            pDisplayUnits->GetGeneralForceUnit(),    false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, shear_per_length, pDisplayUnits->GetForcePerLengthUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,         fy,               pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,         stress,           pDisplayUnits->GetStressUnit(),          false);
   INIT_UV_PROTOTYPE( rptStressUnitValue,         stress_with_tag,  pDisplayUnits->GetStressUnit(),          true);
   INIT_UV_PROTOTYPE( rptAreaPerLengthValue,      AvS,              pDisplayUnits->GetAvOverSUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         dim,              pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,           area,             pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptLength3UnitValue,        l3,               pDisplayUnits->GetSectModulusUnit(),     false);
   INIT_UV_PROTOTYPE( rptLength4UnitValue,        l4,               pDisplayUnits->GetMomentOfInertiaUnit(), false);

   //location.IncludeSpanAndGirder(span == ALL_SPANS);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   // find the first applicable artifact per segment
   const pgsStirrupCheckAtPoisArtifact** p_first_sartifact = new const pgsStirrupCheckAtPoisArtifact*[nSegments];
   const pgsHorizontalShearArtifact**    p_first_artifact  = new const pgsHorizontalShearArtifact*[nSegments];
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
      const pgsStirrupCheckArtifact* pstirrup_artifact= pSegmentArtifact->GetStirrupCheckArtifact();
      ATLASSERT(pstirrup_artifact);

      CollectionIndexType nArtifacts = pstirrup_artifact->GetStirrupCheckAtPoisArtifactCount(intervalIdx,ls);
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         p_first_sartifact[segIdx] = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );
         const pgsHorizontalShearArtifact* p_tmp = p_first_sartifact[segIdx]->GetHorizontalShearArtifact();
         if (p_tmp->IsApplicable())
         {
            p_first_artifact[segIdx] = p_tmp;
            break;
         }
         else
         {
            p_first_artifact[segIdx] = NULL;
         }
      } // next artifact
   } // next segment

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   rptParagraph* pPara;
   // Initial Capacity Table
   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   (*pPara) << _T("Details for Horizontal Interface Shear Capacity (") << OLE2T(pEventMap->GetLimitStateName(ls)) << _T(") [5.8.4.1]") << rptNewLine;

   pPara = new rptParagraph();
   *pChapter << pPara;

   ColumnIndexType nCol = pSpecEntry->GetShearFlowMethod() == sfmLRFD ? 6 : 7;

   rptRcTable* vui_table = pgsReportStyleHolder::CreateDefaultTable(nCol);
   *pPara << vui_table << rptNewLine;

   //if ( span == ALL_SPANS )
   //{
   //   vui_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   vui_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   ColumnIndexType col = 0;

   (*vui_table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   if ( pSpecEntry->GetShearFlowMethod() == sfmLRFD )
   {
     (*vui_table)(0,col++) << COLHDR(Sub2(_T("d"),_T("vi")),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   }
   else
   {
      (*vui_table)(0,col++) << COLHDR(_T("I"),rptLength4UnitTag,pDisplayUnits->GetMomentOfInertiaUnit());
      (*vui_table)(0,col++) << COLHDR(Sub2(_T("Q"),_T("slab")),rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit());
   }

   (*vui_table)(0,col++) << COLHDR(Sub2(_T("V"),_T("u")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());

   if ( pSpecEntry->GetShearFlowMethod() == sfmLRFD )
   {
      (*vui_table)(0,col++) << COLHDR(Sub2(_T("v"),_T("ui")) << _T(" = ") << Sub2(_T("V"),_T("u")) << _T("/") << Sub2(_T("d"),_T("vi")),rptForcePerLengthUnitTag,pDisplayUnits->GetForcePerLengthUnit());
   }
   else
   {
      (*vui_table)(0,col++) << COLHDR(Sub2(_T("v"),_T("ui")) << _T(" = ") << Sub2(_T("V"),_T("u")) << _T("Q/I"),rptForcePerLengthUnitTag,pDisplayUnits->GetForcePerLengthUnit());
   }

   (*vui_table)(0,col++) << COLHDR(Sub2(_T("b"),_T("vi")),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*vui_table)(0,col++) << COLHDR(Sub2(symbol(nu),_T("ui")),rptStressUnitTag,pDisplayUnits->GetStressUnit());

   if ( pSpecEntry->GetShearFlowMethod() == sfmLRFD )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2(_T("d"),_T("vi")) << _T(" = ") << Sub2(_T("Y"),_T("t girder")) << _T(" + Strand Eccentricity + ") << Sub2(_T("t"),_T("slab")) << _T("/2") << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;
   }

   // TRICKY: create av/s table to be filled in same loop as next table
   rptRcTable* av_table = pgsReportStyleHolder::CreateDefaultTable(6);
   *pPara << av_table;

   //if ( span == ALL_SPANS )
   //{
   //   av_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   av_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   (*av_table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*av_table)(0,1)  << COLHDR(_T("A") << Sub(_T("vf"))<<rptNewLine<<_T("Primary") , rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*av_table)(0,2)  << COLHDR(_T("S")<<rptNewLine<<_T("Primary"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*av_table)(0,3)  << COLHDR(_T("A") << Sub(_T("vf"))<<rptNewLine<<_T("Additional") , rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*av_table)(0,4)  << COLHDR(_T("S")<<rptNewLine<<_T("Additional"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*av_table)(0,5)  << COLHDR(_T("a") << Sub(_T("vf"))<<rptNewLine<<_T("Composite") , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );


   bool spec2007OrOlder = lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType();
   Float64 fy_max = ::ConvertToSysUnits(60.0,unitMeasure::KSI); // LRFD 2013 5.8.4.1

   // general quantities
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      if ( 1 < nSegments )
      {
         *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }

      CSegmentKey segmentKey(girderKey,segIdx);

      *pPara << _T("Coeff. of Friction (")   << symbol(mu)<<_T(") = ") << p_first_artifact[segIdx]->GetFrictionFactor() << rptTab
             << _T("Cohesion Factor (c) = ") << stress_with_tag.SetValue(p_first_artifact[segIdx]->GetCohesionFactor()) << rptTab;

      if ( spec2007OrOlder )
      {
         *pPara << Sub2(_T("K"),_T("1")) << _T(" = ") << p_first_artifact[segIdx]->GetK1() << rptTab
                << Sub2(_T("K"),_T("2")) << _T(" = ") << stress_with_tag.SetValue(p_first_artifact[segIdx]->GetK2()) << rptTab;
      }

      *pPara << symbol(phi) << _T(" = ") << p_first_artifact[segIdx]->GetPhi() << rptTab
             << RPT_FC << _T(" = ") << stress_with_tag.SetValue(p_first_artifact[segIdx]->GetFc()) << rptTab
             << RPT_FY << _T(" = ") << stress_with_tag.SetValue(p_first_artifact[segIdx]->GetFy());
      if ( p_first_artifact[segIdx]->WasFyLimited() )
      {
         *pPara << _T(", ") << RPT_FY << _T(" is limited to ") << stress_with_tag.SetValue(fy_max) << _T(" (LRFD 5.8.4.1)");
      }
      *pPara << rptNewLine;

      if ( segIdx < nSegments-1 )
      {
         *pPara << _T("Closure Pour ") << LABEL_SEGMENT(segIdx) << rptNewLine;

         const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
         const pgsStirrupCheckArtifact* pstirrup_artifact= pSegmentArtifact->GetStirrupCheckArtifact();
         CollectionIndexType nArtifacts = pstirrup_artifact->GetStirrupCheckAtPoisArtifactCount(intervalIdx,ls);
         const pgsStirrupCheckAtPoisArtifact* pStirrupAtPoiArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,nArtifacts-1 );
         ATLASSERT(pStirrupAtPoiArtifact->GetPointOfInterest().HasAttribute(POI_CLOSURE));
         const pgsHorizontalShearArtifact* pHorizShearArtifact = pStirrupAtPoiArtifact->GetHorizontalShearArtifact();
         ATLASSERT(pHorizShearArtifact->IsApplicable());

         *pPara << _T("Coeff. of Friction (")   << symbol(mu)<<_T(") = ") << pHorizShearArtifact->GetFrictionFactor() << rptTab
                << _T("Cohesion Factor (c) = ") << stress_with_tag.SetValue(pHorizShearArtifact->GetCohesionFactor()) << rptTab;

         if ( spec2007OrOlder )
         {
            *pPara << Sub2(_T("K"),_T("1")) << _T(" = ") << pHorizShearArtifact->GetK1() << rptTab
                   << Sub2(_T("K"),_T("2")) << _T(" = ") << stress_with_tag.SetValue(pHorizShearArtifact->GetK2()) << rptTab;
         }

         *pPara << symbol(phi) << _T(" = ") << pHorizShearArtifact->GetPhi() << rptTab
                << RPT_FC << _T(" = ") << stress_with_tag.SetValue(pHorizShearArtifact->GetFc()) << rptTab
                << RPT_FY << _T(" = ") << stress_with_tag.SetValue(pHorizShearArtifact->GetFy());
         if ( pHorizShearArtifact->WasFyLimited() )
         {
            *pPara << _T(", ") << RPT_FY << _T(" is limited to ") << stress_with_tag.SetValue(fy_max) << _T(" (LRFD 5.8.4.1)");
         }
         *pPara << rptNewLine;
         *pPara << rptNewLine;
      }
   }

   if ( spec2007OrOlder )
   {
      *pPara << Sub2(_T("v"),_T("ni"))<<_T(" = min( c") << Sub2(_T("a"),_T("cv"))<<_T(" + ")<<symbol(mu)<<_T("[ ") << Sub2(_T("a"),_T("vf"))<< RPT_FY <<_T(" + ") << Sub2(_T("p"),_T("c"))<<_T("], ")
                                       << Sub2(_T("K"),_T("1")) << RPT_FC << Sub2(_T("a"),_T("cv"))<<_T(", ") << Sub2(_T("K"),_T("2")) << Sub2(_T("a"),_T("cv"))<<_T(" )")<<rptNewLine;
   }
   else
   {
      *pPara << Sub2(_T("v"),_T("ni"))<<_T(" = min( ca")<<Sub(_T("cv"))<<_T(" + ")<<symbol(mu)<<_T("[ a")<<Sub(_T("vf "))<<RPT_FY<<_T(" + p")<<Sub(_T("c"))<<_T("], ")
                                       <<_T("0.2 ") << RPT_FC <<_T("a")<<Sub(_T("cv"))<<_T(", ");

      if ( IS_SI_UNITS(pDisplayUnits) )
         *pPara<<_T(" 5.5 a")<<Sub(_T("cv"))<<_T(" )")<<rptNewLine;
      else
         *pPara<<_T(" 0.8 a")<<Sub(_T("cv"))<<_T(" )")<<rptNewLine;
   }

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(8,_T(""));
   *pPara << table;

 
   //if ( span == ALL_SPANS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,1)  << COLHDR(Sub2(_T("a"),_T("cv")) , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0,2)  << COLHDR(Sub2(_T("a"),_T("vf")) , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0,3)  << COLHDR(Sub2(_T("p"),_T("c")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   (*table)(0,4)  << COLHDR(_T("c a")<<Sub(_T("cv"))<<_T(" + ")<<rptNewLine<<symbol(mu)<<_T("[a")<<Sub(_T("vf "))<<_T("f")<<Sub(_T("y"))<<_T(" + p")<<Sub(_T("c"))<<_T("]"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

   if ( spec2007OrOlder )
   {
      (*table)(0,5)  << COLHDR(Sub2(_T("K"),_T("1")) << RPT_FC << Sub2(_T("a"),_T("cv")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
      (*table)(0,6)  << COLHDR(Sub2(_T("K"),_T("2")) << Sub2(_T("a"),_T("cv")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   }
   else
   {
      (*table)(0,5)  << COLHDR(_T("0.2 f'")<<Sub(_T("c"))<<_T("a")<<Sub(_T("cv")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
      if ( IS_SI_UNITS(pDisplayUnits) )
         (*table)(0,6)  << COLHDR(_T("5.5 a")<<Sub(_T("cv")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
      else
         (*table)(0,6)  << COLHDR(_T("0.8 a")<<Sub(_T("cv")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   }

   (*table)(0,7)  << COLHDR(symbol(phi) << Sub2(_T("v"),_T("ni")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

   // Fill up the tables
   Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));

   RowIndexType vui_row = vui_table->GetNumberOfHeaderRows();
   RowIndexType av_row  = av_table->GetNumberOfHeaderRows();
   RowIndexType row     = table->GetNumberOfHeaderRows();

   // these are needed later
   bool is_roughened (false);
   bool do_all_stirrups_engage_deck(false);

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
      const pgsStirrupCheckArtifact* pstirrup_artifact= pSegmentArtifact->GetStirrupCheckArtifact();
      ATLASSERT(pstirrup_artifact);

      CollectionIndexType nArtifacts = pstirrup_artifact->GetStirrupCheckAtPoisArtifactCount(intervalIdx,ls);
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( intervalIdx, ls, idx );

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();
         ATLASSERT(poi.GetSegmentKey() == segmentKey);

         const pgsHorizontalShearArtifact* pArtifact = psArtifact->GetHorizontalShearArtifact();

         // Don't report values in vui and capacity table for poi's in end zone outside of CSS
         bool is_app = pArtifact->IsApplicable();

         is_roughened |= pArtifact->IsTopFlangeRoughened();
         do_all_stirrups_engage_deck |= pArtifact->DoAllPrimaryStirrupsEngageDeck();


         if (is_app)
         {
            // vui table
            col = 0;
            Float64 Vui = pArtifact->GetDemand();
            (*vui_table)(vui_row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

            if ( pSpecEntry->GetShearFlowMethod() == sfmLRFD )
            {
               (*vui_table)(vui_row,col++) << dim.SetValue( pArtifact->GetDv() );
            }
            else
            {
               (*vui_table)(vui_row,col++) << l4.SetValue( pArtifact->GetI() );
               (*vui_table)(vui_row,col++) << l3.SetValue( pArtifact->GetQ() );
            }

            (*vui_table)(vui_row,col++) << shear.SetValue( pArtifact->GetVu() );
            (*vui_table)(vui_row,col++) << shear_per_length.SetValue(Vui);
            (*vui_table)(vui_row,col++) << dim.SetValue(pArtifact->GetBv());
            (*vui_table)(vui_row,col++) << stress.SetValue(Vui/pArtifact->GetBv());

            vui_row++;
         }

         // av/s table
         (*av_table)(av_row,0)  <<  location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
         (*av_table)(av_row,1)  <<  area.SetValue(pArtifact->GetAvfGirder());

         Float64 sv = pArtifact->GetSGirder();
         if (sv>0.0)
            (*av_table)(av_row,2)  <<  dim.SetValue(sv);
         else
            (*av_table)(av_row,2)  <<  symbol(INFINITY);

         (*av_table)(av_row,3)  <<  area.SetValue(pArtifact->GetAvfAdditional());

         sv = pArtifact->GetSAdditional();
         if (sv>0.0)
            (*av_table)(av_row,4)  <<  dim.SetValue(sv);
         else
            (*av_table)(av_row,4)  <<  symbol(INFINITY);

         (*av_table)(av_row,5)  <<  AvS.SetValue(pArtifact->GetAvOverS());

         av_row++;

         if (is_app)
         {
            // capacity table
            (*table)(row,0) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
            (*table)(row,1) << AvS.SetValue(pArtifact->GetAcv());
            (*table)(row,2) << AvS.SetValue(pArtifact->GetAvOverS());
            (*table)(row,3) << shear_per_length.SetValue( pArtifact->GetNormalCompressionForce() );

            Float64 Vn1, Vn2, Vn3; 
            pArtifact->GetVn(&Vn1, &Vn2, &Vn3);

            (*table)(row,4) << shear_per_length.SetValue( Vn1 );
            (*table)(row,5) << shear_per_length.SetValue( Vn2 );
            (*table)(row,6) << shear_per_length.SetValue( Vn3 ); 
            (*table)(row,7) << shear_per_length.SetValue( pArtifact->GetCapacity() );

            row++;
         }
      } // next artifact
   } // next segment

   // Next, fill table for min Avf
   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Details for Minimum Horizontal Interface Shear Reinforcement [5.8.4.4]")<<rptNewLine;

   rptParagraph* pParaEqn = new rptParagraph(); // for equation
   *pChapter << pParaEqn;
   pPara = new rptParagraph();
   *pChapter << pPara;

   Float64 llss = lrfdConcreteUtil::LowerLimitOfShearStrength(is_roughened,do_all_stirrups_engage_deck);
   if ( 0 < llss )
   {
      *pPara << _T("Girder/slab interfaces are intentionally roughened and all primary vertical shear reinforcement is extended across the interface. ")
             << _T("Hence, the minimum reinforcement requirement of ") 
             << Sub2(_T("a"),_T("vf")) << _T(" may be waived if ");

      if ( spec2007OrOlder )
      {
         *pPara << Sub2(_T("v"),_T("ui"));
      }
      else
      {
         *pPara << Sub2(_T("v"),_T("ni")) << _T("/") << Sub2(_T("a"),_T("cv"));
      }

      *pPara << _T(" is less than ") << stress_with_tag.SetValue(llss) << rptNewLine;
   }

   table = pgsReportStyleHolder::CreateDefaultTable(spec2007OrOlder ? 7 : 5,_T(""));
   *pPara << table << rptNewLine;

   if ( pBridge->GetDeckType() != pgsTypes::sdtNone && !do_all_stirrups_engage_deck )
   {
      *pPara << _T("Minimum reinforcement requirements cannot be waived. All of the primary vertical shear reinforcement does not extend across the girder/slab interface.") << rptNewLine;
   }

   //if ( span == ALL_SPANS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   col = 0;
   (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++)  << COLHDR(_T("a") << Sub(_T("cv")) , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );

   if ( spec2007OrOlder )
   {
      (*table)(0,col++)  << COLHDR(Sub2(_T("v"),_T("ui")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      (*table)(0,col++)<<COLHDR(Sub2(_T("a"),_T("vf min")) << rptNewLine << _T("(5.8.4.4-1)"), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
      (*table)(0,col++)<<COLHDR(Sub2(_T("a"),_T("vf min")) << rptNewLine << _T("(5.8.4.1-3)"), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
      (*table)(0,col++)<<COLHDR(Sub2(_T("a"),_T("vf min")), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );

      if ( IS_SI_UNITS(pDisplayUnits) )
         *pParaEqn << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("AvfMin_SI.png")) << rptNewLine;
      else
         *pParaEqn << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("AvfMin_US.png")) << rptNewLine;
   }
   else
   {
      (*table)(0,col++)  << COLHDR(Sub2(_T("v"),_T("ni")) << _T("/") << Sub2(_T("a"),_T("cv")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      if ( IS_SI_UNITS(pDisplayUnits) )
         (*table)(0,col++)<<COLHDR(Sub2(_T("a"),_T("vf min")) << _T(" = ") << Sub2(_T("0.35a"),_T("cv")) <<_T("/") << RPT_FY, rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
      else
         (*table)(0,col++)<<COLHDR(Sub2(_T("a"),_T("vf min")) << _T(" = ") << Sub2(_T("0.05a"),_T("cv")) <<_T("/") << RPT_FY, rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   }

   (*table)(0,col)  << _T("Min Reinforcement") << rptNewLine << _T("Requirement") << rptNewLine << _T("Waived?");

   // Fill up the table
   row = 1;
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
      const pgsStirrupCheckArtifact* pstirrup_artifact= pSegmentArtifact->GetStirrupCheckArtifact();
      ATLASSERT(pstirrup_artifact);

      CollectionIndexType nArtifacts = pstirrup_artifact->GetStirrupCheckAtPoisArtifactCount(intervalIdx,ls);
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         col = 0;
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( intervalIdx, ls, idx );
         if ( psArtifact == NULL )
            continue;

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();
         ATLASSERT(poi.GetSegmentKey() == segmentKey);

         const pgsHorizontalShearArtifact* pArtifact = psArtifact->GetHorizontalShearArtifact();

         // Don't report values in vui and capacity table for poi's in end zone outside of CSS
         if( !pArtifact->IsApplicable() )
            continue;

         (*table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
         (*table)(row,col++) << AvS.SetValue(pArtifact->GetAcv());
         (*table)(row,col++) << stress.SetValue( pArtifact->GetVsAvg() );

         if ( spec2007OrOlder )
         {
            (*table)(row,col++) << AvS.SetValue(pArtifact->GetAvOverSMin_5_8_4_4_1());
            (*table)(row,col++) << AvS.SetValue(pArtifact->GetAvOverSMin_5_8_4_1_3());
         }

         (*table)(row,col++) << AvS.SetValue(pArtifact->GetAvOverSMin());
         (*table)(row,col++) << (pArtifact->GetVsAvg() < llss ? _T("Yes") : _T("No"));

         row++;
      } // next artifact
   } // next segment

   // delete the array of points, not the pointers themself
   delete[] p_first_artifact;
   delete[] p_first_sartifact;
}
