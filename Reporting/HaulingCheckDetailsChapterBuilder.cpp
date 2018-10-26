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
#include <Reporting\HaulingCheckDetailsChapterBuilder.h>
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
   CHaulingCheckDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CHaulingCheckDetailsChapterBuilder::CHaulingCheckDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CHaulingCheckDetailsChapterBuilder::GetName() const
{
   return TEXT("Hauling Check Details");
}

rptChapter* CHaulingCheckDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
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
   INIT_UV_PROTOTYPE( rptPointOfInterest, location,       pDisplayUnits->GetSpanLengthUnit(),    false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc,            pDisplayUnits->GetSpanLengthUnit(),    false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,          pDisplayUnits->GetShearUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDisplayUnits->GetStressUnit(),        false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, mod_e,          pDisplayUnits->GetModEUnit(),          false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment,         pDisplayUnits->GetMomentUnit(),        false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, angle,           pDisplayUnits->GetRadAngleUnit(),      false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, wt_len, pDisplayUnits->GetForcePerLengthUnit(),false );
   INIT_UV_PROTOTYPE( rptMomentPerAngleUnitValue, spring, pDisplayUnits->GetMomentPerAngleUnit(),false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area,           pDisplayUnits->GetAreaUnit(),      false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, mom_I,  pDisplayUnits->GetMomentOfInertiaUnit(),         true );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Details for Check for Hauling to Bridge Site [5.5.4.3]")<<rptNewLine;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;


   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if (!pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
   {
      *p <<color(Red)<<_T("Hauling analysis disabled in Project Criteria library entry. No analysis performed.")<<color(Black)<<rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,girder);
   const pgsHaulingCheckArtifact* pHaul = pArtifact->GetHaulingCheckArtifact();

   *p << Sub2(_T("l"),_T("g")) << _T(" = Overall Length of girder = ")<<loc.SetValue(pHaul->GetGirderLength())<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;

   Float64 leadingOH  = pHaul->GetLeadingOverhang();
   Float64 trailingOH = pHaul->GetTrailingOverhang();

   FormatDimension(leadingOH,pDisplayUnits->GetSpanLengthUnit());
   *p << _T("Leading Overhang = ")<<loc.SetValue(leadingOH)<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;
   *p << _T("Trailing Overhang = ")<<loc.SetValue(trailingOH)<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("l"),_T("l"))<<_T(" = Clear span length between supports = ")<<loc.SetValue(pHaul->GetClearSpanBetweenSupportLocations())<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;
   *p << _T("w = average girder weight/length = ")<<wt_len.SetValue(pHaul->GetAvgGirderWeightPerLength())<<_T(" ")<<_T(" ")<<wt_len.GetUnitTag()<<rptNewLine;
   *p << _T("W = girder weight = ")<<force.SetValue(pHaul->GetGirderWeight())<<_T(" ")<<_T(" ")<<force.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("I"),_T("y")) << _T(" = ") << mom_I.SetValue(pHaul->GetIy())<< rptNewLine;
   *p << Sub2(_T("h"),_T("r"))<<_T(" = Height of roll center above roadway = ")<<dim.SetValue(pHaul->GetHeightOfRollCenterAboveRoadway())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << _T("y = height of C.G. of Girder above roll center (Increased for camber by 2 percent) =")<<dim.SetValue(pHaul->GetHeightOfCgOfGirderAboveRollCenter())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("K"),symbol(theta))<<_T(" = roll stiffness of vehicle = ")<<spring.SetValue(pHaul->GetRollStiffnessOfvehicle())<<_T(" ")<<spring.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("z"),_T("max"))<<_T(" = distance from center of vehicle to center of dual tires = ")<<dim.SetValue(pHaul->GetAxleWidth()/2.0)<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << symbol(alpha)<<_T(" = Roadway superelevation = ")<<scalar.SetValue(pHaul->GetRoadwaySuperelevation())<<rptNewLine;
   *p << _T("Upward Impact during Hauling = ")<<pHaul->GetUpwardImpact()<<rptNewLine;
   *p << _T("Downward Impact during Hauling = ")<<pHaul->GetDownwardImpact()<<rptNewLine;
   *p << _T("Sweep tolerance = ")<<pHaul->GetSweepTolerance()<<loc.GetUnitTag()<<_T("/")<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("e"),_T("truck"))<<_T(" = tolerance in placement of supports =")<<dim.SetValue(pHaul->GetSupportPlacementTolerance())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << RPT_STRESS(_T("c")) << _T(" = concrete strength = ") << stress.SetValue(pHaul->GetConcreteStrength()) << _T(" ") << stress.GetUnitTag() << rptNewLine;
   *p << RPT_STRESS(_T("r"))<<_T(" = modulus of rupture at Hauling = ")<<stress.SetValue(pHaul->GetModRupture())<<_T(" ")<<stress.GetUnitTag()<<rptNewLine;
   *p << _T("Elastic modulus of girder concrete at Hauling = ")<<mod_e.SetValue(pHaul->GetElasticModulusOfGirderConcrete())<<_T(" ")<<mod_e.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("e"),_T("s"))<<_T(" = eccentricity due to sweep = ")<<dim.SetValue(pHaul->GetEccentricityDueToSweep())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("F"),_T("o"))<<_T(" = offset factor = ") << _T("(") << Sub2(_T("l"),_T("l")) << _T("/") << Sub2(_T("l"),_T("g")) << _T(")") << Super(_T("2")) << _T(" - 1/3 = ") << pHaul->GetOffsetFactor()<<rptNewLine;
   *p << Sub2(_T("e"),_T("i"))<<_T(" = total initial eccentricity = ") << Sub2(_T("e"),_T("s"))<<_T("*") << Sub2(_T("F"),_T("o"))<<_T(" + ") << Sub2(_T("e"),_T("truck"))<<_T(" = ")<<dim.SetValue(pHaul->GetTotalInitialEccentricity())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("zo.png") )<<_T(" = ")<<dim.SetValue(pHaul->GetZo())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("z"),_T("o")) << _T(" is based on average girder unit weight and mid-span section properties") << rptNewLine;
   *p << _T("r = Radius of stability = ") << Sub2(_T("K"),symbol(theta))<<_T("/W = ")<<dim.SetValue(pHaul->GetRadiusOfStability())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << _T("Equilibrium angle = ")<<rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ThetaHauling.png") )<<_T(" = ")<<angle.SetValue(pHaul->GetEqualibriumAngle())<<_T(" ")<<angle.GetUnitTag()<<rptNewLine;
   *p << _T("Lateral Moment = (Vertical Moment)(Equilibrium Angle)") << rptNewLine;

   pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;

   *pTitle << _T("Girder Forces and Stresses At Hauling");
   p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(6,_T("Hauling Forces"));
   *p << p_table<<rptNewLine;

   (*p_table)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR(_T("Effective") << rptNewLine << _T("Prestress") << rptNewLine << _T("Force"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*p_table)(0,2) << COLHDR(_T("Eccentricity"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(0,3) << COLHDR(_T("Moment") << rptNewLine << _T("Impact Up"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,4) << COLHDR(_T("Moment") << rptNewLine << _T("No Impact"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,5) << COLHDR(_T("Moment") << rptNewLine << _T("Impact Down"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   Float64 overhang = pHaul->GetTrailingOverhang();

   GET_IFACE2(pBroker,IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest);
   std::vector<pgsPointOfInterest> poi_vec;
   poi_vec = pGirderHaulingPointsOfInterest->GetHaulingPointsOfInterest(span,girder,POI_FLEXURESTRESS);

   RowIndexType row = 1;
   std::vector<pgsPointOfInterest>::const_iterator i;
   for (i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      pgsHaulingStressCheckArtifact stressArtifact =  pHaul->GetHaulingStressCheckArtifact(poi.GetDistFromStart());
 
      (*p_table)(row,0) << location.SetValue( pgsTypes::Hauling, poi, overhang );
      (*p_table)(row,1) << force.SetValue( stressArtifact.GetEffectiveHorizPsForce());
      (*p_table)(row,2) << dim.SetValue( stressArtifact.GetEccentricityPsForce());

      double M1,M2,M3;
      stressArtifact.GetMomentImpact(&M1,&M2,&M3);
      (*p_table)(row,3) << moment.SetValue(M1);
      (*p_table)(row,4) << moment.SetValue(M2);
      (*p_table)(row,5) << moment.SetValue(M3);
      row++;
   }

   p_table = pgsReportStyleHolder::CreateDefaultTable(9,_T("Hauling Stresses - Plumb Girder"));
   *p << p_table;

   p_table->SetNumberOfHeaderRows(2);
   p_table->SetRowSpan(0,0,2);
   p_table->SetRowSpan(1,0,-1);
   (*p_table)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   p_table->SetColumnSpan(0,1,4);
   (*p_table)(0,1) << _T("Top Stress, ") << RPT_FTOP;

   p_table->SetColumnSpan(0,2,4);
   (*p_table)(0,2) << _T("Bottom Stress ") << RPT_FBOT;

   p_table->SetColumnSpan(0,3,-1);
   p_table->SetColumnSpan(0,4,-1);
   p_table->SetColumnSpan(0,5,-1);
   p_table->SetColumnSpan(0,6,-1);
   p_table->SetColumnSpan(0,7,-1);
   p_table->SetColumnSpan(0,8,-1);

   (*p_table)(1,1) << COLHDR(_T("Prestress"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,2) << COLHDR(_T("Impact") << rptNewLine << _T("Up"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,3) << COLHDR(_T("No") << rptNewLine << _T("Impact"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,4) << COLHDR(_T("Impact") << rptNewLine << _T("Down"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,5) << COLHDR(_T("Prestress"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,6) << COLHDR(_T("Impact") << rptNewLine << _T("Up"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,7) << COLHDR(_T("No") << rptNewLine << _T("Impact"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,8) << COLHDR(_T("Impact") << rptNewLine << _T("Down"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   rptRcTable* p_table2 = pgsReportStyleHolder::CreateDefaultTable(7,_T("Hauling Stresses - Tilted Girder"));
   *p << p_table2;
   *p << RPT_STRESS(_T("tu")) << _T(" = top fiber stress, uphill side") << rptNewLine;
   *p << RPT_STRESS(_T("td")) << _T(" = top fiber stress, downhill side") << rptNewLine;
   *p << RPT_STRESS(_T("bu")) << _T(" = bottom fiber stress, uphill side") << rptNewLine;
   *p << RPT_STRESS(_T("bd")) << _T(" = bottom fiber stress, downhill side") << rptNewLine;
   (*p_table2)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table2)(0,1) << COLHDR(Sub2(_T("M"),_T("vert")),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table2)(0,2) << COLHDR(Sub2(_T("M"),_T("lat")),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table2)(0,3) << COLHDR(RPT_STRESS(_T("tu")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table2)(0,4) << COLHDR(RPT_STRESS(_T("td")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table2)(0,5) << COLHDR(RPT_STRESS(_T("bu")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table2)(0,6) << COLHDR(RPT_STRESS(_T("bd")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );


   rptRcTable* p_table3 = pgsReportStyleHolder::CreateDefaultTable(5,_T("Rebar Requirements for Tensile Stress Limit [C5.9.4.1.2]"));
   *p << p_table3 << rptNewLine;
   (*p_table3)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table3)(0,1) << COLHDR(Sub2(_T("Y"),_T("na")),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table3)(0,2) << COLHDR(Sub2(_T("A"),_T("t")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*p_table3)(0,3) << COLHDR(_T("T"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*p_table3)(0,4) << COLHDR(Sub2(_T("A"),_T("s")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );

   RowIndexType row1 = 2;
   RowIndexType row2 = 1;
   for (i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      pgsHaulingStressCheckArtifact stressArtifact =  pHaul->GetHaulingStressCheckArtifact(poi.GetDistFromStart());
 
      (*p_table)(row1,0) << location.SetValue( pgsTypes::Hauling, poi,overhang );
      
      Float64 ps, up, down, no;
      stressArtifact.GetTopFiberStress(&ps, &up,&no,&down);
      (*p_table)(row1,1) << stress.SetValue( ps );
      (*p_table)(row1,2) << stress.SetValue( up );
      (*p_table)(row1,3) << stress.SetValue( no );
      (*p_table)(row1,4) << stress.SetValue( down );
      
      stressArtifact.GetBottomFiberStress(&ps,&up,&no,&down);
      (*p_table)(row1,5) << stress.SetValue( ps );
      (*p_table)(row1,6) << stress.SetValue( up );
      (*p_table)(row1,7) << stress.SetValue( no );
      (*p_table)(row1,8) << stress.SetValue( down );

      (*p_table2)(row2,0) << location.SetValue(pgsTypes::Hauling, poi, overhang);

      Float64 Mu,Mn,Md;
      stressArtifact.GetMomentImpact(&Mu,&Mn,&Md);

      Float64 lat_moment;
      lat_moment  = stressArtifact.GetLateralMoment();
      (*p_table2)(row2,1) << moment.SetValue(Mn);
      (*p_table2)(row2,2) << moment.SetValue(lat_moment);

      Float64 ftu, ftd, fbu, fbd;
      stressArtifact.GetIncludedGirderStresses(&ftu,&ftd,&fbu,&fbd);
      (*p_table2)(row2,3) << stress.SetValue(ftu);
      (*p_table2)(row2,4) << stress.SetValue(ftd);
      (*p_table2)(row2,5) << stress.SetValue(fbu);
      (*p_table2)(row2,6) << stress.SetValue(fbd);



      (*p_table3)(row2,0) << location.SetValue( pgsTypes::Hauling, poi, overhang );

      Float64 YnaUp,YnaDown, YnaInclined,At,T,As;
      ImpactDir controlling_impact;
      GirderOrientation controlling_orientation;
      stressArtifact.GetAlternativeTensileStressParameters(&YnaUp,&YnaDown,&YnaInclined,&controlling_impact,&controlling_orientation,&At,&T,&As);

      switch( controlling_impact )
      {
      case Up:
         if (YnaUp < 0 )
             (*p_table3)(row2,1) << _T("-");
         else
            (*p_table3)(row2,1) << dim.SetValue(YnaUp);
         break;

      case None:
         if (YnaInclined < 0 )
             (*p_table3)(row2,1) << _T("-");
         else
            (*p_table3)(row2,1) << dim.SetValue(YnaInclined);
         break;

      case Down:
         if (YnaDown < 0 )
             (*p_table3)(row2,1) << _T("-");
         else
            (*p_table3)(row2,1) << dim.SetValue(YnaDown);
         break;

      }

      (*p_table3)(row2,2) << area.SetValue(At);
      (*p_table3)(row2,3) << force.SetValue(T);
      (*p_table3)(row2,4) << area.SetValue(As);

      row1++;
      row2++;
   }

   p = new rptParagraph;
   *pChapter << p;

   // FS Cracking
   p_table = pgsReportStyleHolder::CreateDefaultTable(7,_T("Factor of Safety Against Cracking"));
   *p << p_table << rptNewLine;
   *p << RPT_STRESS(_T("t")) << _T(" = governing tension stress")<<rptNewLine;

   (*p_table)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR(RPT_STRESS(_T("t")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,2) << _T("Governing") << rptNewLine << _T("Flange");
   (*p_table)(0,3) << COLHDR(Sub2(_T("M"),_T("lat")),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,4) << COLHDR(Sub2(_T("M"),_T("vert")),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,5) << COLHDR(Sub2(symbol(theta),_T("max")),rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit() );
   (*p_table)(0,6) << Sub2(_T("FS"),_T("cr"));

   row=1;
   for (i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      pgsHaulingCrackingCheckArtifact crackArtifact =  pHaul->GetHaulingCrackingCheckArtifact(poi.GetDistFromStart());
 
      (*p_table)(row,0) << location.SetValue( pgsTypes::Hauling, poi, overhang);
      (*p_table)(row,1) << stress.SetValue( crackArtifact.GetLateralMomentStress() );

      if (crackArtifact.GetCrackedFlange()==BottomFlange)
         (*p_table)(row,2) << _T("Bottom");
      else
         (*p_table)(row,2) << _T("Top");

      (*p_table)(row,3) << moment.SetValue( crackArtifact.GetLateralMoment() );
      (*p_table)(row,4) << moment.SetValue( crackArtifact.GetVerticalMoment() );
      (*p_table)(row,5) << angle.SetValue( crackArtifact.GetThetaCrackingMax() );
      (*p_table)(row,6) << scalar.SetValue(crackArtifact.GetFsCracking());
      row++;
   }

   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Mlat.png") )<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ThetaMax.png") )<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("FScrHauling.png") )<<rptNewLine;

   // FS Rollover
   pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Factor of Safety Against Rollover")<<rptNewLine;
   p = new rptParagraph;
   *pChapter << p;

   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ThetaPrimeMaxHauling.png") )<<_T(" = ")<<angle.SetValue(pHaul->GetThetaRolloverMax())<<_T(" ")<<angle.GetUnitTag()<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("zo_prime_hauling.png") )<<_T(" = ")<<dim.SetValue(pHaul->GetZoPrime())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("FSrHauling.png") )<<_T(" = ")<<scalar.SetValue(pHaul->GetFsRollover())<<rptNewLine;

   return pChapter;
}


CChapterBuilder* CHaulingCheckDetailsChapterBuilder::Clone() const
{
   return new CHaulingCheckDetailsChapterBuilder;
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
