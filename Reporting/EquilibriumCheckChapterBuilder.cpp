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
#include <Reporting\EquilibriumCheckChapterBuilder.h>
#include <Reporting\EquilibriumCheckReportSpecification.h>
#include <IFace\PointOfInterest.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
#include <IFace\Intervals.h>

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

rptChapter* CEquilibriumCheckChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CEquilibriumCheckReportSpecification* pGdrRptSpec = dynamic_cast<CEquilibriumCheckReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGdrRptSpec->GetBroker(&pBroker);

   pgsPointOfInterest poi(pGdrRptSpec->GetPOI());
   IntervalIndexType intervalIdx = pGdrRptSpec->GetInterval();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() != pgsTypes::TIME_STEP )
   {
      *pPara << color(Red) << _T("Time Step analysis results not available.") << color(Black) << rptNewLine;
      return pChapter;
   }

   CGirderKey girderKey(poi.GetSegmentKey());
   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeometry);
   DuctIndexType nDucts = pTendonGeometry->GetDuctCount(girderKey);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    true);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetMomentUnit(),          true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue,    ecc,        pDisplayUnits->GetComponentDimUnit(),    true);

   GET_IFACE2(pBroker,ILosses,pLosses);
   const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);

   const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

   *pPara << _T("ID = ") << poi.GetID() << rptNewLine;
   *pPara << _T("Interval = ") << LABEL_INTERVAL(intervalIdx) << rptNewLine;

   *pPara << _T("Axial") << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("External Forces") << rptNewLine;

   int N = sizeof(tsDetails.dPi)/sizeof(tsDetails.dPi[0]);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   Float64 dPext = 0;
   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;

      *pPara << _T("dP (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = ") << force.SetValue(tsDetails.dPi[pfType]) << rptNewLine;

      dPext += tsDetails.dPi[pfType];
   }

   *pPara << rptNewLine;

   *pPara << _T("Total External dP = ") << force.SetValue(dPext) << rptNewLine;

   *pPara << rptNewLine;

   *pPara << _T("Internal Forces") << rptNewLine;

   *pPara << rptNewLine;

   Float64 dPint = 0;

   *pPara << _T("Girder") << rptNewLine;
   Float64 dPgirder = 0;
   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;
      
      Float64 dP = tsDetails.Girder.dPi[pfType];
      *pPara << _T("dP (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = ") << force.SetValue(dP) << rptNewLine;

      dPgirder += dP;
   }
   *pPara << _T("Total Girder dP = ") << force.SetValue(dPgirder) << rptNewLine;
   dPint += dPgirder;

   *pPara << rptNewLine;

   *pPara << _T("Deck") << rptNewLine;
   Float64 dPdeck = 0;
   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;
      Float64 dP = tsDetails.Deck.dPi[pfType];
      *pPara << _T("dP (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = ") << force.SetValue(dP) << rptNewLine;

      dPdeck += dP;
   }
   *pPara << _T("Total Deck dP = ") << force.SetValue(dPdeck) << rptNewLine;
   dPint += dPdeck;

   *pPara << rptNewLine;
   *pPara << _T("Deck Rebar - Top Mat") << rptNewLine;
   Float64 dPtopMat = 0;
   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;
      Float64 dP = tsDetails.DeckRebar[pgsTypes::drmTop].dPi[pfType];
      *pPara << _T("dP (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = ") << force.SetValue(dP) << rptNewLine;
      dPtopMat += dP;
   }
   *pPara << _T("Top Mat Deck Rebar dP = ") << force.SetValue(dPtopMat) << rptNewLine;
   dPint += dPtopMat;

   *pPara << rptNewLine;
   *pPara << _T("Deck Rebar - Bottom Mat") << rptNewLine;
   Float64 dPbotMat = 0;
   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;

      Float64 dP = tsDetails.DeckRebar[pgsTypes::drmBottom].dPi[pfType];
      *pPara << _T("dP (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = ") << force.SetValue(dP) << rptNewLine;
      dPbotMat += dP;
   }
   *pPara << _T("Bottom Mat Deck Rebar dP = ") << force.SetValue(dPbotMat) << rptNewLine;
   dPint += dPbotMat;

   *pPara << rptNewLine;

   *pPara << _T("Girder Rebar") << rptNewLine;
   std::vector<TIME_STEP_REBAR>::const_iterator iter(tsDetails.GirderRebar.begin());
   std::vector<TIME_STEP_REBAR>::const_iterator end(tsDetails.GirderRebar.end());
   for ( ; iter != end; iter++ )
   {
      *pPara << _T("Rebar Entry ") << (iter - tsDetails.GirderRebar.begin())+1 << rptNewLine;
      Float64 dPrebar = 0;
      for ( int i = 0; i < N; i++ )
      {
         ProductForceType pfType = (ProductForceType)i;
         Float64 dP = iter->dPi[pfType];
         *pPara << _T("dP (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = ") << force.SetValue(dP) << rptNewLine;
         dPrebar += dP;
      }
      *pPara << _T("Rebar Entry ") << (iter - tsDetails.GirderRebar.begin())+1 << _T(" dP = ") << force.SetValue(dPrebar) << rptNewLine;
      *pPara << rptNewLine;
      dPint += dPrebar;
   }

   *pPara << rptNewLine;

   *pPara << _T("Straight Strands") << rptNewLine;
   Float64 dPstrand = 0;
   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;
      Float64 dP = tsDetails.Strands[pgsTypes::Straight].dPi[pfType];
      *pPara << _T("dP (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = ") << force.SetValue(dP) << rptNewLine;
      dPstrand += dP;
   }
   dPint += dPstrand;
   *pPara << _T("Straight Strands Total dP = ") << force.SetValue(dPstrand) << rptNewLine;

   *pPara << rptNewLine;

   *pPara << _T("Harped Strands") << rptNewLine;
   dPstrand = 0;
   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;
      Float64 dP = tsDetails.Strands[pgsTypes::Harped].dPi[pfType];
      *pPara << _T("dP (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = ") << force.SetValue(dP) << rptNewLine;
      dPstrand += dP;
   }
   dPint += dPstrand;
   *pPara << _T("Harped Strands Total dP = ") << force.SetValue(dPstrand) << rptNewLine;

   *pPara << rptNewLine;

   *pPara << _T("Temporary Strands") << rptNewLine;
   dPstrand = 0;
   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;
      Float64 dP = tsDetails.Strands[pgsTypes::Temporary].dPi[pfType];
      *pPara << _T("dP (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = ") << force.SetValue(dP) << rptNewLine;
      dPstrand += dP;
   }
   dPint += dPstrand;
   *pPara << _T("Temporary Strands Total dP = ") << force.SetValue(dPstrand) << rptNewLine;

   *pPara << rptNewLine;
   *pPara << _T("Tendons") << rptNewLine;
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      *pPara << _T("Duct ") << LABEL_DUCT(ductIdx) << rptNewLine;

      Float64 dPtendon = 0;
      for ( int i = 0; i < N; i++ )
      {
         ProductForceType pfType = (ProductForceType)i;

         Float64 dP = tsDetails.Tendons[ductIdx].dPi[i];
         *pPara << _T("dP (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = ") << force.SetValue(dP) << rptNewLine;
         dPtendon += dP;
      }
      *pPara << _T("Duct ") << LABEL_DUCT(ductIdx) << _T(" Total dP = ") << force.SetValue(dPtendon) << rptNewLine;
      *pPara << rptNewLine;
      dPint += dPtendon;
   }

   *pPara << rptNewLine;

   *pPara << _T("Total Internal dP = ") << force.SetValue(dPint) << rptNewLine;

   *pPara << rptNewLine;

   *pPara << _T("Moment") << rptNewLine;
   *pPara << rptNewLine;

   *pPara << _T("External Forces") << rptNewLine;

   Float64 dMext = 0;

   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;

      *pPara << _T("dM (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = ") << moment.SetValue(tsDetails.dMi[pfType]) << rptNewLine;

      dMext += tsDetails.dMi[pfType];
   }

   *pPara << rptNewLine;

   *pPara << _T("Total External dM = ") << moment.SetValue(dMext) << rptNewLine;

   *pPara << rptNewLine;

   *pPara << _T("Internal Forces") << rptNewLine;

   *pPara << rptNewLine;
   *pPara << _T("Girder") << rptNewLine;

   Float64 dMint = 0;
   Float64 dMgirder = 0;
   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;

      Float64 dM = tsDetails.Girder.dMi[pfType] + tsDetails.Girder.dPi[pfType]*(tsDetails.Ytr - tsDetails.Girder.Yn);
      *pPara << _T("dM (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = dM + dP(Ytr - Yn) = ");
      *pPara << moment.SetValue(tsDetails.Girder.dMi[pfType]) << _T(" + ") << force.SetValue(tsDetails.Girder.dPi[pfType]);
      *pPara << _T("(") << ecc.SetValue(tsDetails.Ytr) << _T(" - ");
      *pPara << ecc.SetValue(tsDetails.Girder.Yn) << _T(") = ");
      *pPara << moment.SetValue(dM) << rptNewLine;

      dMgirder += dM;
   }
   *pPara << _T("Total Girder dM = ") << moment.SetValue(dMgirder) << rptNewLine;
   dMint += dMgirder;

   *pPara << rptNewLine;

   *pPara << _T("Deck") << rptNewLine;
   Float64 dMdeck = 0;
   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;
      Float64 dM = tsDetails.Deck.dMi[pfType] + tsDetails.Deck.dPi[pfType]*(tsDetails.Ytr - tsDetails.Deck.Yn);
      *pPara << _T("dM (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = dM + dP(Ytr - Yn) = ");
      *pPara << moment.SetValue(tsDetails.Deck.dMi[pfType]) << _T(" + ") << force.SetValue(tsDetails.Deck.dPi[pfType]);
      *pPara << _T("(") << ecc.SetValue(tsDetails.Ytr) << _T(" - ");
      *pPara << ecc.SetValue(tsDetails.Deck.Yn) << _T(") = ");
      *pPara << moment.SetValue(dM) << rptNewLine;
      dMdeck += dM;
   }
   *pPara << _T("Total Deck dM = ") << moment.SetValue(dMdeck) << rptNewLine;
   dMint += dMdeck;

   *pPara << rptNewLine;
   *pPara << _T("Deck Rebar - Top Mat") << rptNewLine;
   Float64 dMtopMat = 0;
   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;
      Float64 dP = tsDetails.DeckRebar[pgsTypes::drmTop].dPi[pfType];
      Float64 dM = dP*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmTop].Ys);
      *pPara << _T("dM (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = dP(Ytr - Ys) = ") << force.SetValue(dP);
      *pPara << _T("(") << ecc.SetValue(tsDetails.Ytr) << _T(" - ");
      *pPara << ecc.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop].Ys) << _T(") = ");
      *pPara << moment.SetValue(dM) << rptNewLine;
      dMtopMat += dM;
   }
   *pPara << _T("Top Mat Deck Rebar dM = ") << moment.SetValue(dMtopMat) << rptNewLine;
   dMint += dMtopMat;

   *pPara << rptNewLine;
   *pPara << _T("Deck Rebar - Bottom Mat") << rptNewLine;
   Float64 dMbotMat = 0;
   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;
      Float64 dP = tsDetails.DeckRebar[pgsTypes::drmBottom].dPi[pfType];
      Float64 dM = dP*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmBottom].Ys);
      *pPara << _T("dM (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = dP(Ytr - Ys) = ") << force.SetValue(dP);
      *pPara << _T("(") << ecc.SetValue(tsDetails.Ytr) << _T(" - ");
      *pPara << ecc.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom].Ys) << _T(") = ");
      *pPara << moment.SetValue(dM) << rptNewLine;
      dMbotMat += dM;
   }
   *pPara << _T("Bottom Mat Deck Rebar dM = ") << moment.SetValue(dMbotMat) << rptNewLine;
   dMint += dMbotMat;

   *pPara << rptNewLine;

   *pPara << _T("Girder Rebar") << rptNewLine;
   iter = tsDetails.GirderRebar.begin();
   for ( ; iter != end; iter++ )
   {
      *pPara << _T("Rebar Entry ") << (iter - tsDetails.GirderRebar.begin())+1 << rptNewLine;
      Float64 dMrebar = 0;
      for ( int i = 0; i < N; i++ )
      {
         ProductForceType pfType = (ProductForceType)i;
         Float64 dP = iter->dPi[pfType];
         Float64 dM = dP*(tsDetails.Ytr - iter->Ys);
         *pPara << _T("dM (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = dP(Ytr - Ys) = ") << force.SetValue(dP);
         *pPara << ecc.SetValue(tsDetails.Ytr) << _T(" - ");
         *pPara << ecc.SetValue(iter->Ys) << _T(") = ");
         *pPara << moment.SetValue(dM) << rptNewLine;
         dMrebar += dM;
      }
      *pPara << _T("Rebar Entry ") << (iter - tsDetails.GirderRebar.begin())+1 << _T(" dM = ") << moment.SetValue(dMrebar) << rptNewLine;
      *pPara << rptNewLine;
      dMint += dMrebar;
   }


   *pPara << rptNewLine;

   *pPara << _T("Straight Strands") << rptNewLine;
   Float64 dMstrand = 0;
   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;
      Float64 dP = tsDetails.Strands[pgsTypes::Straight].dPi[pfType];
      Float64 dM = dP*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Straight].Ys);
      *pPara << _T("dM (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = dP(Ytr - Ys) = ") << force.SetValue(dP);
      *pPara << _T("(") << ecc.SetValue(tsDetails.Ytr) << _T(" - ");
      *pPara << ecc.SetValue(tsDetails.Strands[pgsTypes::Straight].Ys) << _T(") = ");
      *pPara << moment.SetValue(dM) << rptNewLine;
      dMstrand += dM;
   }
   dMint += dMstrand;
   *pPara << _T("Straight Strands Total dM = ") << moment.SetValue(dMstrand) << rptNewLine;

   *pPara << rptNewLine;

   *pPara << _T("Harped Strands") << rptNewLine;
   dMstrand = 0;
   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;
      Float64 dP = tsDetails.Strands[pgsTypes::Harped].dPi[pfType];
      Float64 dM = dP*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Harped].Ys);
      *pPara << _T("dM (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = dP(Ytr - Ys) = ") << force.SetValue(dP);
      *pPara << _T("(") << ecc.SetValue(tsDetails.Ytr) << _T(" - ");
      *pPara << ecc.SetValue(tsDetails.Strands[pgsTypes::Harped].Ys) << _T(") = ");
      *pPara << moment.SetValue(dM) << rptNewLine;
      dMstrand += dM;
   }
   dMint += dMstrand;
   *pPara << _T("Harped Strands Total dM = ") << moment.SetValue(dMstrand) << rptNewLine;

   *pPara << rptNewLine;

   *pPara << _T("Temporary Strands") << rptNewLine;
   dMstrand = 0;
   for ( int i = 0; i < N; i++ )
   {
      ProductForceType pfType = (ProductForceType)i;
      Float64 dP = tsDetails.Strands[pgsTypes::Temporary].dPi[pfType];
      Float64 dM = dP*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Temporary].Ys);
      *pPara << _T("dM (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = dP(Ytr - Ys) = ") << force.SetValue(dP);
      *pPara << _T("(") << ecc.SetValue(tsDetails.Ytr) << _T(" - ");
      *pPara << ecc.SetValue(tsDetails.Strands[pgsTypes::Temporary].Ys) << _T(") = ");
      *pPara << moment.SetValue(dM) << rptNewLine;
      dMstrand += dM;
   }
   dMint += dMstrand;
   *pPara << _T("Temporary Strands Total dM = ") << moment.SetValue(dMstrand) << rptNewLine;

   *pPara << rptNewLine;
   *pPara << _T("Tendons") << rptNewLine;
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      *pPara << _T("Duct ") << LABEL_DUCT(ductIdx) << rptNewLine;
      Float64 dMtendon = 0;
      for ( int i = 0; i < N; i++ )
      {
         ProductForceType pfType = (ProductForceType)i;
         Float64 dM = tsDetails.Tendons[ductIdx].dPi[i]*(tsDetails.Ytr - tsDetails.Tendons[ductIdx].Ys);
         *pPara << _T("dM (") << pProductLoads->GetProductLoadName(pfType).c_str() << _T(") = dP(Ytr - Ys) = ") << force.SetValue(tsDetails.Tendons[ductIdx].dPi[i]);
         *pPara << _T("(") << ecc.SetValue(tsDetails.Ytr) << _T(" - ");
         *pPara << ecc.SetValue(tsDetails.Tendons[ductIdx].Ys) << _T(") = ") << moment.SetValue(dM) << rptNewLine;
         dMtendon += dM;
      }
      *pPara << _T("Duct ") << LABEL_DUCT(ductIdx) << _T(" Total dM = ") << moment.SetValue(dMtendon) << rptNewLine;
      *pPara << rptNewLine;
      dMint += dMtendon;
   }

   *pPara << rptNewLine;

   *pPara << _T("Total Internal dM = ") << moment.SetValue(dMint) << rptNewLine;

   *pPara << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Girder.PrCreep = ") << force.SetValue(tsDetails.Girder.PrCreep) << rptNewLine;
   *pPara << _T("Girder.PrShrinkage = ") << force.SetValue(tsDetails.Girder.PrShrinkage) << rptNewLine;
   *pPara << _T("Deck.PrCreep = ") << force.SetValue(tsDetails.Deck.PrCreep) << rptNewLine;
   *pPara << _T("Deck.PrShrinkage = ") << force.SetValue(tsDetails.Deck.PrShrinkage) << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Girder.MrCreep = ") << moment.SetValue(tsDetails.Girder.MrCreep) << rptNewLine;
   *pPara << _T("Deck.MrCreep = ") << moment.SetValue(tsDetails.Deck.MrCreep) << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Pr Creep = ") << force.SetValue(tsDetails.Pr[TIMESTEP_CR]) << rptNewLine;
   *pPara << _T("Pr Shrinkage = ") << force.SetValue(tsDetails.Pr[TIMESTEP_SH]) << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Mr Creep = ") << moment.SetValue(tsDetails.Mr[TIMESTEP_CR]) << rptNewLine;
   *pPara << _T("Mr Shrinkage = ") << moment.SetValue(tsDetails.Mr[TIMESTEP_SH]) << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Pre Creep = ") << force.SetValue(tsDetails.Pre[TIMESTEP_CR]) << rptNewLine;
   *pPara << _T("Pre Shrinkage = ") << force.SetValue(tsDetails.Pre[TIMESTEP_SH]) << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Mre Creep = ") << moment.SetValue(tsDetails.Mre[TIMESTEP_CR]) << rptNewLine;
   *pPara << _T("Mre Shrinkage = ") << moment.SetValue(tsDetails.Mre[TIMESTEP_SH]) << rptNewLine;

   *pPara << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Axial") << rptNewLine;
   for ( int i = 0; i < N; i++ )
   {
      *pPara << rptNewLine;

      Float64 sum_dP = 0;
      Float64 dP = 0;
      ProductForceType pfType = (ProductForceType)i;
      *pPara << pProductLoads->GetProductLoadName(pfType).c_str() << rptNewLine;

      dP = tsDetails.Girder.dPi[pfType];
      sum_dP += dP;
      *pPara << _T("Girder dP = ") << force.SetValue(dP) << rptNewLine;

      dP = tsDetails.Deck.dPi[pfType];
      sum_dP += dP;
      *pPara << _T("Deck dP = ") << _T("(") << dP << _T(") ") << force.SetValue(dP) << rptNewLine;

      dP = tsDetails.DeckRebar[pgsTypes::drmTop].dPi[pfType];
      sum_dP += dP;
      *pPara << _T("Deck Rebar Top Mat dP = ") << force.SetValue(dP) << rptNewLine;

      dP = tsDetails.DeckRebar[pgsTypes::drmBottom].dPi[pfType];
      sum_dP += dP;
      *pPara << _T("Deck Rebar Bot Mat dP = ") << force.SetValue(dP) << rptNewLine;

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

      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         dP = tsDetails.Tendons[ductIdx].dPi[pfType];
         sum_dP += dP;
         *pPara << _T("Duct ") << LABEL_DUCT(ductIdx) << _T(" dP = ") << force.SetValue(dP) << rptNewLine;
      }

      *pPara << _T("Total Interval dP = ") << force.SetValue(sum_dP) << rptNewLine;
      *pPara << _T("Total External dP = ") << force.SetValue(tsDetails.dPi[pfType]) << rptNewLine;
   }

   *pPara << rptNewLine;
   *pPara << _T("Moment") << rptNewLine;
   for ( int i = 0; i < N; i++ )
   {
      *pPara << rptNewLine;

      Float64 sum_dM = 0;
      Float64 dP = 0;
      Float64 dM = 0;
      ProductForceType pfType = (ProductForceType)i;
      *pPara << pProductLoads->GetProductLoadName(pfType).c_str() << rptNewLine;

      dP = tsDetails.Girder.dPi[pfType];
      dM = tsDetails.Girder.dMi[pfType] + dP*(tsDetails.Ytr - tsDetails.Girder.Yn);
      sum_dM += dM;
      *pPara << _T("Girder dM = ") << moment.SetValue(dM) << rptNewLine;

      dP = tsDetails.Deck.dPi[pfType];
      dM = tsDetails.Deck.dMi[pfType] + dP*(tsDetails.Ytr - tsDetails.Deck.Yn);
      sum_dM += dM;
      *pPara << _T("Deck dM = ") << _T("(") << dM << _T(") ") << moment.SetValue(dM) << rptNewLine;

      dP = tsDetails.DeckRebar[pgsTypes::drmTop].dPi[pfType];
      dM = dP*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmTop].Ys);
      sum_dM += dM;
      *pPara << _T("Deck Rebar Top Mat dM = ") << moment.SetValue(dM) << rptNewLine;

      dP = tsDetails.DeckRebar[pgsTypes::drmBottom].dPi[pfType];
      dM = dP*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmBottom].Ys);
      sum_dM += dM;
      *pPara << _T("Deck Rebar Bot Mat dM = ") << moment.SetValue(dM) << rptNewLine;

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

      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         dP = tsDetails.Tendons[ductIdx].dPi[pfType];
         dM = dP*(tsDetails.Ytr - tsDetails.Tendons[ductIdx].Ys);
         sum_dM += dM;
         *pPara << _T("Duct ") << LABEL_DUCT(ductIdx) << _T(" dM = ") << moment.SetValue(dM) << rptNewLine;
      }

      *pPara << _T("Total Interval dM = ") << moment.SetValue(sum_dM) << rptNewLine;
      *pPara << _T("Total External dM = ") << moment.SetValue(tsDetails.dMi[pfType]) << rptNewLine;
   }

   return pChapter;
}

CChapterBuilder* CEquilibriumCheckChapterBuilder::Clone() const
{
   return new CEquilibriumCheckChapterBuilder;
}
