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
#include <Reporting\InterfaceShearDetails.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
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
                                  IDisplayUnits* pDisplayUnits,
                                  pgsTypes::Stage stage,
                                  pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IStirrupGeometry, pStirrupGeometry);
   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);

   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsStirrupCheckArtifact* pstirrup_artifact= gdrArtifact->GetStirrupCheckArtifact();
   CHECK(pstirrup_artifact);

   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( stage, span, girder, POI_TABULAR|POI_SHEAR );

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

   if (ls==pgsTypes::StrengthI)
      *pPara << "Details for Horizontal Interface Shear Capacity (Strength I) [5.8.4.1]"<<rptNewLine;
   else if (ls==pgsTypes::StrengthII)
      *pPara << "Details for Horizontal Interface Shear Capacity (Strength II) [5.8.4.1]"<<rptNewLine;
   else
      ATLASSERT(false);

   pPara = new rptParagraph();
   *pChapter << pPara;

   ColumnIndexType nCol = pSpecEntry->GetShearFlowMethod() == sfmLRFD ? 6 : 7;

   rptRcTable* vui_table = pgsReportStyleHolder::CreateDefaultTable(nCol,"");
   *pPara << vui_table << rptNewLine;

   ColumnIndexType col = 0;

   if ( stage == pgsTypes::CastingYard )
      (*vui_table)(0,col++)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*vui_table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   if ( pSpecEntry->GetShearFlowMethod() == sfmLRFD )
   {
     (*vui_table)(0,col++) << COLHDR(Sub2("d","vi"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   }
   else
   {
      (*vui_table)(0,col++) << COLHDR("I",rptLength4UnitTag,pDisplayUnits->GetMomentOfInertiaUnit());
      (*vui_table)(0,col++) << COLHDR(Sub2("Q","slab"),rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit());
   }

   (*vui_table)(0,col++) << COLHDR(Sub2("V","u"),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());

   if ( pSpecEntry->GetShearFlowMethod() == sfmLRFD )
   {
      (*vui_table)(0,col++) << COLHDR(Sub2("v","ui") << " = " << Sub2("V","u") << "/" << Sub2("d","vi"),rptForcePerLengthUnitTag,pDisplayUnits->GetForcePerLengthUnit());
   }
   else
   {
      (*vui_table)(0,col++) << COLHDR(Sub2("v","ui") << " = " << Sub2("V","u") << "Q/I",rptForcePerLengthUnitTag,pDisplayUnits->GetForcePerLengthUnit());
   }

   (*vui_table)(0,col++) << COLHDR(Sub2("b","vi"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*vui_table)(0,col++) << COLHDR(Sub2(symbol(nu),"ui"),rptStressUnitTag,pDisplayUnits->GetStressUnit());

   if ( pSpecEntry->GetShearFlowMethod() == sfmLRFD )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2("d","vi") << " = " << Sub2("Y","t girder") << " + Strand Eccentricity + " << Sub2("t","slab") << "/2" << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;
   }

   // TRICKY: create av/s table to be filled in same loop as next table
   rptRcTable* av_table = pgsReportStyleHolder::CreateDefaultTable(6,"");
   *pPara << av_table;

   if ( stage == pgsTypes::CastingYard )
      (*av_table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*av_table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*av_table)(0,1)  << COLHDR("A" << Sub("vf")<<rptNewLine<<"Girder" , rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*av_table)(0,2)  << COLHDR("S"<<rptNewLine<<"Girder", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*av_table)(0,3)  << COLHDR("A" << Sub("vf")<<rptNewLine<<"Top Flange" , rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*av_table)(0,4)  << COLHDR("S"<<rptNewLine<<"Top Flange", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*av_table)(0,5)  << COLHDR("a" << Sub("vf")<<rptNewLine<<"Composite" , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );

   // general quantities
   double Es,Fy;
   pMaterial->GetTransverseRebarProperties(span,girder,&Es,&Fy);

   *pPara << "Coeff. of Friction ("<<symbol(mu)<<") = "<< p_first_artifact->GetFrictionFactor()<<rptNewLine;
   *pPara << "Cohesion Factor (c) = "<< stress_with_tag.SetValue(p_first_artifact->GetCohesionFactor())<<rptNewLine;

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
   {
      *pPara << Sub2("K","1") << " = " << p_first_artifact->GetK1() << rptNewLine;
      *pPara << Sub2("K","2") << " = " << stress_with_tag.SetValue(p_first_artifact->GetK2()) << rptNewLine;
   }

   *pPara << RPT_FC<<" = "<<stress_with_tag.SetValue(p_first_artifact->GetFc())<<rptNewLine;
   *pPara << RPT_FY<<" = "<<stress_with_tag.SetValue(Fy)<<rptNewLine;
   *pPara << symbol(phi)<<" = "<< p_first_artifact->GetPhi()<<rptNewLine;

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
   {
      *pPara << Sub2("v","ni")<<" = min( c" << Sub2("a","cv")<<" + "<<symbol(mu)<<"[ " << Sub2("a","vf")<< Sub2("f","y")<<" + " << Sub2("p","c")<<"], "
                                       << Sub2("K","1") << RPT_FC << Sub2("a","cv")<<", " << Sub2("K","2") << Sub2("a","cv")<<" )"<<rptNewLine;
   }
   else
   {
      *pPara << Sub2("v","ni")<<" = min( ca"<<Sub("cv")<<" + "<<symbol(mu)<<"[ a"<<Sub("vf ")<<"f"<<Sub("y")<<" + p"<<Sub("c")<<"], "
                                       <<"0.2 f'"<<Sub("c")<<"a"<<Sub("cv")<<", ";

      if ( IS_SI_UNITS(pDisplayUnits) )
         *pPara<<" 5.5 a"<<Sub("cv")<<" )"<<rptNewLine;
      else
         *pPara<<" 0.8 a"<<Sub("cv")<<" )"<<rptNewLine;
   }

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(8,"");
   *pPara << table;

 
   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR(Sub2("a","cv") , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0,2)  << COLHDR(Sub2("a","vf") , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0,3)  << COLHDR(Sub2("p","c"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   (*table)(0,4)  << COLHDR("c a"<<Sub("cv")<<" + "<<rptNewLine<<symbol(mu)<<"[a"<<Sub("vf ")<<"f"<<Sub("y")<<" + p"<<Sub("c")<<"]", rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
   {
      (*table)(0,5)  << COLHDR(Sub2("K","1") << RPT_FC << Sub2("c","cv"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
      (*table)(0,6)  << COLHDR(Sub2("K","2") << Sub2("a","cv"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   }
   else
   {
      (*table)(0,5)  << COLHDR("0.2 f'"<<Sub("c")<<"a"<<Sub("cv"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
      if ( IS_SI_UNITS(pDisplayUnits) )
         (*table)(0,6)  << COLHDR("5.5 a"<<Sub("cv"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
      else
         (*table)(0,6)  << COLHDR("0.8 a"<<Sub("cv"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   }

   (*table)(0,7)  << COLHDR(symbol(phi) << Sub2("v","ni"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );

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
      (*vui_table)(vui_row,col++) << location.SetValue( poi, end_size );

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
      (*av_table)(av_row,0)  <<  location.SetValue( poi, end_size );
      (*av_table)(av_row,1)  <<  area.SetValue(pArtifact->GetAvfGirder());
      (*av_table)(av_row,2)  <<  dim.SetValue(pArtifact->GetSGirder());
      (*av_table)(av_row,3)  <<  area.SetValue(pArtifact->GetAvfTopFlange());
      (*av_table)(av_row,4)  <<  dim.SetValue(pArtifact->GetSTopFlange());
      (*av_table)(av_row,5)  <<  AvS.SetValue(pArtifact->GetAvOverS());

      av_row++;

      // capacity table
      (*table)(row,0) << location.SetValue( poi, end_size );
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
   *pPara << "Details for Minimum Horizontal Interface Shear Steel"<<rptNewLine;

   pPara = new rptParagraph();
   *pChapter << pPara;

   bool is_roughened = pBridge->AreGirderTopFlangesRoughened();
   Float64 llss = lrfdConcreteUtil::LowerLimitOfShearStrength(is_roughened);
   if ( 0 < llss )
   {
      *pPara << "The minimum reinforcement requirement of " 
             << Sub2("a","vf") << " may be waived if " 
             << Sub2("v","ni") << "/" << Sub2("a","cv") 
             << " is less than " << stress_with_tag.SetValue(llss) << rptNewLine;
   }

   table = pgsReportStyleHolder::CreateDefaultTable(6,"");
   *pPara << table;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR("a" << Sub("cv") , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0,2)  << COLHDR("a" << Sub("vf") , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
   {
      (*table)(0,3)<<COLHDR(Sub2("a","vf min"), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );

      if ( IS_SI_UNITS(pDisplayUnits) )
         *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "AvfMin_SI.gif") << rptNewLine;
      else
         *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "AvfMin_US.gif") << rptNewLine;
   }
   else
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
         (*table)(0,3)<<COLHDR(Sub2("a","vf min") << " = " << Sub2("0.35a","cv") <<"/" << Sub2("f","y") , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
      else
         (*table)(0,3)<<COLHDR(Sub2("a","vf min") << " = " << Sub2("0.05a","cv") <<"/" << Sub2("f","y") , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   }

   (*table)(0,4)  << COLHDR(Sub2("v","ni") << "/" << Sub2("a","cv"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,5)  << "Min Reinforcement" << rptNewLine << "Requirement Waived?";

   // Fill up the table
   row = 1;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      const pgsHorizontalShearArtifact* pArtifact = psArtifact->GetHorizontalShearArtifact();

      (*table)(row,0) << location.SetValue( poi, end_size );
      (*table)(row,1) << AvS.SetValue(pArtifact->GetAcv());
      (*table)(row,2) << AvS.SetValue(pArtifact->GetAvOverS());
      (*table)(row,3) << AvS.SetValue(pArtifact->GetAvOverSMin());
      (*table)(row,4) << stress.SetValue( pArtifact->GetVsAvg() );
      (*table)(row,5) << (pArtifact->GetVsAvg() < llss ? "Yes" : "No");

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
   os << "Dump for CInterfaceShearDetails" << endl;
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
