///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include "..\PGSuperException.h"

#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>

#include <PgsExt\ReportStyleHolder.h>
#include <PgsExt\PointOfInterest.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\BridgeDescription.h>
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


template <class T>
void ReportRow(T* pTable,rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   if ( pTable == NULL )
      return;

   pTable->AddRow(pChapter,pBroker,poi,row,details,pDisplayUnits,level);
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
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(IBridge,pBridge);
   const GDRCONFIG& config = pBridge->GetGirderConfiguration(span,gdr);

   return ComputeLosses(beamType,poi,config);
}

LOSSDETAILS CPsLossEngineer::ComputeLosses(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   LOSSDETAILS details;

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const unitmgtLengthData& length = pDisplayUnits->GetSpanLengthUnit();

   GET_IFACE(ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   switch (  pSpecEntry->GetLossMethod() )
   {
   case LOSSES_AASHTO_REFINED:
      LossesByRefinedEstimate(beamType,poi,config,&details,laAASHTO);
      break;

   case LOSSES_WSDOT_REFINED:
      LossesByRefinedEstimate(beamType,poi,config,&details,laWSDOT);
      break;

   case LOSSES_TXDOT_REFINED_2004:
      LossesByRefinedEstimate(beamType,poi,config,&details,laTxDOT);
      break;

   case LOSSES_TXDOT_REFINED_2013:
      LossesByRefinedEstimateTxDOT2013(beamType,poi,config,&details);
      break;

   case LOSSES_AASHTO_LUMPSUM:
      LossesByApproxLumpSum(beamType,poi,config,&details,false);
      break;

   case LOSSES_WSDOT_LUMPSUM:
      LossesByApproxLumpSum(beamType,poi,config,&details,true);
      break;

   case LOSSES_GENERAL_LUMPSUM:
      LossesByGeneralLumpSum(beamType,poi,config,&details);
      break;

   default:
      CHECK(false); // Should never get here
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

void CPsLossEngineer::BuildReport(BeamType beamType,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE(ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   Uint16 level = 0;
   switch( pSpecEntry->GetLossMethod() )
   {
   case LOSSES_AASHTO_REFINED:
      ReportRefinedMethod(beamType,span,gdr,pChapter,pDisplayUnits,level,laAASHTO);
      break;

   case LOSSES_WSDOT_REFINED:
      ReportRefinedMethod(beamType,span,gdr,pChapter,pDisplayUnits,level,laWSDOT);
      break;

   case LOSSES_TXDOT_REFINED_2004:
      ReportRefinedMethod(beamType,span,gdr,pChapter,pDisplayUnits,level,laTxDOT);
      break;

   case LOSSES_AASHTO_LUMPSUM:
      ReportApproxLumpSumMethod(beamType,span,gdr,pChapter,pDisplayUnits,level,false);
      break;

   case LOSSES_WSDOT_LUMPSUM:
      ReportApproxLumpSumMethod(beamType,span,gdr,pChapter,pDisplayUnits,level,true);
      break;

   case LOSSES_GENERAL_LUMPSUM:
      ReportGeneralLumpSumMethod(beamType,span,gdr,pChapter,pDisplayUnits,true,level);
      break;

   case LOSSES_TXDOT_REFINED_2013:
      ReportRefinedMethodTxDOT2013(pChapter,beamType,span,gdr,pDisplayUnits,level);
      break;

   default:
      CHECK(false); // Should never get here
   }
}

void CPsLossEngineer::ReportRefinedMethod(BeamType beamType,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,Uint16 level,LossAgency lossAgency)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 ||
        lossAgency==laTxDOT)
   {
      ReportRefinedMethodBefore2005(pChapter,beamType,span,gdr,pDisplayUnits,level);
   }
   else
   {
      ReportRefinedMethod2005(pChapter,beamType,span,gdr,pDisplayUnits,level);
   }
}

void CPsLossEngineer::ReportApproxLumpSumMethod(BeamType beamType,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,Uint16 level,bool isWsdot)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 )
   {
      ReportApproxMethod(pChapter,beamType,span,gdr,pDisplayUnits,level,isWsdot);
   }
   else
   {
      ReportApproxMethod2005(pChapter,beamType,span,gdr,pDisplayUnits,level);
   }
}

void CPsLossEngineer::ReportGeneralLumpSumMethod(BeamType beamType,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,bool bDesign,Uint16 level)
{
   ReportLumpSumMethod(pChapter,beamType,span,gdr,pDisplayUnits,bDesign,level);
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
   pLosses->Method = LOSSES_AASHTO_REFINED;

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
   Float64 Volume;
   Float64 SurfaceArea;
   Float64 Ad;
   Float64 ed;
   Float64 Ksh;
   Float64 eperm;// eccentricity of the permanent strands on the non-composite section
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
                     &grade, &type, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ig, &Ybg, &Ac, &Ic, &Ybc, &Volume, &SurfaceArea,&Ad, &ed, &Ksh,
                     &eperm, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl, &Mllim, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &Ns, &Nh, &Nt,
                     &GdrCreepK1, &GdrCreepK2, &GdrShrinkageK1, &GdrShrinkageK2,
                     &DeckCreepK1, &DeckCreepK2, &DeckShrinkageK1, &DeckShrinkageK2,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,&end_size,
                     &Aslab,&Pslab,&usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   // get time to prestress transfer
   GET_IFACE( ISpecification,   pSpec);
   GET_IFACE(ILibrary,pLib);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   Float64 shipping_loss = pSpecEntry->GetShippingLosses();

   boost::shared_ptr<lrfdRefinedLosses> pLoss (new lrfdRefinedLosses(poi.GetDistFromStart(),
                                                                    girder_length,
                                                                    grade,
                                                                    type,
                                                                    fpjPerm,
                                                                    fpjTTS,
                                                                    ApsPerm,
                                                                    ApsTTS,
                                                                    aps,
                                                                    eperm,
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
         pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,1,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());
      }
      else if ( e.GetReasonCode() == lrfdXPsLosses::fcOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Concrete strength is out of range per LRFD 5.4.2.1 and 5.9.5.1");
         pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,2,m_StatusGroupID,m_scidGirderDescriptionWarning,msg.c_str());
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
   Float64 Volume;
   Float64 SurfaceArea;
   Float64 Ad;
   Float64 ed;
   Float64 Ksh;
   Float64 eperm;// eccentricity of the permanent strands on the non-composite section
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
                     &grade, &type, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ig, &Ybg, &Ac, &Ic, &Ybc, &Volume, &SurfaceArea, &Ad, &ed, &Ksh,
                     &eperm, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl, &Mllim, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &Ns, &Nh, &Nt,
                     &GdrCreepK1, &GdrCreepK2, &GdrShrinkageK1, &GdrShrinkageK2,
                     &DeckCreepK1, &DeckCreepK2, &DeckShrinkageK1, &DeckShrinkageK2,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,&end_size,
                     &Aslab,&Pslab,&usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   // get time to prestress transfer
   GET_IFACE( ISpecification,   pSpec);
   GET_IFACE( ILibrary,         pLib);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   if ( lossAgency==laWSDOT )
   {
      pLosses->Method = LOSSES_WSDOT_REFINED_2005;
   }
   else
   {
      pLosses->Method = LOSSES_AASHTO_REFINED_2005;
   }

   lrfdRefinedLosses2005::RelaxationLossMethod relaxationMethod = (lrfdRefinedLosses2005::RelaxationLossMethod)pSpecEntry->GetRelaxationLossMethod();
   boost::shared_ptr<lrfdRefinedLosses2005> pLoss(new lrfdRefinedLosses2005(poi.GetDistFromStart(),
                                                  girder_length,
                                                  grade,
                                                  type,
                                                  fpjPerm,
                                                  fpjTTS,
                                                  ApsPerm,
                                                  ApsTTS,
                                                  aps,
                                                  eperm,
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
         pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,1,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());
      }
      else if ( e.GetReasonCode() == lrfdXPsLosses::StrandType )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("The relaxation loss of 1.2 ksi can only be used with low relaxation strands (see Article 5.9.5.4.2c)\nChange the strand type or select a different method for computing losses");
         pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,1,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());
      }
      else if ( e.GetReasonCode() == lrfdXPsLosses::fcOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Concrete strength is out of range per LRFD 5.4.2.1 and 5.9.5.1");
         pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,2,m_StatusGroupID,m_scidGirderDescriptionWarning,msg.c_str());
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
   // Compute details - This is a bit tricky: We practically need to compute losses in order to determine which method to use
   //                   for elastic shortening. So might as well save on code and compute them - then figure out if we can cache
   //                   This may be first time through, so we'll check on the back side and; if we are using the 
   //                   simplified method, we need to recompute at mid-girder
   lrfdElasticShortening::FcgpComputationMethod method = LossesByRefinedEstimateTxDOT2013_Compute(beamType,poi,config,pLosses);

   if(method == lrfdElasticShortening::fcgp07Fpu)
   {
      // Elastic shortening uses the 0.7Fpu method. We only need to compute at mid-girder and then cache results for other locations
      GET_IFACE( IPointOfInterest,   pPOI);
      std::vector<pgsPointOfInterest> vPoi = pPOI->GetPointsOfInterest( poi.GetSpan(), poi.GetGirder(), pgsTypes::CastingYard, POI_MIDSPAN);
      pgsPointOfInterest midpoi = vPoi[0];

      lrfdElasticShortening::FcgpComputationMethod newmethod = LossesByRefinedEstimateTxDOT2013_Compute(beamType, midpoi, config, pLosses);
      ATLASSERT(newmethod == lrfdElasticShortening::fcgp07Fpu);
   }
}

lrfdElasticShortening::FcgpComputationMethod CPsLossEngineer::LossesByRefinedEstimateTxDOT2013_Compute(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses)
{
   pLosses->Method = LOSSES_TXDOT_REFINED_2013;

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
   Float64 Volume;
   Float64 SurfaceArea;
   Float64 Ad;
   Float64 ed;
   Float64 Ksh;
   Float64 eperm;// eccentricity of the permanent strands on the non-composite section
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
                     &grade, &type, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ig, &Ybg, &Ac, &Ic, &Ybc, &Volume, &SurfaceArea,&Ad, &ed, &Ksh,
                     &eperm, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl, &Mllim, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &Ns, &Nh, &Nt,
                     &GdrCreepK1, &GdrCreepK2, &GdrShrinkageK1, &GdrShrinkageK2,
                     &DeckCreepK1, &DeckCreepK2, &DeckShrinkageK1, &DeckShrinkageK2,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,&end_size,
                     &Aslab,&Pslab,&usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

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

      // Use 0.7Fpu method to compute Fcgp if: permanent strands are jacked to 0.75*Fpu and,
      // no temp strands exist and, beam is prismatic, and no debonding exists.
      // Otherwise, use iterative method
      method = lrfdElasticShortening::fcgpIterative;
      if ( ApsPerm >= 0.0 && IsEqual(ApsTTS, 0.0) )
      {
         Float64 Fpu = lrfdPsStrand::GetUltimateStrength( grade );

         if (ApsPerm==0.0 || IsEqual(Fpu*0.75, fpjPerm, 1000.0)) // Pa's are very small
         {
            GET_IFACE(IGirder,pGirder);
            if ( pGirder->IsPrismatic(pgsTypes::CastingYard,span,gdr) )
            {
               if( config.PrestressConfig.Debond[pgsTypes::Straight].empty() && config.PrestressConfig.Debond[pgsTypes::Harped].empty() )
               {
                  method = lrfdElasticShortening::fcgp07Fpu;
               }
            }
         }
      }
   }

   boost::shared_ptr<lrfdRefinedLossesTxDOT2013> pLoss(new lrfdRefinedLossesTxDOT2013( poi.GetDistFromStart(),
                                                  girder_length,
                                                  grade,
                                                  type,
                                                  fpjPerm,
                                                  fpjTTS,
                                                  ApsPerm,
                                                  ApsTTS,
                                                  aps,
                                                  eperm,
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
         CEAFStatusItem* pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,1,m_StatusGroupID,m_scidGirderDescriptionWarning,msg.c_str());
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
         pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,1,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());
      }
      else if ( e.GetReasonCode() == lrfdXPsLosses::fcOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Concrete strength is out of range per LRFD 5.4.2.1 and 5.9.5.1");
         pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,2,m_StatusGroupID,m_scidGirderDescriptionWarning,msg.c_str());
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

   return method;
}

void CPsLossEngineer::LossesByApproxLumpSum(BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses,bool isWsdot)
{
   PRECONDITION(pLosses != 0 );

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
   Float64 Volume;
   Float64 SurfaceArea;
   Float64 Ad;
   Float64 ed;
   Float64 Ksh;
   Float64 eperm;// eccentricity of the permanent strands on the non-composite section
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

   GET_IFACE(IBridgeMaterialEx,pMaterial);
   pgsTypes::ConcreteType girderConcreteType = pMaterial->GetGdrConcreteType(poi.GetSpan(),poi.GetGirder());
   pgsTypes::ConcreteType slabConcreteType   = pMaterial->GetSlabConcreteType();

   if ( girderConcreteType != pgsTypes::Normal || slabConcreteType != pgsTypes::Normal )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      std::_tstring msg(_T("The approximate estimate of time-dependent losses given in LRFD 5.9.5.3 is for members made from normal-weight concrete"));
      pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidConcreteTypeError,msg.c_str());
      pStatusCenter->Add(pStatusItem);

      msg += std::_tstring(_T("\nSee Status Center for Details"));
      THROW_UNWIND(msg.c_str(),XREASON_LRFD_VERSION);
   }

   GetLossParameters(poi,config,
                     &grade, &type, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ig, &Ybg, &Ac, &Ic, &Ybc, &Volume, &SurfaceArea, &Ad, &ed, &Ksh,
                     &eperm, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl, &Mllim, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &Ns, &Nh, &Nt,
                     &GdrCreepK1, &GdrCreepK2, &GdrShrinkageK1, &GdrShrinkageK2,
                     &DeckCreepK1, &DeckCreepK2, &DeckShrinkageK1, &DeckShrinkageK2,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,&end_size,
                     &Aslab,&Pslab,&usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

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
            pLosses->Method = LOSSES_WSDOT_LUMPSUM;
         }
         else
         {
            GET_IFACE(ILongRebarGeometry,pLongRebarGeom);
            ppr = pLongRebarGeom->GetPPRBottomHalf(poi);
            pLosses->Method = LOSSES_AASHTO_LUMPSUM;
         }

         Float64 shipping_loss = pSpecEntry->GetShippingLosses();

         boost::shared_ptr<lrfdApproximateLosses> pLoss (new lrfdApproximateLosses(
                                                       (lrfdApproximateLosses::BeamType)beamType,
                                                       shipping_loss,
                                                       ppr,
                                                       poi.GetDistFromStart(), // location along girder where losses are computed
                                                       girder_length,    // girder length
                                                       grade,
                                                       type,
                                                       fpjPerm,
                                                       fpjTTS,
                                                       ApsPerm,  // area of permanent strand
                                                       ApsTTS,  // area of TTS 
                                                       aps,      // area of one strand
                                                       eperm, // eccentricty of permanent ps strands with respect to CG of girder
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

         boost::shared_ptr<lrfdApproximateLosses2005> pLoss (new lrfdApproximateLosses2005(poi.GetDistFromStart(), // location along girder where losses are computed
                            girder_length,    // girder length
                            grade,
                            type,
                            fpjPerm,
                            fpjTTS,
                            ApsPerm,  // area of permanent strand
                            ApsTTS,  // area of TTS 
                            aps,      // area of one strand
                            eperm, // eccentricty of permanent ps strands with respect to CG of girder
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

                            rh,      // relative humidity
                            ti,   // Time until prestress transfer
                            !isWsdot,
                            false
                            ));

         if ( isWsdot )
         {
            pLosses->Method = LOSSES_WSDOT_LUMPSUM_2005;
         }
         else
         {
            pLosses->Method = LOSSES_AASHTO_LUMPSUM_2005;
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
         pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,1,m_StatusGroupID,m_scidGirderDescriptionError,msg.c_str());
      }
      else if ( e.GetReasonCode() == lrfdXPsLosses::fcOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = _T("Concrete strength is out of range per LRFD 5.4.2.1 and 5.9.5.1");
         pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,0,m_StatusGroupID,m_scidGirderDescriptionWarning,msg.c_str());
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
   PRECONDITION(pLosses != 0 );
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
   Float64 Volume;
   Float64 SurfaceArea;
   Float64 Ad;
   Float64 ed;
   Float64 Ksh;
   Float64 eperm;// eccentricity of the permanent strands on the non-composite section
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
                     &grade, &type, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ig, &Ybg, &Ac, &Ic, &Ybc, &Volume, &SurfaceArea, &Ad, &ed, &Ksh,
                     &eperm, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl, &Mllim, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &Ns, &Nh, &Nt,
                     &GdrCreepK1, &GdrCreepK2, &GdrShrinkageK1, &GdrShrinkageK2,
                     &DeckCreepK1, &DeckCreepK2, &DeckShrinkageK1, &DeckShrinkageK2,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,&end_size,
                     &Aslab,&Pslab,&usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   pLosses->Method = LOSSES_GENERAL_LUMPSUM;

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
      GET_IFACE(ISpecification,pSpec);
      std::_tstring strSpecName = pSpec->GetSpecification();

      GET_IFACE(ILibrary,pLib);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

      boost::shared_ptr<const lrfdLumpSumLosses> pLoss (new lrfdLumpSumLosses(ApsPerm,ApsTTS,fpjPerm,fpjTTS,usage,
                                           pSpecEntry->GetBeforeXferLosses(),
                                           pSpecEntry->GetAfterXferLosses(),
                                           pSpecEntry->GetLiftingLosses(),
                                           pSpecEntry->GetShippingLosses(),
                                           pSpecEntry->GetBeforeTempStrandRemovalLosses(),
                                           pSpecEntry->GetAfterTempStrandRemovalLosses(),
                                           pSpecEntry->GetAfterDeckPlacementLosses(),
                                           pSpecEntry->GetAfterSIDLLosses(),
                                           pSpecEntry->GetFinalLosses()));

      pLosses->pLosses = boost::static_pointer_cast<const lrfdLosses>(pLoss);
      ATLASSERT(pLosses->pLosses!=NULL);
   }
}


void CPsLossEngineer::ReportRefinedMethodBefore2005(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   rptParagraph* pParagraph;

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE(IGirderData,pGirderData);
   const CGirderData* pgirderData = pGirderData->GetGirderData(span,gdr);

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Temporary);

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Refined Estimate of Time-Dependent Losses [5.9.5.4]") << rptNewLine;

   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> bsPoi = pIPoi->GetPointsOfInterest( span, gdr, pgsTypes::BridgeSite3, POI_TABULAR );
   std::vector<pgsPointOfInterest> cyPoi = pIPoi->GetPointsOfInterest( span, gdr, pgsTypes::CastingYard, POI_TABULAR | POI_PICKPOINT, POIFIND_OR);
   std::vector<pgsPointOfInterest>::iterator iter;

   PoiSet sPoi;
   PoiSet::iterator siter;
   for ( iter = bsPoi.begin(); iter != bsPoi.end(); iter++ )
   {
      sPoi.insert( std::make_pair(*iter,pgsTypes::BridgeSite3) );
   }

   for ( iter = cyPoi.begin(); iter != cyPoi.end(); iter++ )
   {
      sPoi.insert( std::make_pair(*iter,pgsTypes::CastingYard) );
   }

   GET_IFACE(ILosses,pILosses);
   LOSSDETAILS details = pILosses->GetLossDetails( cyPoi[0] );

   GET_IFACE(IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength( span,gdr );

   // Do some preliminary setup for the tables.
   INIT_UV_PROTOTYPE( rptForceUnitValue,   force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,        pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  ecc,         pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cg,          pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,  moment,      pDisplayUnits->GetMomentUnit(),          false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  mod_e,       pDisplayUnits->GetModEUnit(),            false );

   bool bTemporaryStrands = ( 0 < Nt && pgirderData->PrestressData.TempStrandUsage == pgsTypes::ttsPretensioned ? true : false);

   // Relaxation At Prestress Transfer
   ReportInitialRelaxation(pChapter,bTemporaryStrands,details.pLosses.get(),pDisplayUnits,level);

   CElasticShorteningTable*                     pES  = CElasticShorteningTable::PrepareTable(pChapter,m_pBroker,span,gdr,bTemporaryStrands,
                                                                                             details,pDisplayUnits,level);
   CFrictionLossTable*                          pFR  = NULL;
   CPostTensionInteractionTable*                pPTT = NULL;
   CEffectOfPostTensionedTemporaryStrandsTable* pPTP = NULL;
   
   if ( 0 < Nt && pgirderData->PrestressData.TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   }

   CTimeDependentLossesAtShippingTable*            pPSH = NULL;
   CPostTensionTimeDependentLossesAtShippingTable* pPTH = NULL;

   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if ( pGirderHaulingSpecCriteria->IsHaulingCheckEnabled() )
   {
      pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,span,gdr,bTemporaryStrands,pDisplayUnits,level);

      if ( 0 < Nt ) 
         pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   }

   CTemporaryStrandRemovalTable*        pPTR = NULL;
   if (0 < Nt )
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Time dependent losses") << rptNewLine;

   CChangeOfConcreteStressTable*   pDeltaFcdp = CChangeOfConcreteStressTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   CCreepAndShrinkageTable*        pCR        = CCreepAndShrinkageTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   CRelaxationAfterTransferTable*  pR2        = CRelaxationAfterTransferTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   CFinalPrestressLossTable*       pT         = CFinalPrestressLossTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);

   pgsPointOfInterest prev_poi(0,0,0);
   bool bSkipToNextRow = false;
   int row1 = 1;
   int row2 = 1;
   for ( siter = sPoi.begin(); siter != sPoi.end(); siter++ )
   {
      bSkipToNextRow = false;

      pgsPointOfInterest& poi = (*siter).first;
      pgsTypes::Stage stage = (*siter).second;

      if ( row1 != 1 && IsEqual(prev_poi.GetDistFromStart(),poi.GetDistFromStart()) )
      {
         row1--;
         bSkipToNextRow = true;
      }

      // write the location information into the tables
      ReportLocation2(pES,  row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pFR,  row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPTT, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPTP, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPSH, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPTH, row1,stage,poi,end_size,pDisplayUnits);

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportLocation(pPTR,      row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pDeltaFcdp,row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pCR,       row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pR2,       row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pT,        row2,stage,poi,end_size,pDisplayUnits);
      }

      // fill each row1 with data
      if ( !bSkipToNextRow )
      {
         details = pILosses->GetLossDetails( poi );

         ReportRow(pES, pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pFR, pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPTT,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPTP,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPSH,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPTH,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
      }

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportRow(pPTR,      pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pDeltaFcdp,pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pCR,       pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pR2,       pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pT,        pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);

         row2++;
      }
      
      row1++;
      prev_poi = poi;
   }
}

void CPsLossEngineer::ReportRefinedMethod2005(rptChapter* pChapter,BeamType beamType,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   rptParagraph* pParagraph;

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeMaterial,pMaterial);

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Temporary);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   Float64 end_size = pBridge->GetGirderStartConnectionLength( span,gdr );

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Refined Estimate of Time-Dependent Losses [5.9.5.4]") << rptNewLine;

   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> bsPoi = pIPoi->GetPointsOfInterest( span, gdr, pgsTypes::BridgeSite3, POI_TABULAR );
   std::vector<pgsPointOfInterest> cyPoi = pIPoi->GetPointsOfInterest( span, gdr, pgsTypes::CastingYard, POI_TABULAR | POI_PICKPOINT, POIFIND_OR );
   std::vector<pgsPointOfInterest>::iterator iter;

   PoiSet sPoi;
   PoiSet::iterator siter;
   for ( iter = bsPoi.begin(); iter != bsPoi.end(); iter++ )
   {
      sPoi.insert( std::make_pair(*iter,pgsTypes::BridgeSite3) );
   }

   for ( iter = cyPoi.begin(); iter != cyPoi.end(); iter++ )
   {
      sPoi.insert( std::make_pair(*iter,pgsTypes::CastingYard) );
   }

   GET_IFACE(ILosses,pILosses);
   LOSSDETAILS details = pILosses->GetLossDetails( cyPoi[0] );

   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);

   GET_IFACE(IGirderData,pGirderData);
   const CGirderData* pgirderData = pGirderData->GetGirderData(span,gdr);


   bool bTemporaryStrands = ( 0 < Nt && pgirderData->PrestressData.TempStrandUsage == pgsTypes::ttsPretensioned ? true : false);

   ReportInitialRelaxation(pChapter,bTemporaryStrands,details.pLosses.get(),pDisplayUnits,level);

   ////////////////////////////////////////////////////////////////////////////////////////
   // Create the tables for losses - order is important here... The factory methods
   // put content into the chapter and return a table that needs to be filled up
   CElasticShorteningTable*                     pES  = CElasticShorteningTable::PrepareTable(pChapter,m_pBroker,span,gdr,bTemporaryStrands,
                                                                                             details,pDisplayUnits,level);
   CFrictionLossTable*                          pFR  = NULL;
   CPostTensionInteractionTable*                pPTT = NULL;
   CEffectOfPostTensionedTemporaryStrandsTable* pPTP = NULL;

   

   if ( 0 < Nt && pgirderData->PrestressData.TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
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

   pSRH = CShrinkageAtHaulingTable::PrepareTable(pChapter,m_pBroker,span,gdr,bTemporaryStrands,details,pDisplayUnits,level);
   pCRH = CCreepAtHaulingTable::PrepareTable(pChapter,m_pBroker,span,gdr,bTemporaryStrands,details,pDisplayUnits,level);
   pR1H = CRelaxationAtHaulingTable::PrepareTable(pChapter,m_pBroker,span,gdr,bTemporaryStrands,details,pDisplayUnits,level);
   pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,span,gdr,bTemporaryStrands,pDisplayUnits,level);
   pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);


   ///////////////////////////////////////////////////////////////////////////////////////////
   // Time-dependent losses between transfer and deck placement
   ///////////////////////////////////////////////////////////////////////////////////////////

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   if ( pBridge->IsCompositeDeck() )
      *pParagraph << _T("Time dependent losses between transfer and deck placement [5.9.5.4.2]") << rptNewLine;
   else
      *pParagraph << _T("Time dependent losses between transfer and installation of precast members [5.9.5.4.2, 5.9.5.4.4]") << rptNewLine;

   CShrinkageAtDeckPlacementTable*      pSR = CShrinkageAtDeckPlacementTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);
   CCreepAtDeckPlacementTable*          pCR = CCreepAtDeckPlacementTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   CRelaxationAtDeckPlacementTable*     pR1 = CRelaxationAtDeckPlacementTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);
   CTimeDependentLossesAtDeckPlacementTable* pLTid = CTimeDependentLossesAtDeckPlacementTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   CTemporaryStrandRemovalTable*        pPTR = NULL;
   
   if ( 0 < Nt )
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);

   ///////////////////////////////////////////////////////////////////////////////////////////
   // Time-dependent losses between deck placement and final time
   ///////////////////////////////////////////////////////////////////////////////////////////

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   if ( pBridge->IsCompositeDeck() )
      *pParagraph << _T("Losses: Time of Deck Placement to Final Time [5.9.5.4.3]") << rptNewLine;
   else
      *pParagraph << _T("Losses: Time of Installation of Precast Members to Final Time [5.9.5.4.3, 5.9.5.4.4]") << rptNewLine;
   
   CShrinkageAtFinalTable*      pSD = CShrinkageAtFinalTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);
   CCreepAtFinalTable*          pCD = CCreepAtFinalTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);
   CRelaxationAtFinalTable*     pR2 = CRelaxationAtFinalTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);

   CTimeDependentLossFinalTable* pLTdf = CTimeDependentLossFinalTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   CTimeDependentLossesTable*    pLT   = CTimeDependentLossesTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   CTotalPrestressLossTable*     pT    = CTotalPrestressLossTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Change in effective prestress due to externally applied loads (elastic gains)") << rptNewLine;
   CElasticGainDueToDeckPlacementTable*   pED   = CElasticGainDueToDeckPlacementTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   CElasticGainDueToSIDLTable*            pSIDL = CElasticGainDueToSIDLTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   CElasticGainDueToLiveLoadTable*        pLLIM = CElasticGainDueToLiveLoadTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   CElasticGainDueToDeckShrinkageTable*   pSS   = CElasticGainDueToDeckShrinkageTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);
   CEffectivePrestressTable*              pPE   = CEffectivePrestressTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);

   ///////////////////////////////////////////////////////////////////////
   // Loop over all the POIs and populate the tables with loss information
   pgsPointOfInterest prev_poi(0,0,0);
   bool bSkipToNextRow = false;
   int row1 = 1;
   int row2 = 1;
   for ( siter = sPoi.begin(); siter != sPoi.end(); siter++ )
   {
      bSkipToNextRow = false;

      pgsPointOfInterest& poi = (*siter).first;
      pgsTypes::Stage stage = (*siter).second;

      if ( row1 != 1 && IsEqual(prev_poi.GetDistFromStart(),poi.GetDistFromStart()) )
      {
         row1--;
         bSkipToNextRow = true;
      }

      // write the location information into the tables
      ReportLocation2(pES,  row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pFR,  row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPTT, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPTP, row1,stage,poi,end_size,pDisplayUnits);

      ReportLocation2(pSRH, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pCRH, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pR1H, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPSH, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPTH, row1,stage,poi,end_size,pDisplayUnits);

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportLocation(pSR,  row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pCR,  row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pR1,  row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pLTid,row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pPTR, row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pSD,  row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pCD,  row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pR2,  row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pSS,  row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pLTdf,row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pLT,  row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pT,   row2,stage,poi,end_size,pDisplayUnits);

         ReportLocation(pED,  row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pSIDL,row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pLLIM,row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pPE,  row2,stage,poi,end_size,pDisplayUnits);
      }

      // fill each row1 with data
      if ( !bSkipToNextRow )
      {
         details = pILosses->GetLossDetails( poi );

         ReportRow(pES, pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pFR, pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPTT,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPTP,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);

         ReportRow(pSRH, pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pCRH, pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pR1H, pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPSH, pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPTH, pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
      }

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportRow(pSR,  pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pCR,  pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pR1,  pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pLTid,pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pPTR, pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pSD,  pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pCD,  pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pR2,  pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pLTdf,pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pLT,  pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pT,   pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);

         ReportRow(pED,  pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pSIDL,pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pLLIM,pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pSS,  pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pPE,  pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);

         row2++;
      }
      
      row1++;
      prev_poi = poi;
   }
}

void CPsLossEngineer::ReportRefinedMethodTxDOT2013(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   rptParagraph* pParagraph;

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE(IGirderData,pGirderData);
   const CGirderData* pgirderData = pGirderData->GetGirderData(span,gdr);

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Temporary);

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Refined Estimate of Time-Dependent Losses Per TxDOT Research Report 0-6374-2") << rptNewLine;

   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> bsPoi = pIPoi->GetPointsOfInterest( span, gdr, pgsTypes::BridgeSite3, POI_TABULAR );
   std::vector<pgsPointOfInterest> cyPoi = pIPoi->GetPointsOfInterest( span, gdr, pgsTypes::CastingYard, POI_TABULAR | POI_PICKPOINT, POIFIND_OR);
   std::vector<pgsPointOfInterest>::iterator iter;

   PoiSet sPoi;
   PoiSet::iterator siter;
   for ( iter = bsPoi.begin(); iter != bsPoi.end(); iter++ )
   {
      sPoi.insert( std::make_pair(*iter,pgsTypes::BridgeSite3) );
   }

   for ( iter = cyPoi.begin(); iter != cyPoi.end(); iter++ )
   {
      sPoi.insert( std::make_pair(*iter,pgsTypes::CastingYard) );
   }

   GET_IFACE(ILosses,pILosses);
   LOSSDETAILS details = pILosses->GetLossDetails( cyPoi[0] );

   GET_IFACE(IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength( span,gdr );

   // Do some preliminary setup for the tables.
   INIT_UV_PROTOTYPE( rptForceUnitValue,   force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,        pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  ecc,         pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cg,          pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,  moment,      pDisplayUnits->GetMomentUnit(),          false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  mod_e,       pDisplayUnits->GetModEUnit(),            false );

   bool bTemporaryStrands = ( 0 < Nt && pgirderData->PrestressData.TempStrandUsage == pgsTypes::ttsPretensioned ? true : false);

   // NOTE: The order of everything from here to the loop is important. 
   // It is the order in which things are put into the report
   ReportInitialRelaxation(pChapter,bTemporaryStrands,details.pLosses.get(),pDisplayUnits,level);

   CElasticShorteningTable*                     pES  = CElasticShorteningTable::PrepareTable(pChapter,m_pBroker,span,gdr,bTemporaryStrands,
                                                                                             details,pDisplayUnits,level);
   CFrictionLossTable*                          pFR  = NULL;
   CPostTensionInteractionTable*                pPTT = NULL;
   CEffectOfPostTensionedTemporaryStrandsTable* pPTP = NULL;

   if ( 0 < Nt && pgirderData->PrestressData.TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   }

   CTimeDependentLossesAtShippingTable*            pPSH = NULL;
   CPostTensionTimeDependentLossesAtShippingTable* pPTH = NULL;

   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if ( pGirderHaulingSpecCriteria->IsHaulingCheckEnabled() )
   {
      pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,span,gdr,bTemporaryStrands,pDisplayUnits,level);

      if ( 0 < Nt ) 
         pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   }

   CTemporaryStrandRemovalTable*        pPTR = NULL;
   if (0 < Nt )
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Time dependent losses") << rptNewLine;

   CTxDOT2013ChangeOfConcreteStressTable*   pDeltaFcdp = CTxDOT2013ChangeOfConcreteStressTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);
   CTxDOT2013CreepAndShrinkageTable*        pCR        = CTxDOT2013CreepAndShrinkageTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);
   CTxDOT2013RelaxationAfterTransferTable*  pR2        = CTxDOT2013RelaxationAfterTransferTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);
   CTxDOT2013TimeDependentLossesTable*      pLT        = CTxDOT2013TimeDependentLossesTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   CTotalPrestressLossTable*                pT         = CTotalPrestressLossTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);

   pgsPointOfInterest prev_poi(0,0,0);
   bool bSkipToNextRow = false;
   int row1 = 1;
   int row2 = 1;
   for ( siter = sPoi.begin(); siter != sPoi.end(); siter++ )
   {
      bSkipToNextRow = false;

      pgsPointOfInterest& poi = (*siter).first;
      pgsTypes::Stage stage = (*siter).second;

      if ( row1 != 1 && IsEqual(prev_poi.GetDistFromStart(),poi.GetDistFromStart()) )
      {
         row1--;
         bSkipToNextRow = true;
      }

      // write the location information into the tables
      ReportLocation2(pES,  row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pFR,  row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPTT, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPTP, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPSH, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPTH, row1,stage,poi,end_size,pDisplayUnits);

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportLocation(pPTR,      row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pDeltaFcdp,row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pCR,       row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pR2,       row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pLT,       row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pT,        row2,stage,poi,end_size,pDisplayUnits);
      }

      // fill each row1 with data
      if ( !bSkipToNextRow )
      {
         details = pILosses->GetLossDetails( poi );

         ReportRow(pES, pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pFR, pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPTT,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPTP,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPSH,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPTH,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
      }

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportRow(pPTR,      pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pDeltaFcdp,pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pCR,       pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pR2,       pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pLT,       pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pT,        pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);

         row2++;
      }
      
      row1++;
      prev_poi = poi;
   }
}

void CPsLossEngineer::ReportApproxMethod(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,Uint16 level,bool isWsdot)
{
   rptParagraph* pParagraph;

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE(IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength( span,gdr );

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Temporary);

   GET_IFACE(IGirderData,pGirderData);
   const CGirderData* pgirderData = pGirderData->GetGirderData(span,gdr);

   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> bsPoi = pIPoi->GetPointsOfInterest( span, gdr, pgsTypes::BridgeSite3, POI_TABULAR );
   std::vector<pgsPointOfInterest> cyPoi = pIPoi->GetPointsOfInterest( span, gdr, pgsTypes::CastingYard, POI_TABULAR | POI_PICKPOINT, POIFIND_OR );
   std::vector<pgsPointOfInterest>::iterator iter;

   std::set< std::pair<pgsPointOfInterest,pgsTypes::Stage> > sPoi;
   for ( iter = bsPoi.begin(); iter != bsPoi.end(); iter++ )
   {
      sPoi.insert( std::make_pair(*iter,pgsTypes::BridgeSite3) );
   }

   for ( iter = cyPoi.begin(); iter != cyPoi.end(); iter++ )
   {
      sPoi.insert( std::make_pair(*iter,pgsTypes::CastingYard) );
   }

   GET_IFACE(ILosses,pILosses);
   LOSSDETAILS details = pILosses->GetLossDetails( cyPoi[0] );

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   if (isWsdot)
      *pParagraph << _T("Approximate Lump Sum Estimate of Time-Dependent Losses [WSDOT BDM 6.1.5.B3]") << rptNewLine;
   else
      *pParagraph << _T("Approximate Lump Sum Estimate of Time-Dependent Losses [5.9.5.3]") << rptNewLine;

   bool bTemporaryStrands = ( 0 < Nt && pgirderData->PrestressData.TempStrandUsage == pgsTypes::ttsPretensioned ? true : false);

   //////////////////////////////////////////////////////////
   // NOTE: The order of everything from here to the loop is important. 
   // It is the order in which things are put into the report
   ReportInitialRelaxation(pChapter,bTemporaryStrands,details.pLosses.get(),pDisplayUnits,level);

   CElasticShorteningTable*                        pES  = CElasticShorteningTable::PrepareTable(pChapter,m_pBroker,span,gdr,bTemporaryStrands,
                                                                                                details,pDisplayUnits,level);

   ReportLumpSumTimeDependentLossesAtShipping(pChapter,details,pDisplayUnits,level);

   CFrictionLossTable*                             pFR  = NULL;
   CPostTensionInteractionTable*                   pPTT = NULL;
   CEffectOfPostTensionedTemporaryStrandsTable*    pPTP = NULL;

   if ( 0 < Nt && pgirderData->PrestressData.TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   }

   CTimeDependentLossesAtShippingTable*            pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,span,gdr,bTemporaryStrands,pDisplayUnits,level);
   CPostTensionTimeDependentLossesAtShippingTable* pPTH  = NULL;
   
   if ( 0 < Nt )
      pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);

   CTemporaryStrandRemovalTable*                   pPTR = NULL;
   if ( 0 < Nt )
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);

   ReportLumpSumTimeDependentLosses(pChapter,details,pDisplayUnits,level);

   CTotalPrestressLossTable* pT = CTotalPrestressLossTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);

   
   PoiSet::iterator siter;

   pgsPointOfInterest prev_poi(0,0,0);
   bool bSkipToNextRow = false;
   int row1 = 1;
   int row2 = 1;
   // **** NOTE **** Use 2 row counters.. one for the table with CY and BS poi, and a different on for BS poi only
   for ( siter = sPoi.begin(); siter != sPoi.end(); siter++ )
   {
      bSkipToNextRow = false;

      pgsPointOfInterest& poi = (*siter).first;
      pgsTypes::Stage stage = (*siter).second;

      if ( row1 != 1 && IsEqual(prev_poi.GetDistFromStart(),poi.GetDistFromStart()) )
      {
         row1--;
         bSkipToNextRow = true;
      }

      ReportLocation2(pES,  row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pFR,  row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPTT, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPTP, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPSH, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPTH, row1,stage,poi,end_size,pDisplayUnits);

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportLocation(pPTR,row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pT,  row2,stage,poi,end_size,pDisplayUnits);
      }

      if ( !bSkipToNextRow )
      {
         details = pILosses->GetLossDetails( poi );

         ReportRow(pES, pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pFR, pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPTT,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPTP,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPSH,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPTH,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
      }

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportRow(pPTR,pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pT,  pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         row2++;
      }
      
      row1++;
      prev_poi = poi;
   }
}

void CPsLossEngineer::ReportApproxMethod2005(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   rptParagraph* pParagraph;

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE(IGirderData,pGirderData);
   const CGirderData* pgirderData = pGirderData->GetGirderData(span,gdr);

   GET_IFACE(IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength( span,gdr );

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Temporary);

   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> bsPoi = pIPoi->GetPointsOfInterest( span, gdr, pgsTypes::BridgeSite3, POI_TABULAR );
   std::vector<pgsPointOfInterest> cyPoi = pIPoi->GetPointsOfInterest( span, gdr, pgsTypes::CastingYard, POI_TABULAR | POI_PICKPOINT, POIFIND_OR );
   std::vector<pgsPointOfInterest>::iterator iter;

   std::set< std::pair<pgsPointOfInterest,pgsTypes::Stage> > sPoi;
   for ( iter = bsPoi.begin(); iter != bsPoi.end(); iter++ )
   {
      sPoi.insert( std::make_pair(*iter,pgsTypes::BridgeSite3) );
   }

   for ( iter = cyPoi.begin(); iter != cyPoi.end(); iter++ )
   {
      sPoi.insert( std::make_pair(*iter,pgsTypes::CastingYard) );
   }

   GET_IFACE(ILosses,pILosses);
   LOSSDETAILS details = pILosses->GetLossDetails( cyPoi[0] );

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Approximate Estimate of Time-Dependent Losses [5.9.5.3]") << rptNewLine;

   bool bTemporaryStrands = ( 0 < Nt && pgirderData->PrestressData.TempStrandUsage == pgsTypes::ttsPretensioned ? true : false);

   //////////////////////////////////////////////////////////
   // NOTE: The order of everything from here to the loop is important. 
   // It is the order in which things are put into the report
   ReportInitialRelaxation(pChapter,bTemporaryStrands,details.pLosses.get(),pDisplayUnits,level);

   CElasticShorteningTable*                        pES  = CElasticShorteningTable::PrepareTable(pChapter,m_pBroker,span,gdr,bTemporaryStrands,
                                                                                                details,pDisplayUnits,level);

   ReportLumpSumTimeDependentLossesAtShipping(pChapter,details,pDisplayUnits,level);

   CFrictionLossTable*                             pFR  = NULL;
   CPostTensionInteractionTable*                   pPTT = NULL;
   CEffectOfPostTensionedTemporaryStrandsTable*    pPTP = NULL;

   if ( 0 < Nt && pgirderData->PrestressData.TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   }

   CTimeDependentLossesAtShippingTable*            pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,span,gdr,bTemporaryStrands,pDisplayUnits,level);
   CPostTensionTimeDependentLossesAtShippingTable* pPTH = NULL;

   if ( 0 < Nt )
      pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);

   CTemporaryStrandRemovalTable*                   pPTR = NULL;
   if ( 0 < Nt )
   {
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   }

   ReportLumpSumTimeDependentLosses(pChapter,details,pDisplayUnits,level);

   CTotalPrestressLossTable* pT = CTotalPrestressLossTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);

   CElasticGainDueToDeckPlacementTable*   pED   = CElasticGainDueToDeckPlacementTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   CElasticGainDueToSIDLTable*            pSIDL = CElasticGainDueToSIDLTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   CElasticGainDueToLiveLoadTable*        pLLIM = CElasticGainDueToLiveLoadTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,level);
   CEffectivePrestressTable*              pPE   = CEffectivePrestressTable::PrepareTable(pChapter,m_pBroker,span,gdr,details,pDisplayUnits,level);

   PoiSet::iterator siter;

   pgsPointOfInterest prev_poi(0,0,0);
   bool bSkipToNextRow = false;
   RowIndexType row1 = 1;
   RowIndexType row2 = 1;
   // **** NOTE **** Use 2 row counters.. one for the table with CY and BS poi, and a different one for BS poi only
   for ( siter = sPoi.begin(); siter != sPoi.end(); siter++ )
   {
      bSkipToNextRow = false;

      pgsPointOfInterest& poi = (*siter).first;
      pgsTypes::Stage stage = (*siter).second;

      if ( row1 != 1 && IsEqual(prev_poi.GetDistFromStart(),poi.GetDistFromStart()) )
      {
         row1--;
         bSkipToNextRow = true;
      }

      ReportLocation2(pES,  row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pFR,  row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPTT, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPTP, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPSH, row1,stage,poi,end_size,pDisplayUnits);
      ReportLocation2(pPTH, row1,stage,poi,end_size,pDisplayUnits);

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportLocation(pPTR, row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pT,   row2,stage,poi,end_size,pDisplayUnits);

         ReportLocation(pED,  row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pSIDL,row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pLLIM,row2,stage,poi,end_size,pDisplayUnits);
         ReportLocation(pPE,  row2,stage,poi,end_size,pDisplayUnits);
      }

      if ( !bSkipToNextRow )
      {
         details = pILosses->GetLossDetails( poi );

         ReportRow(pES, pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pFR, pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPTT,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPTP,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPSH,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
         ReportRow(pPTH,pChapter,m_pBroker,poi,row1,details,pDisplayUnits,level);
      }

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportRow(pPTR,  pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pT,    pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);

         ReportRow(pED,   pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pSIDL, pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pLLIM, pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         ReportRow(pPE,   pChapter,m_pBroker,poi,row2,details,pDisplayUnits,level);
         row2++;
      }
      
      row1++;
      prev_poi = poi;
   }
}


void CPsLossEngineer::ReportLumpSumMethod(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,bool bDesign,Uint16 level)
{
   rptParagraph* pParagraph;

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(span,gdr,pgsTypes::Temporary);

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
   std::vector<pgsPointOfInterest> cyPoi = pIPoi->GetPointsOfInterest( span, gdr, pgsTypes::CastingYard, POI_TABULAR );

   GET_IFACE(ILosses,pILosses);
   LOSSDETAILS details = pILosses->GetLossDetails( cyPoi[0] );

   // Typecast to our known type (eating own doggy food)
   boost::shared_ptr<const lrfdLumpSumLosses> ptl = boost::dynamic_pointer_cast<const lrfdLumpSumLosses>(details.pLosses);
   if (!ptl)
   {
      ATLASSERT(0); // made a bad cast? Bail...
      return;
   }

   int row = 0;

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

void CPsLossEngineer::ReportLocation2(rptRcTable* pTable,RowIndexType row,pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 endsize,IEAFDisplayUnits* pDisplayUnits)
{
   if ( pTable == NULL )
      return;

   INIT_UV_PROTOTYPE( rptPointOfInterest,  spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptPointOfInterest,  gdrloc,      pDisplayUnits->GetSpanLengthUnit(),      false );

   RowIndexType rowOffset = pTable->GetNumberOfHeaderRows() - 1;

   if ( stage == pgsTypes::CastingYard )
   {
      (*pTable)(row+rowOffset,0) << gdrloc.SetValue( stage, poi );
      (*pTable)(row+rowOffset,1) << _T("");
   }
   else
   {
      (*pTable)(row+rowOffset,0) << _T("");
      (*pTable)(row+rowOffset,1) << spanloc.SetValue( stage, poi, endsize );
   }
}

void CPsLossEngineer::ReportLocation(rptRcTable* pTable,RowIndexType row,pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 endsize,IEAFDisplayUnits* pDisplayUnits)
{
   if ( pTable == NULL )
      return;

   INIT_UV_PROTOTYPE( rptPointOfInterest,  spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );

   RowIndexType rowOffset = pTable->GetNumberOfHeaderRows() - 1;
   (*pTable)(row+rowOffset,0) << spanloc.SetValue( stage, poi, endsize );
}

void CPsLossEngineer::ReportLumpSumTimeDependentLossesAtShipping(rptChapter* pChapter,const LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   // Lump Sum Loss at time of shipping
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Approximate Lump Sum Estimate of Time Dependent Losses at Shipping") << rptNewLine;

   if ( details.Method == LOSSES_AASHTO_LUMPSUM || details.Method == LOSSES_WSDOT_LUMPSUM )
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
      *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pLTH")) << _T(" = ") << stress.SetValue(details.pLosses->PermanentStrand_TimeDependentLossesAtShipping()) << rptNewLine;
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
      boost::shared_ptr<const lrfdApproximateLosses2005> ptl = boost::dynamic_pointer_cast<const lrfdApproximateLosses2005>(details.pLosses);
      if (!ptl)
      {
         ATLASSERT(0); // made a bad cast? Bail...
         return;
      }

      (*table)(1,0) << stress.SetValue( details.pLosses->GetFci() );
      (*table)(1,1) << scalar.SetValue( ptl->GetStrengthFactor() );
      (*table)(1,2) << details.pLosses->GetRelHumidity();
      (*table)(1,3) << scalar.SetValue( ptl->GetHumidityFactor() );
      (*table)(1,4) << stress.SetValue( ptl->GetFpi() );
      (*table)(1,5) << area.SetValue( details.pLosses->GetApsPermanent() );
      (*table)(1,6) << area.SetValue( details.pLosses->GetAg() );
      (*table)(1,7) << stress.SetValue( ptl->PermanentStrand_RelaxationLossesAtXfer() );
      (*table)(1,8) << stress.SetValue( details.pLosses->PermanentStrand_TimeDependentLossesAtShipping() );
   }
}

void CPsLossEngineer::ReportLumpSumTimeDependentLosses(rptChapter* pChapter,const LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Approximate Lump Sum Estimate of Time Dependent Losses") << rptNewLine;

   if ( details.Method == LOSSES_AASHTO_LUMPSUM || details.Method == LOSSES_WSDOT_LUMPSUM )
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

      int method = (details.Method == LOSSES_WSDOT_LUMPSUM) ? 1 : 0;

      // Typecast to our known type (eating own doggy food)
      boost::shared_ptr<const lrfdApproximateLosses> ptl = boost::dynamic_pointer_cast<const lrfdApproximateLosses>(details.pLosses);
      if (!ptl)
      {
         ATLASSERT(0); // made a bad cast? Bail...
         return;
      }

      int beam = (int)ptl->GetBeamType();
      int strand = details.pLosses->GetStrandType() == matPsStrand::LowRelaxation ? 0 : 1;
      int units = IS_SI_UNITS(pDisplayUnits);

      
      pParagraph = new rptParagraph;
      *pChapter << pParagraph;
      *pParagraph<< rptRcImage(strImagePath + strLossEqnImage[method][beam][strand][units]) << rptNewLine;

      INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDisplayUnits->GetStressUnit(),          true );
      *pParagraph << RPT_FC << _T(" = ") << stress.SetValue(details.pLosses->GetFc() ) << rptNewLine;
      *pParagraph << _T("PPR = ") << ptl->GetPPR() << rptNewLine;
      *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pLT")) << _T(" = ") << stress.SetValue( details.pLosses->TimeDependentLosses() ) << rptNewLine;
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
      boost::shared_ptr<const lrfdApproximateLosses2005> ptl = boost::dynamic_pointer_cast<const lrfdApproximateLosses2005>(details.pLosses);
      if (!ptl)
      {
         ATLASSERT(0); // made a bad cast? Bail...
         return;
      }

      (*table)(1,0) << stress.SetValue( details.pLosses->GetFci() );
      (*table)(1,1) << scalar.SetValue( ptl->GetStrengthFactor() );
      (*table)(1,2) << details.pLosses->GetRelHumidity();
      (*table)(1,3) << scalar.SetValue( ptl->GetHumidityFactor() );
      (*table)(1,4) << stress.SetValue( ptl->GetFpi() );
      (*table)(1,5) << area.SetValue( details.pLosses->GetApsPermanent() );
      (*table)(1,6) << area.SetValue( details.pLosses->GetAg() );
      (*table)(1,7) << stress.SetValue( ptl->PermanentStrand_RelaxationLossesAtXfer() );
      (*table)(1,8) << stress.SetValue( details.pLosses->TimeDependentLosses() );
   }
}

void CPsLossEngineer::GetLossParameters(const pgsPointOfInterest& poi,const GDRCONFIG& config,
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
   Float64* pVolume,
   Float64* pSurfaceArea,
   Float64* pAd,
   Float64* ped,
   Float64* pKsh,
   Float64* peperm,// eccentricity of the permanent strands on the non-composite section
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
   GET_IFACE( IBridge,          pBridge );
   GET_IFACE( IStrandGeometry,  pStrandGeom );
   GET_IFACE( IGirderData,      pGirderData );
   GET_IFACE( ISectProp2,       pSectProp2 );
   GET_IFACE( IProductForces,   pProdForces );
   GET_IFACE( IEnvironment,     pEnv );
   GET_IFACE( IPointOfInterest, pIPOI );
   GET_IFACE( IBridgeMaterialEx,pMaterial );
   GET_IFACE( ISpecification,   pSpec);
   GET_IFACE( IBridgeDescription,pIBridgeDesc);
   GET_IFACE( ILibrary,         pLibrary);
   GET_IFACE( ILoadFactors,     pILoadFactors );
   GET_IFACE( IGirder,          pGirder);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   *pPjS = config.PrestressConfig.Pjack[pgsTypes::Straight];
   *pNs = config.PrestressConfig.GetNStrands(pgsTypes::Straight);
   
   *pPjH = config.PrestressConfig.Pjack[pgsTypes::Harped];
   *pNh = config.PrestressConfig.GetNStrands(pgsTypes::Harped);
   
   *pPjT = config.PrestressConfig.Pjack[pgsTypes::Temporary];
   *pNt  = config.PrestressConfig.GetNStrands(pgsTypes::Temporary);

   const matPsStrand* pstrand = pGirderData->GetStrandMaterial(span,gdr,pgsTypes::Straight);
   CHECK(pstrand);
   *pGrade = pstrand->GetGrade();
   *pType  = pstrand->GetType();
   *paps   = pstrand->GetNominalArea();
   Float64 Aps[3];
   Aps[pgsTypes::Straight] = *paps;
   Aps[pgsTypes::Harped]   = pGirderData->GetStrandMaterial(span,gdr,pgsTypes::Harped)->GetNominalArea();
   Aps[pgsTypes::Temporary]= pGirderData->GetStrandMaterial(span,gdr,pgsTypes::Temporary)->GetNominalArea();

   *pGirderLength = pBridge->GetGirderLength( span, gdr);
   *pSpanLength   = pBridge->GetSpanLength( span, gdr );
   *pEndSize      = pBridge->GetGirderStartConnectionLength( span, gdr );

   Float64 nStrandsEffective;
   
   // eccentricity of the permanent strands
   *peperm = pStrandGeom->GetEccentricity( poi, config.PrestressConfig, false, &nStrandsEffective);

   // eccentricity of the temporary strands
   *petemp = pStrandGeom->GetTempEccentricity( poi, config.PrestressConfig, &nStrandsEffective);

   *pPerimeter = pSectProp2->GetPerimeter(poi);
   *pAg  = pSectProp2->GetAg( pgsTypes::BridgeSite1, poi );
   *pIg  = pSectProp2->GetIx( pgsTypes::BridgeSite1, poi );
   *pYbg = pSectProp2->GetYb( pgsTypes::BridgeSite1, poi );
   *pAc  = pSectProp2->GetAg( pgsTypes::BridgeSite2, poi, config.Fc );
   *pIc  = pSectProp2->GetIx( pgsTypes::BridgeSite2, poi, config.Fc );
   *pYbc = pSectProp2->GetYb( pgsTypes::BridgeSite2, poi, config.Fc );

   *pVolume = pSectProp2->GetVolume(span,gdr);
   *pSurfaceArea = pSectProp2->GetSurfaceArea(span,gdr);

   // area of deck
   if ( pBridge->IsCompositeDeck() )
       *pAd  = pSectProp2->GetGrossDeckArea( poi );
   else
       *pAd = 0.0;

   // eccentricity of deck
   *ped  = pSectProp2->GetYtGirder( pgsTypes::BridgeSite2, poi, config.Fc ) 
         + pBridge->GetStructuralSlabDepth(poi)/2;
   *ped *= -1;

   *pApsPerm = Aps[pgsTypes::Straight]*(*pNs) + Aps[pgsTypes::Harped]*(*pNh);
   *pApsTTS  = Aps[pgsTypes::Temporary]*(*pNt);

   *pGdrCreepK1 = pMaterial->GetCreepK1Gdr(span,gdr);
   *pGdrCreepK2 = pMaterial->GetCreepK2Gdr(span,gdr);
   *pGdrShrinkageK1 = pMaterial->GetShrinkageK1Gdr(span,gdr);
   *pGdrShrinkageK2 = pMaterial->GetShrinkageK2Gdr(span,gdr);
   *pDeckCreepK1 = pMaterial->GetCreepK1Slab();
   *pDeckCreepK2 = pMaterial->GetCreepK2Slab();
   *pDeckShrinkageK1 = pMaterial->GetShrinkageK1Slab();
   *pDeckShrinkageK2 = pMaterial->GetShrinkageK2Slab();


   *pFci    = config.Fci;
   *pFc     = config.Fc;
   *pFcSlab = pMaterial->GetFcSlab();

   if ( config.bUserEci )
      *pEci = config.Eci;
   else
      *pEci = pMaterial->GetEconc(config.Fci,pMaterial->GetStrDensityGdr(span,gdr),pMaterial->GetEccK1Gdr(span,gdr),pMaterial->GetEccK2Gdr(span,gdr));

   if( config.bUserEc )
      *pEc = config.Ec;
   else
      *pEc  = pMaterial->GetEconc(config.Fc, pMaterial->GetStrDensityGdr(span,gdr),pMaterial->GetEccK1Gdr(span,gdr),pMaterial->GetEccK2Gdr(span,gdr));

   if (pDeck->SlabUserEc)
      *pEcSlab = pDeck->SlabEc;
   else
      *pEcSlab = pMaterial->GetEcSlab(); 

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

   *pMdlg  = pProdForces->GetMoment( pgsTypes::CastingYard, pftGirder,    poi, SimpleSpan);

   // determine the bridge analysis type based on the analysis type setting 
   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple     ? SimpleSpan : 
                             analysisType == pgsTypes::Continuous ? ContinuousSpan : MaxSimpleContinuousEnvelope);

   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());

   // effectiveness of deck shrinkage
   *pKsh = pSpecEntry->GetDeckShrinkageElasticGain();

   Float64 K_slab = pSpecEntry->GetSlabElasticGain();
   Float64 K_slabpad = pSpecEntry->GetSlabPadElasticGain();
   Float64 K_dia = pSpecEntry->GetDiaphragmElasticGain();
   Float64 K_userdc1 = pSpecEntry->GetUserDCElasticGain(pgsTypes::BridgeSite1);
   Float64 K_userdw1 = pSpecEntry->GetUserDWElasticGain(pgsTypes::BridgeSite1);

   if ( poi.GetDistFromStart() < *pEndSize || *pEndSize + *pSpanLength < poi.GetDistFromStart() )
   {
      *pMadlg = 0;
   }
   else
   {
      *pMadlg = K_slab    * pProdForces->GetMoment( pgsTypes::BridgeSite1, pftSlab,      poi, bat ) +
                K_slabpad * pProdForces->GetMoment( pgsTypes::BridgeSite1, pftSlabPad,   poi, bat ) + 
                K_dia     *(pProdForces->GetMoment( pgsTypes::BridgeSite1, pftDiaphragm, poi, bat ) + 
                            pProdForces->GetMoment( pgsTypes::BridgeSite1, pftShearKey,  poi, bat )) + 
                K_userdc1 * pProdForces->GetMoment( pgsTypes::BridgeSite1, pftUserDC,    poi, bat ) +
                K_userdw1 * pProdForces->GetMoment( pgsTypes::BridgeSite1, pftUserDW,    poi, bat );

      if ( pDeck->DeckType == pgsTypes::sdtCompositeSIP )
      {
         *pMadlg += K_slab * pProdForces->GetMoment( pgsTypes::BridgeSite1, pftSlabPanel, poi, bat );
      }
   }

   if ( m_bComputingLossesForDesign )
   {
      // get the additional moment caused by the difference in input and design "A" dimension
      Float64 Mslab = pProdForces->GetDesignSlabMomentAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi);
      *pMadlg += K_slab*Mslab;

      Float64 Mslabpad = pProdForces->GetDesignSlabPadMomentAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi);
      *pMadlg += K_slabpad*Mslabpad;
   }

   Float64 K_railing = pSpecEntry->GetRailingSystemElasticGain();
   Float64 K_userdc2 = pSpecEntry->GetUserDCElasticGain(pgsTypes::BridgeSite2);
   Float64 K_userdw2 = pSpecEntry->GetUserDWElasticGain(pgsTypes::BridgeSite2);
   Float64 K_overlay = pSpecEntry->GetOverlayElasticGain();
   Float64 K_liveload = pSpecEntry->GetLiveLoadElasticGain();
   if ( poi.GetDistFromStart() < *pEndSize || *pEndSize + *pSpanLength < poi.GetDistFromStart() )
   {
      *pMsidl = 0;
      *pMllim = 0;
   }
   else
   {
      *pMsidl = K_railing * (pProdForces->GetMoment( pgsTypes::BridgeSite2, pftTrafficBarrier, poi, bat ) +
                             pProdForces->GetMoment( pgsTypes::BridgeSite2, pftSidewalk,       poi, bat )) +
                K_userdc2 *  pProdForces->GetMoment( pgsTypes::BridgeSite2, pftUserDC,         poi, bat ) +
                K_userdw2 *  pProdForces->GetMoment( pgsTypes::BridgeSite2, pftUserDW,         poi, bat );

      // include the overlay dead load. even future overlays contribute dead load and creep effects
      // from the time it is installed until final. we don't know when "future" is and it is conservative
      // to assume it is applied at a time when it contributes to the creep loss.
      // See PCI BDM Example 9.1a that supports this approach
      if ( pBridge->HasOverlay() )
      {
         *pMsidl += K_overlay*pProdForces->GetMoment( pgsTypes::BridgeSite2, pftOverlay, poi, bat );
      }

      
      Float64 Mmin, Mmax;
      pProdForces->GetLiveLoadMoment(pgsTypes::lltDesign,pgsTypes::BridgeSite3,poi,bat,true,true,&Mmin,&Mmax);

      Float64 gMaxI   = pILoadFactors->GetLoadFactors()->LLIMmax[pgsTypes::ServiceI];
      Float64 gMaxIII = pILoadFactors->GetLoadFactors()->LLIMmax[pgsTypes::ServiceIII];
      *pMllim = K_liveload*_cpp_max(gMaxI,gMaxIII)*Mmax;
   }



   *prh = pEnv->GetRelHumidity();

   // get time to prestress transfer
   *pti = pSpecEntry->GetXferTime();
   *pth = pSpecEntry->GetShippingTime();
   *ptd = pSpecEntry->GetCreepDuration2Max();
   *ptf = pSpecEntry->GetTotalCreepDuration();

   *pAslab = pSectProp2->GetTributaryDeckArea(poi);

   Float64 wTop = 0;
   FlangeIndexType nFlanges = pGirder->GetNumberOfTopFlanges(span,gdr);
   for ( FlangeIndexType flangeIdx = 0; flangeIdx < nFlanges; flangeIdx++ )
   {
      Float64 wtf = pGirder->GetTopFlangeWidth(poi,flangeIdx);
      wTop += wtf;
   }
   *pPslab = 2*pSectProp2->GetTributaryFlangeWidth(poi) - wTop;

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

   *pAnchorSet = pSpecEntry->GetAnchorSet();
   *pWobble = pSpecEntry->GetWobbleFrictionCoefficient();
   *pCoeffFriction = pSpecEntry->GetFrictionCoefficient();
   *pAngleChange = 0;
}

void CPsLossEngineer::ReportFinalLosses(BeamType beamType,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE(ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   Uint16 level = 0;
   switch( pSpecEntry->GetLossMethod() )
   {
   case LOSSES_AASHTO_REFINED:
   case LOSSES_AASHTO_LUMPSUM:
   case LOSSES_TXDOT_REFINED_2013:
      ReportFinalLossesRefinedMethod(pChapter,beamType,span,gdr,pDisplayUnits,laAASHTO);
      break;

   case LOSSES_WSDOT_REFINED:
   case LOSSES_WSDOT_LUMPSUM:
      ReportFinalLossesRefinedMethod(pChapter,beamType,span,gdr,pDisplayUnits,laWSDOT);
      break;

   case LOSSES_TXDOT_REFINED_2004:
      ReportFinalLossesRefinedMethod(pChapter,beamType,span,gdr,pDisplayUnits,laTxDOT);
      break;

   case LOSSES_GENERAL_LUMPSUM:
      ReportGeneralLumpSumMethod(beamType,span,gdr,pChapter,pDisplayUnits,false,0);
      break;

   default:
      CHECK(false); // Should never get here
   }
}


void CPsLossEngineer::ReportFinalLossesRefinedMethod(rptChapter* pChapter,BeamType beamType,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,LossAgency lossAgency)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 ||
        lossAgency==laTxDOT)
   {
      ReportFinalLossesRefinedMethodBefore2005(pChapter,beamType,span,gdr,pDisplayUnits);
   }
   else
   {
      ReportFinalLossesRefinedMethod(pChapter,beamType,span,gdr,pDisplayUnits);
   }
}

void CPsLossEngineer::ReportFinalLossesRefinedMethod(rptChapter* pChapter,BeamType beamType,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* pParagraph;

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeMaterial,pMaterial);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   Float64 end_size = pBridge->GetGirderStartConnectionLength( span,gdr );

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Final Prestress Losses") << rptNewLine;

   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> bsPoi = pIPoi->GetPointsOfInterest( span, gdr, pgsTypes::BridgeSite3, POI_TABULAR );
   std::vector<pgsPointOfInterest>::iterator iter;

   GET_IFACE(ILosses,pILosses);
   LOSSDETAILS details = pILosses->GetLossDetails( bsPoi[0] );

   CTotalPrestressLossTable* pT = CTotalPrestressLossTable::PrepareTable(pChapter,m_pBroker,span,gdr,details, pDisplayUnits,0);

   ///////////////////////////////////////////////////////////////////////
   // Loop over all the POIs and populate the tables with loss information
   RowIndexType row = 1;
   for ( iter = bsPoi.begin(); iter != bsPoi.end(); iter++ )
   {
      pgsPointOfInterest& poi = *iter;
      pgsTypes::Stage stage = pgsTypes::BridgeSite3;

      LOSSDETAILS details = pILosses->GetLossDetails( poi );

      ReportLocation(pT,row,stage,poi,end_size,pDisplayUnits);

      // fill each row1 with data
      ReportRow(pT,pChapter,m_pBroker,poi,row,details,pDisplayUnits,0);
      row++;
   }
}

void CPsLossEngineer::ReportFinalLossesRefinedMethodBefore2005(rptChapter* pChapter,CPsLossEngineer::BeamType beamType,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Refined Estimate of Time-Dependent Losses [5.9.5.4]") << rptNewLine;

   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> bsPoi = pIPoi->GetPointsOfInterest( span, gdr, pgsTypes::BridgeSite3, POI_TABULAR );
   std::vector<pgsPointOfInterest>::iterator iter;


   GET_IFACE(ILosses,pILosses);

   GET_IFACE(IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength( span,gdr );

   CFinalPrestressLossTable* pT = CFinalPrestressLossTable::PrepareTable(pChapter,m_pBroker,span,gdr,pDisplayUnits,0);

   RowIndexType row = 1;
   for ( iter = bsPoi.begin(); iter != bsPoi.end(); iter++ )
   {
      pgsPointOfInterest& poi = *iter;
      pgsTypes::Stage stage = pgsTypes::BridgeSite3;

      ReportLocation(pT,row,stage,poi,end_size,pDisplayUnits);
      LOSSDETAILS details = pILosses->GetLossDetails( poi );

      ReportRow(pT, pChapter,m_pBroker,poi,row,details,pDisplayUnits,0);

      row++;
   }
}
