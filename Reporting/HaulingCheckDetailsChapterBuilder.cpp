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

#include <IFace\DisplayUnits.h>
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
CHaulingCheckDetailsChapterBuilder::CHaulingCheckDetailsChapterBuilder()
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

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);

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

   location.MakeSpanPoi();

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << "Details for Check for Hauling to Bridge Site [5.5.4.3]"<<rptNewLine;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;


   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if (!pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
   {
      *p <<color(Red)<<"Hauling analysis disabled in Project Criteria library entry. No analysis performed."<<color(Black)<<rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,girder);
   const pgsHaulingCheckArtifact* pHaul = pArtifact->GetHaulingCheckArtifact();

   *p << Sub2("l","g") << " = Overall Length of girder = "<<loc.SetValue(pHaul->GetGirderLength())<<" "<<loc.GetUnitTag()<<rptNewLine;

   Float64 leadingOH  = pHaul->GetLeadingOverhang();
   Float64 trailingOH = pHaul->GetTrailingOverhang();

   FormatDimension(leadingOH,pDisplayUnits->GetSpanLengthUnit());
   *p << "Leading Overhang = "<<loc.SetValue(leadingOH)<<" "<<loc.GetUnitTag()<<rptNewLine;
   *p << "Trailing Overhang = "<<loc.SetValue(trailingOH)<<" "<<loc.GetUnitTag()<<rptNewLine;
   *p << Sub2("l","l")<<" = Clear span length between supports = "<<loc.SetValue(pHaul->GetClearSpanBetweenSupportLocations())<<" "<<loc.GetUnitTag()<<rptNewLine;
   *p << "w = average girder weight/length = "<<wt_len.SetValue(pHaul->GetAvgGirderWeightPerLength())<<" "<<" "<<wt_len.GetUnitTag()<<rptNewLine;
   *p << "W = girder weight = "<<force.SetValue(pHaul->GetGirderWeight())<<" "<<" "<<force.GetUnitTag()<<rptNewLine;
   *p << Sub2("I","y") << " = " << mom_I.SetValue(pHaul->GetIy())<< rptNewLine;
   *p << Sub2("h","r")<<" = Height of roll center above roadway = "<<dim.SetValue(pHaul->GetHeightOfRollCenterAboveRoadway())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << "y = height of C.G. of Girder above roll center (Increased for camber by 2 percent) ="<<dim.SetValue(pHaul->GetHeightOfCgOfGirderAboveRollCenter())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << Sub2("K",symbol(theta))<<" = roll stiffness of vehicle = "<<spring.SetValue(pHaul->GetRollStiffnessOfvehicle())<<" "<<spring.GetUnitTag()<<rptNewLine;
   *p << Sub2("z","max")<<" = distance from center of vehicle to center of dual tires = "<<dim.SetValue(pHaul->GetAxleWidth()/2.0)<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << symbol(alpha)<<" = Roadway superelevation = "<<scalar.SetValue(pHaul->GetRoadwaySuperelevation())<<rptNewLine;
   *p << "Upward Impact during Hauling = "<<pHaul->GetUpwardImpact()<<rptNewLine;
   *p << "Downward Impact during Hauling = "<<pHaul->GetDownwardImpact()<<rptNewLine;
   *p << "Sweep tolerance = "<<pHaul->GetSweepTolerance()<<loc.GetUnitTag()<<"/"<<" "<<loc.GetUnitTag()<<rptNewLine;
   *p << Sub2("e","truck")<<" = tolerance in placement of supports ="<<dim.SetValue(pHaul->GetSupportPlacementTolerance())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << Sub2("f","c") << " = concrete strength = " << stress.SetValue(pHaul->GetConcreteStrength()) << " " << stress.GetUnitTag() << rptNewLine;
   *p << Sub2("f","r")<<" = modulus of rupture at Hauling = "<<stress.SetValue(pHaul->GetModRupture())<<" "<<stress.GetUnitTag()<<rptNewLine;
   *p << "Elastic modulus of girder concrete at Hauling = "<<mod_e.SetValue(pHaul->GetElasticModulusOfGirderConcrete())<<" "<<mod_e.GetUnitTag()<<rptNewLine;
   *p << Sub2("e","s")<<" = eccentricity due to sweep = "<<dim.SetValue(pHaul->GetEccentricityDueToSweep())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << Sub2("F","o")<<" = offset factor = " << "(" << Sub2("l","l") << "/" << Sub2("l","g") << ")" << Super("2") << " - 1/3 = " << pHaul->GetOffsetFactor()<<rptNewLine;
   *p << Sub2("e","i")<<" = total initial eccentricity = " << Sub2("e","s")<<"*" << Sub2("F","o")<<" + " << Sub2("e","truck")<<" = "<<dim.SetValue(pHaul->GetTotalInitialEccentricity())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Zo - Lifting.jpg" )<<" = "<<dim.SetValue(pHaul->GetZo())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << Sub2("z","o") << " is based on average girder unit weight and mid-span section properties" << rptNewLine;
   *p << "r = Radius of stability = " << Sub2("K",symbol(theta))<<"/W = "<<dim.SetValue(pHaul->GetRadiusOfStability())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << "Equilibrium angle = "<<rptRcImage(pgsReportStyleHolder::GetImagePath() + "EquilAngleEqn.jpg" )<<" = "<<angle.SetValue(pHaul->GetEqualibriumAngle())<<" "<<angle.GetUnitTag()<<rptNewLine;
   *p << "Lateral Moment = (Vertical Moment)(Equilibrium Angle)" << rptNewLine;

   pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;

   *pTitle << "Girder Forces and Stresses At Hauling";
   p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(6,"Hauling Forces");
   *p << p_table<<rptNewLine;

   (*p_table)(0,0) << COLHDR("Location from" << rptNewLine << "Left Bunk Point",    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR("Effective" << rptNewLine << "Prestress" << rptNewLine << "Force",rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*p_table)(0,2) << COLHDR("Eccentricity",rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(0,3) << COLHDR("Moment" << rptNewLine << "Impact Up",rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,4) << COLHDR("Moment" << rptNewLine << "No Impact",rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,5) << COLHDR("Moment" << rptNewLine << "Impact Down",rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

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
 
      (*p_table)(row,0) << location.SetValue( poi, overhang );
      (*p_table)(row,1) << force.SetValue( stressArtifact.GetEffectiveHorizPsForce());
      (*p_table)(row,2) << dim.SetValue( stressArtifact.GetEccentricityPsForce());

      double M1,M2,M3;
      stressArtifact.GetMomentImpact(&M1,&M2,&M3);
      (*p_table)(row,3) << moment.SetValue(M1);
      (*p_table)(row,4) << moment.SetValue(M2);
      (*p_table)(row,5) << moment.SetValue(M3);
      row++;
   }

   p_table = pgsReportStyleHolder::CreateDefaultTable(9,"Hauling Stresses - Plumb Girder");
   *p << p_table;

   p_table->SetNumberOfHeaderRows(2);
   p_table->SetRowSpan(0,0,2);
   p_table->SetRowSpan(1,0,-1);
   (*p_table)(0,0) << COLHDR("Location from" << rptNewLine << "Left Bunk Point",    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   p_table->SetColumnSpan(0,1,4);
   (*p_table)(0,1) << "Top Stress, " << RPT_FTOP;

   p_table->SetColumnSpan(0,2,4);
   (*p_table)(0,2) << "Bottom Stress " << RPT_FBOT;

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

   rptRcTable* p_table2 = pgsReportStyleHolder::CreateDefaultTable(7,"Hauling Stresses - Tilted Girder");
   *p << p_table2;
   *p << Sub2(symbol(sigma),"tu") << " = top fiber stress, uphill side" << rptNewLine;
   *p << Sub2(symbol(sigma),"td") << " = top fiber stress, downhill side" << rptNewLine;
   *p << Sub2(symbol(sigma),"bu") << " = bottom fiber stress, uphill side" << rptNewLine;
   *p << Sub2(symbol(sigma),"bd") << " = bottom fiber stress, downhill side" << rptNewLine;
   (*p_table2)(0,0) << COLHDR("Location from" << rptNewLine << "Left Bunk Point", rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table2)(0,1) << COLHDR(Sub2("M","vert"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table2)(0,2) << COLHDR(Sub2("M","lat"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table2)(0,3) << COLHDR(Sub2(symbol(sigma),"tu"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table2)(0,4) << COLHDR(Sub2(symbol(sigma),"td"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table2)(0,5) << COLHDR(Sub2(symbol(sigma),"bu"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table2)(0,6) << COLHDR(Sub2(symbol(sigma),"bd"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );


   rptRcTable* p_table3 = pgsReportStyleHolder::CreateDefaultTable(5,"Rebar Requirements for Tensile Stress Limit [C5.9.4.1.2]");
   *p << p_table3 << rptNewLine;
   (*p_table3)(0,0) << COLHDR("Location from" << rptNewLine << "Left Bunk Point",    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table3)(0,1) << COLHDR(Sub2("Y","na"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table3)(0,2) << COLHDR(Sub2("A","t"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*p_table3)(0,3) << COLHDR("T",rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*p_table3)(0,4) << COLHDR(Sub2("A","s"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );

   RowIndexType row1 = 2;
   RowIndexType row2 = 1;
   for (i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      pgsHaulingStressCheckArtifact stressArtifact =  pHaul->GetHaulingStressCheckArtifact(poi.GetDistFromStart());
 
      (*p_table)(row1,0) << location.SetValue( poi,overhang );
      
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

      (*p_table2)(row2,0) << location.SetValue(poi, overhang);

      Float64 Mu,Mn,Md;
      stressArtifact.GetMomentImpact(&Mu,&Mn,&Md);

      Float64 lat_moment;
      lat_moment  = stressArtifact.GetLateralMoment();
      (*p_table2)(row2,1) << moment.SetValue(lat_moment);
      (*p_table2)(row2,2) << moment.SetValue(Mn);

      Float64 ftu, ftd, fbu, fbd;
      stressArtifact.GetIncludedGirderStresses(&ftu,&ftd,&fbu,&fbd);
      (*p_table2)(row2,3) << stress.SetValue(ftu);
      (*p_table2)(row2,4) << stress.SetValue(ftd);
      (*p_table2)(row2,5) << stress.SetValue(fbu);
      (*p_table2)(row2,6) << stress.SetValue(fbd);



      (*p_table3)(row2,0) << location.SetValue( poi, overhang );

      Float64 YnaUp,YnaDown, YnaInclined,At,T,As;
      ImpactDir controlling_impact;
      GirderOrientation controlling_orientation;
      stressArtifact.GetAlternativeTensileStressParameters(&YnaUp,&YnaDown,&YnaInclined,&controlling_impact,&controlling_orientation,&At,&T,&As);

      switch( controlling_impact )
      {
      case Up:
         if (YnaUp < 0 )
             (*p_table3)(row2,1) << "-";
         else
            (*p_table3)(row2,1) << dim.SetValue(YnaUp);
         break;

      case None:
         if (YnaInclined < 0 )
             (*p_table3)(row2,1) << "-";
         else
            (*p_table3)(row2,1) << dim.SetValue(YnaInclined);
         break;

      case Down:
         if (YnaDown < 0 )
             (*p_table3)(row2,1) << "-";
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
   p_table = pgsReportStyleHolder::CreateDefaultTable(7,"Factor of Safety Against Cracking");
   *p << p_table << rptNewLine;
   *p << Sub2("f","t") << " = governing tension stress"<<rptNewLine;

   (*p_table)(0,0) << COLHDR("Location from" << rptNewLine << "Left Bunk Point",    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR(Sub2("f","t"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,2) << "Governing" << rptNewLine << "Flange";
   (*p_table)(0,3) << COLHDR(Sub2("M","lat"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,4) << COLHDR(Sub2("M","vert"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,5) << COLHDR(Sub2(symbol(theta),"max"),rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit() );
   (*p_table)(0,6) << Sub2("FS","cr");

   row=1;
   for (i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      pgsHaulingCrackingCheckArtifact crackArtifact =  pHaul->GetHaulingCrackingCheckArtifact(poi.GetDistFromStart());
 
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

   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Mlat - Lifting.jpg" )<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Theta Max - Lifting.jpg" )<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + "FS Cracking - Hauling.jpg" )<<rptNewLine;

   // FS Rollover
   pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << "Factor of Safety Against Rollover"<<rptNewLine;
   p = new rptParagraph;
   *pChapter << p;

   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Theta prime Max - Hauilng.jpg" )<<" = "<<angle.SetValue(pHaul->GetThetaRolloverMax())<<" "<<angle.GetUnitTag()<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Zo Prime - Hauling.jpg" )<<" = "<<dim.SetValue(pHaul->GetZoPrime())<<" "<<dim.GetUnitTag()<<rptNewLine;
   *p << rptRcImage(pgsReportStyleHolder::GetImagePath() + "FS Rollover - Hauling.jpg" )<<" = "<<scalar.SetValue(pHaul->GetFsRollover())<<rptNewLine;

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
