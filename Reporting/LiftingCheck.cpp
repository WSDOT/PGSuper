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
#include <Reporting\LiftingCheck.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Artifact.h>
#include <EAF\EAFDisplayUnits.h>
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
                              IEAFDisplayUnits* pDisplayUnits) const
{

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Check for Lifting In Casting Yard [5.5.4.3]")<<rptNewLine;
   *pTitle << _T("Lifting Stresses and Factor of Safety Against Cracking")<<rptNewLine;

   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,    pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress_u, pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);

   location.IncludeSpanAndGirder(span == ALL_SPANS);


   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   if (!pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
   {
      *p <<color(Red)<<_T("Lifting analysis disabled in Project Criteria library entry. No analysis performed.")<<color(Black)<<rptNewLine;
      return;
   }

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,girder);
   const pgsLiftingCheckArtifact* pLiftArtifact = pArtifact->GetLiftingCheckArtifact();

   // unstable girders are a problem
   if (!pLiftArtifact->IsGirderStable())
   {
      *pTitle<<_T("Warning! - Girder is unstable - CG is higher than pick points")<<rptNewLine;
   }

   GET_IFACE2(pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker, ILibrary,       pLib );
   std::_tstring specName = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( specName.c_str() );

   Float64 c; // compression coefficient
   Float64 t; // tension coefficient
   Float64 t_max; // maximum allowable tension
   bool b_t_max; // true if max allowable tension is applicable

   c = pSpecEntry->GetCyCompStressLifting();
   t = pSpecEntry->GetCyMaxConcreteTensLifting();
   pSpecEntry->GetCyAbsMaxConcreteTensLifting(&b_t_max,&t_max);

   Float64 t2 = pSpecEntry->GetMaxConcreteTensWithRebarLifting();

   *p <<_T("Maximum allowable concrete compressive stress = -") << c << RPT_FCI << _T(" = ") << 
      stress.SetValue(pLiftArtifact->GetAllowableCompressionStress())<< _T(" ") <<
      stress.GetUnitTag()<< rptNewLine;
   *p <<_T("Maximum allowable concrete tensile stress = ") << tension_coeff.SetValue(t) << symbol(ROOT) << RPT_FCI;
   if ( b_t_max )
      *p << _T(" but not more than: ") << stress.SetValue(t_max);
   *p << _T(" = ") << stress.SetValue(pLiftArtifact->GetAllowableTensileStress())<< _T(" ") <<
      stress.GetUnitTag()<< rptNewLine;

   Float64 As_reqd = pLiftArtifact->GetAlterantiveTensileStressAsMax();
   *p <<_T("Maximum allowable concrete tensile stress = ") << tension_coeff.SetValue(t2) << symbol(ROOT) << RPT_FCI
       << _T(" = ") << stress.SetValue(pLiftArtifact->GetAlternativeTensionAllowableStress()) << _T(" ") << stress.GetUnitTag();
   if ( !IsZero(As_reqd) )
       *p << _T(" if at least ") << area.SetValue(As_reqd) << _T(" of mild reinforcement is provided") << rptNewLine;
   else
       *p << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.") << rptNewLine;

   *p <<_T("Allowable factor of safety against cracking = ")<<pLiftArtifact->GetAllowableFsForCracking()<<rptNewLine;

   Float64 fc_reqd_comp,fc_reqd_tens;
   bool min_rebar_reqd;
   pLiftArtifact->GetRequiredConcreteStrength(&fc_reqd_comp,&fc_reqd_tens,&min_rebar_reqd);

   Float64 fci_reqd = max(fc_reqd_comp,fc_reqd_tens);

   if ( 0 < fci_reqd )
      *p << RPT_FCI << _T(" required to satisfy this stress check = ") << stress_u.SetValue( fci_reqd ) << rptNewLine;

   GET_IFACE2(pBroker,IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
   std::vector<pgsPointOfInterest> poi_vec;
   poi_vec = pGirderLiftingPointsOfInterest->GetLiftingPointsOfInterest(span,girder,POI_FLEXURESTRESS);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(8,_T(""));
   *p << p_table;
   (*p_table)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Pick Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR(_T("Min") << rptNewLine << _T("Stress"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,2) << COLHDR(_T("Max") << rptNewLine << _T("Stress"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,3) << _T("Tension") << rptNewLine << _T("Status") << rptNewLine << _T("w/o Rebar");
   (*p_table)(0,4) << _T("Tension") << rptNewLine << _T("Status") << rptNewLine << _T("w/  Rebar");
   (*p_table)(0,5) << _T("Compression") << rptNewLine << _T("Status");
   (*p_table)(0,6) << Sub2(_T("FS"),_T("cr"));
   (*p_table)(0,7) << _T("FS") << rptNewLine << _T("Status");

   Float64 overhang = (pLiftArtifact->GetGirderLength() - pLiftArtifact->GetClearSpanBetweenPickPoints())/2.0;

   RowIndexType row=1;
   for (std::vector<pgsPointOfInterest>::const_iterator i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      pgsLiftingStressCheckArtifact stressArtifact = pLiftArtifact->GetLiftingStressCheckArtifact(poi.GetDistFromStart());
      pgsLiftingCrackingCheckArtifact crackArtifact =  pLiftArtifact->GetLiftingCrackingCheckArtifact(poi.GetDistFromStart());
 
      (*p_table)(row,0) << location.SetValue( pgsTypes::Lifting,poi,overhang );
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
   *pTitle << _T("Factor of Safety Against Failure");

   p = new rptParagraph;
   *pChapter << p;

   p_table = pgsReportStyleHolder::CreateTableNoHeading(2,_T(""));
   *p << p_table;
   (*p_table)(0,0) << _T("Factor of Safety Against Failure (FS") << Sub(_T("f")) << _T(")");
   (*p_table)(1,0) << _T("Allowable Factor of Safety Against Failure");
   (*p_table)(2,0) << _T("Status");

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
   os << _T("Dump for CLiftingCheck") << endl;
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
