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
#include <Reporting\HaulingCheck.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Artifact.h>
#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Project.h>

#include <PgsExt\GirderPointOfInterest.h>
#include <PgsExt\HaulingCheckArtifact.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\CapacityToDemand.h>

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
                              IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                              IEAFDisplayUnits* pDisplayUnits) const
{
#pragma Reminder("UPDATE: it would be more efficient to get the hauling artifact once and pass it down to the lower level functions")
   // Each of the build functions below look up the hauling artifact

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey thisSegmentKey(girderKey,segIdx);
      if ( BuildImpactedStressTable(pChapter,pBroker,thisSegmentKey,pDisplayUnits) )
      {
         BuildInclinedStressTable(pChapter,pBroker,thisSegmentKey,pDisplayUnits);

         BuildOtherTables(pChapter,pBroker,thisSegmentKey,pDisplayUnits);
      }
   }
}

bool CHaulingCheck::BuildImpactedStressTable(rptChapter* pChapter,
                              IBroker* pBroker,const CSegmentKey& segmentKey,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Check for Hauling to Bridge Site [5.5.4.3]")<<rptNewLine;
   *pTitle << _T("Hauling Stresses for Plumb Girder With Impact, and Factor of Safety Against Cracking")<<rptNewLine;

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

   rptCapacityToDemand cap_demand;

   location.IncludeSpanAndGirder(segmentKey.groupIndex == ALL_GROUPS);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if (!pGirderHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      *p <<color(Red)<<_T("Hauling analysis disabled in Project Criteria library entry. No analysis performed.")<<color(Black)<<rptNewLine;
      return false;
   }

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pGdrArtifact = pArtifacts->GetGirderArtifact(segmentKey);
   const pgsSegmentArtifact* pSegmentArtifact = pGdrArtifact->GetSegmentArtifact(segmentKey.segmentIndex);
   const pgsHaulingAnalysisArtifact* pHaulArtifact = pSegmentArtifact->GetHaulingAnalysisArtifact();

   GET_IFACE2(pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker, ILibrary,       pLib );
   std::_tstring specName = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( specName.c_str() );

   Float64 c; // compression coefficient
   Float64 t; // tension coefficient
   Float64 t_max; // maximum allowable tension
   bool b_t_max; // true if max allowable tension is applicable

   c = pSpecEntry->GetHaulingCompStress();
   t = pSpecEntry->GetMaxConcreteTensHauling();
   pSpecEntry->GetAbsMaxConcreteTensHauling(&b_t_max,&t_max);

   Float64 t2 = pSpecEntry->GetMaxConcreteTensWithRebarHauling();

   Float64 capCompression = pGirderHaulingSpecCriteria->GetHaulingAllowableCompressiveConcreteStress(segmentKey);

   *p <<_T("Maximum allowable concrete compressive stress = -") << c << RPT_FC << _T(" = ") << 
      stress.SetValue(capCompression)<< _T(" ") <<
      stress.GetUnitTag()<< rptNewLine;
   *p <<_T("Maximum allowable concrete tensile stress = ") << tension_coeff.SetValue(t) << symbol(ROOT) << RPT_FC;
   if ( b_t_max )
      *p << _T(" but not more than: ") << stress.SetValue(t_max);
   *p << _T(" = ") << stress.SetValue(pGirderHaulingSpecCriteria->GetHaulingAllowableTensileConcreteStress(segmentKey))<< _T(" ") <<
      stress.GetUnitTag()<< rptNewLine;

   *p <<_T("Maximum allowable concrete tensile stress = ") << tension_coeff.SetValue(t2) << symbol(ROOT) << RPT_FC
      << _T(" = ") << stress.SetValue(pGirderHaulingSpecCriteria->GetHaulingWithMildRebarAllowableStress(segmentKey)) << _T(" ") << stress.GetUnitTag()
      << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.") << rptNewLine;

   *p <<_T("Allowable factor of safety against cracking = ")<<pHaulArtifact->GetAllowableFsForCracking()<<rptNewLine; 

   Float64 fc_reqd_comp,fc_reqd_tens, fc_reqd_tens_wrebar;
   pHaulArtifact->GetRequiredConcreteStrength(&fc_reqd_comp,&fc_reqd_tens,&fc_reqd_tens_wrebar);

   *p << RPT_FC << _T(" required for Compressive stress = ");
   if ( 0 < fc_reqd_comp )
      *p << stress_u.SetValue( fc_reqd_comp ) << rptNewLine;
   else
      *p << symbol(INFINITY) << rptNewLine;

   *p << RPT_FC << _T(" required for Tensile stress without sufficient reinforcement = ");
   if ( 0 < fc_reqd_tens )
      *p << stress_u.SetValue( fc_reqd_tens ) << rptNewLine;
   else
      *p << symbol(INFINITY) << rptNewLine;

   *p << RPT_FC << _T(" required for Tensile stress with sufficient reinforcement to resist the tensile force in the concrete = ");
   if ( 0 < fc_reqd_tens_wrebar )
      *p << stress_u.SetValue( fc_reqd_tens_wrebar ) << rptNewLine;
   else
      *p << symbol(INFINITY) << rptNewLine;

   GET_IFACE2(pBroker,IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest);
   std::vector<pgsPointOfInterest> poi_vec;
   poi_vec = pGirderHaulingPointsOfInterest->GetHaulingPointsOfInterest(segmentKey,0,POIFIND_OR);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(11,_T(""));
   *p << p_table;

   int col1=0;
   int col2=0;
   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);

   (*p_table)(0,col1++) << COLHDR(_T("Location from") << rptNewLine << _T("Left Pick Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   p_table->SetColumnSpan(0,col1,2);
   (*p_table)(0,col1++) << _T("Max") << rptNewLine << _T("Demand");
   (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   p_table->SetColumnSpan(0,col1,2);
   (*p_table)(0,col1++) << _T("Min") << rptNewLine << _T("Demand");
   (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   p_table->SetColumnSpan(0,col1,2);
   (*p_table)(0,col1++) << _T("Tensile") << rptNewLine << _T("Capacity");
   (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);
   (*p_table)(0,col1++) << _T("Tension") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);
   (*p_table)(0,col1++) << _T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);
   (*p_table)(0,col1++) << Sub2(_T("FS"),_T("cr"));

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);
   (*p_table)(0,col1++) << _T("FS") << rptNewLine << _T("Status");

   p_table->SetNumberOfHeaderRows(2);
   for ( ColumnIndexType i = col1; i < p_table->GetNumberOfColumns(); i++ )
      p_table->SetColumnSpan(0,i,SKIP_CELL);

   Float64 overhang = pHaulArtifact->GetTrailingOverhang();

   RowIndexType row=2;
   for (std::vector<pgsPointOfInterest>::const_iterator i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      const pgsHaulingStressAnalysisArtifact* stressArtifact = pHaulArtifact->GetHaulingStressAnalysisArtifact(poi.GetDistFromStart());
      const pgsHaulingCrackingAnalysisArtifact* crackArtifact =  pHaulArtifact->GetHaulingCrackingAnalysisArtifact(poi.GetDistFromStart());

      if (stressArtifact==NULL || crackArtifact==NULL)
      {
         ATLASSERT(0); // this should not happen
         continue;
      }
      (*p_table)(row,0) << location.SetValue( POI_HAUL_SEGMENT, poi, overhang );

      // Tension
      Float64 fTensTop, fTensBottom, tensCapacityTop, tensCapacityBottom;
      stressArtifact->GetMaxPlumbTensileStress(&fTensTop, &fTensBottom, &tensCapacityTop, &tensCapacityBottom);

      // Compression
      Float64 fPs, fTopUpward, fTopNoImpact, fTopDownward;
      stressArtifact->GetTopFiberStress(&fPs, &fTopUpward, &fTopNoImpact, &fTopDownward);

      Float64 fBotUpward, fBotNoImpact, fBotDownward;
      stressArtifact->GetBottomFiberStress(&fPs, &fBotUpward, &fBotNoImpact, &fBotDownward);

      Float64 fTopMin = Min3(fTopUpward, fTopNoImpact, fTopDownward);
      Float64 fBotMin = Min3(fBotUpward, fBotNoImpact, fBotDownward);

      ColumnIndexType col = 1;
      (*p_table)(row,col++) << stress.SetValue(fTensTop);
      (*p_table)(row,col++) << stress.SetValue(fTensBottom);
      (*p_table)(row,col++) << stress.SetValue(fTopMin);
      (*p_table)(row,col++) << stress.SetValue(fBotMin);

      if (fTensTop>0)
      {
         (*p_table)(row,col++) << stress.SetValue(tensCapacityTop);
      }
      else
      {
         (*p_table)(row,col++) << _T("-");
      }

      if (fTensBottom>0)
      {
         (*p_table)(row,col++) << stress.SetValue(tensCapacityBottom);
      }
      else
      {
         (*p_table)(row,col++) << _T("-");
      }

      // Determine which c/d controls. top or bottom
      Float64 fTens, capTens;
      if( IsCDLess(cdPositive, tensCapacityTop, fTensTop, tensCapacityBottom, fTensBottom))
      {
         fTens = fTensTop;
         capTens = tensCapacityTop;
      }
      else
      {
         fTens = fTensBottom;
         capTens = tensCapacityBottom;
      }

      if ( stressArtifact->TensionPassed() )
          (*p_table)(row,col++) << RPT_PASS << rptNewLine <<_T("(")<< cap_demand.SetValue(capTens,fTens,true)<<_T(")");
      else
          (*p_table)(row,col++) << RPT_FAIL << rptNewLine <<_T("(")<< cap_demand.SetValue(capTens,fTens,false)<<_T(")");

      Float64 fComp = min(fTopMin, fBotMin);
      
      if ( stressArtifact->CompressionPassed() )
          (*p_table)(row,col++) << RPT_PASS << rptNewLine <<_T("(")<< cap_demand.SetValue(capCompression,fComp,true)<<_T(")");
      else
          (*p_table)(row,col++) << RPT_FAIL << rptNewLine <<_T("(")<< cap_demand.SetValue(capCompression,fComp,false)<<_T(")");

      (*p_table)(row,col++) << scalar.SetValue(crackArtifact->GetFsCracking());
      
      if (crackArtifact->Passed() )
         (*p_table)(row,col++) << RPT_PASS;
      else
      {
         (*p_table)(row,col++) << RPT_FAIL;
      }

      row++;
   }

   return true;
}

void CHaulingCheck::BuildInclinedStressTable(rptChapter* pChapter,
                              IBroker* pBroker,const CSegmentKey& segmentKey,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);
   rptCapacityToDemand cap_demand;

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Hauling Stresses for Inclined Girder Without Impact")<<rptNewLine;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker, ILibrary,       pLib );
   std::_tstring specName = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( specName.c_str() );

   Float64 c = pSpecEntry->GetHaulingCompStress(); // compression coefficient

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pGdrArtifact = pArtifacts->GetGirderArtifact(segmentKey);
   const pgsSegmentArtifact* pSegmentArtifact = pGdrArtifact->GetSegmentArtifact(segmentKey.segmentIndex);
   const pgsHaulingAnalysisArtifact* pHaulArtifact = pSegmentArtifact->GetHaulingAnalysisArtifact();

   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   Float64 all_comp = pGirderHaulingSpecCriteria->GetHaulingAllowableCompressiveConcreteStress(segmentKey);
   Float64 mod_rupture = pHaulArtifact->GetModRupture();
   Float64 mod_rupture_coeff = pHaulArtifact->GetModRuptureCoefficient();

   *p <<_T("Maximum allowable concrete compressive stress = -") << c << RPT_FC << _T(" = ") << 
      stress.SetValue(all_comp)<< _T(" ") << stress.GetUnitTag()<< rptNewLine;

   *p <<_T("Maximum allowable concrete tensile stress, inclined girder without impact = ") << RPT_STRESS(_T("r")) << _T(" = ") << tension_coeff.SetValue(mod_rupture_coeff) << symbol(ROOT) << RPT_FC;
   *p << _T(" = ") << stress.SetValue(mod_rupture)<< _T(" ") << stress.GetUnitTag()<< rptNewLine;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(7,_T(""));
   *p << p_table;
   *p << RPT_STRESS(_T("tu")) << _T(" = top fiber stress, uphill side; ") 
      << RPT_STRESS(_T("td")) << _T(" = top fiber stress, downhill side") << rptNewLine;
   *p << RPT_STRESS(_T("bu")) << _T(" = bottom fiber stress, uphill side; ")
      << RPT_STRESS(_T("bd")) << _T(" = bottom fiber stress, downhill side") << rptNewLine;

   (*p_table)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR(RPT_STRESS(_T("tu")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,2) << COLHDR(RPT_STRESS(_T("td")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,3) << COLHDR(RPT_STRESS(_T("bu")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,4) << COLHDR(RPT_STRESS(_T("bd")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,5) << _T("Tension") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");
   (*p_table)(0,6) << _T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");

   Float64 overhang = pHaulArtifact->GetTrailingOverhang();

   GET_IFACE2(pBroker,IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest);
   std::vector<pgsPointOfInterest> poi_vec;
   poi_vec = pGirderHaulingPointsOfInterest->GetHaulingPointsOfInterest(segmentKey,0,POIFIND_OR);

   RowIndexType row = 1;
   for (std::vector<pgsPointOfInterest>::const_iterator i = poi_vec.begin(); i!= poi_vec.end(); i++)
   {
      const pgsPointOfInterest& poi = *i;

      const pgsHaulingStressAnalysisArtifact* stressArtifact =  pHaulArtifact->GetHaulingStressAnalysisArtifact(poi.GetDistFromStart());
 
      (*p_table)(row,0) << location.SetValue(POI_HAUL_SEGMENT, poi, overhang);

      Float64 ftu, ftd, fbu, fbd;
      stressArtifact->GetInclinedGirderStresses(&ftu,&ftd,&fbu,&fbd);
      (*p_table)(row,1) << stress.SetValue(ftu);
      (*p_table)(row,2) << stress.SetValue(ftd);
      (*p_table)(row,3) << stress.SetValue(fbu);
      (*p_table)(row,4) << stress.SetValue(fbd);

      Float64 fTens = Max4(ftu, ftd, fbu, fbd);
      Float64 fComp = Min4(ftu, ftd, fbu, fbd);
      
      if ( fTens <= mod_rupture )
          (*p_table)(row,5) << RPT_PASS << rptNewLine <<_T("(")<< cap_demand.SetValue(mod_rupture,fTens,true)<<_T(")");
      else
          (*p_table)(row,5) << RPT_FAIL << rptNewLine <<_T("(")<< cap_demand.SetValue(mod_rupture,fTens,false)<<_T(")");

      if ( fComp >= all_comp )
          (*p_table)(row,6) << RPT_PASS << rptNewLine <<_T("(")<< cap_demand.SetValue(all_comp,fComp,true)<<_T(")");
      else
          (*p_table)(row,6) << RPT_FAIL << rptNewLine <<_T("(")<< cap_demand.SetValue(all_comp,fComp,false)<<_T(")");

      row++;
   }
}

void CHaulingCheck::BuildOtherTables(rptChapter* pChapter,
                              IBroker* pBroker,const CSegmentKey& segmentKey,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   // FS for failure
   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Factor of Safety Against Rollover");

   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );

   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc,      pDisplayUnits->GetSpanLengthUnit(), true  );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,    pDisplayUnits->GetShearUnit(),        false );

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = pgsReportStyleHolder::CreateTableNoHeading(2,_T(""));
   p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
   *p << p_table;

   (*p_table)(0,0) << _T("Factor of Safety Against Rollover (FS") << Sub(_T("r")) << _T(")");
   (*p_table)(1,0) << _T("Allowable Factor of Safety Against Rollover");
   (*p_table)(2,0) << _T("Status");

   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pGdrArtifact = pArtifacts->GetGirderArtifact(segmentKey);
   const pgsSegmentArtifact* pSegmentArtifact = pGdrArtifact->GetSegmentArtifact(segmentKey.segmentIndex);
   const pgsHaulingAnalysisArtifact* pHaulArtifact = pSegmentArtifact->GetHaulingAnalysisArtifact();

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
   *pTitle << _T("Spacing Between Truck Supports for Hauling");

   p = new rptParagraph;
   *pChapter << p;

   p_table = pgsReportStyleHolder::CreateTableNoHeading(2,_T(""));
   p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
   *p << p_table;
   (*p_table)(0,0) << _T("Distance Between Supports");
   (*p_table)(1,0) << _T("Max. Allowable Distance Between Supports");
   (*p_table)(2,0) << _T("Status");

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
   *pTitle << _T("Girder Support Configuration");

   p = new rptParagraph;
   *pChapter << p;

   p_table = pgsReportStyleHolder::CreateTableNoHeading(2,_T(""));
   p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
   *p << p_table;

   (*p_table)(0,0) << _T("Leading Overhang (closest to cab of truck)");
   (*p_table)(1,0) << _T("Max. Allowable Leading Overhang");
   (*p_table)(2,0) << _T("Status");
   Float64 oh  = pHaulArtifact->GetTrailingOverhang();
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
   *pTitle << _T("Maximum Girder Weight");

   p = new rptParagraph;
   *pChapter << p;

   p_table = pgsReportStyleHolder::CreateTableNoHeading(2,_T(""));
   p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
   *p << p_table;
   (*p_table)(0,0) << _T("Girder Weight");
   (*p_table)(1,0) << _T("Maximum Allowable Weight");
   (*p_table)(2,0) << _T("Status");
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
   os << _T("Dump for CHaulingCheck") << endl;
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
