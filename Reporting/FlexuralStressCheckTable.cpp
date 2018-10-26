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
#include <Reporting\FlexuralStressCheckTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\FlexuralStressArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\Artifact.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CFlexuralStressCheckTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CFlexuralStressCheckTable::CFlexuralStressCheckTable()
{
}

CFlexuralStressCheckTable::CFlexuralStressCheckTable(const CFlexuralStressCheckTable& rOther)
{
   MakeCopy(rOther);
}

CFlexuralStressCheckTable::~CFlexuralStressCheckTable()
{
}

//======================== OPERATORS  =======================================
CFlexuralStressCheckTable& CFlexuralStressCheckTable::operator= (const CFlexuralStressCheckTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CFlexuralStressCheckTable::Build(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                           IDisplayUnits* pDisplayUnits,
                                           pgsTypes::Stage stage,
                                           pgsTypes::LimitState limitState,
                                           pgsTypes::StressType stressType) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress, pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);

   rptCapacityToDemand cap_demand;

   if ( stage == pgsTypes::CastingYard )
      location.MakeGirderPoi();
   else
      location.MakeSpanPoi();

   std::string strStage;
   std::string aux_msg1;
   switch(stage)
   {
   case pgsTypes::CastingYard:
      strStage = "Casting Yard Stage (At Release)";
      aux_msg1 = "For temporary stresses before losses ";
      break;

//   case pgsTypes::GirderPlacement:
//      strStage = "Girder Placement";
//      aux_msg1 = "For stresses at service limit state after losses ";
//      break;

   case pgsTypes::TemporaryStrandRemoval:
      strStage = "Temporary Strand Removal";
      aux_msg1 = "For stresses at service limit state after losses ";
      break;

   case pgsTypes::BridgeSite1:
      strStage = "Deck and Diaphragm Placement (Bridge Site 1)";
      aux_msg1 = "For stresses at service limit state after losses ";
      break;

   case pgsTypes::BridgeSite2:
      strStage = "Superimposed Dead Loads (Bridge Site 2)";
      aux_msg1 = "For stresses at service limit state after losses ";
      break;

   case pgsTypes::BridgeSite3:
      strStage = "Final with Live Load (Bridge Site 3)";
      aux_msg1 = "For stresses at service limit state after losses ";
      break;

   default:
      ATLASSERT(false);
   }

   std::string strLimitState;
   std::string aux_msg2;
   switch(limitState)
   {
   case pgsTypes::ServiceI:
      strLimitState = "Service I";
      if (stage==pgsTypes::CastingYard)
      {
         if (stressType == pgsTypes::Compression)
            aux_msg2 = "in areas other than the precompressed tensile zones and without bonded auxiliary reinforcement.";
         else
            aux_msg2 = "in pretensioned components";
      }
      else if (/*stage == pgsTypes::GirderPlacement ||*/ stage == pgsTypes::TemporaryStrandRemoval || stage==pgsTypes::BridgeSite1 || stage==pgsTypes::BridgeSite2)
      {
         if (stressType == pgsTypes::Compression)
            aux_msg2 = "in other than segmentally constructed bridges due to permanent loads";
         else
            aux_msg2 = "for components with bonded prestressing tendons other than piles";
      }
      else if (stage==pgsTypes::BridgeSite3)
      {
         if (stressType == pgsTypes::Compression)
            aux_msg2 = "in other than segmentally constructed bridges due to permanent and transient loads";
         else
            ATLASSERT(0); // shouldn't happen
      }
      else
         ATLASSERT(0);

      break;

   case pgsTypes::ServiceIA:
      strLimitState = "Service IA";

      if (stage==pgsTypes::BridgeSite3)
      {
         if (stressType == pgsTypes::Compression)
            aux_msg2 = "in other than segmentally constructed bridges due to live load plus one-half of the permanent loads";
         else
            CHECK(0); // shouldn't happen
      }
      else
         CHECK(0);

      break;

   case pgsTypes::ServiceIII:
      strLimitState = "Service III";

      if (stage==pgsTypes::BridgeSite3)
      {
         if (stressType == pgsTypes::Tension)
            aux_msg2 = "which involve traffic loading in members with bonded prestressing tendons other than piles";
         else
            CHECK(0); // shouldn't happen
      }
      else
         CHECK(0);

      break;

   case pgsTypes::FatigueI:
      strLimitState = "Fatigue I";

      if (stage==pgsTypes::BridgeSite3)
      {
         if (stressType == pgsTypes::Compression)
            aux_msg2 = "in other than segmentally constructed bridges due to the Fatigue I load combination and one-half the sum of effective prestress and permanent loads";
         else
            CHECK(0); // shouldn't happen
      }
      else
         CHECK(0);

      break;
   }

   GET_IFACE2(pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker, ILibrary,       pLib );
   GET_IFACE2(pBroker, IEnvironment,   pEnvironment);

   std::string specName = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( specName.c_str() );
   double c; // compression coefficient
   double t; // tension coefficient
   double t_max; // maximum allowable tension
   double t_with_rebar; // allowable tension when sufficient rebar is used
   bool b_t_max; // true if max allowable tension is applicable

   switch ( stage )
   {
   case pgsTypes::CastingYard:
      c = pSpecEntry->GetCyCompStressService();
      t = pSpecEntry->GetCyMaxConcreteTens();
      t_with_rebar = pSpecEntry->GetCyMaxConcreteTensWithRebar();
      pSpecEntry->GetCyAbsMaxConcreteTens(&b_t_max,&t_max);
      break;

//   case pgsTypes::GirderPlacement:
   case pgsTypes::TemporaryStrandRemoval:
      c = pSpecEntry->GetTempStrandRemovalCompStress();
      t = pSpecEntry->GetTempStrandRemovalMaxConcreteTens();
      pSpecEntry->GetTempStrandRemovalAbsMaxConcreteTens(&b_t_max,&t_max);
      break;

   case pgsTypes::BridgeSite1:
      c = pSpecEntry->GetBs1CompStress();
      t = pSpecEntry->GetBs1MaxConcreteTens();
      pSpecEntry->GetBs1AbsMaxConcreteTens(&b_t_max,&t_max);
      break;

   case pgsTypes::BridgeSite2:
      c = pSpecEntry->GetBs2CompStress();
      break;

   case pgsTypes::BridgeSite3:
      c = (limitState == pgsTypes::ServiceIA || limitState == pgsTypes::FatigueI ? pSpecEntry->GetBs3CompStressService1A() : pSpecEntry->GetBs3CompStressService() );
      t = (pEnvironment->GetExposureCondition() == expNormal ? pSpecEntry->GetBs3MaxConcreteTensNc() : pSpecEntry->GetBs3MaxConcreteTensSc() );
      pEnvironment->GetExposureCondition() == expNormal ? pSpecEntry->GetBs3AbsMaxConcreteTensNc(&b_t_max,&t_max) : pSpecEntry->GetBs3AbsMaxConcreteTensSc(&b_t_max,&t_max);
      break;

   default:
      ATLASSERT(false);
   }

   std::string article; // LRFD article number
   if ( stage == pgsTypes::CastingYard && stressType == pgsTypes::Compression )
      article = "[5.9.4.1.1]";
   else if ( stage == pgsTypes::CastingYard && stressType == pgsTypes::Tension )
      article = "[5.9.4.1.2]";
//   else if ( stage == pgsTypes::GirderPlacement && stressType == pgsTypes::Compression )
//      article = "[5.9.4.2.1]";
//   else if ( stage == pgsTypes::GirderPlacement && stressType == pgsTypes::Tension )
//      article = "[5.9.4.2.2]";
   else if ( stage == pgsTypes::TemporaryStrandRemoval && stressType == pgsTypes::Compression )
      article = "[5.9.4.2.1]";
   else if ( stage == pgsTypes::TemporaryStrandRemoval && stressType == pgsTypes::Tension )
      article = "[5.9.4.2.2]";
   else if ( stage == pgsTypes::BridgeSite1 && stressType == pgsTypes::Compression )
      article = "[5.9.4.2.1]";
   else if ( stage == pgsTypes::BridgeSite1 && stressType == pgsTypes::Tension )
      article = "[5.9.4.2.2]";
   else if ( stage == pgsTypes::BridgeSite2 && stressType == pgsTypes::Compression )
      article = "[5.9.4.2.1]";
   else if ( stage == pgsTypes::BridgeSite3 && stressType == pgsTypes::Compression )
      article = (limitState == pgsTypes::ServiceIA ? "[5.9.4.2.1]" : "[5.5.3.1]");
   else if ( stage == pgsTypes::BridgeSite3 && stressType == pgsTypes::Tension )
      article = "[5.9.4.2.2]";
   else
      ATLASSERT(false);

   std::ostringstream os;
   if (stage == pgsTypes::BridgeSite3)
   {
      os << "Stress Check for " << (stressType == pgsTypes::Compression ? "Compressive" : "Tensile" ) 
         << " Stresses for " << strLimitState << " for " << strStage << std::endl;
   }
   else
   {
      os << "Stress Check for " << strLimitState << " for " << strStage << std::endl;
   }

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << os.str()  << " " << article;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;
   *p << aux_msg1 << aux_msg2 <<rptNewLine;

   // get allowable stresses
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( stage, span, girder, POI_FLEXURESTRESS | POI_TABULAR );
   CHECK(vPoi.size()>0);

   const pgsFlexuralStressArtifact* pArtifact;

   Float64 allowable_tension;
   Float64 allowable_tension_with_rebar;
   Float64 allowable_compression;

   if (stressType==pgsTypes::Tension && (stage != pgsTypes::BridgeSite2))
   {
      pArtifact = gdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(stage,limitState,pgsTypes::Tension,vPoi.begin()->GetDistFromStart()) );
      allowable_tension = pArtifact->GetCapacity();
      allowable_tension_with_rebar = gdrArtifact->GetCastingYardCapacityWithMildRebar();
      *p << "Allowable tensile stress";

      if ( stage == pgsTypes::BridgeSite3 )
          *p << " in the precompressed tensile zone";

      *p << " = " << tension_coeff.SetValue(t) << symbol(ROOT);

      if ( stage == pgsTypes::CastingYard )
         *p << RPT_FCI;
      else
         *p << RPT_FC;

      if ( b_t_max )
         *p << " but not more than " << stress_u.SetValue(t_max);

      *p  << " = " << stress_u.SetValue(allowable_tension)<<rptNewLine;

      if ( stage == pgsTypes::CastingYard )
      {
          Float64 As_reqd = gdrArtifact->GetCastingYardMildRebarRequirement();
          *p << "Allowable tensile stress = " << tension_coeff.SetValue(t_with_rebar) << symbol(ROOT) << RPT_FCI;
          *p << " = " << stress_u.SetValue(allowable_tension_with_rebar);
          if ( !IsZero(As_reqd) )
             *p << " if at least " << area.SetValue(As_reqd) << " of mild reinforcement is provided" << rptNewLine;
          else
             *p << " if bonded reinforcement sufficient to resist the tensile force in the concrete is provided." << rptNewLine;
      }
   }

   if (stressType==pgsTypes::Compression || stage != pgsTypes::BridgeSite3)
   {
      pArtifact = gdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(stage,limitState,pgsTypes::Compression,vPoi.begin()->GetDistFromStart()) );
      allowable_compression = pArtifact->GetCapacity();
      *p << "Allowable compressive stress = -" << c;
      if (stage == pgsTypes::CastingYard)
         *p << RPT_FCI;
      else
         *p << RPT_FC;
      *p << " = " <<stress_u.SetValue(allowable_compression)<<rptNewLine;
   }

   double fc_reqd = gdrArtifact->GetRequiredConcreteStrength(stage,limitState);
   if ( 0 < fc_reqd )
   {
      if ( stage == pgsTypes::CastingYard )
         *p << RPT_FCI << " required to satisfy this stress check = " << stress_u.SetValue( fc_reqd ) << rptNewLine;
      else
         *p << RPT_FC  << " required to satisfy this stress check = " << stress_u.SetValue( fc_reqd ) << rptNewLine;
   }
   else if ( IsZero(fc_reqd) )
   {
      // do nothing if exactly zero
   }
   else
   {
      *p << "Regardless of the concrete strength, the stress requirements will not be satisfied." << rptNewLine;
   }

   // create and set up table
   rptRcTable* p_table;
   if (stage == pgsTypes::BridgeSite3 && limitState == pgsTypes::ServiceIII)
      p_table = pgsReportStyleHolder::CreateDefaultTable(5,"");
   else if (stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3)
      p_table = pgsReportStyleHolder::CreateDefaultTable(8,"");
   else if (stage == pgsTypes::CastingYard )
      p_table = pgsReportStyleHolder::CreateDefaultTable(10,"");
   else
      p_table = pgsReportStyleHolder::CreateDefaultTable(9,"");

   *p << p_table;

   int col1=0;
   int col2=0;

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,-1);
   if ( stage == pgsTypes::CastingYard )
      (*p_table)(0,col1++) << COLHDR(RPT_GDR_END_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   else
      (*p_table)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   if ( limitState == pgsTypes::ServiceIII )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) << COLHDR("Prestress" << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << "Prestress";
      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( limitState == pgsTypes::ServiceIII )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) << COLHDR(strLimitState << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << strLimitState;
      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( limitState == pgsTypes::ServiceIII )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) << COLHDR("Demand" << rptNewLine << RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << "Demand";
      (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if (stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3 )
   {
      if ( stage == pgsTypes::BridgeSite2 || (stage == pgsTypes::BridgeSite3 && limitState != pgsTypes::ServiceIII) )
      {
         p_table->SetRowSpan(0,col1,2);
         p_table->SetRowSpan(1,col2++,-1);
         (*p_table)(0,col1++) <<"Compression" << rptNewLine << "Status" << rptNewLine << "(C/D)";
      }

      if ( stage == pgsTypes::BridgeSite3 && limitState == pgsTypes::ServiceIII )
      {
         p_table->SetRowSpan(0,col1,2);
         p_table->SetRowSpan(1,col2++,-1);
         (*p_table)(0,col1) <<"Tension" << rptNewLine << "Status";
         if ( !IsZero(allowable_tension) )
            (*p_table)(0,col1) << rptNewLine << "(C/D)";

         col1++;
      }
   }
   else if ( stage == pgsTypes::CastingYard )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) << "Tension" << rptNewLine << "Status";
      if ( !IsZero(allowable_tension) )
         (*p_table)(0,col1-1) << rptNewLine << "w/o rebar" << rptNewLine << "(C/D)";

      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) << "Tension" << rptNewLine << "Status" << rptNewLine << "w/ rebar";
      if ( !IsZero(allowable_tension_with_rebar) )
         (*p_table)(0,col1-1) << rptNewLine << "(C/D)";

      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) << "Compression" << rptNewLine << "Status" << rptNewLine << "(C/D)";
   }
   else
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) <<"Tension"<<rptNewLine<<"Status";
      if ( !IsZero(allowable_tension) )
         (*p_table)(0,col1-1) << rptNewLine << "(C/D)";

      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,-1);
      (*p_table)(0,col1++) <<"Compression"<<rptNewLine<<"Status" << rptNewLine << "(C/D)";
   }

   p_table->SetNumberOfHeaderRows(2);
   for ( ColumnIndexType i = col1; i < p_table->GetNumberOfColumns(); i++ )
      p_table->SetColumnSpan(0,i,-1);

   // Fill up the table
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      ColumnIndexType col = 0;

      const pgsPointOfInterest& poi = *iter;
      (*p_table)(row,col) << location.SetValue( poi, end_size );

      const pgsFlexuralStressArtifact* pOtherArtifact=0;
      if(stage==pgsTypes::BridgeSite2 || stage==pgsTypes::BridgeSite3)
      {
         pArtifact = gdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(stage,limitState,stressType,poi.GetDistFromStart()) );
         ATLASSERT( pArtifact != NULL );
         if ( pArtifact == NULL )
            continue;
      }
      else
      {
         // get both tension and compression for other than bss3
         pArtifact      = gdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(stage,limitState,pgsTypes::Tension,poi.GetDistFromStart()) );
         ATLASSERT( pArtifact != NULL );
         if ( pArtifact == NULL )
            continue;

         pOtherArtifact = gdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(stage,limitState,pgsTypes::Compression,poi.GetDistFromStart()) );
         ATLASSERT( pOtherArtifact != NULL );
         if ( pOtherArtifact == NULL )
            continue;
      }

      Float64 fTop, fBot;

      pArtifact->GetPrestressEffects( &fTop, &fBot );
      if (limitState != pgsTypes::ServiceIII)
      {
         (*p_table)(row,++col) << stress.SetValue( fTop );
      }

      (*p_table)(row,++col) << stress.SetValue( fBot );

      pArtifact->GetExternalEffects( &fTop, &fBot );
      if (limitState != pgsTypes::ServiceIII)
      {
         (*p_table)(row,++col) << stress.SetValue( fTop );
      }

      (*p_table)(row,++col) << stress.SetValue( fBot );

      pArtifact->GetDemand( &fTop, &fBot );
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
         bool bPassed = (limitState == pgsTypes::ServiceIII ? pArtifact->BottomPassed(pgsFlexuralStressArtifact::WithoutRebar) : pArtifact->Passed(pgsFlexuralStressArtifact::WithoutRebar));
	      if ( bPassed )
		     (*p_table)(row,++col) << RPT_PASS;
	      else
		     (*p_table)(row,++col) << RPT_FAIL;

         if ( !IsZero(allowable_tension) )
         {
            double f = (limitState == pgsTypes::ServiceIII ? fBot : max(fBot,fTop));
           (*p_table)(row,col) << rptNewLine <<"("<< cap_demand.SetValue(allowable_tension,f,bPassed)<<")";
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
            (*p_table)(row,col) << rptNewLine <<"("<< cap_demand.SetValue(allowable_tension_with_rebar,f,bPassed)<<")";
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
            bPassed = pArtifact->Passed(pgsFlexuralStressArtifact::WithoutRebar);
         }
         else
         {
            bPassed = pOtherArtifact->Passed(pgsFlexuralStressArtifact::WithoutRebar);
            pOtherArtifact->GetDemand( &fTop, &fBot );
         }

         if ( bPassed )
            (*p_table)(row, ++col) << RPT_PASS;
	      else
		      (*p_table)(row, ++col) << RPT_FAIL;

         double f = min(fTop,fBot);
         (*p_table)(row,col) << rptNewLine <<"("<< cap_demand.SetValue(allowable_compression,f,bPassed)<<")";
      }

      row++;
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CFlexuralStressCheckTable::MakeCopy(const CFlexuralStressCheckTable& rOther)
{
   // Add copy code here...
}

void CFlexuralStressCheckTable::MakeAssignment(const CFlexuralStressCheckTable& rOther)
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
bool CFlexuralStressCheckTable::AssertValid() const
{
   return true;
}

void CFlexuralStressCheckTable::Dump(dbgDumpContext& os) const
{
   os << "Dump for CCombinedMomentsTable" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CFlexuralStressCheckTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CFlexuralStressCheckTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CFlexuralStressCheckTable");

   TESTME_EPILOG("CFlexuralStressCheckTable");
}
#endif // _UNITTEST
