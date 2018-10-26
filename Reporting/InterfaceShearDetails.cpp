///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>

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


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CInterfaceShearDetails::CInterfaceShearDetails()
{
}

CInterfaceShearDetails::~CInterfaceShearDetails()
{
}

//======================== OPERATORS  =======================================

//======================== OPERATIONS =======================================
void CInterfaceShearDetails::Build( IBroker* pBroker, rptChapter* pChapter,
                                  SpanIndexType span,GirderIndexType girder,
                                  IEAFDisplayUnits* pDisplayUnits,
                                  pgsTypes::Stage stage,
                                  pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IStirrupGeometry, pStirrupGeometry);
   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);

   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsStirrupCheckArtifact* pstirrup_artifact= gdrArtifact->GetStirrupCheckArtifact();
   CHECK(pstirrup_artifact);

   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( span, girder, stage, POI_TABULAR|POI_SHEAR );

   // get poi-independent values
   std::vector<pgsPointOfInterest>::const_iterator ip = vPoi.begin();
   if (ip == vPoi.end())
      return;

   INIT_UV_PROTOTYPE( rptPointOfInterest,         location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,          shear,    pDisplayUnits->GetGeneralForceUnit(),        false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, shear_per_length,    pDisplayUnits->GetForcePerLengthUnit(),        false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,         fy,       pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,         stress,   pDisplayUnits->GetStressUnit(),       false);
   INIT_UV_PROTOTYPE( rptStressUnitValue,         stress_with_tag,  pDisplayUnits->GetStressUnit(),       true);
   INIT_UV_PROTOTYPE( rptAreaPerLengthValue,      AvS,      pDisplayUnits->GetAvOverSUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         dim,      pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,           area,     pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptLength3UnitValue,        l3,       pDisplayUnits->GetSectModulusUnit(), false);
   INIT_UV_PROTOTYPE( rptLength4UnitValue,        l4,       pDisplayUnits->GetMomentOfInertiaUnit(), false);

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   const pgsStirrupCheckAtPoisArtifact* p_first_sartifact = NULL;
   for ( ip = vPoi.begin(); ip != vPoi.end(); ip++ )
   {
      p_first_sartifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,ip->GetDistFromStart()) );
      if ( p_first_sartifact != NULL )
         break;
   }
   const pgsHorizontalShearArtifact* p_first_artifact = p_first_sartifact->GetHorizontalShearArtifact();

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   rptParagraph* pPara;
   // Initial Capacity Table
   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   (*pPara) << _T("Details for Horizontal Interface Shear Capacity (") << OLE2T(pStageMap->GetLimitStateName(ls)) << _T(") [5.8.4.1]") << rptNewLine;

   pPara = new rptParagraph();
   *pChapter << pPara;

   ColumnIndexType nCol = pSpecEntry->GetShearFlowMethod() == sfmLRFD ? 6 : 7;

   rptRcTable* vui_table = pgsReportStyleHolder::CreateDefaultTable(nCol,_T(""));
   *pPara << vui_table << rptNewLine;

   if ( span == ALL_SPANS )
   {
      vui_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      vui_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   ColumnIndexType col = 0;

   if ( stage == pgsTypes::CastingYard )
      (*vui_table)(0,col++)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
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
   rptRcTable* av_table = pgsReportStyleHolder::CreateDefaultTable(6,_T(""));
   *pPara << av_table;

   if ( span == ALL_SPANS )
   {
      av_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      av_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   if ( stage == pgsTypes::CastingYard )
      (*av_table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*av_table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*av_table)(0,1)  << COLHDR(_T("A") << Sub(_T("vf"))<<rptNewLine<<_T("Girder") , rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*av_table)(0,2)  << COLHDR(_T("S")<<rptNewLine<<_T("Girder"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*av_table)(0,3)  << COLHDR(_T("A") << Sub(_T("vf"))<<rptNewLine<<_T("Top Flange") , rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*av_table)(0,4)  << COLHDR(_T("S")<<rptNewLine<<_T("Top Flange"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*av_table)(0,5)  << COLHDR(_T("a") << Sub(_T("vf"))<<rptNewLine<<_T("Composite") , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );

   // general quantities
   Float64 Es,Fy,Fu;
   pMaterial->GetTransverseRebarProperties(span,girder,&Es,&Fy,&Fu);

   *pPara << _T("Coeff. of Friction (")<<symbol(mu)<<_T(") = ")<< p_first_artifact->GetFrictionFactor()<<rptNewLine;
   *pPara << _T("Cohesion Factor (c) = ")<< stress_with_tag.SetValue(p_first_artifact->GetCohesionFactor())<<rptNewLine;

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
   {
      *pPara << Sub2(_T("K"),_T("1")) << _T(" = ") << p_first_artifact->GetK1() << rptNewLine;
      *pPara << Sub2(_T("K"),_T("2")) << _T(" = ") << stress_with_tag.SetValue(p_first_artifact->GetK2()) << rptNewLine;
   }

   *pPara << RPT_FC<<_T(" = ")<<stress_with_tag.SetValue(p_first_artifact->GetFc())<<rptNewLine;
   *pPara << RPT_FY<<_T(" = ")<<stress_with_tag.SetValue(Fy)<<rptNewLine;
   *pPara << symbol(phi)<<_T(" = ")<< p_first_artifact->GetPhi()<<rptNewLine;

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
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

 
   if ( span == ALL_SPANS )
   {
      table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR(Sub2(_T("a"),_T("cv")) , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0,2)  << COLHDR(Sub2(_T("a"),_T("vf")) , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0,3)  << COLHDR(Sub2(_T("p"),_T("c")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   (*table)(0,4)  << COLHDR(_T("c a")<<Sub(_T("cv"))<<_T(" + ")<<rptNewLine<<symbol(mu)<<_T("[a")<<Sub(_T("vf "))<<_T("f")<<Sub(_T("y"))<<_T(" + p")<<Sub(_T("c"))<<_T("]"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
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
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   RowIndexType vui_row = vui_table->GetNumberOfHeaderRows();
   RowIndexType av_row  = av_table->GetNumberOfHeaderRows();
   RowIndexType row     = table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      const pgsHorizontalShearArtifact* pArtifact = psArtifact->GetHorizontalShearArtifact();

      // vui table
      col = 0;
      Float64 Vui = pArtifact->GetDemand();
      (*vui_table)(vui_row,col++) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );

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


      // av/s table
      (*av_table)(av_row,0)  <<  location.SetValue( pgsTypes::BridgeSite3, poi, end_size );
      (*av_table)(av_row,1)  <<  area.SetValue(pArtifact->GetAvfGirder());
      (*av_table)(av_row,2)  <<  dim.SetValue(pArtifact->GetSGirder());
      (*av_table)(av_row,3)  <<  area.SetValue(pArtifact->GetAvfTopFlange());
      (*av_table)(av_row,4)  <<  dim.SetValue(pArtifact->GetSTopFlange());
      (*av_table)(av_row,5)  <<  AvS.SetValue(pArtifact->GetAvOverS());

      av_row++;

      // capacity table
      (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );
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

   // Next, fill table for min Avf
   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Details for Minimum Horizontal Interface Shear Reinforcement [5.8.4.4]")<<rptNewLine;

   pPara = new rptParagraph();
   *pChapter << pPara;

   bool is_roughened = pBridge->AreGirderTopFlangesRoughened(span,girder);
   Float64 llss = lrfdConcreteUtil::LowerLimitOfShearStrength(is_roughened);
   if ( 0 < llss )
   {
      *pPara << _T("The minimum reinforcement requirement of ") 
             << Sub2(_T("a"),_T("vf")) << _T(" may be waived if ") 
             << Sub2(_T("v"),_T("ni")) << _T("/") << Sub2(_T("a"),_T("cv")) 
             << _T(" is less than ") << stress_with_tag.SetValue(llss) << rptNewLine;
   }

   table = pgsReportStyleHolder::CreateDefaultTable(6,_T(""));
   *pPara << table;

   if ( span == ALL_SPANS )
   {
      table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR(_T("a") << Sub(_T("cv")) , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0,2)  << COLHDR(_T("a") << Sub(_T("vf")) , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
   {
      (*table)(0,3)<<COLHDR(Sub2(_T("a"),_T("vf min")), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );

      if ( IS_SI_UNITS(pDisplayUnits) )
         *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("AvfMin_SI.png")) << rptNewLine;
      else
         *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("AvfMin_US.png")) << rptNewLine;
   }
   else
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
         (*table)(0,3)<<COLHDR(Sub2(_T("a"),_T("vf min")) << _T(" = ") << Sub2(_T("0.35a"),_T("cv")) <<_T("/") << RPT_FY, rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
      else
         (*table)(0,3)<<COLHDR(Sub2(_T("a"),_T("vf min")) << _T(" = ") << Sub2(_T("0.05a"),_T("cv")) <<_T("/") << RPT_FY, rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   }

   (*table)(0,4)  << COLHDR(Sub2(_T("v"),_T("ni")) << _T("/") << Sub2(_T("a"),_T("cv")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,5)  << _T("Min Reinforcement") << rptNewLine << _T("Requirement Waived?");

   // Fill up the table
   row = 1;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      const pgsHorizontalShearArtifact* pArtifact = psArtifact->GetHorizontalShearArtifact();

      (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );
      (*table)(row,1) << AvS.SetValue(pArtifact->GetAcv());
      (*table)(row,2) << AvS.SetValue(pArtifact->GetAvOverS());
      (*table)(row,3) << AvS.SetValue(pArtifact->GetAvOverSMin());
      (*table)(row,4) << stress.SetValue( pArtifact->GetVsAvg() );
      (*table)(row,5) << (pArtifact->GetVsAvg() < llss ? _T("Yes") : _T("No"));

      row++;
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
bool CInterfaceShearDetails::AssertValid() const
{
   return true;
}

void CInterfaceShearDetails::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CInterfaceShearDetails") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CInterfaceShearDetails::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CInterfaceShearDetails");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CInterfaceShearDetails");

   TESTME_EPILOG("CInterfaceShearDetails");
}
#endif // _UNITTEST
