///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <Reporting\EquilibriumCheckChapterBuilder.h>
#include <Reporting\EquilibriumCheckReportSpecification.h>
#include <IFace\PointOfInterest.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>
#include <IFace\ReportOptions.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CEquilibriumCheckChapterBuilder::CEquilibriumCheckChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CEquilibriumCheckChapterBuilder::GetName() const
{
   return TEXT("Equilibrium Check");
}

rptChapter* CEquilibriumCheckChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGdrRptSpec = std::dynamic_pointer_cast<const CEquilibriumCheckReportSpecification>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGdrRptSpec->GetBroker(&pBroker);

   pgsPointOfInterest poi(pGdrRptSpec->GetPOI());
   IntervalIndexType intervalIdx = pGdrRptSpec->GetInterval();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      *pPara << color(Red) << _T("Time Step analysis results not available.") << color(Black) << rptNewLine;
      return pChapter;
   }

   *pPara << _T("This is a debugging report used to check equilibrium between internal and external forces.") << rptNewLine;
   *pPara << _T("At each interval, there is a change in internal and external forces. In a loading interval, external forces are applied and the change in internal component forces must equal the external forces.") << rptNewLine;
   *pPara << _T("Similarly, in a time step interval when no loads are applied, creep, shrinkage, and relaxation deformations are restrained causing internal forces in the section components. These internal forces must be in equilibrium.") << rptNewLine;
   *pPara << _T("This report lists the forces occuring during an interval and the change in force in each section component. Verify that the Total Internal and Total External change in each component force are equal.") << rptNewLine;
   *pPara << _T("If the analysis is correct, this report isn't very interesting. If, however, the analysis is firing asserts (DEBUG and BETA builds), this report is helpful in diagosing the problem") << rptNewLine;

   *pPara << rptNewLine << rptNewLine;

   CGirderKey girderKey(poi.GetSegmentKey());
   GET_IFACE2(pBroker,IGirderTendonGeometry,pTendonGeometry);
   DuctIndexType nDucts = pTendonGeometry->GetDuctCount(girderKey);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    true);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetSmallMomentUnit(),     true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue,    dist,       pDisplayUnits->GetComponentDimUnit(),    true);
   INIT_UV_PROTOTYPE(rptAreaUnitValue,      area,       pDisplayUnits->GetAreaUnit(),            true);
   INIT_UV_PROTOTYPE(rptLength4UnitValue,   inertia,    pDisplayUnits->GetMomentOfInertiaUnit(), true);
   INIT_UV_PROTOTYPE(rptStressUnitValue,    modE,       pDisplayUnits->GetModEUnit(),            true);
   INIT_UV_PROTOTYPE(rptPointOfInterest,    location,   pDisplayUnits->GetSpanLengthUnit(),      true);

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(girderKey));

   GET_IFACE2(pBroker,ILosses,pLosses);
   const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);

   const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   *pPara << _T("ID = ") << poi.GetID() << _T(" ") << location.SetValue(POI_ERECTED_SEGMENT,poi) << rptNewLine;
   *pPara << _T("Interval ") << LABEL_INTERVAL(intervalIdx) << _T(" : ") << pIntervals->GetDescription(intervalIdx) << rptNewLine;

   *pPara << rptNewLine;

   *pPara << _T("Composite Properties") << rptNewLine;
   *pPara << _T("Atr = ") << area.SetValue(tsDetails.Atr) << rptNewLine;
   *pPara << _T("Itr = ") << inertia.SetValue(tsDetails.Itr) << rptNewLine;
   *pPara << _T("Ytr = ") << dist.SetValue(tsDetails.Ytr) << rptNewLine;
   *pPara << _T("Ea  = ") << modE.SetValue(tsDetails.Ea) << rptNewLine;
   *pPara << rptNewLine;

   *pPara << _T("Girder Properties") << rptNewLine;
   *pPara << _T("An = ") << area.SetValue(tsDetails.Girder.An) << rptNewLine;
   *pPara << _T("In = ") << inertia.SetValue(tsDetails.Girder.In) << rptNewLine;
   *pPara << _T("Ys = ") << dist.SetValue(tsDetails.Girder.Yn) << rptNewLine;
   *pPara << _T("E  = ") << modE.SetValue(tsDetails.Girder.E) << rptNewLine;
   *pPara << _T("Ea = ") << modE.SetValue(tsDetails.Girder.Ea) << rptNewLine;
   *pPara << rptNewLine;

   *pPara << _T("Deck Properties") << rptNewLine;
   *pPara << _T("An = ") << area.SetValue(tsDetails.Deck.An) << rptNewLine;
   *pPara << _T("In = ") << inertia.SetValue(tsDetails.Deck.In) << rptNewLine;
   *pPara << _T("Ys = ") << dist.SetValue(tsDetails.Deck.Yn) << rptNewLine;
   *pPara << _T("E  = ") << modE.SetValue(tsDetails.Deck.E) << rptNewLine;
   *pPara << _T("Ea = ") << modE.SetValue(tsDetails.Deck.Ea) << rptNewLine;
   *pPara << rptNewLine;

   *pPara << _T("Deck Rebar - Top Mat - Individual Bars") << rptNewLine;
   *pPara << _T("An = ") << area.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual].As) << rptNewLine;
   *pPara << _T("Ys = ") << dist.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual].Ys) << rptNewLine;
   *pPara << _T("E  = ")  << modE.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual].E) << rptNewLine;
   *pPara << rptNewLine;

   *pPara << _T("Deck Rebar - Top Mat - Lump Sum Bars") << rptNewLine;
   *pPara << _T("An = ") << area.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbLumpSum].As) << rptNewLine;
   *pPara << _T("Ys = ") << dist.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbLumpSum].Ys) << rptNewLine;
   *pPara << _T("E  = ")  << modE.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbLumpSum].E) << rptNewLine;
   *pPara << rptNewLine;

   *pPara << _T("Deck Rebar - Bottom Mat - Individual Bars") << rptNewLine;
   *pPara << _T("An = ") << area.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].As) << rptNewLine;
   *pPara << _T("Ys = ") << dist.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].Ys) << rptNewLine;
   *pPara << _T("E  = ")  << modE.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].E) << rptNewLine;
   *pPara << rptNewLine;

   *pPara << _T("Deck Rebar - Bottom Mat - Lump Sum Bars") << rptNewLine;
   *pPara << _T("An = ") << area.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum].As) << rptNewLine;
   *pPara << _T("Ys = ") << dist.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum].Ys) << rptNewLine;
   *pPara << _T("E  = ")  << modE.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum].E) << rptNewLine;
   *pPara << rptNewLine;

   *pPara << _T("Girder Rebar") << rptNewLine;
   std::vector<TIME_STEP_REBAR>::const_iterator iter(tsDetails.GirderRebar.begin());
   std::vector<TIME_STEP_REBAR>::const_iterator end(tsDetails.GirderRebar.end());
   for ( ; iter != end; iter++ )
   {
      *pPara << _T("Rebar ") << (iter - tsDetails.GirderRebar.begin())+1 << rptNewLine;
      const TIME_STEP_REBAR& rebar(*iter);
      *pPara << _T("An = ") << area.SetValue(rebar.As) << rptNewLine;
      *pPara << _T("Ys = ") << dist.SetValue(rebar.Ys) << rptNewLine;
      *pPara << _T("E  = ")  << modE.SetValue(rebar.E) << rptNewLine;
      *pPara << rptNewLine;
   }

   *pPara << _T("Straight Strand") << rptNewLine;
   *pPara << _T("An = ") << area.SetValue(tsDetails.Strands[pgsTypes::Straight].As) << rptNewLine;
   *pPara << _T("Ys = ") << dist.SetValue(tsDetails.Strands[pgsTypes::Straight].Ys) << rptNewLine;
   *pPara << _T("E  = ")  << modE.SetValue(tsDetails.Strands[pgsTypes::Straight].E) << rptNewLine;
   *pPara << rptNewLine;

   *pPara << _T("Harped Strand") << rptNewLine;
   *pPara << _T("An = ") << area.SetValue(tsDetails.Strands[pgsTypes::Harped].As) << rptNewLine;
   *pPara << _T("Ys = ") << dist.SetValue(tsDetails.Strands[pgsTypes::Harped].Ys) << rptNewLine;
   *pPara << _T("E = ")  << modE.SetValue(tsDetails.Strands[pgsTypes::Harped].E) << rptNewLine;
   *pPara << rptNewLine;

   *pPara << _T("Temporary Strand") << rptNewLine;
   *pPara << _T("An = ") << area.SetValue(tsDetails.Strands[pgsTypes::Temporary].As) << rptNewLine;
   *pPara << _T("Ys = ") << dist.SetValue(tsDetails.Strands[pgsTypes::Temporary].Ys) << rptNewLine;
   *pPara << _T("E  = ")  << modE.SetValue(tsDetails.Strands[pgsTypes::Temporary].E) << rptNewLine;
   *pPara << rptNewLine;

   *pPara << _T("Segment Tendons") << rptNewLine;
   nDucts = tsDetails.SegmentTendons.size();
   for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
   {
      *pPara << _T("Tendon ") << LABEL_DUCT(ductIdx) << rptNewLine;
      *pPara << _T("An = ") << area.SetValue(tsDetails.SegmentTendons[ductIdx].As) << rptNewLine;
      *pPara << _T("Ys = ") << dist.SetValue(tsDetails.SegmentTendons[ductIdx].Ys) << rptNewLine;
      *pPara << _T("E  = ") << modE.SetValue(tsDetails.SegmentTendons[ductIdx].E) << rptNewLine;
      *pPara << rptNewLine;
   }

   *pPara << _T("Girder Tendons") << rptNewLine;
   nDucts = tsDetails.GirderTendons.size();
   for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
   {
      *pPara << _T("Tendon ") << LABEL_DUCT(ductIdx) << rptNewLine;
      *pPara << _T("An = ") << area.SetValue(tsDetails.GirderTendons[ductIdx].As) << rptNewLine;
      *pPara << _T("Ys = ") << dist.SetValue(tsDetails.GirderTendons[ductIdx].Ys) << rptNewLine;
      *pPara << _T("E  = ") << modE.SetValue(tsDetails.GirderTendons[ductIdx].E) << rptNewLine;
      *pPara << rptNewLine;
   }

   int N = sizeof(tsDetails.dPi)/sizeof(tsDetails.dPi[0]);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   *pPara << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Component restraining force") << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Girder.PrCreep = ") << force.SetValue(tsDetails.Girder.PrCreep) << rptNewLine;
   *pPara << _T("Girder.PrShrinkage = ") << force.SetValue(tsDetails.Girder.PrShrinkage) << rptNewLine;
   *pPara << _T("Deck.PrCreep = ") << force.SetValue(tsDetails.Deck.PrCreep) << rptNewLine;
   *pPara << _T("Deck.PrShrinkage = ") << force.SetValue(tsDetails.Deck.PrShrinkage) << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Girder.MrCreep = ") << moment.SetValue(tsDetails.Girder.MrCreep) << rptNewLine;
   *pPara << _T("Deck.MrCreep = ") << moment.SetValue(tsDetails.Deck.MrCreep) << rptNewLine;
   *pPara << rptNewLine;
   
   *pPara << _T("Pr Creep = (Girder.PrCreep + Deck.PrCreep) = (") << force.SetValue(tsDetails.Girder.PrCreep) << _T(" + ");
   *pPara << force.SetValue(tsDetails.Deck.PrCreep) << _T(") = ");
   *pPara << force.SetValue(tsDetails.Pr[TIMESTEP_CR]) << rptNewLine;
   *pPara << _T("Pr Shrinkage = (Girder.PrShrinkage + Deck.PrShrinkage) = (") << force.SetValue(tsDetails.Girder.PrShrinkage) << _T(" + ");
   *pPara << force.SetValue(tsDetails.Deck.PrShrinkage) << _T(") = ");
   *pPara << force.SetValue(tsDetails.Pr[TIMESTEP_SH]) << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Mr Creep = (Girder.MrCreep + Girder.PrCreep(Ytr - Ys) + Deck.MrCreep + Deck.PrCreep(Ytr - Ys)) = (");
   *pPara << moment.SetValue(tsDetails.Girder.MrCreep) << _T(" + ");
   *pPara << force.SetValue(tsDetails.Girder.PrCreep) << _T("(") << dist.SetValue(tsDetails.Ytr) << _T(" - ");
   *pPara << dist.SetValue(tsDetails.Girder.Yn) << _T(") + ");
   *pPara << moment.SetValue(tsDetails.Deck.MrCreep) << _T(" + ");
   *pPara << force.SetValue(tsDetails.Deck.PrCreep) << _T("(") << dist.SetValue(tsDetails.Ytr) << _T(" - ");
   *pPara << dist.SetValue(tsDetails.Deck.Yn) << _T(")) = ");
   *pPara << moment.SetValue(tsDetails.Mr[TIMESTEP_CR]) << rptNewLine;
   *pPara << _T("Mr Shrinkage = (Girder.PrShrinkage(Ytr - Ys) + Deck.PrShrinkage(Ytr - Ys)) = (");
   *pPara << force.SetValue(tsDetails.Girder.PrShrinkage) << _T("(") << dist.SetValue(tsDetails.Ytr) << _T(" - ");
   *pPara << dist.SetValue(tsDetails.Girder.Yn) << _T(") + ");
   *pPara << force.SetValue(tsDetails.Deck.PrShrinkage) << _T("(") << dist.SetValue(tsDetails.Ytr) << _T(" - ");
   *pPara << dist.SetValue(tsDetails.Deck.Yn) << _T(")) = ");
   *pPara << moment.SetValue(tsDetails.Mr[TIMESTEP_SH]) << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Pr Relaxation = (Straight.PrRelaxation + Harped.PrRelaxation + Temporary.PrRelaxation + Sum(SegmentTendon.PrRelaxation) + Sum(GirderTendon.PrRelaxation) = (");
   *pPara << force.SetValue(tsDetails.Strands[pgsTypes::Straight].PrRelaxation) << _T(" + ");
   *pPara << force.SetValue(tsDetails.Strands[pgsTypes::Harped].PrRelaxation) << _T(" + ");
   *pPara << force.SetValue(tsDetails.Strands[pgsTypes::Temporary].PrRelaxation);
   for (const auto& tsTendon : tsDetails.SegmentTendons)
   {
      *pPara << _T(" + ") << force.SetValue(tsTendon.PrRelaxation);
   }
   for (const auto& tsTendon : tsDetails.GirderTendons)
   {
      *pPara << _T(" + ") << force.SetValue(tsTendon.PrRelaxation);
   }
   *pPara << _T(") = ");
   *pPara << force.SetValue(tsDetails.Pr[TIMESTEP_RE]) << rptNewLine;
   *pPara << _T("Mr Relaxation = (Straight.PrRelaxation(Ytr - Ys) + Harped.PrRelaxation(Ytr - Ys) + Temporary.PrRelaxation(Ytr - Ys) + Sum(SegmentTendon.PrRelaxation(Ytr - Ys)) + Sum(GirderTendon.PrRelaxation(Ytr-Ys)) = ");
   *pPara << _T("(") << force.SetValue(tsDetails.Strands[pgsTypes::Straight].PrRelaxation) << _T("(") << dist.SetValue(tsDetails.Ytr) << _T(" - ");
   *pPara << dist.SetValue(tsDetails.Strands[pgsTypes::Straight].Ys) << _T(") + ");
   *pPara << force.SetValue(tsDetails.Strands[pgsTypes::Harped].PrRelaxation) << _T("(") << dist.SetValue(tsDetails.Ytr) << _T(" - ");
   *pPara << dist.SetValue(tsDetails.Strands[pgsTypes::Harped].Ys) << _T(") + ");
   *pPara << force.SetValue(tsDetails.Strands[pgsTypes::Temporary].PrRelaxation) << _T("(") << dist.SetValue(tsDetails.Ytr) << _T(" - ");
   *pPara << dist.SetValue(tsDetails.Strands[pgsTypes::Temporary].Ys) << _T(")");
   for (const auto& tsTendon : tsDetails.SegmentTendons)
   {
      *pPara << _T(" + ");
      *pPara << force.SetValue(tsTendon.PrRelaxation) << _T("(") << dist.SetValue(tsDetails.Ytr) << _T(" - ");
      *pPara << dist.SetValue(tsTendon.Ys) << _T(")");
   }
   for (const auto& tsTendon : tsDetails.GirderTendons)
   {
      *pPara << _T(" + ");
      *pPara << force.SetValue(tsTendon.PrRelaxation) << _T("(") << dist.SetValue(tsDetails.Ytr) << _T(" - ");
      *pPara << dist.SetValue(tsTendon.Ys) << _T(")");
   }
   *pPara << _T(" = ");
   *pPara << moment.SetValue(tsDetails.Mr[TIMESTEP_RE]) << rptNewLine;
   *pPara << rptNewLine;

   *pPara << rptNewLine;
   *pPara << _T("Response to restraining force") << rptNewLine;
   *pPara << rptNewLine;
   GET_IFACE2(pBroker,IProductForces,pProductForces);
   *pPara << _T("Pre Creep = ") << force.SetValue(pProductForces->GetAxial(intervalIdx,pgsTypes::pftCreep,poi,pgsTypes::ContinuousSpan,rtIncremental)) << rptNewLine;
   *pPara << _T("Pre Shrinkage = ") << force.SetValue(pProductForces->GetAxial(intervalIdx,pgsTypes::pftShrinkage,poi,pgsTypes::ContinuousSpan,rtIncremental)) << rptNewLine;
   *pPara << _T("Pre Relaxation = ") << force.SetValue(pProductForces->GetAxial(intervalIdx,pgsTypes::pftRelaxation,poi,pgsTypes::ContinuousSpan,rtIncremental)) << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Mre Creep = ") << moment.SetValue(pProductForces->GetMoment(intervalIdx,pgsTypes::pftCreep,poi,pgsTypes::ContinuousSpan,rtIncremental)) << rptNewLine;
   *pPara << _T("Mre Shrinkage = ") << moment.SetValue(pProductForces->GetMoment(intervalIdx,pgsTypes::pftShrinkage,poi,pgsTypes::ContinuousSpan,rtIncremental)) << rptNewLine;
   *pPara << _T("Mre Relaxation = ") << moment.SetValue(pProductForces->GetMoment(intervalIdx,pgsTypes::pftRelaxation,poi,pgsTypes::ContinuousSpan,rtIncremental)) << rptNewLine;

   *pPara << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Change in internal component forces") << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Axial") << rptNewLine;
   for ( int i = 0; i < N; i++ )
   {
      *pPara << rptNewLine;

      Float64 sum_dP = 0;
      Float64 dP = 0;
      pgsTypes::ProductForceType pfType = (pgsTypes::ProductForceType)i;
      *pPara << pProductLoads->GetProductLoadName(pfType) << rptNewLine;

      dP = tsDetails.Girder.dPi[pfType];
      sum_dP += dP;
      *pPara << _T("Girder dP = ") << force.SetValue(dP) << rptNewLine;

      dP = tsDetails.Deck.dPi[pfType];
      sum_dP += dP;
      *pPara << _T("Deck dP = ") << force.SetValue(dP) << rptNewLine;

      dP = tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual].dPi[pfType];
      sum_dP += dP;
      *pPara << _T("Deck Rebar Top Mat (Individual Bars) dP = ") << force.SetValue(dP) << rptNewLine;

      dP = tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbLumpSum].dPi[pfType];
      sum_dP += dP;
      *pPara << _T("Deck Rebar Top Mat (Lump Sum Bars) dP = ") << force.SetValue(dP) << rptNewLine;

      dP = tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].dPi[pfType];
      sum_dP += dP;
      *pPara << _T("Deck Rebar Bot Mat (Individual Bars) dP = ") << force.SetValue(dP) << rptNewLine;

      dP = tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum].dPi[pfType];
      sum_dP += dP;
      *pPara << _T("Deck Rebar Bot Mat (Lump Sum Bars) dP = ") << force.SetValue(dP) << rptNewLine;

      iter = tsDetails.GirderRebar.begin();
      for ( ; iter != end; iter++ )
      {
         dP = iter->dPi[pfType];
         sum_dP += dP;
         *pPara << _T("Girder Rebar ") << (iter - tsDetails.GirderRebar.begin())+1 << _T(" dP = ") << force.SetValue(dP) << rptNewLine;
      }

      dP = tsDetails.Strands[pgsTypes::Straight].dPi[pfType];
      sum_dP += dP;
      *pPara << _T("Straight Strand dP = ") << force.SetValue(dP) << rptNewLine;

      dP = tsDetails.Strands[pgsTypes::Harped].dPi[pfType];
      sum_dP += dP;
      *pPara << _T("Harped Strand dP = ") << force.SetValue(dP) << rptNewLine;

      dP = tsDetails.Strands[pgsTypes::Temporary].dPi[pfType];
      sum_dP += dP;
      *pPara << _T("Temporary Strand dP = ") << force.SetValue(dP) << rptNewLine;


      DuctIndexType ductIdx = 0;
      for (const auto& tendon : tsDetails.SegmentTendons)
      {
         dP = tendon.dPi[pfType];
         sum_dP += dP;
         *pPara << _T("Segment Duct ") << LABEL_DUCT(ductIdx) << _T(" dP = ") << force.SetValue(dP) << rptNewLine;

         ductIdx++;
      }

      ductIdx = 0;
      for(const auto& tendon : tsDetails.GirderTendons)
      {
         dP = tendon.dPi[pfType];
         sum_dP += dP;
         *pPara << _T("Girder Duct ") << LABEL_DUCT(ductIdx) << _T(" dP = ") << force.SetValue(dP) << rptNewLine;

         ductIdx++;
      }

      dP = tsDetails.dPi[pfType];
      *pPara << _T("Sum Individual dP = ") << force.SetValue(sum_dP) << rptNewLine;
      *pPara << _T("Total Internal dP = ") << force.SetValue(dP) << rptNewLine;
      *pPara << _T("Total External dP = ") << force.SetValue(pProductForces->GetAxial(intervalIdx,pfType,poi,pgsTypes::ContinuousSpan,rtIncremental)) << rptNewLine;
   }

   *pPara << rptNewLine;
   *pPara << _T("Moment") << rptNewLine;
   for ( int i = 0; i < N; i++ )
   {
      *pPara << rptNewLine;

      Float64 sum_dM = 0;
      Float64 dP = 0;
      Float64 dM = 0;
      pgsTypes::ProductForceType pfType = (pgsTypes::ProductForceType)i;
      *pPara << pProductLoads->GetProductLoadName(pfType) << rptNewLine;

      dP = tsDetails.Girder.dPi[pfType];
      dM = tsDetails.Girder.dMi[pfType] + dP*(tsDetails.Ytr - tsDetails.Girder.Yn);
      sum_dM += dM;
      *pPara << _T("Girder dM = ") << moment.SetValue(dM) << rptNewLine;

      dP = tsDetails.Deck.dPi[pfType];
      dM = tsDetails.Deck.dMi[pfType] + dP*(tsDetails.Ytr - tsDetails.Deck.Yn);
      sum_dM += dM;
      *pPara << _T("Deck dM = ") << moment.SetValue(dM) << rptNewLine;

      dP = tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual].dPi[pfType];
      dM = dP*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual].Ys);
      sum_dM += dM;
      *pPara << _T("Deck Rebar Top Mat (Individual Bars) dM = ") << moment.SetValue(dM) << rptNewLine;

      dP = tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbLumpSum].dPi[pfType];
      dM = dP*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbLumpSum].Ys);
      sum_dM += dM;
      *pPara << _T("Deck Rebar Top Mat (Lump Sum Bars) dM = ") << moment.SetValue(dM) << rptNewLine;

      dP = tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].dPi[pfType];
      dM = dP*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].Ys);
      sum_dM += dM;
      *pPara << _T("Deck Rebar Bot Mat (Individual Bars) dM = ") << moment.SetValue(dM) << rptNewLine;

      dP = tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum].dPi[pfType];
      dM = dP*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum].Ys);
      sum_dM += dM;
      *pPara << _T("Deck Rebar Bot Mat (Lump Sum Bars) dM = ") << moment.SetValue(dM) << rptNewLine;

      iter = tsDetails.GirderRebar.begin();
      for ( ; iter != end; iter++ )
      {
         dP = iter->dPi[pfType];
         dM = dP*(tsDetails.Ytr - iter->Ys);
         sum_dM += dM;
         *pPara << _T("Girder Rebar ") << (iter - tsDetails.GirderRebar.begin())+1 << _T(" dM = ") << moment.SetValue(dM) << rptNewLine;
      }

      dP = tsDetails.Strands[pgsTypes::Straight].dPi[pfType];
      dM = dP*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Straight].Ys);
      sum_dM += dM;
      *pPara << _T("Straight Strand dM = ") << moment.SetValue(dM) << rptNewLine;

      dP = tsDetails.Strands[pgsTypes::Harped].dPi[pfType];
      dM = dP*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Harped].Ys);
      sum_dM += dM;
      *pPara << _T("Harped Strand dM = ") << moment.SetValue(dM) << rptNewLine;

      dP = tsDetails.Strands[pgsTypes::Temporary].dPi[pfType];
      dM = dP*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Temporary].Ys);
      sum_dM += dM;
      *pPara << _T("Temporary Strand dM = ") << moment.SetValue(dM) << rptNewLine;

      DuctIndexType ductIdx = 0;
      for (const auto& tendon : tsDetails.SegmentTendons)
      {
         dP = tendon.dPi[pfType];
         dM = dP*(tsDetails.Ytr - tendon.Ys);
         sum_dM += dM;
         *pPara << _T("Segment Duct ") << LABEL_DUCT(ductIdx) << _T(" dM = ") << moment.SetValue(dM) << rptNewLine;
         ductIdx++;
      }

      ductIdx = 0;
      for (const auto& tendon : tsDetails.GirderTendons)
      {
         dP = tendon.dPi[pfType];
         dM = dP*(tsDetails.Ytr - tendon.Ys);
         sum_dM += dM;
         *pPara << _T("Girder Duct ") << LABEL_DUCT(ductIdx) << _T(" dM = ") << moment.SetValue(dM) << rptNewLine;
         ductIdx++;
      }


      dM = tsDetails.dMi[pfType];
      *pPara << _T("Sum Individual dM = ") << moment.SetValue(sum_dM) << rptNewLine;
      *pPara << _T("Total Internal dM = ") << moment.SetValue(dM) << rptNewLine;
      *pPara << _T("Total External dM = ") << moment.SetValue(pProductForces->GetMoment(intervalIdx,pfType,poi,pgsTypes::ContinuousSpan,rtIncremental)) << rptNewLine;
   }

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CEquilibriumCheckChapterBuilder::Clone() const
{
   return std::make_unique<CEquilibriumCheckChapterBuilder>();
}
