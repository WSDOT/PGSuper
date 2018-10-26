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
#include <Reporting\LiftingCheckDetailsChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\GirderHandlingPointOfInterest.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLiftingCheckDetailsChapterBuilder
****************************************************************************/



////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CLiftingCheckDetailsChapterBuilder::CLiftingCheckDetailsChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CLiftingCheckDetailsChapterBuilder::GetName() const
{
   return TEXT("Lifting Check Details");
}

rptChapter* CLiftingCheckDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );
   INIT_UV_PROTOTYPE( rptPointOfInterest, location,       pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc,            pDisplayUnits->GetSpanLengthUnit(),    false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,          pDisplayUnits->GetShearUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDisplayUnits->GetStressUnit(),        false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, mod_e,           pDisplayUnits->GetModEUnit(),        false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment,         pDisplayUnits->GetMomentUnit(),        false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, angle,           pDisplayUnits->GetRadAngleUnit(),         false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, degangle,        pDisplayUnits->GetAngleUnit(),         false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, wt_len, pDisplayUnits->GetForcePerLengthUnit(),false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area,        pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, mom_I,  pDisplayUnits->GetMomentOfInertiaUnit(),         true );

   location.IncludeSpanAndGirder(span == ALL_SPANS);
   location.MakeSpanPoi();

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << "Details for Check for Lifting In Casting Yard [5.5.4.3]"<<rptNewLine;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;


   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   if (!pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
   {
      *p <<color(Red)<<"Lifting analysis disabled in Project Criteria library entry. No analysis performed."<<color(Black)<<rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,girder);
   const pgsLiftingCheckArtifact* pLift = pArtifact->GetLiftingCheckArtifact();

   Float64 bracket_hgt = pGirderLiftingSpecCriteria->GetHeightOfPickPointAboveGirderTop();

   *p << Sub2("l","g") << " = Overall Length of girder = "<<loc.SetValue(pLift->GetGirderLength())<<" "<<loc.GetUnitTag()<<rptNewLine;

   Float64 leftOH  = pLift->GetLeftOverhang();
   Float64 rightOH = pLift->GetRightOverhang();
   *p << "Left Support Overhang = "<<loc.SetValue(leftOH)<<" "<<loc.GetUnitTag()<<rptNewLine;
   *p << "Right Support Overhang = "<<loc.SetValue(rightOH)<<" "<<loc.GetUnitTag()<<rptNewLine;

   *p << Sub2("l","l")<<" = Clear span length between pick points = "<<loc.SetValue(pLift->GetClearSpanBetweenPickPoints())<<" "<<loc.GetUnitTag()<<rptNewLine;
   *p << "w = girder weight/length = "<<wt_len.SetValue(pLift->GetAvgGirderWeightPerLength())<<" "<<" "<<wt_len.GetUnitTag()<<rptNewLine;
   *p << "W = girder weight = "<<force.SetValue(pLift->GetGirderWeight())<<" "<<" "<<force.GetUnitTag()<<rptNewLine;
   *p << Sub2("I","y") << " = " << mom_I.SetValue(pLift->GetIy()) << rptNewLine;
   *p << "Height of Pick Point Above Top of Girder = " << dim.SetValue(bracket_hgt) << " "<<dim.GetUnitTag() <<rptNewLine;
   *p << "y"<<Sub("t")<<" = Vertical distance from pick point to girder C.G. = "<<dim.SetValue(pLift->GetVerticalDistanceFromPickPointToGirderCg())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << "Upward Impact during lifting = "<<pLift->GetUpwardImpact()<<rptNewLine;
   *p << "Downward Impact during lifting = "<<pLift->GetDownwardImpact()<<rptNewLine;
   *p << "Sweep tolerance = "<<pLift->GetSweepTolerance()<<loc.GetUnitTag()<<"/"<<" "<<loc.GetUnitTag()<<rptNewLine;
   *p << "Tolerance in placement of lifting device ="<<dim.SetValue(pLift->GetLiftingDeviceTolerance())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << "f" << Sub("ci") << " = concrete strength = " << stress.SetValue(pLift->GetConcreteStrength()) << " " << stress.GetUnitTag() << rptNewLine;
   *p << "f"<<Sub("r")<<" = modulus of rupture at lifting = "<<stress.SetValue(pLift->GetModRupture())<<" "<<stress.GetUnitTag()<<rptNewLine;
   *p << "Elastic modulus of girder concrete at lifting = "<<mod_e.SetValue(pLift->GetElasticModulusOfGirderConcrete())<<" "<<mod_e.GetUnitTag()<<rptNewLine;
   *p << "Additional axial compressive force due to inclination of lifting cables = "<<force.SetValue(pLift->GetAxialCompressiveForceDueToInclinationOfLiftingCables())<<" "<<force.GetUnitTag()<<rptNewLine;
   *p << "Additional moment in girder due to inclination of lifting cables = "<<moment.SetValue(pLift->GetMomentInGirderDueToInclinationOfLiftingCables())<<" "<<moment.GetUnitTag()<<rptNewLine;
   *p << "Inclination of lifting cables from horizonal = " << degangle.SetValue(pLift->GetInclinationOfLiftingCables()) << " " << degangle.GetUnitTag() << rptNewLine;
   *p << "e"<<Sub("s")<<" = eccentricity due to sweep = "<<dim.SetValue(pLift->GetEccentricityDueToSweep())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << "e"<<Sub("lift")<<" = eccentricity due to placement tolerance = "<<dim.SetValue(pLift->GetEccentricityDueToPlacementTolerance())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << "F"<<Sub("o")<<" = offset factor = " << "(" << Sub2("l","l") << "/" << Sub2("l","g") << ")" << Super("2") << " - 1/3 = " <<pLift->GetOffsetFactor()<<rptNewLine;
   *p << "e"<<Sub("i")<<" = total initial eccentricity = e"<<Sub("s")<<"*F"<<Sub("o")<<" + e"<<Sub("lift")<<" = "<<dim.SetValue(pLift->GetTotalInitialEccentricity())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << symbol(DELTA)<<Sub("end")<<" = camber due to self weight at cantilever end ="<<dim.SetValue(pLift->GetCamberDueToSelfWeightOverhang())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << symbol(DELTA)<<Sub("mid")<<" = camber due to self weight at mid-span ="<<dim.SetValue(pLift->GetCamberDueToSelfWeight())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << symbol(DELTA)<<Sub("ps")<<" = camber due to prestress mid-span = "<<dim.SetValue(pLift->GetCamberDueToPrestress())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << symbol(DELTA)<<Sub("total")<< "= total camber at lifting = "<<symbol(DELTA)<<Sub("mid")<<" - "<<symbol(DELTA)<<Sub("end")<<" + "<<symbol(DELTA)<<Sub("ps")<<" ="<<dim.SetValue(pLift->GetTotalCamberAtLifting())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << "Adjusted y"<<Sub("r")<<" = y"<<Sub("t")<<" - "<< Sub2("F","o") << "(" << symbol(DELTA)<<Sub("total")<<") = "<<dim.SetValue(pLift->GetAdjustedYr())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Zo - Lifting.jpg" )<<" = "<<dim.SetValue(pLift->GetZo())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << Sub2("z","o") << " is based on average girder unit weight and mid-span section properties" << rptNewLine;
   *p << symbol(theta)<<Sub("i")<<" = initial tilt angle = e"<<Sub("i")<<" / y"<<Sub("r")<<" = "<<angle.SetValue(pLift->GetInitialTiltAngle())<<" "<<angle.GetUnitTag()<<rptNewLine;

   pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;

   *pTitle << "Girder Forces and Stresses At Lifting";
   p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(6,"Lifting Forces");

   *p << p_table<<rptNewLine;

   (*p_table)(0,0) << COLHDR("Location from" << rptNewLine << "Left Pick Point",    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR("Effective" << rptNewLine << "Prestress" << rptNewLine << "Force",rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*p_table)(0,2) << COLHDR("Eccentricity ",rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(0,3) << COLHDR("Moment" << rptNewLine << "Impact Up",rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,4) << COLHDR("Moment" << rptNewLine << "No Impact",rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,5) << COLHDR("Moment" << rptNewLine << "Impact Down",rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   Float64 overhang = (pLift->GetGirderLength()-pLift->GetClearSpanBetweenPickPoints())/2.0;

   GET_IFACE2(pBroker,IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
   std::vector<pgsPointOfInterest> poi_vec;
   poi_vec = pGirderLiftingPointsOfInterest->GetLiftingPointsOfInterest(span,girder,POI_FLEXURESTRESS);

   RowIndexType row=1;
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      pgsLiftingStressCheckArtifact stressArtifact =  pLift->GetLiftingStressCheckArtifact(poi.GetDistFromStart());
 
      (*p_table)(row,0) << location.SetValue( poi, overhang );
      (*p_table)(row,1) << force.SetValue( stressArtifact.GetEffectiveHorizPsForce());
      (*p_table)(row,2) << dim.SetValue( stressArtifact.GetEccentricityPsForce());
      Float64 up, down, no;
      stressArtifact.GetMomentImpact(&up,&no,&down);
      (*p_table)(row,3) << moment.SetValue(up);
      (*p_table)(row,4) << moment.SetValue(no);
      (*p_table)(row,5) << moment.SetValue(down);
      row++;
   }

   p_table = pgsReportStyleHolder::CreateDefaultTable(9,"Lifting Stresses");
   *p << p_table << rptNewLine;

   p_table->SetNumberOfHeaderRows(2);
   p_table->SetRowSpan(0,0,2);
   p_table->SetRowSpan(1,0,-1);
   (*p_table)(0,0) << COLHDR("Location from" << rptNewLine << "Left Pick Point",    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   p_table->SetColumnSpan(0,1,4);
   (*p_table)(0,1) << "Top Stress, " << RPT_FTOP;

   p_table->SetColumnSpan(0,2,4);
   (*p_table)(0,2) << "Bottom Stress, " << RPT_FBOT;

   p_table->SetColumnSpan(0,3,-1);
   p_table->SetColumnSpan(0,4,-1);
   p_table->SetColumnSpan(0,5,-1);
   p_table->SetColumnSpan(0,6,-1);
   p_table->SetColumnSpan(0,7,-1);
   p_table->SetColumnSpan(0,8,-1);

   (*p_table)(1,1) << COLHDR("Prestress",rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,2) << COLHDR("Impact" << rptNewLine << "Up",rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,3) << COLHDR("No" << rptNewLine << "Impact",rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,4) << COLHDR("Impact" << rptNewLine << "Down",rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,5) << COLHDR("Prestress",rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,6) << COLHDR("Impact" << rptNewLine << "Up",rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,7) << COLHDR("No" << rptNewLine << "Impact",rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,8) << COLHDR("Impact" << rptNewLine << "Down",rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   rptRcTable* p_table2 = pgsReportStyleHolder::CreateDefaultTable(7,"Rebar Requirements for Tensile Stress Limit [C5.9.4.1.2]");
   *p << p_table2 << rptNewLine;
   (*p_table2)(0,0) << COLHDR("Location from" << rptNewLine << "Left Pick Point",    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table2)(0,1) << COLHDR(Sub2("Y","na")<<rptNewLine<<" Impact" << rptNewLine << "Up",rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table2)(0,2) << COLHDR(Sub2("Y","na")<<rptNewLine<<" No" << rptNewLine << "Impact",rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table2)(0,3) << COLHDR(Sub2("Y","na")<<rptNewLine<<" Impact" << rptNewLine << "Down",rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table2)(0,4) << COLHDR(Sub2("A","t"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*p_table2)(0,5) << COLHDR("T",rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*p_table2)(0,6) << COLHDR(Sub2("A","s"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );

   RowIndexType row1=2;
   RowIndexType row2=1;
   for (i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      pgsLiftingStressCheckArtifact stressArtifact =  pLift->GetLiftingStressCheckArtifact(poi.GetDistFromStart());
 
      (*p_table)(row1,0) << location.SetValue( poi,overhang );
      Float64 ps, up, down, no;
      stressArtifact.GetTopFiberStress(&ps,&up,&no,&down);
      (*p_table)(row1,1) << stress.SetValue( ps );
      (*p_table)(row1,2) << stress.SetValue( up );
      (*p_table)(row1,3) << stress.SetValue( no );
      (*p_table)(row1,4) << stress.SetValue( down );
      stressArtifact.GetBottomFiberStress(&ps,&up,&no,&down);
      (*p_table)(row1,5) << stress.SetValue( ps );
      (*p_table)(row1,6) << stress.SetValue( up );
      (*p_table)(row1,7) << stress.SetValue( no );
      (*p_table)(row1,8) << stress.SetValue( down );
      
 
      (*p_table2)(row2,0) << location.SetValue( poi, overhang );

      Float64 YnaUp,YnaNone,YnaDown,At,T,As;
      stressArtifact.GetAlternativeTensileStressParameters(&YnaUp,&YnaNone,&YnaDown,&At,&T,&As);

      if (YnaUp < 0 )
          (*p_table2)(row2,1) << "-";
      else
         (*p_table2)(row2,1) << dim.SetValue(YnaUp);

      if (YnaNone < 0 )
          (*p_table2)(row2,2) << "-";
      else
         (*p_table2)(row2,2) << dim.SetValue(YnaNone);

      if (YnaDown < 0 )
          (*p_table2)(row2,3) << "-";
      else
         (*p_table2)(row2,3) << dim.SetValue(YnaDown);

      (*p_table2)(row2,4) << area.SetValue(At);
      (*p_table2)(row2,5) << force.SetValue(T);
      (*p_table2)(row2,6) << area.SetValue(As);

      row1++;
      row2++;
   }

   
   p = new rptParagraph;
   *pChapter << p;

   // FS Cracking

   p_table = pgsReportStyleHolder::CreateDefaultTable(7,"Factor of Safety Against Cracking");
   *p << p_table << rptNewLine;
   *p << Sub2("f","t") << " = governing tension stress"<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Mlat - Lifting.jpg" )<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Theta Max - Lifting.jpg" )<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + "FS Cracking - Lifting.jpg" )<<rptNewLine;

   (*p_table)(0,0) << COLHDR("Location from" << rptNewLine << "Left Pick Point",    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR("f"<<Sub("t"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,2) << "Governing" << rptNewLine << "Flange";
   (*p_table)(0,3) << COLHDR(Sub2("M","lat"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,4) << COLHDR(Sub2("M","vert"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,5) << COLHDR(symbol(theta)<<Sub("max"),rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit() );
   (*p_table)(0,6) << Sub2("FS","cr");

   row=1;
   for (i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      pgsLiftingCrackingCheckArtifact crackArtifact =  pLift->GetLiftingCrackingCheckArtifact(poi.GetDistFromStart());
 
      (*p_table)(row,0) << location.SetValue( poi,overhang);
      (*p_table)(row,1) << stress.SetValue( crackArtifact.GetLateralMomentStress() );

      if (crackArtifact.GetCrackedFlange()==BottomFlange)
         (*p_table)(row,2) << "Bottom";
      else
         (*p_table)(row,2) << "Top";

      (*p_table)(row,3) << moment.SetValue( crackArtifact.GetLateralMoment() );
      (*p_table)(row,4) << moment.SetValue( crackArtifact.GetVerticalMoment() );
      (*p_table)(row,5) << angle.SetValue( crackArtifact.GetThetaCrackingMax() );
      (*p_table)(row,6) << scalar.SetValue(crackArtifact.GetFsCracking());
      row++;
   }

   pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << "Factor of Safety Against Failure"<<rptNewLine;

   p = new rptParagraph;
   *pChapter << p;
   // FS Failure
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Theta Prime Max - Lifting.jpg" )<<" = "<< angle.SetValue(pLift->GetThetaFailureMax())<<" "<<angle.GetUnitTag()<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Zo Prime - Lifting.jpg" )<<" = "<<dim.SetValue(pLift->GetZoPrime())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + "FS Failure - Lifting.jpg" )<<" = "<< scalar.SetValue(pLift->GetBasicFsFailure())<<rptNewLine;
   *p << rptNewLine;
   *p << "If " << Sub2("FS","f") << " < Minimum " << Sub2("FS","cr") << ", " << Sub2("FS","f") << " = Minimum " << Sub2("FS","cr") << rptNewLine;
   *p << Sub2("FS","f") << " = " << scalar.SetValue(pLift->GetFsFailure()) << rptNewLine;

   return pChapter;
}


CChapterBuilder* CLiftingCheckDetailsChapterBuilder::Clone() const
{
   return new CLiftingCheckDetailsChapterBuilder;
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
