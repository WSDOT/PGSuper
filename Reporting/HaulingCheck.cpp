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
#include <Reporting\HaulingCheck.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Artifact.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\GirderHandlingPointOfInterest.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Project.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\HaulingCheckArtifact.h>
#include <PgsExt\GirderArtifact.h>

#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CHaulingCheck
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CHaulingCheck::CHaulingCheck()
{
}

CHaulingCheck::CHaulingCheck(const CHaulingCheck& rOther)
{
   MakeCopy(rOther);
}

CHaulingCheck::~CHaulingCheck()
{
}

//======================== OPERATORS  =======================================
CHaulingCheck& CHaulingCheck::operator= (const CHaulingCheck& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CHaulingCheck::Build(rptChapter* pChapter,
                              IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                              IEAFDisplayUnits* pDisplayUnits) const
{

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << "Check for Hauling to Bridge Site [5.5.4.3]"<<rptNewLine;
   *pTitle << "Hauling Stresses and Factor of Safety Against Cracking"<<rptNewLine;

   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc,      pDisplayUnits->GetSpanLengthUnit(), true  );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,    pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress_u, pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;


   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if (!pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
   {
      *p <<color(Red)<<"Hauling analysis disabled in Project Criteria library entry. No analysis performed."<<color(Black)<<rptNewLine;
      return;
   }

   GET_IFACE2(pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker, ILibrary,       pLib );
   std::string specName = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( specName.c_str() );

   double c; // compression coefficient
   double t; // tension coefficient
   double t_max; // maximum allowable tension
   bool b_t_max; // true if max allowable tension is applicable

   c = pSpecEntry->GetHaulingCompStress();
   t = pSpecEntry->GetMaxConcreteTensHauling();
   pSpecEntry->GetAbsMaxConcreteTensHauling(&b_t_max,&t_max);

   Float64 t2 = pSpecEntry->GetMaxConcreteTensWithRebarHauling();

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,girder);
   const pgsHaulingCheckArtifact* pHaulArtifact = pArtifact->GetHaulingCheckArtifact();

   *p <<"Maximum allowable concrete compressive stress = -" << c << RPT_FC << " = " << 
      stress.SetValue(pHaulArtifact->GetAllowableCompressionStress())<< " " <<
      stress.GetUnitTag()<< rptNewLine;
   *p <<"Maximum allowable concrete tensile stress, plumb girder with impact = " << tension_coeff.SetValue(t) << symbol(ROOT) << RPT_FC;
   if ( b_t_max )
      *p << " but not more than: " << stress.SetValue(t_max);
      
   *p << " = " << stress.SetValue(pHaulArtifact->GetAllowableTensileStress())<< " " <<
      stress.GetUnitTag()<< rptNewLine;

   double As_reqd = pHaulArtifact->GetAlterantiveTensileStressAsMax();
   *p <<"Maximum allowable concrete tensile stress, plumb girder with impact = " << tension_coeff.SetValue(t2) << symbol(ROOT) << RPT_FC
       << " = " << stress.SetValue(pHaulArtifact->GetAlternativeTensionAllowableStress()) << " " << stress.GetUnitTag();
   if ( !IsZero(As_reqd) )
       *p << " if at least " << area.SetValue(As_reqd) << " of mild reinforcement is provided" << rptNewLine;
   else
       *p << " if bonded reinforcement sufficient to resist the tensile force in the concrete is provided." << rptNewLine;

   GET_IFACE2(pBroker,IBridgeMaterialEx,pMaterial);
   t = pSpecEntry->GetHaulingModulusOfRuptureCoefficient(pMaterial->GetGdrConcreteType(span,girder));
   *p <<"Maximum allowable concrete tensile stress, inclined girder without impact = " << Sub2("f","r") << " = " << tension_coeff.SetValue(t) << symbol(ROOT) << RPT_FC;
   *p << " = " << stress.SetValue(pHaulArtifact->GetModRupture())<< " " <<
      stress.GetUnitTag()<< rptNewLine;

   *p <<"Allowable factor of safety against cracking = "<<pHaulArtifact->GetAllowableFsForCracking()<<rptNewLine;

   double fcMax = IS_SI_UNITS(pDisplayUnits) ? ::ConvertToSysUnits(105,unitMeasure::MPa) : ::ConvertToSysUnits(15.0,unitMeasure::KSI);

   double fc_reqd_comp,fc_reqd_tens;
   bool min_rebar_reqd;
   pHaulArtifact->GetRequiredConcreteStrength(&fc_reqd_comp,&fc_reqd_tens,&min_rebar_reqd,fcMax,false);

   double fc_reqd = max(fc_reqd_comp,fc_reqd_tens);

   if ( 0 < fc_reqd )
      *p << RPT_FC << " required to satisfy stress and stability criteria = " << stress_u.SetValue( fc_reqd ) << rptNewLine;
   else
      *p << "There is no concrete strength that will satisfy the stress and stability criteria." << rptNewLine;

   GET_IFACE2(pBroker,IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest);
   std::vector<pgsPointOfInterest> poi_vec;
   poi_vec = pGirderHaulingPointsOfInterest->GetHaulingPointsOfInterest(span,girder,POI_FLEXURESTRESS);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(10,"");
   *p << p_table << rptNewLine;
   (*p_table)(0,0) << COLHDR("Location from" << rptNewLine << "Left Bunk Point",    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR("Min" << rptNewLine << "Stress" << Super("#"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,2) << COLHDR("Max" << rptNewLine << "Stress" << Super("#"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,3) << COLHDR("Min" << rptNewLine << "Stress" << Super("*"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,4) << COLHDR("Max" << rptNewLine << "Stress" << Super("*"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,5) << "Tension" << rptNewLine << "Status" << rptNewLine << "w/o Rebar";
   (*p_table)(0,6) << "Tension" << rptNewLine << "Status" << rptNewLine << "w/  Rebar";
   (*p_table)(0,7) << "Compression" << rptNewLine << "Status";
   (*p_table)(0,8) << Sub2("FS","cr");
   (*p_table)(0,9) << "FS" << rptNewLine << "Status";

   *p << Super("#") << " based on inclined girder without impact" << rptNewLine;
   *p << Super("*") << " based on plumb girder with impact" << rptNewLine;

   Float64 overhang = pHaulArtifact->GetTrailingOverhang();

   RowIndexType row=1;
   for (std::vector<pgsPointOfInterest>::const_iterator i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      pgsHaulingStressCheckArtifact stressArtifact = pHaulArtifact->GetHaulingStressCheckArtifact(poi.GetDistFromStart());
      pgsHaulingCrackingCheckArtifact crackArtifact =  pHaulArtifact->GetHaulingCrackingCheckArtifact(poi.GetDistFromStart());
 
      (*p_table)(row,0) << location.SetValue( pgsTypes::Hauling, poi,overhang );
      (*p_table)(row,1) << stress.SetValue(stressArtifact.GetMaximumInclinedConcreteCompressiveStress());
      (*p_table)(row,2) << stress.SetValue(stressArtifact.GetMaximumInclinedConcreteTensileStress());
      (*p_table)(row,3) << stress.SetValue(stressArtifact.GetMaximumConcreteCompressiveStress());
      (*p_table)(row,4) << stress.SetValue(stressArtifact.GetMaximumConcreteTensileStress());

      if ( stressArtifact.TensionPassed() )
          (*p_table)(row,5) << RPT_PASS;
      else
          (*p_table)(row,5) << RPT_FAIL;


      if ( stressArtifact.AlternativeTensionPassed() )
          (*p_table)(row,6) << RPT_PASS;
      else
          (*p_table)(row,6) << RPT_FAIL;

      if ( stressArtifact.CompressionPassed() )
          (*p_table)(row,7) << RPT_PASS;
      else
          (*p_table)(row,7) << RPT_FAIL;

      (*p_table)(row,8) << scalar.SetValue(crackArtifact.GetFsCracking());
      
      if (crackArtifact.Passed() )
         (*p_table)(row,9) << RPT_PASS;
      else
      {
         (*p_table)(row,9) << RPT_FAIL;
      }

      row++;
   }

   // FS for failure
   pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << "Factor of Safety Against Rollover";

   p = new rptParagraph;
   *pChapter << p;

   p_table = pgsReportStyleHolder::CreateTableNoHeading(2,"");
   p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
   *p << p_table;

   (*p_table)(0,0) << "Factor of Safety Against Rollover (FS" << Sub("r") << ")";
   (*p_table)(1,0) << "Allowable Factor of Safety Against Rollover";
   (*p_table)(2,0) << "Status";
   Float64 fs_fail  = pHaulArtifact->GetFsRollover();
   Float64 all_fail = pHaulArtifact->GetAllowableFsForRollover();
   (*p_table)(0,1) << scalar.SetValue(pHaulArtifact->GetFsRollover());
   (*p_table)(1,1) << scalar.SetValue(pHaulArtifact->GetAllowableFsForRollover());
   if (IsLE(all_fail,fs_fail))
      (*p_table)(2,1) << RPT_PASS;
   else
      (*p_table)(2,1) << RPT_FAIL;

   // Truck support spacing
   pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << "Spacing Between Truck Supports for Hauling";

   p = new rptParagraph;
   *pChapter << p;

   p_table = pgsReportStyleHolder::CreateTableNoHeading(2,"");
   p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
   *p << p_table;
   (*p_table)(0,0) << "Distance Between Supports";
   (*p_table)(1,0) << "Max. Allowable Distance Between Supports";
   (*p_table)(2,0) << "Status";

   Float64 span_length  = pHaulArtifact->GetClearSpanBetweenSupportLocations();
   Float64 allowable_span_length = pHaulArtifact->GetAllowableSpanBetweenSupportLocations();

   (*p_table)(0,1) << loc.SetValue(span_length);
   (*p_table)(1,1) << loc.SetValue(allowable_span_length);

   if ( IsLE(span_length,allowable_span_length) )
      (*p_table)(2,1) << RPT_PASS;
   else
      (*p_table)(2,1) << RPT_FAIL;



   // Truck support spacing
   pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << "Girder Support Configuration";

   p = new rptParagraph;
   *pChapter << p;

   p_table = pgsReportStyleHolder::CreateTableNoHeading(2,"");
   p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
   *p << p_table;

   (*p_table)(0,0) << "Leading Overhang (closest to cab of truck)";
   (*p_table)(1,0) << "Max. Allowable Leading Overhang";
   (*p_table)(2,0) << "Status";
   Float64 oh  = pHaulArtifact->GetLeadingOverhang();
   Float64 all_oh = pHaulArtifact->GetAllowableLeadingOverhang();
   (*p_table)(0,1) << loc.SetValue(oh);
   (*p_table)(1,1) << loc.SetValue(all_oh);
   if ( IsLE(oh,all_oh) )
      (*p_table)(2,1) << RPT_PASS;
   else
      (*p_table)(2,1) << RPT_FAIL;

   p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));

   // Max Girder Weight
   pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << "Maximum Girder Weight";

   p = new rptParagraph;
   *pChapter << p;

   p_table = pgsReportStyleHolder::CreateTableNoHeading(2,"");
   p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
   *p << p_table;
   (*p_table)(0,0) << "Girder Weight";
   (*p_table)(1,0) << "Maximum Allowable Weight";
   (*p_table)(2,0) << "Status";
   Float64 wgt  = pHaulArtifact->GetGirderWeight();
   Float64 maxwgt = pHaulArtifact->GetMaxGirderWgt();
   force.ShowUnitTag(true);
   (*p_table)(0,1) << force.SetValue(wgt);
   (*p_table)(1,1) << force.SetValue(maxwgt);
   if ( IsLE(wgt,maxwgt) )
      (*p_table)(2,1) << RPT_PASS;
   else
      (*p_table)(2,1) << RPT_FAIL;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CHaulingCheck::MakeCopy(const CHaulingCheck& rOther)
{
   // Add copy code here...
}

void CHaulingCheck::MakeAssignment(const CHaulingCheck& rOther)
{
   MakeCopy( rOther );
}

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
bool CHaulingCheck::AssertValid() const
{
   return true;
}

void CHaulingCheck::Dump(dbgDumpContext& os) const
{
   os << "Dump for CHaulingCheck" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CHaulingCheck::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CHaulingCheck");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CHaulingCheck");

   TESTME_EPILOG("CHaulingCheck");
}
#endif // _UNITTEST
