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
#include <Reporting\SpecCheckChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\StrandStressCheckTable.h>
#include <Reporting\FlexuralStressCheckTable.h>
#include <Reporting\FlexuralCapacityCheckTable.h>
#include <Reporting\ShearCheckTable.h>
#include <Reporting\InterfaceShearTable.h>
#include <Reporting\ConfinementCheckTable.h>
#include <Reporting\StrandSlopeCheck.h>
#include <Reporting\HoldDownForceCheck.h>
#include <Reporting\ConstructabilityCheckTable.h>
#include <Reporting\CamberTable.h>
#include <Reporting\GirderDetailingCheck.h>
#include <Reporting\LiftingCheck.h>
#include <Reporting\HaulingCheck.h>
#include <Reporting\LongReinfShearCheck.h>
#include <Reporting\OptionalDeflectionCheck.h>
#include <Reporting\DebondCheckTable.h>
#include <Reporting\ContinuityCheck.h>

#include <MathEx.h>

#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\Allowables.h>

#include <PgsExt\GirderArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void write_splitting_zone_check(IBroker* pBroker,
                               IDisplayUnits* pDisplayUnits,
                               SpanIndexType span,
                               GirderIndexType gdr,
                               rptChapter* pChapter);

/****************************************************************************
CLASS
   CSpecCheckChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSpecCheckChapterBuilder::CSpecCheckChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CSpecCheckChapterBuilder::GetName() const
{
   return TEXT("Specification Checks");
}

rptChapter* CSpecCheckChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   *pPara << "Specification = " << pSpec->GetSpecification() << rptNewLine;


   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,girder);
   rptParagraph* p = new rptParagraph;
   *p << CStrandStressCheckTable().Build(pBroker,pArtifact->GetStrandStressArtifact(),pDisplayUnits) << rptNewLine;

   p->SetName("Strand Stresses");
   *pChapter << p;

   // report the required concrete strengths for the current bridge configuration
   p = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << p;
   *p << "Required Concrete Strengths" << rptNewLine;

   p = new rptParagraph;
   *pChapter << p;
   p->SetName("Girder Stresses");
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   double fci_reqd = pArtifact->GetRequiredReleaseStrength();
   double fc_reqd  = pArtifact->GetRequiredConcreteStrength();
   if ( 0 <= fci_reqd )
   {
      double fci_rounded = IS_SI_UNITS(pDisplayUnits) ? CeilOff(fci_reqd,::ConvertToSysUnits(6,unitMeasure::MPa)) : CeilOff(fci_reqd,::ConvertToSysUnits(100,unitMeasure::PSI));
      *p << "Required " << RPT_FCI << " = " << stress_u.SetValue(fci_reqd);
      *p << " " << symbol(RIGHT_DOUBLE_ARROW) << " " << stress_u.SetValue(fci_rounded) << rptNewLine;

      *p << "Actual " << RPT_FCI << " = " << stress_u.SetValue( pMaterial->GetFciGdr(span,girder)) << rptNewLine;
   }
   else
   {
      *p << "Regardless of the release strength, the stress requirements will not be satisfied." << rptNewLine;
   }

   *p << rptNewLine;

   if ( 0 <= fc_reqd )
   {
      double fc_rounded = IS_SI_UNITS(pDisplayUnits) ? CeilOff(fc_reqd,::ConvertToSysUnits(6,unitMeasure::MPa)) : CeilOff(fc_reqd,::ConvertToSysUnits(100,unitMeasure::PSI));
      *p << "Required " << RPT_FC  << " = " << stress_u.SetValue(fc_reqd);
      *p << " " << symbol(RIGHT_DOUBLE_ARROW) << " " << stress_u.SetValue(fc_rounded) << rptNewLine;

      *p << "Actual " << RPT_FC << " = " << stress_u.SetValue( pMaterial->GetFcGdr(span,girder)) << rptNewLine;
   }
   else
   {
      *p << "Regardless of the concrete strength, the stress requirements will not be satisfied." << rptNewLine;
   }

   // report flexural stresses at various stages
   CContinuityCheck().Build(pChapter,pBroker,span,girder,pDisplayUnits);
   CFlexuralStressCheckTable().Build(pChapter,pBroker,span,girder,pDisplayUnits,pgsTypes::CastingYard,pgsTypes::ServiceI);

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType Nt    = pStrandGeom->GetNumStrands(span,girder,pgsTypes::Temporary);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(span,girder,pgsTypes::Temporary);

//   CFlexuralStressCheckTable().Build(pChapter,pBroker,span,girder,pDisplayUnits,pgsTypes::GirderPlacement,pgsTypes::ServiceI);

   if ( 0 < NtMax && 0 < Nt )
      CFlexuralStressCheckTable().Build(pChapter,pBroker,span,girder,pDisplayUnits,pgsTypes::TemporaryStrandRemoval,pgsTypes::ServiceI);

   CFlexuralStressCheckTable().Build(pChapter,pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite1,pgsTypes::ServiceI);
   CFlexuralStressCheckTable().Build(pChapter,pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite2,pgsTypes::ServiceI,   pgsTypes::Compression);
   CFlexuralStressCheckTable().Build(pChapter,pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::ServiceI,   pgsTypes::Compression);

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      CFlexuralStressCheckTable().Build(pChapter,pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::ServiceIA,  pgsTypes::Compression);

   CFlexuralStressCheckTable().Build(pChapter,pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::ServiceIII, pgsTypes::Tension);

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      CFlexuralStressCheckTable().Build(pChapter,pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::FatigueI,  pgsTypes::Compression);

   // Flexural Capacity

// NOTE
// No longer designing/checking for ultimate moment in temporary construction state
// per e-mail from Bijan Khaleghi, dated 4/28/1999.  See project log.
//   p = new rptParagraph;
//   *p << CFlexuralCapacityCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite1,pgsTypes::StrengthI) << rptNewLine;
//   *pChapter << p;

   p = new rptParagraph;
   bool bOverReinforced;
   p->SetName("Moment Capacities");
   *p << CFlexuralCapacityCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthI,true,&bOverReinforced) << rptNewLine;
   if ( bOverReinforced )
   {
      *p << "* Over reinforced sections may be adequate if M" << Sub("u") << " does not exceed the minimum resistance specified in LRFD C5.7.3.3.1" << rptNewLine;
      *p << "  Limiting capacity of over reinforced sections are shown in parentheses" << rptNewLine;
      *p << "  See Moment Capacity Details chapter for additional information" << rptNewLine;
   }
   *pChapter << p;

   // Strength II if permit load exists
   if (bPermit)
   {
      p = new rptParagraph;
      *p << CFlexuralCapacityCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthII,true,&bOverReinforced) << rptNewLine;
      if ( bOverReinforced )
      {
         *p << "* Over reinforced sections may be adequate if M" << Sub("u") << " does not exceed the minimum resistance specified in LRFD C5.7.3.3.1" << rptNewLine;
         *p << "  Limiting capacity of over reinforced sections are shown in parentheses" << rptNewLine;
         *p << "  See Moment Capacity Details chapter for additional information" << rptNewLine;
      }
      *pChapter << p;
   }
   
   *p << rptNewLine;

   if ( pBridge->ProcessNegativeMoments(span) )
   {
      p = new rptParagraph;
      *p << CFlexuralCapacityCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthI,false,&bOverReinforced) << rptNewLine;
      if ( bOverReinforced )
      {
         *p << "* Over reinforced sections may be adequate if M" << Sub("u") << " does not exceed the minimum resistance specified in LRFD C5.7.3.3.1" << rptNewLine;
         *p << "  Limiting capacity of over reinforced sections are shown in parentheses" << rptNewLine;
         *p << "  See Moment Capacity Details chapter for additional information" << rptNewLine;
      }
      *pChapter << p;

      // Strength II if permit load exists
      if (bPermit)
      {
         p = new rptParagraph;
         *p << CFlexuralCapacityCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthII,false,&bOverReinforced) << rptNewLine;
         if ( bOverReinforced )
         {
            *p << "* Over reinforced sections may be adequate if M" << Sub("u") << " does not exceed the minimum resistance specified in LRFD C5.7.3.3.1" << rptNewLine;
            *p << "  Limiting capacity of over reinforced sections are shown in parentheses" << rptNewLine;
            *p << "  See Moment Capacity Details chapter for additional information" << rptNewLine;
         }
         *pChapter << p;
      }
   }

   // Vertical Shear check
   p = new rptParagraph;
   p->SetName("Shear");
   bool bStrutAndTieRequired;
   *pChapter << p;
   *p << CShearCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthI,bStrutAndTieRequired) << rptNewLine;

   if ( bStrutAndTieRequired )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << p;
      *p << STRUT_AND_TIE_REQUIRED << rptNewLine << rptNewLine;
   }
   else
   {
      p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << p;
      *p << SUPPORT_COMPRESSION << rptNewLine << rptNewLine;
   }

   if ( bPermit )
   {
      p = new rptParagraph;
      *pChapter << p;
      *p << CShearCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthII,bStrutAndTieRequired) << rptNewLine;

      if ( bStrutAndTieRequired )
      {
         p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
         *pChapter << p;
         *p << STRUT_AND_TIE_REQUIRED << rptNewLine << rptNewLine;
      }
      else
      {
         p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
         *pChapter << p;
         *p << SUPPORT_COMPRESSION << rptNewLine << rptNewLine;
      }
   }

   // Interface Shear check
   if ( pBridge->IsCompositeDeck() )
   {
      CInterfaceShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthI);

      if ( bPermit )
         CInterfaceShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthII);
   }

   // Optional live load deflection
   COptionalDeflectionCheck().Build(pChapter,pArtifact,span,girder,pDisplayUnits);

   if (pSpecEntry->IsAnchorageCheckEnabled())
   {
      // Splitting Zone check
      write_splitting_zone_check(pBroker,pDisplayUnits,span,girder,pChapter);

      // confinement check
      p = new rptParagraph;
      *p << CConfinementCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3) << rptNewLine;
      *pChapter << p;
   }

   // Longitudinal reinforcement for shear
   CLongReinfShearCheck().Build(pChapter,pBroker,span,girder,pgsTypes::BridgeSite3,pgsTypes::StrengthI,pDisplayUnits);
   if ( bPermit )
      CLongReinfShearCheck().Build(pChapter,pBroker,span,girder,pgsTypes::BridgeSite3,pgsTypes::StrengthII,pDisplayUnits);

   // Lifting
   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);

   if (pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
   {
      p = new rptParagraph;
      p->SetName("Lifting");
      *pChapter << p;

      CLiftingCheck().Build(pChapter,pBroker,span,girder,pDisplayUnits);
   }

   // Hauling
   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if (pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
   {
      p = new rptParagraph;
      p->SetName("Hauling");
      *pChapter << p;

      CHaulingCheck().Build(pChapter,pBroker,span,girder,pDisplayUnits);
   }

   // Strand Slope
   p = new rptParagraph;
   p->SetName("Constructability");
   *pChapter << p;

   // Girder Detailing
   CGirderDetailingCheck().Build(pChapter,pBroker,span,girder,pDisplayUnits);

   // Debonding
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   if ( pStrandGeometry->GetNumDebondedStrands(span,girder,pgsTypes::Straight) )
   {
      CDebondCheckTable().Build(pChapter, pBroker,span,girder,pgsTypes::Straight, pDisplayUnits);
   }

   CStrandSlopeCheck().Build(pChapter,pBroker,span,girder,pDisplayUnits);

   // Hold Down Force
   rptRcTable* hdtable = CHoldDownForceCheck().Build(pBroker,span,girder,pDisplayUnits);
   if (hdtable!=NULL)
   {
      p = new rptParagraph;
      *p << hdtable << rptNewLine;
      *pChapter << p;
   }

   // "A" Dimension check
   rptRcTable* atable = CConstructabilityCheckTable().BuildSlabOffsetTable(pBroker,span,girder,pDisplayUnits);
   if (atable!=NULL)
   { 
      rptParagraph* p = new rptParagraph;
      *p << atable << rptNewLine;
      *pChapter << p;
   }

   // Global Stability Check
   CConstructabilityCheckTable().BuildGlobalGirderStabilityCheck(pChapter,pBroker,span,girder,pDisplayUnits);
 
   return pChapter;
}

CChapterBuilder* CSpecCheckChapterBuilder::Clone() const
{
   return new CSpecCheckChapterBuilder;
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

void write_splitting_zone_check(IBroker* pBroker,
                               IDisplayUnits* pDisplayUnits,
                               SpanIndexType span,
                               GirderIndexType gdr,
                               rptChapter* pChapter)
{
   GET_IFACE2(pBroker,IStirrupGeometry, pStirrupGeometry);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,gdr);
   const pgsSplittingZoneArtifact* pArtifact = gdrArtifact->GetSplittingZoneArtifact();

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    length, pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,     force,  pDisplayUnits->GetGeneralForceUnit(), true );

   std::string strName;
   if ( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() )
      strName = "Splitting";
   else
      strName = "Bursting";

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   (*pPara) << strName << " Zone Stirrup Check [5.10.10.1]" << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   (*pPara) << strName << " Zone Length = " << length.SetValue(pArtifact->GetSplittingZoneLength()) << rptNewLine;
   (*pPara) << strName << " Force = " << force.SetValue(pArtifact->GetSplittingForce()) << rptNewLine;
   (*pPara) << strName << " Resistance = " << force.SetValue(pArtifact->GetSplittingResistance()) << rptNewLine;
   (*pPara) << "Status = ";
   if ( pArtifact->Passed() )
      (*pPara) << RPT_PASS;
   else
      (*pPara) << RPT_FAIL;
}
