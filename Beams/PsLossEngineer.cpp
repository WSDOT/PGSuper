///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

// IPsLossEngineer.cpp : Implementation of CPsLossEngineer
#include "stdafx.h"
#include "PsLossEngineer.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\StatusCenter.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Intervals.h>

#include <PGSuperException.h>

#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <psgLib/CreepCriteria.h>


#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\BridgeDescription2.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\LoadFactors.h>

#include <Materials/PsStrand.h>

#include <Reporting\ReportNotes.h>

#include "ElasticShorteningTable.h"
#include "AutogenousShrinkageTable.h"
#include "FrictionLossTable.h"
#include "PostTensionInteractionTable.h"
#include "EffectOfPostTensionedTemporaryStrandsTable.h"
#include "TimeDependentLossesAtShippingTable.h"
#include "PostTensionTimeDependentLossesAtShippingTable.h"
#include "ElasticGainDueToDeckPlacementTable.h"
#include "ElasticGainDueToSIDLTable.h"
#include "ElasticGainDueToLiveLoadTable.h"
#include "TemporaryStrandRemovalTable.h"
#include "TotalPrestressLossTable.h"
#include "ShrinkageAtHaulingTable.h"
#include "CreepAtHaulingTable.h"
#include "RelaxationAtHaulingTable.h"
#include "ShrinkageAtDeckPlacementTable.h"
#include "ChangeOfConcreteStressTable.h"
#include "TxDOT2013ChangeOfConcreteStressTable.h"
#include "CreepAtDeckPlacementTable.h"
#include "RelaxationAtDeckPlacementTable.h"
#include "TimeDependentLossesAtDeckPlacementTable.h"
#include "ShrinkageAtFinalTable.h"
#include "CreepAtFinalTable.h"
#include "RelaxationAtFinalTable.h"
#include "DeckShrinkageLossTable.h"
#include "TimeDependentLossFinalTable.h"
#include "TimeDependentLossesTable.h"

#include "CreepAndShrinkageTable.h"
#include "TxDOT2013CreepAndShrinkageTable.h"
#include "RelaxationAfterTransferTable.h"
#include "TxDOT2013RelaxationAfterTransferTable.h"
#include "TxDOT2013TimeDependentLossesTable.h"
#include "FinalPrestressLossTable.h"

#include "EffectivePrestressTable.h"
#include "EffectivePrestressForceTable.h"

#include <algorithm>

#include <psgLib/SpecificationCriteria.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// utility function so we can use the WBFL::LRFD::ApproximateLosses::BeanType enum as an array index
inline constexpr auto operator+(WBFL::LRFD::ApproximateLosses::BeamType t) noexcept { return std::underlying_type<WBFL::LRFD::ApproximateLosses::BeamType>::type(t); }

template <class T>
void ReportRow(T* pTable,rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   if ( pTable == nullptr )
   {
      return;
   }

   pTable->AddRow(pChapter,pBroker,poi,row,pDetails,pDisplayUnits,level);
}

template <class T>
void ReportRow(T* pTable, rptChapter* pChapter, IBroker* pBroker, const pgsPointOfInterest& poi, RowIndexType row, IEAFDisplayUnits* pDisplayUnits, Uint16 level)
{
   if (pTable == nullptr)
   {
      return;
   }

   pTable->AddRow(pChapter, pBroker, poi, row, pDisplayUnits, level);
}

/////////////////////////////////////////////////////////////////////////////
// CPsLossEngineer
void CPsLossEngineer::Init(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   m_scidUnknown = pStatusCenter->RegisterCallback( new pgsUnknownErrorStatusCallback() );
   m_scidGirderDescriptionError = pStatusCenter->RegisterCallback( new pgsGirderDescriptionStatusCallback(m_pBroker,eafTypes::statusError) );
   m_scidGirderDescriptionWarning = pStatusCenter->RegisterCallback( new pgsGirderDescriptionStatusCallback(m_pBroker,eafTypes::statusWarning) );
   m_scidLRFDVersionError = pStatusCenter->RegisterCallback( new pgsInformationalStatusCallback(eafTypes::statusError) );
   m_scidConcreteTypeError = pStatusCenter->RegisterCallback( new pgsInformationalStatusCallback(eafTypes::statusError) );
}

LOSSDETAILS CPsLossEngineer::ComputeLosses(BeamType beamType,const pgsPointOfInterest& poi)
{
   return ComputeLosses(beamType,poi,nullptr);
}

LOSSDETAILS CPsLossEngineer::ComputeLosses(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig)
{
   LOSSDETAILS details;

#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   ATLASSERT(pPoi->IsOnSegment(poi));
#endif

   GET_IFACE(ILossParameters,pLossParameters);
   PrestressLossCriteria::LossMethodType loss_method = pLossParameters->GetLossMethod();

   // if the girder is UHPC the loss method must be AASHTO_REFINED, WSDOT_REFINED, or GENERAL_LUMPSUM
   // and the base LRFD specification must beo 9th Edition 2020 or later
   // If it isn't, post to status center and throw and unwind exception
   GET_IFACE(IMaterials, pMaterials);
   auto concrete_type = pMaterials->GetSegmentConcreteType(poi.GetSegmentKey());
   if (IsUHPC(concrete_type))
   {
      if (
         !(loss_method == PrestressLossCriteria::LossMethodType::AASHTO_REFINED || loss_method == PrestressLossCriteria::LossMethodType::WSDOT_REFINED || loss_method == PrestressLossCriteria::LossMethodType::GENERAL_LUMPSUM)
         && 
         (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::NinthEdition2020)
         )
      {
         GET_IFACE(IEAFStatusCenter, pStatusCenter);
         std::_tstring msg(_T("The project criteria must be based on AASHTO LRFD 9th Edition 2020 or later and the prestress loss method must be set to Refined Estimate per LRFD 5.9.3.4 or Refined Estimate per WSDOT Bridge Design Manual in the Project Criteria for compatibility with PCI-UHPC Structural Design Guidance"));
         pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID, m_scidConcreteTypeError, msg.c_str());
         pStatusCenter->Add(pStatusItem);

         msg += std::_tstring(_T("\nSee Status Center for Details"));
         THROW_UNWIND(msg.c_str(), XREASON_PRESTRESS_LOSS_METHOD);
      }
   }

   switch ( loss_method )
   {
   case PrestressLossCriteria::LossMethodType::AASHTO_REFINED:
      LossesByRefinedEstimate(beamType,poi,pConfig,&details,laAASHTO);
      break;

   case PrestressLossCriteria::LossMethodType::WSDOT_REFINED:
      LossesByRefinedEstimate(beamType,poi, pConfig,&details,laWSDOT);
      break;

   case PrestressLossCriteria::LossMethodType::TXDOT_REFINED_2004:
      LossesByRefinedEstimate(beamType,poi, pConfig,&details,laTxDOT);
      break;

   case PrestressLossCriteria::LossMethodType::TXDOT_REFINED_2013:
      LossesByRefinedEstimateTxDOT2013(beamType,poi, pConfig,&details);
      break;

   case PrestressLossCriteria::LossMethodType::AASHTO_LUMPSUM:
      LossesByApproxLumpSum(beamType,poi, pConfig,&details,false);
      break;

   case PrestressLossCriteria::LossMethodType::WSDOT_LUMPSUM:
      LossesByApproxLumpSum(beamType,poi, pConfig,&details,true);
      break;

   case PrestressLossCriteria::LossMethodType::GENERAL_LUMPSUM:
      LossesByGeneralLumpSum(beamType,poi, pConfig,&details);
      break;

   default:
      ATLASSERT(false); // Should never get here
   }

   return details;
}

LOSSDETAILS CPsLossEngineer::ComputeLossesForDesign(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   LOSSDETAILS details;

   m_bComputingLossesForDesign = true;
   details = ComputeLosses(beamType,poi, &config);
   m_bComputingLossesForDesign = false;

   return details;
}

void CPsLossEngineer::BuildReport(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE(ILossParameters,pLossParameters);
   PrestressLossCriteria::LossMethodType loss_method = pLossParameters->GetLossMethod();

   Uint16 level = 0;
   switch( loss_method )
   {
   case PrestressLossCriteria::LossMethodType::AASHTO_REFINED:
      ReportRefinedMethod(beamType,girderKey,pChapter,pDisplayUnits,level,laAASHTO);
      break;

   case PrestressLossCriteria::LossMethodType::WSDOT_REFINED:
      ReportRefinedMethod(beamType,girderKey,pChapter,pDisplayUnits,level,laWSDOT);
      break;

   case PrestressLossCriteria::LossMethodType::TXDOT_REFINED_2004:
      ReportRefinedMethod(beamType,girderKey,pChapter,pDisplayUnits,level,laTxDOT);
      break;

   case PrestressLossCriteria::LossMethodType::AASHTO_LUMPSUM:
      ReportApproxLumpSumMethod(beamType,girderKey,pChapter,pDisplayUnits,level,false);
      break;

   case PrestressLossCriteria::LossMethodType::WSDOT_LUMPSUM:
      ReportApproxLumpSumMethod(beamType,girderKey,pChapter,pDisplayUnits,level,true);
      break;

   case PrestressLossCriteria::LossMethodType::GENERAL_LUMPSUM:
      ReportGeneralLumpSumMethod(beamType,girderKey,pChapter,pDisplayUnits,true,level);
      break;

   case PrestressLossCriteria::LossMethodType::TXDOT_REFINED_2013:
      ReportRefinedMethodTxDOT2013(pChapter,beamType,girderKey,pDisplayUnits,level);
      break;

   default:
      ATLASSERT(false); // Should never get here
   }
}

void CPsLossEngineer::ReportRefinedMethod(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,Uint16 level,LossAgency lossAgency)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if ( pSpecEntry->GetSpecificationCriteria().GetEdition() <= WBFL::LRFD::BDSManager::Edition::ThirdEdition2004 ||
        lossAgency==laTxDOT)
   {
      ReportRefinedMethodBefore2005(pChapter,beamType,girderKey,pDisplayUnits,level);
   }
   else
   {
      ReportRefinedMethod2005(pChapter,beamType,girderKey,pDisplayUnits,level);
   }
}

void CPsLossEngineer::ReportApproxLumpSumMethod(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,Uint16 level,bool isWsdot)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if ( pSpecEntry->GetSpecificationCriteria().GetEdition() <= WBFL::LRFD::BDSManager::Edition::ThirdEdition2004 )
   {
      ReportApproxMethod(pChapter,beamType,girderKey,pDisplayUnits,level,isWsdot);
   }
   else
   {
      ReportApproxMethod2005(pChapter,beamType,girderKey,pDisplayUnits,level);
   }
}

void CPsLossEngineer::ReportGeneralLumpSumMethod(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,bool bDesign,Uint16 level)
{
   ReportLumpSumMethod(pChapter,beamType,girderKey,pDisplayUnits,bDesign,level);

#if defined _DEBUG
   // this method is only applicable to PGSuper
   GET_IFACE(IBridge, pIBridge);
   ATLASSERT(pIBridge->GetSegmentCount(girderKey) == 1);
#endif
   CSegmentKey segmentKey(girderKey, 0);

   PoiList vPoi;
   GetPointsOfInterest(girderKey, &vPoi);
   CEffectivePrestressForceTable* pP = CEffectivePrestressForceTable::PrepareTable(pChapter, m_pBroker, segmentKey, pDisplayUnits, level);

   pgsPointOfInterest prev_poi(CSegmentKey(0, 0, 0), 0.0);
   RowIndexType row = 1;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      ReportLocation(pP, row, poi, pDisplayUnits);
      ReportRow(pP, pChapter, m_pBroker, poi, row, pDisplayUnits, level);
      row++;
      prev_poi = poi;
   }
}

void CPsLossEngineer::LossesByRefinedEstimate(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG*pConfig,LOSSDETAILS* pLosses,LossAgency lossAgency)
{
   PRECONDITION(pLosses != 0 );

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if ( pSpecEntry->GetSpecificationCriteria().GetEdition() <= WBFL::LRFD::BDSManager::Edition::ThirdEdition2004 ||
        lossAgency==laTxDOT)
   {
      LossesByRefinedEstimateBefore2005(beamType,poi,pConfig,pLosses);
   }
   else
   {
      LossesByRefinedEstimate2005(beamType,poi, pConfig,pLosses,lossAgency);
   }
}

void CPsLossEngineer::LossesByRefinedEstimateBefore2005(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,LOSSDETAILS* pLosses)
{
   pLosses->LossMethod = PrestressLossCriteria::LossMethodType::AASHTO_REFINED;

   WBFL::Materials::PsStrand::Grade gradePerm, gradeTemp;
   WBFL::Materials::PsStrand::Type typePerm, typeTemp;
  WBFL::Materials::PsStrand::Coating coatingPerm, coatingTemp;
   WBFL::LRFD::Losses::SectionPropertiesType spType;
   Float64 fpjPerm;
   Float64 fpjTTS;
   Float64 perimeter;
   Float64 Ag;
   Float64 Ixx,Iyy,Ixy;
   Float64 Ybg;
   Float64 Ac1;
   Float64 Ic1;
   Float64 Ybc1;
   Float64 Ac2;
   Float64 Ic2;
   Float64 Ybc2;
   Float64 An;
   Float64 Ixxn,Iyyn,Ixyn;
   Float64 Ybn;
   Float64 Acn;
   Float64 Icn;
   Float64 Ybcn;
   Float64 Ad;
   Float64 ed;
   Float64 Ksh;
   WBFL::Geometry::Point2d epermRelease;// eccentricity of the permanent strands on the non-composite section
   WBFL::Geometry::Point2d epermFinal;
   WBFL::Geometry::Point2d etemp;
   Float64 aps;  // area of one prestress strand
   Float64 ApsPerm;
   Float64 ApsTTS;
   Float64 Mdlg;
   std::vector<std::pair<Float64, Float64>> Madlg;
   std::vector<std::pair<Float64, Float64>> Msidl1;
   std::vector<std::pair<Float64, Float64>> Msidl2;

   Float64 rh;
   Float64 ti,th,td,tf; // initial time, time of hauling,time of deck placment, final time

   Float64 PjS, PjH, PjT;

   Float64 GdrShrinkageK1, GdrShrinkageK2;
   Float64 DeckShrinkageK1, DeckShrinkageK2;

   Float64 fci,fc,fcSlab;
   Float64 Eci,Ec,EcSlab;

   Float64 girder_length, span_length;

   WBFL::LRFD::Losses::TempStrandUsage usage;

   Float64 anchorSet,wobble,coeffFriction,angleChange;

   GetLossParameters(poi, pConfig,
                     &spType,
                     &gradePerm, &typePerm, &coatingPerm, &gradeTemp, &typeTemp, &coatingTemp, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ixx, &Iyy, &Ixy, &Ybg, &Ac1, &Ic1, &Ybc1, &Ac2, &Ic2, &Ybc2, &An, &Ixxn, &Iyyn, &Ixyn, &Ybn, &Acn, &Icn, &Ybcn, &Ad, &ed, &Ksh,
                     &epermRelease, &epermFinal, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl1, &Msidl2, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &GdrShrinkageK1, &GdrShrinkageK2,
                     &DeckShrinkageK1, &DeckShrinkageK2,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,
                     &usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();


   // get time to prestress transfer
   GET_IFACE( ISpecification,   pSpec);
   GET_IFACE(ILibrary,pLib);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   const auto& prestress_loss_criteria = pSpecEntry->GetPrestressLossCriteria();

   Float64 shipping_loss = prestress_loss_criteria.ShippingLosses;

   std::shared_ptr<WBFL::LRFD::RefinedLosses> pLoss(new WBFL::LRFD::RefinedLosses(poi.GetDistFromStart(),
                                girder_length, spType,
                                gradePerm,
                                typePerm,
                                coatingPerm,
                                gradeTemp,
                                typeTemp,
                                coatingTemp,
                                fpjPerm,
                                fpjTTS,
                                ApsPerm,
                                ApsTTS,
                                aps,
                                epermRelease,
                                epermFinal,
                                etemp,
                                usage,
                                anchorSet,
                                wobble,
                                coeffFriction,
                                angleChange,
                                fc,
                                fci,
                                fcSlab,
                                Ec,
                                Eci,
                                EcSlab,

                                Mdlg,
                                Madlg,
                                Msidl1,
                                Msidl2,

                                Ag,
                                Ixx, Iyy, Ixy,
                                Ybg,
                                Ac1,
                                Ic1,
                                Ybc1,
                                Ac2, Ic2, Ybc2,

                                An,
                                Ixxn, Iyyn, Ixyn,
                                Ybn,
                                Acn,
                                Icn,
                                Ybcn,

                                rh,
                                ti,
                                shipping_loss,
                                false
                                ));


   // Any of the "get" methods on WBFL::LRFD::PsLosses can throw an WBFL::LRFD::XPsLosses::Reason exception if
   // the input data is bad.  To make sure we have everything correct, lets request
   // the elastic shortening losses and make sure an exception doesn't throw.
   try
   {
      Float64 pES = pLoss->PermanentStrand_ElasticShorteningLosses();
      // store in shared pointer to base class
      pLosses->pLosses = std::static_pointer_cast<const WBFL::LRFD::Losses>(pLoss);
      ATLASSERT(pLosses->pLosses!=nullptr);
   }
   catch( const WBFL::LRFD::XPsLosses& e )
   {
      Int32 reason = XREASON_AGENTVALIDATIONFAILURE;
      std::_tstring msg = std::_tstring(SEGMENT_LABEL(segmentKey)) + _T(": ");

      CEAFStatusItem* pStatusItem = nullptr;

      if ( e.GetReasonCode() == WBFL::LRFD::XPsLosses::Reason::fpjOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims )
         {
            msg += _T("Prestress losses could not be computed because the prestress jacking stress fpj does not exceed 0.5fpu (see Article 5.9.5.4.4b)\nAdjust the prestress jacking forces");
         }
         else
         {
            msg += _T("Prestress losses could not be computed because the prestress jacking stress fpj does not exceed 0.5fpu (see Article ") + std::_tstring(WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.2c"),_T("5.9.3.4.2c"))) + _T("\nAdjust the prestress jacking forces");
         }
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,1,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());
      }
      else if ( e.GetReasonCode() == WBFL::LRFD::XPsLosses::Reason::fcOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg += _T("Concrete strength is out of range per LRFD 5.4.2.1 and ") +  std::_tstring(WBFL::LRFD::LrfdCw8th(_T("5.9.5.1"),_T("5.9.3.1")));
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,2,m_StatusGroupID,m_scidGirderDescriptionWarning,msg.c_str());
      }
      else
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg += _T("Prestress losses could not be computed because an unspecified error occured");
         pStatusItem = new pgsUnknownErrorStatusItem(m_StatusGroupID,m_scidUnknown,_T(__FILE__),__LINE__,msg.c_str());
      }

      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      ATLASSERT(pStatusItem != nullptr);
      pStatusCenter->Add(pStatusItem);

      msg += std::_tstring(_T("\nSee Status Center for Details"));
      THROW_UNWIND(msg.c_str(),reason);
   }
}

void CPsLossEngineer::LossesByRefinedEstimate2005(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,LOSSDETAILS* pLosses,LossAgency lossAgency)
{
   assert(lossAgency!=laTxDOT); // Did TxDOT change their mind about using the 05 revisions?

   WBFL::LRFD::Losses::SectionPropertiesType spType;
   WBFL::Materials::PsStrand::Grade gradePerm, gradeTemp;
   WBFL::Materials::PsStrand::Type typePerm, typeTemp;
  WBFL::Materials::PsStrand::Coating coatingPerm, coatingTemp;
   Float64 fpjPerm;
   Float64 fpjTTS;
   Float64 perimeter;
   Float64 Ag;
   Float64 Ixx, Iyy, Ixy;
   Float64 Ybg;
   Float64 Ac1;
   Float64 Ic1;
   Float64 Ybc1;
   Float64 Ac2;
   Float64 Ic2;
   Float64 Ybc2;
   Float64 An;
   Float64 Ixxn, Iyyn, Ixyn;
   Float64 Ybn;
   Float64 Acn;
   Float64 Icn;
   Float64 Ybcn;
   Float64 Ad;
   Float64 ed;
   Float64 Ksh;
   WBFL::Geometry::Point2d epermRelease;// eccentricity of the permanent strands on the non-composite section
   WBFL::Geometry::Point2d epermFinal;
   WBFL::Geometry::Point2d etemp;
   Float64 aps;  // area of one prestress strand
   Float64 ApsPerm;
   Float64 ApsTTS;
   Float64 Mdlg;
   std::vector<std::pair<Float64, Float64>> Madlg;
   std::vector<std::pair<Float64, Float64>> Msidl1;
   std::vector<std::pair<Float64, Float64>> Msidl2;
   Float64 rh;
   Float64 ti,th,td,tf; // initial time, time of hauling,time of deck placment, final time

   Float64 PjS, PjH, PjT;

   Float64 GdrShrinkageK1, GdrShrinkageK2;
   Float64 DeckShrinkageK1, DeckShrinkageK2;

   Float64 fci,fc,fcSlab;
   Float64 Eci,Ec,EcSlab;

   Float64 girder_length, span_length;

   WBFL::LRFD::Losses::TempStrandUsage usage;

   Float64 anchorSet,wobble,coeffFriction,angleChange;

   GetLossParameters(poi, pConfig,&spType,
                     &gradePerm, &typePerm, &coatingPerm, &gradeTemp, &typeTemp, &coatingTemp, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ixx, &Iyy, &Ixy, &Ybg, &Ac1, &Ic1, &Ybc1, &Ac2, &Ic2, &Ybc2, &An, &Ixxn, &Iyyn, &Ixyn, &Ybn, &Acn, &Icn, &Ybcn, &Ad, &ed, &Ksh,
                     &epermRelease, &epermFinal, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl1, &Msidl2, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &GdrShrinkageK1, &GdrShrinkageK2,
                     &DeckShrinkageK1, &DeckShrinkageK2,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,
                     &usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();


   // get time to prestress transfer
   GET_IFACE( ISpecification,   pSpec);
   GET_IFACE( ILibrary,         pLib);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   const auto& prestress_loss_criteria = pSpecEntry->GetPrestressLossCriteria();
   if ( lossAgency==laWSDOT )
   {
      pLosses->LossMethod = PrestressLossCriteria::LossMethodType::WSDOT_REFINED_2005;
   }
   else
   {
      pLosses->LossMethod = PrestressLossCriteria::LossMethodType::AASHTO_REFINED_2005;
   }

   WBFL::LRFD::RefinedLosses2005::RelaxationLossMethod relaxationMethod = prestress_loss_criteria.RelaxationLossMethod;

   std::shared_ptr<WBFL::LRFD::RefinedLosses2005> pLoss;

   GET_IFACE(ICamber, pCamber);
   std::shared_ptr<const WBFL::LRFD::CreepCoefficient> pGirderCreep = pCamber->GetGirderCreepModel(segmentKey, pConfig);
   std::shared_ptr<const WBFL::LRFD::CreepCoefficient2005> pDeckCreep = pCamber->GetDeckCreepModel(0);

   GET_IFACE(IMaterials, pMaterials);
   auto concrete_type = pMaterials->GetSegmentConcreteType(segmentKey);
   if (concrete_type == pgsTypes::PCI_UHPC)
   {
      GET_IFACE(ISegmentData, pSegment);
      bool bPCTTGirder = pSegment->GetSegmentMaterial(segmentKey)->Concrete.bPCTT;
      Float64 GdrAutogenousShrinkage = pMaterials->GetSegmentAutogenousShrinkage(segmentKey);

      pLoss = std::make_shared<WBFL::LRFD::PCIUHPCLosses>(poi.GetDistFromStart(),
         girder_length,
         spType,
         gradePerm,
         typePerm,
         coatingPerm,
         gradeTemp,
         typeTemp,
         coatingTemp,
         fpjPerm,
         fpjTTS,
         ApsPerm,
         ApsTTS,
         aps,
         epermRelease,
         epermFinal,
         etemp,
         usage,
         anchorSet,
         wobble,
         coeffFriction,
         angleChange,
         GdrShrinkageK1, GdrShrinkageK2, GdrAutogenousShrinkage,
         DeckShrinkageK1, DeckShrinkageK2, 
         fc,
         fci,
         fcSlab,
         Ec,
         Eci,
         EcSlab,
         Ag,
         Ixx, Iyy, Ixy,
         Ybg,
         Ac1,
         Ic1,
         Ybc1,
         Ac2, Ic2, Ybc2,
         An,
         Ixxn, Iyyn, Ixyn,
         Ybn,
         Acn,
         Icn,
         Ybcn,
         Ad,
         ed,
         Ksh,
         Mdlg,
         Madlg,
         Msidl1, Msidl2,
         rh,
         ti,
         th,
         td,
         tf,
         lossAgency != laWSDOT, // ignore initial relaxation if not WSDOT
         false,
         relaxationMethod,
         std::dynamic_pointer_cast<const WBFL::LRFD::CreepCoefficient2005>(pGirderCreep),pDeckCreep,
         true, bPCTTGirder);
   }
   else if (concrete_type == pgsTypes::UHPC)
   {
   pLoss = std::make_shared<WBFL::LRFD::UHPCLosses>(poi.GetDistFromStart(),
      girder_length,
      spType,
      gradePerm,
      typePerm,
      coatingPerm,
      gradeTemp,
      typeTemp,
      coatingTemp,
      fpjPerm,
      fpjTTS,
      ApsPerm,
      ApsTTS,
      aps,
      epermRelease,
      epermFinal,
      etemp,
      usage,
      anchorSet,
      wobble,
      coeffFriction,
      angleChange,
      GdrShrinkageK1, GdrShrinkageK2,
      DeckShrinkageK1, DeckShrinkageK2,
      fc,
      fci,
      fcSlab,
      Ec,
      Eci,
      EcSlab,
      Ag,
      Ixx, Iyy, Ixy,
      Ybg,
      Ac1,
      Ic1,
      Ybc1,
      Ac2, Ic2, Ybc2,
      An,
      Ixxn, Iyyn, Ixyn,
      Ybn,
      Acn,
      Icn,
      Ybcn,
      Ad,
      ed,
      Ksh,
      Mdlg,
      Madlg,
      Msidl1, Msidl2,
      rh,
      ti,
      th,
      td,
      tf,
      lossAgency != laWSDOT, // ignore initial relaxation if not WSDOT
      false,
      relaxationMethod,
      std::dynamic_pointer_cast<const WBFL::LRFD::CreepCoefficient2005>(pGirderCreep), pDeckCreep);
   }
   else
   {
      pLoss = std::make_shared<WBFL::LRFD::RefinedLosses2005>(poi.GetDistFromStart(),
         girder_length,
         spType,
         gradePerm,
         typePerm,
         coatingPerm,
         gradeTemp,
         typeTemp,
         coatingTemp,
         fpjPerm,
         fpjTTS,
         ApsPerm,
         ApsTTS,
         aps,
         epermRelease,
         epermFinal,
         etemp,
         usage,
         anchorSet,
         wobble,
         coeffFriction,
         angleChange,
         GdrShrinkageK1, GdrShrinkageK2,
         DeckShrinkageK1, DeckShrinkageK2,
         fc,
         fci,
         fcSlab,
         Ec,
         Eci,
         EcSlab,
         Ag,
         Ixx, Iyy, Ixy,
         Ybg,
         Ac1,
         Ic1,
         Ybc1,
         Ac2, Ic2, Ybc2,
         An,
         Ixxn, Iyyn, Ixyn,
         Ybn,
         Acn,
         Icn,
         Ybcn,
         Ad,
         ed,
         Ksh,
         Mdlg,
         Madlg,
         Msidl1, Msidl2,
         rh,
         ti,
         th,
         td,
         tf,
         lossAgency != laWSDOT, // ignore initial relaxation if not WSDOT
         false,
         relaxationMethod,
          std::dynamic_pointer_cast<const WBFL::LRFD::CreepCoefficient2005>(pGirderCreep), pDeckCreep
          );
   }


   // Any of the _T("get") methods on WBFL::LRFD::PsLosses can throw an WBFL::LRFD::XPsLosses::Reason exception if
   // the input data is bad.  To make sure we have everything correct, lets request
   // the elastic shortening losses and make sure an exception doesn't throw.
   try
   {
      Float64 pES = pLoss->PermanentStrand_ElasticShorteningLosses();
      // store in shared pointer of base class
      pLosses->pLosses = std::static_pointer_cast<const WBFL::LRFD::Losses>(pLoss);
      ATLASSERT(pLosses->pLosses!=nullptr);
   }
   catch( const WBFL::LRFD::XPsLosses& e )
   {
      Int32 reason = XREASON_AGENTVALIDATIONFAILURE;
      std::_tstring msg = std::_tstring(SEGMENT_LABEL(segmentKey)) + _T(". ");

      CEAFStatusItem* pStatusItem = nullptr;

      if ( e.GetReasonCode() == WBFL::LRFD::XPsLosses::Reason::fpjOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims )
         {
            msg += _T("Prestress losses could not be computed because the prestress jacking stress fpj does not exceed 0.5fpu (see Article 5.9.5.4.4b)\nAdjust the prestress jacking forces");
         }
         else
         {
            msg += _T("Prestress losses could not be computed because the prestress jacking stress fpj does not exceed 0.5fpu (see Article") + std::_tstring(WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.2c"),_T("5.9.3.4.2c"))) + _T("\nAdjust the prestress jacking forces");
         }
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,1,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());
      }
      else if ( e.GetReasonCode() == WBFL::LRFD::XPsLosses::Reason::StrandType )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg += _T("The relaxation loss of 1.2 ksi can only be used with low relaxation strands (see Article ") + std::_tstring(WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.2c"),_T("5.9.3.4.2c"))) +_T("\nChange the strand type or select a different method for computing losses");
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,1,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());
      }
      else if ( e.GetReasonCode() == WBFL::LRFD::XPsLosses::Reason::fcOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg += _T("Concrete strength is out of range per LRFD 5.4.2.1 and ") + std::_tstring(WBFL::LRFD::LrfdCw8th(_T("5.9.5.1"),_T("5.9.3.1")));
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,2,m_StatusGroupID,m_scidGirderDescriptionWarning,msg.c_str());
      }
      else
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg += _T("Prestress losses could not be computed because an unspecified error occured");
         pStatusItem = new pgsUnknownErrorStatusItem(m_StatusGroupID,m_scidUnknown,_T(__FILE__),__LINE__,msg.c_str());
      }

      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      ATLASSERT(pStatusItem != nullptr);
      pStatusCenter->Add(pStatusItem);

      msg += std::_tstring(_T("\nSee Status Center for Details"));
      THROW_UNWIND(msg.c_str(),reason);
   }
}

void CPsLossEngineer::LossesByRefinedEstimateTxDOT2013(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,LOSSDETAILS* pLosses)
{
   // Compute details - This is a bit tricky: We practically need to compute losses in order to determine which method to use
   //                   for elastic shortening. So might as well save on code and compute them - then figure out if we can cache
   //                   This may be first time through, so we'll check on the back side and; if we are using the 
   //                   simplified method, we need to recompute at mid-girder
   WBFL::LRFD::ElasticShortening::FcgpComputationMethod method = LossesByRefinedEstimateTxDOT2013_Compute(beamType,poi, pConfig,pLosses);

   if(method == WBFL::LRFD::ElasticShortening::FcgpComputationMethod::AssumedFpe)
   {
      // Elastic shortening uses the 0.7Fpu method. We only need to compute at mid-girder and then cache results for other locations
      GET_IFACE( IPointOfInterest, pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(poi.GetSegmentKey(), POI_5L | POI_RELEASED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 1);
      const pgsPointOfInterest& midpoi = vPoi.front();

      WBFL::LRFD::ElasticShortening::FcgpComputationMethod newmethod = LossesByRefinedEstimateTxDOT2013_Compute(beamType, midpoi, pConfig, pLosses);
      ATLASSERT(newmethod == WBFL::LRFD::ElasticShortening::FcgpComputationMethod::AssumedFpe);
   }
}

WBFL::LRFD::ElasticShortening::FcgpComputationMethod CPsLossEngineer::LossesByRefinedEstimateTxDOT2013_Compute(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,LOSSDETAILS* pLosses)
{
   pLosses->LossMethod = PrestressLossCriteria::LossMethodType::TXDOT_REFINED_2013;

   WBFL::LRFD::Losses::SectionPropertiesType spType;
   WBFL::Materials::PsStrand::Grade gradePerm, gradeTemp;
   WBFL::Materials::PsStrand::Type typePerm, typeTemp;
  WBFL::Materials::PsStrand::Coating coatingPerm, coatingTemp;
   Float64 fpjPerm;
   Float64 fpjTTS;
   Float64 perimeter;
   Float64 Ag;
   Float64 Ixx, Iyy, Ixy;
   Float64 Ybg;
   Float64 Ac1;
   Float64 Ic1;
   Float64 Ybc1;
   Float64 Ac2;
   Float64 Ic2;
   Float64 Ybc2;
   Float64 An;
   Float64 Ixxn, Iyyn, Ixyn;
   Float64 Ybn;
   Float64 Acn;
   Float64 Icn;
   Float64 Ybcn;
   Float64 Ad;
   Float64 ed;
   Float64 Ksh;
   WBFL::Geometry::Point2d epermRelease;// eccentricity of the permanent strands on the non-composite section
   WBFL::Geometry::Point2d epermFinal;
   WBFL::Geometry::Point2d etemp;
   Float64 aps;  // area of one prestress strand
   Float64 ApsPerm;
   Float64 ApsTTS;
   Float64 Mdlg;
   std::vector<std::pair<Float64, Float64>> Madlg;
   std::vector<std::pair<Float64, Float64>> Msidl1;
   std::vector<std::pair<Float64, Float64>> Msidl2;

   Float64 rh;
   Float64 ti,th,td,tf; // initial time, time of hauling,time of deck placment, final time

   Float64 PjS, PjH, PjT;

   Float64 GdrShrinkageK1, GdrShrinkageK2;
   Float64 DeckShrinkageK1, DeckShrinkageK2;

   Float64 fci,fc,fcSlab;
   Float64 Eci,Ec,EcSlab;

   Float64 girder_length, span_length;

   WBFL::LRFD::Losses::TempStrandUsage usage;

   Float64 anchorSet,wobble,coeffFriction,angleChange;

   GetLossParameters(poi, pConfig,&spType,
                     &gradePerm, &typePerm, &coatingPerm, &gradeTemp, &typeTemp, &coatingTemp, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ixx, &Iyy, &Ixy, &Ybg, &Ac1, &Ic1, &Ybc1, &Ac2, &Ic2, &Ybc2, &An, &Ixxn, &Iyyn, &Ixyn, &Ybn, &Acn, &Icn, &Ybcn, &Ad, &ed, &Ksh,
                     &epermRelease, &epermFinal, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl1, &Msidl2, &rh,
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &GdrShrinkageK1, &GdrShrinkageK2,
                     &DeckShrinkageK1, &DeckShrinkageK2,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,
                     &usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE( ISpecification,   pSpec);
   GET_IFACE(ILibrary,pLib);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   const auto& prestress_loss_criteria = pSpecEntry->GetPrestressLossCriteria();
   Float64 shipping_loss = prestress_loss_criteria.ShippingLosses;

   // fcgp Computation method - Elastic shortening
   auto fcgp_method = prestress_loss_criteria.FcgpComputationMethod;

   WBFL::LRFD::ElasticShortening::FcgpComputationMethod method;
   if (fcgp_method == PrestressLossCriteria::FcgpMethodType::Assume07fpu)
   {
      method = WBFL::LRFD::ElasticShortening::FcgpComputationMethod::AssumedFpe;
   }
   else if (fcgp_method == PrestressLossCriteria::FcgpMethodType::Iterative)
   {
      method = WBFL::LRFD::ElasticShortening::FcgpComputationMethod::Iterative;
   }
   else
   {
      ATLASSERT(fcgp_method == PrestressLossCriteria::FcgpMethodType::Hybrid);

      // Use 0.7Fpu method to compute Fcgp if: permanent strands are jacked to 0.75*Fpu and,
      // no temp strands exist and, beam is prismatic, and no debonding exists.
      // Otherwise, use iterative method
      method = WBFL::LRFD::ElasticShortening::FcgpComputationMethod::Iterative;
      if ( 0.0 <= ApsPerm && IsEqual(ApsTTS, 0.0) )
      {
         Float64 Fpu = WBFL::LRFD::PsStrand::GetUltimateStrength( gradePerm );

         if (ApsPerm==0.0 || IsEqual(Fpu*0.75, fpjPerm, 1000.0)) // Pa's are very small
         {
            GET_IFACE(IGirder,pGirder);
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
            if ( pGirder->IsPrismatic(releaseIntervalIdx,segmentKey) )
            {
               if (pConfig)
               {
                  if (pConfig->PrestressConfig.Debond[pgsTypes::Straight].empty() && pConfig->PrestressConfig.Debond[pgsTypes::Harped].empty())
                  {
                     method = WBFL::LRFD::ElasticShortening::FcgpComputationMethod::AssumedFpe;
                  }
               }
               else
               {
                  GET_IFACE(IStrandGeometry, pStrandGeom);
                  if (!pStrandGeom->HasDebonding(segmentKey))
                  {
                     method = WBFL::LRFD::ElasticShortening::FcgpComputationMethod::AssumedFpe;
                  }
               }
            }
         }
      }
   }

   std::shared_ptr<WBFL::LRFD::RefinedLossesTxDOT2013> pLoss(std::make_shared<WBFL::LRFD::RefinedLossesTxDOT2013>( poi.GetDistFromStart(),
                                                  girder_length,
                                                  spType,
                                                  gradePerm,
                                                  typePerm,
                                                  coatingPerm,
                                                  gradeTemp,
                                                  typeTemp,
                                                  coatingTemp,
                                                  fpjPerm,
                                                  fpjTTS,
                                                  ApsPerm,
                                                  ApsTTS,
                                                  aps,
                                                  epermRelease,
                                                  epermFinal,
                                                  etemp,
                                                  usage,
                                                  anchorSet,
                                                  wobble,
                                                  coeffFriction,
                                                  angleChange,
                                                  fc,
                                                  fci,
                                                  fcSlab,
                                                  Ec,
                                                  Eci,
                                                  EcSlab,
                                                  Mdlg,
                                                  Madlg,
                                                  Msidl1, Msidl2,
                                                  Ag,
                                                  Ixx, Iyy, Ixy,
                                                  Ybg,
                                                  Ac1,
                                                  Ic1,
                                                  Ybc1,
                                                  Ac2, Ic2, Ybc2,
                                                  An,
                                                  Ixxn, Iyyn, Ixyn,
                                                  Ybn,
                                                  Acn,
                                                  Icn,
                                                  Ybcn,
                                                  rh,
                                                  ti,
                                                  shipping_loss,
                                                  method,
                                                  false));

   // Any of the "get" methods can throw an WBFL::LRFD::XPsLosses::Reason exception if
   // the input data is bad.  To make sure we have everything correct, lets request
   // the elastic shortening losses and make sure an exception doesn't throw.
   try
   {
      Float64 pES = pLoss->PermanentStrand_ElasticShorteningLosses();
      // store in shared pointer to base class
      pLosses->pLosses = std::static_pointer_cast<const WBFL::LRFD::Losses>(pLoss);
      ATLASSERT(pLosses->pLosses!=nullptr);

      if(fcgp_method == PrestressLossCriteria::FcgpMethodType::Hybrid && 
         pLoss->GetElasticShortening().GetFcgpComputationMethod() == WBFL::LRFD::ElasticShortening::FcgpComputationMethod::Iterative)
      {
         // Elastic shortening loss method switches to iterative solution if jacking stress is not
         // equal to 0.75Fpu. Let user know if this happened.
         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         std::_tstring msg = std::_tstring(SEGMENT_LABEL(segmentKey)) + _T(": ");
         msg += _T("Either the Jacking stress is not equal to 0.75Fpu, or Debonded strands are present, or Temporary strands are present, or the girder is Not Prismatic. Therefore, for the calculation of elastic shortening; an iterative solution was used to find Fcgp after release rather than assuming 0.7*Fpu per the TxDOT design manual.");
         CEAFStatusItem* pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,1,m_StatusGroupID,m_scidGirderDescriptionWarning,msg.c_str());
         pStatusCenter->Add(pStatusItem);
      }
   }
   catch( const WBFL::LRFD::XPsLosses& e )
   {
      Int32 reason = XREASON_AGENTVALIDATIONFAILURE;
      std::_tstring msg = std::_tstring(SEGMENT_LABEL(segmentKey)) + _T(": ");

      CEAFStatusItem* pStatusItem = nullptr;

      if ( e.GetReasonCode() == WBFL::LRFD::XPsLosses::Reason::fpjOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims )
         {
            msg += _T("Prestress losses could not be computed because the prestress jacking stress fpj does not exceed 0.5fpu (see Article 5.9.5.4.4b)\nAdjust the prestress jacking forces");
         }
         else
         {
            msg += _T("Prestress losses could not be computed because the prestress jacking stress fpj does not exceed 0.5fpu (see Article %s\nAdjust the prestress jacking forces") + std::_tstring(WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.2c"), _T("5.9.3.4.2c")));
         }
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,1,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());
      }
      else if ( e.GetReasonCode() == WBFL::LRFD::XPsLosses::Reason::fcOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg += _T("Concrete strength is out of range per LRFD 5.4.2.1 and ") + std::_tstring(WBFL::LRFD::LrfdCw8th(_T("5.9.5.1"), _T("5.9.3.1")));
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,2,m_StatusGroupID,m_scidGirderDescriptionWarning,msg.c_str());
      }
      else
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg += _T("Prestress losses could not be computed because an unspecified error occured");
         pStatusItem = new pgsUnknownErrorStatusItem(m_StatusGroupID,m_scidUnknown,_T(__FILE__),__LINE__,msg.c_str());
      }

      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      ATLASSERT(pStatusItem != nullptr);
      pStatusCenter->Add(pStatusItem);

      msg += _T("\nSee Status Center for Details");
      THROW_UNWIND(msg.c_str(),reason);
   }

   return method;
}

void CPsLossEngineer::LossesByApproxLumpSum(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,LOSSDETAILS* pLosses,bool isWsdot)
{
   PRECONDITION(pLosses != 0 );

   WBFL::LRFD::Losses::SectionPropertiesType spType;
   WBFL::Materials::PsStrand::Grade gradePerm, gradeTemp;
   WBFL::Materials::PsStrand::Type typePerm, typeTemp;
  WBFL::Materials::PsStrand::Coating coatingPerm, coatingTemp;
   Float64 fpjPerm;
   Float64 fpjTTS;
   Float64 perimeter;
   Float64 Ag;
   Float64 Ixx, Iyy, Ixy;
   Float64 Ybg;
   Float64 Ac1;
   Float64 Ic1;
   Float64 Ybc1;
   Float64 Ac2;
   Float64 Ic2;
   Float64 Ybc2;
   Float64 An;
   Float64 Ixxn, Iyyn, Ixyn;
   Float64 Ybn;
   Float64 Acn;
   Float64 Icn;
   Float64 Ybcn;
   Float64 Ad;
   Float64 ed;
   Float64 Ksh;
   WBFL::Geometry::Point2d epermRelease;// eccentricity of the permanent strands on the non-composite section
   WBFL::Geometry::Point2d epermFinal;
   WBFL::Geometry::Point2d etemp;
   Float64 aps;  // area of one prestress strand
   Float64 ApsPerm;
   Float64 ApsTTS;
   Float64 Mdlg;
   std::vector<std::pair<Float64, Float64>> Madlg;
   std::vector<std::pair<Float64, Float64>> Msidl1;
   std::vector<std::pair<Float64, Float64>> Msidl2;

   Float64 rh;
   Float64 ti,th,td,tf; // initial time, time of hauling,time of deck placment, final time

   Float64 PjS, PjH, PjT;

   Float64 GdrShrinkageK1, GdrShrinkageK2;
   Float64 DeckShrinkageK1, DeckShrinkageK2;

   Float64 fci,fc,fcSlab;
   Float64 Eci,Ec,EcSlab;

   Float64 girder_length, span_length;

   WBFL::LRFD::Losses::TempStrandUsage usage;

   Float64 anchorSet,wobble,coeffFriction,angleChange;

   GET_IFACE(IMaterials,pMaterial);
   pgsTypes::ConcreteType girderConcreteType = pMaterial->GetSegmentConcreteType(poi.GetSegmentKey());
   pgsTypes::ConcreteType slabConcreteType   = pMaterial->GetDeckConcreteType();

   if ( girderConcreteType != pgsTypes::Normal || slabConcreteType != pgsTypes::Normal )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      std::_tstring msg(_T("The approximate estimate of time-dependent losses given in LRFD ") +  std::_tstring(WBFL::LRFD::LrfdCw8th(_T("5.9.5.3"),_T("5.9.3.3"))) + _T(" is for members made from normal-weight concrete"));
      pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidConcreteTypeError,msg.c_str());
      pStatusCenter->Add(pStatusItem);

      msg += std::_tstring(_T("\nSee Status Center for Details"));
      THROW_UNWIND(msg.c_str(),XREASON_LRFD_VERSION);
   }

   GetLossParameters(poi, pConfig,&spType,
                     &gradePerm, &typePerm, &coatingPerm, &gradeTemp, &typeTemp, &coatingTemp, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ixx, &Iyy, &Ixy, &Ybg, &Ac1, &Ic1, &Ybc1, &Ac2, &Ic2, &Ybc2, &An, &Ixxn, &Iyyn, &Ixyn, &Ybn, &Acn, &Icn, &Ybcn, &Ad, &ed, &Ksh,
                     &epermRelease, &epermFinal, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl1, &Msidl2, &rh,
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &GdrShrinkageK1, &GdrShrinkageK2,
                     &DeckShrinkageK1, &DeckShrinkageK2,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,
                     &usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();


   // get time to prestress transfer
   GET_IFACE( ISpecification,   pSpec);
   GET_IFACE( ILibrary,         pLib);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   const auto& prestress_loss_criteria = pSpecEntry->GetPrestressLossCriteria();
   try
   {
      if ( WBFL::LRFD::BDSManager::GetEdition() <= WBFL::LRFD::BDSManager::Edition::ThirdEdition2004 )
      {
         // partial prestressing ratio=1 for wsdot method
         Float64 ppr;
         if (isWsdot)
         {
            ppr = 1.0;
            pLosses->LossMethod = PrestressLossCriteria::LossMethodType::WSDOT_LUMPSUM;
         }
         else
         {
            GET_IFACE(ILongRebarGeometry,pLongRebarGeom);
            ppr = pLongRebarGeom->GetPPRBottomHalf(poi);
            pLosses->LossMethod = PrestressLossCriteria::LossMethodType::AASHTO_LUMPSUM;
         }

         Float64 shipping_loss = prestress_loss_criteria.ShippingLosses;

         GET_IFACE_NOCHECK(IMaterials, pMaterial);
         pgsTypes::ConcreteType concreteType = (pConfig ? pConfig->ConcType : pMaterial->GetSegmentConcreteType(segmentKey));

         std::shared_ptr<WBFL::LRFD::ApproximateLosses> pLoss(std::make_shared<WBFL::LRFD::ApproximateLosses>(
                            (WBFL::LRFD::ApproximateLosses::BeamType)beamType,
                            shipping_loss,
                            ppr,
                            poi.GetDistFromStart(), // location along girder where losses are computed
                            girder_length,    // girder length
                            spType,
                            gradePerm,
                            typePerm,
                            coatingPerm,
                            gradeTemp,
                            typeTemp,
                            coatingTemp,
                            fpjPerm,
                            fpjTTS,
                            ApsPerm,  // area of permanent strand
                            ApsTTS,  // area of TTS 
                            aps,      // area of one strand
                            epermRelease, // eccentricty of permanent ps strands with respect to CG of girder
                            epermFinal,
                            etemp, // eccentricty of temporary strands with respect to CG of girder
                            usage,
                            anchorSet,
                            wobble,
                            coeffFriction,
                            angleChange,

                            (WBFL::Materials::ConcreteType)concreteType,
                            fc,
                            fci,
                            fcSlab,
                            Ec,   // Modulus of elasticity of girder
                            Eci,  // Modulus of elasticity of girder at transfer
                            EcSlab,  // Modulus of elasticity of deck

                            Mdlg,  // Dead load moment of girder only
                            Madlg,  // Additional dead load on girder section
                            Msidl1, Msidl2, // Superimposed dead loads

                            Ag,
                            Ixx, Iyy, Ixy,
                            Ybg,
                            Ac1,
                            Ic1,
                            Ybc1,
                            Ac2, Ic2, Ybc2,
                         
                            An,
                            Ixxn, Iyyn, Ixyn,
                            Ybn,
                            Acn,
                            Icn,
                            Ybcn,

                            rh,      // relative humidity
                            ti,   // Time until prestress transfer
                            !isWsdot,false));

         // Any of the "get" methods on WBFL::LRFD::PsLosses can throw an WBFL::LRFD::XPsLosses::Reason exception if
         // the input data is bad.  To make sure we have everything correct, lets request
         // the elastic shortening losses and make sure an exception doesn't throw.
         Float64 pES = pLoss->PermanentStrand_ElasticShorteningLosses();
         // store in shared pointer to base class
         pLosses->pLosses = std::static_pointer_cast<const WBFL::LRFD::Losses>(pLoss);
         ATLASSERT(pLosses->pLosses!=nullptr);
      }
      else
      {
         // 3rd edition /w 2005 interims and later

         // LRFD 5th Edition, 2010, C5.9.5.3 ( or >= 2017, C5.9.3.3)
         // The approximate estimates of time-dependent prestress losses given in Eq 5.9.3.3-1 are intended for sections with composite decks only
         GET_IFACE_NOCHECK(IBridge,pBridge);
         if ( WBFL::LRFD::BDSManager::Edition::FifthEdition2010 <= WBFL::LRFD::BDSManager::GetEdition() && !pBridge->IsCompositeDeck() )
         {
            GET_IFACE(IEAFStatusCenter,pStatusCenter);
            std::_tstring msg(_T("The approximate estimates of time-dependent prestress losses given in Eq ") + std::_tstring(WBFL::LRFD::LrfdCw8th(_T("5.9.5.3-1"),_T("5.9.3.3-1"))) + _T(" are intended for sections with composite decks only."));
            pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidLRFDVersionError,msg.c_str());
            pStatusCenter->Add(pStatusItem);

            msg += std::_tstring(_T("\nSee Status Center for Details"));
            THROW_UNWIND(msg.c_str(),XREASON_LRFD_VERSION);
         }

         std::shared_ptr<WBFL::LRFD::ApproximateLosses2005> pLoss(std::make_shared<WBFL::LRFD::ApproximateLosses2005>(poi.GetDistFromStart(), // location along girder where losses are computed
                            girder_length,    // girder length
                            spType,
                            gradePerm,
                            typePerm,
                            coatingPerm,
                            gradeTemp,
                            typeTemp,
                            coatingTemp,
                            fpjPerm,
                            fpjTTS,
                            ApsPerm,  // area of permanent strand
                            ApsTTS,  // area of TTS 
                            aps,      // area of one strand
                            epermRelease, // eccentricty of permanent ps strands with respect to CG of girder
                            epermFinal,
                            etemp, // eccentricty of temporary strands with respect to CG of girder
                            usage,
                            anchorSet,
                            wobble,
                            coeffFriction,
                            angleChange,

                            fc,
                            fci,
                            fcSlab,
                            Ec,   // Modulus of elasticity of girder
                            Eci,  // Modulus of elasticity of girder at transfer
                            EcSlab,  // Modulus of elasticity of deck

                            Mdlg,  // Dead load moment of girder only
                            Madlg,  // Additional dead load on girder section
                            Msidl1, Msidl2, // Superimposed dead loads

                            Ag,
                            Ixx, Iyy, Ixy,
                            Ybg,
                            Ac1,
                            Ic1,
                            Ybc1,
                            Ac2, Ic2, Ybc2,
                         
                            An,
                            Ixxn, Iyyn, Ixyn,
                            Ybn,
                            Acn,
                            Icn,
                            Ybcn,

                            rh,      // relative humidity
                            ti,   // Time until prestress transfer
                            !isWsdot,
                            false
                            ));

         if ( isWsdot )
         {
            pLosses->LossMethod = PrestressLossCriteria::LossMethodType::WSDOT_LUMPSUM_2005;
         }
         else
         {
            pLosses->LossMethod = PrestressLossCriteria::LossMethodType::AASHTO_LUMPSUM_2005;
         }

         // Any of the "get" methods on WBFL::LRFD::PsLosses can throw an WBFL::LRFD::XPsLosses::Reason exception if
         // the input data is bad.  To make sure we have everything correct, lets request
         // the elastic shortening losses and make sure an exception doesn't throw.
         Float64 pES = pLoss->PermanentStrand_ElasticShorteningLosses();
         // store in shared pointer to base class
         pLosses->pLosses = std::static_pointer_cast<const WBFL::LRFD::Losses>(pLoss);
         ATLASSERT(pLosses->pLosses!=nullptr);
      }

   } // end of try block
   catch( const WBFL::LRFD::XPsLosses& e )
   {
      Int32 reason = XREASON_AGENTVALIDATIONFAILURE;
      std::_tstring msg = std::_tstring(SEGMENT_LABEL(segmentKey)) + _T(": ");

      CEAFStatusItem* pStatusItem = nullptr;

      if ( e.GetReasonCode() == WBFL::LRFD::XPsLosses::Reason::fpjOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims )
         {
            msg += _T("Prestress losses could not be computed because the prestress jacking stress fpj does not exceed 0.5fpu (see Article 5.9.5.4.4b)\nAdjust the prestress jacking forces");
         }
         else
         {
            msg += _T("Prestress losses could not be computed because the prestress jacking stress fpj does not exceed 0.5fpu (see Article ") +  std::_tstring(WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.2c"),_T("5.9.3.4.2c"))) +_T(")\nAdjust the prestress jacking forces");
         }
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,1,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());
      }
      else if ( e.GetReasonCode() == WBFL::LRFD::XPsLosses::Reason::fcOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg += _T("Concrete strength is out of range per LRFD 5.4.2.1 and ") + std::_tstring(WBFL::LRFD::LrfdCw8th(_T("5.9.5.1"),_T("5.9.3.1")));
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,0,m_StatusGroupID,m_scidGirderDescriptionWarning,msg.c_str());
      }
      else
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg += _T("Prestress losses could not be computed because an unspecified error occured");
         pStatusItem = new pgsUnknownErrorStatusItem(m_StatusGroupID,m_scidUnknown,_T(__FILE__),__LINE__,msg.c_str());
      }

      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      ATLASSERT(pStatusItem != nullptr);
      pStatusCenter->Add(pStatusItem);

      msg += std::_tstring(_T("\nSee Status Center for Details"));
      THROW_UNWIND(msg.c_str(),reason);
   }
}

void CPsLossEngineer::LossesByGeneralLumpSum(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,LOSSDETAILS* pLosses)
{
   // Need the following parameters for the lump sum loss object: ApsPerm,ApsTTS,fpjPerm,fpjTTS,usage
   // It is easier to call the general GetLossParameters method and get everything this
   // to create yet another function to get just those parameters we need.
   PRECONDITION(pLosses != 0 );
   WBFL::LRFD::Losses::SectionPropertiesType spType;
   WBFL::Materials::PsStrand::Grade gradePerm, gradeTemp;
   WBFL::Materials::PsStrand::Type typePerm, typeTemp;
  WBFL::Materials::PsStrand::Coating coatingPerm, coatingTemp;
   Float64 fpjPerm;
   Float64 fpjTTS;
   Float64 perimeter;
   Float64 Ag;
   Float64 Ixx, Iyy, Ixy;
   Float64 Ybg;
   Float64 Ac1;
   Float64 Ic1;
   Float64 Ybc1;
   Float64 Ac2;
   Float64 Ic2;
   Float64 Ybc2;
   Float64 An;
   Float64 Ixxn, Iyyn, Ixyn;
   Float64 Ybn;
   Float64 Acn;
   Float64 Icn;
   Float64 Ybcn;
   Float64 Ad;
   Float64 ed;
   Float64 Ksh;
   WBFL::Geometry::Point2d epermRelease;// eccentricity of the permanent strands on the non-composite section
   WBFL::Geometry::Point2d epermFinal;
   WBFL::Geometry::Point2d etemp;
   Float64 aps;  // area of one prestress strand
   Float64 ApsPerm;
   Float64 ApsTTS;
   Float64 Mdlg;
   std::vector<std::pair<Float64, Float64>> Madlg;
   std::vector<std::pair<Float64, Float64>> Msidl1;
   std::vector<std::pair<Float64, Float64>> Msidl2;

   Float64 rh;
   Float64 ti,th,td,tf; // initial time, time of hauling,time of deck placment, final time

   Float64 PjS, PjH, PjT;

   Float64 GdrShrinkageK1, GdrShrinkageK2;
   Float64 DeckShrinkageK1, DeckShrinkageK2;

   Float64 fci,fc,fcSlab;
   Float64 Eci,Ec,EcSlab;

   Float64 girder_length, span_length;

   WBFL::LRFD::Losses::TempStrandUsage usage;

   Float64 anchorSet,wobble,coeffFriction,angleChange;

   GetLossParameters(poi, pConfig,
                     &spType,
                     &gradePerm, &typePerm, &coatingPerm, &gradeTemp, &typeTemp, &coatingTemp, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ixx, &Iyy, &Ixy, &Ybg, &Ac1, &Ic1, &Ybc1, &Ac2, &Ic2, &Ybc2, &An, &Ixxn, &Iyyn, &Ixyn, &Ybn, &Acn, &Icn, &Ybcn, &Ad, &ed, &Ksh,
                     &epermRelease, &epermFinal, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl1, &Msidl2, &rh,
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &GdrShrinkageK1, &GdrShrinkageK2,
                     &DeckShrinkageK1, &DeckShrinkageK2,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,
                     &usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   pLosses->LossMethod = PrestressLossCriteria::LossMethodType::GENERAL_LUMPSUM;

   GET_IFACE(ILossParameters,pLossParameters);

   // the lump sum loss object can deal with no area of prestress (no strand) cases.
   std::shared_ptr<const WBFL::LRFD::LumpSumLosses> pLoss(std::make_shared<WBFL::LRFD::LumpSumLosses>(ApsPerm,ApsTTS,fpjPerm,fpjTTS,usage,
                                          pLossParameters->GetBeforeXferLosses(),
                                          pLossParameters->GetAfterXferLosses(),
                                          pLossParameters->GetLiftingLosses(),
                                          pLossParameters->GetShippingLosses(),
                                          pLossParameters->GetBeforeTempStrandRemovalLosses(),
                                          pLossParameters->GetAfterTempStrandRemovalLosses(),
                                          pLossParameters->GetAfterDeckPlacementLosses(),
                                          pLossParameters->GetAfterSIDLLosses(),
                                          pLossParameters->GetFinalLosses()));

   pLosses->pLosses = std::static_pointer_cast<const WBFL::LRFD::Losses>(pLoss);
   ATLASSERT(pLosses->pLosses!=nullptr);
}


void CPsLossEngineer::ReportRefinedMethodBefore2005(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
#if defined _DEBUG
   // this method is only applicable to PGSuper
   GET_IFACE(IBridge,pIBridge);
   ATLASSERT(pIBridge->GetSegmentCount(girderKey) == 1);
#endif
   CSegmentKey segmentKey(girderKey,0);

   rptParagraph* pParagraph;

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   GET_IFACE(ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Refined Estimate of Time-Dependent Losses [") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.4"),_T("5.9.3.4")) <<_T("]") << rptNewLine;

   PoiList vPoi;
   GetPointsOfInterest(girderKey, &vPoi);

   GET_IFACE(ILosses,pILosses);
   const LOSSDETAILS* pDetails = pILosses->GetLossDetails( vPoi.front() );

   // Do some preliminary setup for the tables.
   INIT_UV_PROTOTYPE( rptForceUnitValue,   force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,        pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  ecc,         pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cg,          pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,  moment,      pDisplayUnits->GetMomentUnit(),          false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  mod_e,       pDisplayUnits->GetModEUnit(),            false );

   bool bTemporaryStrands = ( 0 < Nt && pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPretensioned ? true : false);

   // Relaxation At Prestress Transfer
   ReportInitialRelaxation(pChapter,bTemporaryStrands,pDetails->pLosses.get(),pDisplayUnits,level);

   CElasticShorteningTable*                     pES  = CElasticShorteningTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDetails,pDisplayUnits,level);
   CFrictionLossTable*                          pFR  = nullptr;
   CPostTensionInteractionTable*                pPTT = nullptr;
   CEffectOfPostTensionedTemporaryStrandsTable* pPTP = nullptr;
   
   if ( 0 < Nt && pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   CTimeDependentLossesAtShippingTable*            pPSH = nullptr;
   CPostTensionTimeDependentLossesAtShippingTable* pPTH = nullptr;

   GET_IFACE(ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
   if ( pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled() )
   {
      pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDisplayUnits,level);

      if ( 0 < Nt ) 
      {
         pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
      }
   }

   CTemporaryStrandRemovalTable*        pPTR = nullptr;
   if (0 < Nt )
   {
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Time dependent losses") << rptNewLine;

   CChangeOfConcreteStressTable*   pDeltaFcdp = CChangeOfConcreteStressTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CCreepAndShrinkageTable*        pCR        = CCreepAndShrinkageTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CRelaxationAfterTransferTable*  pR2        = CRelaxationAfterTransferTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CFinalPrestressLossTable*       pT         = CFinalPrestressLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CEffectivePrestressForceTable* pP = CEffectivePrestressForceTable::PrepareTable(pChapter, m_pBroker, segmentKey, pDisplayUnits, level);

   pgsPointOfInterest prev_poi(CSegmentKey(0,0,0),0.0);
   bool bSkipToNextRow = false;
   RowIndexType row1 = 1;
   RowIndexType row2 = 1;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      bSkipToNextRow = false;

      if (row1 != 1 && prev_poi == poi)
      {
         row1--;
         bSkipToNextRow = true;
      }

      // write the location information into the tables
      ReportLocation2(pES,  row1, poi, pDisplayUnits);
      ReportLocation2(pFR,  row1, poi, pDisplayUnits);
      ReportLocation2(pPTT, row1, poi, pDisplayUnits);
      ReportLocation2(pPTP, row1, poi, pDisplayUnits);
      ReportLocation2(pPSH, row1, poi, pDisplayUnits);
      ReportLocation2(pPTH, row1, poi, pDisplayUnits);

      ReportLocation(pPTR,       row2, poi, pDisplayUnits);
      ReportLocation(pDeltaFcdp, row2, poi, pDisplayUnits);
      ReportLocation(pCR,        row2, poi, pDisplayUnits);
      ReportLocation(pR2,        row2, poi, pDisplayUnits);
      ReportLocation(pT,         row2, poi, pDisplayUnits);
      ReportLocation(pP, row2, poi, pDisplayUnits);

      // fill each row1 with data
      if ( !bSkipToNextRow )
      {
         pDetails = pILosses->GetLossDetails( poi );

         ReportRow(pES, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pFR, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPTT,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPTP,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPSH,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPTH,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
      }

      ReportRow(pPTR,      pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pDeltaFcdp,pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pCR,       pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pR2,       pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pT,        pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pP, pChapter, m_pBroker, poi, row2, pDisplayUnits, level);

      row2++;
      
      row1++;
      prev_poi = poi;
   }
}

void CPsLossEngineer::ReportRefinedMethod2005(rptChapter* pChapter,BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
#if defined _DEBUG
   // this method is only applicable to PGSuper
   GET_IFACE(IBridge,pIBridge);
   ATLASSERT(pIBridge->GetSegmentCount(girderKey) == 1);
#endif
   CSegmentKey segmentKey(girderKey,0);

   rptParagraph* pParagraph;

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   GET_IFACE(IBridge,pBridge);

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Refined Estimate of Time-Dependent Losses [") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.4"),_T("5.9.3.4")) <<_T("]") << rptNewLine;

   PoiList vPoi;
   GetPointsOfInterest(girderKey, &vPoi);

   GET_IFACE(ILosses,pILosses);
   const LOSSDETAILS* pDetails = pILosses->GetLossDetails( vPoi.front() );

   GET_IFACE(ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   bool bTemporaryStrands = ( 0 < Nt && pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPretensioned ? true : false);

   bool bDeckShrinkage = pILosses->IsDeckShrinkageApplicable();

   ReportInitialRelaxation(pChapter,bTemporaryStrands,pDetails->pLosses.get(),pDisplayUnits,level);

   ////////////////////////////////////////////////////////////////////////////////////////
   // Create the tables for losses - order is important here... The factory methods
   // put content into the chapter and return a table that needs to be filled up
   CElasticShorteningTable*                     pES  = CElasticShorteningTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDetails,pDisplayUnits,level);
   CAutogenousShrinkageTable*                   pAS  = nullptr;
   CFrictionLossTable*                          pFR  = nullptr;
   CPostTensionInteractionTable*                pPTT = nullptr;
   CEffectOfPostTensionedTemporaryStrandsTable* pPTP = nullptr;

   GET_IFACE(IMaterials, pMaterials);
   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
   {
      pAS = CAutogenousShrinkageTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDetails, pDisplayUnits,level);
   }
   

   if ( 0 < Nt && pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   CShrinkageAtHaulingTable*                       pSRH = nullptr;
   CCreepAtHaulingTable*                           pCRH = nullptr;
   CRelaxationAtHaulingTable*                      pR1H = nullptr;
   CTimeDependentLossesAtShippingTable*            pPSH = nullptr;
   
   CPostTensionTimeDependentLossesAtShippingTable* pPTH  = nullptr;

   // must report this even if not checking hauling because
   // temporary strand removal effects depend on losses at end of hauling stage
	pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
	*pChapter << pParagraph;
   pParagraph->SetName(_T("Time dependent losses between transfer and hauling"));
	*pParagraph << pParagraph->GetName() << _T(" [") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.2"),_T("5.9.3.4.2")) << _T("]") << rptNewLine << rptNewLine;
	
	pSRH = CShrinkageAtHaulingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDetails,pDisplayUnits,level);
	pCRH = CCreepAtHaulingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDetails,pDisplayUnits,level);
	pR1H = CRelaxationAtHaulingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDetails,pDisplayUnits,level);
	pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDisplayUnits,level);
	pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);


   ///////////////////////////////////////////////////////////////////////////////////////////
   // Time-dependent losses between transfer and deck placement
   ///////////////////////////////////////////////////////////////////////////////////////////

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   if ( pBridge->IsCompositeDeck() )
   {
      pParagraph->SetName(_T("Time dependent losses between transfer and deck placement"));
      *pParagraph << pParagraph->GetName() << _T(" [") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.2"), _T("5.9.3.4.2")) << _T("]") << rptNewLine;
   }
   else
   {
      pParagraph->SetName(_T("Time dependent losses between transfer and installation of precast members"));
      *pParagraph << pParagraph->GetName() << _T(" [") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.2, "), _T("5.9.3.4.2, ")) << WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.4"), _T("5.9.3.4.4")) << _T("]") << rptNewLine;
   }

   CShrinkageAtDeckPlacementTable*      pSR = CShrinkageAtDeckPlacementTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CCreepAtDeckPlacementTable*          pCR = CCreepAtDeckPlacementTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CRelaxationAtDeckPlacementTable*     pR1 = CRelaxationAtDeckPlacementTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CTimeDependentLossesAtDeckPlacementTable* pLTid = CTimeDependentLossesAtDeckPlacementTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CTemporaryStrandRemovalTable*        pPTR = nullptr;
   
   if ( 0 < Nt )
   {
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   ///////////////////////////////////////////////////////////////////////////////////////////
   // Time-dependent losses between deck placement and final time
   ///////////////////////////////////////////////////////////////////////////////////////////

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   if ( pBridge->IsCompositeDeck() )
   {
      pParagraph->SetName(_T("Time dependent losses from deck placement to final time"));
      *pParagraph << pParagraph->GetName() << _T(" [") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.3"),_T("5.9.3.4.3")) << _T("]") << rptNewLine;
   }
   else
   {
      pParagraph->SetName(_T("Time dependent losses from installation of precast member to final time"));
      *pParagraph << pParagraph->GetName() << _T(" [") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.3, "),_T("5.9.3.4.3, ")) << WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.4"),_T("5.9.3.4.4")) << _T("]") << rptNewLine;
   }
   
   CShrinkageAtFinalTable*      pSD = CShrinkageAtFinalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CCreepAtFinalTable*          pCD = CCreepAtFinalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CRelaxationAtFinalTable*     pR2 = CRelaxationAtFinalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CElasticGainDueToDeckShrinkageTable*   pSS = nullptr;
   if (bDeckShrinkage)
   {
      pSS = CElasticGainDueToDeckShrinkageTable::PrepareTable(pChapter, m_pBroker, segmentKey, pDetails, pDisplayUnits, level);
   }

   CTimeDependentLossFinalTable* pLTdf = CTimeDependentLossFinalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CTimeDependentLossesTable*    pLT   = CTimeDependentLossesTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);

   CElasticGainDueToDeckPlacementTable*   pED   = CElasticGainDueToDeckPlacementTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CElasticGainDueToSIDLTable*            pSIDL = CElasticGainDueToSIDLTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CElasticGainDueToLiveLoadTable*        pLLIM = CElasticGainDueToLiveLoadTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);

   CEffectivePrestressTable*     pPE   = CEffectivePrestressTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CTotalPrestressLossTable*     pT    = CTotalPrestressLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CEffectivePrestressForceTable* pP = CEffectivePrestressForceTable::PrepareTable(pChapter, m_pBroker, segmentKey, pDisplayUnits, level);

   ///////////////////////////////////////////////////////////////////////
   // Loop over all the POIs and populate the tables with loss information
   pgsPointOfInterest prev_poi(CSegmentKey(0,0,0),0.00);
   bool bSkipToNextRow = false;
   RowIndexType row1 = 1;
   RowIndexType row2 = 1;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      bSkipToNextRow = false;

      if (row1 != 1 && prev_poi == poi)
      {
         row1--;
         bSkipToNextRow = true;
      }

      // write the location information into the tables
      ReportLocation2(pES,  row1, poi, pDisplayUnits);
      ReportLocation2(pAS,  row1, poi, pDisplayUnits);
      ReportLocation2(pFR,  row1, poi, pDisplayUnits);
      ReportLocation2(pPTT, row1, poi, pDisplayUnits);
      ReportLocation2(pPTP, row1, poi, pDisplayUnits); 

      ReportLocation2(pSRH, row1, poi, pDisplayUnits);
      ReportLocation2(pCRH, row1, poi, pDisplayUnits);
      ReportLocation2(pR1H, row1, poi, pDisplayUnits);
      ReportLocation2(pPSH, row1, poi, pDisplayUnits);
      ReportLocation2(pPTH, row1, poi, pDisplayUnits);

      ReportLocation(pSR,   row2, poi, pDisplayUnits);
      ReportLocation(pCR,   row2, poi, pDisplayUnits);
      ReportLocation(pR1,   row2, poi, pDisplayUnits);
      ReportLocation(pLTid, row2, poi, pDisplayUnits);
      ReportLocation(pPTR,  row2, poi, pDisplayUnits);
      ReportLocation(pSD,   row2, poi, pDisplayUnits);
      ReportLocation(pCD,   row2, poi, pDisplayUnits);
      ReportLocation(pR2,   row2, poi, pDisplayUnits);
      if (bDeckShrinkage)
      {
         ReportLocation(pSS, row2, poi, pDisplayUnits);
      }
      ReportLocation(pLTdf, row2, poi, pDisplayUnits);
      ReportLocation(pLT,   row2, poi, pDisplayUnits);
      ReportLocation(pED,   row2, poi, pDisplayUnits);
      ReportLocation(pSIDL, row2, poi, pDisplayUnits);
      ReportLocation(pLLIM, row2, poi, pDisplayUnits);
      ReportLocation(pPE,   row2, poi, pDisplayUnits);
      ReportLocation(pT,    row2, poi, pDisplayUnits);
      ReportLocation(pP, row2, poi, pDisplayUnits);

      // fill each row1 with data
      if ( !bSkipToNextRow )
      {
         pDetails = pILosses->GetLossDetails( poi );

         ReportRow(pES, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pAS, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits, level);
         ReportRow(pFR, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPTT,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPTP,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);

         ReportRow(pSRH, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pCRH, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pR1H, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPSH, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPTH, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
      }

      ReportRow(pSR,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pCR,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pR1,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pLTid,pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pPTR, pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pSD,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pCD,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pR2,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      if (bDeckShrinkage)
      {
         ReportRow(pSS, pChapter, m_pBroker, poi, row2, pDetails, pDisplayUnits, level);
      }
      ReportRow(pED,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pSIDL,pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pLLIM,pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);

      ReportRow(pLTdf,pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pLT,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pT,   pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);

      ReportRow(pPE,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pP, pChapter, m_pBroker, poi, row2, pDisplayUnits, level);

      row2++;
      
      row1++;
      prev_poi = poi;
   }
}

void CPsLossEngineer::ReportRefinedMethodTxDOT2013(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
#if defined _DEBUG
   // this method is only applicable to PGSuper
   GET_IFACE(IBridge,pIBridge);
   ATLASSERT(pIBridge->GetSegmentCount(girderKey) == 1);
#endif
   CSegmentKey segmentKey(girderKey,0);

   rptParagraph* pParagraph;

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Refined Estimate of Time-Dependent Losses Per TxDOT Research Report 0-6374-2") << rptNewLine;

   PoiList vPoi;
   GetPointsOfInterest(girderKey, &vPoi);

   GET_IFACE(ILosses,pILosses);
   const LOSSDETAILS* pDetails = pILosses->GetLossDetails( vPoi.front() );

   // Do some preliminary setup for the tables.
   INIT_UV_PROTOTYPE( rptForceUnitValue,   force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,        pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  ecc,         pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cg,          pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,  moment,      pDisplayUnits->GetMomentUnit(),          false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  mod_e,       pDisplayUnits->GetModEUnit(),            false );

   GET_IFACE(ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
   bool bTemporaryStrands = ( 0 < Nt && pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPretensioned ? true : false);

   // NOTE: The order of everything from here to the loop is important. 
   // It is the order in which things are put into the report
   ReportInitialRelaxation(pChapter,bTemporaryStrands,pDetails->pLosses.get(),pDisplayUnits,level);

   CElasticShorteningTable*                     pES  = CElasticShorteningTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,
                                                                                             pDetails,pDisplayUnits,level);
   CFrictionLossTable*                          pFR  = nullptr;
   CPostTensionInteractionTable*                pPTT = nullptr;
   CEffectOfPostTensionedTemporaryStrandsTable* pPTP = nullptr;
   
   if ( 0 < Nt && pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   CTimeDependentLossesAtShippingTable*            pPSH = nullptr;
   CPostTensionTimeDependentLossesAtShippingTable* pPTH = nullptr;

   GET_IFACE(ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
   if ( pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled() )
   {
      pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDisplayUnits,level);

      if ( 0 < Nt ) 
      {
         pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
      }
   }

   CTemporaryStrandRemovalTable* pPTR = nullptr;
   if (0 < Nt )
   {
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName(_T("Time dependent losses"));
   *pParagraph << pParagraph->GetName() << rptNewLine;

   CTxDOT2013ChangeOfConcreteStressTable*   pDeltaFcdp = CTxDOT2013ChangeOfConcreteStressTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CTxDOT2013CreepAndShrinkageTable*        pCR        = CTxDOT2013CreepAndShrinkageTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CTxDOT2013RelaxationAfterTransferTable*  pR2        = CTxDOT2013RelaxationAfterTransferTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CTxDOT2013TimeDependentLossesTable*      pLT        = CTxDOT2013TimeDependentLossesTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CTotalPrestressLossTable*                pT         = CTotalPrestressLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CEffectivePrestressForceTable* pP = CEffectivePrestressForceTable::PrepareTable(pChapter, m_pBroker, segmentKey, pDisplayUnits, level);

   pgsPointOfInterest prev_poi(CSegmentKey(0,0,0),0.00);
   bool bSkipToNextRow = false;
   RowIndexType row1 = 1;
   RowIndexType row2 = 1;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      bSkipToNextRow = false;

      if (row1 != 1 && prev_poi == poi)
      {
         row1--;
         bSkipToNextRow = true;
      }

      // write the location information into the tables
      ReportLocation2(pES,  row1, poi, pDisplayUnits);
      ReportLocation2(pFR,  row1, poi, pDisplayUnits);
      ReportLocation2(pPTT, row1, poi, pDisplayUnits);
      ReportLocation2(pPTP, row1, poi, pDisplayUnits);
      ReportLocation2(pPSH, row1, poi, pDisplayUnits);
      ReportLocation2(pPTH, row1, poi, pDisplayUnits);

      ReportLocation(pPTR,       row2, poi, pDisplayUnits);
      ReportLocation(pDeltaFcdp, row2, poi, pDisplayUnits);
      ReportLocation(pCR,        row2, poi, pDisplayUnits);
      ReportLocation(pR2,        row2, poi, pDisplayUnits);
      ReportLocation(pLT,        row2, poi, pDisplayUnits);
      ReportLocation(pT,         row2, poi, pDisplayUnits);
      ReportLocation(pP,         row2, poi, pDisplayUnits);

      // fill each row1 with data
      if ( !bSkipToNextRow )
      {
         pDetails = pILosses->GetLossDetails( poi );

         ReportRow(pES, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pFR, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPTT,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPTP,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPSH,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPTH,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
      }

      ReportRow(pPTR,      pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pDeltaFcdp,pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pCR,       pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pR2,       pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pLT,       pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pT,        pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pP,        pChapter, m_pBroker, poi, row2, pDisplayUnits, level);

      row2++;
      
      row1++;
      prev_poi = poi;
   }
}

void CPsLossEngineer::ReportApproxMethod(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level,bool isWsdot)
{
#if defined _DEBUG
   // this method is only applicable to PGSuper
   GET_IFACE(IBridge,pIBridge);
   ATLASSERT(pIBridge->GetSegmentCount(girderKey) == 1);
#endif
   CSegmentKey segmentKey(girderKey,0);

   rptParagraph* pParagraph;

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);

   GET_IFACE(ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   PoiList vPoi;
   GetPointsOfInterest(girderKey, &vPoi);

   GET_IFACE(ILosses,pILosses);
   const LOSSDETAILS* pDetails = pILosses->GetLossDetails( vPoi.front() );

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   if (isWsdot)
   {
      *pParagraph << _T("Approximate Lump Sum Estimate of Time-Dependent Losses [WSDOT BDM 6.1.5.B3]") << rptNewLine;
   }
   else
   {
      *pParagraph << _T("Approximate Lump Sum Estimate of Time-Dependent Losses [") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.3"),_T("5.9.3.3")) << _T("]") << rptNewLine;
   }

   bool bTemporaryStrands = ( 0 < Nt && pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPretensioned ? true : false);

   //////////////////////////////////////////////////////////
   // NOTE: The order of everything from here to the loop is important. 
   // It is the order in which things are put into the report
   ReportInitialRelaxation(pChapter,bTemporaryStrands,pDetails->pLosses.get(),pDisplayUnits,level);

   CElasticShorteningTable*                        pES  = CElasticShorteningTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDetails,pDisplayUnits,level);

   ReportLumpSumTimeDependentLossesAtShipping(pChapter,pDetails,pDisplayUnits,level);

   CFrictionLossTable*                             pFR  = nullptr;
   CPostTensionInteractionTable*                   pPTT = nullptr;
   CEffectOfPostTensionedTemporaryStrandsTable*    pPTP = nullptr;

   if ( 0 < Nt && pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   CTimeDependentLossesAtShippingTable*            pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDisplayUnits,level);
   CPostTensionTimeDependentLossesAtShippingTable* pPTH  = nullptr;
   
   if ( 0 < Nt )
   {
      pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   CTemporaryStrandRemovalTable*                   pPTR = nullptr;
   if ( 0 < Nt )
   {
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   ReportLumpSumTimeDependentLosses(pChapter,pDetails,pDisplayUnits,level);

   CTotalPrestressLossTable* pT = CTotalPrestressLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CEffectivePrestressForceTable* pP = CEffectivePrestressForceTable::PrepareTable(pChapter, m_pBroker, segmentKey, pDisplayUnits, level);

   pgsPointOfInterest prev_poi(CSegmentKey(0,0,0),0.0);
   bool bSkipToNextRow = false;
   RowIndexType row1 = 1;
   RowIndexType row2 = 1;
   // **** NOTE **** Use 2 row counters.. one for the table with CY and BS poi, and a different on for BS poi only
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      bSkipToNextRow = false;

      if (row1 != 1 && prev_poi == poi)
      {
         row1--;
         bSkipToNextRow = true;
      }

      ReportLocation2(pES,  row1, poi, pDisplayUnits);
      ReportLocation2(pFR,  row1, poi, pDisplayUnits);
      ReportLocation2(pPTT, row1, poi, pDisplayUnits);
      ReportLocation2(pPTP, row1, poi, pDisplayUnits);
      ReportLocation2(pPSH, row1, poi, pDisplayUnits);
      ReportLocation2(pPTH, row1, poi, pDisplayUnits);

      ReportLocation(pPTR, row2, poi, pDisplayUnits);
      ReportLocation(pT,   row2, poi, pDisplayUnits);
      ReportLocation(pP, row2, poi, pDisplayUnits);

      if ( !bSkipToNextRow )
      {
         pDetails = pILosses->GetLossDetails( poi );

         ReportRow(pES, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pFR, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPTT,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPTP,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPSH,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPTH,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
      }

         ReportRow(pPTR,pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
         ReportRow(pT,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pP, pChapter, m_pBroker, poi, row2, pDisplayUnits, level);

         row2++;
      row1++;
      prev_poi = poi;
   }
}

void CPsLossEngineer::ReportApproxMethod2005(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
#if defined _DEBUG
   // this method is only applicable to PGSuper
   GET_IFACE(IBridge,pIBridge);
   ATLASSERT(pIBridge->GetSegmentCount(girderKey) == 1);
#endif
   CSegmentKey segmentKey(girderKey,0);

   rptParagraph* pParagraph;

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   GET_IFACE(ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);

   PoiList vPoi;
   GetPointsOfInterest(girderKey, &vPoi);

   GET_IFACE(ILosses,pILosses);
   const LOSSDETAILS* pDetails = pILosses->GetLossDetails( vPoi.front() );

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Approximate Estimate of Time-Dependent Losses [") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.3"),_T("5.9.3.3")) << _T("]") << rptNewLine;

   bool bTemporaryStrands = ( 0 < Nt && pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPretensioned ? true : false);

   //////////////////////////////////////////////////////////
   // NOTE: The order of everything from here to the loop is important. 
   // It is the order in which things are put into the report
   ReportInitialRelaxation(pChapter,bTemporaryStrands,pDetails->pLosses.get(),pDisplayUnits,level);

   CElasticShorteningTable*                        pES  = CElasticShorteningTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDetails,pDisplayUnits,level);

   ReportLumpSumTimeDependentLossesAtShipping(pChapter,pDetails,pDisplayUnits,level);

   CFrictionLossTable*                             pFR  = nullptr;
   CPostTensionInteractionTable*                   pPTT = nullptr;
   CEffectOfPostTensionedTemporaryStrandsTable*    pPTP = nullptr;

   if ( 0 < Nt && pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   CTimeDependentLossesAtShippingTable*            pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDisplayUnits,level);
   CPostTensionTimeDependentLossesAtShippingTable* pPTH = nullptr;

   if ( 0 < Nt )
   {
      pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   CTemporaryStrandRemovalTable*                   pPTR = nullptr;
   if ( 0 < Nt )
   {
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   ReportLumpSumTimeDependentLosses(pChapter,pDetails,pDisplayUnits,level);

   CElasticGainDueToDeckPlacementTable*   pED   = CElasticGainDueToDeckPlacementTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CElasticGainDueToSIDLTable*            pSIDL = CElasticGainDueToSIDLTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CElasticGainDueToLiveLoadTable*        pLLIM = CElasticGainDueToLiveLoadTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CEffectivePrestressTable*              pPE   = CEffectivePrestressTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);

   CTotalPrestressLossTable* pT = CTotalPrestressLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CEffectivePrestressForceTable* pP = CEffectivePrestressForceTable::PrepareTable(pChapter, m_pBroker, segmentKey, pDisplayUnits, level);

   pgsPointOfInterest prev_poi(CSegmentKey(0,0,0),0.0);
   bool bSkipToNextRow = false;
   RowIndexType row1 = 1;
   RowIndexType row2 = 1;
   // **** NOTE **** Use 2 row counters.. one for the table with CY and BS poi, and a different one for BS poi only
   for( const pgsPointOfInterest& poi : vPoi)
   {
      bSkipToNextRow = false;

      if (row1 != 1 && prev_poi == poi)
      {
         row1--;
         bSkipToNextRow = true;
      }

      ReportLocation2(pES,  row1, poi, pDisplayUnits);
      ReportLocation2(pFR,  row1, poi, pDisplayUnits);
      ReportLocation2(pPTT, row1, poi, pDisplayUnits);
      ReportLocation2(pPTP, row1, poi, pDisplayUnits);
      ReportLocation2(pPSH, row1, poi, pDisplayUnits);
      ReportLocation2(pPTH, row1, poi, pDisplayUnits);

      ReportLocation(pPTR,  row2, poi, pDisplayUnits);
      ReportLocation(pED,   row2, poi, pDisplayUnits);
      ReportLocation(pSIDL, row2, poi, pDisplayUnits);
      ReportLocation(pLLIM, row2, poi, pDisplayUnits);
      ReportLocation(pPE,   row2, poi, pDisplayUnits);
      ReportLocation(pT,    row2, poi, pDisplayUnits);
      ReportLocation(pP,    row2, poi, pDisplayUnits);

      if ( !bSkipToNextRow )
      {
         pDetails = pILosses->GetLossDetails( poi );

         ReportRow(pES, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pFR, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPTT,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPTP,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPSH,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
         ReportRow(pPTH,pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
      }

      //if ( intervalIdx == liveLoadIntervalIdx )
      {
         ReportRow(pPTR,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
         ReportRow(pED,   pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
         ReportRow(pSIDL, pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
         ReportRow(pLLIM, pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
         ReportRow(pPE,   pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
         ReportRow(pT,    pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
         ReportRow(pP,    pChapter,m_pBroker,poi,row2,pDisplayUnits,level);

         row2++;
      }
      
      row1++;
      prev_poi = poi;
   }
}


void CPsLossEngineer::ReportLumpSumMethod(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,bool bDesign,Uint16 level)
{
#if defined _DEBUG
   // this method is only applicable to PGSuper
   GET_IFACE(IBridge,pBridge);
   ATLASSERT(pBridge->GetSegmentCount(girderKey) == 1);
#endif
   CSegmentKey segmentKey(girderKey,0);

   rptParagraph* pParagraph;

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary);

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName(_T("General Lump Sum Estimate Losses"));
   *pParagraph << pParagraph->GetName() << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   rptRcTable* table = rptStyleManager::CreateDefaultTable(2,_T(""));
   table->SetColumnWidth(0,3.0);
   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pParagraph << table << rptNewLine;

   // Do some preliminary setup for the tables.
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   PoiList vPoi;
   GetPointsOfInterest(girderKey, &vPoi);

   GET_IFACE(ILosses,pILosses);
   const LOSSDETAILS* pDetails = pILosses->GetLossDetails( vPoi.front() );

   // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const WBFL::LRFD::LumpSumLosses> ptl = std::dynamic_pointer_cast<const WBFL::LRFD::LumpSumLosses>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return;
   }

   RowIndexType row = 0;

   (*table)(row,0) << _T("Stage"); (*table)(row++,1) << COLHDR(_T("Loss"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   if ( bDesign )
   {
      (*table)(row,0) << _T("Before prestress transfer"); (*table)(row++,1) << stress.SetValue(ptl->GetBeforeXferLosses());
      (*table)(row,0) << _T("After prestress transfer");  (*table)(row++,1) << stress.SetValue(ptl->GetAfterXferLosses());
      (*table)(row,0) << _T("At girder lifting");         (*table)(row++,1) << stress.SetValue(ptl->GetLiftingLosses());
      (*table)(row,0) << _T("At girder shipping");        (*table)(row++,1) << stress.SetValue(ptl->GetShippingLosses());

      if ( 0 < NtMax )
      {
         (*table)(row,0) << _T("Before temporary strand removal"); (*table)(row++,1) << stress.SetValue(ptl->GetBeforeTempStrandRemovalLosses());
         (*table)(row,0) << _T("After temporary strand removal");  (*table)(row++,1) << stress.SetValue(ptl->GetAfterTempStrandRemovalLosses());
      }

      (*table)(row,0) << _T("After deck placement");            (*table)(row++,1) << stress.SetValue(ptl->GetAfterDeckPlacementLosses());
   }

   (*table)(row,0) << _T("Final");  (*table)(row++,1) << stress.SetValue(ptl->GetFinalLosses());
}


//////////////////////////////////////////////////
// Utility functions for reporting
void CPsLossEngineer::ReportInitialRelaxation(rptChapter* pChapter,bool bTemporaryStrands,const WBFL::LRFD::Losses* pLosses,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   if ( pLosses->IgnoreInitialRelaxation() )
   {
      return; // nothing to do
   }

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDisplayUnits->GetStressUnit(),          false );

   // Relaxation At Prestress Transfer
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName(_T("Relaxation before transfer"));
   *pParagraph <<  pParagraph->GetName() << rptNewLine;

   rptRcTable* table;

   pParagraph = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Permanent Strands") << rptNewLine;
   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if ( pLosses->GetPermanentStrandType() == WBFL::Materials::PsStrand::Type::LowRelaxation )
   {
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR0_LR.png")) << rptNewLine;
   }
   else 
   {
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR0_SR.png")) <<rptNewLine;
   }

   if ( pLosses->GetPermanentStrandCoating() != WBFL::Materials::PsStrand::Coating::None )
   {
      *pParagraph << EPOXY_RELAXATION_NOTE << rptNewLine;
   }

   table = rptStyleManager::CreateDefaultTable(4);
   *pParagraph << table << rptNewLine;

   (*table)(0,0) << _T("t") << rptNewLine << _T("(Days)");
   (*table)(0,1) << COLHDR(RPT_FPJ, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,2) << COLHDR(RPT_FPY, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,3) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR0")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   (*table)(1,0) << WBFL::Units::ConvertFromSysUnits( pLosses->GetInitialAge(), WBFL::Units::Measure::Day );
   (*table)(1,1) << stress.SetValue( pLosses->GetFpjPermanent() );
   (*table)(1,2) << stress.SetValue( pLosses->GetFpyPermanent() );
   (*table)(1,3) << stress.SetValue( pLosses->PermanentStrand_RelaxationLossesBeforeTransfer() );

   if ( bTemporaryStrands )
   {
      pParagraph = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      *pChapter << pParagraph;
      *pParagraph << _T("Temporary Strands") << rptNewLine;
      pParagraph = new rptParagraph;
      *pChapter << pParagraph;

      if ( pLosses->GetTemporaryStrandType() == WBFL::Materials::PsStrand::Type::LowRelaxation )
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR0_LR.png")) << rptNewLine;
      }
      else 
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR0_SR.png")) <<rptNewLine;
      }

      if ( pLosses->GetTemporaryStrandCoating() != WBFL::Materials::PsStrand::Coating::None )
      {
         *pParagraph << EPOXY_RELAXATION_NOTE << rptNewLine;
      }

      table = rptStyleManager::CreateDefaultTable(4);
      *pParagraph << table << rptNewLine;

      (*table)(0,0) << _T("t") << rptNewLine << _T("(Days)");
      (*table)(0,1) << COLHDR(RPT_FPJ, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,2) << COLHDR(RPT_FPY, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,3) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR0")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      (*table)(1,0) << WBFL::Units::ConvertFromSysUnits( pLosses->GetInitialAge(), WBFL::Units::Measure::Day );
      (*table)(1,1) << stress.SetValue( pLosses->GetFpjTemporary() );
      (*table)(1,2) << stress.SetValue( pLosses->GetFpyTemporary() );
      (*table)(1,3) << stress.SetValue( pLosses->TemporaryStrand_RelaxationLossesBeforeTransfer() );
   }
}

void CPsLossEngineer::ReportLocation2(rptRcTable* pTable,RowIndexType row,const pgsPointOfInterest& poi,IEAFDisplayUnits* pDisplayUnits)
{
   if ( pTable == nullptr )
   {
      return;
   }

   INIT_UV_PROTOTYPE( rptPointOfInterest,  spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptPointOfInterest,  gdrloc,      pDisplayUnits->GetSpanLengthUnit(),      false );

   RowIndexType rowOffset = pTable->GetNumberOfHeaderRows() - 1;

   (*pTable)(row+rowOffset,0) << gdrloc.SetValue( POI_RELEASED_SEGMENT, poi );
   (*pTable)(row+rowOffset,1) << spanloc.SetValue( POI_ERECTED_SEGMENT, poi );
}

void CPsLossEngineer::ReportLocation(rptRcTable* pTable,RowIndexType row,const pgsPointOfInterest& poi,IEAFDisplayUnits* pDisplayUnits)
{
   if ( pTable == nullptr )
   {
      return;
   }

   INIT_UV_PROTOTYPE( rptPointOfInterest,  spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );

   RowIndexType rowOffset = pTable->GetNumberOfHeaderRows() - 1;
   (*pTable)(row+rowOffset,0) << spanloc.SetValue( POI_ERECTED_SEGMENT, poi );
}

void CPsLossEngineer::ReportLumpSumTimeDependentLossesAtShipping(rptChapter* pChapter,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   // Lump Sum Loss at time of shipping
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   CString strApproxMethod(WBFL::LRFD::BDSManager::GetEdition() <= WBFL::LRFD::BDSManager::Edition::ThirdEdition2004 ? _T("Approximate Lump Sum Estimate") : _T("Approximate Estimate"));
   *pParagraph << strApproxMethod << _T(" of Time Dependent Losses at Hauling") << rptNewLine;

   if ( pDetails->LossMethod == PrestressLossCriteria::LossMethodType::AASHTO_LUMPSUM || pDetails->LossMethod == PrestressLossCriteria::LossMethodType::WSDOT_LUMPSUM )
   {
      // Approximate methods before 2005
      GET_IFACE( ISpecification,   pSpec);
      GET_IFACE( ILibrary,         pLib);
      std::_tstring spec_name = pSpec->GetSpecification();
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
      const auto& prestress_loss_criteria = pSpecEntry->GetPrestressLossCriteria();
      Float64 shipping_losses = prestress_loss_criteria.ShippingLosses;

      pParagraph = new rptParagraph;
      *pChapter << pParagraph;

      if ( shipping_losses < 0 )
      {
         // % of long term
         *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pLTH")) << _T(" = ") << -1*shipping_losses << _T("(") << symbol(DELTA) << RPT_STRESS(_T("pLT")) << _T(")") << rptNewLine;
      }

      INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDisplayUnits->GetStressUnit(),          true );
      *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pLTH")) << _T(" = ") << stress.SetValue(pDetails->pLosses->PermanentStrand_TimeDependentLossesAtShipping()) << rptNewLine;
   }
   else
   {
      // Approximate methods, 2005
      if ( IS_SI_UNITS(pDisplayUnits) )
      {
         *pParagraph<< rptRcImage(strImagePath + _T("LumpSumLoss_Shipping_2005_SI.png")) << rptNewLine;
      }
      else
      {
         *pParagraph<< rptRcImage(strImagePath + _T("LumpSumLoss_Shipping_2005_US.png")) << rptNewLine;
      }

      rptRcTable* table = rptStyleManager::CreateDefaultTable(9,_T(""));

      INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,        pDisplayUnits->GetAreaUnit(),            false );
      INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDisplayUnits->GetStressUnit(),          false );

      rptRcScalar scalar;
      scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Automatic );
      scalar.SetWidth(6);
      scalar.SetPrecision(2);

      *pParagraph << table << rptNewLine;

      RowIndexType row = 0;
      ColumnIndexType col = 0;
      (*table)(row, col++) << COLHDR(RPT_FCI, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(row, col++) << Sub2(symbol(gamma),_T("st"));
      (*table)(row, col++) << _T("Relative") << rptNewLine << _T("Humidity (%)");
      (*table)(row, col++) << Sub2(symbol(gamma),_T("h"));
      (*table)(row, col++) << COLHDR(RPT_STRESS(_T("pi")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(row, col++) << COLHDR(Sub2(_T("A"),_T("ps")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(row, col++) << COLHDR(Sub2(_T("A"),_T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(row, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(row, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLTH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      // Typecast to our known type (eating own doggy food)
      std::shared_ptr<const WBFL::LRFD::ApproximateLosses2005> ptl = std::dynamic_pointer_cast<const WBFL::LRFD::ApproximateLosses2005>(pDetails->pLosses);
      if (!ptl)
      {
         ATLASSERT(false); // made a bad cast? Bail...
         return;
      }

      row++;
      col = 0;
      (*table)(row, col++) << stress.SetValue( pDetails->pLosses->GetFci() );
      (*table)(row, col++) << scalar.SetValue( ptl->GetStrengthFactor() );
      (*table)(row, col++) << pDetails->pLosses->GetRelHumidity();
      (*table)(row, col++) << scalar.SetValue( ptl->GetHumidityFactor() );
      (*table)(row, col++) << stress.SetValue( ptl->GetFpi() );
      (*table)(row, col++) << area.SetValue( pDetails->pLosses->GetApsPermanent() );
      Float64 Ag, Ybg, Ixx, Iyy, Ixy;
      pDetails->pLosses->GetNoncompositeProperties(&Ag, &Ybg, &Ixx, &Iyy, &Ixy);
      (*table)(row, col++) << area.SetValue( Ag );
      (*table)(row, col++) << stress.SetValue( ptl->PermanentStrand_RelaxationLossesAtXfer() );
      (*table)(row, col++) << stress.SetValue( pDetails->pLosses->PermanentStrand_TimeDependentLossesAtShipping() );
   }
}

void CPsLossEngineer::ReportLumpSumTimeDependentLosses(rptChapter* pChapter,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   CString strApproxMethod(WBFL::LRFD::BDSManager::GetEdition() <= WBFL::LRFD::BDSManager::Edition::ThirdEdition2004 ? _T("Approximate Lump Sum Estimate") : _T("Approximate Estimate"));
   *pParagraph << strApproxMethod << _T(" of Time Dependent Losses") << rptNewLine;

   if ( pDetails->LossMethod == PrestressLossCriteria::LossMethodType::AASHTO_LUMPSUM || pDetails->LossMethod == PrestressLossCriteria::LossMethodType::WSDOT_LUMPSUM )
   {
      std::_tstring strLossEqnImage[2][5][2][2]; 
      // dim 0... 0 = LRFD, 1 = WSDOT
      // dim 1... 0 = I Beam, 1 = U Beam, 2 = SolidSlab, 3 = Box Beams, 4 = Single T
      // dim 2... 0 = Low Relax, 1 = Stress Rel
      // dim 2... 0 = SI, 1 = US
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::IBeam][0][0]     = _T("ApproxLoss_LRFD_IBeam_LowRelax_SI.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::IBeam][1][0]     = _T("ApproxLoss_LRFD_IBeam_StressRel_SI.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::UBeam][0][0]     = _T("ApproxLoss_LRFD_UBeam_LowRelax_SI.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::UBeam][1][0]     = _T("ApproxLoss_LRFD_UBeam_StressRel_SI.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::SolidSlab][0][0] = _T("ApproxLoss_LRFD_SolidSlab_LowRelax_SI.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::SolidSlab][1][0] = _T("ApproxLoss_LRFD_SolidSlab_StressRel_SI.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::BoxBeam][0][0]   = _T("ApproxLoss_LRFD_BoxGirder_LowRelax_SI.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::BoxBeam][1][0]   = _T("ApproxLoss_LRFD_BoxGirder_StressRel_SI.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::SingleT][0][0]   = _T("ApproxLoss_LRFD_SingleT_LowRelax_SI.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::SingleT][1][0]   = _T("ApproxLoss_LRFD_SingleT_StressRel_SI.png");

      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::IBeam][0][0]     = _T("ApproxLoss_WSDOT_IBeam_LowRelax_SI.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::IBeam][1][0]     = _T("ApproxLoss_WSDOT_IBeam_StressRel_SI.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::UBeam][0][0]     = _T("ApproxLoss_WSDOT_UBeam_LowRelax_SI.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::UBeam][1][0]     = _T("ApproxLoss_WSDOT_UBeam_StressRel_SI.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::SolidSlab][0][0] = _T("ApproxLoss_LRFD_SolidSlab_LowRelax_SI.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::SolidSlab][1][0] = _T("ApproxLoss_LRFD_SolidSlab_StressRel_SI.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::BoxBeam][0][0]   = _T("ApproxLoss_LRFD_BoxGirder_LowRelax_SI.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::BoxBeam][1][0]   = _T("ApproxLoss_LRFD_BoxGirder_StressRel_SI.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::SingleT][0][0]   = _T("ApproxLoss_LRFD_SingleT_LowRelax_SI.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::SingleT][1][0]   = _T("ApproxLoss_LRFD_SingleT_StressRel_SI.png");

      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::IBeam][0][1]     = _T("ApproxLoss_LRFD_IBeam_LowRelax_US.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::IBeam][1][1]     = _T("ApproxLoss_LRFD_IBeam_StressRel_US.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::UBeam][0][1]     = _T("ApproxLoss_LRFD_UBeam_LowRelax_US.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::UBeam][1][1]     = _T("ApproxLoss_LRFD_UBeam_StressRel_US.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::SolidSlab][0][1] = _T("ApproxLoss_LRFD_SolidSlab_LowRelax_US.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::SolidSlab][1][1] = _T("ApproxLoss_LRFD_SolidSlab_StressRel_US.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::BoxBeam][0][1]   = _T("ApproxLoss_LRFD_BoxGirder_LowRelax_US.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::BoxBeam][1][1]   = _T("ApproxLoss_LRFD_BoxGirder_StressRel_US.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::SingleT][0][1]   = _T("ApproxLoss_LRFD_SingleT_LowRelax_US.png");
      strLossEqnImage[0][+WBFL::LRFD::ApproximateLosses::BeamType::SingleT][1][1]   = _T("ApproxLoss_LRFD_SingleT_StressRel_US.png");

      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::IBeam][0][1]     = _T("ApproxLoss_WSDOT_IBeam_LowRelax_US.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::IBeam][1][1]     = _T("ApproxLoss_WSDOT_IBeam_StressRel_US.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::UBeam][0][1]     = _T("ApproxLoss_WSDOT_UBeam_LowRelax_US.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::UBeam][1][1]     = _T("ApproxLoss_WSDOT_UBeam_StressRel_US.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::SolidSlab][0][1] = _T("ApproxLoss_LRFD_SolidSlab_LowRelax_US.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::SolidSlab][1][1] = _T("ApproxLoss_LRFD_SolidSlab_StressRel_US.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::BoxBeam][0][1]   = _T("ApproxLoss_LRFD_BoxGirder_LowRelax_US.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::BoxBeam][1][1]   = _T("ApproxLoss_LRFD_BoxGirder_StressRel_US.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::SingleT][0][1]   = _T("ApproxLoss_LRFD_SingleT_LowRelax_US.png");
      strLossEqnImage[1][+WBFL::LRFD::ApproximateLosses::BeamType::SingleT][1][1]   = _T("ApproxLoss_LRFD_SingleT_StressRel_US.png");

      int method = (pDetails->LossMethod == PrestressLossCriteria::LossMethodType::WSDOT_LUMPSUM) ? 1 : 0;

      // Typecast to our known type (eating own doggy food)
      std::shared_ptr<const WBFL::LRFD::ApproximateLosses> ptl = std::dynamic_pointer_cast<const WBFL::LRFD::ApproximateLosses>(pDetails->pLosses);
      if (!ptl)
      {
         ATLASSERT(false); // made a bad cast? Bail...
         return;
      }

      int beam = (int)ptl->GetBeamType();
      int strand = pDetails->pLosses->GetPermanentStrandType() == WBFL::Materials::PsStrand::Type::LowRelaxation ? 0 : 1;
      int units = IS_SI_UNITS(pDisplayUnits);

      
      pParagraph = new rptParagraph;
      *pChapter << pParagraph;
      *pParagraph<< rptRcImage(strImagePath + strLossEqnImage[method][beam][strand][units]) << rptNewLine;

      INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDisplayUnits->GetStressUnit(),          true );
      *pParagraph << RPT_FC << _T(" = ") << stress.SetValue(pDetails->pLosses->GetFc() ) << rptNewLine;
      *pParagraph << _T("PPR = ") << ptl->GetPPR() << rptNewLine;
      *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pLT")) << _T(" = ") << stress.SetValue( pDetails->pLosses->TimeDependentLosses() ) << rptNewLine;
   }
   else
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
      {
         *pParagraph<< rptRcImage(strImagePath + _T("LumpSumLoss_2005_SI.png")) << rptNewLine;
      }
      else
      {
         *pParagraph<< rptRcImage(strImagePath + _T("LumpSumLoss_2005_US.png")) << rptNewLine;
      }

      rptRcTable* table = rptStyleManager::CreateDefaultTable(9,_T(""));

      INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,        pDisplayUnits->GetAreaUnit(),            false );
      INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDisplayUnits->GetStressUnit(),          false );

      rptRcScalar scalar;
      scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Automatic );
      scalar.SetWidth(6);
      scalar.SetPrecision(2);

      *pParagraph << table << rptNewLine;

      RowIndexType row = 0;
      ColumnIndexType col = 0;

      (*table)(row, col++) << COLHDR(RPT_FCI, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(row, col++) << Sub2(symbol(gamma),_T("st"));
      (*table)(row, col++) << _T("Relative") << rptNewLine << _T("Humidity (%)");
      (*table)(row, col++) << Sub2(symbol(gamma),_T("h"));
      (*table)(row, col++) << COLHDR(RPT_STRESS(_T("pi")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(row, col++) << COLHDR(Sub2(_T("A"),_T("ps")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(row, col++) << COLHDR(Sub2(_T("A"),_T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(row, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(row, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      // Typecast to our known type (eating own doggy food)
      std::shared_ptr<const WBFL::LRFD::ApproximateLosses2005> ptl = std::dynamic_pointer_cast<const WBFL::LRFD::ApproximateLosses2005>(pDetails->pLosses);
      if (!ptl)
      {
         ATLASSERT(false); // made a bad cast? Bail...
         return;
      }

      row++;
      col = 0;
      (*table)(row, col++) << stress.SetValue( pDetails->pLosses->GetFci() );
      (*table)(row, col++) << scalar.SetValue( ptl->GetStrengthFactor() );
      (*table)(row, col++) << pDetails->pLosses->GetRelHumidity();
      (*table)(row, col++) << scalar.SetValue( ptl->GetHumidityFactor() );
      (*table)(row, col++) << stress.SetValue( ptl->GetFpi() );
      (*table)(row, col++) << area.SetValue( pDetails->pLosses->GetApsPermanent() );
      Float64 Ag, Ybg, Ixx, Iyy, Ixy;
      pDetails->pLosses->GetNoncompositeProperties(&Ag, &Ybg, &Ixx, &Iyy, &Ixy);
      (*table)(row, col++) << area.SetValue( Ag );
      (*table)(row, col++) << stress.SetValue( ptl->PermanentStrand_RelaxationLossesAtXfer() );
      (*table)(row, col++) << stress.SetValue( pDetails->pLosses->TimeDependentLosses() );
   }
}

void CPsLossEngineer::GetLossParameters(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,
   WBFL::LRFD::Losses::SectionPropertiesType* pSectionProperties,
   WBFL::Materials::PsStrand::Grade* pGradePerm,
   WBFL::Materials::PsStrand::Type* pTypePerm,
  WBFL::Materials::PsStrand::Coating* pCoatingPerm,
   WBFL::Materials::PsStrand::Grade* pGradeTemp,
   WBFL::Materials::PsStrand::Type* pTypeTemp,
  WBFL::Materials::PsStrand::Coating* pCoatingTemp,
   Float64* pFpjPerm,
   Float64* pFpjTTS,
   Float64* pPerimeter,
   Float64* pAg,
   Float64* pIxx,
   Float64* pIyy,
   Float64* pIxy,
   Float64* pYbg,
   Float64* pAc1,
   Float64* pIc1,
   Float64* pYbc1,
   Float64* pAc2,
   Float64* pIc2,
   Float64* pYbc2,
   Float64* pAn,
   Float64* pIxxn,
   Float64* pIyyn,
   Float64* pIxyn,
   Float64* pYbn,
   Float64* pAcn,
   Float64* pIcn,
   Float64* pYbcn,
   Float64* pAd,
   Float64* ped,
   Float64* pKsh,
   WBFL::Geometry::Point2d* pepermRelease,// eccentricity of the permanent strands on the non-composite section
   WBFL::Geometry::Point2d* pepermFinal,
   WBFL::Geometry::Point2d* petemp,
   Float64* paps,  // area of one prestress strand
   Float64* pApsPerm,
   Float64* pApsTTS,
   Float64* pMdlg,
   std::vector<std::pair<Float64, Float64>>* pMadlg,
   std::vector<std::pair<Float64, Float64>>* pMsidl1,
   std::vector<std::pair<Float64, Float64>>* pMsidl2,
   Float64* prh,
   Float64* pti,
   Float64* pth,
   Float64* ptd,
   Float64* ptf,
   Float64* pPjS,
   Float64* pPjH,
   Float64* pPjT,
   Float64* pGdrShrinkageK1,
   Float64* pGdrShrinkageK2,
   Float64* pDeckShrinkageK1,
   Float64* pDeckShrinkageK2,
   Float64* pFci,
   Float64* pFc,
   Float64* pFcSlab,
   Float64* pEci,
   Float64* pEc,
   Float64* pEcSlab,
   Float64* pGirderLength,
   Float64* pSpanLength,
   WBFL::LRFD::Losses::TempStrandUsage* pUsage,
   Float64* pAnchorSet,
   Float64* pWobble,
   Float64* pCoeffFriction,
   Float64* pAngleChange
)
{
   GET_IFACE(IBridge, pBridge);
   GET_IFACE(IStrandGeometry, pStrandGeom);
   GET_IFACE(ISegmentData, pSegmentData);
   GET_IFACE(ISectionProperties, pSectProp);
   GET_IFACE(IProductForces, pProdForces);
   GET_IFACE(IEnvironment, pEnv);
   GET_IFACE(IMaterials, pMaterial);
   GET_IFACE(ISpecification, pSpec);
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   GET_IFACE(ILibrary, pLibrary);
   GET_IFACE(IIntervals, pIntervals);
   GET_IFACE(IGirder, pGirder);
   GET_IFACE(IPointOfInterest, pPoi);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);

   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType tsInstallationIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
   IntervalIndexType erectIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();
   IntervalIndexType castShearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();
   IntervalIndexType castLongitudinalJointIntervalIdx = pIntervals->GetCastLongitudinalJointInterval();
   IntervalIndexType compositeLongitudinalJointIntervalIdx = pIntervals->GetCompositeLongitudinalJointInterval();
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(deckCastingRegionIdx);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   IntervalIndexType noncompositeIntervalIdx = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType noncompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();
   IntervalIndexType compositeUserLoadIntervalIdx = pIntervals->GetCompositeUserLoadInterval();

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   *pApsPerm = pStrandGeom->GetStrandArea(poi, releaseIntervalIdx, pgsTypes::Permanent, pConfig);
   *pApsTTS = pStrandGeom->GetStrandArea(poi, releaseIntervalIdx, pgsTypes::Temporary, pConfig);
   *pPjS = pStrandGeom->GetPjack(segmentKey, pgsTypes::Straight, pConfig);
   *pPjH = pStrandGeom->GetPjack(segmentKey, pgsTypes::Harped, pConfig);
   *pPjT = pStrandGeom->GetPjack(segmentKey, pgsTypes::Temporary, pConfig);
   *pFpjPerm = pStrandGeom->GetJackingStress(segmentKey, pgsTypes::Permanent, pConfig);
   *pFpjTTS = pStrandGeom->GetJackingStress(segmentKey, pgsTypes::Temporary, pConfig);

   const auto* pPermStrand = pSegmentData->GetStrandMaterial(segmentKey, pgsTypes::Straight);
   ATLASSERT(pPermStrand);
   *pGradePerm = pPermStrand->GetGrade();
   *pTypePerm = pPermStrand->GetType();
   *pCoatingPerm = pPermStrand->GetCoating();

   const auto* pTempStrand = pSegmentData->GetStrandMaterial(segmentKey, pgsTypes::Temporary);
   ATLASSERT(pTempStrand);
   *pGradeTemp = pTempStrand->GetGrade();
   *pTypeTemp = pTempStrand->GetType();
   *pCoatingTemp = pTempStrand->GetCoating();
   *paps = pTempStrand->GetNominalArea();

   *pGirderLength = pBridge->GetSegmentLength(segmentKey);
   *pSpanLength = pBridge->GetSegmentSpanLength(segmentKey);

   Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);


   //
   // Material Properties
   //

   // Girder
   *pGdrShrinkageK1 = pMaterial->GetSegmentShrinkageK1(segmentKey);
   *pGdrShrinkageK2 = pMaterial->GetSegmentShrinkageK2(segmentKey);

   if (pConfig)
   {
      *pFci = pConfig->fci;
      *pFc = pConfig->fc;

      if (pConfig->bUserEci)
      {
         *pEci = pConfig->Eci;
      }
      else
      {
         *pEci = pMaterial->GetEconc(pConfig->ConcType,pConfig->fci, pMaterial->GetSegmentStrengthDensity(segmentKey), pMaterial->GetSegmentEccK1(segmentKey), pMaterial->GetSegmentEccK2(segmentKey));
      }

      if (pConfig->bUserEc)
      {
         *pEc = pConfig->Ec;
      }
      else
      {
         *pEc = pMaterial->GetEconc(pConfig->ConcType, pConfig->fc, pMaterial->GetSegmentStrengthDensity(segmentKey), pMaterial->GetSegmentEccK1(segmentKey), pMaterial->GetSegmentEccK2(segmentKey));
      }
   }
   else
   {
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey); // steps up to f'c at hauling (see Concrete Manager)
      *pFci = pMaterial->GetSegmentFc(segmentKey, releaseIntervalIdx);
      *pEci = pMaterial->GetSegmentEc(segmentKey, releaseIntervalIdx);

      *pFc = pMaterial->GetSegmentFc(segmentKey, haulingIntervalIdx);
      *pEc = pMaterial->GetSegmentEc(segmentKey, haulingIntervalIdx);
   }

   // Deck
   if (IsNonstructuralDeck(pBridge->GetDeckType()))
   {
      *pDeckShrinkageK1 = 0;
      *pDeckShrinkageK2 = 0;

      *pFcSlab = 0;
      *pEcSlab = 0;
   }
   else
   {
      *pDeckShrinkageK1 = pMaterial->GetDeckShrinkageK1();
      *pDeckShrinkageK2 = pMaterial->GetDeckShrinkageK2();

      *pFcSlab = pMaterial->GetDeckFc(deckCastingRegionIdx,compositeDeckIntervalIdx);

      if (pDeck->Concrete.bUserEc)
      {
         *pEcSlab = pDeck->Concrete.Ec;
      }
      else
      {
         *pEcSlab = pMaterial->GetDeckEc(deckCastingRegionIdx,compositeDeckIntervalIdx);
      }
   }


   // eccentricity of the permanent strands at release
   pepermRelease->Move(pStrandGeom->GetEccentricity(releaseIntervalIdx, poi, pgsTypes::Permanent, pConfig));

   // eccentricity of permanent strands at the last interval when the girder is noncomposite
   pepermFinal->Move(pStrandGeom->GetEccentricity(noncompositeIntervalIdx, poi, pgsTypes::Permanent, pConfig));

   // eccentricity of the temporary strands
   petemp->Move(pStrandGeom->GetEccentricity(tsInstallationIntervalIdx, poi, pgsTypes::Temporary, pConfig));

   pgsTypes::SectionPropertyType spType = (pSectProp->GetSectionPropertiesMode() == pgsTypes::spmGross ? pgsTypes::sptGross : pgsTypes::sptTransformed);
   *pSectionProperties = (spType == pgsTypes::sptGross ? WBFL::LRFD::Losses::SectionPropertiesType::Gross : WBFL::LRFD::Losses::SectionPropertiesType::Transformed);

   *pPerimeter = pSectProp->GetPerimeter(poi);
   *pAg = pSectProp->GetAg(spType, releaseIntervalIdx, poi);
   *pIxx = pSectProp->GetIxx(spType, releaseIntervalIdx, poi);
   *pIyy = pSectProp->GetIyy(spType, releaseIntervalIdx, poi);
   *pIxy = pSectProp->GetIxy(spType, releaseIntervalIdx, poi);
   *pYbg = pSectProp->GetY(spType, releaseIntervalIdx, poi, pgsTypes::BottomGirder);
   if (pGirder->HasStructuralLongitudinalJoints() && IsStructuralDeck(pDeck->GetDeckType()))
   {
      *pAc1 = pSectProp->GetAg(spType, compositeLongitudinalJointIntervalIdx, poi, pConfig);
      *pIc1 = pSectProp->GetIxx(spType, compositeLongitudinalJointIntervalIdx, poi, pConfig);
      *pYbc1 = pSectProp->GetY(spType, compositeLongitudinalJointIntervalIdx, poi, pgsTypes::BottomGirder, pConfig);

      *pAc2 = pSectProp->GetAg(spType, liveLoadIntervalIdx, poi, pConfig);
      *pIc2 = pSectProp->GetIxx(spType, liveLoadIntervalIdx, poi, pConfig);
      *pYbc2 = pSectProp->GetY(spType, liveLoadIntervalIdx, poi, pgsTypes::BottomGirder, pConfig);
   }
   else
   {
      *pAc1 = pSectProp->GetAg(spType, liveLoadIntervalIdx, poi, pConfig);
      *pIc1 = pSectProp->GetIxx(spType, liveLoadIntervalIdx, poi, pConfig);
      *pYbc1 = pSectProp->GetY(spType, liveLoadIntervalIdx, poi, pgsTypes::BottomGirder, pConfig);
      *pAc2 = *pAc1;
      *pIc2 = *pIc1;
      *pYbc2 = *pYbc1;
   }

   if ( spType == pgsTypes::sptTransformed )
   {
      *pAn   = pSectProp->GetNetAg(erectIntervalIdx, poi);
      *pIxxn = pSectProp->GetNetIxx(erectIntervalIdx, poi);
      *pIyyn = pSectProp->GetNetIyy(erectIntervalIdx, poi);
      *pIxyn = pSectProp->GetNetIxy(erectIntervalIdx, poi);
      *pYbn  = pSectProp->GetNetYbg(erectIntervalIdx, poi);

      // compute composite net properties
      Float64 Eg = *pEc;
      Float64 Ed = *pEcSlab;
      Float64 Ang  = pSectProp->GetNetAg(liveLoadIntervalIdx,poi);
      Float64 Ing  = pSectProp->GetNetIxx(liveLoadIntervalIdx,poi);
      Float64 Ybng = pSectProp->GetNetYbg(liveLoadIntervalIdx,poi);
      Float64 Ytng = pSectProp->GetNetYtg(liveLoadIntervalIdx,poi);
      Float64 And  = pSectProp->GetNetAd(liveLoadIntervalIdx,poi);
      Float64 Ind  = pSectProp->GetNetId(liveLoadIntervalIdx,poi);
      Float64 Ybnd = Ybng + Ytng + pSectProp->GetNetYbd(liveLoadIntervalIdx,poi);

      Float64 Anc  = (Eg*Ang + Ed*And)/Eg;
      Float64 Ybnc = (Eg*Ang*Ybng + Ed*And*Ybnd)/(Eg*Anc);
      Float64 Inc  = (Eg*(Ing + Ang*(Ybnc - Ybng)*(Ybnc - Ybng)) + Ed*(Ind + And*(Ybnc - Ybnd)*(Ybnc - Ybnd)))/Eg;

      *pAcn  = Anc;
      *pIcn  = Inc;
      *pYbcn = Ybnc;
   }
   else
   {
      // gross
      *pAn   = *pAg;
      *pIxxn = *pIxx;
      *pIyyn = *pIyy;
      *pIxyn = *pIxy;
      *pYbn  = *pYbg;
      *pAcn  = *pAc2;
      *pIcn  = *pIc2;
      *pYbcn = *pYbc2;
   }

   // area of deck
   if ( pBridge->IsCompositeDeck() )
   {
      if ( spType == pgsTypes::sptTransformed )
      {
         *pAd  = pSectProp->GetNetAd( compositeDeckIntervalIdx, poi );
      }
      else
      {
         *pAd  = pSectProp->GetGrossDeckArea( poi );
      }

      // eccentricity of deck... use gross slab depth because sacrificial wearing surface hasn't worn off while early age shrinkage is occurring
      *ped = pSectProp->GetY(compositeDeckIntervalIdx, poi, pgsTypes::TopGirder, pConfig) + pBridge->GetGrossSlabDepth(poi) / 2;
      *ped *= -1;
   }
   else
   {
       *pAd = 0.0;
       *ped = 0.0;
   }

   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   *pMdlg  = pProdForces->GetMoment( releaseIntervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative);

   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());
   const auto& prestress_loss_criteria = pSpecEntry->GetPrestressLossCriteria();

   // effectiveness of deck shrinkage
   *pKsh = prestress_loss_criteria.SlabShrinkageElasticGain;

   Float64 K_slab    = prestress_loss_criteria.SlabElasticGain;
   Float64 K_slabpad = prestress_loss_criteria.SlabPadElasticGain;
   Float64 K_dia     = prestress_loss_criteria.DiaphragmElasticGain;
   Float64 K_userdc1 = prestress_loss_criteria.UserDCElasticGain_BeforeDeckPlacement;
   Float64 K_userdw1 = prestress_loss_criteria.UserDWElasticGain_BeforeDeckPlacement;

   Float64 K_railing = prestress_loss_criteria.RailingSystemElasticGain;
   Float64 K_userdc2 = prestress_loss_criteria.UserDCElasticGain_AfterDeckPlacement;
   Float64 K_userdw2 = prestress_loss_criteria.UserDWElasticGain_AfterDeckPlacement;
   Float64 K_overlay = prestress_loss_criteria.OverlayElasticGain;

   if ( spType == pgsTypes::sptTransformed )
   {
      // effectiveness factors don't apply for transformed properties
      // elastic gains are computed implicitly and can't be scaled.
      *pKsh     = 1.0;
      K_slab    = 1.0;
      K_slabpad = 1.0;
      K_dia     = 1.0;
      K_userdc1 = 1.0;
      K_userdw1 = 1.0;
      K_railing = 1.0;
      K_userdc2 = 1.0;
      K_userdw2 = 1.0;
      K_overlay = 1.0;
   }

   if ( poi.GetDistFromStart() < end_size || end_size + *pSpanLength < poi.GetDistFromStart() )
   {
      *pMadlg = std::vector<std::pair<Float64, Float64>>{ std::make_pair(0.0,0.0) };
   }
   else
   {
      *pMadlg = std::vector<std::pair<Float64, Float64>>();
      if (castDiaphragmIntervalIdx != INVALID_INDEX)
      {
         pMadlg->emplace_back(pProdForces->GetMoment(castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm, poi, bat, rtCumulative), K_dia);
      }

      if (castShearKeyIntervalIdx != INVALID_INDEX)
      {
         pMadlg->emplace_back(pProdForces->GetMoment(castShearKeyIntervalIdx, pgsTypes::pftShearKey, poi, bat, rtCumulative), K_dia);
      }

      if (castLongitudinalJointIntervalIdx != INVALID_INDEX)
      {
         pMadlg->emplace_back(pProdForces->GetMoment(castLongitudinalJointIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, bat, rtCumulative), K_slab);
      }
      
      if ( !pGirder->HasStructuralLongitudinalJoints() )
      {
         // if the bridge doesn't have structural longitudinal joints the deck is applied to the non-composite girder section as added dead load
         // (see below for the case when the bridge has structural longitudinal joints)
         if (castDeckIntervalIdx != INVALID_INDEX)
         {
            pMadlg->emplace_back(pProdForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlab, poi, bat, rtCumulative), K_slab);
            pMadlg->emplace_back(pProdForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlabPad, poi, bat, rtCumulative), K_slabpad);
         }
      }

      GET_IFACE(IPointOfInterest,pPoi);
      CSpanKey spanKey;
      Float64 Xspan;
      pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);
      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::ProductForceType pfType = (i == 0 ? pgsTypes::pftUserDC : pgsTypes::pftUserDW);
         Float64 K = (i == 0 ? K_userdc1 : K_userdw1);
         std::vector<IntervalIndexType> vUserLoadIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey,pfType);
         for( const auto& intervalIdx : vUserLoadIntervals)
         {
            if ( intervalIdx <= noncompositeUserLoadIntervalIdx )
            {
               pMadlg->emplace_back(pProdForces->GetMoment(intervalIdx, pfType, poi, bat, rtIncremental), K);
            }
         }
      }


      if ( pDeck->GetDeckType() == pgsTypes::sdtCompositeSIP )
      {
         pMadlg->emplace_back(pProdForces->GetMoment( castDeckIntervalIdx, pgsTypes::pftSlabPanel, poi, bat, rtCumulative ), K_slab);
      }
   }

   *pMsidl1 = std::vector<std::pair<Float64, Float64>>();;
   *pMsidl2 = std::vector<std::pair<Float64, Float64>>();;
   pMsidl2->emplace_back(pProdForces->GetMoment( railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtCumulative ) +
                         pProdForces->GetMoment( railingSystemIntervalIdx, pgsTypes::pftSidewalk,       poi, bat, rtCumulative ), K_railing);

   if (pGirder->HasStructuralLongitudinalJoints())
   {
      // if the bridge has structural longitudinal joints, the deck is applied to the section that is composite
      // of the bare girder and the joints
      if (castDeckIntervalIdx != INVALID_INDEX)
      {
         pMsidl1->emplace_back(pProdForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlab, poi, bat, rtCumulative), K_slab);
         pMsidl1->emplace_back(pProdForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlabPad, poi, bat, rtCumulative), K_slabpad);
      }
   }

   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);
   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::ProductForceType pfType = (i == 0 ? pgsTypes::pftUserDC : pgsTypes::pftUserDW);
      Float64 K = (i == 0 ? K_userdc2 : K_userdw2);
      std::vector<IntervalIndexType> vUserLoadIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey,pfType);
      for( const auto& intervalIdx : vUserLoadIntervals)
      {
         if (compositeUserLoadIntervalIdx <= intervalIdx )
         {
            if (pGirder->HasStructuralLongitudinalJoints() && intervalIdx < compositeDeckIntervalIdx)
            {
               pMsidl1->emplace_back(pProdForces->GetMoment(intervalIdx, pfType, poi, bat, rtIncremental),K);
            }
            else
            {
               pMsidl2->emplace_back(pProdForces->GetMoment(intervalIdx, pfType, poi, bat, rtIncremental),K);
            }
         }
      }
   }

   // include the overlay dead load. even future overlays contribute dead load and creep effects
   // from the time it is installed until final. we don't know when "future" is and it is conservative
   // to assume it is applied at a time when it contributes to the creep loss.
   // See PCI BDM Example 9.1a that supports this approach
   if ( pBridge->HasOverlay() && overlayIntervalIdx != INVALID_INDEX )
   {
      pMsidl2->emplace_back(pProdForces->GetMoment( overlayIntervalIdx, pgsTypes::pftOverlay, poi, bat, rtCumulative ), K_overlay);
   }



   if (m_bComputingLossesForDesign)
   {
      // get the additional moment caused by the difference in input and design "A" dimension
      ATLASSERT(pConfig != nullptr); // if we are designing, we must be using a config object
      Float64 Mslab = pProdForces->GetDesignSlabMomentAdjustment(poi, pConfig);
      Float64 Mslabpad = pProdForces->GetDesignSlabPadMomentAdjustment(poi, pConfig);

      if (pGirder->HasStructuralLongitudinalJoints())
      {
         pMsidl1->emplace_back(Mslab, K_slab);
         pMsidl1->emplace_back(Mslabpad, K_slabpad);
      }
      else
      {
         pMadlg->emplace_back(Mslab, K_slab);
         pMadlg->emplace_back(Mslabpad, K_slabpad);
      }
   }

  
   *prh = pEnv->GetRelHumidity();

   // get time to prestress transfer
   const auto& creep_criteria = pSpecEntry->GetCreepCriteria();
   *pti = creep_criteria.XferTime;
   *pth = prestress_loss_criteria.ShippingTime;
   *ptd = creep_criteria.CreepDuration2Max;
   *ptf = creep_criteria.TotalCreepDuration;

   // Update the data members of the loss calculation object.  It will take care of the rest
   switch (pConfig ? pConfig->PrestressConfig.TempStrandUsage : pStrands->GetTemporaryStrandUsage())
   {
   case pgsTypes::ttsPretensioned:
      *pUsage = WBFL::LRFD::Losses::TempStrandUsage::Pretensioned;
      break;

   case pgsTypes::ttsPTBeforeLifting:
      *pUsage = WBFL::LRFD::Losses::TempStrandUsage::PTBeforeLifting;
      break;

   case pgsTypes::ttsPTAfterLifting:
      *pUsage = WBFL::LRFD::Losses::TempStrandUsage::PTAfterLifting;
      break;

   case pgsTypes::ttsPTBeforeShipping:
      *pUsage = WBFL::LRFD::Losses::TempStrandUsage::PTBeforeShipping;
      break;
   }

   GET_IFACE(ILossParameters,pLossParameters);
   pLossParameters->GetTemporaryStrandPostTensionParameters(pAnchorSet,pWobble,pCoeffFriction);

   Float64 precamber = pGirder->GetPrecamber(segmentKey);
   Float64 L = *pGirderLength;
   *pAngleChange = fabs(8*precamber/L);
}

void CPsLossEngineer::ReportFinalLosses(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE(ILossParameters,pLossParameters);
   PrestressLossCriteria::LossMethodType loss_method = pLossParameters->GetLossMethod();

   Uint16 level = 0;
   switch( loss_method )
   {
   case PrestressLossCriteria::LossMethodType::AASHTO_REFINED:
   case PrestressLossCriteria::LossMethodType::AASHTO_LUMPSUM:
   case PrestressLossCriteria::LossMethodType::TXDOT_REFINED_2013:
      ReportFinalLossesRefinedMethod(pChapter,beamType,girderKey,pDisplayUnits,laAASHTO);
      break;

   case PrestressLossCriteria::LossMethodType::WSDOT_REFINED:
   case PrestressLossCriteria::LossMethodType::WSDOT_LUMPSUM:
      ReportFinalLossesRefinedMethod(pChapter,beamType,girderKey,pDisplayUnits,laWSDOT);
      break;

   case PrestressLossCriteria::LossMethodType::TXDOT_REFINED_2004:
      ReportFinalLossesRefinedMethod(pChapter,beamType,girderKey,pDisplayUnits,laTxDOT);
      break;

   case PrestressLossCriteria::LossMethodType::GENERAL_LUMPSUM:
      ReportGeneralLumpSumMethod(beamType,girderKey,pChapter,pDisplayUnits,false,0);
      break;

   default:
      ATLASSERT(false); // Should never get here
   }
}


void CPsLossEngineer::ReportFinalLossesRefinedMethod(rptChapter* pChapter,BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,LossAgency lossAgency)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if ( pSpecEntry->GetSpecificationCriteria().GetEdition() <= WBFL::LRFD::BDSManager::Edition::ThirdEdition2004 ||
        lossAgency==laTxDOT)
   {
      ReportFinalLossesRefinedMethodBefore2005(pChapter,beamType,girderKey,pDisplayUnits);
   }
   else
   {
      ReportFinalLossesRefinedMethod(pChapter,beamType,girderKey,pDisplayUnits);
   }
}

void CPsLossEngineer::ReportFinalLossesRefinedMethod(rptChapter* pChapter,BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits)
{
#if defined _DEBUG
   // this method is only applicable to PGSuper
   GET_IFACE(IBridge,pIBridge);
   ATLASSERT(pIBridge->GetSegmentCount(girderKey) == 1);
#endif
   CSegmentKey segmentKey(girderKey,0);

   rptParagraph* pParagraph;

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Final Prestress Losses") << rptNewLine;

   PoiList vPoi;
   GetPointsOfInterest(girderKey, &vPoi);

   GET_IFACE(ILosses,pILosses);
   const LOSSDETAILS* pDetails = pILosses->GetLossDetails( vPoi.front() );

   CTotalPrestressLossTable* pT = CTotalPrestressLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,0);
   CEffectivePrestressForceTable* pP = CEffectivePrestressForceTable::PrepareTable(pChapter, m_pBroker, segmentKey, pDisplayUnits, 0);

   ///////////////////////////////////////////////////////////////////////
   // Loop over all the POIs and populate the tables with loss information
   RowIndexType row = 1;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      const LOSSDETAILS* pDetails = pILosses->GetLossDetails( poi );

      ReportLocation(pT,row,poi,pDisplayUnits);
      ReportLocation(pP, row, poi, pDisplayUnits);

      // fill each row1 with data
      ReportRow(pT,pChapter,m_pBroker,poi,row,pDetails,pDisplayUnits,0);
      ReportRow(pP, pChapter, m_pBroker, poi, row, pDisplayUnits, 0);
      row++;
   }
}

void CPsLossEngineer::ReportFinalLossesRefinedMethodBefore2005(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits)
{
#if defined _DEBUG
   // this method is only applicable to PGSuper
   GET_IFACE(IBridge,pIBridge);
   ATLASSERT(pIBridge->GetSegmentCount(girderKey) == 1);
#endif
   CSegmentKey segmentKey(girderKey,0);

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Refined Estimate of Time-Dependent Losses [") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.4"),_T("5.9.3.4")) << _T("]") << rptNewLine;

   PoiList vPoi;
   GetPointsOfInterest(girderKey, &vPoi);

   GET_IFACE(ILosses,pILosses);

   CFinalPrestressLossTable* pT = CFinalPrestressLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,0);
   CEffectivePrestressForceTable* pP = CEffectivePrestressForceTable::PrepareTable(pChapter, m_pBroker, segmentKey, pDisplayUnits, 0);

   RowIndexType row = 1;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      ReportLocation(pT, row, poi, pDisplayUnits);
      ReportLocation(pP, row, poi, pDisplayUnits);

      const LOSSDETAILS* pDetails = pILosses->GetLossDetails( poi );

      ReportRow(pT, pChapter, m_pBroker, poi, row, pDetails, pDisplayUnits, 0);
      ReportRow(pP, pChapter, m_pBroker, poi, row, pDisplayUnits, 0);

      row++;
   }
}

void CPsLossEngineer::GetPointsOfInterest(const CGirderKey& girderKey,PoiList* pPoiList)
{
#if defined _DEBUG
   // this method is only applicable to PGSuper
   GET_IFACE(IBridge,pIBridge);
   ATLASSERT(pIBridge->GetSegmentCount(girderKey) == 1);
#endif
   CSegmentKey segmentKey(girderKey,0);

   GET_IFACE(IPointOfInterest,pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT, pPoiList);
   PoiList vPoi2;
   pPoi->GetPointsOfInterest(segmentKey,POI_RELEASED_SEGMENT, &vPoi2);
   pPoiList->insert(std::end(*pPoiList), std::cbegin(vPoi2), std::cend(vPoi2));
   pPoi->GetPointsOfInterest(segmentKey,POI_SPAN, pPoiList);
   pPoi->GetPointsOfInterest(segmentKey,(POI_SPECIAL | POI_SECTCHANGE_LEFTFACE | POI_SECTCHANGE_RIGHTFACE | POI_START_FACE | POI_END_FACE) & ~POI_CLOSURE, pPoiList, POIFIND_OR);
   pPoi->SortPoiList(pPoiList); // sorts and removes duplicates

   // remove all POI that are not between the ends of the actual segment
   Float64 Xmin = vPoi2.front().get().GetDistFromStart(); // poi's at end of released segment
   Float64 Xmax = vPoi2.back().get().GetDistFromStart();
   pPoiList->erase(std::remove_if(std::begin(*pPoiList), std::end(*pPoiList), [Xmin=Xmin, Xmax=Xmax](const pgsPointOfInterest& poi) {return poi.GetDistFromStart() < Xmin || Xmax < poi.GetDistFromStart();}),std::end(*pPoiList));
}
