///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Project.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\LiftingAnalysisArtifact.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <limits>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLiftingCheck
****************************************************************************/
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

CLiftingCheck& CLiftingCheck::operator= (const CLiftingCheck& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CLiftingCheck::Build(rptChapter* pChapter,
                          IBroker* pBroker,const CGirderKey& girderKey,
                          IEAFDisplayUnits* pDisplayUnits) const
{

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Check for Lifting In Casting Yard [5.5.4.3]")<<rptNewLine;
   *pTitle << _T("Lifting Stresses and Factor of Safety Against Cracking")<<rptNewLine;

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   INIT_UV_PROTOTYPE( rptPointOfInterest,   location,      pDisplayUnits->GetSpanLengthUnit(),         false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,    force,         pDisplayUnits->GetShearUnit(),              false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,   dim,           pDisplayUnits->GetComponentDimUnit(),       false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,   stress,        pDisplayUnits->GetStressUnit(),             false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,   stress_u,      pDisplayUnits->GetStressUnit(),             true  );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,     area,          pDisplayUnits->GetAreaUnit(),               true  );

   rptCapacityToDemand cap_demand;

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   if (!pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled())
   {
      *p <<color(Red)<<_T("Lifting analysis disabled in Project Criteria library entry. No analysis performed.")<<color(Black)<<rptNewLine;
      return;
   }

   GET_IFACE2(pBroker,IArtifact,pArtifacts);

   GET_IFACE2(pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker, ILibrary,       pLib );
   std::_tstring specName = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( specName.c_str() );

   Float64 c; // compression coefficient
   Float64 t; // tension coefficient
   Float64 t_max; // maximum allowable tension
   bool b_t_max; // true if max allowable tension is applicable

   c = pSpecEntry->GetLiftingCompressionStressFactor();
   t = pSpecEntry->GetLiftingTensionStressFactor();
   pSpecEntry->GetLiftingMaximumTensionStress(&b_t_max,&t_max);

   Float64 t2 = pSpecEntry->GetLiftingTensionStressFactorWithRebar();

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey thisSegmentKey(girderKey,segIdx);

      if ( 1 < nSegments )
      {
         p = new rptParagraph( pgsReportStyleHolder::GetSubheadingStyle() );
         *pChapter << p;
         *p << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }

      p = new rptParagraph;
      *pChapter << p;

      const pgsLiftingAnalysisArtifact* pLiftArtifact = pArtifacts->GetLiftingAnalysisArtifact(thisSegmentKey);

      // unstable girders are a problem
      if (!pLiftArtifact->IsGirderStable())
      {
         *pTitle<<_T("Warning! - Girder is unstable - CG is higher than pick points")<<rptNewLine;
      }

      Float64 capCompression = pSegmentLiftingSpecCriteria->GetLiftingAllowableCompressiveConcreteStress(thisSegmentKey);

      *p <<_T("Maximum allowable concrete compressive stress = -") << c << RPT_FCI << _T(" = ") << 
         stress.SetValue(capCompression)<< _T(" ") <<
         stress.GetUnitTag()<< rptNewLine;
      *p <<_T("Maximum allowable concrete tensile stress = ") << tension_coeff.SetValue(t) << symbol(ROOT) << RPT_FCI;
      if ( b_t_max )
      {
         *p << _T(" but not more than: ") << stress.SetValue(t_max);
      }
      *p << _T(" = ") << stress.SetValue(pSegmentLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStress(thisSegmentKey))<< _T(" ") <<
         stress.GetUnitTag()<< rptNewLine;

      *p <<_T("Maximum allowable concrete tensile stress = ") << tension_coeff.SetValue(t2) << symbol(ROOT) << RPT_FCI
         << _T(" = ") << stress.SetValue(pSegmentLiftingSpecCriteria->GetLiftingWithMildRebarAllowableStress(thisSegmentKey)) << _T(" ") << stress.GetUnitTag()
         << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.") << rptNewLine;

      *p <<_T("Allowable factor of safety against cracking = ")<<pLiftArtifact->GetAllowableFsForCracking()<<rptNewLine;

      Float64 fc_reqd_comp,fc_reqd_tens, fc_reqd_tens_wrebar;
      pLiftArtifact->GetRequiredConcreteStrength(&fc_reqd_comp,&fc_reqd_tens,&fc_reqd_tens_wrebar);

      *p << RPT_FCI << _T(" required for Compressive stress = ");
      if ( 0 < fc_reqd_comp )
      {
         *p << stress_u.SetValue( fc_reqd_comp ) << rptNewLine;
      }
      else
      {
         *p << symbol(INFINITY) << rptNewLine;
      }

      *p << RPT_FCI << _T(" required for Tensile stress without sufficient reinforcement = ");
      if ( 0 < fc_reqd_tens )
      {
         *p << stress_u.SetValue( fc_reqd_tens ) << rptNewLine;
      }
      else
      {
         *p << symbol(INFINITY) << rptNewLine;
      }

      *p << RPT_FCI << _T(" required for Tensile stress with sufficient reinforcement to resist the tensile force in the concrete = ");
      if ( 0 < fc_reqd_tens_wrebar )
      {
         *p << stress_u.SetValue( fc_reqd_tens_wrebar ) << rptNewLine;
      }
      else
      {
         *p << symbol(INFINITY) << rptNewLine;
      }

      rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(11,_T(""));
      *p << p_table;

      ColumnIndexType col1 = 0;
      ColumnIndexType col2 = 0;
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
      {
         p_table->SetColumnSpan(0,i,SKIP_CELL);
      }

      Float64 overhang = pLiftArtifact->GetLeftOverhang();

      RowIndexType row = p_table->GetNumberOfHeaderRows();

      const std::vector<pgsPointOfInterest>& vPoi( pLiftArtifact->GetLiftingPointsOfInterest() );
      std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
      for ( ; poiIter != poiIterEnd; poiIter++ )
      {
         const pgsPointOfInterest& poi = *poiIter;

         const pgsLiftingStressAnalysisArtifact*   pStressArtifact = pLiftArtifact->GetLiftingStressAnalysisArtifact(poi);
         const pgsLiftingCrackingAnalysisArtifact* pCrackArtifact  = pLiftArtifact->GetLiftingCrackingAnalysisArtifact(poi);

         if (pStressArtifact == NULL || pCrackArtifact == NULL)
         {
            ATLASSERT(false); // this should not happen
            continue;
         }
         (*p_table)(row,0) << location.SetValue( POI_LIFT_SEGMENT, poi, overhang );

         // Tension
         Float64 fTensTop, fTensBottom, tensCapacityTop, tensCapacityBottom;
         pStressArtifact->GetMaxTensileStress(&fTensTop, &fTensBottom, &tensCapacityTop, &tensCapacityBottom);

         // Compression
         Float64 fPs, fTopUpward, fTopNoImpact, fTopDownward;
         pStressArtifact->GetTopFiberStress(&fPs, &fTopUpward, &fTopNoImpact, &fTopDownward);

         Float64 fBotUpward, fBotNoImpact, fBotDownward;
         pStressArtifact->GetBottomFiberStress(&fPs, &fBotUpward, &fBotNoImpact, &fBotDownward);

         Float64 fTopMin = Min(fTopUpward, fTopNoImpact, fTopDownward);
         Float64 fBotMin = Min(fBotUpward, fBotNoImpact, fBotDownward);

         ColumnIndexType col = 1;
         (*p_table)(row,col++) << stress.SetValue(fTensTop);
         (*p_table)(row,col++) << stress.SetValue(fTensBottom);
         (*p_table)(row,col++) << stress.SetValue(fTopMin);
         (*p_table)(row,col++) << stress.SetValue(fBotMin);

         if (0 < fTensTop)
         {
            (*p_table)(row,col++) << stress.SetValue(tensCapacityTop);
         }
         else
         {
            (*p_table)(row,col++) << _T("-");
         }

         if (0 < fTensBottom)
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

         if ( pStressArtifact->TensionPassed() )
         {
             (*p_table)(row,col++) << RPT_PASS << rptNewLine <<_T("(")<< cap_demand.SetValue(capTens,fTens,true)<<_T(")");
         }
         else
         {
             (*p_table)(row,col++) << RPT_FAIL << rptNewLine <<_T("(")<< cap_demand.SetValue(capTens,fTens,false)<<_T(")");
         }

         Float64 fComp = Min(fTopMin, fBotMin);
         
         if ( pStressArtifact->CompressionPassed() )
         {
             (*p_table)(row,col++) << RPT_PASS << rptNewLine <<_T("(")<< cap_demand.SetValue(capCompression,fComp,true)<<_T(")");
         }
         else
         {
             (*p_table)(row,col++) << RPT_FAIL << rptNewLine <<_T("(")<< cap_demand.SetValue(capCompression,fComp,false)<<_T(")");
         }

         Float64 FScr = pCrackArtifact->GetFsCracking();
         if ( FScr == Float64_Inf )
         {
            (*p_table)(row,col++) << symbol(INFINITY);
         }
         else
         {
            (*p_table)(row,col++) << scalar.SetValue(pCrackArtifact->GetFsCracking());
         }
         
         if (pCrackArtifact->Passed() )
         {
            (*p_table)(row,col++) << RPT_PASS;
         }
         else
         {
            (*p_table)(row,col++) << RPT_FAIL;
         }

         row++;
      }

      // FS for failure
      p = new rptParagraph;
      *pChapter << p;

      p_table = pgsReportStyleHolder::CreateTableNoHeading(2);

      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT));
      p_table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));

      *p << p_table;
      (*p_table)(0,0) << _T("Factor of Safety Against Failure (FS") << Sub(_T("f")) << _T(")");
      (*p_table)(1,0) << _T("Allowable Factor of Safety Against Failure");
      (*p_table)(2,0) << _T("Status");

      (*p_table)(0,1) << scalar.SetValue(pLiftArtifact->GetFsFailure());
      (*p_table)(1,1) << scalar.SetValue(pLiftArtifact->GetAllowableFsForFailure());

      if (pLiftArtifact->PassedFailureCheck())
      {
         (*p_table)(2,1) << RPT_PASS;
      }
      else
      {
         (*p_table)(2,1) << RPT_FAIL;
      }
   } // next segment
}

void CLiftingCheck::MakeCopy(const CLiftingCheck& rOther)
{
   // Add copy code here...
}

void CLiftingCheck::MakeAssignment(const CLiftingCheck& rOther)
{
   MakeCopy( rOther );
}
