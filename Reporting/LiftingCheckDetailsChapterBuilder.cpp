///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <PgsExt\CapacityToDemand.h>

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
CLiftingCheckDetailsChapterBuilder::CLiftingCheckDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
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

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());
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

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Details for Check for Lifting In Casting Yard [5.5.4.3]")<<rptNewLine;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;


   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   if (!pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
   {
      *p <<color(Red)<<_T("Lifting analysis disabled in Project Criteria library entry. No analysis performed.")<<color(Black)<<rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,girder);
   const pgsLiftingAnalysisArtifact* pLift = pArtifact->GetLiftingAnalysisArtifact();

   Float64 bracket_hgt = pGirderLiftingSpecCriteria->GetHeightOfPickPointAboveGirderTop();

   *p << Sub2(_T("L"),_T("g")) << _T(" = Overall Length of girder = ")<<loc.SetValue(pLift->GetGirderLength())<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;

   Float64 leftOH  = pLift->GetLeftOverhang();
   Float64 rightOH = pLift->GetRightOverhang();
   *p << _T("Left Support Overhang = ")<<loc.SetValue(leftOH)<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;
   *p << _T("Right Support Overhang = ")<<loc.SetValue(rightOH)<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;

   *p << Sub2(_T("L"),_T("l"))<<_T(" = Clear span length between pick points = ")<<loc.SetValue(pLift->GetClearSpanBetweenPickPoints())<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;
   *p << _T("w = girder weight/length = ")<<wt_len.SetValue(pLift->GetAvgGirderWeightPerLength())<<_T(" ")<<_T(" ")<<wt_len.GetUnitTag()<<rptNewLine;
   *p << _T("W = girder weight = ")<<force.SetValue(pLift->GetGirderWeight())<<_T(" ")<<_T(" ")<<force.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("I"),_T("x")) << _T(" = ") << mom_I.SetValue(pLift->GetIx()) << rptNewLine;
   *p << Sub2(_T("I"),_T("y")) << _T(" = ") << mom_I.SetValue(pLift->GetIy()) << rptNewLine;
   *p << _T("Height of Pick Point Above Top of Girder = ") << dim.SetValue(bracket_hgt) << _T(" ")<<dim.GetUnitTag() <<rptNewLine;
   *p << _T("y")<<Sub(_T("t"))<<_T(" = Vertical distance from pick point to girder C.G. = ")<<dim.SetValue(pLift->GetVerticalDistanceFromPickPointToGirderCg())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << _T("Upward Impact during lifting = ")<<pLift->GetUpwardImpact()<<rptNewLine;
   *p << _T("Downward Impact during lifting = ")<<pLift->GetDownwardImpact()<<rptNewLine;
   *p << _T("Sweep tolerance = ")<<pLift->GetSweepTolerance()<<loc.GetUnitTag()<<_T("/")<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;
   *p << _T("Tolerance in placement of lifting device =")<<dim.SetValue(pLift->GetLiftingDeviceTolerance())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << RPT_FCI << _T(" = concrete strength = ") << stress.SetValue(pLift->GetConcreteStrength()) << _T(" ") << stress.GetUnitTag() << rptNewLine;
   *p << RPT_STRESS(_T("r"))<<_T(" = modulus of rupture at lifting = ")<<stress.SetValue(pLift->GetModRupture())<<_T(" ")<<stress.GetUnitTag()<<rptNewLine;
   *p << _T("Elastic modulus of girder concrete at lifting = ")<<mod_e.SetValue(pLift->GetElasticModulusOfGirderConcrete())<<_T(" ")<<mod_e.GetUnitTag()<<rptNewLine;
   *p << _T("Additional axial compressive force due to inclination of lifting cables = ")<<force.SetValue(pLift->GetAxialCompressiveForceDueToInclinationOfLiftingCables())<<_T(" ")<<force.GetUnitTag()<<rptNewLine;
   *p << _T("Additional moment in girder due to inclination of lifting cables = ")<<moment.SetValue(pLift->GetMomentInGirderDueToInclinationOfLiftingCables())<<_T(" ")<<moment.GetUnitTag()<<rptNewLine;
   *p << _T("Inclination of lifting cables from horizonal = ") << degangle.SetValue(pLift->GetInclinationOfLiftingCables()) << _T(" ") << degangle.GetUnitTag() << rptNewLine;
   *p << _T("e")<<Sub(_T("s"))<<_T(" = eccentricity due to sweep = ")<<dim.SetValue(pLift->GetEccentricityDueToSweep())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << _T("e")<<Sub(_T("lift"))<<_T(" = eccentricity due to placement tolerance = ")<<dim.SetValue(pLift->GetEccentricityDueToPlacementTolerance())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << _T("F")<<Sub(_T("o"))<<_T(" = offset factor = ") << _T("(") << Sub2(_T("L"),_T("l")) << _T("/") << Sub2(_T("L"),_T("g")) << _T(")") << Super(_T("2")) << _T(" - 1/3 = ") <<pLift->GetOffsetFactor()<<rptNewLine;
   *p << _T("e")<<Sub(_T("i"))<<_T(" = total initial eccentricity = e")<<Sub(_T("s"))<<_T("*F")<<Sub(_T("o"))<<_T(" + e")<<Sub(_T("lift"))<<_T(" = ")<<dim.SetValue(pLift->GetTotalInitialEccentricity())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << Sub2(symbol(DELTA),_T("mid"))  << _T(" = camber due to self weight at mid-span = ") << dim.SetValue(pLift->GetCamberDueToSelfWeight())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << Sub2(symbol(DELTA),_T("ps"))   << _T(" = camber due to prestress mid-span = ")<<dim.SetValue(pLift->GetCamberDueToPrestress())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << Sub2(symbol(DELTA),_T("total"))<< _T(" = total camber at lifting = ")<<symbol(DELTA)<<Sub(_T("mid"))<<_T(" + ")<<symbol(DELTA)<<Sub(_T("ps"))<<_T(" =")<<dim.SetValue(pLift->GetTotalCamberAtLifting())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << _T("Adjusted y")<<Sub(_T("r"))<<_T(" = y")<<Sub(_T("t"))<<_T(" - ")<< Sub2(_T("F"),_T("o")) << _T("(") << symbol(DELTA)<<Sub(_T("total"))<<_T(") = ")<<dim.SetValue(pLift->GetAdjustedYr())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("zo.png") )<<_T(" = ")<<dim.SetValue(pLift->GetZo())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("z"),_T("o")) << _T(" is based on average girder unit weight and mid-span section properties") << rptNewLine;
   *p << symbol(theta)<<Sub(_T("i"))<<_T(" = initial tilt angle = e")<<Sub(_T("i"))<<_T(" / y")<<Sub(_T("r"))<<_T(" = ")<<angle.SetValue(pLift->GetInitialTiltAngle())<<_T(" ")<<angle.GetUnitTag()<<rptNewLine;

   pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;

   *pTitle << _T("Girder Forces and Stresses At Lifting");
   p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(6,_T("Lifting Forces"));

   *p << p_table<<rptNewLine;

   (*p_table)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Pick Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR(_T("Effective") << rptNewLine << _T("Prestress") << rptNewLine << _T("Force"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*p_table)(0,2) << COLHDR(_T("Eccentricity "),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(0,3) << COLHDR(_T("Moment") << rptNewLine << _T("Impact Up"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,4) << COLHDR(_T("Moment") << rptNewLine << _T("No Impact"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,5) << COLHDR(_T("Moment") << rptNewLine << _T("Impact Down"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   Float64 overhang = (pLift->GetGirderLength()-pLift->GetClearSpanBetweenPickPoints())/2.0;

   GET_IFACE2(pBroker,IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
   std::vector<pgsPointOfInterest> poi_vec;
   poi_vec = pGirderLiftingPointsOfInterest->GetLiftingPointsOfInterest(span,girder,POI_FLEXURESTRESS | POI_SECTCHANGE,POIFIND_OR);

   RowIndexType row=1;
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      const pgsLiftingStressAnalysisArtifact* stressArtifact =  pLift->GetLiftingStressAnalysisArtifact(poi.GetDistFromStart());
      if(stressArtifact==NULL)
      {
         ATLASSERT(0);
         continue;
      }
 
      (*p_table)(row,0) << location.SetValue( pgsTypes::Lifting, poi, overhang );
      (*p_table)(row,1) << force.SetValue( stressArtifact->GetEffectiveHorizPsForce());
      (*p_table)(row,2) << dim.SetValue( stressArtifact->GetEccentricityPsForce());
      Float64 up, down, no;
      stressArtifact->GetMomentImpact(&up,&no,&down);
      (*p_table)(row,3) << moment.SetValue(up);
      (*p_table)(row,4) << moment.SetValue(no);
      (*p_table)(row,5) << moment.SetValue(down);
      row++;
   }

   p_table = pgsReportStyleHolder::CreateDefaultTable(9,_T("Lifting Stresses"));
   *p << p_table << rptNewLine;

   p_table->SetNumberOfHeaderRows(2);
   p_table->SetRowSpan(0,0,2);
   p_table->SetRowSpan(1,0,SKIP_CELL);
   (*p_table)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Pick Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   p_table->SetColumnSpan(0,1,4);
   (*p_table)(0,1) << _T("Top Stress, ") << RPT_FTOP;

   p_table->SetColumnSpan(0,2,4);
   (*p_table)(0,2) << _T("Bottom Stress, ") << RPT_FBOT;

   p_table->SetColumnSpan(0,3,SKIP_CELL);
   p_table->SetColumnSpan(0,4,SKIP_CELL);
   p_table->SetColumnSpan(0,5,SKIP_CELL);
   p_table->SetColumnSpan(0,6,SKIP_CELL);
   p_table->SetColumnSpan(0,7,SKIP_CELL);
   p_table->SetColumnSpan(0,8,SKIP_CELL);

   (*p_table)(1,1) << COLHDR(_T("Prestress"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,2) << COLHDR(_T("Impact") << rptNewLine << _T("Up"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,3) << COLHDR(_T("No") << rptNewLine << _T("Impact"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,4) << COLHDR(_T("Impact") << rptNewLine << _T("Down"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,5) << COLHDR(_T("Prestress"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,6) << COLHDR(_T("Impact") << rptNewLine << _T("Up"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,7) << COLHDR(_T("No") << rptNewLine << _T("Impact"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,8) << COLHDR(_T("Impact") << rptNewLine << _T("Down"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   RowIndexType row1=2;
   for (i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      const pgsLiftingStressAnalysisArtifact* stressArtifact =  pLift->GetLiftingStressAnalysisArtifact(poi.GetDistFromStart());
      if(stressArtifact==NULL)
      {
         ATLASSERT(0);
         continue;
      }
 
      (*p_table)(row1,0) << location.SetValue( pgsTypes::Lifting,poi,overhang );
      Float64 ps, up, down, no;
      stressArtifact->GetTopFiberStress(&ps,&up,&no,&down);
      (*p_table)(row1,1) << stress.SetValue( ps );
      (*p_table)(row1,2) << stress.SetValue( up );
      (*p_table)(row1,3) << stress.SetValue( no );
      (*p_table)(row1,4) << stress.SetValue( down );
      stressArtifact->GetBottomFiberStress(&ps,&up,&no,&down);
      (*p_table)(row1,5) << stress.SetValue( ps );
      (*p_table)(row1,6) << stress.SetValue( up );
      (*p_table)(row1,7) << stress.SetValue( no );
      (*p_table)(row1,8) << stress.SetValue( down );
      
      row1++;
   }

   // Rebar requirements tables. Only build table if impact is non zero
   if (pLift->GetDownwardImpact() > 0.0)
      BuildRebarTable(pBroker, pChapter, span, girder, idDown);

   BuildRebarTable(pBroker, pChapter, span, girder, idNone);

   if (pLift->GetUpwardImpact() > 0.0)
      BuildRebarTable(pBroker, pChapter, span, girder, idUp);
   
   p = new rptParagraph;
   *pChapter << p;

   // FS Cracking

   p_table = pgsReportStyleHolder::CreateDefaultTable(7,_T("Factor of Safety Against Cracking"));
   *p << p_table << rptNewLine;
   *p << RPT_STRESS(_T("t")) << _T(" = governing tension stress")<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Mlat.png") )<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ThetaMax.png") )<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("FScrLifting.png") )<<rptNewLine;

   (*p_table)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Pick Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR(RPT_STRESS(_T("t")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,2) << _T("Governing") << rptNewLine << _T("Flange");
   (*p_table)(0,3) << COLHDR(Sub2(_T("M"),_T("lat")),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,4) << COLHDR(Sub2(_T("M"),_T("vert")),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,5) << COLHDR(symbol(theta)<<Sub(_T("max")),rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit() );
   (*p_table)(0,6) << Sub2(_T("FS"),_T("cr"));

   row=1;
   for (i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      const pgsLiftingCrackingAnalysisArtifact* crackArtifact =  pLift->GetLiftingCrackingAnalysisArtifact(poi.GetDistFromStart());
      if (crackArtifact==NULL)
      {
         ATLASSERT(0);
         continue;
      }
 
      (*p_table)(row,0) << location.SetValue( pgsTypes::Lifting, poi, overhang);
      (*p_table)(row,1) << stress.SetValue( crackArtifact->GetLateralMomentStress() );

      if (crackArtifact->GetCrackedFlange()==BottomFlange)
         (*p_table)(row,2) << _T("Bottom");
      else
         (*p_table)(row,2) << _T("Top");

      (*p_table)(row,3) << moment.SetValue( crackArtifact->GetLateralMoment() );
      (*p_table)(row,4) << moment.SetValue( crackArtifact->GetVerticalMoment() );
      (*p_table)(row,5) << angle.SetValue( crackArtifact->GetThetaCrackingMax() );
      (*p_table)(row,6) << scalar.SetValue(crackArtifact->GetFsCracking());
      row++;
   }

   pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Factor of Safety Against Failure")<<rptNewLine;

   p = new rptParagraph;
   *pChapter << p;
   // FS Failure
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ThetaPrimeMaxLifting.png") )<<_T(" = ")<< angle.SetValue(pLift->GetThetaFailureMax())<<_T(" ")<<angle.GetUnitTag()<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("zo_prime_lifting.png") )<<_T(" = ")<<dim.SetValue(pLift->GetZoPrime())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("FSfLifting.png") )<<_T(" = ")<< scalar.SetValue(pLift->GetBasicFsFailure())<<rptNewLine;
   *p << rptNewLine;
   *p << _T("If ") << Sub2(_T("FS"),_T("f")) << _T(" < Minimum ") << Sub2(_T("FS"),_T("cr")) << _T(", ") << Sub2(_T("FS"),_T("f")) << _T(" = Minimum ") << Sub2(_T("FS"),_T("cr")) << rptNewLine;
   *p << Sub2(_T("FS"),_T("f")) << _T(" = ") << scalar.SetValue(pLift->GetFsFailure()) << rptNewLine;

   return pChapter;
}

void CLiftingCheckDetailsChapterBuilder::BuildRebarTable(IBroker* pBroker,rptChapter* pChapter, SpanIndexType span, GirderIndexType girder, 
                                                         ImpactDir dir) const
{
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());
   INIT_UV_PROTOTYPE( rptPointOfInterest, location,       pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(span == ALL_SPANS);
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,          pDisplayUnits->GetShearUnit(),         false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area,        pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );

   rptCapacityToDemand cap_demand;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,girder);
   const pgsLiftingAnalysisArtifact* pLift = pArtifact->GetLiftingAnalysisArtifact();

   Float64 overhang = (pLift->GetGirderLength()-pLift->GetClearSpanBetweenPickPoints())/2.0;

   GET_IFACE2(pBroker,IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
   std::vector<pgsPointOfInterest> vPoi = pGirderLiftingPointsOfInterest->GetLiftingPointsOfInterest(span,girder,POI_FLEXURESTRESS | POI_SECTCHANGE,POIFIND_OR);
   CHECK(vPoi.size()>0);

   std::_tstring tablename;
   if (dir==idDown)
      tablename=_T("Rebar Requirements for Tensile Stress Limit [C5.9.4.1.2] - Lifting, Downward Impact");
   else if (dir==idNone)
      tablename=_T("Rebar Requirements for Tensile Stress Limit [C5.9.4.1.2] - Lifting, Without Impact");
   else
      tablename=_T("Rebar Requirements for Tensile Stress Limit [C5.9.4.1.2] - Lifting, Upward Impact");

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(10,tablename);
   *p << pTable << rptNewLine;

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << COLHDR(_T("Location from") << rptNewLine << _T("Left Pick Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*pTable)(0,col++) << _T("Tension") << rptNewLine << _T("Face");
   (*pTable)(0,col++) << COLHDR(Sub2(_T("Y"),_T("na")),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(_T("f"),_T("ci"))<<rptNewLine<<_T("Demand"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(_T("A"),_T("t")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(0,col++) << COLHDR(_T("T"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(_T("* A"),_T("s"))<< rptNewLine << _T("Provided"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(_T("A"),_T("s"))<< rptNewLine << _T("Required"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Tensile")<<rptNewLine<<_T("Capacity"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,col++) <<_T("Tension") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");

   Int16 row=1;
   for (std::vector<pgsPointOfInterest>::iterator i = vPoi.begin(); i!= vPoi.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      const pgsLiftingStressAnalysisArtifact* stressArtifact =  pLift->GetLiftingStressAnalysisArtifact(poi.GetDistFromStart());
      if(stressArtifact==NULL)
      {
         ATLASSERT(0);
         continue;
      }

      ATLASSERT(pArtifact != NULL);
      if ( pArtifact == NULL )
         continue;

      Float64 Yna, At, T, AsProvd, AsReqd, fAllow;
      stressArtifact->GetAlternativeTensileStressParameters(dir, &Yna, &At, &T, &AsProvd, &AsReqd, &fAllow);

      (*pTable)(row,0) << location.SetValue( pgsTypes::Lifting, poi, overhang );

      if (Yna < 0 )
      {
         // Entire section is in compression - blank out row
         (*pTable)(row,1) << _T("Neither");
         for ( ColumnIndexType ic = 2; ic < pTable->GetNumberOfColumns(); ic++ )
         {
           (*pTable)(row,ic) << _T("-");
         }
      }
      else
      {
         // Stress demand
         Float64 fps;
         Float64 fTopUp, fTopNone, fTopDown;
         Float64 fBottomUp, fBottomNone, fBottomDown;
         stressArtifact->GetTopFiberStress(&fps, &fTopUp, &fTopNone, &fTopDown);
         stressArtifact->GetBottomFiberStress(&fps, &fBottomUp, &fBottomNone, &fBottomDown);

         Float64 fTop, fBot;
         if(dir==idDown)
         {
            fTop = fTopDown;
            fBot = fBottomDown;
         }
         else if(dir==idNone)
         {
            fTop = fTopNone;
            fBot = fBottomNone;
         }
         else
         {
            fTop = fTopUp;
            fBot = fBottomUp;
         }

         Float64 fTens;
         if (fTop>0.0)
         {
            fTens = fTop;
            (*pTable)(row,1) << _T("Top");
         }
         else
         {
            fTens = fBot;
            (*pTable)(row,1) << _T("Bottom");
         }

         (*pTable)(row,2) << dim.SetValue(Yna);
         (*pTable)(row,3) << stress.SetValue(fTens);
         (*pTable)(row,4) << area.SetValue(At);
         (*pTable)(row,5) << force.SetValue(T);
         (*pTable)(row,6) << area.SetValue(AsProvd);
         (*pTable)(row,7) << area.SetValue(AsReqd);
         (*pTable)(row,8) << stress.SetValue(fAllow);
         (*pTable)(row,9) <<_T("(")<< cap_demand.SetValue(fAllow,fTens,true)<<_T(")");
      }

      row++;
   }

   *p << _T("* Bars must be fully developed and lie within tension area of section before they are considered.");
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
