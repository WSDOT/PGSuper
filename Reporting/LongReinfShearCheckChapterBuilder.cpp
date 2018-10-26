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

#include <Reporting\LongReinfShearCheckChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <IFace\DisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Artifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CLongReinfShearCheckChapterBuilder
****************************************************************************/

   rptParagraph* create_table1(IBroker* pBroker,
                              pgsTypes::Stage stage,
                              pgsTypes::LimitState ls,
                              const std::vector<pgsPointOfInterest>& rPoi,
                              Float64 endSize,
                              const pgsStirrupCheckArtifact* pstirrupArtifact,
                              IDisplayUnits* pDisplayUnits,
                              Uint16 level);

   rptParagraph* create_table2(IBroker* pBroker,
                              pgsTypes::Stage stage,
                              pgsTypes::LimitState ls,
                              const std::vector<pgsPointOfInterest>& rPoi,
                              Float64 endSize,
                              const pgsStirrupCheckArtifact* pstirrupArtifact,
                              IDisplayUnits* pDisplayUnits,
                              Uint16 level);

   rptParagraph* create_table3(IBroker* pBroker,
                              pgsTypes::Stage stage,
                              pgsTypes::LimitState ls,
                              const std::vector<pgsPointOfInterest>& rPoi,
                              Float64 endSize,
                              const pgsStirrupCheckArtifact* pstirrupArtifact,
                              IDisplayUnits* pDisplayUnits,
                              Uint16 level);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CLongReinfShearCheckChapterBuilder::CLongReinfShearCheckChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CLongReinfShearCheckChapterBuilder::GetName() const
{
   return TEXT("Longitudinal Reinforcement for Shear");
}

rptChapter* CLongReinfShearCheckChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);
   
   pgsTypes::Stage stage = pgsTypes::BridgeSite3;
   pgsTypes::LimitState ls = pgsTypes::StrengthI;

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "5.8.3.5" << rptNewLine;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      *pParagraph <<rptRcImage(pgsReportStyleHolder::GetImagePath() + "Longitudinal Reinforcement Check Equation 2005.gif")<<rptNewLine;
   else
      *pParagraph <<rptRcImage(pgsReportStyleHolder::GetImagePath() + "Longitudinal Reinforcement Check Equation.gif")<<rptNewLine;

   pParagraph = new rptParagraph();
   *pChapter << pParagraph;

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaPerLengthValue, avs,      pDisplayUnits->GetAvOverSUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,      pDisplayUnits->GetComponentDimUnit(),    false );

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsStirrupCheckArtifact* pstirrup_artifact= gdrArtifact->GetStirrupCheckArtifact();
   CHECK(pstirrup_artifact);

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( stage, span, girder, POI_TABULAR|POI_SHEAR );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "Strength I" << rptNewLine;

   // tables of details
   rptParagraph* ppar1 = create_table1(pBroker, stage, ls, vPoi, end_size, pstirrup_artifact, pDisplayUnits, level);
   *pChapter <<ppar1;
   pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pParagraph;
   *pParagraph << Super("*") << " " << "Adjusted for development length" << rptNewLine;

   rptParagraph* ppar2 = create_table2(pBroker, stage, ls, vPoi, end_size, pstirrup_artifact, pDisplayUnits, level);
   *pChapter <<ppar2;

   if ( lrfdVersionMgr::SecondEditionWith2000Interims <= lrfdVersionMgr::GetVersion()  )
   {
      pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pParagraph;
      *pParagraph << Super("*") << " " << Sub2("V","s") << " shall not be taken greater than " << Sub2("V","u") << "/" << Sub2(symbol(phi),"v") << rptNewLine;
   }

   rptParagraph* ppar3 = create_table3(pBroker, stage, ls, vPoi, end_size, pstirrup_artifact, pDisplayUnits, level);
   *pChapter <<ppar3;

   if ( bPermit )
   {
      ls = pgsTypes::StrengthII;

      pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pParagraph;
      *pParagraph << "Strength II" << rptNewLine;

      // tables of details
      ppar1 = create_table1(pBroker, stage, ls, vPoi, end_size, pstirrup_artifact, pDisplayUnits, level);
      *pChapter <<ppar1;
      pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pParagraph;
      *pParagraph << Super("*") << " " << "Adjusted for development length" << rptNewLine;

      ppar2 = create_table2(pBroker, stage, ls, vPoi, end_size, pstirrup_artifact, pDisplayUnits, level);
      *pChapter <<ppar2;

      if ( lrfdVersionMgr::SecondEditionWith2000Interims <= lrfdVersionMgr::GetVersion()  )
      {
         pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
         *pChapter << pParagraph;
         *pParagraph << Super("*") << " " << Sub2("V","s") << " shall not be taken greater than " << Sub2("V","u") << "/" << Sub2(symbol(phi),"v") << rptNewLine;
      }

      ppar3 = create_table3(pBroker, stage, ls, vPoi, end_size, pstirrup_artifact, pDisplayUnits, level);
      *pChapter <<ppar3;
   }

   return pChapter;
}


CChapterBuilder* CLongReinfShearCheckChapterBuilder::Clone() const
{
   return new CLongReinfShearCheckChapterBuilder;
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

rptParagraph* create_table1(IBroker* pBroker,
                           pgsTypes::Stage stage,
                           pgsTypes::LimitState ls,
                           const std::vector<pgsPointOfInterest>& rPoi,
                           Float64 endSize,
                           const pgsStirrupCheckArtifact* pstirrupArtifact,
                           IDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,      pDisplayUnits->GetAreaUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,     pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear,  pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment,  pDisplayUnits->GetMomentUnit(), false );

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(8,"Longitudinal Reinforcement Shear Check Details - Table 1 of 3");
   *pParagraph << table;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR(Sub2("A","s"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,2)  << COLHDR("f"<<Sub("y") << Super("*"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,3)  << COLHDR(Sub2("A","ps"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,4)  << COLHDR("f"<<Sub("ps") << Super("*"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,5)  << COLHDR("M"<<Sub("u"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,6)  << COLHDR("d"<<Sub("v"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,7)  << "Flexure" << rptNewLine << Sub2(symbol(phi),"f");

   RowIndexType row = table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = rPoi.begin(); i != rPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrupArtifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      const pgsLongReinfShearArtifact* pArtifact = psArtifact->GetLongReinfShearArtifact();

      if ( pArtifact->IsApplicable() )
      {
         (*table)(row,0) << location.SetValue( poi, endSize );
         (*table)(row,1) << area.SetValue( pArtifact->GetAs());
         (*table)(row,2) << stress.SetValue( pArtifact->GetFy());
         (*table)(row,3) << area.SetValue( pArtifact->GetAps());
         (*table)(row,4) << stress.SetValue( pArtifact->GetFps());
         (*table)(row,5) << moment.SetValue( pArtifact->GetMu());
         (*table)(row,6) << dim.SetValue( pArtifact->GetDv());
         (*table)(row,7) << pArtifact->GetFlexuralPhi();

         row++;
      }
   }

   return pParagraph;
}

rptParagraph* create_table2(IBroker* pBroker,
                           pgsTypes::Stage stage,
                           pgsTypes::LimitState ls,
                           const std::vector<pgsPointOfInterest>& rPoi,
                           Float64 endSize,
                           const pgsStirrupCheckArtifact* pstirrupArtifact,
                           IDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,      pDisplayUnits->GetAreaUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,     pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,  pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment,  pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,     angle, pDisplayUnits->GetAngleUnit(), false );
   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ATLASSERT(pSpecEntry!=0);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(8,"Longitudinal Reinforcement Shear Check Details - Table 2 of 3");
   *pParagraph << table;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR("N"<<Sub("u"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,2)  << "Axial" << rptNewLine << Sub2(symbol(phi),"a");
   (*table)(0,3)  << COLHDR("V"<<Sub("u"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,4)  << "Shear" << rptNewLine << Sub2(symbol(phi),"v");
   
   if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::SecondEditionWith2000Interims )
      (*table)(0,5)  << COLHDR("V"<<Sub("s"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   else
      (*table)(0,5)  << COLHDR("V"<<Sub("s") << Super("*"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   (*table)(0,6)  << COLHDR("V"<<Sub("p"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,7)  << COLHDR(symbol(theta),rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );

   RowIndexType row = table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = rPoi.begin(); i != rPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrupArtifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      const pgsLongReinfShearArtifact* pArtifact = psArtifact->GetLongReinfShearArtifact();

      if ( pArtifact->IsApplicable() )
      {
         (*table)(row,0) << location.SetValue( poi, endSize );
         (*table)(row,1) << shear.SetValue( pArtifact->GetNu());
         (*table)(row,2) << scalar.SetValue( pArtifact->GetAxialPhi());
         (*table)(row,3) << shear.SetValue( pArtifact->GetVu());
         (*table)(row,4) << scalar.SetValue( pArtifact->GetShearPhi());
         (*table)(row,5) << shear.SetValue( pArtifact->GetVs());
         (*table)(row,6) << shear.SetValue( pArtifact->GetVp());
         (*table)(row,7) << angle.SetValue( pArtifact->GetTheta());

         row++;
      }
   }

   return pParagraph;
}


rptParagraph* create_table3(IBroker* pBroker,
                           pgsTypes::Stage stage,
                           pgsTypes::LimitState ls,
                           const std::vector<pgsPointOfInterest>& rPoi,
                           Float64 endSize,
                           const pgsStirrupCheckArtifact* pstirrupArtifact,
                           IDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,      pDisplayUnits->GetAreaUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,     pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,  pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment,  pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,     angle, pDisplayUnits->GetAngleUnit(), false );
   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6); 
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   CHECK(pSpecEntry!=0);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(4,"Longitudinal Reinforcement Shear Check Details - Table 3 of 3");
   *pParagraph << table;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR("Demand",rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,2)  << COLHDR("Capacity",rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,3)  << "Equation";

   RowIndexType row = table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = rPoi.begin(); i != rPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrupArtifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      const pgsLongReinfShearArtifact* pArtifact = psArtifact->GetLongReinfShearArtifact();

      if ( pArtifact->IsApplicable() )
      {
         (*table)(row,0) << location.SetValue( poi, endSize );
         (*table)(row,1) << shear.SetValue( pArtifact->GetDemandForce());
         (*table)(row,2) << shear.SetValue( pArtifact->GetCapacityForce());
         (*table)(row,3) << (pArtifact->GetEquation() == 1 ? "5.8.3.5-1" : "5.8.3.5-2");

         row++;
      }
   }

   return pParagraph;
}


