///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include "TogaStressChecksChapterBuilder.h"
#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignUtilities.h"

#include <Reporting\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\FlexuralStressCheckTable.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\Allowables.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\CapacityToDemand.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CTogaStressChecksChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTogaStressChecksChapterBuilder::CTogaStressChecksChapterBuilder():
CPGSuperChapterBuilder(true)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTogaStressChecksChapterBuilder::GetName() const
{
   return TEXT("Stress Checks");
}

rptChapter* CTogaStressChecksChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,ISpecification,pSpec);
   *pPara << _T("Specification = ") << pSpec->GetSpecification() << rptNewLine;

   *pPara << Bold(_T("Notes: "))<< rptNewLine;
   *pPara <<symbol(DOT)<<_T(" Calculated total external load top and bottom stresses are multiplied by the appropriate (Top or Bottom) ratio of (Input Design Load Stress)/(Calculated Stress).");
   *pPara << _T(" This results in the Analysis Stress")<<rptNewLine;
   *pPara <<symbol(DOT)<<_T(" Stress Checks reflect the following sign convention: Compressive stress is negative. Tensile stress is positive.");

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   BuildTableAndNotes(pChapter,pBroker,pDisplayUnits,pgsTypes::CastingYard,pgsTypes::ServiceI);
   BuildTableAndNotes(pChapter,pBroker,pDisplayUnits,pgsTypes::BridgeSite1,pgsTypes::ServiceI);
   BuildTableAndNotes(pChapter,pBroker,pDisplayUnits,pgsTypes::BridgeSite2,pgsTypes::ServiceI,pgsTypes::Compression);
   BuildTableAndNotes(pChapter,pBroker,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::ServiceI,pgsTypes::Compression);

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      BuildTableAndNotes(pChapter,pBroker,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::ServiceIA,pgsTypes::Compression);

   BuildTableAndNotes(pChapter,pBroker,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension);

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      BuildTableAndNotes(pChapter,pBroker,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::FatigueI,pgsTypes::Compression);

   return pChapter;
}

void CTogaStressChecksChapterBuilder::BuildTableAndNotes(rptChapter* pChapter, IBroker* pBroker,
                      IEAFDisplayUnits* pDisplayUnits,
                      pgsTypes::Stage stage,
                      pgsTypes::LimitState ls,
                      pgsTypes::StressType stress) const
{
   // We need the artifact that we've doctored for txdot reasons
   GET_IFACE2(pBroker,IGetTogaResults,pGetTogaResults);
   const pgsGirderArtifact* pFactoredGdrArtifact = pGetTogaResults->GetFabricatorDesignArtifact();

   // Write notes from pgsuper default table, then our notes, then table
  CFlexuralStressCheckTable().BuildNotes(pChapter, pFactoredGdrArtifact, pBroker, TOGA_SPAN, TOGA_FABR_GDR, pDisplayUnits,
              stage, ls, stress);


   if (stage!=pgsTypes::CastingYard)
   {
     // Toga Special notes
      rptParagraph* p = new rptParagraph;
      *pChapter << p;

      Float64 stress_val, stress_fac, stress_loc;
      pGetTogaResults-> GetControllingCompressiveStress(&stress_val, &stress_fac, &stress_loc);
      *p<<_T("Ratio applied to Top Stresses = ")<< stress_fac << rptNewLine;

      pGetTogaResults->GetControllingTensileStress(&stress_val, &stress_fac, &stress_loc);
      *p<<_T("Ratio applied to Bottom Stresses = ")<< stress_fac << rptNewLine;
   }


   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pUnfactoredGdrArtifact = pIArtifact->GetArtifact(TOGA_SPAN, TOGA_FABR_GDR);

   BuildTable(pChapter, pBroker, pFactoredGdrArtifact, pUnfactoredGdrArtifact,
              pDisplayUnits, stage, ls, stress);
}

void CTogaStressChecksChapterBuilder::BuildTable(rptChapter* pChapter, IBroker* pBroker,
                      const pgsGirderArtifact* pFactoredGdrArtifact, const pgsGirderArtifact* pUnfactoredGdrArtifact,
                      IEAFDisplayUnits* pDisplayUnits,
                      pgsTypes::Stage stage,
                      pgsTypes::LimitState limitState,
                      pgsTypes::StressType stressType) const
{
   USES_CONVERSION;

   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress, pDisplayUnits->GetStressUnit(), false );

   rptCapacityToDemand cap_demand;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   // create and set up table
   const ColumnIndexType add_cols = 2;
   rptRcTable* p_table;
   if (stage == pgsTypes::BridgeSite3 && limitState == pgsTypes::ServiceIII)
      p_table = pgsReportStyleHolder::CreateDefaultTable(5+1,_T(""));
   else if (stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3)
      p_table = pgsReportStyleHolder::CreateDefaultTable(8+add_cols,_T(""));
   else if (stage == pgsTypes::CastingYard )
      p_table = pgsReportStyleHolder::CreateDefaultTable(10,_T(""));
   else
      p_table = pgsReportStyleHolder::CreateDefaultTable(9+add_cols,_T(""));

   *p << p_table;


   int col1=0;
   int col2=0;

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);
   if ( stage == pgsTypes::CastingYard )
      (*p_table)(0,col1++) << COLHDR(RPT_GDR_END_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   else
      (*p_table)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   GET_IFACE2(pBroker, IStageMap, pStageMap );
   std::_tstring strLimitState = OLE2T(pStageMap->GetLimitStateName(limitState));

   if ( limitState == pgsTypes::ServiceIII )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1++) << COLHDR(_T("Prestress") << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << _T("Prestress");
      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( limitState == pgsTypes::ServiceIII )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1++) << COLHDR(_T("Calculated") << rptNewLine << strLimitState << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1++) << COLHDR(_T("Analysis") << rptNewLine << strLimitState << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else if (stage==pgsTypes::CastingYard)
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << strLimitState;
      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << _T("Calculated") << rptNewLine << strLimitState;
      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << _T("Analysis") << rptNewLine << strLimitState;
      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( limitState == pgsTypes::ServiceIII )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1++) << COLHDR(_T("Demand") << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << _T("Demand");
      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   // get allowable stresses
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( TOGA_SPAN, TOGA_FABR_GDR, stage, POI_FLEXURESTRESS | POI_TABULAR );
   CHECK(vPoi.size()>0);

   const pgsFlexuralStressArtifact* pFactoredStressArtifact(NULL);

   Float64 allowable_tension;
   Float64 allowable_tension_with_rebar;
   Float64 allowable_compression;

   if (stressType==pgsTypes::Tension && (stage != pgsTypes::BridgeSite2))
   {
      pFactoredStressArtifact = pFactoredGdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(stage,limitState,pgsTypes::Tension,vPoi.begin()->GetDistFromStart()) );
      allowable_tension = pFactoredStressArtifact->GetCapacity();
      allowable_tension_with_rebar = pFactoredGdrArtifact->GetCastingYardCapacityWithMildRebar();
   }

   if (stressType==pgsTypes::Compression || stage != pgsTypes::BridgeSite3)
   {
      pFactoredStressArtifact = pFactoredGdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(stage,limitState,pgsTypes::Compression,vPoi.begin()->GetDistFromStart()) );
      allowable_compression = pFactoredStressArtifact->GetCapacity();
   }


   if (stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3 )
   {
      if ( stage == pgsTypes::BridgeSite2 || (stage == pgsTypes::BridgeSite3 && limitState != pgsTypes::ServiceIII) )
      {
         p_table->SetRowSpan(0,col1,2);
         p_table->SetRowSpan(1,col2++,SKIP_CELL);
         (*p_table)(0,col1++) <<_T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");
      }

      if ( stage == pgsTypes::BridgeSite3 && limitState == pgsTypes::ServiceIII )
      {
         p_table->SetRowSpan(0,col1,2);
         p_table->SetRowSpan(1,col2++,SKIP_CELL);
         (*p_table)(0,col1) <<_T("Tension") << rptNewLine << _T("Status");
         if ( !IsZero(allowable_tension) )
            (*p_table)(0,col1) << rptNewLine << _T("(C/D)");

         col1++;
      }
   }
   else if ( stage == pgsTypes::CastingYard )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1++) << _T("Tension") << rptNewLine << _T("Status");
      if ( !IsZero(allowable_tension) )
         (*p_table)(0,col1-1) << rptNewLine << _T("w/o rebar") << rptNewLine << _T("(C/D)");

      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1++) << _T("Tension") << rptNewLine << _T("Status") << rptNewLine << _T("w/ rebar");
      if ( !IsZero(allowable_tension_with_rebar) )
         (*p_table)(0,col1-1) << rptNewLine << _T("(C/D)");

      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1++) << _T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");
   }
   else
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1++) <<_T("Tension")<<rptNewLine<<_T("Status");
      if ( !IsZero(allowable_tension) )
         (*p_table)(0,col1-1) << rptNewLine << _T("(C/D)");

      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1++) <<_T("Compression")<<rptNewLine<<_T("Status") << rptNewLine << _T("(C/D)");
   }

   p_table->SetNumberOfHeaderRows(2);
   for ( ColumnIndexType i = col1; i < p_table->GetNumberOfColumns(); i++ )
      p_table->SetColumnSpan(0,i,SKIP_CELL);

   // Fill up the table
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(TOGA_SPAN,TOGA_FABR_GDR);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      ColumnIndexType col = 0;

      const pgsPointOfInterest& poi = *iter;
      (*p_table)(row,col) << location.SetValue( stage, poi, end_size );

      const pgsFlexuralStressArtifact* pFactoredOtherStressArtifact=0;
      const pgsFlexuralStressArtifact* pUnfactoredStressArtifact=0;

      if(stage==pgsTypes::BridgeSite2 || stage==pgsTypes::BridgeSite3)
      {
         pFactoredStressArtifact = pFactoredGdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(stage,limitState,stressType,poi.GetDistFromStart()) );
         ATLASSERT( pFactoredStressArtifact != NULL );
         if ( pFactoredStressArtifact == NULL )
            continue;

         pUnfactoredStressArtifact = pUnfactoredGdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(stage,limitState,stressType,poi.GetDistFromStart()) );
         ATLASSERT( pUnfactoredStressArtifact != NULL );
         if ( pUnfactoredStressArtifact == NULL )
            continue;
      }
      else
      {
         // get both tension and compression for other than bss3
         pFactoredStressArtifact      = pFactoredGdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(stage,limitState,pgsTypes::Tension,poi.GetDistFromStart()) );
         ATLASSERT( pFactoredStressArtifact != NULL );
         if ( pFactoredStressArtifact == NULL )
            continue;

         pFactoredOtherStressArtifact = pFactoredGdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(stage,limitState,pgsTypes::Compression,poi.GetDistFromStart()) );
         ATLASSERT( pFactoredOtherStressArtifact != NULL );
         if ( pFactoredOtherStressArtifact == NULL )
            continue;

         pUnfactoredStressArtifact      = pUnfactoredGdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(stage,limitState,pgsTypes::Tension,poi.GetDistFromStart()) );
         ATLASSERT( pUnfactoredStressArtifact != NULL );
         if ( pUnfactoredStressArtifact == NULL )
            continue;
      }

      Float64 fTop, fBot;
      // prestress
      pFactoredStressArtifact->GetPrestressEffects( &fTop, &fBot );
      if (limitState != pgsTypes::ServiceIII)
      {
         (*p_table)(row,++col) << stress.SetValue( fTop );
      }

      (*p_table)(row,++col) << stress.SetValue( fBot );

      // Calculated
      if (stage != pgsTypes::CastingYard)
      {
         pUnfactoredStressArtifact->GetExternalEffects( &fTop, &fBot );
         if (limitState != pgsTypes::ServiceIII)
         {
            (*p_table)(row,++col) << stress.SetValue( fTop );
         }

         (*p_table)(row,++col) << stress.SetValue( fBot );
      }

      // Analysis
      pFactoredStressArtifact->GetExternalEffects( &fTop, &fBot );
      if (limitState != pgsTypes::ServiceIII)
      {
         (*p_table)(row,++col) << stress.SetValue( fTop );
      }

      (*p_table)(row,++col) << stress.SetValue( fBot );

      // Demands
      pFactoredStressArtifact->GetDemand( &fTop, &fBot );
      if (limitState != pgsTypes::ServiceIII)
      {
         (*p_table)(row,++col) << stress.SetValue( fTop );
      }

      (*p_table)(row,++col) << stress.SetValue( fBot );

      // Tension w/o rebar
      if ( stage == pgsTypes::CastingYard || 
//           stage == pgsTypes::GirderPlacement || 
           stage == pgsTypes::TemporaryStrandRemoval || 
           stage == pgsTypes::BridgeSite1 || 
          (stage == pgsTypes::BridgeSite3 && limitState == pgsTypes::ServiceIII)
         )
      {
         bool bPassed = (limitState == pgsTypes::ServiceIII ? pFactoredStressArtifact->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar) : pFactoredStressArtifact->Passed(pgsFlexuralStressArtifact::WithoutRebar));
	      if ( bPassed )
		     (*p_table)(row,++col) << RPT_PASS;
	      else
		     (*p_table)(row,++col) << RPT_FAIL;

         if ( !IsZero(allowable_tension) )
         {
            double f = (limitState == pgsTypes::ServiceIII ? fBot : max(fBot,fTop));
           (*p_table)(row,col) << rptNewLine <<_T("(")<< cap_demand.SetValue(allowable_tension,f,bPassed)<<_T(")");
         }
      }

      // Tension w/ rebar
      if ( stage == pgsTypes::CastingYard )
      {
         bool bPassed = ( fTop <= allowable_tension_with_rebar) && (fBot <= allowable_tension_with_rebar);
         if (bPassed)
         {
           (*p_table)(row,++col) << RPT_PASS;
         }
         else
         {
           (*p_table)(row,++col) << RPT_FAIL;
         }

         if ( !IsZero(allowable_tension_with_rebar) )
         {
            double f = max(fTop,fBot);
            (*p_table)(row,col) << rptNewLine <<_T("(")<< cap_demand.SetValue(allowable_tension_with_rebar,f,bPassed)<<_T(")");
          }
      }

      // Compression
      if (stage == pgsTypes::CastingYard || 
//          stage == pgsTypes::GirderPlacement ||
          stage == pgsTypes::TemporaryStrandRemoval ||
          stage == pgsTypes::BridgeSite1 ||
          stage == pgsTypes::BridgeSite2 ||
         (stage == pgsTypes::BridgeSite3 && (limitState == pgsTypes::ServiceI || limitState == pgsTypes::ServiceIA || limitState == pgsTypes::FatigueI))
         )
      {
         bool bPassed;
         if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3 )
         {
            bPassed = pFactoredStressArtifact->Passed(pgsFlexuralStressArtifact::WithoutRebar);
         }
         else
         {
            bPassed = pFactoredOtherStressArtifact->Passed(pgsFlexuralStressArtifact::WithoutRebar);
            pFactoredOtherStressArtifact->GetDemand( &fTop, &fBot );
         }

         if ( bPassed )
            (*p_table)(row, ++col) << RPT_PASS;
	      else
		      (*p_table)(row, ++col) << RPT_FAIL;

         double f = min(fTop,fBot);
         (*p_table)(row,col) << rptNewLine <<_T("(")<< cap_demand.SetValue(allowable_compression,f,bPassed)<<_T(")");
      }

      row++;
   }
}

CChapterBuilder* CTogaStressChecksChapterBuilder::Clone() const
{
   return new CTogaStressChecksChapterBuilder;
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

