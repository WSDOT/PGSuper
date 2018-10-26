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
#include <Reporting\SpecCheckChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\StrandStressCheckTable.h>
#include <Reporting\FlexuralStressCheckTable.h>
#include <Reporting\FlexuralCapacityCheckTable.h>
#include <Reporting\ShearCheckTable.h>
#include <Reporting\InterfaceShearTable.h>
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

#include <Reporting\RatingSummaryTable.h>

#include <MathEx.h>

#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\Allowables.h>
#include <IFace\RatingSpecification.h>

#include <PgsExt\GirderArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static void write_splitting_zone_check(IBroker* pBroker,
                                       IEAFDisplayUnits* pDisplayUnits,
                                       SpanIndexType span,
                                       GirderIndexType gdr,
                                       rptChapter* pChapter);

static void write_confinement_check(IBroker* pBroker,
                                    IEAFDisplayUnits* pDisplayUnits,
                                    SpanIndexType span,
                                    GirderIndexType gdr,
                                    rptChapter* pChapter);

/****************************************************************************
CLASS
   CSpecCheckChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSpecCheckChapterBuilder::CSpecCheckChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
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

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(span, girder);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   *pPara << _T("Specification = ") << pSpec->GetSpecification() << rptNewLine;


   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,girder);
   rptParagraph* p = new rptParagraph;
   *p << CStrandStressCheckTable().Build(pBroker,pArtifact->GetStrandStressArtifact(),pDisplayUnits) << rptNewLine;

   p->SetName(_T("Strand Stresses"));
   *pChapter << p;

   // report the required concrete strengths for the current bridge configuration
   p = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << p;
   *p << _T("Required Concrete Strengths") << rptNewLine;

   p = new rptParagraph;
   *pChapter << p;
   p->SetName(_T("Girder Stresses"));
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   Float64 fci_reqd = pArtifact->GetRequiredReleaseStrength();
   Float64 fc_reqd  = pArtifact->GetRequiredConcreteStrength();
   if ( 0 <= fci_reqd )
   {
      Float64 fci_rounded = IS_SI_UNITS(pDisplayUnits) ? CeilOff(fci_reqd,::ConvertToSysUnits(6,unitMeasure::MPa)) : CeilOff(fci_reqd,::ConvertToSysUnits(100,unitMeasure::PSI));
      *p << _T("Required ") << RPT_FCI << _T(" = ") << stress_u.SetValue(fci_reqd);
      *p << _T(" ") << symbol(RIGHT_DOUBLE_ARROW) << _T(" ") << stress_u.SetValue(fci_rounded) << rptNewLine;
   }
   else
   {
      *p << _T("Required ") << RPT_FCI << _T(" = Regardless of the release strength, the stress requirements will not be satisfied.") << rptNewLine;
   }
   *p << _T("Actual ") << RPT_FCI << _T(" = ") << stress_u.SetValue( pMaterial->GetFciGdr(span,girder)) << rptNewLine;

   *p << rptNewLine;

   if ( 0 <= fc_reqd )
   {
      Float64 fc_rounded = IS_SI_UNITS(pDisplayUnits) ? CeilOff(fc_reqd,::ConvertToSysUnits(6,unitMeasure::MPa)) : CeilOff(fc_reqd,::ConvertToSysUnits(100,unitMeasure::PSI));
      *p << _T("Required ") << RPT_FC  << _T(" = ") << stress_u.SetValue(fc_reqd);
      *p << _T(" ") << symbol(RIGHT_DOUBLE_ARROW) << _T(" ") << stress_u.SetValue(fc_rounded) << rptNewLine;
   }
   else
   {
      *p << _T("Required ") << RPT_FC << _T(" = Regardless of the concrete strength, the stress requirements will not be satisfied.") << rptNewLine;
   }
   *p << _T("Actual ") << RPT_FC << _T(" = ") << stress_u.SetValue( pMaterial->GetFcGdr(span,girder)) << rptNewLine;

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
   p->SetName(_T("Moment Capacities"));
   *p << CFlexuralCapacityCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthI,true,&bOverReinforced) << rptNewLine;
   if ( bOverReinforced )
   {
      *p << _T("* Over reinforced sections may be adequate if M") << Sub(_T("u")) << _T(" does not exceed the minimum resistance specified in LRFD C5.7.3.3.1") << rptNewLine;
      *p << _T("  Limiting capacity of over reinforced sections are shown in parentheses") << rptNewLine;
      *p << _T("  See Moment Capacity Details chapter for additional information") << rptNewLine;
   }
   *pChapter << p;

   // Strength II if permit load exists
   if (bPermit)
   {
      p = new rptParagraph;
      *p << CFlexuralCapacityCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthII,true,&bOverReinforced) << rptNewLine;
      if ( bOverReinforced )
      {
         *p << _T("* Over reinforced sections may be adequate if M") << Sub(_T("u")) << _T(" does not exceed the minimum resistance specified in LRFD C5.7.3.3.1") << rptNewLine;
         *p << _T("  Limiting capacity of over reinforced sections are shown in parentheses") << rptNewLine;
         *p << _T("  See Moment Capacity Details chapter for additional information") << rptNewLine;
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
         *p << _T("* Over reinforced sections may be adequate if M") << Sub(_T("u")) << _T(" does not exceed the minimum resistance specified in LRFD C5.7.3.3.1") << rptNewLine;
         *p << _T("  Limiting capacity of over reinforced sections are shown in parentheses") << rptNewLine;
         *p << _T("  See Moment Capacity Details chapter for additional information") << rptNewLine;
      }
      *pChapter << p;

      // Strength II if permit load exists
      if (bPermit)
      {
         p = new rptParagraph;
         *p << CFlexuralCapacityCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthII,false,&bOverReinforced) << rptNewLine;
         if ( bOverReinforced )
         {
            *p << _T("* Over reinforced sections may be adequate if M") << Sub(_T("u")) << _T(" does not exceed the minimum resistance specified in LRFD C5.7.3.3.1") << rptNewLine;
            *p << _T("  Limiting capacity of over reinforced sections are shown in parentheses") << rptNewLine;
            *p << _T("  See Moment Capacity Details chapter for additional information") << rptNewLine;
         }
         *pChapter << p;
      }
   }

   // Vertical Shear check
   p = new rptParagraph;
   p->SetName(_T("Shear"));
   *pChapter << p;
   bool bStrutAndTieRequired;
   *p << CShearCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthI,bStrutAndTieRequired);

   CShearCheckTable().BuildNotes(pChapter,pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthI,bStrutAndTieRequired);

   if ( bPermit )
   {
      p = new rptParagraph;
      *pChapter << p;
      *p << CShearCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthII,bStrutAndTieRequired) << rptNewLine;

      CShearCheckTable().BuildNotes(pChapter,pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthII,bStrutAndTieRequired);
   }

   // Interface Shear check
   if ( pBridge->IsCompositeDeck() )
   {
      CInterfaceShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthI);

      if ( bPermit )
         CInterfaceShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthII);
   }

   if (pSpecEntry->IsSplittingCheckEnabled())
   {
      // Splitting Zone check
      write_splitting_zone_check(pBroker,pDisplayUnits,span,girder,pChapter);
   }

   if (pSpecEntry->IsConfinementCheckEnabled())
   {
      // confinement check
      write_confinement_check(pBroker,pDisplayUnits,span,girder,pChapter);
   }

   // Longitudinal reinforcement for shear
   CLongReinfShearCheck().Build(pChapter,pBroker,span,girder,pgsTypes::BridgeSite3,pgsTypes::StrengthI,pDisplayUnits);
   if ( bPermit )
      CLongReinfShearCheck().Build(pChapter,pBroker,span,girder,pgsTypes::BridgeSite3,pgsTypes::StrengthII,pDisplayUnits);

   // Optional live load deflection
   COptionalDeflectionCheck().Build(pChapter,pArtifact,span,girder,pDisplayUnits);

   // Lifting
   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);

   if (pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
   {
      p = new rptParagraph;
      p->SetName(_T("Lifting"));
      *pChapter << p;

      CLiftingCheck().Build(pChapter,pBroker,span,girder,pDisplayUnits);
   }

   // Hauling
   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if (pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
   {
      p = new rptParagraph;
      p->SetName(_T("Hauling"));
      *pChapter << p;

      CHaulingCheck().Build(pChapter,pBroker,span,girder,pDisplayUnits);
   }

   // Strand Slope
   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *p << _T("Constructability") << rptNewLine;
   p->SetName(_T("Constructability"));
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

   // _T("A") Dimension check
   std::vector<SpanGirderHashType> girder_list;
   SpanGirderHashType sgh = HashSpanGirder(span, girder);
   girder_list.push_back(sgh);

   rptRcTable* atable = CConstructabilityCheckTable().BuildSlabOffsetTable(pBroker,girder_list,pDisplayUnits);
   if (atable!=NULL)
   { 
      rptParagraph* p = new rptParagraph;
      *p << atable << rptNewLine;
      *pChapter << p;
   }

   // Camber Check
   CConstructabilityCheckTable().BuildCamberCheck(pChapter,pBroker,span,girder,pDisplayUnits);

   // Global Stability Check
   CConstructabilityCheckTable().BuildGlobalGirderStabilityCheck(pChapter,pBroker,span,girder,pDisplayUnits);

   // Load rating
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   if ( !pRatingSpec->AlwaysLoadRate() )
      return pChapter;

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      (*pChapter) << pPara;
      pPara->SetName(_T("Design Load Rating"));
      (*pPara) << pPara->GetName() << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;
      (*pPara) << CRatingSummaryTable().BuildByLimitState(pBroker,girder, CRatingSummaryTable::Design ) << rptNewLine;
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) || pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      (*pChapter) << pPara;
      pPara->SetName(_T("Legal Load Rating"));
      (*pPara) << pPara->GetName() << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         rptRcTable* pTable = CRatingSummaryTable().BuildByVehicle(pBroker,girder, pgsTypes::lrLegal_Routine);
         if ( pTable )
            (*pPara) << pTable << rptNewLine;

         pTable = CRatingSummaryTable().BuildLoadPosting(pBroker,girder, pgsTypes::lrLegal_Routine);
         if ( pTable )
            (*pPara) << pTable << rptNewLine;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         rptRcTable* pTable = CRatingSummaryTable().BuildByVehicle(pBroker,girder, pgsTypes::lrLegal_Special);
         if ( pTable )
            (*pPara) << pTable << rptNewLine;

         pTable = CRatingSummaryTable().BuildLoadPosting(pBroker,girder, pgsTypes::lrLegal_Special);
         if ( pTable )
            (*pPara) << pTable << rptNewLine;
      }
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) || pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      (*pChapter) << pPara;
      pPara->SetName(_T("Permit Load Rating"));
      (*pPara) << pPara->GetName() << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;
      (*pPara) << Super(_T("*")) << _T("MBE 6A.4.5.2 Permit load rating should only be used if the bridge has a rating factor greater than 1.0 when evaluated for AASHTO legal loads.") << rptNewLine;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         rptRcTable* pTable = CRatingSummaryTable().BuildByVehicle(pBroker,girder, pgsTypes::lrPermit_Routine);
         if ( pTable )
            (*pPara) << pTable << rptNewLine;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         rptRcTable* pTable = CRatingSummaryTable().BuildByVehicle(pBroker,girder, pgsTypes::lrPermit_Special);
         if ( pTable )
            (*pPara) << pTable << rptNewLine;
      }
   }

 
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
                               IEAFDisplayUnits* pDisplayUnits,
                               SpanIndexType span,
                               GirderIndexType gdr,
                               rptChapter* pChapter)
{
   GET_IFACE2(pBroker,IStirrupGeometry, pStirrupGeometry);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,gdr);
   const pgsSplittingZoneArtifact* pArtifact = gdrArtifact->GetStirrupCheckArtifact()->GetSplittingZoneArtifact();

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    length, pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,     force,  pDisplayUnits->GetGeneralForceUnit(), true );

   std::_tstring strName;
   if ( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() )
      strName = _T("Splitting");
   else
      strName = _T("Bursting");

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   (*pPara) << strName << _T(" Zone Stirrup Check [5.10.10.1]") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   (*pPara) << Bold(_T("Left End of Girder:")) << rptNewLine;
   (*pPara) << strName << _T(" Zone Length = ") << length.SetValue(pArtifact->GetStartSplittingZoneLength()) << rptNewLine;
   (*pPara) << strName << _T(" Force = ") << force.SetValue(pArtifact->GetStartSplittingForce()) << rptNewLine;
   (*pPara) << strName << _T(" Resistance = ") << force.SetValue(pArtifact->GetStartSplittingResistance()) << rptNewLine;
   (*pPara) << _T("Status = ");
   if ( pArtifact->StartPassed() )
      (*pPara) << RPT_PASS;
   else
      (*pPara) << RPT_FAIL;

   (*pPara) <<rptNewLine<<rptNewLine;

   (*pPara) << Bold(_T("Right End of Girder:")) << rptNewLine;
   (*pPara) << strName << _T(" Zone Length = ") << length.SetValue(pArtifact->GetEndSplittingZoneLength()) << rptNewLine;
   (*pPara) << strName << _T(" Force = ") << force.SetValue(pArtifact->GetEndSplittingForce()) << rptNewLine;
   (*pPara) << strName << _T(" Resistance = ") << force.SetValue(pArtifact->GetEndSplittingResistance()) << rptNewLine;
   (*pPara) << _T("Status = ");
   if ( pArtifact->EndPassed() )
      (*pPara) << RPT_PASS;
   else
      (*pPara) << RPT_FAIL;

}

void write_confinement_check(IBroker* pBroker,
                             IEAFDisplayUnits* pDisplayUnits,
                             SpanIndexType span,
                             GirderIndexType gdr,
                             rptChapter* pChapter)
{
   GET_IFACE2(pBroker,IStirrupGeometry, pStirrupGeometry);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,gdr);
   const pgsStirrupCheckArtifact *pStirrups = gdrArtifact->GetStirrupCheckArtifact();
   const pgsConfinementArtifact& rConfine = pStirrups->GetConfinementArtifact();

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    length, pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,    pDisplayUnits->GetComponentDimUnit(), true );

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   (*pPara) << _T("Confinement Stirrup Check [5.10.10.2]") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   (*pPara) << _T("  Minimum Required Bar Size in Confinement Zone: ")<< lrfdRebarPool::GetBarSize(rConfine.GetMinBar()->GetSize()) << rptNewLine;
   (*pPara) << _T("  Maximum Required Bar Spacing in Confinement Zone = ")<< dim.SetValue(rConfine.GetSMax()) << rptNewLine << rptNewLine;

   (*pPara) << Bold(_T("Left End of Girder:")) << rptNewLine;
   (*pPara) << _T("  Required Confinement Zone Length: ")<<rConfine.GetZoneLengthFactor()<<_T("d = ")
            <<rConfine.GetZoneLengthFactor()<<_T(" *")<<length.SetValue(rConfine.GetStartd())<<_T(" = ")
            << length.SetValue(rConfine.GetStartRequiredZoneLength()) << rptNewLine;
   (*pPara) << _T("  Provided Confinement Zone Length within Required Zone Length = ") << length.SetValue(rConfine.GetStartProvidedZoneLength()) << rptNewLine;
   matRebar::Size size = rConfine.GetStartBar()==NULL ? matRebar::bsNone : rConfine.GetStartBar()->GetSize();
   (*pPara) << _T("  Bar Size in Zone: ")<< lrfdRebarPool::GetBarSize(size) << rptNewLine;
   (*pPara) << _T("  Bar Spacing in Zone = ")<< dim.SetValue(rConfine.GetStartS()) << rptNewLine;
   (*pPara) << _T("  Status = ");
   if ( rConfine.StartPassed() )
      (*pPara) << RPT_PASS;
   else
      (*pPara) << RPT_FAIL;

   (*pPara) <<rptNewLine<<rptNewLine;

   (*pPara) << Bold(_T("Right End of Girder:")) << rptNewLine;
   (*pPara) << _T("  Required Confinement Zone Length: ")<<rConfine.GetZoneLengthFactor()<<_T("d = ")
            <<rConfine.GetZoneLengthFactor()<<_T(" *")<<length.SetValue(rConfine.GetEndd())<<_T(" = ")
            << length.SetValue(rConfine.GetEndRequiredZoneLength()) << rptNewLine;
   (*pPara) << _T("  Provided Confinement Zone Length within Required Zone Length = ") << length.SetValue(rConfine.GetEndProvidedZoneLength()) << rptNewLine;
   size = rConfine.GetEndBar()==NULL ? matRebar::bsNone : rConfine.GetEndBar()->GetSize();
   (*pPara) << _T("  Bar Size in Zone: ")<< lrfdRebarPool::GetBarSize(size) << rptNewLine;
   (*pPara) << _T("  Bar Spacing in Zone = ")<< dim.SetValue(rConfine.GetEndS()) << rptNewLine;
   (*pPara) << _T("  Status = ");
   if ( rConfine.EndPassed() )
      (*pPara) << RPT_PASS;
   else
      (*pPara) << RPT_FAIL;

}
