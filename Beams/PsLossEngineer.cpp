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

#include "..\PGSuperException.h"

#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>

#include <PgsExt\ReportStyleHolder.h>
#include <PgsExt\GirderPointOfInterest.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\BridgeDescription2.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\LoadFactors.h>

#include <Material\PsStrand.h>

#include "ElasticShorteningTable.h"
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


template <class T>
void ReportRow(T* pTable,rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   if ( pTable == NULL )
      return;

   pTable->AddRow(pChapter,pBroker,poi,row,pDetails,pDisplayUnits,level);
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
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridge,pBridge);
   const GDRCONFIG& config = pBridge->GetSegmentConfiguration(segmentKey);

   return ComputeLosses(beamType,poi,config);
}

LOSSDETAILS CPsLossEngineer::ComputeLosses(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   LOSSDETAILS details;

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const unitmgtLengthData& length = pDisplayUnits->GetSpanLengthUnit();

   GET_IFACE(ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE(ILossParameters,pLossParameters);
   pgsTypes::LossMethod loss_method = pLossParameters->GetLossMethod();

   switch ( loss_method )
   {
   case pgsTypes::AASHTO_REFINED:
      LossesByRefinedEstimate(beamType,poi,config,&details,laAASHTO);
      break;

   case pgsTypes::WSDOT_REFINED:
      LossesByRefinedEstimate(beamType,poi,config,&details,laWSDOT);
      break;

   case pgsTypes::TXDOT_REFINED_2004:
      LossesByRefinedEstimate(beamType,poi,config,&details,laTxDOT);
      break;

   case pgsTypes::TXDOT_REFINED_2013:
      LossesByRefinedEstimateTxDOT2013(beamType,poi,config,&details);
      break;

   case pgsTypes::AASHTO_LUMPSUM:
      LossesByApproxLumpSum(beamType,poi,config,&details,false);
      break;

   case pgsTypes::WSDOT_LUMPSUM:
      LossesByApproxLumpSum(beamType,poi,config,&details,true);
      break;

   case pgsTypes::GENERAL_LUMPSUM:
      LossesByGeneralLumpSum(beamType,poi,config,&details);
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
   details = ComputeLosses(beamType,poi,config);
   m_bComputingLossesForDesign = false;

   return details;
}

void CPsLossEngineer::BuildReport(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE(ILossParameters,pLossParameters);
   pgsTypes::LossMethod loss_method = pLossParameters->GetLossMethod();

   Uint16 level = 0;
   switch( loss_method )
   {
   case pgsTypes::AASHTO_REFINED:
      ReportRefinedMethod(beamType,girderKey,pChapter,pDisplayUnits,level,laAASHTO);
      break;

   case pgsTypes::WSDOT_REFINED:
      ReportRefinedMethod(beamType,girderKey,pChapter,pDisplayUnits,level,laWSDOT);
      break;

   case pgsTypes::TXDOT_REFINED_2004:
      ReportRefinedMethod(beamType,girderKey,pChapter,pDisplayUnits,level,laTxDOT);
      break;

   case pgsTypes::AASHTO_LUMPSUM:
      ReportApproxLumpSumMethod(beamType,girderKey,pChapter,pDisplayUnits,level,false);
      break;

   case pgsTypes::WSDOT_LUMPSUM:
      ReportApproxLumpSumMethod(beamType,girderKey,pChapter,pDisplayUnits,level,true);
      break;

   case pgsTypes::GENERAL_LUMPSUM:
      ReportGeneralLumpSumMethod(beamType,girderKey,pChapter,pDisplayUnits,true,level);
      break;

   case pgsTypes::TXDOT_REFINED_2013:
      ReportRefinedMethodTxDOT2013(pChapter,beamType,girderKey,pDisplayUnits,level);
      break;

   default:
      CHECK(false); // Should never get here
   }
}

void CPsLossEngineer::ReportRefinedMethod(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,Uint16 level,LossAgency lossAgency)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 ||
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

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 )
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
}

void CPsLossEngineer::LossesByRefinedEstimate(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses,LossAgency lossAgency)
{
   PRECONDITION(pLosses != 0 );

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 ||
        lossAgency==laTxDOT)
   {
      LossesByRefinedEstimateBefore2005(beamType,poi,config,pLosses);
   }
   else
   {
      LossesByRefinedEstimate2005(beamType,poi,config,pLosses,lossAgency);
   }
}

void CPsLossEngineer::LossesByRefinedEstimateBefore2005(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses)
{
   pLosses->LossMethod = pgsTypes::AASHTO_REFINED;

   matPsStrand::Grade grade;
   matPsStrand::Type type;
   lrfdLosses::SectionPropertiesType spType;
   Float64 fpjPerm;
   Float64 fpjTTS;
   Float64 perimeter;
   Float64 Ag;
   Float64 Ig;
   Float64 Ybg;
   Float64 Ac;
   Float64 Ic;
   Float64 Ybc;
   Float64 Volume;
   Float64 SurfaceArea;
   Float64 An;
   Float64 In;
   Float64 Ybn;
   Float64 Acn;
   Float64 Icn;
   Float64 Ybcn;
   Float64 Ad;
   Float64 ed;
   Float64 Ksh;
   Float64 epermRelease;// eccentricity of the permanent strands on the non-composite section
   Float64 epermFinal;
   Float64 etemp;
   Float64 aps;  // area of one prestress strand
   Float64 ApsPerm;
   Float64 ApsTTS;
   Float64 Mdlg;
   Float64 Madlg;
   Float64 Msidl;
   Float64 Mllim;
   Float64 rh;
   Float64 ti,th,td,tf; // initial time, time of hauling,time of deck placment, final time

   Float64 PjS, PjH, PjT;
   StrandIndexType Ns, Nh, Nt;

   Float64 GdrCreepK1, GdrCreepK2, GdrShrinkageK1, GdrShrinkageK2;
   Float64 DeckCreepK1, DeckCreepK2, DeckShrinkageK1, DeckShrinkageK2;

   Float64 fci,fc,fcSlab;
   Float64 Eci,Ec,EcSlab;

   Float64 girder_length, span_length, end_size;

   Float64 Aslab;
   Float64 Pslab;
   lrfdLosses::TempStrandUsage usage;

   Float64 anchorSet,wobble,coeffFriction,angleChange;

   GetLossParameters(poi,config,
                     &spType,
                     &grade, &type, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ig, &Ybg, &Ac, &Ic, &Ybc, &An, &In, &Ybn, &Acn, &Icn, &Ybcn, &Volume, &SurfaceArea, &Ad, &ed, &Ksh,
                     &epermRelease, &epermFinal, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl, &Mllim, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &Ns, &Nh, &Nt,
                     &GdrCreepK1, &GdrCreepK2, &GdrShrinkageK1, &GdrShrinkageK2,
                     &DeckCreepK1, &DeckCreepK2, &DeckShrinkageK1, &DeckShrinkageK2,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,&end_size,
                     &Aslab,&Pslab,&usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();


   // get time to prestress transfer
   GET_IFACE( ISpecification,   pSpec);
   GET_IFACE(ILibrary,pLib);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   Float64 shipping_loss = pSpecEntry->GetShippingLosses();

   boost::shared_ptr<lrfdRefinedLosses> pLoss(new lrfdRefinedLosses(poi.GetDistFromStart(),
                                girder_length, spType,
                                grade,
                                type,
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
                                Msidl,
                                Mllim,

                                Ag,
                                Ig,
                                Ybg,
                                Ac,
                                Ic,
                                Ybc,

                                An,
                                In,
                                Ybn,
                                Acn,
                                Icn,
                                Ybcn,

                                rh,
                                ti,
                                shipping_loss,
                                false
                                ));


   // Any of the "get" methods on lrfdPsLosses can throw an lrfdXPsLosses exception if
   // the input data is bad.  To make sure we have everything correct, lets request
   // the elastic shortening losses and make sure an exception doesn't throw.
   try
   {
      Float64 pES = pLoss->PermanentStrand_ElasticShorteningLosses();
      // store in shared pointer to base class
      pLosses->pLosses = boost::static_pointer_cast<const lrfdLosses>(pLoss);
      ATLASSERT(pLosses->pLosses!=NULL);
   }
   catch( const lrfdXPsLosses& e )
   {
      Int32 reason = XREASON_AGENTVALIDATIONFAILURE;
      std::_tstring msg;

      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      CEAFStatusItem* pStatusItem;

      if ( e.GetReasonCode() == lrfdXPsLosses::fpjOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Prestress losses could not be computed because the prestress jacking stress fpj does not exceed 0.5fpu (see Article 5.9.5.4.4b of LRFD 3rd Edition 2004)\nAdjust the prestress jacking forces");
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,1,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());
      }
      else if ( e.GetReasonCode() == lrfdXPsLosses::fcOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Concrete strength is out of range per LRFD 5.4.2.1 and 5.9.5.1");
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,2,m_StatusGroupID,m_scidGirderDescriptionWarning,msg.c_str());
      }
      else
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Prestress losses could not be computed because an unspecified error occured");
         pStatusItem = new pgsUnknownErrorStatusItem(m_StatusGroupID,m_scidUnknown,_T(__FILE__),__LINE__,msg.c_str());
      }

      pStatusCenter->Add(pStatusItem);

      msg += std::_tstring(_T("\nSee Status Center for Details"));
      THROW_UNWIND(msg.c_str(),reason);
   }
}

void CPsLossEngineer::LossesByRefinedEstimate2005(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses,LossAgency lossAgency)
{
   assert(lossAgency!=laTxDOT); // Did TxDOT change their mind about using the 05 revisions?

   lrfdLosses::SectionPropertiesType spType;
   matPsStrand::Grade grade;
   matPsStrand::Type type;
   Float64 fpjPerm;
   Float64 fpjTTS;
   Float64 perimeter;
   Float64 Ag;
   Float64 Ig;
   Float64 Ybg;
   Float64 Ac;
   Float64 Ic;
   Float64 Ybc;
   Float64 An;
   Float64 In;
   Float64 Ybn;
   Float64 Acn;
   Float64 Icn;
   Float64 Ybcn;
   Float64 Volume;
   Float64 SurfaceArea;
   Float64 Ad;
   Float64 ed;
   Float64 Ksh;
   Float64 epermRelease;// eccentricity of the permanent strands on the non-composite section
   Float64 epermFinal;
   Float64 etemp;
   Float64 aps;  // area of one prestress strand
   Float64 ApsPerm;
   Float64 ApsTTS;
   Float64 Mdlg;
   Float64 Madlg;
   Float64 Msidl;
   Float64 Mllim;
   Float64 rh;
   Float64 ti,th,td,tf; // initial time, time of hauling,time of deck placment, final time

   Float64 PjS, PjH, PjT;
   StrandIndexType Ns, Nh, Nt;

   Float64 GdrCreepK1, GdrCreepK2, GdrShrinkageK1, GdrShrinkageK2;
   Float64 DeckCreepK1, DeckCreepK2, DeckShrinkageK1, DeckShrinkageK2;

   Float64 fci,fc,fcSlab;
   Float64 Eci,Ec,EcSlab;

   Float64 girder_length, span_length, end_size;

   Float64 Aslab;
   Float64 Pslab;
   lrfdLosses::TempStrandUsage usage;

   Float64 anchorSet,wobble,coeffFriction,angleChange;

   GetLossParameters(poi,config,&spType,
                     &grade, &type, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ig, &Ybg, &Ac, &Ic, &Ybc, &An, &In, &Ybn, &Acn, &Icn, &Ybcn, &Volume, &SurfaceArea, &Ad, &ed, &Ksh,
                     &epermRelease, &epermFinal, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl, &Mllim, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &Ns, &Nh, &Nt,
                     &GdrCreepK1, &GdrCreepK2, &GdrShrinkageK1, &GdrShrinkageK2,
                     &DeckCreepK1, &DeckCreepK2, &DeckShrinkageK1, &DeckShrinkageK2,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,&end_size,
                     &Aslab,&Pslab,&usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();


   // get time to prestress transfer
   GET_IFACE( ISpecification,   pSpec);
   GET_IFACE( ILibrary,         pLib);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   if ( lossAgency==laWSDOT )
   {
      pLosses->LossMethod = pgsTypes::WSDOT_REFINED_2005;
   }
   else
   {
      pLosses->LossMethod = pgsTypes::AASHTO_REFINED_2005;
   }

   lrfdRefinedLosses2005::RelaxationLossMethod relaxationMethod = (lrfdRefinedLosses2005::RelaxationLossMethod)pSpecEntry->GetRelaxationLossMethod();
   boost::shared_ptr<lrfdRefinedLosses2005> pLoss(new lrfdRefinedLosses2005(poi.GetDistFromStart(),
                                girder_length,
                                spType,
                                grade,
                                type,
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
                                GdrCreepK1,  GdrCreepK2,  GdrShrinkageK1,  GdrShrinkageK2,
                                DeckCreepK1, DeckCreepK2, DeckShrinkageK1, DeckShrinkageK2,
                                fc,
                                fci,
                                fcSlab,
                                Ec,
                                Eci,
                                EcSlab,
                                Volume, // volume/length
                                SurfaceArea, // surface area/length
                                Aslab, // volume/length for slab
                                Pslab, // surface area/length for slab
                                Ag,
                                Ig,
                                Ybg,
                                Ac,
                                Ic,
                                Ybc,
                                An,
                                In,
                                Ybn,
                                Acn,
                                Icn,
                                Ybcn,
                                Ad,
                                ed,
                                Ksh,
                                Mdlg,
                                Madlg,
                                Msidl,
                                Mllim,
                                rh,
                                ti,
                                th,
                                td,
                                tf,
                                pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient2005::Accelerated : lrfdCreepCoefficient2005::Normal,
                                pSpecEntry->GetCuringMethodTimeAdjustmentFactor(),
                                lossAgency!=laWSDOT, // ignore initial relaxation if not WSDOT
                                false,
                                relaxationMethod));


   // Any of the _T("get") methods on lrfdPsLosses can throw an lrfdXPsLosses exception if
   // the input data is bad.  To make sure we have everything correct, lets request
   // the elastic shortening losses and make sure an exception doesn't throw.
   try
   {
      Float64 pES = pLoss->PermanentStrand_ElasticShorteningLosses();
      // store in shared pointer of base class
      pLosses->pLosses = boost::static_pointer_cast<const lrfdLosses>(pLoss);
      ATLASSERT(pLosses->pLosses!=NULL);
   }
   catch( const lrfdXPsLosses& e )
   {
      Int32 reason = XREASON_AGENTVALIDATIONFAILURE;
      std::_tstring msg;

      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      CEAFStatusItem* pStatusItem;

      if ( e.GetReasonCode() == lrfdXPsLosses::fpjOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Prestress losses could not be computed because the prestress jacking stress fpj does not exceed 0.5fpu (see Article 5.9.5.4.4b of LRFD 3rd Edition 2004)\nAdjust the prestress jacking forces");
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,1,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());
      }
      else if ( e.GetReasonCode() == lrfdXPsLosses::StrandType )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("The relaxation loss of 1.2 ksi can only be used with low relaxation strands (see Article 5.9.5.4.2c)\nChange the strand type or select a different method for computing losses");
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,1,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());
      }
      else if ( e.GetReasonCode() == lrfdXPsLosses::fcOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Concrete strength is out of range per LRFD 5.4.2.1 and 5.9.5.1");
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,2,m_StatusGroupID,m_scidGirderDescriptionWarning,msg.c_str());
      }
      else
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Prestress losses could not be computed because an unspecified error occured");
         pStatusItem = new pgsUnknownErrorStatusItem(m_StatusGroupID,m_scidUnknown,_T(__FILE__),__LINE__,msg.c_str());
      }

      pStatusCenter->Add(pStatusItem);

      msg += std::_tstring(_T("\nSee Status Center for Details"));
      THROW_UNWIND(msg.c_str(),reason);
   }
}

void CPsLossEngineer::LossesByRefinedEstimateTxDOT2013(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses)
{
   pLosses->LossMethod = pgsTypes::TXDOT_REFINED_2013;

   lrfdLosses::SectionPropertiesType spType;
   matPsStrand::Grade grade;
   matPsStrand::Type type;
   Float64 fpjPerm;
   Float64 fpjTTS;
   Float64 perimeter;
   Float64 Ag;
   Float64 Ig;
   Float64 Ybg;
   Float64 Ac;
   Float64 Ic;
   Float64 Ybc;
   Float64 An;
   Float64 In;
   Float64 Ybn;
   Float64 Acn;
   Float64 Icn;
   Float64 Ybcn;
   Float64 Volume;
   Float64 SurfaceArea;
   Float64 Ad;
   Float64 ed;
   Float64 Ksh;
   Float64 epermRelease;// eccentricity of the permanent strands on the non-composite section
   Float64 epermFinal;
   Float64 etemp;
   Float64 aps;  // area of one prestress strand
   Float64 ApsPerm;
   Float64 ApsTTS;
   Float64 Mdlg;
   Float64 Madlg;
   Float64 Msidl;
   Float64 Mllim;
   Float64 rh;
   Float64 ti,th,td,tf; // initial time, time of hauling,time of deck placment, final time

   Float64 PjS, PjH, PjT;
   StrandIndexType Ns, Nh, Nt;

   Float64 GdrCreepK1, GdrCreepK2, GdrShrinkageK1, GdrShrinkageK2;
   Float64 DeckCreepK1, DeckCreepK2, DeckShrinkageK1, DeckShrinkageK2;

   Float64 fci,fc,fcSlab;
   Float64 Eci,Ec,EcSlab;

   Float64 girder_length, span_length, end_size;

   Float64 Aslab;
   Float64 Pslab;
   lrfdLosses::TempStrandUsage usage;

   Float64 anchorSet,wobble,coeffFriction,angleChange;

   GetLossParameters(poi,config,&spType,
                     &grade, &type, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ig, &Ybg, &Ac, &Ic, &Ybc, &An, &In, &Ybn, &Acn, &Icn, &Ybcn, &Volume, &SurfaceArea, &Ad, &ed, &Ksh,
                     &epermRelease, &epermFinal, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl, &Mllim, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &Ns, &Nh, &Nt,
                     &GdrCreepK1, &GdrCreepK2, &GdrShrinkageK1, &GdrShrinkageK2,
                     &DeckCreepK1, &DeckCreepK2, &DeckShrinkageK1, &DeckShrinkageK2,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,&end_size,
                     &Aslab,&Pslab,&usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE( ISpecification,   pSpec);
   GET_IFACE(ILibrary,pLib);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   Float64 shipping_loss = pSpecEntry->GetShippingLosses();

   // fcgp Computation method - Elastic shortening
   Int16 fcgp_method = pSpecEntry->GetFcgpComputationMethod();

   lrfdElasticShortening::FcgpComputationMethod method;
   if (fcgp_method == FCGP_07FPU)
   {
      method = lrfdElasticShortening::fcgp07Fpu;
   }
   else if (fcgp_method == FCGP_ITERATIVE)
   {
      method = lrfdElasticShortening::fcgpIterative;
   }
   else
   {
      ATLASSERT(fcgp_method == FCGP_HYBRID);
      // Use 0.7Fpu method to compute Fcgp if permanent strands are jacked to 0.75*Fpu and,
      // no temp strands exist, otherwise use iterative method
      method = lrfdElasticShortening::fcgpIterative;
      if ( 0.0 <= ApsPerm && IsEqual(ApsTTS, 0.0) )
      {
         Float64 Fpu = lrfdPsStrand::GetUltimateStrength( grade );

         if (ApsPerm==0.0 || IsEqual(Fpu*0.75, fpjPerm, 1000.0)) // Pa's are very small
         {
            method = lrfdElasticShortening::fcgp07Fpu;
         }
      }
   }

   boost::shared_ptr<lrfdRefinedLossesTxDOT2013> pLoss(new lrfdRefinedLossesTxDOT2013( poi.GetDistFromStart(),
                                                  girder_length,
                                                  spType,
                                                  grade,
                                                  type,
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
                                                  Msidl,
                                                  Mllim,
                                                  Ag,
                                                  Ig,
                                                  Ybg,
                                                  Ac,
                                                  Ic,
                                                  Ybc,
                                                  An,
                                                  In,
                                                  Ybn,
                                                  Acn,
                                                  Icn,
                                                  Ybcn,
                                                  rh,
                                                  ti,
                                                  shipping_loss,
                                                  method,
                                                  false));

   // Any of the "get" methods can throw an lrfdXPsLosses exception if
   // the input data is bad.  To make sure we have everything correct, lets request
   // the elastic shortening losses and make sure an exception doesn't throw.
   try
   {
      Float64 pES = pLoss->PermanentStrand_ElasticShorteningLosses();
      // store in shared pointer to base class
      pLosses->pLosses = boost::static_pointer_cast<const lrfdLosses>(pLoss);
      ATLASSERT(pLosses->pLosses!=NULL);

      if(fcgp_method == FCGP_HYBRID && 
         pLoss->ElasticShortening().GetFcgpComputationMethod() == lrfdElasticShortening::fcgpIterative)
      {
         // Elastic shortening loss method switches to iterative solution if jacking stress is not
         // equal to 0.75Fpu. Let user know if this happened.
         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         std::_tstring msg = _T("The Jacking stress is not equal to 0.75Fpu, or temporary strands exist. Hence, for calculation of elastic shortening, an iterative solution was used to find Fcgp after release rather than assuming 0.7*Fpu.");
         CEAFStatusItem* pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,1,m_StatusGroupID,m_scidGirderDescriptionWarning,msg.c_str());
         pStatusCenter->Add(pStatusItem);
      }
   }
   catch( const lrfdXPsLosses& e )
   {
      Int32 reason = XREASON_AGENTVALIDATIONFAILURE;
      std::_tstring msg;

      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      CEAFStatusItem* pStatusItem;

      if ( e.GetReasonCode() == lrfdXPsLosses::fpjOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Prestress losses could not be computed because the prestress jacking stress fpj does not exceed 0.5fpu (see Article 5.9.5.4.4b of LRFD 3rd Edition 2004)\nAdjust the prestress jacking forces");
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,1,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());
      }
      else if ( e.GetReasonCode() == lrfdXPsLosses::fcOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Concrete strength is out of range per LRFD 5.4.2.1 and 5.9.5.1");
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,2,m_StatusGroupID,m_scidGirderDescriptionWarning,msg.c_str());
      }
      else
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Prestress losses could not be computed because an unspecified error occured");
         pStatusItem = new pgsUnknownErrorStatusItem(m_StatusGroupID,m_scidUnknown,_T(__FILE__),__LINE__,msg.c_str());
      }

      pStatusCenter->Add(pStatusItem);

      msg += std::_tstring(_T("\nSee Status Center for Details"));
      THROW_UNWIND(msg.c_str(),reason);
   }
}

void CPsLossEngineer::LossesByApproxLumpSum(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses,bool isWsdot)
{
   PRECONDITION(pLosses != 0 );

   lrfdLosses::SectionPropertiesType spType;
   matPsStrand::Grade grade;
   matPsStrand::Type type;
   Float64 fpjPerm;
   Float64 fpjTTS;
   Float64 perimeter;
   Float64 Ag;
   Float64 Ig;
   Float64 Ybg;
   Float64 Ac;
   Float64 Ic;
   Float64 Ybc;
   Float64 An;
   Float64 In;
   Float64 Ybn;
   Float64 Acn;
   Float64 Icn;
   Float64 Ybcn;
   Float64 Volume;
   Float64 SurfaceArea;
   Float64 Ad;
   Float64 ed;
   Float64 Ksh;
   Float64 epermRelease;// eccentricity of the permanent strands on the non-composite section
   Float64 epermFinal;
   Float64 etemp;
   Float64 aps;  // area of one prestress strand
   Float64 ApsPerm;
   Float64 ApsTTS;
   Float64 Mdlg;
   Float64 Madlg;
   Float64 Msidl;
   Float64 Mllim;
   Float64 rh;
   Float64 ti,th,td,tf; // initial time, time of hauling,time of deck placment, final time

   Float64 PjS, PjH, PjT;
   StrandIndexType Ns, Nh, Nt;

   Float64 GdrCreepK1, GdrCreepK2, GdrShrinkageK1, GdrShrinkageK2;
   Float64 DeckCreepK1, DeckCreepK2, DeckShrinkageK1, DeckShrinkageK2;

   Float64 fci,fc,fcSlab;
   Float64 Eci,Ec,EcSlab;

   Float64 girder_length, span_length, end_size;

   Float64 Aslab;
   Float64 Pslab;
   lrfdLosses::TempStrandUsage usage;

   Float64 anchorSet,wobble,coeffFriction,angleChange;

   GET_IFACE(IMaterials,pMaterial);
   pgsTypes::ConcreteType girderConcreteType = pMaterial->GetSegmentConcreteType(poi.GetSegmentKey());
   pgsTypes::ConcreteType slabConcreteType   = pMaterial->GetDeckConcreteType();

   if ( girderConcreteType != pgsTypes::Normal || slabConcreteType != pgsTypes::Normal )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      std::_tstring msg(_T("The approximate estimate of time-dependent losses given in LRFD 5.9.5.3 is for members made from normal-weight concrete"));
      pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidConcreteTypeError,msg.c_str());
      pStatusCenter->Add(pStatusItem);

      msg += std::_tstring(_T("\nSee Status Center for Details"));
      THROW_UNWIND(msg.c_str(),XREASON_LRFD_VERSION);
   }

   GetLossParameters(poi,config,&spType,
                     &grade, &type, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ig, &Ybg, &Ac, &Ic, &Ybc, &An, &In, &Ybn, &Acn, &Icn, &Ybcn, &Volume, &SurfaceArea, &Ad, &ed, &Ksh,
                     &epermRelease, &epermFinal, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl, &Mllim, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &Ns, &Nh, &Nt,
                     &GdrCreepK1, &GdrCreepK2, &GdrShrinkageK1, &GdrShrinkageK2,
                     &DeckCreepK1, &DeckCreepK2, &DeckShrinkageK1, &DeckShrinkageK2,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,&end_size,
                     &Aslab,&Pslab,&usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();


   // get time to prestress transfer
   GET_IFACE( ISpecification,   pSpec);
   GET_IFACE( ILibrary,         pLib);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   try
   {
      if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 )
      {
         // partial prestressing ratio=1 for wsdot method
         Float64 ppr;
         if (isWsdot)
         {
            ppr = 1.0;
            pLosses->LossMethod = pgsTypes::WSDOT_LUMPSUM;
         }
         else
         {
            GET_IFACE(ILongRebarGeometry,pLongRebarGeom);
            ppr = pLongRebarGeom->GetPPRBottomHalf(poi);
            pLosses->LossMethod = pgsTypes::AASHTO_LUMPSUM;
         }

         Float64 shipping_loss = pSpecEntry->GetShippingLosses();

         boost::shared_ptr<lrfdApproximateLosses> pLoss(new lrfdApproximateLosses(
                            (lrfdApproximateLosses::BeamType)beamType,
                            shipping_loss,
                            ppr,
                            poi.GetDistFromStart(), // location along girder where losses are computed
                            girder_length,    // girder length
                            spType,
                            grade,
                            type,
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

                            (lrfdConcreteUtil::DensityType)config.ConcType,
                            fc,
                            fci,
                            fcSlab,
                            Ec,   // Modulus of elasticity of girder
                            Eci,  // Modulus of elasticity of girder at transfer
                            EcSlab,  // Modulus of elasticity of deck

                            Mdlg,  // Dead load moment of girder only
                            Madlg,  // Additional dead load on girder section
                            Msidl, // Superimposed dead loads
                            Mllim,
                         
                            Ag,
                            Ig,
                            Ybg,
                            Ac,
                            Ic,
                            Ybc,
                         
                            An,
                            In,
                            Ybn,
                            Acn,
                            Icn,
                            Ybcn,

                            rh,      // relative humidity
                            ti,   // Time until prestress transfer
                            !isWsdot,false));

         // Any of the "get" methods on lrfdPsLosses can throw an lrfdXPsLosses exception if
         // the input data is bad.  To make sure we have everything correct, lets request
         // the elastic shortening losses and make sure an exception doesn't throw.
         Float64 pES = pLoss->PermanentStrand_ElasticShorteningLosses();
         // store in shared pointer to base class
         pLosses->pLosses = boost::static_pointer_cast<const lrfdLosses>(pLoss);
         ATLASSERT(pLosses->pLosses!=NULL);
      }
      else
      {
         // 3rd edition /w 2005 interims and later

         // LRFD 5th Edition, 2010, C5.9.5.3
         // The approximate estimates of time-dependent prestress losses given in Eq 5.9.5.3-1 are intended for sections with composite decks only
         GET_IFACE(IBridge,pBridge);
         if ( lrfdVersionMgr::FifthEdition2010 <= lrfdVersionMgr::GetVersion() && !pBridge->IsCompositeDeck() )
         {
            GET_IFACE(IEAFStatusCenter,pStatusCenter);
            std::_tstring msg(_T("The approximate estimates of time-dependent prestress losses given in Eq 5.9.5.3-1 are intended for sections with composite decks only."));
            pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidLRFDVersionError,msg.c_str());
            pStatusCenter->Add(pStatusItem);

            msg += std::_tstring(_T("\nSee Status Center for Details"));
            THROW_UNWIND(msg.c_str(),XREASON_LRFD_VERSION);
         }

         boost::shared_ptr<lrfdApproximateLosses2005> pLoss(new lrfdApproximateLosses2005(poi.GetDistFromStart(), // location along girder where losses are computed
                            girder_length,    // girder length
                            spType,
                            grade,
                            type,
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
                            Msidl, // Superimposed dead loads
                            Mllim,
                         
                            Ag,
                            Ig,
                            Ybg,
                            Ac,
                            Ic,
                            Ybc,
                         
                            An,
                            In,
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
            pLosses->LossMethod = pgsTypes::WSDOT_LUMPSUM_2005;
         }
         else
         {
            pLosses->LossMethod = pgsTypes::AASHTO_LUMPSUM_2005;
         }

         // Any of the "get" methods on lrfdPsLosses can throw an lrfdXPsLosses exception if
         // the input data is bad.  To make sure we have everything correct, lets request
         // the elastic shortening losses and make sure an exception doesn't throw.
         Float64 pES = pLoss->PermanentStrand_ElasticShorteningLosses();
         // store in shared pointer to base class
         pLosses->pLosses = boost::static_pointer_cast<const lrfdLosses>(pLoss);
         ATLASSERT(pLosses->pLosses!=NULL);
      }

   } // end of try block
   catch( const lrfdXPsLosses& e )
   {
      Int32 reason = XREASON_AGENTVALIDATIONFAILURE;
      std::_tstring msg;

      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      CEAFStatusItem* pStatusItem;

      if ( e.GetReasonCode() == lrfdXPsLosses::fpjOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Prestress losses could not be computed because the prestress jacking stress fpj does not exceed 0.5fpu (see Article 5.9.5.4.4b of LRFD 3rd Edition 2004)\nAdjust the prestress jacking forces");
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,1,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());
      }
      else if ( e.GetReasonCode() == lrfdXPsLosses::fcOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Concrete strength is out of range per LRFD 5.4.2.1 and 5.9.5.1");
         pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,0,m_StatusGroupID,m_scidGirderDescriptionWarning,msg.c_str());
      }
      else
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Prestress losses could not be computed because an unspecified error occured");
         pStatusItem = new pgsUnknownErrorStatusItem(m_StatusGroupID,m_scidUnknown,_T(__FILE__),__LINE__,msg.c_str());
      }

      pStatusCenter->Add(pStatusItem);

      msg += std::_tstring(_T("\nSee Status Center for Details"));
      THROW_UNWIND(msg.c_str(),reason);
   }
}

void CPsLossEngineer::LossesByGeneralLumpSum(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses)
{
   // Need the following parameters for the lump sum loss object: ApsPerm,ApsTTS,fpjPerm,fpjTTS,usage
   // It is easier to call the general GetLossParameters method and get everything this
   // to create yet another function to get just those parameters we need.
   PRECONDITION(pLosses != 0 );
   lrfdLosses::SectionPropertiesType spType;
   matPsStrand::Grade grade;
   matPsStrand::Type type;
   Float64 fpjPerm;
   Float64 fpjTTS;
   Float64 perimeter;
   Float64 Ag;
   Float64 Ig;
   Float64 Ybg;
   Float64 Ac;
   Float64 Ic;
   Float64 Ybc;
   Float64 An;
   Float64 In;
   Float64 Ybn;
   Float64 Acn;
   Float64 Icn;
   Float64 Ybcn;
   Float64 Volume;
   Float64 SurfaceArea;
   Float64 Ad;
   Float64 ed;
   Float64 Ksh;
   Float64 epermRelease;// eccentricity of the permanent strands on the non-composite section
   Float64 epermFinal;
   Float64 etemp;
   Float64 aps;  // area of one prestress strand
   Float64 ApsPerm;
   Float64 ApsTTS;
   Float64 Mdlg;
   Float64 Madlg;
   Float64 Msidl;
   Float64 Mllim;
   Float64 rh;
   Float64 ti,th,td,tf; // initial time, time of hauling,time of deck placment, final time

   Float64 PjS, PjH, PjT;
   StrandIndexType Ns, Nh, Nt;

   Float64 GdrCreepK1, GdrCreepK2, GdrShrinkageK1, GdrShrinkageK2;
   Float64 DeckCreepK1, DeckCreepK2, DeckShrinkageK1, DeckShrinkageK2;

   Float64 fci,fc,fcSlab;
   Float64 Eci,Ec,EcSlab;

   Float64 girder_length, span_length, end_size;

   Float64 Aslab;
   Float64 Pslab;
   lrfdLosses::TempStrandUsage usage;

   Float64 anchorSet,wobble,coeffFriction,angleChange;

   GetLossParameters(poi,config,
                     &spType,
                     &grade, &type, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ig, &Ybg, &Ac, &Ic, &Ybc, &An, &In, &Ybn, &Acn, &Icn, &Ybcn, &Volume, &SurfaceArea, &Ad, &ed, &Ksh,
                     &epermRelease, &epermFinal, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl, &Mllim, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &Ns, &Nh, &Nt,
                     &GdrCreepK1, &GdrCreepK2, &GdrShrinkageK1, &GdrShrinkageK2,
                     &DeckCreepK1, &DeckCreepK2, &DeckShrinkageK1, &DeckShrinkageK2,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,&end_size,
                     &Aslab,&Pslab,&usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   pLosses->LossMethod = pgsTypes::GENERAL_LUMPSUM;

   // If there are no strands then there can't be any losses
   StrandIndexType Nstrands = Ns + Nh + Nt;

   if ( Nstrands == 0 )
   {
      boost::shared_ptr<const lrfdLumpSumLosses> pLoss (new lrfdLumpSumLosses(0,0,0,0,usage,0,0,0,0,0,0,0,0,0));

      pLosses->pLosses = boost::static_pointer_cast<const lrfdLosses>(pLoss);
      ATLASSERT(pLosses->pLosses!=NULL);
   }
   else
   {
      GET_IFACE(ILossParameters,pLossParameters);

      boost::shared_ptr<const lrfdLumpSumLosses> pLoss(new lrfdLumpSumLosses(ApsPerm,ApsTTS,fpjPerm,fpjTTS,usage,
                                           pLossParameters->GetBeforeXferLosses(),
                                           pLossParameters->GetAfterXferLosses(),
                                           pLossParameters->GetLiftingLosses(),
                                           pLossParameters->GetShippingLosses(),
                                           pLossParameters->GetBeforeTempStrandRemovalLosses(),
                                           pLossParameters->GetAfterTempStrandRemovalLosses(),
                                           pLossParameters->GetAfterDeckPlacementLosses(),
                                           pLossParameters->GetAfterSIDLLosses(),
                                           pLossParameters->GetFinalLosses()));

      pLosses->pLosses = boost::static_pointer_cast<const lrfdLosses>(pLoss);
      ATLASSERT(pLosses->pLosses!=NULL);
   }
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

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE(ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Refined Estimate of Time-Dependent Losses [5.9.5.4]") << rptNewLine;

   //GET_IFACE(IPointOfInterest,pIPoi);
   //std::vector<pgsPointOfInterest> bsPoi( pIPoi->GetPointsOfInterest( segmentKey ) );
   //std::vector<pgsPointOfInterest> cyPoi( pIPoi->GetPointsOfInterest( segmentKey, POI_PICKPOINT, POIFIND_OR) );

   //GET_IFACE(IIntervals,pIntervals);
   //IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(segmentKey);
   //IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   //PoiSet sPoi;
   //std::vector<pgsPointOfInterest>::iterator bsIter(bsPoi.begin());
   //std::vector<pgsPointOfInterest>::iterator bsIterEnd(bsPoi.end());
   //for ( ; bsIter != bsIterEnd; bsIter++ )
   //{
   //   sPoi.insert( std::make_pair(*bsIter,liveLoadIntervalIdx) );
   //}

   //std::vector<pgsPointOfInterest>::iterator cyIter(cyPoi.begin());
   //std::vector<pgsPointOfInterest>::iterator cyIterEnd(cyPoi.end());
   //for ( ; cyIter != cyIterEnd; cyIter++ )
   //{
   //   sPoi.insert( std::make_pair(*cyIter,releaseIntervalIdx) );
   //}

   //GET_IFACE(ILosses,pILosses);
   //const LOSSDETAILS* pDetails = pILosses->GetLossDetails( cyPoi[0] );

   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(segmentKey) );

   GET_IFACE(ILosses,pILosses);
   const LOSSDETAILS* pDetails = pILosses->GetLossDetails( vPoi[0] );

   GET_IFACE(IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance( segmentKey );

   // Do some preliminary setup for the tables.
   INIT_UV_PROTOTYPE( rptForceUnitValue,   force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,        pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  ecc,         pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cg,          pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,  moment,      pDisplayUnits->GetMomentUnit(),          false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  mod_e,       pDisplayUnits->GetModEUnit(),            false );

   bool bTemporaryStrands = ( 0 < Nt && pStrands->TempStrandUsage == pgsTypes::ttsPretensioned ? true : false);

   // Relaxation At Prestress Transfer
   ReportInitialRelaxation(pChapter,bTemporaryStrands,pDetails->pLosses.get(),pDisplayUnits,level);

   CElasticShorteningTable*                     pES  = CElasticShorteningTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDetails,pDisplayUnits,level);
   CFrictionLossTable*                          pFR  = NULL;
   CPostTensionInteractionTable*                pPTT = NULL;
   CEffectOfPostTensionedTemporaryStrandsTable* pPTP = NULL;
   
   if ( 0 < Nt && pStrands->TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   CTimeDependentLossesAtShippingTable*            pPSH = NULL;
   CPostTensionTimeDependentLossesAtShippingTable* pPTH = NULL;

   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if ( pGirderHaulingSpecCriteria->IsHaulingAnalysisEnabled() )
   {
      pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDisplayUnits,level);

      if ( 0 < Nt ) 
         pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   CTemporaryStrandRemovalTable*        pPTR = NULL;
   if (0 < Nt )
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Time dependent losses") << rptNewLine;

   CChangeOfConcreteStressTable*   pDeltaFcdp = CChangeOfConcreteStressTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CCreepAndShrinkageTable*        pCR        = CCreepAndShrinkageTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CRelaxationAfterTransferTable*  pR2        = CRelaxationAfterTransferTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CFinalPrestressLossTable*       pT         = CFinalPrestressLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);

   pgsPointOfInterest prev_poi(CSegmentKey(0,0,0),0.0);
   bool bSkipToNextRow = false;
   RowIndexType row1 = 1;
   RowIndexType row2 = 1;
   //PoiSet::iterator siter(sPoi.begin());
   //PoiSet::iterator sIterEnd(sPoi.end());
   //for ( ; siter != sIterEnd; siter++ )
   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      bSkipToNextRow = false;

      //pgsPointOfInterest& poi = (*siter).first;
      //IntervalIndexType intervalIdx = (*siter).second;
      pgsPointOfInterest& poi = *iter;

      if ( row1 != 1 && IsEqual(prev_poi.GetDistFromStart(),poi.GetDistFromStart()) )
      {
         row1--;
         bSkipToNextRow = true;
      }

      // write the location information into the tables
      ReportLocation2(pES,  row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pFR,  row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPTT, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPTP, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPSH, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPTH, row1, poi, end_size, pDisplayUnits);

      //if ( intervalIdx == liveLoadIntervalIdx )
      {
         ReportLocation(pPTR,       row2, poi, end_size, pDisplayUnits);
         ReportLocation(pDeltaFcdp, row2, poi, end_size, pDisplayUnits);
         ReportLocation(pCR,        row2, poi, end_size, pDisplayUnits);
         ReportLocation(pR2,        row2, poi, end_size, pDisplayUnits);
         ReportLocation(pT,         row2, poi, end_size, pDisplayUnits);
      }

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

      //if ( intervalIdx == liveLoadIntervalIdx )
      {
         ReportRow(pPTR,      pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
         ReportRow(pDeltaFcdp,pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
         ReportRow(pCR,       pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
         ReportRow(pR2,       pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
         ReportRow(pT,        pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);

         row2++;
      }
      
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

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   Float64 end_size = pBridge->GetSegmentStartEndDistance( segmentKey );

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Refined Estimate of Time-Dependent Losses [5.9.5.4]") << rptNewLine;

   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(segmentKey) );

   GET_IFACE(ILosses,pILosses);
   const LOSSDETAILS* pDetails = pILosses->GetLossDetails( vPoi[0] );

   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);

   GET_IFACE(ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   bool bTemporaryStrands = ( 0 < Nt && pStrands->TempStrandUsage == pgsTypes::ttsPretensioned ? true : false);

   ReportInitialRelaxation(pChapter,bTemporaryStrands,pDetails->pLosses.get(),pDisplayUnits,level);

   ////////////////////////////////////////////////////////////////////////////////////////
   // Create the tables for losses - order is important here... The factory methods
   // put content into the chapter and return a table that needs to be filled up
   CElasticShorteningTable*                     pES  = CElasticShorteningTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDetails,pDisplayUnits,level);
   CFrictionLossTable*                          pFR  = NULL;
   CPostTensionInteractionTable*                pPTT = NULL;
   CEffectOfPostTensionedTemporaryStrandsTable* pPTP = NULL;

   

   if ( 0 < Nt && pStrands->TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   CShrinkageAtHaulingTable*                       pSRH = NULL;
   CCreepAtHaulingTable*                           pCRH = NULL;
   CRelaxationAtHaulingTable*                      pR1H = NULL;
   CTimeDependentLossesAtShippingTable*            pPSH = NULL;
   
   CPostTensionTimeDependentLossesAtShippingTable* pPTH  = NULL;

   // must report this even if not checking hauling because
   // temporary strand removal effects depend on losses at end of hauling stage
	pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
	*pChapter << pParagraph;
	*pParagraph << _T("Time dependent losses between transfer and hauling [5.9.5.4.2]") << rptNewLine << rptNewLine;
	
	pSRH = CShrinkageAtHaulingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDetails,pDisplayUnits,level);
	pCRH = CCreepAtHaulingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDetails,pDisplayUnits,level);
	pR1H = CRelaxationAtHaulingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDetails,pDisplayUnits,level);
	pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDisplayUnits,level);
	pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);


   ///////////////////////////////////////////////////////////////////////////////////////////
   // Time-dependent losses between transfer and deck placement
   ///////////////////////////////////////////////////////////////////////////////////////////

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   if ( pBridge->IsCompositeDeck() )
      *pParagraph << _T("Time dependent losses between transfer and deck placement [5.9.5.4.2]") << rptNewLine;
   else
      *pParagraph << _T("Time dependent losses between transfer and installation of precast members [5.9.5.4.2, 5.9.5.4.4]") << rptNewLine;

   CShrinkageAtDeckPlacementTable*      pSR = CShrinkageAtDeckPlacementTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CCreepAtDeckPlacementTable*          pCR = CCreepAtDeckPlacementTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CRelaxationAtDeckPlacementTable*     pR1 = CRelaxationAtDeckPlacementTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CTimeDependentLossesAtDeckPlacementTable* pLTid = CTimeDependentLossesAtDeckPlacementTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CTemporaryStrandRemovalTable*        pPTR = NULL;
   
   if ( 0 < Nt )
   {
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   ///////////////////////////////////////////////////////////////////////////////////////////
   // Time-dependent losses between deck placement and final time
   ///////////////////////////////////////////////////////////////////////////////////////////

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   if ( pBridge->IsCompositeDeck() )
      *pParagraph << _T("Losses: Time of Deck Placement to Final Time [5.9.5.4.3]") << rptNewLine;
   else
      *pParagraph << _T("Losses: Time of Installation of Precast Members to Final Time [5.9.5.4.3, 5.9.5.4.4]") << rptNewLine;
   
   CShrinkageAtFinalTable*      pSD = CShrinkageAtFinalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CCreepAtFinalTable*          pCD = CCreepAtFinalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CRelaxationAtFinalTable*     pR2 = CRelaxationAtFinalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);

   CElasticGainDueToDeckPlacementTable*   pED   = CElasticGainDueToDeckPlacementTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CElasticGainDueToSIDLTable*            pSIDL = CElasticGainDueToSIDLTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CElasticGainDueToLiveLoadTable*        pLLIM = CElasticGainDueToLiveLoadTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CElasticGainDueToDeckShrinkageTable*   pSS   = CElasticGainDueToDeckShrinkageTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);

   CTimeDependentLossFinalTable* pLTdf = CTimeDependentLossFinalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CTimeDependentLossesTable*    pLT   = CTimeDependentLossesTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);

   CEffectivePrestressTable*     pPE   = CEffectivePrestressTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CTotalPrestressLossTable*     pT    = CTotalPrestressLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);

   ///////////////////////////////////////////////////////////////////////
   // Loop over all the POIs and populate the tables with loss information
   pgsPointOfInterest prev_poi(CSegmentKey(0,0,0),0.00);
   bool bSkipToNextRow = false;
   RowIndexType row1 = 1;
   RowIndexType row2 = 1;
   std::vector<pgsPointOfInterest>::iterator sIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator sIterEnd(vPoi.end());
   for ( ; sIter != sIterEnd; sIter++ )
   {
      bSkipToNextRow = false;

      pgsPointOfInterest& poi = (*sIter);

      if ( row1 != 1 && IsEqual(prev_poi.GetDistFromStart(),poi.GetDistFromStart()) )
      {
         row1--;
         bSkipToNextRow = true;
      }

      // write the location information into the tables
      ReportLocation2(pES,  row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pFR,  row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPTT, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPTP, row1, poi, end_size, pDisplayUnits); 

      ReportLocation2(pSRH, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pCRH, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pR1H, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPSH, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPTH, row1, poi, end_size, pDisplayUnits);

      ReportLocation(pSR,   row2, poi, end_size, pDisplayUnits);
      ReportLocation(pCR,   row2, poi, end_size, pDisplayUnits);
      ReportLocation(pR1,   row2, poi, end_size, pDisplayUnits);
      ReportLocation(pLTid, row2, poi, end_size, pDisplayUnits);
      ReportLocation(pPTR,  row2, poi, end_size, pDisplayUnits);
      ReportLocation(pSD,   row2, poi, end_size, pDisplayUnits);
      ReportLocation(pCD,   row2, poi, end_size, pDisplayUnits);
      ReportLocation(pR2,   row2, poi, end_size, pDisplayUnits);
      ReportLocation(pSS,   row2, poi, end_size, pDisplayUnits);
      ReportLocation(pLTdf, row2, poi, end_size, pDisplayUnits);
      ReportLocation(pLT,   row2, poi, end_size, pDisplayUnits);
      ReportLocation(pED,   row2, poi, end_size, pDisplayUnits);
      ReportLocation(pSIDL, row2, poi, end_size, pDisplayUnits);
      ReportLocation(pLLIM, row2, poi, end_size, pDisplayUnits);
      ReportLocation(pPE,   row2, poi, end_size, pDisplayUnits);
      ReportLocation(pT,    row2, poi, end_size, pDisplayUnits);

      // fill each row1 with data
      if ( !bSkipToNextRow )
      {
         pDetails = pILosses->GetLossDetails( poi );

         ReportRow(pES, pChapter,m_pBroker,poi,row1,pDetails,pDisplayUnits,level);
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
      ReportRow(pED,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pSIDL,pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pLLIM,pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pSS,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);

      ReportRow(pLTdf,pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pLT,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
      ReportRow(pT,   pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);

      ReportRow(pPE,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);

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

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Refined Estimate of Time-Dependent Losses Per TxDOT Research Report 0-6374-2") << rptNewLine;

   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(segmentKey) );

   GET_IFACE(ILosses,pILosses);
   const LOSSDETAILS* pDetails = pILosses->GetLossDetails( vPoi[0] );

   GET_IFACE(IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance( segmentKey );

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
   bool bTemporaryStrands = ( 0 < Nt && pStrands->TempStrandUsage == pgsTypes::ttsPretensioned ? true : false);

   // NOTE: The order of everything from here to the loop is important. 
   // It is the order in which things are put into the report
   ReportInitialRelaxation(pChapter,bTemporaryStrands,pDetails->pLosses.get(),pDisplayUnits,level);

   CElasticShorteningTable*                     pES  = CElasticShorteningTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,
                                                                                             pDetails,pDisplayUnits,level);
   CFrictionLossTable*                          pFR  = NULL;
   CPostTensionInteractionTable*                pPTT = NULL;
   CEffectOfPostTensionedTemporaryStrandsTable* pPTP = NULL;
   
   if ( 0 < Nt && pStrands->TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   CTimeDependentLossesAtShippingTable*            pPSH = NULL;
   CPostTensionTimeDependentLossesAtShippingTable* pPTH = NULL;

   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if ( pGirderHaulingSpecCriteria->IsHaulingAnalysisEnabled() )
   {
      pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDisplayUnits,level);

      if ( 0 < Nt ) 
         pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   CTemporaryStrandRemovalTable* pPTR = NULL;
   if (0 < Nt )
   {
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Time dependent losses") << rptNewLine;

   CChangeOfConcreteStressTable*            pDeltaFcdp = CChangeOfConcreteStressTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CTxDOT2013CreepAndShrinkageTable*        pCR        = CTxDOT2013CreepAndShrinkageTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CTxDOT2013RelaxationAfterTransferTable*  pR2        = CTxDOT2013RelaxationAfterTransferTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
   CTxDOT2013TimeDependentLossesTable*      pLT        = CTxDOT2013TimeDependentLossesTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   CTotalPrestressLossTable*                pT         = CTotalPrestressLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);

   pgsPointOfInterest prev_poi(CSegmentKey(0,0,0),0.00);
   bool bSkipToNextRow = false;
   RowIndexType row1 = 1;
   RowIndexType row2 = 1;
   std::vector<pgsPointOfInterest>::iterator sIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator sIterEnd(vPoi.end());
   for ( ; sIter != sIterEnd; sIter++ )
   {
      bSkipToNextRow = false;

      pgsPointOfInterest& poi = (*sIter);

      if ( row1 != 1 && IsEqual(prev_poi.GetDistFromStart(),poi.GetDistFromStart()) )
      {
         row1--;
         bSkipToNextRow = true;
      }

      // write the location information into the tables
      ReportLocation2(pES,  row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pFR,  row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPTT, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPTP, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPSH, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPTH, row1, poi, end_size, pDisplayUnits);

      ReportLocation(pPTR,       row2, poi, end_size, pDisplayUnits);
      ReportLocation(pDeltaFcdp, row2, poi, end_size, pDisplayUnits);
      ReportLocation(pCR,        row2, poi, end_size, pDisplayUnits);
      ReportLocation(pR2,        row2, poi, end_size, pDisplayUnits);
      ReportLocation(pLT,        row2, poi, end_size, pDisplayUnits);
      ReportLocation(pT,         row2, poi, end_size, pDisplayUnits);

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

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE(IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance( segmentKey );

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);

   GET_IFACE(ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   //GET_IFACE(IPointOfInterest,pIPoi);
   //std::vector<pgsPointOfInterest> bsPoi( pIPoi->GetPointsOfInterest(segmentKey) );

   //GET_IFACE(IGirderLiftingPointsOfInterest,pLiftingPoi);
   //std::vector<pgsPointOfInterest> cyPoi( pLiftingPoi->GetLiftingPointsOfInterest( segmentKey, POI_PICKPOINT, POIFIND_OR ) );

   //GET_IFACE(IIntervals,pIntervals);
   //IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(segmentKey);
   //IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   //PoiSet sPoi;
   //std::vector<pgsPointOfInterest>::iterator bsIter(bsPoi.begin());
   //std::vector<pgsPointOfInterest>::iterator bsIterEnd(bsPoi.end());
   //for ( ; bsIter != bsIterEnd; bsIter++ )
   //{
   //   sPoi.insert( std::make_pair(*bsIter,liveLoadIntervalIdx) );
   //}

   //std::vector<pgsPointOfInterest>::iterator cyIter(cyPoi.begin());
   //std::vector<pgsPointOfInterest>::iterator cyIterEnd(cyPoi.end());
   //for ( ; cyIter != cyIterEnd; cyIter++ )
   //{
   //   sPoi.insert( std::make_pair(*cyIter,releaseIntervalIdx) );
   //}

   //GET_IFACE(ILosses,pILosses);
   //const LOSSDETAILS* pDetails = pILosses->GetLossDetails( cyPoi[0] );

   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(segmentKey) );

   GET_IFACE(ILosses,pILosses);
   const LOSSDETAILS* pDetails = pILosses->GetLossDetails( vPoi[0] );

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   if (isWsdot)
      *pParagraph << _T("Approximate Lump Sum Estimate of Time-Dependent Losses [WSDOT BDM 6.1.5.B3]") << rptNewLine;
   else
      *pParagraph << _T("Approximate Lump Sum Estimate of Time-Dependent Losses [5.9.5.3]") << rptNewLine;

   bool bTemporaryStrands = ( 0 < Nt && pStrands->TempStrandUsage == pgsTypes::ttsPretensioned ? true : false);

   //////////////////////////////////////////////////////////
   // NOTE: The order of everything from here to the loop is important. 
   // It is the order in which things are put into the report
   ReportInitialRelaxation(pChapter,bTemporaryStrands,pDetails->pLosses.get(),pDisplayUnits,level);

   CElasticShorteningTable*                        pES  = CElasticShorteningTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDetails,pDisplayUnits,level);

   ReportLumpSumTimeDependentLossesAtShipping(pChapter,pDetails,pDisplayUnits,level);

   CFrictionLossTable*                             pFR  = NULL;
   CPostTensionInteractionTable*                   pPTT = NULL;
   CEffectOfPostTensionedTemporaryStrandsTable*    pPTP = NULL;

   if ( 0 < Nt && pStrands->TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   CTimeDependentLossesAtShippingTable*            pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDisplayUnits,level);
   CPostTensionTimeDependentLossesAtShippingTable* pPTH  = NULL;
   
   if ( 0 < Nt )
      pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);

   CTemporaryStrandRemovalTable*                   pPTR = NULL;
   if ( 0 < Nt )
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);

   ReportLumpSumTimeDependentLosses(pChapter,pDetails,pDisplayUnits,level);

   CTotalPrestressLossTable* pT = CTotalPrestressLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);

   pgsPointOfInterest prev_poi(CSegmentKey(0,0,0),0.0);
   bool bSkipToNextRow = false;
   RowIndexType row1 = 1;
   RowIndexType row2 = 1;
   // **** NOTE **** Use 2 row counters.. one for the table with CY and BS poi, and a different on for BS poi only
   //PoiSet::iterator siter(sPoi.begin());
   //PoiSet::iterator sIterEnd(sPoi.end());
   //for ( ; siter != sIterEnd; siter++ )
   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      bSkipToNextRow = false;

      //pgsPointOfInterest& poi = (*siter).first;
      //IntervalIndexType intervalIdx = (*siter).second;

      pgsPointOfInterest& poi(*iter);

      if ( row1 != 1 && IsEqual(prev_poi.GetDistFromStart(),poi.GetDistFromStart()) )
      {
         row1--;
         bSkipToNextRow = true;
      }

      ReportLocation2(pES,  row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pFR,  row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPTT, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPTP, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPSH, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPTH, row1, poi, end_size, pDisplayUnits);

      //if ( intervalIdx == liveLoadIntervalIdx )
      {
         ReportLocation(pPTR, row2, poi, end_size, pDisplayUnits);
         ReportLocation(pT,   row2, poi, end_size, pDisplayUnits);
      }

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
         ReportRow(pPTR,pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
         ReportRow(pT,  pChapter,m_pBroker,poi,row2,pDetails,pDisplayUnits,level);
         row2++;
      }
      
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

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE(ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   GET_IFACE(IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance( segmentKey );

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);

   //GET_IFACE(IPointOfInterest,pIPoi);
   //std::vector<pgsPointOfInterest> bsPoi( pIPoi->GetPointsOfInterest(segmentKey) );
   //std::vector<pgsPointOfInterest> cyPoi( pIPoi->GetPointsOfInterest( segmentKey, POI_PICKPOINT, POIFIND_OR ) );

   //GET_IFACE(IIntervals,pIntervals);
   //IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(segmentKey);
   //IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   //PoiSet sPoi;
   //std::vector<pgsPointOfInterest>::iterator bsIter(bsPoi.begin());
   //std::vector<pgsPointOfInterest>::iterator bsIterEnd(bsPoi.end());
   //for ( ; bsIter != bsIterEnd; bsIter++ )
   //{
   //   sPoi.insert( std::make_pair(*bsIter,liveLoadIntervalIdx) );
   //}

   //std::vector<pgsPointOfInterest>::iterator cyIter(cyPoi.begin());
   //std::vector<pgsPointOfInterest>::iterator cyIterEnd(cyPoi.end());
   //for ( ; cyIter != cyIterEnd; cyIter++ )
   //{
   //   sPoi.insert( std::make_pair(*cyIter,releaseIntervalIdx) );
   //}

   //GET_IFACE(ILosses,pILosses);
   //const LOSSDETAILS* pDetails = pILosses->GetLossDetails( cyPoi[0] );

   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(segmentKey) );

   GET_IFACE(ILosses,pILosses);
   const LOSSDETAILS* pDetails = pILosses->GetLossDetails( vPoi[0] );

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Approximate Estimate of Time-Dependent Losses [5.9.5.3]") << rptNewLine;

   bool bTemporaryStrands = ( 0 < Nt && pStrands->TempStrandUsage == pgsTypes::ttsPretensioned ? true : false);

   //////////////////////////////////////////////////////////
   // NOTE: The order of everything from here to the loop is important. 
   // It is the order in which things are put into the report
   ReportInitialRelaxation(pChapter,bTemporaryStrands,pDetails->pLosses.get(),pDisplayUnits,level);

   CElasticShorteningTable*                        pES  = CElasticShorteningTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDetails,pDisplayUnits,level);

   ReportLumpSumTimeDependentLossesAtShipping(pChapter,pDetails,pDisplayUnits,level);

   CFrictionLossTable*                             pFR  = NULL;
   CPostTensionInteractionTable*                   pPTT = NULL;
   CEffectOfPostTensionedTemporaryStrandsTable*    pPTP = NULL;

   if ( 0 < Nt && pStrands->TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);
   }

   CTimeDependentLossesAtShippingTable*            pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,bTemporaryStrands,pDisplayUnits,level);
   CPostTensionTimeDependentLossesAtShippingTable* pPTH = NULL;

   if ( 0 < Nt )
      pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,level);

   CTemporaryStrandRemovalTable*                   pPTR = NULL;
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

   pgsPointOfInterest prev_poi(CSegmentKey(0,0,0),0.0);
   bool bSkipToNextRow = false;
   RowIndexType row1 = 1;
   RowIndexType row2 = 1;
   // **** NOTE **** Use 2 row counters.. one for the table with CY and BS poi, and a different one for BS poi only
   //PoiSet::iterator siter(sPoi.begin());
   //PoiSet::iterator siterEnd(sPoi.end());
   //for ( ; siter != siterEnd; siter++ )
   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      bSkipToNextRow = false;

      //pgsPointOfInterest& poi = (*siter).first;
      //IntervalIndexType intervalIdx = (*siter).second;
      pgsPointOfInterest& poi(*iter);

      if ( row1 != 1 && IsEqual(prev_poi.GetDistFromStart(),poi.GetDistFromStart()) )
      {
         row1--;
         bSkipToNextRow = true;
      }

      ReportLocation2(pES,  row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pFR,  row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPTT, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPTP, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPSH, row1, poi, end_size, pDisplayUnits);
      ReportLocation2(pPTH, row1, poi, end_size, pDisplayUnits);

      //if ( intervalIdx == liveLoadIntervalIdx )
      {
         ReportLocation(pPTR,  row2, poi, end_size, pDisplayUnits);
         ReportLocation(pED,   row2, poi, end_size, pDisplayUnits);
         ReportLocation(pSIDL, row2, poi, end_size, pDisplayUnits);
         ReportLocation(pLLIM, row2, poi, end_size, pDisplayUnits);
         ReportLocation(pPE,   row2, poi, end_size, pDisplayUnits);
         ReportLocation(pT,    row2, poi, end_size, pDisplayUnits);
      }

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

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary);

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("General Lump Sum Estimate Losses") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(2,_T(""));
   table->SetColumnWidth(0,3.0);
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pParagraph << table << rptNewLine;

   // Do some preliminary setup for the tables.
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> cyPoi( pIPoi->GetPointsOfInterest(segmentKey) );

   GET_IFACE(ILosses,pILosses);
   const LOSSDETAILS* pDetails = pILosses->GetLossDetails( cyPoi[0] );

   // Typecast to our known type (eating own doggy food)
   boost::shared_ptr<const lrfdLumpSumLosses> ptl = boost::dynamic_pointer_cast<const lrfdLumpSumLosses>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(0); // made a bad cast? Bail...
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
void CPsLossEngineer::ReportInitialRelaxation(rptChapter* pChapter,bool bTemporaryStrands,const lrfdLosses* pLosses,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   if ( pLosses->IgnoreInitialRelaxation() )
      return; // nothing to do

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDisplayUnits->GetStressUnit(),          false );

   // Relaxation At Prestress Transfer
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Prestress loss due to relaxation before transfer") << rptNewLine;

   if ( pLosses->GetStrandType() == matPsStrand::LowRelaxation )
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR0_LR.png")) << rptNewLine;
   else 
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR0_SR.png")) <<rptNewLine;

   rptRcTable* table;

   if ( bTemporaryStrands )
   {
      table = pgsReportStyleHolder::CreateDefaultTable(4,_T("Temporary Strands"));
      *pParagraph << table << rptNewLine;

      (*table)(0,0) << _T("t") << rptNewLine << _T("(Days)");
      (*table)(0,1) << COLHDR(RPT_FPJ, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,2) << COLHDR(RPT_FPY, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,3) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR0")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      (*table)(1,0) << ::ConvertFromSysUnits( pLosses->GetInitialAge(), unitMeasure::Day );
      (*table)(1,1) << stress.SetValue( pLosses->GetFpjTemporary() );
      (*table)(1,2) << stress.SetValue( pLosses->GetFpy() );
      (*table)(1,3) << stress.SetValue( pLosses->TemporaryStrand_RelaxationLossesBeforeTransfer() );
   }

   table = pgsReportStyleHolder::CreateDefaultTable(4,_T("Permanent Strands"));
   *pParagraph << table << rptNewLine;

   (*table)(0,0) << _T("t") << rptNewLine << _T("(Days)");
   (*table)(0,1) << COLHDR(RPT_FPJ, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,2) << COLHDR(RPT_FPY, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,3) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR0")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   (*table)(1,0) << ::ConvertFromSysUnits( pLosses->GetInitialAge(), unitMeasure::Day );
   (*table)(1,1) << stress.SetValue( pLosses->GetFpjPermanent() );
   (*table)(1,2) << stress.SetValue( pLosses->GetFpy() );
   (*table)(1,3) << stress.SetValue( pLosses->PermanentStrand_RelaxationLossesBeforeTransfer() );
}

void CPsLossEngineer::ReportLocation2(rptRcTable* pTable,RowIndexType row,const pgsPointOfInterest& poi,Float64 endsize,IEAFDisplayUnits* pDisplayUnits)
{
   if ( pTable == NULL )
      return;

   INIT_UV_PROTOTYPE( rptPointOfInterest,  spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptPointOfInterest,  gdrloc,      pDisplayUnits->GetSpanLengthUnit(),      false );

   RowIndexType rowOffset = pTable->GetNumberOfHeaderRows() - 1;

   (*pTable)(row+rowOffset,0) << gdrloc.SetValue( POI_RELEASED_SEGMENT, poi );
   (*pTable)(row+rowOffset,1) << spanloc.SetValue( POI_ERECTED_SEGMENT, poi, endsize );
}

void CPsLossEngineer::ReportLocation(rptRcTable* pTable,RowIndexType row,const pgsPointOfInterest& poi,Float64 endsize,IEAFDisplayUnits* pDisplayUnits)
{
   if ( pTable == NULL )
      return;

   INIT_UV_PROTOTYPE( rptPointOfInterest,  spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );

   RowIndexType rowOffset = pTable->GetNumberOfHeaderRows() - 1;
   (*pTable)(row+rowOffset,0) << spanloc.SetValue( POI_ERECTED_SEGMENT, poi, endsize );
}

void CPsLossEngineer::ReportLumpSumTimeDependentLossesAtShipping(rptChapter* pChapter,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   // Lump Sum Loss at time of shipping
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Approximate Lump Sum Estimate of Time Dependent Losses at Shipping") << rptNewLine;

   if ( pDetails->LossMethod == pgsTypes::AASHTO_LUMPSUM || pDetails->LossMethod == pgsTypes::WSDOT_LUMPSUM )
   {
      // Approximate methods before 2005
      GET_IFACE( ISpecification,   pSpec);
      GET_IFACE( ILibrary,         pLib);
      std::_tstring spec_name = pSpec->GetSpecification();
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

      Float64 shipping_losses = pSpecEntry->GetShippingLosses();

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
         *pParagraph<< rptRcImage(strImagePath + _T("LumpSumLoss_Shipping_2005_SI.png")) << rptNewLine;
      else
         *pParagraph<< rptRcImage(strImagePath + _T("LumpSumLoss_Shipping_2005_US.png")) << rptNewLine;

      rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(9,_T(""));

      INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,        pDisplayUnits->GetAreaUnit(),            false );
      INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDisplayUnits->GetStressUnit(),          false );

      rptRcScalar scalar;
      scalar.SetFormat( sysNumericFormatTool::Automatic );
      scalar.SetWidth(6);
      scalar.SetPrecision(2);

      *pParagraph << table << rptNewLine;
      (*table)(0,0) << COLHDR(RPT_FCI, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,1) << Sub2(symbol(gamma),_T("st"));
      (*table)(0,2) << _T("Relative") << rptNewLine << _T("Humidity (%)");
      (*table)(0,3) << Sub2(symbol(gamma),_T("h"));
      (*table)(0,4) << COLHDR(RPT_STRESS(_T("pi")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,5) << COLHDR(Sub2(_T("A"),_T("ps")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(0,6) << COLHDR(Sub2(_T("A"),_T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(0,7) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,8) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLTH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      // Typecast to our known type (eating own doggy food)
      boost::shared_ptr<const lrfdApproximateLosses2005> ptl = boost::dynamic_pointer_cast<const lrfdApproximateLosses2005>(pDetails->pLosses);
      if (!ptl)
      {
         ATLASSERT(0); // made a bad cast? Bail...
         return;
      }

      (*table)(1,0) << stress.SetValue( pDetails->pLosses->GetFci() );
      (*table)(1,1) << scalar.SetValue( ptl->GetStrengthFactor() );
      (*table)(1,2) << pDetails->pLosses->GetRelHumidity();
      (*table)(1,3) << scalar.SetValue( ptl->GetHumidityFactor() );
      (*table)(1,4) << stress.SetValue( ptl->GetFpi() );
      (*table)(1,5) << area.SetValue( pDetails->pLosses->GetApsPermanent() );
      (*table)(1,6) << area.SetValue( pDetails->pLosses->GetAg() );
      (*table)(1,7) << stress.SetValue( ptl->PermanentStrand_RelaxationLossesAtXfer() );
      (*table)(1,8) << stress.SetValue( pDetails->pLosses->PermanentStrand_TimeDependentLossesAtShipping() );
   }
}

void CPsLossEngineer::ReportLumpSumTimeDependentLosses(rptChapter* pChapter,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Approximate Lump Sum Estimate of Time Dependent Losses") << rptNewLine;

   if ( pDetails->LossMethod == pgsTypes::AASHTO_LUMPSUM || pDetails->LossMethod == pgsTypes::WSDOT_LUMPSUM )
   {
      std::_tstring strLossEqnImage[2][5][2][2]; 
      // dim 0... 0 = LRFD, 1 = WSDOT
      // dim 1... 0 = I Beam, 1 = U Beam, 2 = SolidSlab, 3 = Box Beams, 4 = Single T
      // dim 2... 0 = Low Relax, 1 = Stress Rel
      // dim 2... 0 = SI, 1 = US
      strLossEqnImage[0][lrfdApproximateLosses::IBeam][0][0]     = _T("ApproxLoss_LRFD_IBeam_LowRelax_SI.png");
      strLossEqnImage[0][lrfdApproximateLosses::IBeam][1][0]     = _T("ApproxLoss_LRFD_IBeam_StressRel_SI.png");
      strLossEqnImage[0][lrfdApproximateLosses::UBeam][0][0]     = _T("ApproxLoss_LRFD_UBeam_LowRelax_SI.png");
      strLossEqnImage[0][lrfdApproximateLosses::UBeam][1][0]     = _T("ApproxLoss_LRFD_UBeam_StressRel_SI.png");
      strLossEqnImage[0][lrfdApproximateLosses::SolidSlab][0][0] = _T("ApproxLoss_LRFD_SolidSlab_LowRelax_SI.png");
      strLossEqnImage[0][lrfdApproximateLosses::SolidSlab][1][0] = _T("ApproxLoss_LRFD_SolidSlab_StressRel_SI.png");
      strLossEqnImage[0][lrfdApproximateLosses::BoxBeam][0][0]   = _T("ApproxLoss_LRFD_BoxGirder_LowRelax_SI.png");
      strLossEqnImage[0][lrfdApproximateLosses::BoxBeam][1][0]   = _T("ApproxLoss_LRFD_BoxGirder_StressRel_SI.png");
      strLossEqnImage[0][lrfdApproximateLosses::SingleT][0][0]   = _T("ApproxLoss_LRFD_SingleT_LowRelax_SI.png");
      strLossEqnImage[0][lrfdApproximateLosses::SingleT][1][0]   = _T("ApproxLoss_LRFD_SingleT_StressRel_SI.png");

      strLossEqnImage[1][lrfdApproximateLosses::IBeam][0][0]     = _T("ApproxLoss_WSDOT_IBeam_LowRelax_SI.png");
      strLossEqnImage[1][lrfdApproximateLosses::IBeam][1][0]     = _T("ApproxLoss_WSDOT_IBeam_StressRel_SI.png");
      strLossEqnImage[1][lrfdApproximateLosses::UBeam][0][0]     = _T("ApproxLoss_WSDOT_UBeam_LowRelax_SI.png");
      strLossEqnImage[1][lrfdApproximateLosses::UBeam][1][0]     = _T("ApproxLoss_WSDOT_UBeam_StressRel_SI.png");
      strLossEqnImage[1][lrfdApproximateLosses::SolidSlab][0][0] = _T("ApproxLoss_LRFD_SolidSlab_LowRelax_SI.png");
      strLossEqnImage[1][lrfdApproximateLosses::SolidSlab][1][0] = _T("ApproxLoss_LRFD_SolidSlab_StressRel_SI.png");
      strLossEqnImage[1][lrfdApproximateLosses::BoxBeam][0][0]   = _T("ApproxLoss_LRFD_BoxGirder_LowRelax_SI.png");
      strLossEqnImage[1][lrfdApproximateLosses::BoxBeam][1][0]   = _T("ApproxLoss_LRFD_BoxGirder_StressRel_SI.png");
      strLossEqnImage[1][lrfdApproximateLosses::SingleT][0][0]   = _T("ApproxLoss_LRFD_SingleT_LowRelax_SI.png");
      strLossEqnImage[1][lrfdApproximateLosses::SingleT][1][0]   = _T("ApproxLoss_LRFD_SingleT_StressRel_SI.png");

      strLossEqnImage[0][lrfdApproximateLosses::IBeam][0][1]     = _T("ApproxLoss_LRFD_IBeam_LowRelax_US.png");
      strLossEqnImage[0][lrfdApproximateLosses::IBeam][1][1]     = _T("ApproxLoss_LRFD_IBeam_StressRel_US.png");
      strLossEqnImage[0][lrfdApproximateLosses::UBeam][0][1]     = _T("ApproxLoss_LRFD_UBeam_LowRelax_US.png");
      strLossEqnImage[0][lrfdApproximateLosses::UBeam][1][1]     = _T("ApproxLoss_LRFD_UBeam_StressRel_US.png");
      strLossEqnImage[0][lrfdApproximateLosses::SolidSlab][0][1] = _T("ApproxLoss_LRFD_SolidSlab_LowRelax_US.png");
      strLossEqnImage[0][lrfdApproximateLosses::SolidSlab][1][1] = _T("ApproxLoss_LRFD_SolidSlab_StressRel_US.png");
      strLossEqnImage[0][lrfdApproximateLosses::BoxBeam][0][1]   = _T("ApproxLoss_LRFD_BoxGirder_LowRelax_US.png");
      strLossEqnImage[0][lrfdApproximateLosses::BoxBeam][1][1]   = _T("ApproxLoss_LRFD_BoxGirder_StressRel_US.png");
      strLossEqnImage[0][lrfdApproximateLosses::SingleT][0][1]   = _T("ApproxLoss_LRFD_SingleT_LowRelax_US.png");
      strLossEqnImage[0][lrfdApproximateLosses::SingleT][1][1]   = _T("ApproxLoss_LRFD_SingleT_StressRel_US.png");

      strLossEqnImage[1][lrfdApproximateLosses::IBeam][0][1]     = _T("ApproxLoss_WSDOT_IBeam_LowRelax_US.png");
      strLossEqnImage[1][lrfdApproximateLosses::IBeam][1][1]     = _T("ApproxLoss_WSDOT_IBeam_StressRel_US.png");
      strLossEqnImage[1][lrfdApproximateLosses::UBeam][0][1]     = _T("ApproxLoss_WSDOT_UBeam_LowRelax_US.png");
      strLossEqnImage[1][lrfdApproximateLosses::UBeam][1][1]     = _T("ApproxLoss_WSDOT_UBeam_StressRel_US.png");
      strLossEqnImage[1][lrfdApproximateLosses::SolidSlab][0][1] = _T("ApproxLoss_LRFD_SolidSlab_LowRelax_US.png");
      strLossEqnImage[1][lrfdApproximateLosses::SolidSlab][1][1] = _T("ApproxLoss_LRFD_SolidSlab_StressRel_US.png");
      strLossEqnImage[1][lrfdApproximateLosses::BoxBeam][0][1]   = _T("ApproxLoss_LRFD_BoxGirder_LowRelax_US.png");
      strLossEqnImage[1][lrfdApproximateLosses::BoxBeam][1][1]   = _T("ApproxLoss_LRFD_BoxGirder_StressRel_US.png");
      strLossEqnImage[1][lrfdApproximateLosses::SingleT][0][1]   = _T("ApproxLoss_LRFD_SingleT_LowRelax_US.png");
      strLossEqnImage[1][lrfdApproximateLosses::SingleT][1][1]   = _T("ApproxLoss_LRFD_SingleT_StressRel_US.png");

      int method = (pDetails->LossMethod == pgsTypes::WSDOT_LUMPSUM) ? 1 : 0;

      // Typecast to our known type (eating own doggy food)
      boost::shared_ptr<const lrfdApproximateLosses> ptl = boost::dynamic_pointer_cast<const lrfdApproximateLosses>(pDetails->pLosses);
      if (!ptl)
      {
         ATLASSERT(0); // made a bad cast? Bail...
         return;
      }

      int beam = (int)ptl->GetBeamType();
      int strand = pDetails->pLosses->GetStrandType() == matPsStrand::LowRelaxation ? 0 : 1;
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
         *pParagraph<< rptRcImage(strImagePath + _T("LumpSumLoss_2005_SI.png")) << rptNewLine;
      else
         *pParagraph<< rptRcImage(strImagePath + _T("LumpSumLoss_2005_US.png")) << rptNewLine;

      rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(9,_T(""));

      INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,        pDisplayUnits->GetAreaUnit(),            false );
      INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDisplayUnits->GetStressUnit(),          false );

      rptRcScalar scalar;
      scalar.SetFormat( sysNumericFormatTool::Automatic );
      scalar.SetWidth(6);
      scalar.SetPrecision(2);

      *pParagraph << table << rptNewLine;
      (*table)(0,0) << COLHDR(RPT_FCI, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,1) << Sub2(symbol(gamma),_T("st"));
      (*table)(0,2) << _T("Relative") << rptNewLine << _T("Humidity (%)");
      (*table)(0,3) << Sub2(symbol(gamma),_T("h"));
      (*table)(0,4) << COLHDR(RPT_STRESS(_T("pi")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,5) << COLHDR(Sub2(_T("A"),_T("ps")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(0,6) << COLHDR(Sub2(_T("A"),_T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(0,7) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,8) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      // Typecast to our known type (eating own doggy food)
      boost::shared_ptr<const lrfdApproximateLosses2005> ptl = boost::dynamic_pointer_cast<const lrfdApproximateLosses2005>(pDetails->pLosses);
      if (!ptl)
      {
         ATLASSERT(0); // made a bad cast? Bail...
         return;
      }

      (*table)(1,0) << stress.SetValue( pDetails->pLosses->GetFci() );
      (*table)(1,1) << scalar.SetValue( ptl->GetStrengthFactor() );
      (*table)(1,2) << pDetails->pLosses->GetRelHumidity();
      (*table)(1,3) << scalar.SetValue( ptl->GetHumidityFactor() );
      (*table)(1,4) << stress.SetValue( ptl->GetFpi() );
      (*table)(1,5) << area.SetValue( pDetails->pLosses->GetApsPermanent() );
      (*table)(1,6) << area.SetValue( pDetails->pLosses->GetAg() );
      (*table)(1,7) << stress.SetValue( ptl->PermanentStrand_RelaxationLossesAtXfer() );
      (*table)(1,8) << stress.SetValue( pDetails->pLosses->TimeDependentLosses() );
   }
}

void CPsLossEngineer::GetLossParameters(const pgsPointOfInterest& poi,const GDRCONFIG& config,
                                        lrfdLosses::SectionPropertiesType* pSectionProperties,
   matPsStrand::Grade* pGrade,
   matPsStrand::Type* pType,
   Float64* pFpjPerm,
   Float64* pFpjTTS,
   Float64* pPerimeter,
   Float64* pAg,
   Float64* pIg,
   Float64* pYbg,
   Float64* pAc,
   Float64* pIc,
   Float64* pYbc,
   Float64* pAn,
   Float64* pIn,
   Float64* pYbn,
   Float64* pAcn,
   Float64* pIcn,
   Float64* pYbcn,
   Float64* pVolume,
   Float64* pSurfaceArea,
   Float64* pAd,
   Float64* ped,
   Float64* pKsh,
   Float64* pepermRelease,// eccentricity of the permanent strands on the non-composite section
   Float64* pepermFinal,
   Float64* petemp,
   Float64* paps,  // area of one prestress strand
   Float64* pApsPerm,
   Float64* pApsTTS,
   Float64* pMdlg,
   Float64* pMadlg,
   Float64* pMsidl,
   Float64* pMllim,
   Float64* prh,
   Float64* pti,
   Float64* pth,
   Float64* ptd,
   Float64* ptf,
   Float64* pPjS,
   Float64* pPjH,
   Float64* pPjT,
   StrandIndexType* pNs,
   StrandIndexType* pNh,
   StrandIndexType* pNt,
   Float64* pGdrCreepK1,
   Float64* pGdrCreepK2,
   Float64* pGdrShrinkageK1,
   Float64* pGdrShrinkageK2,
   Float64* pDeckCreepK1,
   Float64* pDeckCreepK2,
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
   Float64* pEndSize,
   Float64* pAslab,
   Float64* pPslab,
   lrfdLosses::TempStrandUsage* pUsage,
   Float64* pAnchorSet,
   Float64* pWobble,
   Float64* pCoeffFriction,
   Float64* pAngleChange
)
{
   GET_IFACE( IBridge,                 pBridge );
   GET_IFACE( IStrandGeometry,         pStrandGeom );
   GET_IFACE( ISegmentData,            pSegmentData );
   GET_IFACE( ISectionProperties,      pSectProp );
   GET_IFACE( IProductForces,          pProdForces );
   GET_IFACE( IEnvironment,            pEnv );
   GET_IFACE( IPointOfInterest, pIPOI );
   GET_IFACE( IMaterials,              pMaterial );
   GET_IFACE( ISpecification,          pSpec);
   GET_IFACE( IBridgeDescription,      pIBridgeDesc);
   GET_IFACE( ILibrary,                pLibrary);
   GET_IFACE( ILoadFactors,            pILoadFactors );
   GET_IFACE( IIntervals,              pIntervals );

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   *pPjS = config.PrestressConfig.Pjack[pgsTypes::Straight];
   *pNs = config.PrestressConfig.GetNStrands(pgsTypes::Straight);
   
   *pPjH = config.PrestressConfig.Pjack[pgsTypes::Harped];
   *pNh = config.PrestressConfig.GetNStrands(pgsTypes::Harped);
   
   *pPjT = config.PrestressConfig.Pjack[pgsTypes::Temporary];
   *pNt  = config.PrestressConfig.GetNStrands(pgsTypes::Temporary);

   const matPsStrand* pstrand = pSegmentData->GetStrandMaterial(segmentKey,pgsTypes::Straight);
   CHECK(pstrand);
   *pGrade = pstrand->GetGrade();
   *pType  = pstrand->GetType();
   *paps   = pstrand->GetNominalArea();
   Float64 Aps[3];
   Aps[pgsTypes::Straight] = *paps;
   Aps[pgsTypes::Harped]   = pSegmentData->GetStrandMaterial(segmentKey,pgsTypes::Harped)->GetNominalArea();
   Aps[pgsTypes::Temporary]= pSegmentData->GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetNominalArea();

   *pGirderLength = pBridge->GetSegmentLength(segmentKey);
   *pSpanLength   = pBridge->GetSegmentSpanLength(segmentKey);
   *pEndSize      = pBridge->GetSegmentStartEndDistance(segmentKey);

   Float64 nStrandsEffective;

   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType erectIntervalIdx         = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   
   // eccentricity of the permanent strands at release
   *pepermRelease = pStrandGeom->GetEccentricity( releaseIntervalIdx,  poi, config, false, &nStrandsEffective);

   // eccentricity of permanent strands when the deck is cast
   *pepermFinal   = pStrandGeom->GetEccentricity( castDeckIntervalIdx, poi, config, false, &nStrandsEffective);

   // eccentricity of the temporary strands
   *petemp = pStrandGeom->GetTempEccentricity( releaseIntervalIdx, poi, config, &nStrandsEffective);

   pgsTypes::SectionPropertyType spType = (pgsTypes::SectionPropertyType)(pSectProp->GetSectionPropertiesMode());
   *pSectionProperties = (lrfdLosses::SectionPropertiesType)(spType);

   *pPerimeter = pSectProp->GetPerimeter(poi);
   *pAg  = pSectProp->GetAg( spType, erectIntervalIdx, poi );
   *pIg  = pSectProp->GetIx( spType, erectIntervalIdx, poi );
   *pYbg = pSectProp->GetY(  spType, erectIntervalIdx, poi, pgsTypes::BottomGirder );
   *pAc  = pSectProp->GetAg( spType, liveLoadIntervalIdx, poi, config.Fc );
   *pIc  = pSectProp->GetIx( spType, liveLoadIntervalIdx, poi, config.Fc );
   *pYbc = pSectProp->GetY(  spType, liveLoadIntervalIdx, poi, pgsTypes::BottomGirder, config.Fc );

   *pVolume = pSectProp->GetVolume(segmentKey);
   *pSurfaceArea = pSectProp->GetSurfaceArea(segmentKey);

   if ( spType == pgsTypes::sptTransformed )
   {
      *pAn   = pSectProp->GetNetAg(erectIntervalIdx, poi);
      *pIn   = pSectProp->GetNetIg(erectIntervalIdx, poi);
      *pYbn  = pSectProp->GetNetYbg(erectIntervalIdx, poi);
      *pAcn  = pSectProp->GetNetAg(liveLoadIntervalIdx, poi);
      *pIcn  = pSectProp->GetNetIg(liveLoadIntervalIdx, poi);
      *pYbcn = pSectProp->GetNetYbg(liveLoadIntervalIdx, poi);
   }
   else
   {
      // gross
      *pAn   = *pAg;
      *pIn   = *pIg;
      *pYbn  = *pYbg;
      *pAcn  = *pAc;
      *pIcn  = *pIc;
      *pYbcn = *pYbc;
   }

   // area of deck
   if ( pBridge->IsCompositeDeck() )
   {
      if ( spType == pgsTypes::sptTransformed )
         *pAd  = pSectProp->GetNetAd( compositeDeckIntervalIdx, poi );
      else
         *pAd  = pSectProp->GetGrossDeckArea( poi );
   }
   else
   {
       *pAd = 0.0;
   }

   // eccentricity of deck
   *ped  = pSectProp->GetY( compositeDeckIntervalIdx, poi, pgsTypes::TopGirder, config.Fc ) 
         + pBridge->GetStructuralSlabDepth(poi)/2;
   *ped *= -1;

   *pApsPerm = Aps[pgsTypes::Straight]*(*pNs) + Aps[pgsTypes::Harped]*(*pNh);
   *pApsTTS  = Aps[pgsTypes::Temporary]*(*pNt);

   *pGdrCreepK1      = pMaterial->GetSegmentCreepK1(segmentKey);
   *pGdrCreepK2      = pMaterial->GetSegmentCreepK2(segmentKey);
   *pGdrShrinkageK1  = pMaterial->GetSegmentShrinkageK1(segmentKey);
   *pGdrShrinkageK2  = pMaterial->GetSegmentShrinkageK2(segmentKey);
   *pDeckCreepK1     = pMaterial->GetDeckCreepK1();
   *pDeckCreepK2     = pMaterial->GetDeckCreepK2();
   *pDeckShrinkageK1 = pMaterial->GetDeckShrinkageK1();
   *pDeckShrinkageK2 = pMaterial->GetDeckShrinkageK2();


   *pFci    = config.Fci;
   *pFc     = config.Fc;
   *pFcSlab = pMaterial->GetDeckFc(compositeDeckIntervalIdx);

   if ( config.bUserEci )
      *pEci = config.Eci;
   else
      *pEci = pMaterial->GetEconc(config.Fci,pMaterial->GetSegmentStrengthDensity(segmentKey),pMaterial->GetSegmentEccK1(segmentKey),pMaterial->GetSegmentEccK2(segmentKey));

   if( config.bUserEc )
      *pEc = config.Ec;
   else
      *pEc  = pMaterial->GetEconc(config.Fc, pMaterial->GetSegmentStrengthDensity(segmentKey),pMaterial->GetSegmentEccK1(segmentKey),pMaterial->GetSegmentEccK2(segmentKey));

   if (pDeck->Concrete.bUserEc)
      *pEcSlab = pDeck->Concrete.Ec;
   else
      *pEcSlab = pMaterial->GetDeckEc(compositeDeckIntervalIdx);

   if ( IsZero(*pApsPerm) )
   {
      *pFpjPerm = 0.0;
   }
   else
   {
      *pFpjPerm = (*pPjS + *pPjH) / *pApsPerm;
   }

   if ( !IsZero(*pApsTTS) )
      *pFpjTTS = *pPjT / *pApsTTS;
   else
      *pFpjTTS = 0;

   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   *pMdlg  = pProdForces->GetMoment( releaseIntervalIdx, pftGirder, poi, bat);

   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());

   // effectiveness of deck shrinkage
   *pKsh = pSpecEntry->GetDeckShrinkageElasticGain();

   Float64 K_slab    = pSpecEntry->GetSlabElasticGain();
   Float64 K_slabpad = pSpecEntry->GetSlabPadElasticGain();
   Float64 K_dia     = pSpecEntry->GetDiaphragmElasticGain();
   Float64 K_userdc1 = pSpecEntry->GetUserLoadBeforeDeckDCElasticGain();
   Float64 K_userdw1 = pSpecEntry->GetUserLoadBeforeDeckDWElasticGain();

   Float64 K_railing = pSpecEntry->GetRailingSystemElasticGain();
   Float64 K_userdc2 = pSpecEntry->GetUserLoadAfterDeckDCElasticGain();
   Float64 K_userdw2 = pSpecEntry->GetUserLoadAfterDeckDWElasticGain();
   Float64 K_overlay = pSpecEntry->GetOverlayElasticGain();
   Float64 K_liveload = pSpecEntry->GetLiveLoadElasticGain();

   if ( spType == pgsTypes::sptTransformed )
   {
      // effectiveness factors don't apply for transformed properties
      // elastic gains are computed impliciity and can't be scaled.
      *pKsh = 1.0;
      K_slab = 1.0;
      K_slabpad = 1.0;
      K_dia = 1.0;
      K_userdc1 = 1.0;
      K_userdw1 = 1.0;
      K_railing = 1.0;
      K_userdc2 = 1.0;
      K_userdw2 = 1.0;
      K_overlay = 1.0;
      K_liveload = 1.0;
   }

   if ( poi.GetDistFromStart() < *pEndSize || *pEndSize + *pSpanLength < poi.GetDistFromStart() )
   {
      *pMadlg = 0;
   }
   else
   {
      *pMadlg = K_slab    * pProdForces->GetMoment( castDeckIntervalIdx, pftSlab,      poi, bat ) +
                K_slabpad * pProdForces->GetMoment( castDeckIntervalIdx, pftSlabPad,   poi, bat ) + 
                K_dia     *(pProdForces->GetMoment( castDeckIntervalIdx, pftDiaphragm, poi, bat ) + 
                            pProdForces->GetMoment( castDeckIntervalIdx, pftShearKey,  poi, bat )) + 
                K_userdc1 * pProdForces->GetMoment( castDeckIntervalIdx, pftUserDC,    poi, bat ) +
                K_userdw1 * pProdForces->GetMoment( castDeckIntervalIdx, pftUserDW,    poi, bat );

      if ( pDeck->DeckType == pgsTypes::sdtCompositeSIP )
      {
         *pMadlg += K_slab * pProdForces->GetMoment( castDeckIntervalIdx, pftSlabPanel, poi, bat );
      }
   }

   if ( m_bComputingLossesForDesign )
   {
      // get the additional moment caused by the difference in input and design "A" dimension
      Float64 M = pProdForces->GetDesignSlabPadMomentAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi);
      *pMadlg += K_slabpad*M;
   }

   *pMsidl = K_railing * (pProdForces->GetMoment( railingSystemIntervalIdx, pftTrafficBarrier, poi, bat ) +
                          pProdForces->GetMoment( railingSystemIntervalIdx, pftSidewalk,       poi, bat )) +
             K_userdc2 *  pProdForces->GetMoment( compositeDeckIntervalIdx, pftUserDC,         poi, bat ) +
             K_userdw2 *  pProdForces->GetMoment( compositeDeckIntervalIdx, pftUserDW,         poi, bat );

   // only include overlay load if it is not a future overlay
   if ( !pBridge->IsFutureOverlay() )
   {
      *pMsidl += K_overlay*pProdForces->GetMoment( overlayIntervalIdx, pftOverlay, poi, bat );
   }

   
   Float64 Mmin, Mmax;
   pProdForces->GetLiveLoadMoment(pgsTypes::lltDesign,liveLoadIntervalIdx,poi,bat,true,true,&Mmin,&Mmax);
   Float64 gMaxI   = pILoadFactors->GetLoadFactors()->LLIMmax[pgsTypes::ServiceI];
   Float64 gMaxIII = pILoadFactors->GetLoadFactors()->LLIMmax[pgsTypes::ServiceIII];
   *pMllim = K_liveload*Max(gMaxI,gMaxIII)*Mmax;

   *prh = pEnv->GetRelHumidity();

   // get time to prestress transfer
   GET_IFACE(ILibrary,pLib);
   std::_tstring spec_name = pSpec->GetSpecification();
   *pti = pSpecEntry->GetXferTime();
   *pth = pSpecEntry->GetShippingTime();
   *ptd = pSpecEntry->GetCreepDuration2Max();
   *ptf = pSpecEntry->GetTotalCreepDuration();

   *pAslab = pSectProp->GetTributaryDeckArea(poi);
   *pPslab = pSectProp->GetTributaryFlangeWidth(poi);
   // *NOTE* Only the top portion of the slab is exposed to drying

   // Update the data members of the loss calculation object.  It will take care of the rest
   switch (config.PrestressConfig.TempStrandUsage)
   {
   case pgsTypes::ttsPretensioned:
      *pUsage = lrfdLosses::tsPretensioned;
      break;

   case pgsTypes::ttsPTBeforeLifting:
      *pUsage = lrfdLosses::tsPTBeforeLifting;
      break;

   case pgsTypes::ttsPTAfterLifting:
      *pUsage = lrfdLosses::tsPTAfterLifting;
      break;

   case pgsTypes::ttsPTBeforeShipping:
      *pUsage = lrfdLosses::tsPTBeforeShipping;
      break;
   }

   GET_IFACE(ILossParameters,pLossParameters);
   pLossParameters->GetTemporaryStrandPostTensionParameters(pAnchorSet,pWobble,pCoeffFriction);
   *pAngleChange = 0;
}

void CPsLossEngineer::ReportFinalLosses(BeamType beamType,const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE(ILossParameters,pLossParameters);
   pgsTypes::LossMethod loss_method = pLossParameters->GetLossMethod();

   Uint16 level = 0;
   switch( loss_method )
   {
   case pgsTypes::AASHTO_REFINED:
   case pgsTypes::AASHTO_LUMPSUM:
      ReportFinalLossesRefinedMethod(pChapter,beamType,girderKey,pDisplayUnits,laAASHTO);
      break;

   case pgsTypes::WSDOT_REFINED:
   case pgsTypes::WSDOT_LUMPSUM:
      ReportFinalLossesRefinedMethod(pChapter,beamType,girderKey,pDisplayUnits,laWSDOT);
      break;

   case pgsTypes::TXDOT_REFINED_2004:
      ReportFinalLossesRefinedMethod(pChapter,beamType,girderKey,pDisplayUnits,laTxDOT);
      break;

   case pgsTypes::GENERAL_LUMPSUM:
      ReportGeneralLumpSumMethod(beamType,girderKey,pChapter,pDisplayUnits,false,0);
      break;

   default:
      CHECK(false); // Should never get here
   }
}


void CPsLossEngineer::ReportFinalLossesRefinedMethod(rptChapter* pChapter,BeamType beamType,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,LossAgency lossAgency)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 ||
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

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   Float64 end_size = pBridge->GetSegmentStartEndDistance( segmentKey );

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Final Prestress Losses") << rptNewLine;

   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> bsPoi( pIPoi->GetPointsOfInterest( segmentKey ) );

   GET_IFACE(ILosses,pILosses);
   const LOSSDETAILS* pDetails = pILosses->GetLossDetails( bsPoi[0] );

   CTotalPrestressLossTable* pT = CTotalPrestressLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDetails,pDisplayUnits,0);

   ///////////////////////////////////////////////////////////////////////
   // Loop over all the POIs and populate the tables with loss information
   RowIndexType row = 1;
   std::vector<pgsPointOfInterest>::iterator iter(bsPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(bsPoi.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest& poi = *iter;

      const LOSSDETAILS* pDetails = pILosses->GetLossDetails( poi );

      ReportLocation(pT,row,poi,end_size,pDisplayUnits);

      // fill each row1 with data
      ReportRow(pT,pChapter,m_pBroker,poi,row,pDetails,pDisplayUnits,0);
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

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Refined Estimate of Time-Dependent Losses [5.9.5.4]") << rptNewLine;

   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> bsPoi( pIPoi->GetPointsOfInterest( segmentKey ) );


   GET_IFACE(ILosses,pILosses);

   GET_IFACE(IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance( segmentKey );

   CFinalPrestressLossTable* pT = CFinalPrestressLossTable::PrepareTable(pChapter,m_pBroker,segmentKey,pDisplayUnits,0);

   RowIndexType row = 1;
   std::vector<pgsPointOfInterest>::iterator iter(bsPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(bsPoi.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest& poi = *iter;

      ReportLocation(pT,row,poi,end_size,pDisplayUnits);
      const LOSSDETAILS* pDetails = pILosses->GetLossDetails( poi );

      ReportRow(pT, pChapter,m_pBroker,poi,row,pDetails,pDisplayUnits,0);

      row++;
   }
}
