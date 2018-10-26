///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#include <Reporting\LiftingCheck.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Artifact.h>
#include <IFace\DisplayUnits.h>
#include <IFace\GirderHandlingPointOfInterest.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Project.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\LiftingCheckArtifact.h>
#include <PgsExt\GirderArtifact.h>

#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLiftingCheck
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CLiftingCheck::CLiftingCheck()
{
}

CLiftingCheck::CLiftingCheck(const CLiftingCheck& rOther)
{
   MakeCopy(rOther);
}

CLiftingCheck::~CLiftingCheck()
{
}

//======================== OPERATORS  =======================================
CLiftingCheck& CLiftingCheck::operator= (const CLiftingCheck& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CLiftingCheck::Build(rptChapter* pChapter,
                              IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                              IDisplayUnits* pDispUnit) const
{

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << "Check for Lifting In Casting Yard [5.5.4.3]"<<rptNewLine;
   *pTitle << "Lifting Stresses and Factor of Safety Against Cracking"<<rptNewLine;

   rptRcScalar scalar;
   scalar.SetFormat( pDispUnit->GetScalarFormat().Format );
   scalar.SetWidth( pDispUnit->GetScalarFormat().Width );
   scalar.SetPrecision( pDispUnit->GetScalarFormat().Precision );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDispUnit->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,    pDispUnit->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDispUnit->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDispUnit->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress_u, pDispUnit->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDispUnit->GetTensionCoefficientUnit(), false);
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area, pDispUnit->GetAreaUnit(), true);

   location.MakeSpanPoi();


   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   if (!pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
   {
      *p <<color(Red)<<"Lifting analysis disabled in Project Criteria library entry. No analysis performed."<<color(Black)<<rptNewLine;
      return;
   }

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,girder);
   const pgsLiftingCheckArtifact* pLiftArtifact = pArtifact->GetLiftingCheckArtifact();

   // unstable girders are a problem
   if (!pLiftArtifact->IsGirderStable())
   {
      *pTitle<<"Warning! - Girder is unstable - CG is higher than pick points"<<rptNewLine;
   }

   GET_IFACE2(pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker, ILibrary,       pLib );
   std::string specName = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( specName.c_str() );

   double c; // compression coefficient
   double t; // tension coefficient
   double t_max; // maximum allowable tension
   bool b_t_max; // true if max allowable tension is applicable

   c = pSpecEntry->GetCyCompStressLifting();
   t = pSpecEntry->GetCyMaxConcreteTensLifting();
   pSpecEntry->GetCyAbsMaxConcreteTensLifting(&b_t_max,&t_max);

   Float64 t2 = pSpecEntry->GetMaxConcreteTensWithRebarLifting();

   *p <<"Maximum allowable concrete compressive stress = -" << c << RPT_FCI << " = " << 
      stress.SetValue(pLiftArtifact->GetAllowableCompressionStress())<< " " <<
      stress.GetUnitTag()<< rptNewLine;
   *p <<"Maximum allowable concrete tensile stress = " << tension_coeff.SetValue(t) << symbol(ROOT) << RPT_FCI;
   if ( b_t_max )
      *p << " but not more than: " << stress.SetValue(t_max);
   *p << " = " << stress.SetValue(pLiftArtifact->GetAllowableTensileStress())<< " " <<
      stress.GetUnitTag()<< rptNewLine;

   double As_reqd = pLiftArtifact->GetAlterantiveTensileStressAsMax();
   *p <<"Maximum allowable concrete tensile stress = " << tension_coeff.SetValue(t2) << symbol(ROOT) << RPT_FCI
       << " = " << stress.SetValue(pLiftArtifact->GetAlternativeTensionAllowableStress()) << " " << stress.GetUnitTag();
   if ( !IsZero(As_reqd) )
       *p << " if at least " << area.SetValue(As_reqd) << " of mild reinforcement is provided" << rptNewLine;
   else
       *p << " if bonded reinforcement sufficient to resist the tensile force in the concrete is provided." << rptNewLine;

   *p <<"Allowable factor of safety against cracking = "<<pLiftArtifact->GetAllowableFsForCracking()<<rptNewLine;

   double fc_reqd_comp,fc_reqd_tens;
   bool min_rebar_reqd;
   pLiftArtifact->GetRequiredConcreteStrength(&fc_reqd_comp,&fc_reqd_tens,&min_rebar_reqd);

   double fci_reqd = max(fc_reqd_comp,fc_reqd_tens);

   if ( 0 < fci_reqd )
      *p << RPT_FCI << " required to satisfy this stress check = " << stress_u.SetValue( fci_reqd ) << rptNewLine;

   GET_IFACE2(pBroker,IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
   std::vector<pgsPointOfInterest> poi_vec;
   poi_vec = pGirderLiftingPointsOfInterest->GetLiftingPointsOfInterest(span,girder,POI_FLEXURESTRESS);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(8,"");
   *p << p_table;
   (*p_table)(0,0) << COLHDR("Location from" << rptNewLine << "Left Pick Point",    rptLengthUnitTag, pDispUnit->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR("Min" << rptNewLine << "Stress",rptStressUnitTag, pDispUnit->GetStressUnit() );
   (*p_table)(0,2) << COLHDR("Max" << rptNewLine << "Stress",rptStressUnitTag, pDispUnit->GetStressUnit() );
   (*p_table)(0,3) << "Tension" << rptNewLine << "Status" << rptNewLine << "w/o Rebar";
   (*p_table)(0,4) << "Tension" << rptNewLine << "Status" << rptNewLine << "w/  Rebar";
   (*p_table)(0,5) << "Compression" << rptNewLine << "Status";
   (*p_table)(0,6) << Sub2("FS","cr");
   (*p_table)(0,7) << "FS" << rptNewLine << "Status";

   Float64 overhang = (pLiftArtifact->GetGirderLength() - pLiftArtifact->GetClearSpanBetweenPickPoints())/2.0;

   RowIndexType row=1;
   for (std::vector<pgsPointOfInterest>::const_iterator i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      pgsLiftingStressCheckArtifact stressArtifact = pLiftArtifact->GetLiftingStressCheckArtifact(poi.GetDistFromStart());
      pgsLiftingCrackingCheckArtifact crackArtifact =  pLiftArtifact->GetLiftingCrackingCheckArtifact(poi.GetDistFromStart());
 
      (*p_table)(row,0) << location.SetValue( poi,overhang );
      (*p_table)(row,1) << stress.SetValue(stressArtifact.GetMaximumConcreteCompressiveStress());
      (*p_table)(row,2) << stress.SetValue(stressArtifact.GetMaximumConcreteTensileStress());

      if ( stressArtifact.TensionPassed() )
          (*p_table)(row,3) << RPT_PASS;
      else
          (*p_table)(row,3) << RPT_FAIL;


      if ( stressArtifact.AlternativeTensionPassed() )
          (*p_table)(row,4) << RPT_PASS;
      else
          (*p_table)(row,4) << RPT_FAIL;

      if ( stressArtifact.CompressionPassed() )
          (*p_table)(row,5) << RPT_PASS;
      else
          (*p_table)(row,5) << RPT_FAIL;

      (*p_table)(row,6) << scalar.SetValue(crackArtifact.GetFsCracking());
      
      if (crackArtifact.Passed() )
         (*p_table)(row,7) << RPT_PASS;
      else
      {
         (*p_table)(row,7) << RPT_FAIL;
      }

      row++;
   }

   // FS for failure
   pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << "Factor of Safety Against Failure";

   p = new rptParagraph;
   *pChapter << p;

   p_table = pgsReportStyleHolder::CreateTableNoHeading(2,"");
   *p << p_table;
   (*p_table)(0,0) << "Factor of Safety Against Failure (FS" << Sub("f") << ")";
   (*p_table)(1,0) << "Allowable Factor of Safety Against Failure";
   (*p_table)(2,0) << "Status";

   (*p_table)(0,1) << scalar.SetValue(pLiftArtifact->GetFsFailure());
   (*p_table)(1,1) << scalar.SetValue(pLiftArtifact->GetAllowableFsForFailure());

   if (pLiftArtifact->PassedFailureCheck())
      (*p_table)(2,1) << RPT_PASS;
   else
      (*p_table)(2,1) << RPT_FAIL;

   p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CLiftingCheck::MakeCopy(const CLiftingCheck& rOther)
{
   // Add copy code here...
}

void CLiftingCheck::MakeAssignment(const CLiftingCheck& rOther)
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
bool CLiftingCheck::AssertValid() const
{
   return true;
}

void CLiftingCheck::Dump(dbgDumpContext& os) const
{
   os << "Dump for CLiftingCheck" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CLiftingCheck::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CLiftingCheck");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CLiftingCheck");

   TESTME_EPILOG("LiveLoadDistributionFactorTable");
}
#endif // _UNITTEST
